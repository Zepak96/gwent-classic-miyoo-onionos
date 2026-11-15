#pragma once

#include <SDL.h>
#include <array>
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
  bool initialize(const std::filesystem::path& assetDirectory);
  bool validateAllSfx(std::vector<std::string>& failures);
  bool validateBackgroundMusic(std::vector<std::string>& failures);
  void play(const std::string& key);
  void playMusic(const std::string& key);
  void setEnabled(bool enabled);
  bool enabled() const { return enabled_; }
  bool available() const { return device_ != 0; }
  void shutdown();

private:
  struct Sample { std::vector<Uint8> pcm; };
  struct Voice { const Sample* sample{nullptr}; std::size_t offset{0}; bool loop{false}; };
  static void callback(void* userdata, Uint8* stream, int length);
  const Sample* load(const std::string& key);
  const Sample* loadMusic(const std::string& key);

  SDL_AudioDeviceID device_{0};
  SDL_AudioSpec output_{};
  std::filesystem::path sfxDirectory_;
  std::filesystem::path musicDirectory_;
  std::unordered_map<std::string, Sample> samples_;
  std::unordered_map<std::string, Sample> musicSamples_;
  std::unordered_set<std::string> failed_;
  std::unordered_set<std::string> failedMusic_;
  std::array<Voice, 12> voices_{};
  Voice musicVoice_{};
  std::string musicKey_;
  std::size_t nextVoice_{0};
  bool enabled_{true};
};
}
