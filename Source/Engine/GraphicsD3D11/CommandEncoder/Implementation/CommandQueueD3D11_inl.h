
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D11::GetNextFenceValue() const
{
  return m_uiCompletedFenceValue + 1;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D11::GetCompletedFenceValue() const
{
  return m_uiCompletedFenceValue;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D11::WaitForIdle()
{
  return m_uiCompletedFenceValue;
}
