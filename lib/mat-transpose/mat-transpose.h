#pragma once

#include <cstdint>

void TransposeNaive(uint64_t* src, uint64_t* dst, int M, int N);
void TransposeTiledMultiThreaded(uint64_t* src, uint64_t* dst, int M, int N, int B, int num_threads);
void TransposeRecursive(uint64_t* src, uint64_t* dst, int M, int N);
void TransposeNaiveInPlace(uint64_t* matrix, int N);
void TransposeNaiveInPlaceMultiThreaded(uint64_t* matrix, int N, int B, int num_threads);
void TransposeRecursiveInPlace(uint64_t* matrix, int N);
bool MatricesAreEqual(uint64_t* A, uint64_t* B, int rows, int cols);
