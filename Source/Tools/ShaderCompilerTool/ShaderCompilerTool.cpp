#include <ShaderCompilerTool/ShaderCompilerTool.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/CommandLineOptions.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>

xiiCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .xiiShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
                                      "");

xiiCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Absolute path to the folder of the project, for which shaders should be compiled.",
                                     "");

xiiCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform VK_SM60\n\
  -platform ALL",
                                        "VK_SM60");

xiiCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

xiiCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE\n\
\n\
If a permutation variable is not set to a fixed value, all shader permutations for that variable will generated and compiled.\n\
",
                                 "");

xiiShaderCompilerApplication::xiiShaderCompilerApplication() :
  xiiGameApplication("xiiShaderCompiler", nullptr)
{
}

xiiResult xiiShaderCompilerApplication::BeforeCoreSystemsStartup()
{
  {
    xiiStringBuilder cmdHelp;
    if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_ShaderCompiler"))
    {
      xiiLog::Print(cmdHelp);
      return XII_FAILURE;
    }
  }

  xiiStartup::AddApplicationTag("tool");
  xiiStartup::AddApplicationTag("shadercompiler");

  // only print important messages
  xiiLog::SetDefaultLogLevel(xiiLogMsgType::InfoMsg);

  XII_SUCCEED_OR_RETURN(SUPER::BeforeCoreSystemsStartup());

  auto cmd = xiiCommandLineUtils::GetGlobalInstance();

  m_sShaderFiles = opt_Shader.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  XII_ASSERT_ALWAYS(!m_sShaderFiles.IsEmpty(), "Shader file has not been specified. Use the -shader command followed by a path");

  m_sAppProjectPath = opt_Project.GetOptionValue(xiiCommandLineOption::LogMode::Always);
  XII_ASSERT_ALWAYS(!m_sAppProjectPath.IsEmpty(), "Project directory has not been specified. Use the -project command followed by a path");

  m_sPlatforms = opt_Platform.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  m_bIgnoreErrors = opt_IgnoreErrors.GetOptionValue(xiiCommandLineOption::LogMode::Always);

  const xiiUInt32 pvs = cmd->GetStringOptionArguments("-perm");

  for (xiiUInt32 pv = 0; pv < pvs; ++pv)
  {
    xiiStringBuilder var = cmd->GetStringOption("-perm", pv);

    const char* szEqual = var.FindSubString("=");

    if (szEqual == nullptr)
    {
      xiiLog::Error("Permutation Variable declaration contains no equal sign: '{0}'", var);
      continue;
    }

    xiiStringBuilder val = szEqual + 1;
    var.SetSubString_FromTo(var.GetData(), szEqual);

    val.Trim(" \t");
    var.Trim(" \t");

    xiiLog::Dev("Fixed permutation variable: {0} = {1}", var, val);
    m_FixedPermVars[var].PushBack(val);
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerApplication::AfterCoreSystemsStartup()
{
  xiiSystemInformation info      = xiiSystemInformation::Get();
  const xiiInt32       iCpuCores = info.GetCPUCoreCount();
  xiiTaskSystem::SetWorkerThreadCount(iCpuCores);

  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();
}

xiiResult xiiShaderCompilerApplication::CompileShader(xiiStringView sShaderFile)
{
  XII_PROFILE_SCOPE("xiiShaderCompilerApplication::CompileShader");

  XII_LOG_BLOCK("Compiling Shader", sShaderFile);

  if (ExtractPermutationVarValues(sShaderFile).Failed())
    return XII_FAILURE;

  const xiiUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  xiiLog::Info("Shader has {0} permutations", uiMaxPerms);

  bool bContinue = true;

  xiiTaskSystem::ParallelForIndexed(0, uiMaxPerms, [&](xiiUInt32 uiIndex, xiiUInt32 uiCount) {
    if (!bContinue)
      return;

    xiiHybridArray<xiiGALPermutationVariable, 16> permutationVariables;
    xiiTokenizedFileCache                         fileCache;

    for (xiiUInt32 uiPermutationIndex = uiIndex; uiPermutationIndex < uiCount; ++uiPermutationIndex)
    {
      XII_PROFILE_SCOPE("CompilePermutation");

      XII_LOG_BLOCK("Compiling Permutation");

      m_PermutationGenerator.GetPermutation(uiPermutationIndex, permutationVariables);

      xiiGALShaderCompiler shaderCompiler;
      if (shaderCompiler.CompileShaderPermutationForPlatforms(sShaderFile, permutationVariables, xiiLog::GetThreadLocalLogSystem(), m_sPlatforms, &fileCache).Failed())
      {
        bContinue = false;
        return;
      }
    }
  });

  if (!bContinue)
  {
    xiiLog::Error("Failed to compile shader '{0}'", sShaderFile);
    return XII_FAILURE;
  }

  xiiLog::Success("Compiled Shader '{0}'", sShaderFile);
  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerApplication::ExtractPermutationVarValues(xiiStringView sShaderFile)
{
  XII_PROFILE_SCOPE("xiiShaderCompilerApplication::ExtractPermutationVarValues");

  m_PermutationGenerator.Clear();

  xiiFileReader shaderFile;
  if (shaderFile.Open(sShaderFile).Failed())
  {
    xiiLog::Error("Could not open file '{0}'", sShaderFile);
    return XII_FAILURE;
  }

  xiiString sContent;
  sContent.ReadAll(shaderFile);

  xiiGALShaderTextSectionizer shaderTextSections;
  xiiGALShaderSections::GetShaderSections(sContent, shaderTextSections);

  xiiHybridArray<xiiHashedString, 16>           permutationVariables;
  xiiHybridArray<xiiGALPermutationVariable, 16> fixedPermutationVariables;

  xiiUInt32     uiFirstLine   = 0;
  xiiStringView sPermutations = shaderTextSections.GetSectionContent(xiiGALShaderSections::Permutations, uiFirstLine);
  xiiGALShaderParser::ParsePermutationSection(sPermutations, permutationVariables, fixedPermutationVariables);

  {
    XII_LOG_BLOCK("Permutation Variables");

    for (const auto& s : permutationVariables)
    {
      xiiLog::Dev(s.GetView());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permutationVariables)
    {
      xiiHybridArray<xiiHashedString, 16> values;
      xiiGALShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermutationVariables)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      xiiHashedString sNameHash, sPermutationValue;
      sNameHash.Assign(it.Key().GetView());
      m_PermutationGenerator.RemovePermutations(sNameHash);

      for (const auto& val : it.Value())
      {
        sPermutationValue.Assign(val.GetView());

        m_PermutationGenerator.AddPermutation(sNameHash, sPermutationValue);
      }
    }
  }

  return XII_SUCCESS;
}

void xiiShaderCompilerApplication::PrintConfig()
{
  XII_LOG_BLOCK("ShaderCompiler Config");

  xiiLog::Info("Project: '{0}'", m_sAppProjectPath);
  xiiLog::Info("Shader: '{0}'", m_sShaderFiles);
  xiiLog::Info("Platform: '{0}'", m_sPlatforms);
}

xiiApplication::Execution xiiShaderCompilerApplication::Run()
{
  PrintConfig();

  XII_LOG_BLOCK("Compile All Shaders");

  xiiDynamicArray<xiiString> shadersToCompile;

  xiiStringBuilder files = m_sShaderFiles;

  xiiDynamicArray<xiiStringView> allFiles;
  if (m_sShaderFiles.IsEmpty())
  {
    xiiStringBuilder sPath, sPath2;
    for (xiiUInt32 dirIdx = 0; dirIdx < xiiFileSystem::GetNumDataDirectories(); ++dirIdx)
    {
      sPath = xiiFileSystem::GetDataDirectory(dirIdx)->GetDataDirectoryPath();

      if (sPath.IsEmpty())
        continue;

      if (xiiFileSystem::ResolveSpecialDirectory(sPath, sPath2).Failed())
        continue;

      files.AppendWithSeparator(";", sPath2);
    }
  }

  files.Split(false, allFiles, ";");

  xiiUInt32 uiErrors = 0;
  for (const xiiStringView& sEntry : allFiles)
  {
    xiiStringBuilder sFileOrFolder;
    // Relative paths are always relative to the project.
    if (xiiPathUtils::IsRelativePath(sEntry))
    {
      sFileOrFolder = m_sAppProjectPath;
      sFileOrFolder.AppendPath(sEntry);
    }
    else
    {
      sFileOrFolder = sEntry;
    }

    xiiFileStats stats;
    if (xiiOSFile::GetFileStats(sFileOrFolder, stats).Failed())
    {
      xiiLog::Error("Couldn't find path '{0}'.", sFileOrFolder);
      ++uiErrors;
      continue;
    }

    xiiStringBuilder relPath, absPath;
    if (stats.m_bIsDirectory)
    {
      xiiFileSystemIterator fsIt;
      xiiStringBuilder      fullPath;
      for (fsIt.StartSearch(sFileOrFolder, xiiFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
      {
        if (xiiPathUtils::HasExtension(fsIt.GetStats().m_sName, "xiiShader"))
        {
          fsIt.GetStats().GetFullPath(fullPath);
          if (xiiFileSystem::ResolvePath(fullPath, &absPath, &relPath).Succeeded())
          {
            shadersToCompile.PushBack(relPath);
          }
          else
          {
            xiiLog::Error("Couldn't resolve path '{0}'.", fullPath);
            ++uiErrors;
          }
        }
      }
    }
    else if (xiiFileSystem::ResolvePath(sFileOrFolder, &absPath, &relPath).Succeeded())
    {
      if (absPath.HasExtension("xiiShader"))
      {
        shadersToCompile.PushBack(relPath);
      }
      else
      {
        xiiLog::Error("File '{0}' is not a shader.", absPath);
        ++uiErrors;
      }
    }
    else
    {
      xiiLog::Error("Couldn't resolve path '{0}'.", sFileOrFolder);
    }
  }

  for (const xiiString& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      ++uiErrors;
      if (!opt_IgnoreErrors.GetOptionValue(xiiCommandLineOption::LogMode::Never))
      {
        SetReturnCode(uiErrors);

        return xiiApplication::Execution::Quit;
      }
    }
  }

  SetReturnCode(uiErrors);

  return xiiApplication::Execution::Quit;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderCompilerApplication);
