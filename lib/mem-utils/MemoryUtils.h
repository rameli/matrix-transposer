#pragma once

#include <cstddef>
#include <sys/sysinfo.h>
#include <fstream>
#include <iostream>
#include <string>

class MemoryUtils {
public:
    // Function to get the cache line size
    static std::size_t getCacheLineSize() {
        std::size_t lineSize = 0;
#ifdef __linux__
        std::ifstream cacheInfo("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size");
        if (cacheInfo.is_open()) {
            cacheInfo >> lineSize;
            cacheInfo.close();
        }
#endif
        return lineSize;
    }

    // Function to get the L1 data cache size
    static std::size_t getL1DataCacheSize() {
        return getCacheSize(1, "data");
    }

    // Function to get the L1 instruction cache size
    static std::size_t getL1InstructionCacheSize() {
        return getCacheSize(1, "instruction");
    }

    // Function to get the L2 cache size
    static std::size_t getL2CacheSize() {
        return getCacheSize(2, "unified");
    }

    // Function to get the L3 cache size
    static std::size_t getL3CacheSize() {
        return getCacheSize(3, "unified");
    }

    // Function to get total system memory
    static std::size_t getTotalMemory() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return info.totalram * info.mem_unit;
        }
        return 0;
    }

    // Function to get used memory
    static std::size_t getUsedMemory() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return (info.totalram - info.freeram) * info.mem_unit;
        }
        return 0;
    }

    // Function to get free memory
    static std::size_t getFreeMemory() {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            return info.freeram * info.mem_unit;
        }
        return 0;
    }

private:
    // Helper function to read cache size
    static std::size_t getCacheSize(int level, const std::string &type) {
        std::size_t cacheSize = 0;
#ifdef __linux__
        std::string path = "/sys/devices/system/cpu/cpu0/cache/index" + std::to_string(level) + "/size";
        std::ifstream cacheInfo(path);
        if (cacheInfo.is_open()) {
            std::string sizeStr;
            cacheInfo >> sizeStr;
            cacheInfo.close();
            if (sizeStr.back() == 'K') {
                cacheSize = std::stoi(sizeStr) * 1024;
            } else if (sizeStr.back() == 'M') {
                cacheSize = std::stoi(sizeStr) * 1024 * 1024;
            } else {
                cacheSize = std::stoi(sizeStr);
            }
        }
#endif
        return cacheSize;
    }
};