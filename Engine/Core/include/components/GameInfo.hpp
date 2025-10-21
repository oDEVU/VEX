/**
 *  @file   GameInfo.hpp
 *  @brief  This file defines struct GameInfo.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <iostream>
#include <cstdint>

namespace vex {
    /// @brief this struct contains information about the game. like project name and version.
    /// @todo make it auto load data from project file packed into assets.
struct GameInfo {
  std::string projectName = "ExampleName";
  uint16_t versionMajor = 0;
  uint16_t versionMinor = 0;
  uint16_t versionPatch = 0;
};
}
