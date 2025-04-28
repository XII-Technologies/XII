#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/Shader/Shader.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>
#include <GraphicsFoundation/Device/Device.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShader, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

#define XII_GAL_SHADER_CHECK(expression, ...)  \
  do                                           \
  {                                            \
    XII_ASSERT_DEV((expression), __VA_ARGS__); \
    if (!(expression)) { return {}; }          \
  } while (false)

xiiDelegate<void(xiiShaderUtilities::xiiBuiltinShaderType type, xiiShaderUtilities::xiiBuiltinShader& out_shader)> xiiShaderUtilities::g_RequestBuiltinShaderCallback;

xiiGALShader::xiiGALShader(xiiSharedPtr<xiiGALDevice> pDevice, const xiiGALShaderCreationDescription& creationDescription) :
  xiiGALDeviceObject(pDevice), m_Description(creationDescription)
{
}

xiiGALShader::~xiiGALShader() = default;

xiiSharedPtr<xiiGALInputLayout> xiiGALShader::CreateInputLayout(const xiiGALInputLayoutCreationDescription& description)
{
  XII_GAL_SHADER_CHECK(m_Description.m_ShaderType == xiiGALShaderType::Vertex, "An Input Layout must be created with shaders of type xiiGALShaderType::Vertex.");

  return CreateInputLayoutPlatform(description);
}

#undef XII_GAL_SHADER_CHECK

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_Shader_Implementation_Shader);
