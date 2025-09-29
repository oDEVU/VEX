#pragma once

#include <iostream>
#include <cstdint>

namespace vex {
struct GameInfo {
  std::string projectName = "ExampleName";
  uint16_t versionMajor = 0;
  uint16_t versionMinor = 0;
  uint16_t versionPatch = 0;
};
}
