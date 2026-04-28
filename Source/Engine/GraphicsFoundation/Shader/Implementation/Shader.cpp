/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShader, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

#define XII_GAL_SHADER_CHECK(expression, ...)  \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return {}; }          \
  } while (false)

xiiGALShader::xiiGALShader(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALDeviceObject(std::move(pDevice)), m_Description(creationDescription)
{
}

xiiGALShader::~xiiGALShader() = default;

xiiSharedPtr<xiiGALInputLayout> xiiGALShader::CreateInputLayout(const xiiGALInputLayoutCreationDescription& description)
{
  XII_GAL_SHADER_CHECK(m_Description.m_ShaderType == xiiGALShaderType::Vertex, "An Input Layout must be created with shaders of type xiiGALShaderType::Vertex.");

  for (xiiUInt32 i = 0; i < description.m_LayoutElements.GetCount(); ++i)
  {
    XII_GAL_SHADER_CHECK(description.m_LayoutElements[i].m_uiBufferSlot != xiiInvalidIndex, "LayoutElements[{}].uiBufferSlot('{}'}) is invalid.", i, description.m_LayoutElements[i].m_uiBufferSlot);
  }

  return CreateInputLayoutPlatform(description);
}

#undef XII_GAL_SHADER_CHECK

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_Shader);
