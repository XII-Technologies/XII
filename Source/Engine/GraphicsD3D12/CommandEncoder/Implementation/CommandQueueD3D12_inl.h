
XII_ALWAYS_INLINE ID3D12CommandQueue* xiiGALCommandQueueD3D12::GetD3D12CommandQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::GetNextFenceValue() const
{
  return 0U;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::GetCompletedFenceValue()
{
  return 0U;
}

XII_ALWAYS_INLINE xiiUInt64 xiiGALCommandQueueD3D12::WaitForIdle()
{
  return 0U;
}
