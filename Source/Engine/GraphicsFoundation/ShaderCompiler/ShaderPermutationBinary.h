/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/IO/DependencyFile.h>

#include <GraphicsFoundation/ShaderCompiler/Descriptors.h>
#include <GraphicsFoundation/States/BlendState.h>
#include <GraphicsFoundation/States/DepthStencilState.h>
#include <GraphicsFoundation/States/RasterizerState.h>

struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderStateResourceDescriptor
{
  xiiGALBlendStateCreationDescription        m_BlendDescription;
  xiiGALDepthStencilStateCreationDescription m_DepthStencilDescription;
  xiiGALRasterizerStateCreationDescription   m_RasterizerDescription;

  xiiResult Parse(xiiStringView sSource);
  void      Load(xiiStreamReader& inout_stream);
  void      Save(xiiStreamWriter& inout_stream) const;

  xiiUInt32 CalculateHash() const;
};

/// Serialized state of a shader permutation.
class XII_GRAPHICSFOUNDATION_DLL xiiGALShaderPermutationBinary
{
public:
  xiiGALShaderPermutationBinary();

  xiiResult Write(xiiStreamWriter& inout_stream);
  xiiResult Read(xiiStreamReader& inout_stream, bool& out_bOldVersion);

  // Actual binary will be loaded from the hash.
  xiiMap<xiiGALShaderType::Enum, xiiUInt32> m_ShaderStageHashes;

  xiiDependencyFile m_DependencyFile;

  xiiGALShaderStateResourceDescriptor m_StateDescriptor;

  xiiHybridArray<xiiGALPermutationVariable, 16> m_PermutationVariables;
};
