#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <RendererCore/Declarations.h>
#include <RendererCore/Shader/Implementation/Helper.h>
#include <RendererCore/Shader/ShaderPermutationBinary.h>
#include <RendererCore/ShaderCompiler/PermutationGenerator.h>
#include <RendererFoundation/Descriptors/Descriptors.h>

// \brief Flags that affect the compilation process of a shader
struct xiiShaderCompilerFlags
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Debug   = XII_BIT(0),
    Default = 0,
  };

  struct Bits
  {
    StorageType Debug : 1;
  };
};
XII_DECLARE_FLAGS_OPERATORS(xiiShaderCompilerFlags);

class XII_RENDERERCORE_DLL xiiShaderProgramCompiler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderProgramCompiler, xiiReflectedClass);

public:
  struct xiiShaderProgramData
  {
    xiiShaderProgramData()
    {
      m_szPlatform   = nullptr;
      m_szSourceFile = nullptr;

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        m_bWriteToDisk[stage]   = true;
        m_szShaderSource[stage] = nullptr;
      }
    }

    xiiBitflags<xiiShaderCompilerFlags> m_Flags;
    const char*                         m_szPlatform;
    const char*                         m_szSourceFile;
    const char*                         m_szShaderSource[xiiGALShaderStage::ENUM_COUNT];
    xiiShaderStageBinary                m_StageBinary[xiiGALShaderStage::ENUM_COUNT];
    bool                                m_bWriteToDisk[xiiGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& ref_platforms) = 0;

  virtual xiiResult Compile(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;
};

class XII_RENDERERCORE_DLL xiiShaderCompiler
{
public:
  xiiResult CompileShaderPermutationForPlatforms(
    const char*                                 szFile,
    const xiiArrayPtr<const xiiPermutationVar>& permutationVars,
    xiiLogInterface*                            pLog,
    const char*                                 szPlatform = "ALL");

private:
  xiiResult RunShaderCompiler(const char* szFile, const char* szPlatform, xiiShaderProgramCompiler* pCompiler, xiiLogInterface* pLog);

  void WriteFailedShaderSource(xiiShaderProgramCompiler::xiiShaderProgramData& spd, xiiLogInterface* pLog);

  bool PassThroughUnknownCommandCB(const char* szCmd) { return xiiStringUtils::IsEqual(szCmd, "version"); }

  struct xiiShaderData
  {
    xiiString                             m_Platforms;
    xiiHybridArray<xiiPermutationVar, 16> m_Permutations;
    xiiHybridArray<xiiPermutationVar, 16> m_FixedPermVars;
    xiiString                             m_StateSource;
    xiiString                             m_ShaderStageSource[xiiGALShaderStage::ENUM_COUNT];
  };

  xiiResult FileOpen(const char* szAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification);

  xiiStringBuilder m_StageSourceFile[xiiGALShaderStage::ENUM_COUNT];

  xiiTokenizedFileCache m_FileCache;
  xiiShaderData         m_ShaderData;

  xiiSet<xiiString> m_IncludeFiles;
};
