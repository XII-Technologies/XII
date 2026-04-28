/// Copyright (c) Theophilus Eriata. All Rights Reserved.

xiiInt32 xiiImageFilterWeights::GetFirstSourceSampleIndex(xiiUInt32 uiDstSampleIndex) const
{
  xiiSimdFloat dstSampleInSourceSpace = (xiiSimdFloat(uiDstSampleIndex) + xiiSimdFloat(0.5f)) * m_fDestToSourceScale;

  return xiiInt32(xiiMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline xiiArrayPtr<const float> xiiImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
