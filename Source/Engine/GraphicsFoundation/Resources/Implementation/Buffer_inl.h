
XII_ALWAYS_INLINE xiiGALBufferViewHandle xiiGALBuffer::GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType)
{
  switch (viewType)
  {
    case xiiGALBufferViewType::ShaderResource:
      return m_hDefaultBufferView;

    default:
      return xiiGALBufferViewHandle();
  }
}
