#include "gwent/audio.hpp"

#define DR_MP3_IMPLEMENTATION
#include "../third_party/dr_mp3.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace gwent_port {
bool AudioSystem::initialize(const std::filesystem::path& assetDirectory) {
  if(device_)return true;
  sfxDirectory_=assetDirectory/"sfx";
  musicDirectory_=assetDirectory/"music";
  SDL_AudioSpec requested{};
  requested.freq=22050;
  requested.format=AUDIO_S16SYS;
  requested.channels=1;
  requested.samples=1024;
  requested.callback=&AudioSystem::callback;
  requested.userdata=this;
  device_=SDL_OpenAudioDevice(nullptr,0,&requested,&output_,0);
  if(!device_)return false;
  musicStop_.store(false);
  musicStats_={};musicStats_.capacityBytes=musicCapacity_;
  musicThread_=SDL_CreateThread(&AudioSystem::musicWorker,"gwent-music",this);
  if(!musicThread_) { SDL_CloseAudioDevice(device_);device_=0;return false; }
  SDL_PauseAudioDevice(device_,0);
  return true;
}

const AudioSystem::Sample* AudioSystem::load(const std::string& key) {
  const auto cached=samples_.find(key);
  if(cached!=samples_.end())return &cached->second;
  if(failed_.find(key)!=failed_.end())return nullptr;
  const auto path=sfxDirectory_/(key+".mp3");
  drmp3_config source{};
  drmp3_uint64 frames=0;
  drmp3_int16* decoded=drmp3_open_file_and_read_pcm_frames_s16(path.string().c_str(),&source,&frames,nullptr);
  if(!decoded||frames==0||source.channels==0||source.sampleRate==0) {
    if(decoded)drmp3_free(decoded,nullptr);
    failed_.insert(key);
    return nullptr;
  }
  const std::uint64_t sourceBytes=frames*source.channels*sizeof(drmp3_int16);
  if(sourceBytes>static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    drmp3_free(decoded,nullptr);failed_.insert(key);return nullptr;
  }
  SDL_AudioCVT converter{};
  const int conversion=SDL_BuildAudioCVT(&converter,AUDIO_S16SYS,static_cast<Uint8>(source.channels),static_cast<int>(source.sampleRate),
                                         output_.format,output_.channels,output_.freq);
  if(conversion<0) { drmp3_free(decoded,nullptr);failed_.insert(key);return nullptr; }
  Sample sample;
  const int inputBytes=static_cast<int>(sourceBytes);
  if(conversion==0) {
    sample.pcm.resize(static_cast<std::size_t>(inputBytes));
    std::memcpy(sample.pcm.data(),decoded,static_cast<std::size_t>(inputBytes));
  } else {
    sample.pcm.resize(static_cast<std::size_t>(inputBytes)*static_cast<std::size_t>(converter.len_mult));
    std::memcpy(sample.pcm.data(),decoded,static_cast<std::size_t>(inputBytes));
    converter.buf=sample.pcm.data();converter.len=inputBytes;
    if(SDL_ConvertAudio(&converter)!=0) {
      drmp3_free(decoded,nullptr);failed_.insert(key);return nullptr;
    }
    sample.pcm.resize(static_cast<std::size_t>(converter.len_cvt));
  }
  drmp3_free(decoded,nullptr);
  const auto inserted=samples_.emplace(key,std::move(sample));
  return &inserted.first->second;
}

int AudioSystem::musicWorker(void* userdata) {
  try { return static_cast<AudioSystem*>(userdata)->runMusicWorker(); }
  catch(...) {
    auto& self=*static_cast<AudioSystem*>(userdata);
    SDL_LockAudioDevice(self.device_);++self.musicStats_.failures;SDL_UnlockAudioDevice(self.device_);
    return -1;
  }
}

