#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

xiiGALShader::xiiGALShader(const xiiGALShaderCreationDescription& Description) :
  xiiGALObject(Description)
{
}

xiiGALShader::~xiiGALShader() {}

xiiDelegate<void(xiiShaderUtils::xiiBuiltinShaderType type, xiiShaderUtils::xiiBuiltinShader& out_shader)> xiiShaderUtils::g_RequestBuiltinShaderCallback;

XII_STATICLINK_FILE(RendererFoundation, RendererFoundation_Shader_Implementation_Shader);
