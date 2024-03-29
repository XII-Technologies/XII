
XII_ALWAYS_INLINE ID3D11Device* xiiGALDeviceD3D11::GetDeviceD3D11() const
{
  return m_pDeviceD3D11;
}

XII_ALWAYS_INLINE IDXGIAdapter1* xiiGALDeviceD3D11::GetDXGIAdapter() const
{
  return m_pDXGIAdapter;
}

XII_ALWAYS_INLINE IDXGIFactory4* xiiGALDeviceD3D11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

XII_ALWAYS_INLINE xiiMemoryAllocatorD3D11* xiiGALDeviceD3D11::GetD3D11Allocator() const
{
  return m_pAllocatorD3D11.Borrow();
}

#if 0
XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D11::GetImmediateContext()
{
  return m_pDeviceContexts[0];
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D11::GetComputeContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_COMPUTE)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D11::GetTransferContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_TRANSFER)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceD3D11::GetSparseBindingContext()
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

XII_ALWAYS_INLINE const xiiGALFormatLookupTableD3D11& xiiGALDeviceD3D11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