int AudioSystem::runMusicWorker() {
  struct Resources {
    drmp3 decoder{};bool opened{false};SDL_AudioStream* converter{nullptr};
    void clear() { if(converter)SDL_FreeAudioStream(converter);converter=nullptr;
      if(opened)drmp3_uninit(&decoder);opened=false; }
    ~Resources(){clear();}
  } resource;
  std::array<drmp3_int16,2048> decoded{};
  std::array<Uint8,8192> converted{};
  std::uint64_t generation=~std::uint64_t{0};
  bool failed=false, hadFrames=false;
  while(!musicStop_.load()) {
    SDL_LockAudioDevice(device_);
    const auto requestedGeneration=musicGeneration_;
    const bool active=musicEnabled_&&!musicKey_.empty();
    std::string key;
    try { if(requestedGeneration!=generation)key=musicKey_; }
    catch(...) { SDL_UnlockAudioDevice(device_);throw; }
    const auto freeBytes=musicCapacity_-musicSize_;
    SDL_UnlockAudioDevice(device_);
    if(requestedGeneration!=generation) {
      resource.clear();generation=requestedGeneration;failed=false;hadFrames=false;
      if(active) {
        const auto path=musicDirectory_/(key+".mp3");
        resource.opened=drmp3_init_file(&resource.decoder,path.string().c_str(),nullptr)!=0;
        if(resource.opened&&resource.decoder.channels>=1&&resource.decoder.channels<=2&&
           resource.decoder.sampleRate>=8000&&resource.decoder.sampleRate<=192000)
          resource.converter=SDL_NewAudioStream(AUDIO_S16SYS,static_cast<Uint8>(resource.decoder.channels),
            static_cast<int>(resource.decoder.sampleRate),output_.format,output_.channels,output_.freq);
        if(!resource.converter)failed=true;
        if(failed) { SDL_LockAudioDevice(device_);++musicStats_.failures;SDL_UnlockAudioDevice(device_); }
      }
    }
    if(!active||failed||!resource.converter||freeBytes<converted.size()) { SDL_Delay(2);continue; }
    int available=SDL_AudioStreamAvailable(resource.converter);
    if(available<0) {
      failed=true;SDL_LockAudioDevice(device_);++musicStats_.failures;SDL_UnlockAudioDevice(device_);
      continue;
    }
    if(available==0) {
      const auto frames=drmp3_read_pcm_frames_s16(&resource.decoder,1024,decoded.data());
      if(!frames) {
        if(!hadFrames||!drmp3_seek_to_pcm_frame(&resource.decoder,0))failed=true;
        else { SDL_LockAudioDevice(device_);++musicStats_.loops;SDL_UnlockAudioDevice(device_);hadFrames=false; }
      } else {
        hadFrames=true;
        if(SDL_AudioStreamPut(resource.converter,decoded.data(),static_cast<int>(frames*resource.decoder.channels*2))<0)failed=true;
      }
      if(failed) { SDL_LockAudioDevice(device_);++musicStats_.failures;SDL_UnlockAudioDevice(device_); }
      continue;
    }
    const int bytes=SDL_AudioStreamGet(resource.converter,converted.data(),static_cast<int>(converted.size()));
    if(bytes<0) { failed=true;SDL_LockAudioDevice(device_);++musicStats_.failures;SDL_UnlockAudioDevice(device_);continue; }
    SDL_LockAudioDevice(device_);
    if(generation==musicGeneration_&&musicEnabled_) {
      const auto count=static_cast<std::size_t>(bytes);
      const auto write=(musicRead_+musicSize_)%musicCapacity_;
      const auto first=std::min(count,musicCapacity_-write);
      std::memcpy(musicPcm_.data()+write,converted.data(),first);
      std::memcpy(musicPcm_.data(),converted.data()+first,count-first);
      musicSize_+=count;musicStats_.decodedBytes+=count;
      musicStats_.peakBufferedBytes=std::max(musicStats_.peakBufferedBytes,musicSize_);
    }
    SDL_UnlockAudioDevice(device_);
  }
  return 0;
}

AudioSystem::MusicStreamStats AudioSystem::musicStreamStats() {
  if(!device_)return musicStats_;
  SDL_LockAudioDevice(device_);auto result=musicStats_;result.bufferedBytes=musicSize_;SDL_UnlockAudioDevice(device_);
  return result;
}

bool AudioSystem::validateAllSfx(std::vector<std::string>& failures) {
  failures.clear();
  std::error_code error;
  std::size_t decoded=0;
  for(const auto& item:std::filesystem::directory_iterator(sfxDirectory_,error)) {
    if(error)break;
    if(item.path().extension()!=".mp3")continue;
    const auto key=item.path().stem().string();
    if(!load(key))failures.push_back(key);else ++decoded;
  }
  if(error)failures.push_back("sfx_directory");
  if(decoded!=35)failures.push_back("expected_35_mp3_files");
  return !error&&failures.empty();
}

