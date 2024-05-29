
XII_ALWAYS_INLINE const xiiGALSparseTextureProperties& xiiGALTextureNull::GetSparseProperties() const
{
  static xiiGALSparseTextureProperties temporary;

  return temporary;
}
