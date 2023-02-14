
XII_ALWAYS_INLINE Diligent::IBuffer* xiiGALBufferDiligent::GetBuffer()
{
  return m_pBuffer.RawPtr();
}

XII_ALWAYS_INLINE Diligent::VALUE_TYPE xiiGALBufferDiligent::GetIndexFormat() const
{
  return m_IndexFormat;
}
