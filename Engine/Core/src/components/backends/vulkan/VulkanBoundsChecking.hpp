/**
 *  @file   VulkanBoundsChecking.hpp
 *  @brief  Utilities for bounds checking and validation in Vulkan resource management.
 *  @author Eryk Roszkowski
 ***********************************************/

#pragma once

#include <cstdint>
#include <vector>
#include <SDL3/SDL.h>
#include <cassert>

namespace vex {

    /// @brief Validates that an index is within bounds of a vector.
    /// @param index - The index to validate.
    /// @param size - The size of the container.
    /// @param name - Name of the container for error logging.
    /// @return true if index < size, false otherwise.
    inline bool validateVectorBounds(uint32_t index, size_t size, const char* name) {
        if (index >= size) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: %s index out of bounds (index: %u, size: %zu)",
                        name, index, size);
            return false;
        }
        return true;
    }

    /// @brief Validates that an index is within bounds of a vector.
    /// @param index - The index to validate (size_t version).
    /// @param size - The size of the container.
    /// @param name - Name of the container for error logging.
    /// @return true if index < size, false otherwise.
    inline bool validateVectorBounds(size_t index, size_t size, const char* name) {
        if (index >= size) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: %s index out of bounds (index: %zu, size: %zu)",
                        name, index, size);
            return false;
        }
        return true;
    }

    /// @brief Validates that a frame index is valid.
    /// @param frameIndex - The frame index to validate.
    /// @param maxFrames - Maximum frames in flight.
    /// @return true if frameIndex < maxFrames, false otherwise.
    inline bool validateFrameIndex(uint32_t frameIndex, uint32_t maxFrames) {
        if (frameIndex >= maxFrames) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: Frame index %u exceeds MAX_FRAMES_IN_FLIGHT (%u)",
                        frameIndex, maxFrames);
            return false;
        }
        return true;
    }

    /// @brief Validates that an image index is valid.
    /// @param imageIndex - The image index to validate.
    /// @param imageCount - Total number of swapchain images.
    /// @return true if imageIndex < imageCount, false otherwise.
    inline bool validateImageIndex(uint32_t imageIndex, size_t imageCount) {
        if (imageIndex >= imageCount) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: Image index %u exceeds swapchain image count (%zu)",
                        imageIndex, imageCount);
            return false;
        }
        return true;
    }

    /// @brief Safely accesses a vector element with bounds checking.
    /// @tparam T - The element type.
    /// @param vec - The vector to access.
    /// @param index - The index to access.
    /// @param name - Name of the vector for error logging.
    /// @param defaultValue - Value to return on bounds error.
    /// @return The element at vec[index] or defaultValue if out of bounds.
    template<typename T>
    inline T safeVectorAccess(const std::vector<T>& vec, size_t index, const char* name, T defaultValue) {
        if (index >= vec.size()) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: Safe access to %s failed (index: %zu, size: %zu)",
                        name, index, vec.size());
            return defaultValue;
        }
        return vec[index];
    }

    /// @brief Safely accesses a vector element with bounds checking (mutable version).
    /// @tparam T - The element type.
    /// @param vec - The vector to access.
    /// @param index - The index to access.
    /// @param name - Name of the vector for error logging.
    /// @return Pointer to element or nullptr if out of bounds.
    template<typename T>
    inline T* safeVectorAccessPtr(std::vector<T>& vec, size_t index, const char* name) {
        if (index >= vec.size()) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: Safe pointer access to %s failed (index: %zu, size: %zu)",
                        name, index, vec.size());
            return nullptr;
        }
        return &vec[index];
    }

    /// @brief Validates that a vector is properly sized.
    /// @param vec - The vector to validate.
    /// @param expectedSize - The expected size.
    /// @param name - Name of the vector for error logging.
    /// @return true if vec.size() == expectedSize, false otherwise.
    template<typename T>
    inline bool validateVectorSize(const std::vector<T>& vec, size_t expectedSize, const char* name) {
        if (vec.size() != expectedSize) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: %s size mismatch (expected: %zu, actual: %zu)",
                        name, expectedSize, vec.size());
            return false;
        }
        return true;
    }

    /// @brief Validates that a vector has minimum required size.
    /// @param vec - The vector to validate.
    /// @param minSize - The minimum required size.
    /// @param name - Name of the vector for error logging.
    /// @return true if vec.size() >= minSize, false otherwise.
    template<typename T>
    inline bool validateVectorMinSize(const std::vector<T>& vec, size_t minSize, const char* name) {
        if (vec.size() < minSize) {
            log(LogLevel::ERROR,
                        "VulkanBoundsChecking: %s size too small (minimum: %zu, actual: %zu)",
                        name, minSize, vec.size());
            return false;
        }
        return true;
    }

    /// @brief Validates that multiple frame-dependent vectors have consistent sizes.
    /// @details Useful for ensuring sync object arrays stay synchronized.
    /// @return true if all sizes match, false otherwise.
    inline bool validateFrameVectorConsistency(
        size_t size1, const char* name1,
        size_t size2, const char* name2,
        size_t size3, const char* name3) {

        bool valid = true;

        if (size1 != size2) {
            log(LogLevel::WARNING,
                       "VulkanBoundsChecking: %s size (%zu) != %s size (%zu)",
                       name1, size1, name2, size2);
            valid = false;
        }

        if (size1 != size3) {
            log(LogLevel::WARNING,
                       "VulkanBoundsChecking: %s size (%zu) != %s size (%zu)",
                       name1, size1, name3, size3);
            valid = false;
        }

        return valid;
    }

    /// @brief Validates that all pointers in a vector are non-null.
    /// @tparam T - Pointer type.
    /// @param vec - The vector of pointers to validate.
    /// @param name - Name of the vector for error logging.
    /// @return true if all pointers are non-null, false otherwise.
    template<typename T>
    inline bool validateNonNullHandles(const std::vector<T>& vec, const char* name) {
        for (size_t i = 0; i < vec.size(); ++i) {
            if (vec[i] == nullptr || vec[i] == VK_NULL_HANDLE) {
                log(LogLevel::ERROR,
                            "VulkanBoundsChecking: %s contains null/invalid handle at index %zu",
                            name, i);
                return false;
            }
        }
        return true;
    }

    /// @brief Asserts that all frame-dependent sync object vectors are properly sized.
    /// @details Call this after swapchain creation to ensure consistency.
    /// @param imageAvailSize - Size of imageAvailableSemaphores.
    /// @param renderFinishedSize - Size of renderFinishedSemaphores.
    /// @param fencesSize - Size of inFlightFences.
    /// @param poolsSize - Size of commandPools.
    /// @param buffersSize - Size of commandBuffers.
    /// @param expectedSize - Expected size (MAX_FRAMES_IN_FLIGHT).
    inline void assertSyncObjectConsistency(
        size_t imageAvailSize,
        size_t renderFinishedSize,
        size_t fencesSize,
        size_t poolsSize,
        size_t buffersSize,
        uint32_t expectedSize) {

        assert(imageAvailSize == expectedSize &&
               "imageAvailableSemaphores size mismatch with MAX_FRAMES_IN_FLIGHT");
        assert(renderFinishedSize == expectedSize &&
               "renderFinishedSemaphores size mismatch with MAX_FRAMES_IN_FLIGHT");
        assert(fencesSize == expectedSize &&
               "inFlightFences size mismatch with MAX_FRAMES_IN_FLIGHT");
        assert(poolsSize == expectedSize &&
               "commandPools size mismatch with MAX_FRAMES_IN_FLIGHT");
        assert(buffersSize == expectedSize &&
               "commandBuffers size mismatch with MAX_FRAMES_IN_FLIGHT");
    }

    /// @brief Macro for quick frame index validation with early return.
    /// @details Usage: VEX_CHECK_FRAME_INDEX(frameIndex, context.MAX_FRAMES_IN_FLIGHT, functionName)
    #define VEX_CHECK_FRAME_INDEX(index, maxFrames, funcName) \
        do { \
            if (!vex::validateFrameIndex(index, maxFrames)) { \
                log(LogLevel::ERROR, "%s: Aborting due to invalid frame index", funcName); \
                return; \
            } \
        } while(0)

    /// @brief Macro for quick frame index validation with bool return.
    /// @details Usage: VEX_CHECK_FRAME_INDEX_BOOL(frameIndex, context.MAX_FRAMES_IN_FLIGHT, functionName)
    #define VEX_CHECK_FRAME_INDEX_BOOL(index, maxFrames, funcName) \
        do { \
            if (!vex::validateFrameIndex(index, maxFrames)) { \
                log(LogLevel::ERROR, "%s: Aborting due to invalid frame index", funcName); \
                return false; \
            } \
        } while(0)

    /// @brief Macro for quick image index validation with early return.
    /// @details Usage: VEX_CHECK_IMAGE_INDEX(imageIndex, context.swapchainImages.size(), functionName)
    #define VEX_CHECK_IMAGE_INDEX(index, imageCount, funcName) \
        do { \
            if (!vex::validateImageIndex(index, imageCount)) { \
                log(LogLevel::ERROR, "%s: Aborting due to invalid image index", funcName); \
                return; \
            } \
        } while(0)

    /// @brief Macro for quick vector bounds checking with early return.
    /// @details Usage: VEX_CHECK_BOUNDS(index, vector.size(), "vectorName")
    #define VEX_CHECK_BOUNDS(index, size, name) \
        do { \
            if (!vex::validateVectorBounds(index, size, name)) { \
                log(LogLevel::ERROR, "Bounds check failed for %s", name); \
                return; \
            } \
        } while(0)

    /// @brief Macro for quick vector bounds checking with bool return.
    /// @details Usage: VEX_CHECK_BOUNDS_BOOL(index, vector.size(), "vectorName")
    #define VEX_CHECK_BOUNDS_BOOL(index, size, name) \
        do { \
            if (!vex::validateVectorBounds(index, size, name)) { \
                log(LogLevel::ERROR, "Bounds check failed for %s", name); \
                return false; \
            } \
        } while(0)

} // namespace vex
```

Now let me create a documentation file explaining the bounds checking system:
