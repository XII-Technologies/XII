
XII_ALWAYS_INLINE ID3D11Device* xiiGALDeviceDX11::GetDXDevice() const
{
  return m_pDevice;
}

XII_ALWAYS_INLINE ID3D11Device3* xiiGALDeviceDX11::GetDXDevice3() const
{
  return m_pDevice3;
}

XII_ALWAYS_INLINE ID3D11DeviceContext* xiiGALDeviceDX11::GetDXImmediateContext() const
{
  return m_pImmediateContext;
}

XII_ALWAYS_INLINE IDXGIFactory1* xiiGALDeviceDX11::GetDXGIFactory() const
{
  return m_pDXGIFactory;
}

XII_ALWAYS_INLINE const xiiGALFormatLookupTableDX11& xiiGALDeviceDX11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}

inline ID3D11Query* xiiGALDeviceDX11::GetTimestamp(xiiGALTimestampHandle hTimestamp)
{
  if (hTimestamp.m_uiIndex < m_Timestamps.GetCount())
  {
    return m_Timestamps[static_cast<xiiUInt32>(hTimestamp.m_uiIndex)];
  }

  return nullptr;
}
