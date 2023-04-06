#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <RendererCore/ShaderCompiler/ShaderCompiler.h>
#include <RendererCore/ShaderCompiler/ShaderManager.h>
#include <RendererCore/ShaderCompiler/ShaderParser.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiShaderProgramCompiler, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static bool PlatformEnabled(const xiiString& sPlatforms, const char* szPlatform)
  {
    xiiStringBuilder sTemp;
    sTemp = szPlatform;

    sTemp.Prepend("!");

    // If it contains '!platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return false;

    sTemp = szPlatform;

    // If it contains 'platform'
    if (sPlatforms.FindWholeWord_NoCase(sTemp.GetData(), xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    // Do not enable this when ALL is specified
    if (xiiStringUtils::IsEqual(szPlatform, "DEBUG"))
      return false;

    // If it contains 'ALL'
    if (sPlatforms.FindWholeWord_NoCase("ALL", xiiStringUtils::IsIdentifierDelimiter_C_Code) != nullptr)
      return true;

    return false;
  }

  static void GenerateDefines(const char* szPlatform, const xiiArrayPtr<xiiPermutationVar>& permutationVars, xiiHybridArray<xiiString, 32>& out_Defines)
  {
    xiiStringBuilder sTemp;

    if (out_Defines.IsEmpty())
    {
      out_Defines.PushBack("TRUE 1");
      out_Defines.PushBack("FALSE 0");

      sTemp = szPlatform;
      sTemp.ToUpper();

      out_Defines.PushBack(sTemp.GetData());
    }

    for (const xiiPermutationVar& var : permutationVars)
    {
      const char* szValue   = var.m_sValue.GetData();
      const bool  isBoolVar = xiiStringUtils::IsEqual(szValue, "TRUE") || xiiStringUtils::IsEqual(szValue, "FALSE");

      if (isBoolVar)
      {
        sTemp.Set(var.m_sName, " ", var.m_sValue);
        out_Defines.PushBack(sTemp);
      }
      else
      {
        const char* szName     = var.m_sName.GetData();
        auto        enumValues = xiiShaderManager::GetPermutationEnumValues(var.m_sName);

        for (const auto& ev : enumValues)
        {
          sTemp.Format("{1} {2}", szName, ev.m_sValueName, ev.m_iValueValue);
          out_Defines.PushBack(sTemp);
        }

        if (xiiStringUtils::StartsWith(szValue, szName))
        {
          sTemp.Set(szName, " ", szValue);
        }
        else
        {
          sTemp.Set(szName, " ", szName, "_", szValue);
        }
        out_Defines.PushBack(sTemp);
      }
    }
  }

  static const char* s_szStageDefines[xiiGALShaderStage::ENUM_COUNT] = {
    "VERTEX_SHADER", "PIXEL_SHADER", "GEOMETRY_SHADER", "HULL_SHADER", "DOMAIN_SHADER", "COMPUTE_SHADER", "AMPLIFICATION_SHADER", "MESH_SHADER", "RAYGEN_SHADER", "RAYMISS_SHADER", "RAYANYHIT_SHADER", "RAYCLOSESTHIT_SHADER", "RAYINTERSECTION_SHADER", "CALLABLE_SHADER"};
} // namespace

xiiResult xiiShaderCompiler::FileOpen(const char* szAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification)
{
  if (xiiStringUtils::IsEqual(szAbsoluteFile, "ShaderRenderState"))
  {
    const xiiString& sData    = m_ShaderData.m_StateSource;
    const xiiUInt32  uiCount  = sData.GetElementCount();
    const char*      szString = sData.GetData();

    FileContent.SetCountUninitialized(uiCount);

    if (uiCount > 0)
    {
      xiiMemoryUtils::Copy<xiiUInt8>(FileContent.GetData(), (const xiiUInt8*)szString, uiCount);
    }

    return XII_SUCCESS;
  }

  for (xiiUInt32 stage = 0; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_StageSourceFile[stage] == szAbsoluteFile)
    {
      const xiiString& sData    = m_ShaderData.m_ShaderStageSource[stage];
      const xiiUInt32  uiCount  = sData.GetElementCount();
      const char*      szString = sData.GetData();

      FileContent.SetCountUninitialized(uiCount);

      if (uiCount > 0)
      {
        xiiMemoryUtils::Copy<xiiUInt8>(FileContent.GetData(), (const xiiUInt8*)szString, uiCount);
      }

      return XII_SUCCESS;
    }
  }

  m_IncludeFiles.Insert(szAbsoluteFile);

  xiiFileReader r;
  if (r.Open(szAbsoluteFile).Failed())
  {
    xiiLog::Error("Could not find include file '{0}'", szAbsoluteFile);
    return XII_FAILURE;
  }

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stats;
  if (xiiFileSystem::GetFileStats(szAbsoluteFile, stats).Succeeded())
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

xiiResult xiiShaderCompiler::CompileShaderPermutationForPlatforms(const char* szFile, const xiiArrayPtr<const xiiPermutationVar>& permutationVars, xiiLogInterface* pLog, const char* szPlatform)
{
  xiiStringBuilder sFileContent, sTemp;

  {
    xiiFileReader File;
    if (File.Open(szFile).Failed())
      return XII_FAILURE;

    sFileContent.ReadAll(File);
  }

  xiiShaderHelper::xiiTextSectionizer Sections;
  xiiShaderHelper::GetShaderSections(sFileContent.GetData(), Sections);

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

  for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    xiiStringView sStageSource = Sections.GetSectionContent(xiiShaderHelper::xiiShaderSections::VERTEXSHADER + stage, uiFirstLine);

    // Later code checks whether the string is empty, to see whether we have any shader source, so this has to be kept empty
    if (!sStageSource.IsEmpty())
    {
      sTemp.Clear();

      // Prepend common shader section if there is any
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

  xiiStringBuilder tmp = szFile;
  tmp.MakeCleanPath();

  m_StageSourceFile[xiiGALShaderStage::VertexShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::VertexShader].ChangeFileExtension("vs");

  m_StageSourceFile[xiiGALShaderStage::HullShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::HullShader].ChangeFileExtension("hs");

  m_StageSourceFile[xiiGALShaderStage::DomainShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::DomainShader].ChangeFileExtension("ds");

  m_StageSourceFile[xiiGALShaderStage::GeometryShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::GeometryShader].ChangeFileExtension("gs");

  m_StageSourceFile[xiiGALShaderStage::PixelShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::PixelShader].ChangeFileExtension("ps");

  m_StageSourceFile[xiiGALShaderStage::ComputeShader] = tmp;
  m_StageSourceFile[xiiGALShaderStage::ComputeShader].ChangeFileExtension("cs");

  // try out every compiler that we can find
  xiiRTTI* pRtti = xiiRTTI::GetFirstInstance();
  while (pRtti)
  {
    xiiRTTIAllocator* pAllocator = pRtti->GetAllocator();
    if (pRtti->IsDerivedFrom<xiiShaderProgramCompiler>() && pAllocator->CanAllocate())
    {
      xiiShaderProgramCompiler* pCompiler = pAllocator->Allocate<xiiShaderProgramCompiler>();

      const xiiResult ret = RunShaderCompiler(szFile, szPlatform, pCompiler, pLog);
      pAllocator->Deallocate(pCompiler);

      if (ret.Failed())
        return ret;
    }

    pRtti = pRtti->GetNextInstance();
  }

  return XII_SUCCESS;
}

xiiResult xiiShaderCompiler::RunShaderCompiler(const char* szFile, const char* szPlatform, xiiShaderProgramCompiler* pCompiler, xiiLogInterface* pLog)
{
  XII_LOG_BLOCK(pLog, "Compiling Shader", szFile);

  xiiStringBuilder sProcessed[xiiGALShaderStage::ENUM_COUNT];

  xiiHybridArray<xiiString, 4> Platforms;
  pCompiler->GetSupportedPlatforms(Platforms);

  for (xiiUInt32 p = 0; p < Platforms.GetCount(); ++p)
  {
    if (!PlatformEnabled(szPlatform, Platforms[p].GetData()))
      continue;

    // if this shader is not tagged for this platform, ignore it
    if (!PlatformEnabled(m_ShaderData.m_Platforms, Platforms[p].GetData()))
      continue;

    XII_LOG_BLOCK(pLog, "Platform", Platforms[p].GetData());

    xiiShaderProgramCompiler::xiiShaderProgramData spd;
    spd.m_szSourceFile = szFile;
    spd.m_szPlatform   = Platforms[p].GetData();

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
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_Permutations, defines);
    GenerateDefines(Platforms[p].GetData(), m_ShaderData.m_FixedPermVars, defines);

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
        if (shaderPermutationBinary.m_StateDescriptor.Load(sOutput).Failed())
        {
          xiiLog::Error(pLog, "Failed to interpret the shader state block");
          return XII_FAILURE;
        }
      }
    }

    for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      spd.m_StageBinary[stage].m_Stage        = (xiiGALShaderStage::Enum)stage;
      spd.m_StageBinary[stage].m_uiSourceHash = 0;

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

      xiiUInt32 uiSourceStringLen = 0;
      if (pp.Process(m_StageSourceFile[stage], sProcessed[stage], true, true, true).Failed() || bFoundUndefinedVars)
      {
        sProcessed[stage].Clear();
        spd.m_szShaderSource[stage] = m_StageSourceFile[stage];
        uiSourceStringLen           = m_StageSourceFile[stage].GetElementCount();

        xiiLog::Error(pLog, "Shader preprocessing failed");
        return XII_FAILURE;
      }
      else
      {
        spd.m_szShaderSource[stage] = sProcessed[stage];
        uiSourceStringLen           = sProcessed[stage].GetElementCount();
      }

      spd.m_StageBinary[stage].m_uiSourceHash = xiiHashingUtils::xxHash32(spd.m_szShaderSource[stage], uiSourceStringLen);

      if (spd.m_StageBinary[stage].m_uiSourceHash != 0)
      {
        xiiShaderStageBinary* pBinary = xiiShaderStageBinary::LoadStageBinary((xiiGALShaderStage::Enum)stage, spd.m_StageBinary[stage].m_uiSourceHash);

        if (pBinary)
        {
          spd.m_StageBinary[stage]  = *pBinary;
          spd.m_bWriteToDisk[stage] = pBinary->GetByteCode().IsEmpty();
        }
      }
    }

    // copy the source hashes
    for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      shaderPermutationBinary.m_uiShaderStageHashes[stage] = spd.m_StageBinary[stage].m_uiSourceHash;
    }

    // if compilation failed, the stage binary for the source hash will simply not exist and therefore cannot be loaded
    // the .xiiPermutation file should be updated, however, to store the new source hash to the broken shader
    if (pCompiler->Compile(spd, xiiLog::GetThreadLocalLogSystem()).Failed())
    {
      WriteFailedShaderSource(spd, pLog);
      return XII_FAILURE;
    }

    for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
    {
      if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
      {
        spd.m_StageBinary[stage].m_bWasCompiledWithDebug = spd.m_Flags.IsSet(xiiShaderCompilerFlags::Debug);

        if (spd.m_StageBinary[stage].WriteStageBinary(pLog).Failed())
        {
          xiiLog::Error(pLog, "Writing stage {0} binary failed", stage);
          return XII_FAILURE;
        }
      }
    }

    xiiStringBuilder sTemp = xiiShaderManager::GetCacheDirectory();
    sTemp.AppendPath(Platforms[p].GetData());
    sTemp.AppendPath(szFile);
    sTemp.ChangeFileExtension("");
    if (sTemp.EndsWith("."))
      sTemp.Shrink(0, 1);

    const xiiUInt32 uiPermutationHash = xiiShaderHelper::CalculateHash(m_ShaderData.m_Permutations);
    sTemp.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    shaderPermutationBinary.m_DependencyFile.Clear();
    shaderPermutationBinary.m_DependencyFile.AddFileDependency(szFile);

    for (auto it = m_IncludeFiles.GetIterator(); it.IsValid(); ++it)
    {
      shaderPermutationBinary.m_DependencyFile.AddFileDependency(it.Key());
    }

    shaderPermutationBinary.m_PermutationVars = m_ShaderData.m_Permutations;

    xiiDeferredFileWriter PermutationFileOut;
    PermutationFileOut.SetOutput(sTemp.GetData());
    XII_SUCCEED_OR_RETURN(shaderPermutationBinary.Write(PermutationFileOut));

    if (PermutationFileOut.Close().Failed())
    {
      xiiLog::Error(pLog, "Could not open file for writing: '{0}'", sTemp);
      return XII_FAILURE;
    }
  }

  return XII_SUCCESS;
}

