#pragma once

#include <d3d11.h>
#if defined(__AVX2__)
#include <immintrin.h>
#endif

// 논문 기본 타일 크기
static constexpr int MSOC_TILE_W = 32;
static constexpr int MSOC_TILE_H = 8;

struct alignas(32) FMaskedTile
{
	// 8개의 scanline => 32bit * 8 => 256bit
	uint32_t CoverageMask[MSOC_TILE_H];
	float    Z0max;
	float    Z1max;
	bool     bFullCovered; // 마스크가 모두 1인 상태인거 확인용
};

struct FSoftwareTri
{
	FVector P0;
	FVector P1;
	FVector P2;
};

struct FProjectedTri
{
	// 스크린 픽셀 좌표, 깊이 
	float x0, y0, z0;
	float x1, y1, z1;
	float x2, y2, z2;

	// AABB 
	int   minX, minY, maxX, maxY;
};

struct FScreenRect { int x0, y0, x1, y1; float zmin; };



static inline void PerspectiveDivide8_AVX2(
	const __m256 Hx, const __m256 Hy, const __m256 Hz, const __m256 Hw,
	__m256& x_ndc, __m256& y_ndc, __m256& z_ndc)
{
	__m256 invw = _mm256_rcp_ps(Hw);                  // 1/w (근사)
	// 뉴튼-랩슨 1회 보정: invw = invw * (2 - Hw*invw)
	const __m256 two = _mm256_set1_ps(2.0f);
	invw = _mm256_mul_ps(invw, _mm256_sub_ps(two, _mm256_mul_ps(Hw, invw)));

	x_ndc = _mm256_mul_ps(Hx, invw);
	y_ndc = _mm256_mul_ps(Hy, invw);
	z_ndc = _mm256_mul_ps(Hz, invw);
}

static inline void NDCToViewport8_AVX2(
	const __m256 x_ndc, const __m256 y_ndc, const __m256 z_ndc,
	float topLeftX, float topLeftY, float width, float height,
	__m256& sx, __m256& sy, __m256& sz)
{
	const __m256 half = _mm256_set1_ps(0.5f);
	const __m256 vTX = _mm256_set1_ps(topLeftX);
	const __m256 vTY = _mm256_set1_ps(topLeftY);
	const __m256 vW = _mm256_set1_ps(width);
	const __m256 vH = _mm256_set1_ps(height);

	// x = tX + (x_ndc*0.5 + 0.5) * W
	sx = _mm256_add_ps(vTX, _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(x_ndc, half), half), vW));
	// y = tY + (-y_ndc*0.5 + 0.5) * H
	sy = _mm256_add_ps(vTY, _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(0.0f), y_ndc), half), half), vH));
	// z = z_ndc*0.5 + 0.5
	sz = _mm256_add_ps(half, _mm256_mul_ps(z_ndc, half));

}

static inline void MulPointsByMatrix8_AVX2(
	const float* __restrict px,  // 8개 x
	const float* __restrict py,  // 8개 y
	const float* __restrict pz,  // 8개 z
	const FMatrix& M,
	__m256& Hx, __m256& Hy, __m256& Hz, __m256& Hw)
{
	__m256 X = _mm256_loadu_ps(px);
	__m256 Y = _mm256_loadu_ps(py);
	__m256 Z = _mm256_loadu_ps(pz);
	__m256 O = _mm256_set1_ps(1.0f);

	// Hx = x*m00 + y*m10 + z*m20 + 1*m30  (열0)
	Hx = _mm256_add_ps(
		_mm256_add_ps(_mm256_mul_ps(X, _mm256_set1_ps(M.Data[0][0])),
			_mm256_mul_ps(Y, _mm256_set1_ps(M.Data[1][0]))),
		_mm256_add_ps(_mm256_mul_ps(Z, _mm256_set1_ps(M.Data[2][0])),
			_mm256_mul_ps(O, _mm256_set1_ps(M.Data[3][0]))));

	Hy = _mm256_add_ps(
		_mm256_add_ps(_mm256_mul_ps(X, _mm256_set1_ps(M.Data[0][1])),
			_mm256_mul_ps(Y, _mm256_set1_ps(M.Data[1][1]))),
		_mm256_add_ps(_mm256_mul_ps(Z, _mm256_set1_ps(M.Data[2][1])),
			_mm256_mul_ps(O, _mm256_set1_ps(M.Data[3][1]))));

	Hz = _mm256_add_ps(
		_mm256_add_ps(_mm256_mul_ps(X, _mm256_set1_ps(M.Data[0][2])),
			_mm256_mul_ps(Y, _mm256_set1_ps(M.Data[1][2]))),
		_mm256_add_ps(_mm256_mul_ps(Z, _mm256_set1_ps(M.Data[2][2])),
			_mm256_mul_ps(O, _mm256_set1_ps(M.Data[3][2]))));

	Hw = _mm256_add_ps(
		_mm256_add_ps(_mm256_mul_ps(X, _mm256_set1_ps(M.Data[0][3])),
			_mm256_mul_ps(Y, _mm256_set1_ps(M.Data[1][3]))),
		_mm256_add_ps(_mm256_mul_ps(Z, _mm256_set1_ps(M.Data[2][3])),
			_mm256_mul_ps(O, _mm256_set1_ps(M.Data[3][3]))));

}

