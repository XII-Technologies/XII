
XII_ALWAYS_INLINE float xiiGALNormalizedDeviceCoordinates::GetZtoDepthBias() const
{
  return -m_fMinZ * m_fZToDepthScale;
}
