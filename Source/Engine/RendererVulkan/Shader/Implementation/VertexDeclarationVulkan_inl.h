

xiiArrayPtr<const vk::VertexInputAttributeDescription> xiiGALVertexDeclarationVulkan::GetAttributes() const
{
  return m_attributes.GetArrayPtr();
}

xiiArrayPtr<const vk::VertexInputBindingDescription> xiiGALVertexDeclarationVulkan::GetBindings() const
{
  return m_bindings.GetArrayPtr();
}
