
XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* xiiGALInputLayoutVulkan::GetLayout() const
{
  return &m_InputLayout;
}

XII_ALWAYS_INLINE xiiArrayPtr<Diligent::LayoutElement> xiiGALInputLayoutVulkan::GetElements()
{
  return m_InputElements;
}
