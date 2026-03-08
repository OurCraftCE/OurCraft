#include "stdafx.h"
#include "SimdNoise.h"
#include <intrin.h>
#include <math.h>

bool SimdNoise::s_featuresDetected = false;
bool SimdNoise::s_hasSSE2 = false;
bool SimdNoise::s_hasAVX2 = false;

// ============================================================
// CPU Feature Detection
// ============================================================

bool SimdNoise::HasSSE2()
{
	if (!s_featuresDetected) DetectFeatures();
	return s_hasSSE2;
}

bool SimdNoise::HasAVX2()
{
	if (!s_featuresDetected) DetectFeatures();
	return s_hasAVX2;
}

void SimdNoise::DetectFeatures()
{
	int cpuInfo[4];

	// Check SSE2 support (CPUID function 1, EDX bit 26)
	__cpuid(cpuInfo, 1);
	s_hasSSE2 = (cpuInfo[3] & (1 << 26)) != 0;

	// Check AVX2 support (CPUID function 7, EBX bit 5)
	__cpuidex(cpuInfo, 7, 0);
	s_hasAVX2 = (cpuInfo[1] & (1 << 5)) != 0;

	s_featuresDetected = true;
}

// ============================================================
// SSE2 Implementation (4 samples at once)
// ============================================================

// Fade curve: 6t^5 - 15t^4 + 10t^3
__m128 SimdNoise::fade_sse(__m128 t)
{
	__m128 t3 = _mm_mul_ps(_mm_mul_ps(t, t), t);
	__m128 t4 = _mm_mul_ps(t3, t);
	__m128 t5 = _mm_mul_ps(t4, t);

	// 6t^5 - 15t^4 + 10t^3
	__m128 six = _mm_set1_ps(6.0f);
	__m128 fifteen = _mm_set1_ps(15.0f);
	__m128 ten = _mm_set1_ps(10.0f);

	return _mm_add_ps(
		_mm_sub_ps(_mm_mul_ps(six, t5), _mm_mul_ps(fifteen, t4)),
		_mm_mul_ps(ten, t3));
}

__m128 SimdNoise::lerp_sse(__m128 t, __m128 a, __m128 b)
{
	// a + t * (b - a)
	return _mm_add_ps(a, _mm_mul_ps(t, _mm_sub_ps(b, a)));
}

// Simplified gradient function for SSE2 - processes 4 hash values
__m128 SimdNoise::grad_sse(__m128i hash, __m128 x, __m128 y, __m128 z)
{
	// h = hash & 15
	__m128i h = _mm_and_si128(hash, _mm_set1_epi32(15));

	// u = h < 8 ? x : y
	__m128i cmp8 = _mm_cmplt_epi32(h, _mm_set1_epi32(8));
	__m128 u = _mm_or_ps(
		_mm_and_ps(_mm_castsi128_ps(cmp8), x),
		_mm_andnot_ps(_mm_castsi128_ps(cmp8), y));

	// v = h < 4 ? y : (h == 12 || h == 14 ? x : z)
	__m128i cmp4 = _mm_cmplt_epi32(h, _mm_set1_epi32(4));
	__m128i eq12 = _mm_cmpeq_epi32(h, _mm_set1_epi32(12));
	__m128i eq14 = _mm_cmpeq_epi32(h, _mm_set1_epi32(14));
	__m128i use_x = _mm_or_si128(eq12, eq14);

	__m128 inner = _mm_or_ps(
		_mm_and_ps(_mm_castsi128_ps(use_x), x),
		_mm_andnot_ps(_mm_castsi128_ps(use_x), z));

	__m128 v = _mm_or_ps(
		_mm_and_ps(_mm_castsi128_ps(cmp4), y),
		_mm_andnot_ps(_mm_castsi128_ps(cmp4), inner));

	// (h & 1) == 0 ? u : -u
	__m128i h_and_1 = _mm_and_si128(h, _mm_set1_epi32(1));
	__m128i cmp_h1 = _mm_cmpeq_epi32(h_and_1, _mm_setzero_si128());
	__m128 sign_u = _mm_or_ps(
		_mm_and_ps(_mm_castsi128_ps(cmp_h1), u),
		_mm_andnot_ps(_mm_castsi128_ps(cmp_h1), _mm_sub_ps(_mm_setzero_ps(), u)));

	// (h & 2) == 0 ? v : -v
	__m128i h_and_2 = _mm_and_si128(h, _mm_set1_epi32(2));
	__m128i cmp_h2 = _mm_cmpeq_epi32(h_and_2, _mm_setzero_si128());
	__m128 sign_v = _mm_or_ps(
		_mm_and_ps(_mm_castsi128_ps(cmp_h2), v),
		_mm_andnot_ps(_mm_castsi128_ps(cmp_h2), _mm_sub_ps(_mm_setzero_ps(), v)));

	return _mm_add_ps(sign_u, sign_v);
}

