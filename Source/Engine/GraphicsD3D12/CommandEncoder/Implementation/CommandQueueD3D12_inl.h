
XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::GetNextFenceValue() const
{
  return m_pCommandQueue->GetNextFenceValue();
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::GetCompletedFenceValue() const
{
  return m_pCommandQueue->GetCompletedFenceValue();
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::WaitForIdle()
{
  return m_pCommandQueue->WaitForIdle();
}

XII_ALWAYS_INLINE const Diligent::ICommandQueue* xiiGALCommandQueueD3D12::GetCommandQueue() const
{
  return m_pCommandQueue;
}
