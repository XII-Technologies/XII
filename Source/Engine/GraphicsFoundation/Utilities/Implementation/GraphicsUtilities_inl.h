
XII_ALWAYS_INLINE bool xiiGALGraphicsUtilities::IsIdentityComponentMapping(const xiiGALTextureComponentMapping& mapping)
{
  return ((mapping.m_R == xiiGALTextureComponentSwizzle::Identity || mapping.m_R == xiiGALTextureComponentSwizzle::R) &&
          (mapping.m_G == xiiGALTextureComponentSwizzle::Identity || mapping.m_G == xiiGALTextureComponentSwizzle::G) &&
          (mapping.m_B == xiiGALTextureComponentSwizzle::Identity || mapping.m_B == xiiGALTextureComponentSwizzle::B) &&
          (mapping.m_A == xiiGALTextureComponentSwizzle::Identity || mapping.m_A == xiiGALTextureComponentSwizzle::A));
}
