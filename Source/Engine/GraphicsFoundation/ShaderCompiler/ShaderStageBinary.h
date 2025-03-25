#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Types/SharedPtr.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderStageBinary
{
public:
  xiiGALShaderStageBinary();
  ~xiiGALShaderStageBinary();

  xiiSharedPtr<const xiiGALShaderByteCode> GetByteCode() const;

private:
  friend class xiiGALShaderCompiler;

  xiiResult WriteStageBinary(xiiLogInterface* pLog, xiiStringView sPlatform) const;
  xiiResult Write(xiiStreamWriter& inout_stream) const;
  xiiResult Read(xiiStreamReader& inout_stream);
  xiiResult Write(xiiStreamWriter& inout_stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& layout) const;
  xiiResult Read(xiiStreamReader& inout_stream, xiiDynamicArray<xiiGALShaderVariableDescription>& out_layout);

private:
  xiiUInt32                          m_uiSourceHash = 0U;
  xiiSharedPtr<xiiGALShaderByteCode> m_pGALByteCode;

private: // statics
  static xiiGALShaderStageBinary* LoadStageBinary(xiiEnum<xiiGALShaderType> stage, xiiUInt32 uiHash, xiiStringView sPlatform);

  static void OnEngineShutdown();

  static xiiMap<xiiUInt32, xiiGALShaderStageBinary> s_ShaderStageBinaries[xiiGALShaderType::ENUM_COUNT];
};
