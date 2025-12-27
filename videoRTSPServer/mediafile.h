#pragma once
#include <cstdint>
#include <fstream>
#include <vector>
struct Nalu {
  std::vector<uint8_t> data;
  bool is_valid = false;
  static Nalu readNextNalu(std::ifstream& video_file);
};
