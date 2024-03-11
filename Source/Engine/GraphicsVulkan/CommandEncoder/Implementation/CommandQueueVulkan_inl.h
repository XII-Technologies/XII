
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetNextFenceValue() const
{
  return m_uiCompletedFenceValue + 1;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetCompletedFenceValue() const
{
  return m_uiCompletedFenceValue;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  m_pContext->EnqueueSignal(m_pFence, ++m_uiCompletedFenceValue);
  m_pContext->Flush();
  m_pContext->DeviceWaitForFence(m_pFence, m_uiCompletedFenceValue);

  return m_uiCompletedFenceValue;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALCommandQueueVulkan::GetContext() const
{
  return m_pContext;
}
