
XII_ALWAYS_INLINE xiiGALCommandListPoolD3D12* xiiGALDeviceD3D12::GetCommandListPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Graphics))
    return m_pGraphicsCommandListPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandListPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandListPool.Borrow();

  return m_pGraphicsCommandListPool.Borrow();
}

XII_ALWAYS_INLINE xiiGALQueryPoolD3D12* xiiGALDeviceD3D12::GetCommandQueueQueryPool(xiiBitflags<xiiGALCommandQueueFlags> queueFlags) const
{
  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Graphics))
    return m_pGraphicsCommandQueueQueryPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Compute) && m_pComputeCommandQueue != nullptr)
    return m_pComputeCommandQueueQueryPool.Borrow();

  if (queueFlags.IsSet(xiiGALCommandQueueFlags::Transfer) && m_pTransferCommandQueue != nullptr)
    return m_pTransferCommandQueueQueryPool.Borrow();

  return m_pGraphicsCommandQueueQueryPool.Borrow();
}
