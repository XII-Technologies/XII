#pragma once

#ifndef __MM_TRANSPOSE4_PD

// Derived from Chase R. Lewis https://stackoverflow.com/questions/36167517/m256d-transpose4-equivalent

#  define __MM_TRANSPOSE4_PD(row0, row1, row2, row3)     \
    {                                                    \
      __m256d tmp3, tmp2, tmp1, tmp0;                    \
                                                         \
      tmp0 = _mm256_shuffle_pd((row0), (row1), 0x0);     \
      tmp2 = _mm256_shuffle_pd((row0), (row1), 0xF);     \
      tmp1 = _mm256_shuffle_pd((row2), (row3), 0x0);     \
      tmp3 = _mm256_shuffle_pd((row2), (row3), 0xF);     \
                                                         \
      (row0) = _mm256_permute2f128_pd(tmp0, tmp1, 0x20); \
      (row1) = _mm256_permute2f128_pd(tmp2, tmp3, 0x20); \
      (row2) = _mm256_permute2f128_pd(tmp0, tmp1, 0x31); \
      (row3) = _mm256_permute2f128_pd(tmp2, tmp3, 0x31); \
    }

#endif // !__MM_TRANSPOSE4_PD(row0, row1, row2, row3)

XII_ALWAYS_INLINE void xiiSimdMat4d::Transpose()
{
  __MM_TRANSPOSE4_PD(m_col0.m_v, m_col1.m_v, m_col2.m_v, m_col3.m_v);
}
