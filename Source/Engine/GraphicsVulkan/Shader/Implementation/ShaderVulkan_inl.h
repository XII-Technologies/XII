
/// \todo GraphicsVulkan: Implement Shader Object overrides.

XII_FORCE_INLINE xiiUInt32 xiiGALShaderVulkan::GetResourceCount() const
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiInvalidIndex;
}

XII_FORCE_INLINE void xiiGALShaderVulkan::GetResourceDescription(xiiUInt32 uiIndex, xiiGALShaderResourceDescription& out_ResourceDescription) const
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetVertexShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetPixelShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Pixel)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetGeometryShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Geometry)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetHullShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Hull)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetDomainShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Domain)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetComputeShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Compute)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetAmplificationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Amplification)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetMeshShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Mesh)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetRayGenerationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayGeneration)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetRayMissShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayMiss)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetRayClosestHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayClosestHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetRayAnyHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayAnyHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetRayIntersectionShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayIntersection)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderVulkan::GetCallableShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Callable)];
}

XII_ALWAYS_INLINE xiiArrayPtr<Diligent::IPipelineResourceSignature*> xiiGALShaderVulkan::GetResourceSignatures()
{
  return m_PipelineResourceSignatures;
}

XII_ALWAYS_INLINE xiiArrayPtr<xiiGALVertexInputLayout> xiiGALShaderVulkan::GetInputLayouts()
{
  return m_VertexInputLayouts;
}

XII_ALWAYS_INLINE xiiArrayPtr<xiiGALShaderResourceBinding> xiiGALShaderVulkan::GetShaderResourceBinding(xiiBitflags<xiiGALShaderStage> e)
{
  return m_ShaderResourceBindings[xiiGALShaderStage::GetStageIndex(e)];
}
