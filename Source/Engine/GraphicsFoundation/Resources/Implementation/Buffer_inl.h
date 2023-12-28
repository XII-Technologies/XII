
XII_ALWAYS_INLINE const xiiGALBufferCreationDescription& xiiGALBuffer::GetDescription() const
{
  return m_Description;
}

XII_ALWAYS_INLINE xiiGALBufferViewHandle xiiGALBuffer::GetDefaultView(xiiEnum<xiiGALBufferViewType> viewType)
{
  XII_ASSERT_DEV(viewType > xiiGALBufferViewType::Undefined && viewType < xiiGALBufferViewType::ENUM_COUNT, "Invalid view type.");

  return m_DefaultBufferViews[viewType.GetValue()];
}
