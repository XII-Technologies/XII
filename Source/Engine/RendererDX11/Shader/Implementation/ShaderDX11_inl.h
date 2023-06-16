
ID3D11VertexShader* xiiGALShaderDX11::GetDXVertexShader() const
{
  return m_pVertexShader;
}

ID3D11HullShader* xiiGALShaderDX11::GetDXHullShader() const
{
  return m_pHullShader;
}

ID3D11DomainShader* xiiGALShaderDX11::GetDXDomainShader() const
{
  return m_pDomainShader;
}

ID3D11GeometryShader* xiiGALShaderDX11::GetDXGeometryShader() const
{
  return m_pGeometryShader;
}

ID3D11PixelShader* xiiGALShaderDX11::GetDXPixelShader() const
{
  return m_pPixelShader;
}

ID3D11ComputeShader* xiiGALShaderDX11::GetDXComputeShader() const
{
  return m_pComputeShader;
}

const xiiDynamicArray<xiiShaderDescriptorSetLayout>& xiiGALShaderDX11::GetDescriptorSets(xiiGALShaderStage::Enum stage) const
{
  return m_DescriptorSets[stage];
}

const xiiHybridArray<xiiShaderVertexInputAttribute, 8>& xiiGALShaderDX11::GetVertexInputAttributes() const
{
  return m_VertexInputAttributes;
}

const xiiArrayPtr<const xiiUInt8> xiiGALShaderDX11::GetByteCode(xiiGALShaderStage::Enum stage) const
{
  return m_pByteCodes[stage];
}
