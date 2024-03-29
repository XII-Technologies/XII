
XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetVertexShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetPixelShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Pixel)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetGeometryShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Geometry)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetHullShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Hull)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetDomainShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Domain)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetComputeShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Compute)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetAmplificationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Amplification)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetMeshShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Mesh)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetRayGenerationShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayGeneration)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetRayMissShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayMiss)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetRayClosestHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayClosestHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetRayAnyHitShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayAnyHit)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetRayIntersectionShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayIntersection)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetCallableShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Callable)];
}

XII_ALWAYS_INLINE Diligent::IShader* xiiGALShaderD3D11::GetTileShader() const
{
  return m_pShaderStages[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Tile)];
}
