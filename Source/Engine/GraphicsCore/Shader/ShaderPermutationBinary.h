#pragma once

#include <Foundation/IO/DependencyFile.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Shader/ShaderStageBinary.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

struct XII_GRAPHICSCORE_DLL xiiShaderStateResourceDescriptor
{
  xiiGALBlendStateCreationDescription        m_BlendDesc;
  xiiGALDepthStencilStateCreationDescription m_DepthStencilDesc;
  xiiGALRasterizerStateCreationDescription   m_RasterizerDesc;

  xiiResult Parse(xiiStringView sSource);
  void      Load(xiiStreamReader& inout_stream);
  void      Save(xiiStreamWriter& inout_stream) const;

  xiiUInt32 CalculateHash() const;
};

/// \brief Serialized state of a shader permutation used by xiiShaderPermutationResourceLoader to convert into a xiiShaderPermutationResource.
class XII_GRAPHICSCORE_DLL xiiShaderPermutationBinary
{
public:
  xiiShaderPermutationBinary();

  xiiResult Write(xiiStreamWriter& inout_stream);
  xiiResult Read(xiiStreamReader& inout_stream, bool& out_bOldVersion);

  // Actual binary will be loaded from the hash via xiiShaderStageBinary::LoadStageBinary to produce xiiShaderStageBinary
  xiiUInt32 m_uiShaderStageHashes[xiiGALShaderStage::ENUM_COUNT];

  xiiDependencyFile m_DependencyFile;

  xiiShaderStateResourceDescriptor m_StateDescriptor;

  xiiHybridArray<xiiPermutationVar, 16> m_PermutationVars;
};