inline bool ProjectToScreen(const FVector& P, const FMatrix& ViewProj, const D3D11_VIEWPORT& VP, float& outX, float& outY, float& outZ, float& outW)
{ 

#if defined(__AVX2__)
	// (x,y,z,1) 브로드캐스트
	const __m256 vx = _mm256_set1_ps(P.X);
	const __m256 vy = _mm256_set1_ps(P.Y);
	const __m256 vz = _mm256_set1_ps(P.Z);
	const __m256 vo = _mm256_set1_ps(1.0f);

	// ViewProj(row-major) 요소 브로드캐스트
	const __m256 m00 = _mm256_set1_ps(ViewProj.Data[0][0]);
	const __m256 m01 = _mm256_set1_ps(ViewProj.Data[0][1]);
	const __m256 m02 = _mm256_set1_ps(ViewProj.Data[0][2]);
	const __m256 m03 = _mm256_set1_ps(ViewProj.Data[0][3]);

	const __m256 m10 = _mm256_set1_ps(ViewProj.Data[1][0]);
	const __m256 m11 = _mm256_set1_ps(ViewProj.Data[1][1]);
	const __m256 m12 = _mm256_set1_ps(ViewProj.Data[1][2]);
	const __m256 m13 = _mm256_set1_ps(ViewProj.Data[1][3]);

	const __m256 m20 = _mm256_set1_ps(ViewProj.Data[2][0]);
	const __m256 m21 = _mm256_set1_ps(ViewProj.Data[2][1]);
	const __m256 m22 = _mm256_set1_ps(ViewProj.Data[2][2]);
	const __m256 m23 = _mm256_set1_ps(ViewProj.Data[2][3]);

	const __m256 m30 = _mm256_set1_ps(ViewProj.Data[3][0]);
	const __m256 m31 = _mm256_set1_ps(ViewProj.Data[3][1]);
	const __m256 m32 = _mm256_set1_ps(ViewProj.Data[3][2]);
	const __m256 m33 = _mm256_set1_ps(ViewProj.Data[3][3]);

	// H = (x,y,z,1) * M
	const __m256 Hx = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vx, m00), _mm256_mul_ps(vy, m10)),
		_mm256_add_ps(_mm256_mul_ps(vz, m20), _mm256_mul_ps(vo, m30)));
	const __m256 Hy = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vx, m01), _mm256_mul_ps(vy, m11)),
		_mm256_add_ps(_mm256_mul_ps(vz, m21), _mm256_mul_ps(vo, m31)));
	const __m256 Hz = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vx, m02), _mm256_mul_ps(vy, m12)),
		_mm256_add_ps(_mm256_mul_ps(vz, m22), _mm256_mul_ps(vo, m32)));
	const __m256 Hw = _mm256_add_ps(_mm256_add_ps(_mm256_mul_ps(vx, m03), _mm256_mul_ps(vy, m13)),
		_mm256_add_ps(_mm256_mul_ps(vz, m23), _mm256_mul_ps(vo, m33)));

	// w 체크(모든 lane 동일), scalar로 추출
	outW = _mm_cvtss_f32(_mm256_castps256_ps128(Hw));
	if (outW <= 0.0f) return false;

	// perspective divide (rcp + Newton 1회)
	__m256 invw = _mm256_rcp_ps(Hw);
	invw = _mm256_mul_ps(invw, _mm256_sub_ps(_mm256_set1_ps(2.0f), _mm256_mul_ps(Hw, invw)));

	const __m256 x_ndc = _mm256_mul_ps(Hx, invw);
	const __m256 y_ndc = _mm256_mul_ps(Hy, invw);
	const __m256 z_ndc = _mm256_mul_ps(Hz, invw);

	// viewport 변환
	const __m256 half = _mm256_set1_ps(0.5f);
	const __m256 vTX = _mm256_set1_ps(VP.TopLeftX);
	const __m256 vTY = _mm256_set1_ps(VP.TopLeftY);
	const __m256 vW = _mm256_set1_ps(VP.Width);
	const __m256 vH = _mm256_set1_ps(VP.Height);

	const __m256 SX = _mm256_add_ps(vTX, _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(x_ndc, half), half), vW));
	const __m256 SY = _mm256_add_ps(vTY, _mm256_mul_ps(_mm256_add_ps(_mm256_mul_ps(_mm256_sub_ps(_mm256_set1_ps(0.0f), y_ndc), half), half), vH));
	const __m256 SZ = _mm256_add_ps(half, _mm256_mul_ps(z_ndc, half));

	// 모든 lane 동일하므로 첫 lane만 사용
	outX = _mm_cvtss_f32(_mm256_castps256_ps128(SX));
	outY = _mm_cvtss_f32(_mm256_castps256_ps128(SY));
	outZ = _mm_cvtss_f32(_mm256_castps256_ps128(SZ));
	return true;

