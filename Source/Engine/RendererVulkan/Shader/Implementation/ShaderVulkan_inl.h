
vk::ShaderModule xiiGALShaderVulkan::GetShader(xiiGALShaderStage::Enum stage) const
{
  return m_Shaders[stage];
}

const xiiGALShaderVulkan::DescriptorSetLayoutDesc& xiiGALShaderVulkan::GetDescriptorSetLayout() const
{
  return m_descriptorSetLayoutDesc;
}

const xiiArrayPtr<const xiiGALShaderVulkan::BindingMapping> xiiGALShaderVulkan::GetBindingMapping() const
{
  return m_BindingMapping;
}

const xiiArrayPtr<const xiiGALShaderVulkan::VertexInputAttribute> xiiGALShaderVulkan::GetVertexInputAttributes() const
{
  return m_VertexInputAttributes;
}