// SSE2 Perlin noise: processes 4 independent 3D noise samples
__m128 SimdNoise::perlin_noise_4x(
	__m128 x, __m128 y, __m128 z,
	const int* perm)
{
	// Floor
	__m128 floorX = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_sub_ps(x, _mm_and_ps(_mm_cmplt_ps(x, _mm_cvtepi32_ps(_mm_cvttps_epi32(x))), _mm_set1_ps(1.0f)))));
	__m128 floorY = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_sub_ps(y, _mm_and_ps(_mm_cmplt_ps(y, _mm_cvtepi32_ps(_mm_cvttps_epi32(y))), _mm_set1_ps(1.0f)))));
	__m128 floorZ = _mm_cvtepi32_ps(_mm_cvttps_epi32(_mm_sub_ps(z, _mm_and_ps(_mm_cmplt_ps(z, _mm_cvtepi32_ps(_mm_cvttps_epi32(z))), _mm_set1_ps(1.0f)))));

	// Integer coordinates & 255
	__m128i xi = _mm_and_si128(_mm_cvttps_epi32(floorX), _mm_set1_epi32(255));
	__m128i yi = _mm_and_si128(_mm_cvttps_epi32(floorY), _mm_set1_epi32(255));
	__m128i zi = _mm_and_si128(_mm_cvttps_epi32(floorZ), _mm_set1_epi32(255));

	// Fractional parts
	__m128 xf = _mm_sub_ps(x, floorX);
	__m128 yf = _mm_sub_ps(y, floorY);
	__m128 zf = _mm_sub_ps(z, floorZ);

	// Fade curves
	__m128 u = fade_sse(xf);
	__m128 v = fade_sse(yf);
	__m128 w = fade_sse(zf);

	// We need to do scalar permutation lookups since the perm table
	// can't easily be gathered with SSE2. Extract, lookup, repack.
	ALIGN16 int xi_arr[4], yi_arr[4], zi_arr[4];
	_mm_store_si128((__m128i*)xi_arr, xi);
	_mm_store_si128((__m128i*)yi_arr, yi);
	_mm_store_si128((__m128i*)zi_arr, zi);

	ALIGN16 int aa[4], ab[4], ba[4], bb[4];
	for (int i = 0; i < 4; i++)
	{
		int A = perm[xi_arr[i]] + yi_arr[i];
		int B = perm[xi_arr[i] + 1] + yi_arr[i];
		aa[i] = perm[A] + zi_arr[i];
		ab[i] = perm[A + 1] + zi_arr[i];
		ba[i] = perm[B] + zi_arr[i];
		bb[i] = perm[B + 1] + zi_arr[i];
	}

	// Load hash values for the 8 corners
	ALIGN16 int h_aa[4], h_ba[4], h_ab[4], h_bb[4];
	ALIGN16 int h_aa1[4], h_ba1[4], h_ab1[4], h_bb1[4];
	for (int i = 0; i < 4; i++)
	{
		h_aa[i] = perm[aa[i]];
		h_ba[i] = perm[ba[i]];
		h_ab[i] = perm[ab[i]];
		h_bb[i] = perm[bb[i]];
		h_aa1[i] = perm[aa[i] + 1];
		h_ba1[i] = perm[ba[i] + 1];
		h_ab1[i] = perm[ab[i] + 1];
		h_bb1[i] = perm[bb[i] + 1];
	}

	__m128 one = _mm_set1_ps(1.0f);
	__m128 xf1 = _mm_sub_ps(xf, one);
	__m128 yf1 = _mm_sub_ps(yf, one);
	__m128 zf1 = _mm_sub_ps(zf, one);

	// Compute gradients for all 8 corners
	__m128 g_aa  = grad_sse(_mm_load_si128((__m128i*)h_aa),  xf,  yf,  zf);
	__m128 g_ba  = grad_sse(_mm_load_si128((__m128i*)h_ba),  xf1, yf,  zf);
	__m128 g_ab  = grad_sse(_mm_load_si128((__m128i*)h_ab),  xf,  yf1, zf);
	__m128 g_bb  = grad_sse(_mm_load_si128((__m128i*)h_bb),  xf1, yf1, zf);
	__m128 g_aa1 = grad_sse(_mm_load_si128((__m128i*)h_aa1), xf,  yf,  zf1);
	__m128 g_ba1 = grad_sse(_mm_load_si128((__m128i*)h_ba1), xf1, yf,  zf1);
	__m128 g_ab1 = grad_sse(_mm_load_si128((__m128i*)h_ab1), xf,  yf1, zf1);
	__m128 g_bb1 = grad_sse(_mm_load_si128((__m128i*)h_bb1), xf1, yf1, zf1);

	// Trilinear interpolation
	__m128 lerp_x0 = lerp_sse(u, g_aa, g_ba);
	__m128 lerp_x1 = lerp_sse(u, g_ab, g_bb);
	__m128 lerp_x2 = lerp_sse(u, g_aa1, g_ba1);
	__m128 lerp_x3 = lerp_sse(u, g_ab1, g_bb1);

	__m128 lerp_y0 = lerp_sse(v, lerp_x0, lerp_x1);
	__m128 lerp_y1 = lerp_sse(v, lerp_x2, lerp_x3);

	return lerp_sse(w, lerp_y0, lerp_y1);
}

