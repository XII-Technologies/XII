xiiInt32 xiiImageFilterWeights::GetFirstSourceSampleIndex(xiiUInt32 dstSampleIndex) const
{
  xiiSimdFloat dstSampleInSourceSpace = (xiiSimdFloat(dstSampleIndex) + xiiSimdFloat(0.5f)) * m_fDestToSourceScale;

  return xiiInt32(xiiMath::Floor(dstSampleInSourceSpace - m_fWidthInSourceSpace));
}

inline xiiArrayPtr<const float> xiiImageFilterWeights::ViewWeights() const
{
  return m_Weights;
}
