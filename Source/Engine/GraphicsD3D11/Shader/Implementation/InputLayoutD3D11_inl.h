
XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* xiiGALInputLayoutD3D11::GetLayout() const
{
  return &m_InputLayout;
}

XII_ALWAYS_INLINE xiiArrayPtr<Diligent::LayoutElement> xiiGALInputLayoutD3D11::GetElements()
{
  return m_InputElements;
}