// Batch SSE2 noise: process grid of noise samples 4 at a time
void SimdNoise::perlin_batch_sse2(
	double* output,
	double startX, double startY, double startZ,
	int xSize, int ySize, int zSize,
	double xScale, double yScale, double zScale,
	const int* perm,
	double xo, double yo, double zo,
	double amplitude)
{
	double scale = 1.0 / amplitude;
	int pp = 0;

	for (int xx = 0; xx < xSize; xx++)
	{
		float fx = (float)(startX + xx * xScale + xo);

		for (int zz = 0; zz < zSize; zz++)
		{
			float fz = (float)(startZ + zz * zScale + zo);

			// Process ySize values in groups of 4
			int yy = 0;
			for (; yy + 3 < ySize; yy += 4)
			{
				__m128 x4 = _mm_set1_ps(fx);
				__m128 z4 = _mm_set1_ps(fz);
				__m128 y4 = _mm_set_ps(
					(float)(startY + (yy + 3) * yScale + yo),
					(float)(startY + (yy + 2) * yScale + yo),
					(float)(startY + (yy + 1) * yScale + yo),
					(float)(startY + (yy + 0) * yScale + yo));

				__m128 result = perlin_noise_4x(x4, y4, z4, perm);

				ALIGN16 float results[4];
				_mm_store_ps(results, result);

				for (int i = 0; i < 4; i++)
				{
					output[pp + yy + i] += (double)results[i] * scale;
				}
			}

			// Handle remaining samples (scalar fallback)
			for (; yy < ySize; yy++)
			{
				float fy = (float)(startY + yy * yScale + yo);

				__m128 x1 = _mm_set_ss(fx);
				__m128 y1 = _mm_set_ss(fy);
				__m128 z1 = _mm_set_ss(fz);

				__m128 result = perlin_noise_4x(x1, y1, z1, perm);

				float val;
				_mm_store_ss(&val, result);
				output[pp + yy] += (double)val * scale;
			}

			pp += ySize;
		}
	}
}

