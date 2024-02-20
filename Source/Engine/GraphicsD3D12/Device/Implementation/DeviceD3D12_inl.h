
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

XII_ALWAYS_INLINE const xiiGALFormatLookupTableD3D12& xiiGALDeviceD3D12::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
