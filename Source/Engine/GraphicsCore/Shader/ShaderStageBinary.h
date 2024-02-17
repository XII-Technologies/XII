#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Enum.h>
#include <GraphicsFoundation/Shader/Shader.h>

class XII_GRAPHICSCORE_DLL xiiShaderStageBinary
{
public:
  enum Version
  {
    Version0,

    ENUM_COUNT,

    VersionCurrent = ENUM_COUNT - 1
  };

  xiiShaderStageBinary();
  ~xiiShaderStageBinary();

  xiiSharedPtr<const xiiGALShaderByteCode> GetByteCode() const;

private:
  friend class xiiRenderContext;
  friend class xiiShaderCompiler;
  friend class xiiShaderPermutationResource;
  friend class xiiShaderPermutationResourceLoader;

  xiiResult WriteStageBinary(xiiLogInterface* pLog) const;
  xiiResult Write(xiiStreamWriter& inout_stream) const;
  xiiResult Read(xiiStreamReader& inout_stream);
  xiiResult Write(xiiStreamWriter& inout_stream, const xiiDynamicArray<xiiGALShaderVariableDescription>& layout) const;
  xiiResult Read(xiiStreamReader& inout_stream, xiiDynamicArray<xiiGALShaderVariableDescription>& out_layout);

private:
  xiiUInt32                          m_uiSourceHash = 0;
  xiiSharedPtr<xiiGALShaderByteCode> m_pGALByteCode;

private: // statics
  static xiiShaderStageBinary* LoadStageBinary(xiiBitflags<xiiGALShaderStage> Stage, xiiUInt32 uiHash);

  static void OnEngineShutdown();

  static xiiMap<xiiUInt32, xiiShaderStageBinary> s_ShaderStageBinaries[xiiGALShaderStage::ENUM_COUNT];
};
