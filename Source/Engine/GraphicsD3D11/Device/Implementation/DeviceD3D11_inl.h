
XII_ALWAYS_INLINE ID3D11Device* xiiGALDeviceD3D11::GetD3D11Device() const
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

XII_ALWAYS_INLINE ID3D11DeviceContext* xiiGALDeviceD3D11::GetImmediateContext()
{
  return m_pDeviceContext;
}

XII_ALWAYS_INLINE const xiiGALFormatLookupTableD3D11& xiiGALDeviceD3D11::GetFormatLookupTable() const
{
  return m_FormatLookupTable;
}
