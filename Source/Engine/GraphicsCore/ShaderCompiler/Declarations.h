#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/SharedPtr.h>

#include <GraphicsFoundation/Shader/ShaderByteCode.h>

#include <GraphicsCore/Declarations.h>

/// \brief Output of ParseShaderResources. A shader resource definition found inside the shader source code.
struct xiiShaderResourceDefinition
{
  /// \brief Just the declaration inside the shader source, e.g. "Texture1D Texture".
  xiiStringView m_sDeclaration;

  /// \brief The declaration with any optional register mappings, e.g. "Texture1D Texture : register(12t, space3)"
  xiiStringView m_sDeclarationAndRegister;

  /// \brief The extracted reflection of the resource containing type, binding index, sets, etc.
  xiiGALShaderResourceDescription m_ResourceDescription;

  /// \brief The extracted reflection of the constant buffer resource containing type, binding index, sets, etc.
  xiiGALShaderBufferDescription m_ResourceBufferDescription;
};

/// \brief Flags that affect the compilation process of a shader
struct xiiShaderCompilerFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Debug   = XII_BIT(0),
    Default = 0U,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiShaderCompilerFlags);

/// \brief Storage used during the shader compilation process.
struct XII_GRAPHICSCORE_DLL xiiShaderProgramData
{
  xiiShaderProgramData()
  {
    m_sPlatform   = {};
    m_sSourceFile = {};

    for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      m_bWriteToDisk[stage] = true;
      m_sShaderSource[stage].Clear();
      m_Resources[stage].Clear();
      m_uiSourceHash[stage] = 0;
      m_ByteCode[stage].Clear();
    }
  }

  xiiBitflags<xiiShaderCompilerFlags>             m_Flags;
  xiiStringView                                   m_sPlatform;
  xiiStringView                                   m_sSourceFile;
  xiiString                                       m_sShaderSource[xiiGALShaderStage::ENUM_COUNT];
  xiiHybridArray<xiiShaderResourceDefinition, 8U> m_Resources[xiiGALShaderStage::ENUM_COUNT];
  xiiUInt32                                       m_uiSourceHash[xiiGALShaderStage::ENUM_COUNT];
  xiiSharedPtr<xiiGALShaderByteCode>              m_ByteCode[xiiGALShaderStage::ENUM_COUNT];
  bool                                            m_bWriteToDisk[xiiGALShaderStage::ENUM_COUNT];
};
