#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Interfaces/RemoteToolingInterface.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Profiling/Profiling.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderPermutationBinary.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderStageBinary.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiGALShaderProgramCompiler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static bool PlatformEnabled(const xiiString& sPlatforms, xiiStringView sPlatform)
  {
    xiiStringBuilder sTemp;
    sTemp = sPlatform;

    sTemp.Prepend("!");

    // if it contains '!platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return false;

    sTemp = sPlatform;

    // if it contains 'platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp, xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    // do not enable this when ALL is specified
    if (sPlatform == "DEBUG")
      return false;

    // if it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(xiiStringView sPlatform, const xiiArrayPtr<xiiGALPermutationVariable>& permutationVariables, xiiHybridArray<xiiString, 32>& out_defines)
  {
    xiiStringBuilder sTemp;

    if (out_defines.IsEmpty())
    {
      out_defines.PushBack("TRUE 1");
      out_defines.PushBack("FALSE 0");

      sTemp = sPlatform;
      sTemp.ToUpper();

      out_defines.PushBack(sTemp);
    }

    for (const xiiGALPermutationVariable& var : permutationVariables)
    {
      xiiStringView sValue             = var.m_sValue;
      const bool    bIsBooleanVariable = sValue == "TRUE" || sValue == "FALSE";

      if (bIsBooleanVariable)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_defines.PushBack(sTemp);
      }
      else
      {
        xiiStringView sName      = var.m_sName;
        auto          enumValues = xiiGALShaderManager::GetPermutationEnumValues(var.m_sName);

        for (const auto& ev : enumValues)
        {
          sTemp.SetFormat("{1} {2}", sName, ev.m_sValueName, ev.m_iValueValue);
          out_defines.PushBack(sTemp);
        }

        if (sValue.StartsWith(sName))
        {
          sTemp.Set(sName, " ", sValue);
        }
        else
        {
          sTemp.Set(sName, " ", sName, "_", sValue);
        }
        out_defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[xiiGALShaderType::ENUM_COUNT] = {
    "VERTEX_SHADER",
    "PIXEL_SHADER",
    "GEOMETRY_SHADER",
    "HULL_SHADER",
    "DOMAIN_SHADER",
    "COMPUTE_SHADER",
    "AMPLIFICATION_SHADER",
    "MESH_SHADER",
    "RAY_GENERATION_SHADER",
    "RAY_MISS_SHADER",
    "RAY_CLOSESTHIT_SHADER",
    "RAY_ANY_HIT_SHADER",
    "RAY_INTERSECTION_SHADER",
    "CALLABLE_SHADER",
    "TILE_SHADER",
  };
} // namespace

xiiResult xiiGALShaderCompiler::FileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& fileContent, xiiTimestamp& out_fileModification)
{
  XII_PROFILE_SCOPE("xiiGALShaderCompiler::FileOpen");

  if (sAbsoluteFile == "ShaderRenderState")
  {
    const xiiString& sData   = m_ShaderData.m_StateSource;
    const xiiUInt32  uiCount = sData.GetElementCount();
    xiiStringView    sString = sData;

    fileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      xiiMemoryUtils::Copy<xiiUInt8>(fileContent.GetData(), (const xiiUInt8*)sString.GetStartPointer(), uiCount);
    }

    return XII_SUCCESS;
  }

  for (auto it : m_StageSourceFile)
  {
    if (it.Value() == sAbsoluteFile)
    {
      const xiiString& sData   = m_ShaderData.m_ShaderStageSource[it.Key()];
      const xiiUInt32  uiCount = sData.GetElementCount();
      xiiStringView    sString = sData;

      fileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        xiiMemoryUtils::Copy<xiiUInt8>(fileContent.GetData(), (const xiiUInt8*)sString.GetStartPointer(), uiCount);
      }

      return XII_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(sAbsoluteFile);

  xiiFileReader r;
  if (r.Open(sAbsoluteFile).Failed())
  {
    xiiLog::Error("Could not find include file '{0}'", sAbsoluteFile);
    return XII_FAILURE;
  }

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stats;
  if (xiiFileSystem::GetFileStats(sAbsoluteFile, stats).Succeeded())
  {
    out_fileModification = stats.m_LastModificationTime;
  }
#endif

  xiiUInt8 Temp[4096];

  while (xiiUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    fileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));
  }

  return XII_SUCCESS;
}

