#pragma once

#include <cstdint>

void naive_transpose                     (uint64_t* src   , uint64_t* dst, int M, int N);
void tile_transpose_mt                   (uint64_t* src   , uint64_t* dst, int M, int N, int B, int num_threads);
void transpose_recursive_wrapper         (uint64_t* src   , uint64_t* dst, int M, int N);
void naive_transpose_in_place            (uint64_t* matrix,                int N);
void tile_transpose_in_place_mt          (uint64_t* matrix,                int N, int B, int num_threads);
void transpose_recursive_in_place_wrapper(uint64_t* matrix,                int N);
bool verify(uint64_t* A, uint64_t* B, int rows, int cols);
