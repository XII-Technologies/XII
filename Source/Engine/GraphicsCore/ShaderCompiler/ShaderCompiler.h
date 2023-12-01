#pragma once

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Shader/Implementation/Helper.h>
#include <GraphicsCore/Shader/ShaderPermutationBinary.h>
#include <GraphicsCore/ShaderCompiler/PermutationGenerator.h>

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

class XII_GRAPHICSCORE_DLL xiiShaderProgramCompiler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShaderProgramCompiler, xiiReflectedClass);

public:
  struct xiiShaderProgramData
  {
    xiiShaderProgramData()
    {
      m_sPlatform   = {};
      m_sSourceFile = {};

      for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
      {
        m_bWriteToDisk[stage]  = true;
        m_sShaderSource[stage] = {};
      }
    }

    xiiBitflags<xiiShaderCompilerFlags> m_Flags;
    xiiStringView                       m_sPlatform;
    xiiStringView                       m_sSourceFile;
    xiiStringView                       m_sShaderSource[xiiGALShaderStage::ENUM_COUNT];
    xiiShaderStageBinary                m_StageBinary[xiiGALShaderStage::ENUM_COUNT];
    bool                                m_bWriteToDisk[xiiGALShaderStage::ENUM_COUNT];
  };

  virtual void GetSupportedPlatforms(xiiHybridArray<xiiString, 4>& ref_platforms) = 0;

  virtual xiiResult Compile(xiiShaderProgramData& inout_data, xiiLogInterface* pLog) = 0;
};

class XII_GRAPHICSCORE_DLL xiiShaderCompiler
{
public:
  xiiResult CompileShaderPermutationForPlatforms(xiiStringView sFile, const xiiArrayPtr<const xiiPermutationVar>& permutationVars, xiiLogInterface* pLog, xiiStringView sPlatform = "ALL");

private:
  xiiResult RunShaderCompiler(xiiStringView sFile, xiiStringView sPlatform, xiiShaderProgramCompiler* pCompiler, xiiLogInterface* pLog);

  void WriteFailedShaderSource(xiiShaderProgramCompiler::xiiShaderProgramData& spd, xiiLogInterface* pLog);

  bool PassThroughUnknownCommandCB(xiiStringView sCmd) { return sCmd == "version"; }

  struct xiiShaderData
  {
    xiiString                             m_Platforms;
    xiiHybridArray<xiiPermutationVar, 16> m_Permutations;
    xiiHybridArray<xiiPermutationVar, 16> m_FixedPermVars;
    xiiString                             m_StateSource;
    xiiString                             m_ShaderStageSource[xiiGALShaderStage::ENUM_COUNT];
  };

  xiiResult FileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification);

  xiiStringBuilder m_StageSourceFile[xiiGALShaderStage::ENUM_COUNT];

  xiiTokenizedFileCache m_FileCache;
  xiiShaderData         m_ShaderData;

  xiiSet<xiiString> m_IncludeFiles;
};
