#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsFoundation/Descriptors/Descriptors.h>

struct XII_RENDERERCORE_DLL xiiShaderStateResourceDescriptor
{
  xiiGALBlendStateCreationDescription        m_BlendDesc;
  xiiGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  xiiGALRasterizerStateCreationDescription   m_RasterizerDesc;

  xiiResult Parse(const char* szSource);
  void      Load(xiiStreamReader& inout_stream);
  void      Save(xiiStreamWriter& inout_stream) const;

  xiiUInt32 CalculateHash() const;
};

class XII_RENDERERCORE_DLL xiiShaderPermutationBinary
{
public:
  xiiShaderPermutationBinary();

  xiiResult Write(xiiStreamWriter& inout_stream);
  xiiResult Read(xiiStreamReader& inout_stream, bool& out_bOldVersion);

  xiiUInt32 m_uiShaderStageHashes[xiiGALShaderStage::ENUM_COUNT];

  xiiDependencyFile m_DependencyFile;

  xiiShaderStateResourceDescriptor m_StateDescriptor;

  xiiHybridArray<xiiPermutationVar, 16> m_PermutationVars;
};
