#include "pch.h"
#include "Render/Cull/MSOC.h"
#if defined(__AVX2__)
#include <immintrin.h>
#endif
// 타일 4코너만으로 FULL/MISS/PARTIAL 판정
enum class ETileClass { Full, Miss, Partial };
// ─────────────────────────────────────────────────────────────
// 타일 4코너 Full/Miss/Partial 분류 (SIMD)
//   - 삼각형은 CCW(이미 area2>0로 보장) 기준
//   - FULL  : 모든 에지에서 4코너 모두 >= 0
//   - MISS  : 어떤 에지든 4코너 모두 < 0
//   - PART  : 나머지
// ─────────────────────────────────────────────────────────────


static FORCEINLINE ETileClass ClassifyTileByCorners(const FProjectedTri& t, int tx, int ty)
{
	const float fx0 = tx * MSOC_TILE_W;
	const float fy0 = ty * MSOC_TILE_H;
	const float fx1 = fx0 + (MSOC_TILE_W - 1);
	const float fy1 = fy0 + (MSOC_TILE_H - 1);

#if defined(__AVX2__)
	// 4개 코너를 벡터로 묶기: (x0,y0),(x1,y0),(x0,y1),(x1,y1)
	const __m128 px = _mm_setr_ps(fx0, fx1, fx0, fx1);
	const __m128 py = _mm_setr_ps(fy0, fy0, fy1, fy1);
	const __m128 z = _mm_set1_ps(0.0f);

	// 각 에지의 계수 (E(P)=A*x + B*y + C). CCW 규칙:
	// e0: v0->v1, e1: v1->v2, e2: v2->v0
	const float A0 = (t.y0 - t.y1), B0 = (t.x1 - t.x0), C0 = (t.x0 * t.y1 - t.y0 * t.x1);
	const float A1 = (t.y1 - t.y2), B1 = (t.x2 - t.x1), C1 = (t.x1 * t.y2 - t.y1 * t.x2);
	const float A2 = (t.y2 - t.y0), B2 = (t.x0 - t.x2), C2 = (t.x2 * t.y0 - t.y2 * t.x0);

	const __m128 A0v = _mm_set1_ps(A0), B0v = _mm_set1_ps(B0), C0v = _mm_set1_ps(C0);
	const __m128 A1v = _mm_set1_ps(A1), B1v = _mm_set1_ps(B1), C1v = _mm_set1_ps(C1);
	const __m128 A2v = _mm_set1_ps(A2), B2v = _mm_set1_ps(B2), C2v = _mm_set1_ps(C2);

	auto edge_eval = [&](const __m128& A, const __m128& B, const __m128& C) -> __m128 {
#if defined(__FMA__)
		// e = A*px + B*py + C
		return _mm_fmadd_ps(B, py, _mm_fmadd_ps(A, px, C));
#else
		return _mm_add_ps(_mm_add_ps(_mm_mul_ps(A, px), _mm_mul_ps(B, py)), C);
#endif
	};

	const __m128 e0 = edge_eval(A0v, B0v, C0v);
	const __m128 e1 = edge_eval(A1v, B1v, C1v);
	const __m128 e2 = edge_eval(A2v, B2v, C2v);

	// 각 에지에 대해 (e >= 0) 마스크(4비트). 0xF면 4코너 모두 inside.
	const int m0_ge = _mm_movemask_ps(_mm_cmp_ps(e0, z, _CMP_GE_OQ));
	const int m1_ge = _mm_movemask_ps(_mm_cmp_ps(e1, z, _CMP_GE_OQ));
	const int m2_ge = _mm_movemask_ps(_mm_cmp_ps(e2, z, _CMP_GE_OQ));

	// MISS: 어떤 에지든 4코너 모두 (e < 0) → (e>=0) 마스크가 0
	if ((m0_ge == 0) | (m1_ge == 0) | (m2_ge == 0))
		return ETileClass::Miss;

	// FULL: 모든 에지에서 4코너 모두 (e >= 0)
	if ((m0_ge == 0xF) & (m1_ge == 0xF) & (m2_ge == 0xF))
		return ETileClass::Full;

	return ETileClass::Partial;
#else
	// 스칼라 폴백(기존 로직)
	auto edge = [](float px, float py, float ax, float ay, float bx, float by) {
		return (px - ax) * (by - ay) - (py - ay) * (bx - ax);
		};
	const float e0c0 = edge(fx0, fy0, t.x0, t.y0, t.x1, t.y1);
	const float e0c1 = edge(fx1, fy0, t.x0, t.y0, t.x1, t.y1);
	const float e0c2 = edge(fx0, fy1, t.x0, t.y0, t.x1, t.y1);
	const float e0c3 = edge(fx1, fy1, t.x0, t.y0, t.x1, t.y1);

	const float e1c0 = edge(fx0, fy0, t.x1, t.y1, t.x2, t.y2);
	const float e1c1 = edge(fx1, fy0, t.x1, t.y1, t.x2, t.y2);
	const float e1c2 = edge(fx0, fy1, t.x1, t.y1, t.x2, t.y2);
	const float e1c3 = edge(fx1, fy1, t.x1, t.y1, t.x2, t.y2);

	const float e2c0 = edge(fx0, fy0, t.x2, t.y2, t.x0, t.y0);
	const float e2c1 = edge(fx1, fy0, t.x2, t.y2, t.x0, t.y0);
	const float e2c2 = edge(fx0, fy1, t.x2, t.y2, t.x0, t.y0);
	const float e2c3 = edge(fx1, fy1, t.x2, t.y2, t.x0, t.y0);

	const float e0min = std::min(std::min(e0c0, e0c1), std::min(e0c2, e0c3));
	const float e1min = std::min(std::min(e1c0, e1c1), std::min(e1c2, e1c3));
	const float e2min = std::min(std::min(e2c0, e2c1), std::min(e2c2, e2c3));
	if (e0min >= 0 && e1min >= 0 && e2min >= 0) return ETileClass::Full;

	const float e0max = std::max(std::max(e0c0, e0c1), std::max(e0c2, e0c3));
	const float e1max = std::max(std::max(e1c0, e1c1), std::max(e1c2, e1c3));
	const float e2max = std::max(std::max(e2c0, e2c1), std::max(e2c2, e2c3));
	if (e0max < 0 || e1max < 0 || e2max < 0) return ETileClass::Miss;

	return ETileClass::Partial;
#endif
}


