
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetNextFenceValue() const
{
  return m_uiCompletedFenceValue + 1;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetCompletedFenceValue()
{
  return m_uiCompletedFenceValue;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  return m_uiCompletedFenceValue;
}
