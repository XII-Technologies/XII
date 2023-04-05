
XII_ALWAYS_INLINE Diligent::IRenderDevice* xiiGALDeviceDiligent::GetDevice()
{
  return m_pDevice;
}

XII_ALWAYS_INLINE Diligent::IEngineFactory* xiiGALDeviceDiligent::GetFactory()
{
  return m_pEngineFactory;
}

XII_ALWAYS_INLINE Diligent::IDeviceContext* xiiGALDeviceDiligent::GetImmediateContext()
{
  return m_pDeviceContexts[0];
}

XII_ALWAYS_INLINE const xiiGALFormatLookupTableDiligent& xiiGALDeviceDiligent::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

XII_ALWAYS_INLINE const Diligent::RENDER_DEVICE_TYPE& xiiGALDeviceDiligent::GetDeviceType() const
{
  return m_DeviceType;
}

XII_ALWAYS_INLINE const xiiInt32 xiiGALDeviceDiligent::GetValidationLevel() const
{
  return m_iValidationLevel;
}