// ============================================================
// AVX2 Implementation (8 samples at once)
// ============================================================

__m256 SimdNoise::fade_avx(__m256 t)
{
	__m256 t3 = _mm256_mul_ps(_mm256_mul_ps(t, t), t);
	__m256 t4 = _mm256_mul_ps(t3, t);
	__m256 t5 = _mm256_mul_ps(t4, t);

	__m256 six = _mm256_set1_ps(6.0f);
	__m256 fifteen = _mm256_set1_ps(15.0f);
	__m256 ten = _mm256_set1_ps(10.0f);

	return _mm256_add_ps(
		_mm256_sub_ps(_mm256_mul_ps(six, t5), _mm256_mul_ps(fifteen, t4)),
		_mm256_mul_ps(ten, t3));
}

__m256 SimdNoise::lerp_avx(__m256 t, __m256 a, __m256 b)
{
	return _mm256_add_ps(a, _mm256_mul_ps(t, _mm256_sub_ps(b, a)));
}

// AVX2 Perlin noise: processes 8 independent 3D noise samples
__m256 SimdNoise::perlin_noise_8x(
	__m256 x, __m256 y, __m256 z,
	const int* perm)
{
	// Floor - use truncation and correction for negative values
	__m256 truncX = _mm256_cvtepi32_ps(_mm256_cvttps_epi32(x));
	__m256 truncY = _mm256_cvtepi32_ps(_mm256_cvttps_epi32(y));
	__m256 truncZ = _mm256_cvtepi32_ps(_mm256_cvttps_epi32(z));

	__m256 floorX = _mm256_sub_ps(truncX, _mm256_and_ps(_mm256_cmp_ps(x, truncX, _CMP_LT_OS), _mm256_set1_ps(1.0f)));
	__m256 floorY = _mm256_sub_ps(truncY, _mm256_and_ps(_mm256_cmp_ps(y, truncY, _CMP_LT_OS), _mm256_set1_ps(1.0f)));
	__m256 floorZ = _mm256_sub_ps(truncZ, _mm256_and_ps(_mm256_cmp_ps(z, truncZ, _CMP_LT_OS), _mm256_set1_ps(1.0f)));

	// Integer coords & 255
	__m256i xi = _mm256_and_si256(_mm256_cvttps_epi32(floorX), _mm256_set1_epi32(255));
	__m256i yi = _mm256_and_si256(_mm256_cvttps_epi32(floorY), _mm256_set1_epi32(255));
	__m256i zi = _mm256_and_si256(_mm256_cvttps_epi32(floorZ), _mm256_set1_epi32(255));

	// Fractional parts
	__m256 xf = _mm256_sub_ps(x, floorX);
	__m256 yf = _mm256_sub_ps(y, floorY);
	__m256 zf = _mm256_sub_ps(z, floorZ);

	// Fade curves
	__m256 u = fade_avx(xf);
	__m256 v = fade_avx(yf);
	__m256 w = fade_avx(zf);

	// Scalar perm table lookups (AVX2 gather could be used but perm table is small)
	ALIGN32 int xi_arr[8], yi_arr[8], zi_arr[8];
	_mm256_store_si256((__m256i*)xi_arr, xi);
	_mm256_store_si256((__m256i*)yi_arr, yi);
	_mm256_store_si256((__m256i*)zi_arr, zi);

	ALIGN32 int h_aa[8], h_ba[8], h_ab[8], h_bb[8];
	ALIGN32 int h_aa1[8], h_ba1[8], h_ab1[8], h_bb1[8];
	for (int i = 0; i < 8; i++)
	{
		int A = perm[xi_arr[i]] + yi_arr[i];
		int B = perm[xi_arr[i] + 1] + yi_arr[i];
		int aa = perm[A] + zi_arr[i];
		int ab = perm[A + 1] + zi_arr[i];
		int ba = perm[B] + zi_arr[i];
		int bb = perm[B + 1] + zi_arr[i];
		h_aa[i] = perm[aa];     h_ba[i] = perm[ba];
		h_ab[i] = perm[ab];     h_bb[i] = perm[bb];
		h_aa1[i] = perm[aa+1];  h_ba1[i] = perm[ba+1];
		h_ab1[i] = perm[ab+1];  h_bb1[i] = perm[bb+1];
	}

	// For AVX2 grad, we do it in scalar and pack results (gradient is cheap, table lookup is the bottleneck)
	__m256 one = _mm256_set1_ps(1.0f);
	__m256 xf1 = _mm256_sub_ps(xf, one);
	__m256 yf1 = _mm256_sub_ps(yf, one);
	__m256 zf1 = _mm256_sub_ps(zf, one);

	// Compute 8 gradients per corner - scalar for correctness, then pack
	ALIGN32 float xf_arr[8], yf_arr[8], zf_arr[8];
	ALIGN32 float xf1_arr[8], yf1_arr[8], zf1_arr[8];
	_mm256_store_ps(xf_arr, xf);   _mm256_store_ps(yf_arr, yf);   _mm256_store_ps(zf_arr, zf);
	_mm256_store_ps(xf1_arr, xf1); _mm256_store_ps(yf1_arr, yf1); _mm256_store_ps(zf1_arr, zf1);

	// Scalar gradient computation helper
	auto scalar_grad = [](int hash, float gx, float gy, float gz) -> float {
		int h = hash & 15;
		float gu = h < 8 ? gx : gy;
		float gv = h < 4 ? gy : (h == 12 || h == 14 ? gx : gz);
		return ((h & 1) == 0 ? gu : -gu) + ((h & 2) == 0 ? gv : -gv);
	};

	ALIGN32 float g[8];

	// Corner (0,0,0)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_aa[i], xf_arr[i], yf_arr[i], zf_arr[i]);
	__m256 g_aa = _mm256_load_ps(g);

	// Corner (1,0,0)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_ba[i], xf1_arr[i], yf_arr[i], zf_arr[i]);
	__m256 g_ba = _mm256_load_ps(g);

	// Corner (0,1,0)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_ab[i], xf_arr[i], yf1_arr[i], zf_arr[i]);
	__m256 g_ab = _mm256_load_ps(g);

	// Corner (1,1,0)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_bb[i], xf1_arr[i], yf1_arr[i], zf_arr[i]);
	__m256 g_bb = _mm256_load_ps(g);

	// Corner (0,0,1)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_aa1[i], xf_arr[i], yf_arr[i], zf1_arr[i]);
	__m256 g_aa1 = _mm256_load_ps(g);

	// Corner (1,0,1)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_ba1[i], xf1_arr[i], yf_arr[i], zf1_arr[i]);
	__m256 g_ba1 = _mm256_load_ps(g);

	// Corner (0,1,1)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_ab1[i], xf_arr[i], yf1_arr[i], zf1_arr[i]);
	__m256 g_ab1 = _mm256_load_ps(g);

	// Corner (1,1,1)
	for (int i = 0; i < 8; i++) g[i] = scalar_grad(h_bb1[i], xf1_arr[i], yf1_arr[i], zf1_arr[i]);
	__m256 g_bb1 = _mm256_load_ps(g);

	// Trilinear interpolation
	__m256 lerp_x0 = lerp_avx(u, g_aa, g_ba);
	__m256 lerp_x1 = lerp_avx(u, g_ab, g_bb);
	__m256 lerp_x2 = lerp_avx(u, g_aa1, g_ba1);
	__m256 lerp_x3 = lerp_avx(u, g_ab1, g_bb1);

	__m256 lerp_y0 = lerp_avx(v, lerp_x0, lerp_x1);
	__m256 lerp_y1 = lerp_avx(v, lerp_x2, lerp_x3);

	return lerp_avx(w, lerp_y0, lerp_y1);
}

