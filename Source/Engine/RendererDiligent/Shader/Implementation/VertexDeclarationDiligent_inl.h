
XII_ALWAYS_INLINE const Diligent::InputLayoutDesc* xiiGALVertexDeclarationDiligent::GetInputLayoutDesc() const
{
  return &m_InputLayoutDesc;
}

XII_ALWAYS_INLINE xiiHybridArray<Diligent::LayoutElement, 8U>& xiiGALVertexDeclarationDiligent::GetInputLayoutElements()
{
  return m_InputElements;
}
