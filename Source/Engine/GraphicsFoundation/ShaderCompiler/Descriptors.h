/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

/// A shader resource definition found inside the shader source code.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderResourceDefinition
{
  /// Just the declaration inside the shader source, e.g. "Texture1D Texture".
  xiiStringView m_sDeclaration;

  /// The declaration with any optional register mappings, e.g. "Texture1D Texture : register(12t, space3)"
  xiiStringView m_sDeclarationAndRegister;

  /// The extracted reflection of the resource containing type, binding index, sets, etc.
  xiiGALShaderResourceDescription m_ResourceDescription;
};

/// Flags that affect the compilation process of a shader.
struct xiiGALShaderCompilerFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Debug                 = XII_BIT(0),
    EnableUnboundedArrays = XII_BIT(1),

    Default = 0U,
  };

  struct Bits
  {
    StorageType Debug : 1;
    StorageType EnableUnboundedArrays : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiGALShaderCompilerFlags);

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSFOUNDATION_DLL, xiiGALShaderCompilerFlags);

/// Storage used during the shader compilation process.
struct XII_GRAPHICSFOUNDATION_DLL xiiGALShaderProgramData
{
  struct StageData
  {
    xiiString                                          m_sShaderSource;
    xiiHybridArray<xiiGALShaderResourceDefinition, 4U> m_Resources;
    xiiUInt32                                          m_uiSourceHash = 0U;
    xiiSharedPtr<xiiGALShaderByteCode>                 m_pByteCode;
    bool                                               m_bWriteToDisk = true;
  };

  xiiBitflags<xiiGALShaderCompilerFlags>    m_Flags;
  xiiStringView                             m_sPlatform;
  xiiStringView                             m_sSourceFile;
  xiiMap<xiiGALShaderType::Enum, StageData> m_StageData;
};

struct XII_GRAPHICSFOUNDATION_DLL xiiGALPermutationVariable
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  xiiHashedString m_sName;
  xiiHashedString m_sValue;

  XII_ALWAYS_INLINE bool operator==(const xiiGALPermutationVariable& rhs) const
  {
    return m_sName == rhs.m_sName && m_sValue == rhs.m_sValue;
  }

  static xiiUInt32 CalculateHash(const xiiArrayPtr<xiiGALPermutationVariable>& permutationVariables);
};
