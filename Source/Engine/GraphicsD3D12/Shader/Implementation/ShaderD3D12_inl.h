
/// \todo GraphicsD3D12: Implement Shader Object overrides.

XII_FORCE_INLINE xiiUInt32 xiiGALShaderD3D12::GetResourceCount() const
{
  XII_ASSERT_NOT_IMPLEMENTED;

  return xiiInvalidIndex;
}

XII_FORCE_INLINE void xiiGALShaderD3D12::GetResourceDescription(xiiUInt32 uiIndex, xiiGALShaderResourceDescription& out_ResourceDescription) const
{
  XII_ASSERT_NOT_IMPLEMENTED;
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetVertexShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetPixelShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Pixel)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetGeometryShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Geometry)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetHullShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Hull)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetDomainShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Domain)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetComputeShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Compute)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetAmplificationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Amplification)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetMeshShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Mesh)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetRayGenerationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayGeneration)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetRayMissShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayMiss)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetRayClosestHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayClosestHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetRayAnyHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayAnyHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetRayIntersectionShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayIntersection)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetCallableShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Callable)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D12::GetTileShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Tile)];
}

XII_ALWAYS_INLINE xiiArrayPtr<Diligent::IPipelineResourceSignature*> xiiGALShaderD3D12::GetResourceSignatures()
{
  return m_PipelineResourceSignatures;
}

XII_ALWAYS_INLINE xiiArrayPtr<xiiGALVertexInputLayout> xiiGALShaderD3D12::GetInputLayouts()
{
  return m_VertexInputLayouts;
}

XII_ALWAYS_INLINE xiiArrayPtr<xiiGALShaderResourceBinding> xiiGALShaderD3D12::GetShaderResourceBinding(xiiBitflags<xiiGALShaderStage> e)
{
  return m_ShaderResourceBindings[xiiGALShaderStage::GetStageIndex(e)];
}
