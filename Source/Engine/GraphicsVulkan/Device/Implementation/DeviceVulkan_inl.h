
XII_ALWAYS_INLINE Diligent::IRenderDevice* xiiGALDeviceVulkan::GetDevice()
{
  return m_pDevice;
}

XII_ALWAYS_INLINE Diligent::IEngineFactory* xiiGALDeviceVulkan::GetFactory()
{
  return m_pEngineFactory;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceVulkan::GetImmediateContext()
{
  return m_pDeviceContexts[0];
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceVulkan::GetComputeContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_COMPUTE)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceVulkan::GetTransferContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_TRANSFER)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceVulkan::GetSparseBindingContext()
{
  for (xiiUInt32 i = 1; i < m_pDeviceContexts.GetCount(); ++i)
  {
    auto pContext = m_pDeviceContexts[i];

    if (pContext->GetDesc().QueueType == Diligent::COMMAND_QUEUE_TYPE_SPARSE_BINDING)
      return pContext;
  }
  return nullptr;
}

XII_ALWAYS_INLINE const xiiGALFormatLookupTableVulkan& xiiGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
