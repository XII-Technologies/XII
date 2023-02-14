
XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetVertexShader()
{
  return m_pShaderStages[xiiGALShaderStage::VertexShader].RawPtr();
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetHullShader()
{
  return m_pShaderStages[xiiGALShaderStage::HullShader].RawPtr();
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetDomainShader()
{
  return m_pShaderStages[xiiGALShaderStage::DomainShader].RawPtr();
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetGeometryShader()
{
  return m_pShaderStages[xiiGALShaderStage::GeometryShader].RawPtr();
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetPixelShader()
{
  return m_pShaderStages[xiiGALShaderStage::PixelShader].RawPtr();
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetComputeShader()
{
  return m_pShaderStages[xiiGALShaderStage::ComputeShader].RawPtr();
}

XII_ALWAYS_INLINE xiiDynamicArray<xiiShaderDescriptorSetLayout>& xiiGALShaderDiligent::GetDescriptorSets(xiiGALShaderStage::Enum stage)
{
  return m_DescriptorSets[stage];
}

XII_ALWAYS_INLINE xiiHybridArray<xiiShaderVertexInputAttribute, 8>& xiiGALShaderDiligent::GetVertexInputAttributes()
{
  return m_VertexInputAttributes;
}
