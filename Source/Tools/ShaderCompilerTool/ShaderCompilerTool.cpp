#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GraphicsCore/ShaderCompiler/ShaderCompiler.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>
#include <ShaderCompilerTool/ShaderCompilerTool.h>

xiiCommandLineOptionString opt_Shader("_ShaderCompiler", "-shader", "\
One or multiple paths to shader files or folders containing shaders.\n\
Paths are separated with semicolons.\n\
Paths may be absolute or relative to the -project directory.\n\
If a path to a folder is specified, all .xiiShader files in that folder are compiled.\n\
\n\
This option has to be specified.",
                                      "");

xiiCommandLineOptionPath opt_Project("_ShaderCompiler", "-project", "\
Path to the folder of the project, for which shaders should be compiled.",
                                     "");

xiiCommandLineOptionString opt_Platform("_ShaderCompiler", "-platform", "The name of the platform for which to compile the shaders.\n\
Examples:\n\
  -platform D3D_SM60\n\
  -platform VK_SM60\n\
  -platform ALL",
                                        "D3D_SM60");

xiiCommandLineOptionBool opt_IgnoreErrors("_ShaderCompiler", "-IgnoreErrors", "If set, a compile error won't stop other shaders from being compiled.", false);

xiiCommandLineOptionDoc opt_Perm("_ShaderCompiler", "-perm", "<string list>", "List of permutation variables to set to fixed values.\n\
Spaces are used to separate multiple arguments, therefore each argument mustn't use spaces.\n\
In the form of 'SOME_VAR=VALUE'\n\
Examples:\n\
  -perm BLEND_MODE=BLEND_MODE_OPAQUE\n\
  -perm TWO_SIDED=FALSE MSAA=TRUE\n\
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
  ExecuteInitFunctions();

  xiiStartup::StartupHighLevelSystems();
}

xiiResult xiiShaderCompilerApplication::CompileShader(xiiStringView sShaderFile)
{
  XII_LOG_BLOCK("Compiling Shader", sShaderFile);

  if (ExtractPermutationVarValues(sShaderFile).Failed())
    return XII_FAILURE;

  xiiHybridArray<xiiPermutationVar, 16> permVars;

  const xiiUInt32 uiMaxPerms = m_PermutationGenerator.GetPermutationCount();

  xiiLog::Info("Shader has {0} permutations", uiMaxPerms);

  for (xiiUInt32 perm = 0; perm < uiMaxPerms; ++perm)
  {
    XII_LOG_BLOCK("Compiling Permutation");

    m_PermutationGenerator.GetPermutation(perm, permVars);
    xiiShaderCompiler sc;
    if (sc.CompileShaderPermutationForPlatforms(sShaderFile, permVars, xiiLog::GetThreadLocalLogSystem(), m_sPlatforms).Failed())
      return XII_FAILURE;
  }

  xiiLog::Success("Compiled Shader '{0}'", sShaderFile);
  return XII_SUCCESS;
}

xiiResult xiiShaderCompilerApplication::ExtractPermutationVarValues(xiiStringView sShaderFile)
{
  m_PermutationGenerator.Clear();

  xiiFileReader shaderFile;
  if (shaderFile.Open(sShaderFile).Failed())
  {
    xiiLog::Error("Could not open file '{0}'", sShaderFile);
    return XII_FAILURE;
  }

  xiiHybridArray<xiiHashedString, 16>   permVars;
  xiiHybridArray<xiiPermutationVar, 16> fixedPermVars;
  xiiShaderParser::ParsePermutationSection(shaderFile, permVars, fixedPermVars);

  {
    XII_LOG_BLOCK("Permutation Vars");
    for (const auto& s : permVars)
    {
      xiiLog::Dev(s.GetData());
    }
  }

  // regular permutation variables
  {
    for (const auto& s : permVars)
    {
      xiiHybridArray<xiiHashedString, 16> values;
      xiiShaderManager::GetPermutationValues(s, values);

      for (const auto& val : values)
      {
        m_PermutationGenerator.AddPermutation(s, val);
      }
    }
  }

  // permutation variables that have fixed values
  {
    for (const auto& s : fixedPermVars)
    {
      m_PermutationGenerator.AddPermutation(s.m_sName, s.m_sValue);
    }
  }

  {
    for (auto it = m_FixedPermVars.GetIterator(); it.IsValid(); ++it)
    {
      xiiHashedString hsname, hsvalue;
      hsname.Assign(it.Key().GetData());
      m_PermutationGenerator.RemovePermutations(hsname);

      for (const auto& val : it.Value())
      {
        hsvalue.Assign(val.GetData());

        m_PermutationGenerator.AddPermutation(hsname, hsvalue);
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

  xiiStringBuilder files = m_sShaderFiles;

  xiiDynamicArray<xiiString> shadersToCompile;

  xiiDynamicArray<xiiStringView> allFiles;
  files.Split(false, allFiles, ";");

  for (const xiiStringView& shader : allFiles)
  {
    xiiStringBuilder file = shader;
    xiiStringBuilder relPath;

    if (xiiFileSystem::ResolvePath(file, nullptr, &relPath).Succeeded())
    {
      shadersToCompile.PushBack(relPath);
    }
    else
    {
      if (xiiPathUtils::IsRelativePath(file))
      {
        file.Prepend(m_sAppProjectPath, "/");
      }

      file.TrimWordEnd("*");
      file.MakeCleanPath();

      if (xiiOSFile::ExistsDirectory(file))
      {
        xiiFileSystemIterator fsIt;
        for (fsIt.StartSearch(file, xiiFileSystemIteratorFlags::ReportFilesRecursive); fsIt.IsValid(); fsIt.Next())
        {
          if (xiiPathUtils::HasExtension(fsIt.GetStats().m_sName, "xiiShader"))
          {
            fsIt.GetStats().GetFullPath(relPath);

            if (relPath.MakeRelativeTo(m_sAppProjectPath).Succeeded())
            {
              shadersToCompile.PushBack(relPath);
            }
          }
        }
      }
      else
      {
        xiiLog::Error("Could not resolve path to shader '{0}'", file);
      }
    }
  }

  for (const auto& shader : shadersToCompile)
  {
    if (CompileShader(shader).Failed())
    {
      if (!m_bIgnoreErrors)
      {
        return xiiApplication::Execution::Quit;
      }
    }
  }

  return xiiApplication::Execution::Quit;
}

XII_CONSOLEAPP_ENTRY_POINT(xiiShaderCompilerApplication);
