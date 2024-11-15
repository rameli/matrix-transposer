#pragma once

#include <cstdint>

void TransposeNaive(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount);
void TransposeNaiveInPlace(uint64_t* matrix, uint32_t rowCount);

void TransposeTiledMultiThreaded(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads);
void TransposeTiledMultiThreadedOptimized(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount, uint32_t tileSize, uint32_t numThreads);
void TransposeTiledInPlaceMultiThreaded(uint64_t* matrix, uint32_t rowCount, uint32_t tileSize, uint32_t numThreads);

void TransposeRecursive(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount);
void TransposeRecursiveInPlace(uint64_t* matrix, uint32_t rowCount);

bool MatricesAreEqual(uint64_t* src, uint64_t* dst, uint32_t rowCount, uint32_t colCount);
