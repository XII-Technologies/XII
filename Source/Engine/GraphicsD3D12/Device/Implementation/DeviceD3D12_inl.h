
XII_ALWAYS_INLINE Diligent::IRenderDevice* xiiGALDeviceD3D12::GetDevice()
{
  return m_pDevice;
}

XII_ALWAYS_INLINE Diligent::IEngineFactory* xiiGALDeviceD3D12::GetFactory()
{
  return m_pEngineFactory;
}

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

XII_ALWAYS_INLINE const xiiGALFormatLookupTableD3D12& xiiGALDeviceD3D12::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