#else
	// 스칼라 경로(기존)
	const FVector4 H = FVector4(P, 1.0f) * ViewProj;
	outW = H.W;
	if (outW <= 0.f) return false;

	const float invW = 1.0f / outW;
	const float x_ndc = H.X * invW;
	const float y_ndc = H.Y * invW;
	const float z_ndc = H.Z * invW;

	outX = VP.TopLeftX + (x_ndc * 0.5f + 0.5f) * VP.Width;
	outY = VP.TopLeftY + (-y_ndc * 0.5f + 0.5f) * VP.Height;
	outZ = z_ndc * 0.5f + 0.5f;
	return true;
#endif
}

inline bool ProjectAABB_ToScreen_AVX2(const FAABB& Box, const FMatrix& VP, const D3D11_VIEWPORT& Vp,
	int ScreenW, int ScreenH, FScreenRect& Out)
{
	const FVector mn = Box.Min, mx = Box.Max;
	const FVector P[8] = {
		{mn.X,mn.Y,mn.Z},{mx.X,mn.Y,mn.Z},{mn.X,mx.Y,mn.Z},{mx.X,mx.Y,mn.Z},
		{mn.X,mn.Y,mx.Z},{mx.X,mn.Y,mx.Z},{mn.X,mx.Y,mx.Z},{mx.X,mx.Y,mx.Z},
	};

	float x[8], y[8], z[8];
	for (int i = 0; i < 8; ++i) { x[i] = P[i].X; y[i] = P[i].Y; z[i] = P[i].Z; }

	__m256 Hx, Hy, Hz, Hw;
	MulPointsByMatrix8_AVX2(x, y, z, VP, Hx, Hy, Hz, Hw);

	// w <= 0 → 실패
	if (_mm256_movemask_ps(_mm256_cmp_ps(Hw, _mm256_set1_ps(0.0f), _CMP_LE_OQ)) != 0)
		return false;

	__m256 xn, yn, zn;
	PerspectiveDivide8_AVX2(Hx, Hy, Hz, Hw, xn, yn, zn);

	__m256 sx, sy, sz;
	NDCToViewport8_AVX2(xn, yn, zn, Vp.TopLeftX, Vp.TopLeftY, Vp.Width, Vp.Height, sx, sy, sz);

	// 스칼라 축약
	alignas(32) float X[8], Y[8], Z[8];
	_mm256_store_ps(X, sx);
	_mm256_store_ps(Y, sy);
	_mm256_store_ps(Z, sz);

	float minx = X[0], maxx = X[0], miny = Y[0], maxy = Y[0], zmin = Z[0];
	auto acc = [&](float a, float b, float c) { if (a < minx)minx = a; if (a > maxx)maxx = a; if (b < miny)miny = b; if (b > maxy)maxy = b; if (c < zmin)zmin = c; };
	for (int i = 1; i < 8; ++i) acc(X[i], Y[i], Z[i]);

	Out.x0 = std::clamp((int)std::floor(minx), 0, ScreenW - 1);
	Out.x1 = std::clamp((int)std::ceil(maxx), 0, ScreenW - 1);
	Out.y0 = std::clamp((int)std::floor(miny), 0, ScreenH - 1);
	Out.y1 = std::clamp((int)std::ceil(maxy), 0, ScreenH - 1);
	Out.zmin = std::clamp(zmin, 0.0f, 1.0f);

	return Out.x0 <= Out.x1 && Out.y0 <= Out.y1;
}