xiiResult xiiGALShaderCompiler::CompileShaderPermutationForPlatforms(xiiStringView sFile, const xiiArrayPtr<const xiiGALPermutationVariable>& permutationVariables, xiiLogInterface* pLog, xiiStringView sPlatform)
{
  XII_PROFILE_SCOPE("xiiGALShaderCompiler::CompileShaderPermutationForPlatforms");

  if (xiiRemoteToolingInterface* pTooling = xiiSingletonRegistry::GetSingletonInstance<xiiRemoteToolingInterface>())
  {
    auto pNet = pTooling->GetRemoteInterface();

    if (pNet && pNet->IsConnectedToServer())
    {
      m_bCompileShaderRemotely = true;

      pNet->SetMessageHandler('SHDR', xiiMakeDelegate(&xiiGALShaderCompiler::ShaderCompileMsg, this));

      xiiRemoteMessage msg('SHDR', 'CMPL');
      msg.GetWriter() << sFile;
      msg.GetWriter() << sPlatform;
      msg.GetWriter() << permutationVariables.GetCount();
      for (auto& pv : permutationVariables)
      {
        msg.GetWriter() << pv.m_sName;
        msg.GetWriter() << pv.m_sValue;
      }

      pNet->Send(xiiRemoteTransmitMode::Reliable, msg);

      while (m_bCompileShaderRemotely)
      {
        pNet->UpdateRemoteInterface();
        pNet->ExecuteAllMessageHandlers();
      }

      pNet->SetMessageHandler('SHDR', {});

      return m_RemoteShaderCompileResult;
    }
  }

  xiiStringBuilder sFileContent, sTemp;

  {
    xiiFileReader File;
    if (File.Open(sFile).Failed())
      return XII_FAILURE;

    sFileContent.ReadAll(File);
  }

  xiiGALShaderTextSectionizer sections;
  xiiGALShaderSections::GetShaderSections(sFileContent, sections);

  xiiUInt32 uiFirstLine = 0;
  sTemp                 = sections.GetSectionContent(xiiGALShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_sPlatform = sTemp;

  xiiHybridArray<xiiHashedString, 16> usedPermutations;
  xiiGALShaderParser::ParsePermutationSection(sections.GetSectionContent(xiiGALShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations, m_ShaderData.m_FixedPermutationVariables);

  for (const xiiHashedString& usedPermutationVariable : usedPermutations)
  {
    xiiUInt32 uiIndex = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < permutationVariables.GetCount(); ++i)
    {
      if (permutationVariables[i].m_sName == usedPermutationVariable)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != xiiInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVariables[uiIndex]);
    }
    else
    {
      xiiLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVariable);

      xiiGALPermutationVariable& finalVariable = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVariable.m_sName                    = usedPermutationVariable;
      finalVariable.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = sections.GetSectionContent(xiiGALShaderSections::RENDERSTATE, uiFirstLine);

  xiiUInt32     uiFirstShaderLine = 0;
  xiiStringView sShaderSource     = sections.GetSectionContent(xiiGALShaderSections::SHADER, uiFirstShaderLine);

  for (xiiUInt32 stage = xiiGALShaderType::GetStageIndex(xiiGALShaderType::Vertex); stage < xiiGALShaderType::ENUM_COUNT; ++stage)
  {
    xiiStringView sStageSource = sections.GetSectionContent(xiiGALShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // prepend common shader section if there is any
      if (!sShaderSource.IsEmpty())
      {
        sTemp.AppendFormat("#line {0}\n{1}", uiFirstShaderLine, sShaderSource);
      }

      sTemp.AppendFormat("#line {0}\n{1}", uiFirstLine, sStageSource);

      m_ShaderData.m_ShaderStageSource[xiiGALShaderType::GetStageFlag(stage)] = sTemp;
    }
  }

  xiiStringBuilder tmp = sFile;
  tmp.MakeCleanPath();

  struct StageSourceData
  {
    xiiEnum<xiiGALShaderType> m_ShaderType;
    xiiString                 m_sExtension;
  };

  xiiStaticArray<StageSourceData, xiiGALShaderType::ENUM_COUNT> shaderTypesAndExtensions;
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Vertex, .m_sExtension = "vs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Pixel, .m_sExtension = "ps"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Geometry, .m_sExtension = "gs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Hull, .m_sExtension = "hs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Domain, .m_sExtension = "ds"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Compute, .m_sExtension = "cs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Amplification, .m_sExtension = "as"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Mesh, .m_sExtension = "ms"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::RayGeneration, .m_sExtension = "rgs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::RayMiss, .m_sExtension = "rms"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::RayClosestHit, .m_sExtension = "rchs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::RayAnyHit, .m_sExtension = "rahs"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::RayIntersection, .m_sExtension = "ris"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Callable, .m_sExtension = "cas"});
  shaderTypesAndExtensions.PushBack({.m_ShaderType = xiiGALShaderType::Tile, .m_sExtension = "ts"});

  for (StageSourceData& stageSourceData : shaderTypesAndExtensions)
  {
    if (m_ShaderData.m_ShaderStageSource.Contains(stageSourceData.m_ShaderType))
    {
      auto& sSourceFile = m_StageSourceFile[stageSourceData.m_ShaderType];
      sSourceFile       = tmp;
      sSourceFile.ChangeFileExtension(stageSourceData.m_sExtension);
    }
  }

  // Try out every compiler that we can find
  // clang-format off
  xiiResult result = XII_SUCCESS;
  xiiRTTI::ForEachDerivedType<xiiGALShaderProgramCompiler>([&](const xiiRTTI* pRtti) {
    xiiUniquePtr<xiiGALShaderProgramCompiler> pCompiler = pRtti->GetAllocator()->Allocate<xiiGALShaderProgramCompiler>();

    if (RunShaderCompiler(sFile, sPlatform, pCompiler.Borrow(), pLog).Failed())
      result = XII_FAILURE;
  },
  xiiRTTI::ForEachOptions::ExcludeNonAllocatable);
  // clang-format on

  return result;
}

