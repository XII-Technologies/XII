#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderProgramCompiler, 1, xiiRTTINoAllocator)
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

  static void GenerateDefines(xiiStringView sPlatform, const xiiArrayPtr<xiiPermutationVar>& permutationVars, xiiHybridArray<xiiString, 32>& out_defines)
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

    for (const xiiPermutationVar& var : permutationVars)
    {
      xiiStringView sValue    = var.m_sValue;
      const bool    isBoolVar = sValue == "TRUE" || sValue == "FALSE";

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_defines.PushBack(sTemp);
      }
      else
      {
        xiiStringView sName      = var.m_sName;
        auto          enumValues = xiiShaderManager::GetPermutationEnumValues(var.m_sName);

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

  static const char* s_szStageDefines[xiiGALShaderStage::ENUM_COUNT] = {
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

xiiResult xiiShaderCompiler::FileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification)
{
  if (sAbsoluteFile == "ShaderRenderState")
  {
    const xiiString& sData   = m_ShaderData.m_StateSource;
    const xiiUInt32  uiCount = sData.GetElementCount();
    xiiStringView    sString = sData;

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      xiiMemoryUtils::Copy<xiiUInt8>(FileContent.GetData(), (const xiiUInt8*)sString.GetStartPointer(), uiCount);
    }

    return XII_SUCCESS;
  }

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == sAbsoluteFile)
    {
      const xiiString& sData    = m_ShaderData.m_ShaderStageSource[stage];
      const xiiUInt32  uiCount  = sData.GetElementCount();
      const char*      szString = sData;

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        xiiMemoryUtils::Copy<xiiUInt8>(FileContent.GetData(), (const xiiUInt8*)szString, uiCount);
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
    out_FileModification = stats.m_LastModificationTime;
  }
#endif

  xiiUInt8 Temp[4096];

  while (xiiUInt64 uiRead = r.ReadBytes(Temp, 4096))
  {
    FileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompiler::CompileShaderPermutationForPlatforms(xiiStringView sFile, const xiiArrayPtr<const xiiPermutationVar>& permutationVars, xiiLogInterface* pLog, xiiStringView sPlatform)
{
  xiiStringBuilder sFileContent, sTemp;

  {
    xiiFileReader File;
    if (File.Open(sFile).Failed())
      return XII_FAILURE;

    sFileContent.ReadAll(File);
  }

  xiiShaderHelper::xiiTextSectionizer Sections;
  xiiShaderHelper::GetShaderSections(sFileContent, Sections);

  xiiUInt32 uiFirstLine = 0;
  sTemp                 = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::PLATFORMS, uiFirstLine);
  sTemp.ToUpper();

  m_ShaderData.m_Platforms = sTemp;

  xiiHybridArray<xiiHashedString, 16> usedPermutations;
  xiiShaderParser::ParsePermutationSection(Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::PERMUTATIONS, uiFirstLine), usedPermutations, m_ShaderData.m_FixedPermVars);

  for (const xiiHashedString& usedPermutationVar : usedPermutations)
  {
    xiiUInt32 uiIndex = xiiInvalidIndex;
    for (xiiUInt32 i = 0; i < permutationVars.GetCount(); ++i)
    {
      if (permutationVars[i].m_sName == usedPermutationVar)
      {
        uiIndex = i;
        break;
      }
    }

    if (uiIndex != xiiInvalidIndex)
    {
      m_ShaderData.m_Permutations.PushBack(permutationVars[uiIndex]);
    }
    else
    {
      xiiLog::Error("No value given for permutation var '{0}'. Assuming default value of zero.", usedPermutationVar);

      xiiPermutationVar& finalVar = m_ShaderData.m_Permutations.ExpandAndGetRef();
      finalVar.m_sName            = usedPermutationVar;
      finalVar.m_sValue.Assign("0");
    }
  }

  m_ShaderData.m_StateSource = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::RENDERSTATE, uiFirstLine);

  xiiUInt32     uiFirstShaderLine = 0;
  xiiStringView sShaderSource     = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::SHADER, uiFirstShaderLine);

  for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    xiiStringView sStageSource = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::VERTEXSHADER + stage, uiFirstLine);

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

      m_ShaderData.m_ShaderStageSource[stage] = sTemp;
    }
    else
    {
      m_ShaderData.m_ShaderStageSource[stage].Clear();
    }
  }

  xiiStringBuilder tmp = sFile;
  tmp.MakeCleanPath();

  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("vs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Pixel)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("ps");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Geometry)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("gs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Hull)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("hs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Domain)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("ds");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Compute)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("cs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Amplification)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("as");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Mesh)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("ms");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayGeneration)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("rg");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayMiss)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("rms");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayClosestHit)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("rchs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayAnyHit)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("rahs");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::RayIntersection)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("ris");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Callable)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("cas");
  }
  {
    auto& sSourceFile = m_StageSourceFile[xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Tile)];
    sSourceFile       = tmp;
    sSourceFile.ChangeFileExtension("ts");
  }

  // Try out every compiler that we can find
  xiiResult result = XII_SUCCESS;
  xiiRTTI::ForEachDerivedType<xiiShaderProgramCompiler>(
    [&](const xiiRTTI* pRtti) {
      xiiUniquePtr<xiiShaderProgramCompiler> pCompiler = pRtti->GetAllocator()->Allocate<xiiShaderProgramCompiler>();

      if (RunShaderCompiler(sFile, sPlatform, pCompiler.Borrow(), pLog).Failed())
        result = XII_FAILURE;
    },
    xiiRTTI::ForEachOptions::ExcludeNonAllocatable);

  return result;
}

