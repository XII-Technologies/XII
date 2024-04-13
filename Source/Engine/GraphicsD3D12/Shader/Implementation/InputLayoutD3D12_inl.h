
XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* xiiGALInputLayoutD3D12::GetLayout() const
{
  return &m_InputLayout;
}

XII_ALWAYS_INLINE xiiArrayPtr<Diligent::LayoutElement> xiiGALInputLayoutD3D12::GetElements()
{
  return m_InputElements;
}
