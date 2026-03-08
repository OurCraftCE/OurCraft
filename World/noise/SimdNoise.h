#pragma once
#include <immintrin.h>

#ifndef ALIGN16
#define ALIGN16 __declspec(align(16))
#endif
#ifndef ALIGN32
#define ALIGN32 __declspec(align(32))
#endif

// SIMD-optimized noise functions for faster terrain generation
// Processes multiple noise samples in parallel using SSE2 (4x) or AVX2 (8x)

class SimdNoise
{
public:
	// Runtime CPU feature detection
	static bool HasSSE2();
	static bool HasAVX2();
	static void DetectFeatures();

	// SSE2 helpers (4 samples at once)
	static __m128 fade_sse(__m128 t);
	static __m128 lerp_sse(__m128 t, __m128 a, __m128 b);
	static __m128 grad_sse(__m128i hash, __m128 x, __m128 y, __m128 z);

	// SSE2 Perlin noise - 4 samples at once
	// Each of x, y, z contains 4 coordinate values
	static __m128 perlin_noise_4x(
		__m128 x, __m128 y, __m128 z,
		const int* perm);

	// Batch process: fill output buffer with noise values for a grid
	// This processes 4 columns at a time using SSE2
	static void perlin_batch_sse2(
		double* output,
		double startX, double startY, double startZ,
		int xSize, int ySize, int zSize,
		double xScale, double yScale, double zScale,
		const int* perm,
		double xo, double yo, double zo,
		double amplitude);

	// AVX2 helpers (8 samples at once)
	static __m256 fade_avx(__m256 t);
	static __m256 lerp_avx(__m256 t, __m256 a, __m256 b);

	// AVX2 Perlin noise - 8 samples at once
	static __m256 perlin_noise_8x(
		__m256 x, __m256 y, __m256 z,
		const int* perm);

	// Batch process using AVX2
	static void perlin_batch_avx2(
		double* output,
		double startX, double startY, double startZ,
		int xSize, int ySize, int zSize,
		double xScale, double yScale, double zScale,
		const int* perm,
		double xo, double yo, double zo,
		double amplitude);

	// Auto-dispatching batch function (picks best available path)
	static void perlin_batch(
		double* output,
		double startX, double startY, double startZ,
		int xSize, int ySize, int zSize,
		double xScale, double yScale, double zScale,
		const int* perm,
		double xo, double yo, double zo,
		double amplitude);

private:
	static bool s_featuresDetected;
	static bool s_hasSSE2;
	static bool s_hasAVX2;
};