inline bool ProjectAABB_ToScreen(const FAABB& Box,
	const FMatrix& ViewProj,
	const D3D11_VIEWPORT& VP,
	int ScreenW, int ScreenH,
	FScreenRect& OutRect)
{
#if defined(__AVX2__)
	// 네가 만든 AVX2 구현을 그대로 호출
	return ProjectAABB_ToScreen_AVX2(Box, ViewProj, VP, ScreenW, ScreenH, OutRect);
#else
	// 기존 스칼라 본문 그대로 붙이기 (혹은 기존 구현 호출)
	float sx[8], sy[8], sz[8], sw[8];
	const FVector mn = Box.Min, mx = Box.Max;
	const FVector corners[8] = {
		{mn.X, mn.Y, mn.Z}, {mx.X, mn.Y, mn.Z},
		{mn.X, mx.Y, mn.Z}, {mx.X, mx.Y, mn.Z},
		{mn.X, mn.Y, mx.Z}, {mx.X, mn.Y, mx.Z},
		{mn.X, mx.Y, mx.Z}, {mx.X, mx.Y, mx.Z},
	};
	for (int i = 0; i < 8; ++i)
		if (!ProjectToScreen(corners[i], ViewProj, VP, sx[i], sy[i], sz[i], sw[i])) return false;

	float minx = sx[0], maxx = sx[0], miny = sy[0], maxy = sy[0], zmin = sz[0];
	for (int i = 1; i < 8; ++i) {
		minx = std::min(minx, sx[i]); maxx = std::max(maxx, sx[i]);
		miny = std::min(miny, sy[i]); maxy = std::max(maxy, sy[i]);
		zmin = std::min(zmin, sz[i]);
	}
	OutRect.x0 = std::clamp((int)std::floor(minx), 0, ScreenW - 1);
	OutRect.x1 = std::clamp((int)std::ceil(maxx), 0, ScreenW - 1);
	OutRect.y0 = std::clamp((int)std::floor(miny), 0, ScreenH - 1);
	OutRect.y1 = std::clamp((int)std::ceil(maxy), 0, ScreenH - 1);
	OutRect.zmin = std::clamp(zmin, 0.0f, 1.0f);
	return (OutRect.x0 <= OutRect.x1) && (OutRect.y0 <= OutRect.y1);
#endif
}

inline bool ProjectTriangle(const FSoftwareTri& T, const FMatrix& ViewProj, const D3D11_VIEWPORT& VP, FProjectedTri& Out, int ScreenW, int ScreenH)
{
	float w0, w1, w2;
	if (!ProjectToScreen(T.P0, ViewProj, VP, Out.x0, Out.y0, Out.z0, w0)) return false;
	if (!ProjectToScreen(T.P1, ViewProj, VP, Out.x1, Out.y1, Out.z1, w1)) return false;
	if (!ProjectToScreen(T.P2, ViewProj, VP, Out.x2, Out.y2, Out.z2, w2)) return false;

	// 삼각형의 면적으로 culling, pseuedo cross product
	const float area2 = (Out.x1 - Out.x0) * (Out.y2 - Out.y0) - (Out.y1 - Out.y0) * (Out.x2 - Out.x0);
	if (area2 <= 0) return false;

	// 화면 AABB
	const float minx = std::min({ Out.x0, Out.x1, Out.x2 });
	const float maxx = std::max({ Out.x0, Out.x1, Out.x2 });
	const float miny = std::min({ Out.y0, Out.y1, Out.y2 });
	const float maxy = std::max({ Out.y0, Out.y1, Out.y2 });

	// (0.1, 2.2) => (0, 3)
	Out.minX = std::clamp((int)std::floor(minx), 0, ScreenW - 1);
	Out.maxX = std::clamp((int)std::ceil(maxx), 0, ScreenW - 1);
	Out.minY = std::clamp((int)std::floor(miny), 0, ScreenH - 1);
	Out.maxY = std::clamp((int)std::ceil(maxy), 0, ScreenH - 1);

	return (Out.minX <= Out.maxX) && (Out.minY <= Out.maxY);
}

