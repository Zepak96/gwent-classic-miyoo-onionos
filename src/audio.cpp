#include "gwent/audio.hpp"

#define DR_MP3_IMPLEMENTATION
#include "../third_party/dr_mp3.h"

#include <algorithm>
#include <cstring>
#include <limits>

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

const AudioSystem::Sample* AudioSystem::loadMusic(const std::string& key) {
  const auto cached=musicSamples_.find(key);
  if(cached!=musicSamples_.end())return &cached->second;
  if(failedMusic_.find(key)!=failedMusic_.end())return nullptr;
  const auto path=musicDirectory_/(key+".mp3");
  drmp3_config source{};
  drmp3_uint64 frames=0;
  drmp3_int16* decoded=drmp3_open_file_and_read_pcm_frames_s16(path.string().c_str(),&source,&frames,nullptr);
  if(!decoded||frames==0||source.channels==0||source.sampleRate==0) {
    if(decoded)drmp3_free(decoded,nullptr);
    failedMusic_.insert(key);
    return nullptr;
  }
  const std::uint64_t sourceBytes=frames*source.channels*sizeof(drmp3_int16);
  if(sourceBytes>static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
    drmp3_free(decoded,nullptr);failedMusic_.insert(key);return nullptr;
  }
  SDL_AudioCVT converter{};
  const int conversion=SDL_BuildAudioCVT(&converter,AUDIO_S16SYS,static_cast<Uint8>(source.channels),static_cast<int>(source.sampleRate),
                                         output_.format,output_.channels,output_.freq);
  if(conversion<0) { drmp3_free(decoded,nullptr);failedMusic_.insert(key);return nullptr; }
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
      drmp3_free(decoded,nullptr);failedMusic_.insert(key);return nullptr;
    }
    sample.pcm.resize(static_cast<std::size_t>(converter.len_cvt));
  }
  drmp3_free(decoded,nullptr);
  const auto inserted=musicSamples_.emplace(key,std::move(sample));
  return &inserted.first->second;
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
    const auto* sample=loadMusic(key);
    if(!sample||sample->pcm.empty())failures.emplace_back(key);
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
  const auto* sample=loadMusic(key);
  if(!sample||sample->pcm.empty())return;
  SDL_LockAudioDevice(device_);
  musicVoice_={sample,0,true};musicKey_=key;
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
  if(!musicEnabled_)musicVoice_={};
  musicKey_.clear();
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
  if(self.musicEnabled_)mixVoice(self.musicVoice_,SDL_MIX_MAXVOLUME/2);
  if(self.enabled_)for(auto& voice:self.voices_)mixVoice(voice,SDL_MIX_MAXVOLUME/3);
}

void AudioSystem::shutdown() {
  if(device_) { SDL_CloseAudioDevice(device_);device_=0; }
  voices_={};musicVoice_={};musicKey_.clear();samples_.clear();musicSamples_.clear();failed_.clear();failedMusic_.clear();
}
}
