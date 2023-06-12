
XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetVertexShader()
{
  return m_pShaderStages[xiiGALShaderStage::VertexShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetHullShader()
{
  return m_pShaderStages[xiiGALShaderStage::HullShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetDomainShader()
{
  return m_pShaderStages[xiiGALShaderStage::DomainShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetGeometryShader()
{
  return m_pShaderStages[xiiGALShaderStage::GeometryShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetPixelShader()
{
  return m_pShaderStages[xiiGALShaderStage::PixelShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetComputeShader()
{
  return m_pShaderStages[xiiGALShaderStage::ComputeShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetAmplificationShader()
{
  return m_pShaderStages[xiiGALShaderStage::AmplificationShader];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderDiligent::GetMeshShader()
{
  return m_pShaderStages[xiiGALShaderStage::MeshShader];
}

XII_ALWAYS_INLINE Diligent::IPipelineResourceSignature** xiiGALShaderDiligent::GetPipelineResourceSignatures()
{
  return m_pPipelineResourceSignature.RawDblPtr();
}

XII_ALWAYS_INLINE xiiUInt32 xiiGALShaderDiligent::GetPipelineResourceSignatureCount()
{
  return m_uiPipelineResourceSignatureCount;
}

XII_ALWAYS_INLINE xiiDynamicArray<xiiShaderDescriptorSetLayout>& xiiGALShaderDiligent::GetDescriptorSets(xiiGALShaderStage::Enum stage)
{
  return m_DescriptorSets[stage];
}

XII_ALWAYS_INLINE xiiHybridArray<xiiShaderVertexInputAttribute, 8>& xiiGALShaderDiligent::GetVertexInputAttributes()
{
  return m_VertexInputAttributes;
}