// 픽셀 중심에서 (half-space) edge test (스칼라 폴백에서 사용)
inline bool PointInTri(float px, float py, float a[2], float b[2], float c[2])
{
	auto edge = [](const float p[2], const float v0[2], const float v1[2])
		{
			return (p[1] - v0[1]) * (v1[0] - v0[0]) - (p[0] - v0[0]) * (v1[1] - v0[1]);
		};
	float p[2] = { px, py };
	const float e0 = edge(p, a, b);
	const float e1 = edge(p, b, c);
	const float e2 = edge(p, c, a);

	return (e0 >= 0 && e1 >= 0 && e2 >= 0);
}

// --- AVX2 유틸: 32픽셀(한 행) 커버리지 마스크 생성 ------------------------------------
#if defined(__AVX2__)
static FORCEINLINE uint32 BuildRowCoverageMask32_AVX2(
	float PxStart, float Py,
	float A0, float B0, float C0,
	float A1, float B1, float C1,
	float A2, float B2, float C2)
{
	// 32픽셀 = 8픽셀 × 4블록
	const __m256 idx = _mm256_set_ps(7.f, 6.f, 5.f, 4.f, 3.f, 2.f, 1.f, 0.f);
	const __m256 yv = _mm256_set1_ps(Py);
	const __m256 z = _mm256_setzero_ps();

	const __m256 A0v = _mm256_set1_ps(A0), B0v = _mm256_set1_ps(B0), C0v = _mm256_set1_ps(C0);
	const __m256 A1v = _mm256_set1_ps(A1), B1v = _mm256_set1_ps(B1), C1v = _mm256_set1_ps(C1);
	const __m256 A2v = _mm256_set1_ps(A2), B2v = _mm256_set1_ps(B2), C2v = _mm256_set1_ps(C2);

	uint32 mask32 = 0;

	auto eval_edge = [&](const __m256& A, const __m256& B, const __m256& C, const __m256& x) -> __m256
	{
#if defined(__FMA__)
		__m256 t = _mm256_fmadd_ps(A, x, C);      // A*x + C
		return _mm256_fmadd_ps(B, yv, t);         // B*y + (A*x + C)
#else
		__m256 t = _mm256_add_ps(_mm256_mul_ps(A, x), C);
		return _mm256_add_ps(_mm256_mul_ps(B, yv), t);
#endif
	};

	for (int block = 0; block < 4; ++block)
	{
		const __m256 base = _mm256_set1_ps(PxStart + float(block * 8));
		const __m256 px = _mm256_add_ps(idx, base);

		const __m256 e0 = eval_edge(A0v, B0v, C0v, px);
		const __m256 e1 = eval_edge(A1v, B1v, C1v, px);
		const __m256 e2 = eval_edge(A2v, B2v, C2v, px);

		const __m256 ge0 = _mm256_cmp_ps(e0, z, _CMP_GE_OQ);
		const __m256 ge1 = _mm256_cmp_ps(e1, z, _CMP_GE_OQ);
		const __m256 ge2 = _mm256_cmp_ps(e2, z, _CMP_GE_OQ);

		const __m256 ok = _mm256_and_ps(ge0, _mm256_and_ps(ge1, ge2));
		const uint32 m8 = static_cast<uint32>(_mm256_movemask_ps(ok)) & 0xFFu;

		mask32 |= (m8 << (block * 8));
	}
	return mask32;
}
#endif
// --------------------------------------------------------------------------------------

// 타일 내 32 x 8 coverage
//TODO SIMD
inline uint32_t RangeMask32(int rx0, int rx1)
{
	rx0 = std::clamp(rx0, 0, 31);
	rx1 = std::clamp(rx1, 0, 31);

	if (rx0 > rx1) return 0u;

	uint32_t L = (rx0 == 0) ? 0xFFFFFFFFu : (~0u << rx0);
	uint32_t R = (rx1 == 31) ? 0xFFFFFFFFu : (~0u >> (31 - rx1));

	return L & R;
}

