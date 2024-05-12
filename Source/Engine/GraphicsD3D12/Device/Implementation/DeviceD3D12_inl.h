
XII_ALWAYS_INLINE ID3D12Device1* xiiGALDeviceD3D12::GetD3D12Device() const
{
  return m_pD3D12Device;
}

XII_ALWAYS_INLINE IDXGIAdapter1* xiiGALDeviceD3D12::GetDXGIAdapter() const
{
  return m_pDXGIAdapter;
}

XII_ALWAYS_INLINE IDXGIFactory4* xiiGALDeviceD3D12::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

XII_ALWAYS_INLINE xiiMemoryAllocatorD3D12* xiiGALDeviceD3D12::GetD3D12Allocator() const
{
  return m_pAllocatorD3D12.Borrow();
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALDeviceD3D12::GetCommandQueueIndex(xiiBitflags<xiiGALCommandQueueType> queueType) const
{
  if (queueType == xiiGALCommandQueueType::Graphics)
    return 0U;

  if (queueType == xiiGALCommandQueueType::Compute)
    return 1U;

  if (queueType == xiiGALCommandQueueType::Transfer)
    return 2U;

  if (queueType == xiiGALCommandQueueType::SparseBinding)
    return 3U;

  XII_REPORT_FAILURE("Unknown command queue type.");

  return 0U;
}
