/**
 *  @file   limits.hpp
 *  @brief  This files defines global variables defining vulkan limits.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <sys/types.h>
#include <glm/glm.hpp>

/// @todo Implement something to dynamically allocate resources and not rely on max textures and models
const uint32_t MAX_TEXTURES = 4096; // This is already a little too much for my liking, i need to change how textures are stored/sent to gpu
const uint32_t MAX_MODELS = 8192; // This can be much higher no problem, but you probably wont even be able to use it as long as you used textured models.
const uint32_t MAX_DYNAMIC_LIGHTS = 255; // This should not be much higher, thats effectivelly per object, so its probably too much.