//void USoftwareOcclusionCuller::RasterizeOcculuderTriangles(const TArray<FSoftwareTri>& Tris)
//{
//	const int ScreenW = HiZ.ScreenW;
//	const int ScreenH = HiZ.ScreenH;
//
//	for (const FSoftwareTri& T : Tris)
//	{
//		FProjectedTri PT;
//
//		if (!ProjectTriangle(T, ViewProj, VP, PT, ScreenW, ScreenH))
//		{
//			continue;
//		}
//
//		// 정점 z 최대 
//		const float ZtriMax = std::max({ PT.z0, PT.z1, PT.z2 });
//
//		// PT: Pixel Position (Screen Space)
//		const int tx0 = std::clamp(PT.minX / MSOC_TILE_W, 0, HiZ.TilesX - 1);
//		const int ty0 = std::clamp(PT.minY / MSOC_TILE_H, 0, HiZ.TilesY - 1);
//		const int tx1 = std::clamp(PT.maxX / MSOC_TILE_W, 0, HiZ.TilesX - 1);
//		const int ty1 = std::clamp(PT.maxY / MSOC_TILE_H, 0, HiZ.TilesY - 1);
//
//		// Make Coverage
//		for (int ty = ty0; ty <= ty1; ++ty)
//		{
//			for (int tx = tx0; tx <= tx1; ++tx)
//			{
//				FMaskedTile& Tile = HiZ.Tiles[ty * HiZ.TilesX + tx];
//
//				// 더 좋아질 기미가 없으면 pass
//				if (ZtriMax >= Tile.Z0max) continue;
//
//				// Make 32 * 8 Coverage  
//				uint32_t cov[MSOC_TILE_H];
//#if defined(__AVX2__)
//				BuildConverageMask_Conservative(PT, tx, ty, cov);
//#else
//				//BuildCoverageMask_Scanline(PT, tx, ty, cov);
//
//#endif
//				bool anyBit = false;
//#if defined(__AVX2__)
//				{
//					// cov[8] 전체가 0인지 한 번에 검사 → 0이면 testz=1
//					const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cov));
//					anyBit = (_mm256_testz_si256(v, v) == 0);
//				}
//#else
//				for (int r = 0; r < MSOC_TILE_H; ++r)
//				{
//					if (cov[r]) { anyBit = true; break; }
//				}
//#endif
//
//				if (!anyBit) continue;
//
//				UpdateTileWithTri(Tile, cov, ZtriMax);
//			}
//		}
//	}
//}

 
void USoftwareOcclusionCuller::RasterizeOcculuderTriangles(const TArray<FSoftwareTri>& Tris)
{
	const int ScreenW = HiZ.ScreenW;
	const int ScreenH = HiZ.ScreenH;

	for (const FSoftwareTri& T : Tris)
	{
		FProjectedTri PT;
		if (!ProjectTriangle(T, ViewProj, VP, PT, ScreenW, ScreenH))
			continue;

		const float ZtriMax = std::max({ PT.z0, PT.z1, PT.z2 });

		const int tx0 = std::clamp(PT.minX / MSOC_TILE_W, 0, HiZ.TilesX - 1);
		const int ty0 = std::clamp(PT.minY / MSOC_TILE_H, 0, HiZ.TilesY - 1);
		const int tx1 = std::clamp(PT.maxX / MSOC_TILE_W, 0, HiZ.TilesX - 1);
		const int ty1 = std::clamp(PT.maxY / MSOC_TILE_H, 0, HiZ.TilesY - 1);

		for (int ty = ty0; ty <= ty1; ++ty)
		{
			for (int tx = tx0; tx <= tx1; ++tx)
			{
				FMaskedTile& Tile = HiZ.Tiles[ty * HiZ.TilesX + tx];

				// 레퍼런스(Z0max)가 더 앞이면 이 삼각형으로는 개선 불가
				if (ZtriMax >= Tile.Z0max) continue;

				// ── ★ 선분류: Full/Miss/Partial
				const ETileClass tc = ClassifyTileByCorners(PT, tx, ty);
				if (tc == ETileClass::Miss) continue;

				if (tc == ETileClass::Full)
				{ 
					if (ZtriMax < Tile.Z0max)
						Tile.Z0max = ZtriMax;
					 
					Tile.Z1max = 0.0f;
#if defined(__AVX2__)
						_mm256_store_si256(reinterpret_cast<__m256i*>(Tile.CoverageMask),
							_mm256_setzero_si256());
#else
					for (int r = 0; r < MSOC_TILE_H; ++r) Tile.CoverageMask[r] = 0u;
#endif
					Tile.bFullCovered = true;
					continue;
				}

				// ── Partial 타일만 커버리지 생성
				uint32_t cov[MSOC_TILE_H];
#if defined(__AVX2__)
				BuildConverageMask_Conservative(PT, tx, ty, cov);
#else
				BuildCoverageMask_Scanline(PT, tx, ty, cov);
#endif

				bool anyBit = false;
#if defined(__AVX2__)
				{
					const __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(cov));
					anyBit = (_mm256_testz_si256(v, v) == 0);
				}
#else
				for (int r = 0; r < MSOC_TILE_H; ++r)
					if (cov[r]) { anyBit = true; break; }
#endif

				if (!anyBit) continue;

				UpdateTileWithTri(Tile, cov, ZtriMax);
			}
		}
	}
}





