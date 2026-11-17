#pragma once

#include <SDL.h>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace gwent_port {
class AudioSystem {
public:
  AudioSystem() = default;
  ~AudioSystem() { shutdown(); }
  AudioSystem(const AudioSystem&) = delete;
  AudioSystem& operator=(const AudioSystem&) = delete;
  struct MusicStreamStats {
    std::size_t capacityBytes{0}, bufferedBytes{0}, peakBufferedBytes{0};
    std::uint64_t decodedBytes{0}, loops{0}, failures{0}, underruns{0};
  };
  MusicStreamStats musicStreamStats();
  bool initialize(const std::filesystem::path& assetDirectory);
  bool validateAllSfx(std::vector<std::string>& failures);
  bool validateBackgroundMusic(std::vector<std::string>& failures);
  void play(const std::string& key);
  void playMusic(const std::string& key);
  void setEnabled(bool enabled);
  void setMusicEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  bool musicEnabled() const { return musicEnabled_; }
  bool available() const { return device_ != 0; }
  void shutdown();

private:
  struct Sample { std::vector<Uint8> pcm; };
  struct Voice { const Sample* sample{nullptr}; std::size_t offset{0}; bool loop{false}; };
  static void callback(void* userdata, Uint8* stream, int length);
  const Sample* load(const std::string& key);
  static int musicWorker(void* userdata);
  int runMusicWorker();

  SDL_AudioDeviceID device_{0};
  SDL_AudioSpec output_{};
  std::filesystem::path sfxDirectory_;
  std::filesystem::path musicDirectory_;
  std::unordered_map<std::string, Sample> samples_;
  std::unordered_set<std::string> failed_;
  std::array<Voice, 12> voices_{};
  static constexpr std::size_t musicCapacity_=65536;
  std::array<Uint8,musicCapacity_> musicPcm_{};
  std::size_t musicRead_{0}, musicSize_{0};
  std::uint64_t musicGeneration_{0};
  MusicStreamStats musicStats_{};
  SDL_Thread* musicThread_{nullptr};
  std::atomic<bool> musicStop_{false};
  std::string musicKey_;
  std::size_t nextVoice_{0};
  bool enabled_{true};
  bool musicEnabled_{true};
};
}
