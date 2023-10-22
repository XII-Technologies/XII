
XII_ALWAYS_INLINE void xiiGALComputeCommandEncoder::CountDispatchCall()
{
  ++m_uiDispatchCalls;
}

XII_ALWAYS_INLINE void xiiGALComputeCommandEncoder::ClearStatisticsCounters()
{
  xiiGALCommandEncoder::ClearStatisticsCounters();

  m_uiDispatchCalls = 0U;
}