bool USoftwareOcclusionCuller::TestAABB(const FAABB& Box) const
{
	const int W = HiZ.ScreenW, H = HiZ.ScreenH;

	FScreenRect R;
	if (!ProjectAABB_ToScreen(Box, ViewProj, VP, W, H, R))
		return false; // 투영 실패 또는 화면 밖 => 오클루전 판정 생략

	// 타일 범위
	int tx0 = R.x0 / MSOC_TILE_W, tx1 = R.x1 / MSOC_TILE_W;
	int ty0 = R.y0 / MSOC_TILE_H, ty1 = R.y1 / MSOC_TILE_H;

	tx0 = std::clamp(tx0, 0, HiZ.TilesX - 1);
	tx1 = std::clamp(tx1, 0, HiZ.TilesX - 1);
	ty0 = std::clamp(ty0, 0, HiZ.TilesY - 1);
	ty1 = std::clamp(ty1, 0, HiZ.TilesY - 1);

	// 모든 교차 타일에서 가림이 만족해야 전체가 occluded
	for (int ty = ty0; ty <= ty1; ++ty)
	{
		for (int tx = tx0; tx <= tx1; ++tx)
		{
			const FMaskedTile& T = HiZ.Tiles[ty * HiZ.TilesX + tx];

			if (RectFullyCoversTile(R, tx, ty))
			{
				const float Ztile = std::max(T.Z0max, T.Z1max);
				if (!(R.zmin > Ztile))
					return false; // 이 타일에서 가림 실패 => 전체 실패
 				continue;
			}

			// 부분 교차
			const int tileY0 = ty * MSOC_TILE_H;
			const int ry0 = std::clamp(R.y0 - tileY0, 0, MSOC_TILE_H - 1);
			const int ry1 = std::clamp(R.y1 - tileY0, 0, MSOC_TILE_H - 1);

			if (T.bFullCovered)
			{
				// 타일 전체 커버로 간주(Reference 레이어)
				if (!(R.zmin > T.Z0max))
					return false;
				continue;
			}

			// Working 레이어의 커버리지 마스크로 행별 완전 커버 확인
			const uint32_t needMask = RowRangeMask_InTile(R, tx);

#if defined(__AVX2__)
			{
				// 8행을 한 번에 검사:
				// fail[r] = (((CoverageMask[r] & needMask) ^ needMask) != 0) 인 래인
				const __m256i covVec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(T.CoverageMask));
				const __m256i need = _mm256_set1_epi32(static_cast<int>(needMask));
				const __m256i andv = _mm256_and_si256(covVec, need);
				const __m256i xorv = _mm256_xor_si256(andv, need); // 0이면 통과, !=0이면 실패

				// 활성 행(ry0..ry1)만 검사하도록 마스크 구성
				alignas(32) uint32_t actArr[MSOC_TILE_H] = {};
				for (int i = ry0; i <= ry1; ++i) actArr[i] = 0xFFFFFFFFu;
				const __m256i actMask = _mm256_load_si256(reinterpret_cast<const __m256i*>(actArr));

				const __m256i fail = _mm256_and_si256(xorv, actMask);
				// fail에 하나라도 비트가 있으면 테스트 실패
				if (_mm256_testz_si256(fail, fail) == 0)
					return false;
			}
#else
			for (int r = ry0; r <= ry1; ++r)
			{
				// 박스가 덮는 행 구간이 타일 커버리지에 완전히 포함되어야 함
				if ((T.CoverageMask[r] & needMask) != needMask)
					return false; // 한 행이라도 빈 구간 → 보수적 실패
			}
#endif

			// 모든 교차 행이 완전히 커버 → 깊이 비교(Working 레이어)
			if (!(R.zmin > T.Z1max))
				return false;
		}
	}
	return true; // 모든 타일에서 가림 보장 → occluded
}
