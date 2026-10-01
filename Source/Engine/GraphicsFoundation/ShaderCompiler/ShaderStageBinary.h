/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Types/UniquePtr.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderStageBinary
{
public:
  xiiGALShaderStageBinary();
  ~xiiGALShaderStageBinary();

  xiiSharedPtr<const xiiGALShaderByteCode> GetByteCode() const;

  static xiiGALShaderStageBinary* LoadStageBinary(xiiEnum<xiiGALShaderType> stage, xiiUInt32 uiHash, xiiStringView sPlatform);

  [[nodiscard]] static bool IsCacheInitialized();
  static void OnEngineStartup();
  static void OnEngineShutdown();

private:
  friend class xiiGALShaderCompiler;

  xiiResult WriteStageBinary(xiiLogInterface* pLog, xiiStringView sPlatform) const;
  xiiResult Write(xiiStreamWriter& inout_stream) const;
  xiiResult Read(xiiStreamReader& inout_stream);
  xiiResult Write(xiiStreamWriter& inout_stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& layout) const;
  xiiResult Read(xiiStreamReader& inout_stream, xiiDynamicArray<xiiGALShaderVariableDescription>& out_layout);
  static void StoreStageBinary(xiiEnum<xiiGALShaderType> stage, const xiiGALShaderStageBinary& binary);

private:
  xiiUInt32                          m_uiSourceHash = 0U;
  xiiSharedPtr<xiiGALShaderByteCode> m_pGALByteCode;

private:
  class CacheState;
  static xiiUniquePtr<CacheState> s_pCacheState;
};
