#pragma once

XII_ALWAYS_INLINE void xiiSimdMat4f::Transpose()
{
  xiiMath::Swap(m_col0.m_v.y, m_col1.m_v.x);
  xiiMath::Swap(m_col0.m_v.z, m_col2.m_v.x);
  xiiMath::Swap(m_col0.m_v.w, m_col3.m_v.x);
  xiiMath::Swap(m_col1.m_v.z, m_col2.m_v.y);
  xiiMath::Swap(m_col1.m_v.w, m_col3.m_v.y);
  xiiMath::Swap(m_col2.m_v.w, m_col3.m_v.z);
}