inline void BuildCoverageMask_Scanline(const FProjectedTri& tri, int tileX, int tileY, uint32_t outMask[MSOC_TILE_H])
{
	for (int r = 0; r < MSOC_TILE_H; ++r) outMask[r] = 0u;

	const int x0 = tileX * MSOC_TILE_W;
	const int y0 = tileY * MSOC_TILE_H;

	const int lx0 = std::max(0, tri.minX - x0);
	const int ly0 = std::max(0, tri.minY - y0);
	const int lx1 = std::min(MSOC_TILE_W - 1, tri.maxX - x0);
	const int ly1 = std::min(MSOC_TILE_H - 1, tri.maxY - y0);

	if (lx0 > lx1 || ly0 > ly1) return;

	// 2D edge
	auto edgeX = [](float y, float x0, float y0, float x1, float y1, float& xL, float& xR) -> bool
		{
			// 수평 에지는 한쪽만 포함(top-left 규칙 비슷하게): 여기서는 보수적으로 양쪽 다 포함
			if ((y < std::min(y0, y1)) || (y > std::max(y0, y1))) return false;
			if (y0 == y1) { xL = std::min(x0, x1); xR = std::max(x0, x1); return true; }
			float t = (y - y0) / (y1 - y0);
			float x = x0 + t * (x1 - x0);
			xL = xR = x;
			return true;
		};
	 
	// 편의
	const float Ax = tri.x0, Ay = tri.y0;
	const float Bx = tri.x1, By = tri.y1;
	const float Cx = tri.x2, Cy = tri.y2;

	for (int ry = ly0; ry <= ly1; ++ry)
	{
		const float py = (float)(y0 + ry) + 0.5f;

		// 각 에지와의 교차 X를 수집
		float xs[6]; int n = 0;
		float xL, xR;
		if (edgeX(py, Ax, Ay, Bx, By, xL, xR)) { xs[n++] = xL; if (xR != xL) xs[n++] = xR; }
		if (edgeX(py, Bx, By, Cx, Cy, xL, xR)) { xs[n++] = xL; if (xR != xL) xs[n++] = xR; }
		if (edgeX(py, Cx, Cy, Ax, Ay, xL, xR)) { xs[n++] = xL; if (xR != xL) xs[n++] = xR; }
		if (n < 2) continue;

		// 교차점들을 정렬 → [x0,x1],[x2,x3]... 페어로 채움
		std::sort(xs, xs + n);
		uint32_t rowMask = 0u;
		for (int k = 0; k + 1 < n; k += 2)
		{
			const float segL = xs[k];
			const float segR = xs[k + 1];

			// 하프픽셀 샘플을 포함하는 보수적 정수 범위
			int rx0i = (int)std::floor(segL - (float)x0 + 0.5f);
			int rx1i = (int)std::floor(segR - (float)x0 - 0.5f);
			rowMask |= RangeMask32(rx0i, rx1i);
		}
		outMask[ry] = rowMask;
	}

}

inline void BuildConverageMask_Conservative(const FProjectedTri& tri, int tileX, int tileY, uint32_t outMask[MSOC_TILE_H])
{
	for (int r = 0; r < MSOC_TILE_H; ++r)
	{
		outMask[r] = 0u;
	}

	// 32  * 8 batching
	const int x0 = tileX * MSOC_TILE_W;
	const int y0 = tileY * MSOC_TILE_H;

	float A[2] = { tri.x0, tri.y0 };
	float B[2] = { tri.x1, tri.y1 };
	float C[2] = { tri.x2, tri.y2 };

	// 삼각형 AABB와 타일 경계의 교집합 (타일 로컬 좌표)
	const int lx0 = std::max(0, tri.minX - x0);
	const int ly0 = std::max(0, tri.minY - y0);

	const int lx1 = std::min(MSOC_TILE_W - 1, tri.maxX - x0);
	const int ly1 = std::min(MSOC_TILE_H - 1, tri.maxY - y0);

#if defined(__AVX2__)
	// Edge 계수 (반시계 half-space: (x2-x1, y2-y1) × (P - v1) >= 0)
	// E(P) = A*x + B*y + C,   A = (y1 - y2), B = (x2 - x1), C = x1*y2 - y1*x2
	const float Ax0 = (A[1] - B[1]), Bx0 = (B[0] - A[0]), Cx0 = (A[0] * B[1] - A[1] * B[0]);
	const float Ax1 = (B[1] - C[1]), Bx1 = (C[0] - B[0]), Cx1 = (B[0] * C[1] - B[1] * C[0]);
	const float Ax2 = (C[1] - A[1]), Bx2 = (A[0] - C[0]), Cx2 = (C[0] * A[1] - C[1] * A[0]);

	for (int ry = ly0; ry <= ly1; ++ry)
	{
		const float py = float(y0 + ry) + 0.5f;
		// PxStart = (타일 절대 x 시작 + 0.5)
		const float PxStart = float(x0) + 0.5f;

		// 32비트 행 마스크 생성 후, [lx0..lx1] 범위만 남김
		uint32_t rowMask = BuildRowCoverageMask32_AVX2(
			PxStart, py,
			Ax0, Bx0, Cx0,
			Ax1, Bx1, Cx1,
			Ax2, Bx2, Cx2
		);

		const uint32_t left = (lx0 == 0) ? 0xFFFFFFFFu : (~0u << lx0);
		const uint32_t right = (lx1 == 31) ? 0xFFFFFFFFu : (~0u >> (31 - lx1));
		rowMask &= (left & right);

		outMask[ry] |= rowMask;
	}
#else
	// 스칼라 폴백
	for (int ry = ly0; ry <= ly1; ++ry)
	{
		const float py = (float)(y0 + ry) + 0.5f;
		uint32_t rowMask = 0u;

		for (int rx = lx0; rx <= lx1; ++rx)
		{
			const float px = (float)(x0 + rx) + 0.5f;
			if (PointInTri(px, py, A, B, C))
			{
				rowMask |= (1u << rx);
			}
		}

		outMask[ry] |= rowMask;
	}
#endif
}

