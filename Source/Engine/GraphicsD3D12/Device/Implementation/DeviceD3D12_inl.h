
XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceD3D12::GetGraphicsQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceD3D12::GetComputeQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceD3D12::GetTransferQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE xiiGALCommandQueue* xiiGALDeviceD3D12::GetSparseBindingQueue() const
{
  return nullptr;
}

XII_ALWAYS_INLINE ID3D12Device* xiiGALDeviceD3D12::GetDeviceD3D12() const
{
  return m_pDeviceD3D12;
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

#if 0
XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D12::GetImmediateContext()
{
  return m_pDeviceContexts[0];
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D12::GetComputeContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_COMPUTE)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D12::GetTransferContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_TRANSFER)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D12::GetSparseBindingContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_SPARSE_BINDING)
      return pContext;
  }
  return nullptr;
}
#endif

XII_ALWAYS_INLINE const xiiGALFormatLookupTableD3D12& xiiGALDeviceD3D12::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
