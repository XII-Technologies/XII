#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/ShaderStageBinary.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

struct XII_RENDERERCORE_DLL xiiShaderStateResourceDescriptor
{
  xiiGALBlendStateCreationDescription        m_BlendDesc;
  xiiGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  xiiGALRasterizerStateCreationDescription   m_RasterizerDesc;

  xiiResult Load(const char* szSource);
  void      Load(xiiStreamReader& ref_stream);
  void      Save(xiiStreamWriter& ref_stream) const;

  xiiUInt32 CalculateHash() const;
};

class XII_RENDERERCORE_DLL xiiShaderPermutationBinary
{
public:
  xiiShaderPermutationBinary();

  xiiResult Write(xiiStreamWriter& ref_stream);
  xiiResult Read(xiiStreamReader& ref_stream, bool& out_bOldVersion);

  xiiUInt32 m_uiShaderStageHashes[xiiGALShaderStage::ENUM_COUNT];

  xiiDependencyFile m_DependencyFile;

  xiiShaderStateResourceDescriptor m_StateDescriptor;

  xiiHybridArray<xiiPermutationVar, 16> m_PermutationVars;
};