bool AudioSystem::validateBackgroundMusic(std::vector<std::string>& failures) {
  failures.clear();
  for(const auto* key:{"menu","match"}) {
    drmp3 decoder{};
    const auto path=musicDirectory_/(std::string(key)+".mp3");
    if(!drmp3_init_file(&decoder,path.string().c_str(),nullptr)) { failures.emplace_back(key);continue; }
    bool valid=decoder.channels>=1&&decoder.channels<=2&&decoder.sampleRate>=8000&&decoder.sampleRate<=192000;
    std::array<drmp3_int16,2048> buffer{};std::uint64_t frames=0;
    if(valid)while(const auto read=drmp3_read_pcm_frames_s16(&decoder,1024,buffer.data()))frames+=read;
    drmp3_uninit(&decoder);
    if(!valid||!frames)failures.emplace_back(key);
  }
  return failures.empty();
}

void AudioSystem::play(const std::string& key) {
  if(!device_||!enabled_)return;
  const Sample* sample=load(key);
  if(!sample||sample->pcm.empty())return;
  SDL_LockAudioDevice(device_);
  const std::size_t first=nextVoice_;
  std::size_t slot=first;
  for(std::size_t i=0;i<voices_.size();++i) {
    const std::size_t candidate=(first+i)%voices_.size();
    if(!voices_[candidate].sample) { slot=candidate;break; }
  }
  voices_[slot]={sample,0,false};
  nextVoice_=(slot+1)%voices_.size();
  SDL_UnlockAudioDevice(device_);
}

void AudioSystem::playMusic(const std::string& key) {
  if(!device_||!musicEnabled_||key==musicKey_)return;
  SDL_LockAudioDevice(device_);
  musicKey_=key;++musicGeneration_;musicRead_=0;musicSize_=0;
  SDL_UnlockAudioDevice(device_);
}

void AudioSystem::setEnabled(bool enabled) {
  if(!device_) { enabled_=enabled;return; }
  SDL_LockAudioDevice(device_);
  enabled_=enabled;
  if(!enabled_)for(auto& voice:voices_)voice={};
  SDL_UnlockAudioDevice(device_);
}

void AudioSystem::setMusicEnabled(bool enabled) {
  if(!device_) { musicEnabled_=enabled;return; }
  SDL_LockAudioDevice(device_);
  musicEnabled_=enabled;
  musicKey_.clear();++musicGeneration_;musicRead_=0;musicSize_=0;
  SDL_UnlockAudioDevice(device_);
}

void AudioSystem::callback(void* userdata,Uint8* stream,int length) {
  auto& self=*static_cast<AudioSystem*>(userdata);
  std::memset(stream,0,static_cast<std::size_t>(length));
  if(length<=0)return;
  const auto mixVoice=[&](Voice& voice,int volume) {
    if(!voice.sample||voice.sample->pcm.empty())return;
    const std::size_t target=static_cast<std::size_t>(length)&~std::size_t{1};
    std::size_t written=0;
    while(written<target&&voice.sample) {
      if(voice.offset>=voice.sample->pcm.size()) {
        if(voice.loop)voice.offset=0;
        else {voice={};break;}
      }
      const std::size_t remaining=voice.sample->pcm.size()-voice.offset;
      const std::size_t bytes=std::min(remaining,target-written)&~std::size_t{1};
      if(!bytes)break;
      SDL_MixAudioFormat(stream+written,voice.sample->pcm.data()+voice.offset,self.output_.format,static_cast<Uint32>(bytes),static_cast<Uint8>(volume));
      voice.offset+=bytes;written+=bytes;
    }
  };
  if(self.musicEnabled_&&!self.musicKey_.empty()) {
    const auto count=std::min(static_cast<std::size_t>(length)&~std::size_t{1},self.musicSize_);
    const auto first=std::min(count,musicCapacity_-self.musicRead_);
    if(first)SDL_MixAudioFormat(stream,self.musicPcm_.data()+self.musicRead_,self.output_.format,static_cast<Uint32>(first),SDL_MIX_MAXVOLUME/2);
    if(count>first)SDL_MixAudioFormat(stream+first,self.musicPcm_.data(),self.output_.format,static_cast<Uint32>(count-first),SDL_MIX_MAXVOLUME/2);
    self.musicRead_=(self.musicRead_+count)%musicCapacity_;self.musicSize_-=count;
    if(count<static_cast<std::size_t>(length))++self.musicStats_.underruns;
  }
  if(self.enabled_)for(auto& voice:self.voices_)mixVoice(voice,SDL_MIX_MAXVOLUME/3);
}

void AudioSystem::shutdown() {
  musicStop_.store(true);
  if(musicThread_) { SDL_WaitThread(musicThread_,nullptr);musicThread_=nullptr; }
  if(device_) { SDL_CloseAudioDevice(device_);device_=0; }
  voices_={};musicKey_.clear();samples_.clear();failed_.clear();musicRead_=0;musicSize_=0;
}
}