inline bool IsFullMask(const uint32_t cov[MSOC_TILE_H])
{
	// 256 bit = 32bit * 8 (모든 행이 0xFFFFFFFF 이어야 full)
	for (int r = 0; r < MSOC_TILE_H; ++r)
	{
		if (cov[r] != 0xFFFFFFFFu)
		{
			return false;
		}
	}
	return true;
}
inline void UpdateTileWithTri(FMaskedTile& tile, const uint32_t cov[MSOC_TILE_H], float ZtriMax)
{
	// 이번 타일 Ztri_max가 reference보다 깊으면 개선 없음 → skip
	if (ZtriMax >= tile.Z0max) return;

	// Discard 휴리스틱 ( dist1t > dist01 => working 폐기 )
	const float dist1t = tile.Z1max - ZtriMax;
	const float dist01 = tile.Z0max - tile.Z1max;

	if (dist1t > dist01)
	{
		tile.Z1max = 0.0f;
#if defined(__AVX2__)
		// CoverageMask[8] = 0
		_mm256_store_si256(reinterpret_cast<__m256i*>(tile.CoverageMask), _mm256_setzero_si256());
#else
		for (int r = 0; r < MSOC_TILE_H; ++r) tile.CoverageMask[r] = 0u;
#endif
		tile.bFullCovered = false;
	}

	// working 레이어 Z 갱신
	tile.Z1max = std::max(tile.Z1max, ZtriMax);

#if defined(__AVX2__)
	// cov(unaligned) | tile.CoverageMask(aligned) → merged
	const __m256i oldCov = _mm256_load_si256(reinterpret_cast<const __m256i*>(tile.CoverageMask)); // FMaskedTile가 alignas(32)면 OK
	const __m256i addCov = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cov));               // cov는 로컬 배열 → loadu
	const __m256i merged = _mm256_or_si256(oldCov, addCov);

	// 저장
	_mm256_store_si256(reinterpret_cast<__m256i*>(tile.CoverageMask), merged);

	// full mask? (각 lane == 0xFFFFFFFF)
	const __m256i fullOnes = _mm256_set1_epi32(-1);
	const __m256i eqFull = _mm256_cmpeq_epi32(merged, fullOnes);
	const int     allBytes = _mm256_movemask_epi8(eqFull);

	if (allBytes == -1) // 32바이트 전부 0xFF → 8행 모두 full
	{
		tile.Z0max = tile.Z1max;
		tile.Z1max = 0.0f;
		_mm256_store_si256(reinterpret_cast<__m256i*>(tile.CoverageMask), _mm256_setzero_si256());
		tile.bFullCovered = true;
	}
#else
	// 스칼라 폴백
	for (int r = 0; r < MSOC_TILE_H; ++r)
		tile.CoverageMask[r] |= cov[r];

	if (IsFullMask(tile.CoverageMask))
	{
		tile.Z0max = tile.Z1max;
		tile.Z1max = 0.f;
		for (int r = 0; r < MSOC_TILE_H; ++r) tile.CoverageMask[r] = 0u;
		tile.bFullCovered = true;
	}
