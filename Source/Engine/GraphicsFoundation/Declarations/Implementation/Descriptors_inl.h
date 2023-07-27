
XII_ALWAYS_INLINE float xiiGALComputeShaderProperties::GetZtoDepthBias() const
{
  return -m_fMinZ * m_fZToDepthScale;
}