xiiResult xiiShaderCompiler::RunShaderCompiler(xiiStringView sFile, xiiStringView sPlatform, xiiShaderProgramCompiler* pCompiler, xiiLogInterface* pLog)
{
  XII_LOG_BLOCK(pLog, "Compiling Shader", sFile);

  xiiStringBuilder sProcessed[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<xiiString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);

  for (xiiUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(sPlatform, Platforms[p]))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p]))
      continue;

    XII_LOG_BLOCK(pLog, "Platform", Platforms[p]);

    xiiShaderProgramData spd;
    spd.m_sSourceFile = sFile;
    spd.m_sPlatform   = Platforms[p];

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    // 'DEBUG' is a platform tag that enables additional compiler flags
    if (PlatformEnabled(m_ShaderData.m_Platforms, "DEBUG"))
    {
      xiiLog::Warning("Shader specifies the 'DEBUG' platform, which enables the debug shader compiler flag.");
      spd.m_Flags.Add(xiiShaderCompilerFlags::Debug);
    }
#endif

    m_IncludeFiles.Clear();

    xiiHybridArray<xiiString, 32> defines;
    GenerateDefines(Platforms[p], m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p], m_ShaderData.m_FixedPermVars, defines);

    xiiShaderPermutationBinary shaderPermutationBinary;

    // Generate Shader State Source
    {
      XII_LOG_BLOCK(pLog, "Preprocessing Shader State Source");

      xiiPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(xiiPreprocessor::FileOpenCB(&xiiShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(false);
      pp.SetPassThroughLine(false);

      for (auto& define : defines)
      {
        XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      bool bFoundUndefinedVars = false;
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const xiiPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == xiiPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          xiiLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      xiiStringBuilder sOutput;
      if (pp.Process("ShaderRenderState", sOutput, false).Failed() || bFoundUndefinedVars)
      {
        xiiLog::Error(pLog, "Preprocessing the Shader State block failed");
        return XII_FAILURE;
      }
      else
      {
        if (shaderPermutationBinary.m_StateDescriptor.Parse(sOutput).Failed())
        {
          xiiLog::Error(pLog, "Failed to interpret the shader state block");
          return XII_FAILURE;
        }
      }
    }

    // Shader Preprocessing
    for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_uiSourceHash[stage] = 0;

      if (m_ShaderData.m_ShaderStageSource[stage].IsEmpty())
        continue;

      bool bFoundUndefinedVars = false;

      xiiPreprocessor pp;
      pp.SetCustomFileCache(&m_FileCache);
      pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
      pp.SetFileOpenFunction(xiiPreprocessor::FileOpenCB(&xiiShaderCompiler::FileOpen, this));
      pp.SetPassThroughPragma(true);
      pp.SetPassThroughUnknownCmdsCB(xiiMakeDelegate(&xiiShaderCompiler::PassThroughUnknownCommandCB, this));
      pp.SetPassThroughLine(false);
      pp.m_ProcessingEvents.AddEventHandler([&bFoundUndefinedVars](const xiiPreprocessor::ProcessingEvent& e) {
        if (e.m_Type == xiiPreprocessor::ProcessingEvent::EvaluateUnknown)
        {
          bFoundUndefinedVars = true;

          xiiLog::Error("Undefined variable is evaluated: '{0}' (File: '{1}', Line: {2}", e.m_pToken->m_DataView, e.m_pToken->m_File, e.m_pToken->m_uiLine);
        }
      });

      XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(s_szStageDefines[stage]));
      for (auto& define : defines)
      {
        XII_SUCCEED_OR_RETURN(pp.AddCustomDefine(define));
      }

      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_sShaderSource[stage] = m_StageSourceFile[stage];

        xiiLog::Error(pLog, "Shader preprocessing failed");
        return XII_FAILURE;
      }
      else
      {
        spd.m_sShaderSource[stage] = sProcessed[stage];
      }
    }

    // Let the shader compiler make any modifications to the source code before we hash and compile the shader.
    if (pCompiler->ModifyShaderSource(spd, pLog).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return XII_FAILURE;
    }

    // Load shader cache
    for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      xiiUInt32 uiSourceStringLen = spd.m_sShaderSource[stage].GetElementCount();
      spd.m_uiSourceHash[stage]   = uiSourceStringLen == 0 ? 0u : xiiHashingUtils::xxHash32(spd.m_sShaderSource[stage].GetData(), uiSourceStringLen);

      if (spd.m_uiSourceHash[stage] != 0)
      {
        xiiShaderStageBinary* pBinary = xiiShaderStageBinary::LoadStageBinary(xiiGALShaderStage::GetStageFlag(stage), spd.m_uiSourceHash[stage]);

        if (pBinary)
        {
          spd.m_ByteCode[stage]     = pBinary->m_pGALByteCode;
          spd.m_bWriteToDisk[stage] = false;
        }
        else
        {
          // Can't find shader with given hash on disk, create a new xiiGALShaderByteCode and let the compiler build it.
          spd.m_ByteCode[stage]                          = XII_DEFAULT_NEW(xiiGALShaderByteCode);
          spd.m_ByteCode[stage]->m_ShaderStage           = xiiGALShaderStage::GetStageFlag(stage);
          spd.m_ByteCode[stage]->m_bWasCompiledWithDebug = spd.m_Flags.IsSet(xiiShaderCompilerFlags::Debug);
        }
      }
    }

    // copy the source hashes
    for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_uiSourceHash[stage];
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .xiiPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return XII_FAILURE;
    }

    for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
      {
        xiiShaderStageBinary bin;
        bin.m_uiSourceHash = spd.m_uiSourceHash[stage];
        bin.m_pGALByteCode = spd.m_ByteCode[stage];

        if (bin.WriteStageBinary(pLog).Failed())
        {
          xiiLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return XII_FAILURE;
        }
        xiiShaderStageBinary::s_ShaderStageBinaries[stage].Insert(bin.m_uiSourceHash, bin);
      }
    }

    xiiStringBuilder sTemp = xiiShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p]);
    sTemp.AppendPath(sFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const xiiUInt32 uiPermutationHash = xiiShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(sFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

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

void xiiShaderCompiler::WriteFailedShaderSource(xiiShaderProgramData& spd, xiiLogInterface* pLog)
{
  for (xiiUInt32 stage = xiiGALShaderStage::GetStageIndex(xiiGALShaderStage::Vertex); stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_uiSourceHash[stage] != 0 && spd.m_bWriteToDisk[stage])
    {
      xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(xiiShaderManager::GetActivePlatform());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.xiiShaderSource", xiiGALShaderStage::Names[stage], xiiArgU(spd.m_uiSourceHash[stage], 8, true, 16, true));

      xiiFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_sShaderSource[stage].GetData(), spd.m_sShaderSource[stage].GetElementCount()).AssertSuccess();
        xiiLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_ShaderCompiler_Implementation_ShaderCompiler);
