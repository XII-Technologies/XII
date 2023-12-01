
XII_ALWAYS_INLINE void xiiGALGraphicsCommandEncoder::ClearStatisticsCounters()
{
  xiiGALCommandEncoder::ClearStatisticsCounters();

  m_uiDrawCalls = 0;
}

XII_ALWAYS_INLINE xiiEnum<xiiGALPrimitiveTopology> xiiGALGraphicsCommandEncoder::GetPrimitiveTopology()
{
  return m_GraphicsState.m_Topology;
}

XII_ALWAYS_INLINE void xiiGALGraphicsCommandEncoder::CountDrawCall()
{
  ++m_uiDrawCalls;
}
