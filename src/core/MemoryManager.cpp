/**
 * Phoenix Engine - Memory Manager
 * 
 * Centralized memory management with tracking and debugging support
 */

#include "../../include/phoenix/core/memory.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <mutex>

namespace phoenix::core {

// Global memory tracking (simplified stub implementation)
static struct {
    size_t totalAllocated = 0;
    size_t allocationCount = 0;
    bool trackingEnabled = false;
    std::mutex mutex;  // Thread safety mutex
} g_memoryStats;

void* MemoryManager::allocate(size_t size) {
    void* ptr = std::malloc(size);
    
    if (g_memoryStats.trackingEnabled && ptr) {
        std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
        g_memoryStats.totalAllocated += size;
        g_memoryStats.allocationCount++;
    }
    
    return ptr;
}

void MemoryManager::deallocate(void* ptr) {
    if (ptr) {
        std::free(ptr);
        
        if (g_memoryStats.trackingEnabled) {
            std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
            // Note: We don't track individual allocation sizes in this stub
            g_memoryStats.allocationCount--;
        }
    }
}

void* MemoryManager::reallocate(void* ptr, size_t newSize) {
    void* newPtr = std::realloc(ptr, newSize);
    
    if (g_memoryStats.trackingEnabled && newPtr && newSize > 0) {
        std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
        // Note: Simplified tracking - ideally we'd track old size and adjust
        g_memoryStats.totalAllocated += newSize;
    }
    
    return newPtr;
}

void* MemoryManager::allocateAligned(size_t size, size_t alignment) {
    void* ptr = nullptr;
    
#ifdef _WIN32
    ptr = _aligned_malloc(size, alignment);
#else
    if (posix_memalign(&ptr, alignment, size) != 0) {
        ptr = nullptr;
    }
#endif
    
    if (g_memoryStats.trackingEnabled && ptr) {
        std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
        g_memoryStats.totalAllocated += size;
        g_memoryStats.allocationCount++;
    }
    
    return ptr;
}

void MemoryManager::deallocateAligned(void* ptr) {
    if (ptr) {
#ifdef _WIN32
        _aligned_free(ptr);
#else
        std::free(ptr);
#endif
        
        if (g_memoryStats.trackingEnabled) {
            std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
            g_memoryStats.allocationCount--;
        }
    }
}

void MemoryManager::zeroMemory(void* ptr, size_t size) {
    if (ptr) {
        std::memset(ptr, 0, size);
    }
}

void MemoryManager::copyMemory(void* dest, const void* src, size_t size) {
    if (dest && src) {
        std::memcpy(dest, src, size);
    }
}

void MemoryManager::moveMemory(void* dest, const void* src, size_t size) {
    if (dest && src) {
        std::memmove(dest, src, size);
    }
}

void MemoryManager::enableTracking() {
    std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
    g_memoryStats.trackingEnabled = true;
    g_memoryStats.totalAllocated = 0;
    g_memoryStats.allocationCount = 0;
}

void MemoryManager::disableTracking() {
    std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
    g_memoryStats.trackingEnabled = false;
}

MemoryStats MemoryManager::getStats() {
    std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
    MemoryStats stats;
    stats.totalAllocated = g_memoryStats.totalAllocated;
    stats.allocationCount = g_memoryStats.allocationCount;
    stats.peakUsage = g_memoryStats.totalAllocated; // Simplified
    return stats;
}

void MemoryManager::printStats() {
    std::lock_guard<std::mutex> lock(g_memoryStats.mutex);
    std::cout << "Memory Manager Statistics:" << std::endl;
    std::cout << "  Total Allocated: " << g_memoryStats.totalAllocated << " bytes" << std::endl;
    std::cout << "  Allocation Count: " << g_memoryStats.allocationCount << std::endl;
}

} // namespace phoenix::core
