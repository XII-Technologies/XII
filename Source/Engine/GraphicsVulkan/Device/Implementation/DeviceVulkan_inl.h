
XII_ALWAYS_INLINE Diligent::IRenderDevice* xiiGALDeviceVulkan::GetDevice()
{
  return m_pDevice;
}

XII_ALWAYS_INLINE Diligent::IEngineFactory* xiiGALDeviceVulkan::GetFactory()
{
  return m_pEngineFactory;
}

XII_ALWAYS_INLINE xiiGALPassVulkan* xiiGALDeviceVulkan::GetDefaultPass()
{
  return m_pDefaultPass.Borrow();
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceVulkan::GetImmediateContext()
{
  return m_pDeviceContexts[0];
}

XII_ALWAYS_INLINE const xiiGALFormatLookupTableVulkan& xiiGALDeviceVulkan::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
