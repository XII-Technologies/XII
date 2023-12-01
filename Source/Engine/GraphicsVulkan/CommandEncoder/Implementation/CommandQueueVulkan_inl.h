
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetNextFenceValue() const
{
  return m_pCommandQueue->GetNextFenceValue();
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::GetCompletedFenceValue() const
{
  return m_pCommandQueue->GetCompletedFenceValue();
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueVulkan::WaitForIdle()
{
  return m_pCommandQueue->WaitForIdle();
}

XII_ALWAYS_INLINE const Diligent::ICommandQueue* xiiGALCommandQueueVulkan::GetCommandQueue() const
{
  return m_pCommandQueue;
}