// Batch AVX2 noise: process grid of noise samples 8 at a time
void SimdNoise::perlin_batch_avx2(
	double* output,
	double startX, double startY, double startZ,
	int xSize, int ySize, int zSize,
	double xScale, double yScale, double zScale,
	const int* perm,
	double xo, double yo, double zo,
	double amplitude)
{
	double scale = 1.0 / amplitude;
	int pp = 0;

	for (int xx = 0; xx < xSize; xx++)
	{
		float fx = (float)(startX + xx * xScale + xo);

		for (int zz = 0; zz < zSize; zz++)
		{
			float fz = (float)(startZ + zz * zScale + zo);

			int yy = 0;
			for (; yy + 7 < ySize; yy += 8)
			{
				__m256 x8 = _mm256_set1_ps(fx);
				__m256 z8 = _mm256_set1_ps(fz);
				__m256 y8 = _mm256_set_ps(
					(float)(startY + (yy + 7) * yScale + yo),
					(float)(startY + (yy + 6) * yScale + yo),
					(float)(startY + (yy + 5) * yScale + yo),
					(float)(startY + (yy + 4) * yScale + yo),
					(float)(startY + (yy + 3) * yScale + yo),
					(float)(startY + (yy + 2) * yScale + yo),
					(float)(startY + (yy + 1) * yScale + yo),
					(float)(startY + (yy + 0) * yScale + yo));

				__m256 result = perlin_noise_8x(x8, y8, z8, perm);

				ALIGN32 float results[8];
				_mm256_store_ps(results, result);

				for (int i = 0; i < 8; i++)
				{
					output[pp + yy + i] += (double)results[i] * scale;
				}
			}

			// Handle remaining with SSE2
			for (; yy + 3 < ySize; yy += 4)
			{
				__m128 x4 = _mm_set1_ps(fx);
				__m128 z4 = _mm_set1_ps(fz);
				__m128 y4 = _mm_set_ps(
					(float)(startY + (yy + 3) * yScale + yo),
					(float)(startY + (yy + 2) * yScale + yo),
					(float)(startY + (yy + 1) * yScale + yo),
					(float)(startY + (yy + 0) * yScale + yo));

				__m128 result = perlin_noise_4x(x4, y4, z4, perm);

				ALIGN16 float results[4];
				_mm_store_ps(results, result);

				for (int i = 0; i < 4; i++)
				{
					output[pp + yy + i] += (double)results[i] * scale;
				}
			}

			// Scalar fallback for tail
			for (; yy < ySize; yy++)
			{
				float fy = (float)(startY + yy * yScale + yo);
				__m128 x1 = _mm_set_ss(fx);
				__m128 y1 = _mm_set_ss(fy);
				__m128 z1 = _mm_set_ss(fz);

				__m128 result = perlin_noise_4x(x1, y1, z1, perm);
				float val;
				_mm_store_ss(&val, result);
				output[pp + yy] += (double)val * scale;
			}

			pp += ySize;
		}
	}
}

// ============================================================
// Auto-dispatching batch function
// ============================================================

void SimdNoise::perlin_batch(
	double* output,
	double startX, double startY, double startZ,
	int xSize, int ySize, int zSize,
	double xScale, double yScale, double zScale,
	const int* perm,
	double xo, double yo, double zo,
	double amplitude)
{
	if (HasAVX2())
	{
		perlin_batch_avx2(output, startX, startY, startZ,
			xSize, ySize, zSize, xScale, yScale, zScale,
			perm, xo, yo, zo, amplitude);
	}
	else if (HasSSE2())
	{
		perlin_batch_sse2(output, startX, startY, startZ,
			xSize, ySize, zSize, xScale, yScale, zScale,
			perm, xo, yo, zo, amplitude);
	}
	// else: caller uses the original scalar path
}
