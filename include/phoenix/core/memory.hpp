/**
 * Phoenix Engine - Memory Manager Header
 * 
 * Centralized memory management with tracking and debugging support
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace phoenix::core {

/**
 * @brief Memory statistics structure
 */
struct MemoryStats {
    size_t totalAllocated = 0;
    size_t allocationCount = 0;
    size_t peakUsage = 0;
};

/**
 * @brief Memory manager for centralized allocation tracking
 */
class MemoryManager {
public:
    /**
     * @brief Allocate memory
     * @param size Number of bytes to allocate
     * @return Pointer to allocated memory, or nullptr on failure
     */
    static void* allocate(size_t size);
    
    /**
     * @brief Deallocate memory
     * @param ptr Pointer to free
     */
    static void deallocate(void* ptr);
    
    /**
     * @brief Reallocate memory with new size
     * @param ptr Existing pointer
     * @param newSize New size in bytes
     * @return Pointer to reallocated memory, or nullptr on failure
     */
    static void* reallocate(void* ptr, size_t newSize);
    
    /**
     * @brief Allocate aligned memory
     * @param size Number of bytes to allocate
     * @param alignment Alignment requirement (must be power of 2)
     * @return Pointer to aligned memory, or nullptr on failure
     */
    static void* allocateAligned(size_t size, size_t alignment);
    
    /**
     * @brief Deallocate aligned memory
     * @param ptr Pointer to free
     */
    static void deallocateAligned(void* ptr);
    
    /**
     * @brief Zero-fill memory
     * @param ptr Pointer to memory
     * @param size Number of bytes to zero
     */
    static void zeroMemory(void* ptr, size_t size);
    
    /**
     * @brief Copy memory
     * @param dest Destination pointer
     * @param src Source pointer
     * @param size Number of bytes to copy
     */
    static void copyMemory(void* dest, const void* src, size_t size);
    
    /**
     * @brief Move memory (handles overlapping regions)
     * @param dest Destination pointer
     * @param src Source pointer
     * @param size Number of bytes to move
     */
    static void moveMemory(void* dest, const void* src, size_t size);
    
    /**
     * @brief Enable memory tracking
     */
    static void enableTracking();
    
    /**
     * @brief Disable memory tracking
     */
    static void disableTracking();
    
    /**
     * @brief Get current memory statistics
     * @return MemoryStats structure with current stats
     */
    static MemoryStats getStats();
    
    /**
     * @brief Print memory statistics to stdout
     */
    static void printStats();
};

} // namespace phoenix::core
