#pragma once
#include <immintrin.h>

// 3-float를 [x,y,z,0] 형태로 로드
static inline __m128 load3(const float* p) {
	return _mm_set_ps(0.0f, p[2], p[1], p[0]);
}
// [x,y,z,(dontcare)]의 하위 3개만 저장
static inline void store3(float* p, __m128 v) {
	p[0] = _mm_cvtss_f32(v);
	p[1] = _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 1, 1)));
	p[2] = _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 1, 2)));
}
// 2-float를 [x,y,0,0] 형태로 로드/스토어
static inline __m128 load2(const float* p) {
	return _mm_set_ps(0.0f, 0.0f, p[1], p[0]);
}
static inline void store2(float* p, __m128 v) {
	p[0] = _mm_cvtss_f32(v);
	p[1] = _mm_cvtss_f32(_mm_shuffle_ps(v, v, _MM_SHUFFLE(3, 2, 1, 1)));
}