void xiiShaderCompiler::WriteFailedShaderSource(xiiShaderProgramCompiler::xiiShaderProgramData& spd, xiiLogInterface* pLog)
{
  for (xiiUInt32 stage = xiiGALShaderStage::VertexShader; stage < xiiGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (spd.m_StageBinary[stage].m_uiSourceHash != 0 && spd.m_bWriteToDisk[stage])
    {
      xiiStringBuilder sShaderStageFile = xiiShaderManager::GetCacheDirectory();

      sShaderStageFile.AppendPath(xiiShaderManager::GetActivePlatform().GetData());
      sShaderStageFile.AppendFormat("/_Failed_{0}_{1}.xiiShaderSource", xiiGALShaderStage::Names[stage], xiiArgU(spd.m_StageBinary[stage].m_uiSourceHash, 8, true, 16, true));

      xiiFileWriter StageFileOut;
      if (StageFileOut.Open(sShaderStageFile.GetData()).Succeeded())
      {
        StageFileOut.WriteBytes(spd.m_szShaderSource[stage], xiiStringUtils::GetStringElementCount(spd.m_szShaderSource[stage])).IgnoreResult();
        xiiLog::Info(pLog, "Failed shader source written to '{0}'", sShaderStageFile);
      }
    }
  }
}

XII_STATICLINK_FILE(RendererCore, RendererCore_ShaderCompiler_Implementation_ShaderCompiler);
