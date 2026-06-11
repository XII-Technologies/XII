/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiInt32 xiiImageFilterWeights::GetFirstSourceSampleIndex(xiiUInt32 uiDstSampleIndex) const
{
  const xiiSimdFloat dstSampleInSourceSpace = (xiiSimdFloat(uiDstSampleIndex) + xiiSimdFloat(0.5f)) * m_fDestToSourceScale;

  return xiiInt32(xiiMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

XII_ALWAYS_INLINE xiiArrayPtr<const float> xiiImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