#endif
}

inline bool RectFullyCoversTile(const FScreenRect& r, int tx, int ty)
{
	const int tileX0 = tx * MSOC_TILE_W;
	const int tileY0 = ty * MSOC_TILE_H;
	const int tileX1 = tileX0 + MSOC_TILE_W - 1;
	const int tileY1 = tileY0 + MSOC_TILE_H - 1;

	return (r.x0 <= tileX0 && r.x1 >= tileX1 &&
		r.y0 <= tileY0 && r.y1 >= tileY1);
}

inline uint32_t RowRangeMask_InTile(const FScreenRect& r, int tx)
{
	const int tileX0 = tx * MSOC_TILE_W;

	const int rx0 = std::clamp(r.x0 - tileX0, 0, MSOC_TILE_W - 1);
	const int rx1 = std::clamp(r.x1 - tileX0, 0, MSOC_TILE_W - 1);

	const uint32_t left = (~0u) << rx0;                 // rx0==0이면 0xFFFFFFFF
	const uint32_t right = (~0u) >> (31 - rx1);          // rx1==31이면 0xFFFFFFFF

	return left & right;
}

static void AppendAABBAsTris(const FAABB& B, TArray<FSoftwareTri>& Out)
{
	const FVector mn = B.Min, mx = B.Max;

	const FVector p000{ mn.X, mn.Y, mn.Z };
	const FVector p100{ mx.X, mn.Y, mn.Z };
	const FVector p010{ mn.X, mx.Y, mn.Z };
	const FVector p110{ mx.X, mx.Y, mn.Z };
	const FVector p001{ mn.X, mn.Y, mx.Z };
	const FVector p101{ mx.X, mn.Y, mx.Z };
	const FVector p011{ mn.X, mx.Y, mx.Z };
	const FVector p111{ mx.X, mx.Y, mx.Z };

	auto tri = [&](FVector a, FVector b, FVector c)
		{
			FSoftwareTri t; t.P0 = a; t.P1 = b; t.P2 = c; Out.Add(t);
		};
	// -Z face
	tri(p000, p100, p110); tri(p000, p110, p010);
	// +Z face
	tri(p001, p111, p101); tri(p001, p011, p111);
	// -X face
	tri(p000, p010, p011); tri(p000, p011, p001);
	// +X face
	tri(p100, p101, p111); tri(p100, p111, p110);
	// -Y face
	tri(p000, p001, p101); tri(p000, p101, p100);
	// +Y face
	tri(p010, p110, p111); tri(p010, p111, p011);
}

struct FMaskedHiZBuffer
{
	int ScreenW = 0, ScreenH = 0;
	int TilesX = 0, TilesY = 0;
	TArray<FMaskedTile> Tiles; // TilesX * TilesY

	void Allocate(int W, int H)
	{
		ScreenW = W;
		ScreenH = H;

		TilesX = (W + MSOC_TILE_W - 1) / MSOC_TILE_W;
		TilesY = (H + MSOC_TILE_H - 1) / MSOC_TILE_H;
		Tiles.resize(TilesX * TilesY);
	}

	void Clear()
	{
		for (auto& T : Tiles)
		{
			for (int r = 0; r < MSOC_TILE_H; ++r)
			{
				T.CoverageMask[r] = 0u;
			}
			T.Z0max = 1.0f;
			T.Z1max = 1.0f;
			T.bFullCovered = false;
		}
	}
};

class USoftwareOcclusionCuller
{
public:
	void Init(int ScreenW, int ScreenH)
	{
		HiZ.Allocate(ScreenW, ScreenH);
		HiZ.Clear();
	}

	void Resize(int ScreenW, int ScreenH)
	{
		HiZ.Allocate(ScreenW, ScreenH);
		HiZ.Clear();
	}

	void BeginFrame(const FMatrix& InViewProj, const D3D11_VIEWPORT& InVP)
	{
		ViewProj = InViewProj;
		VP = InVP;
		HiZ.Clear();
	}

	void RasterizeOcculuderTriangles(const TArray<FSoftwareTri>& Tris);
	bool TestAABB(const FAABB& Box) const;

	void DebugOverlay() const {}

	const FMaskedHiZBuffer& GetHiZ() const { return HiZ; }

private:
	FMaskedHiZBuffer HiZ;
	FMatrix          ViewProj;
	D3D11_VIEWPORT   VP{};
};