xiiResult xiiGALShaderCompiler::RunShaderCompiler(xiiStringView sFile, xiiStringView sPlatform, xiiGALShaderProgramCompiler* pCompiler, xiiLogInterface* pLog)
{
  XII_PROFILE_SCOPE("xiiGALShaderCompiler::RunShaderCompiler");

  XII_LOG_BLOCK(pLog, "Compiling Shader", sFile);

  xiiMap<xiiGALShaderType::Enum, xiiStringBuilder> processedSources;

  xiiHybridArray<xiiString, 4> platforms;
  pCompiler->GetSupportedPlatforms(platforms);

  for (xiiUInt32 p = 0; p < platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(sPlatform, platforms[p]))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_sPlatform, platforms[p]))
      continue;

    XII_LOG_BLOCK(pLog, "Platform", platforms[p]);

    xiiGALShaderProgramData spd;
    spd.m_sSourceFile = sFile;
    spd.m_sPlatform   = platforms[p];

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_sPlatform, "DEBUG"))
    {
      xiiLog::Warning(pLog, "Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");

      spd.m_Flags.Add(xiiGALShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    xiiHybridArray<xiiString, 32> defines;
    GenerateDefines(platforms[p], m_ShaderData.m_Permutations, defines);
    GenerateDefines(platforms[p], m_ShaderData.m_FixedPermutationVariables, defines);

    xiiGALShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      XII_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      xiiPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(xiiPreprocessor::FileOpenCB(&xiiGALShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      bool bFoundUndefinedVariables = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVariables, pLog](const xiiPreprocessor::ProcessingEvent& e) -> void {
        if (e.m_Type == xiiPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVariables = true;

          xiiLog::Error(pLog, "Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      xiiStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVariables)
      {
        xiiLog::Error(pLog, "Preprocessing the Shader State block failed.");
        return XII_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Parse(sOutput).Failed())
        {
          xiiLog::Error(pLog, "Failed to interpret the shader state block.");
          return XII_FAILURE;
        }
      }
    }

    // Shader Preprocessing
    for (auto it : m_ShaderData.m_ShaderStageSource)
    {
      auto& stageSource = it.Value();

      processedSources.Insert(it.Key(), xiiStringBuilder());
      xiiStringBuilder& sProcessedSource = *processedSources.GetValue(it.Key());

      if (stageSource.IsEmpty())
        continue;

      if (spd.m_StageData.Contains(it.Key()))
      {
        spd.m_StageData[it.Key()].m_uiSourceHash = 0U;
      }
      else
      {
        spd.m_StageData.Insert(it.Key(), xiiGALShaderProgramData::StageData());
      }

      bool bFoundUndefinedVariables = false;

      xiiPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(xiiPreprocessor::FileOpenCB(&xiiGALShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(xiiMakeDelegate(&xiiGALShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVariables, pLog](const xiiPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == xiiPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVariables = true;

          xiiLog::Error(pLog, "Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(s_szStageDefines[xiiGALShaderType::GetStageIndex(it.Key())]));
      for (auto& define : defines)
      {
        XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      if (pp.Process(m_StageSourceFile[it.Key()], sProcessedSource, true, true, true).Failed() || bFoundUndefinedVariables)
      {
        sProcessedSource.Clear();
        spd.m_StageData[it.Key()].m_sShaderSource = m_StageSourceFile[it.Key()];

        xiiLog::Error(pLog, "Shader preprocessing failed.");
        return XII_FAILURE;
      }
      else
      {
        spd.m_StageData[it.Key()].m_sShaderSource = sProcessedSource;
      }
    }

    // Let the shader compiler make any modifications to the source code before we hash and compile the shader.
    if (pCompiler->ModifyShaderSource(spd, pLog).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return XII_FAILURE;
    }

    // Load shader cache.
    for (auto it : spd.m_StageData)
    {
      auto& stageData = it.Value();

      xiiUInt32 uiSourceStringLength = stageData.m_sShaderSource.GetElementCount();
      stageData.m_uiSourceHash       = uiSourceStringLength == 0U ? 0U : xiiHashingUtils::xxHash32(stageData.m_sShaderSource.GetData(), uiSourceStringLength);

      if (stageData.m_uiSourceHash != 0U)
      {
        xiiGALShaderStageBinary* pBinary = xiiGALShaderStageBinary::LoadStageBinary(it.Key(), stageData.m_uiSourceHash, sPlatform);

        if (pBinary)
        {
          stageData.m_pByteCode    = pBinary->m_pGALByteCode;
          stageData.m_bWriteToDisk = false;
        }
        else
        {
          // Can't find shader with given hash on disk, create a new xiiGALShaderByteCode and let the compiler build it.
          stageData.m_pByteCode                          = XII_DEFAULT_NEW(xiiGALShaderByteCode);
          stageData.m_pByteCode->m_ShaderStage           = it.Key();
          stageData.m_pByteCode->m_bWasCompiledWithDebug = spd.m_Flags.IsSet(xiiGALShaderCompilerFlags::Debug);
        }
      }
    }

    // copy the source hashes
    for (auto it : spd.m_StageData)
    {
      auto& stageData = it.Value();

      shaderPermutationBinary.m_ShaderStageHashes.Insert(it.Key(), stageData.m_uiSourceHash);
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .xiiPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return XII_FAILURE;
    }

    for (auto it : spd.m_StageData)
    {
      auto& stageData = it.Value();

      if (stageData.m_uiSourceHash != 0 && stageData.m_bWriteToDisk)
      {
        xiiGALShaderStageBinary bin;
        bin.m_uiSourceHash = stageData.m_uiSourceHash;
        bin.m_pGALByteCode = stageData.m_pByteCode;

        if (bin.WriteStageBinary(pLog, sPlatform).Failed())
        {
          xiiLog::Error(pLog, "Writing stage {0} binary failed.", it.Key());
          return XII_FAILURE;
        }

        xiiGALShaderStageBinary::s_ShaderStageBinaries[xiiGALShaderType::GetStageIndex(it.Key())].Insert(bin.m_uiSourceHash, bin);
      }
    }

    xiiStringBuilder sTemp = xiiGALShaderManager::GetCacheDirectory();
    sTemp.AppendPath(platforms[p]);
    sTemp.AppendPath(sFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const xiiUInt32 uiPermutationHash = xiiGALPermutationVariable::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(sFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVariables = m_ShaderData.m_Permutations;

    xiiDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp);
    XII_SUCCEED_OR_RETURN(shaderPermutationBinary.Write(PermutationFileOut));

    if (PermutationFileOut.Close().Failed())
    {
      xiiLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiGALShaderCompiler::WriteFailedShaderSource(xiiGALShaderProgramData& spd, xiiLogInterface* pLog)
{
  XII_PROFILE_SCOPE("xiiGALShaderCompiler::WriteFailedShaderSource");

  for (auto it : spd.m_StageData)
  {
    const auto& stageData = it.Value();

    if (stageData.m_uiSourceHash != 0 && stageData.m_bWriteToDisk)
    {
      xiiStringBuilder sShaderStageFile = xiiGALShaderManager::GetCacheDirectory();
      sShaderStageFile.AppendPath(xiiGALShaderManager::GetActivePlatform());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.xiiShaderSource", xiiGALShaderType::Names[it.Key()], xiiArgU(stageData.m_uiSourceHash, 8, true, 16, true));

      xiiFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile).Succeeded())
      {
        StageFileOut.WriteBytes(stageData.m_sShaderSource.GetData(), stageData.m_sShaderSource.GetElementCount()).AssertSuccess();

        xiiLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

void xiiGALShaderCompiler::ShaderCompileMsg(xiiRemoteMessage& msg)
{
  if (msg.GetMessageID() == 'CRES')
  {
    m_bCompileShaderRemotely    = false;
    m_RemoteShaderCompileResult = XII_SUCCESS;

    bool bSucceeded = false;
    msg.GetReader() >> bSucceeded;
    m_RemoteShaderCompileResult = bSucceeded ? XII_SUCCESS : XII_FAILURE;

    xiiStringBuilder sLog;
    msg.GetReader() >> sLog;

    if (!bSucceeded)
    {
      xiiLog::Error("Shader compilation failed:\n{}", sLog);
    }
  }
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_ShaderCompiler_Implementation_ShaderCompiler);
