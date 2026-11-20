/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginMCP/EditorPluginMCPPCH.h>

#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorPluginMCP/EditorMCP.h>
#include <EditorPluginMCP/EditorMCPArguments.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/System/Process.h>

namespace
{
  constexpr qsizetype MaximumRetainedOutputBytes = 256 * 1024;
  // Leave room in the tool response budget for JSON escaping and result metadata.
  constexpr xiiUInt64 MaximumOutputPageBytes = 12 * 1024;

  QString BuildOperation()
  {
    return QStringLiteral("Building C++ project");
  }
} // namespace

void xiiEditorMCP::ConnectCppBuildProcess()
{
  m_CppBuildProcess.setProcessChannelMode(QProcess::MergedChannels);
  QObject::connect(&m_CppBuildProcess, &QProcess::readyReadStandardOutput, &m_CppBuildProcess, [this] { ReadCppBuildOutput(); });
  QObject::connect(&m_CppBuildProcess, &QProcess::started, &m_CppBuildProcess, [this] { m_CppBuild.m_State = CppBuild::State::Building; });
  QObject::connect(&m_CppBuildProcess, &QProcess::errorOccurred, &m_CppBuildProcess, [this](QProcess::ProcessError error) {
    if (error == QProcess::FailedToStart)
    {
      FinishCppBuild(xiiStatus(xiiFmt("Could not start CMake: {}", qtToXIIString(m_CppBuildProcess.errorString()))));
    }
  });
  QObject::connect(&m_CppBuildProcess, &QProcess::finished, &m_CppBuildProcess, [this](int iExitCode, QProcess::ExitStatus exitStatus) {
    if (exitStatus == QProcess::CrashExit)
    {
      FinishCppBuild(xiiStatus("The CMake build process stopped abnormally."));
      return;
    }

    m_CppBuild.m_iExitCode    = iExitCode;
    m_CppBuild.m_bHasExitCode = true;
    FinishCppBuild(xiiCppProject::CheckCompileResult(m_CppBuild.m_sPluginName, m_CppBuild.m_sOutputDirectory, iExitCode));
  });
}

void xiiEditorMCP::ReadCppBuildOutput()
{
  const QByteArray bytes = m_CppBuildProcess.readAllStandardOutput();
  m_CppBuild.m_uiOutputBytes += static_cast<xiiUInt64>(bytes.size());
  if (bytes.size() >= MaximumRetainedOutputBytes)
  {
    m_CppBuild.m_Output = bytes.last(MaximumRetainedOutputBytes);
    return;
  }

  const auto iDiscard = m_CppBuild.m_Output.size() + bytes.size() - MaximumRetainedOutputBytes;
  if (iDiscard > 0)
  {
    m_CppBuild.m_Output.remove(0, iDiscard);
  }
  m_CppBuild.m_Output.append(bytes);
}

void xiiEditorMCP::FinishCppBuild(const xiiStatus& status)
{
  if (!m_CppBuild.IsActive())
  {
    return;
  }

  ReadCppBuildOutput();
  m_CppBuild.m_Finished = xiiTime::Now();
  m_CppBuild.m_State    = status.Succeeded() ? CppBuild::State::Succeeded : CppBuild::State::Failed;
  m_CppBuild.m_sMessage = status.GetMessageString();
  xiiCppProject::EndCompile();
  if (status.Failed())
  {
    xiiLog::Error("{}", status.GetMessageString());
  }
  if (auto* pEditor = xiiQtEditorApp::GetSingleton())
  {
    Q_EMIT pEditor->ToolExecutionFinished(status.Succeeded(), BuildOperation());
  }
}

void xiiEditorMCP::DrainCppBuild()
{
  if (m_CppBuildProcess.state() != QProcess::NotRunning)
  {
    // Project close requests veto normal closure while building. During teardown, let
    // CMake finish its child jobs and deliver the terminal signal before unloading.
    m_CppBuildProcess.waitForFinished(-1);
  }
}

void xiiEditorMCP::OnProjectRequest(xiiToolsProjectRequest& ref_request)
{
  if (ref_request.m_Type == xiiToolsProjectRequest::Type::CanCloseProject && m_CppBuild.IsActive())
  {
    ref_request.m_bCanClose = false;
    xiiLog::Warning("Wait for the running C++ build before closing the project.");
  }
}

bool xiiEditorMCP::IsCppBuildAuthorized() const
{
  return m_bAllowCppBuilds && xiiToolsProject::IsProjectOpen() &&
    xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID == m_AllowedProject;
}

void xiiEditorMCP::DescribeCppBuild(xiiUInt64 uiOutputOffset, xiiVariantDictionary& out_result) const
{
  const auto&          build            = m_CppBuild;
  const char*          states[]         = {"idle", "starting", "building", "succeeded", "failed"};
  const auto           elapsed          = (build.IsActive() ? xiiTime::Now() : build.m_Finished) - build.m_Started;
  const xiiUInt64      uiRetainedOffset = build.m_uiOutputBytes - static_cast<xiiUInt64>(build.m_Output.size());
  const xiiUInt64      uiStart          = xiiMath::Clamp(uiOutputOffset, uiRetainedOffset, build.m_uiOutputBytes);
  const xiiUInt64      uiCount          = xiiMath::Min(MaximumOutputPageBytes, build.m_uiOutputBytes - uiStart);
  const QByteArrayView output           = QByteArrayView(build.m_Output).sliced(static_cast<qsizetype>(uiStart - uiRetainedOffset), static_cast<qsizetype>(uiCount));
  out_result["build"]                   = xiiEditorMCPArguments::FormatGuid(build.m_Id);
  out_result["generation"]              = build.m_uiProjectGeneration;
  out_result["plugin"]                  = build.m_sPluginName;
  out_result["state"]                   = xiiString(states[static_cast<xiiUInt8>(build.m_State)]);
  out_result["elapsedSeconds"]          = elapsed.GetSeconds();
  out_result["output"]                  = qtToXIIString(QString::fromUtf8(output));
  out_result["outputBytes"]             = build.m_uiOutputBytes;
  out_result["retainedOutputOffset"]    = uiRetainedOffset;
  out_result["outputOffset"]            = uiStart;
  out_result["nextOutputOffset"]        = uiStart + uiCount;
  if (!build.m_sMessage.IsEmpty())
  {
    out_result["message"] = build.m_sMessage;
  }
  if (build.m_bHasExitCode)
  {
    out_result["exitCode"] = build.m_iExitCode;
  }
  if (build.m_State == CppBuild::State::Succeeded)
  {
    out_result["outputDirectory"] = build.m_sOutputDirectory;
  }
}

xiiStatus xiiEditorMCP::StartCppBuild(const xiiMCPCallContext& context, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  xiiUInt64 uiGeneration = 0;
  if (!IsCppBuildAuthorized() || !xiiEditorMCPArguments::ReadUnsigned(arguments, "generation", uiGeneration) ||
      !xiiToolsProject::IsProjectGenerationCurrent(uiGeneration))
  {
    return xiiStatus("C++ builds require build permission for the current project generation.");
  }
  if (m_CppBuild.IsActive())
  {
    return xiiStatus("A C++ project build is already running.");
  }

  xiiCppSettings settings;
  if (settings.Load().Failed() || !xiiCppSettings::IsValidPluginName(settings.m_sPluginName) || !xiiCppProject::ExistsProjectCMakeListsTxt())
  {
    return xiiStatus("The current project has no configured C++ plugin source.");
  }
  if (!xiiCppProject::ExistsSolution(settings) || xiiCppProject::CheckCMakeCache(settings).Failed())
  {
    return xiiStatus("Generate the C++ project for the current SDK and Editor configuration before building.");
  }
  if (context.HasExpired())
  {
    return xiiStatus("The request expired before starting the C++ build.");
  }

  xiiProcessOptions process;
  XII_SUCCEED_OR_RETURN(xiiCppProject::BeginCompile(settings, process));

  m_CppBuild                       = {};
  m_CppBuild.m_Id                  = xiiUuid::MakeUuid();
  m_CppBuild.m_uiProjectGeneration = uiGeneration;
  m_CppBuild.m_sPluginName         = settings.m_sPluginName;
  m_CppBuild.m_sOutputDirectory    = xiiCppProject::GetPluginOutputDir();
  m_CppBuild.m_State               = CppBuild::State::Starting;
  m_CppBuild.m_Started             = xiiTime::Now();

  QStringList args;
  for (const auto& sArgument : process.m_Arguments)
  {
    args.push_back(xiiMakeQString(sArgument));
  }
  m_CppBuildProcess.setWorkingDirectory(xiiMakeQString(process.m_sWorkingDirectory));
  Q_EMIT xiiQtEditorApp::GetSingleton()->ToolExecutionStarted(BuildOperation());
  m_CppBuildProcess.start(xiiMakeQString(process.m_sProcess), args);
  DescribeCppBuild(0, out_result);
  return XII_SUCCESS;
}

xiiStatus xiiEditorMCP::GetCppBuild(const xiiMCPCallContext&, const xiiVariantDictionary& arguments, xiiVariantDictionary& out_result)
{
  if (!IsCppBuildAuthorized() || !xiiToolsProject::IsProjectGenerationCurrent(m_CppBuild.m_uiProjectGeneration) ||
      !m_CppBuild.m_Id.IsValid() || (arguments.Contains("build") && m_CppBuild.m_Id != xiiEditorMCPArguments::ReadGuid(arguments, "build")))
  {
    return xiiStatus("The C++ build is unavailable in the authorized project. Use the ID of its latest build.");
  }
  const auto uiAvailable    = m_CppBuild.m_uiOutputBytes;
  xiiUInt64  uiOutputOffset = uiAvailable - xiiMath::Min(uiAvailable, MaximumOutputPageBytes);
  if (arguments.Contains("outputOffset") && (!xiiEditorMCPArguments::ReadUnsigned(arguments, "outputOffset", uiOutputOffset) || uiOutputOffset > uiAvailable))
  {
    return xiiStatus("The output offset must be between zero and the total bytes produced.");
  }
  DescribeCppBuild(uiOutputOffset, out_result);
  return XII_SUCCESS;
}

xiiResult xiiEditorMCP::RegisterCppBuild()
{
  const auto execute = xiiMCPPermission::Execute;
  XII_SUCCEED_OR_RETURN(m_Registry.RegisterTool("editor.build_cpp_project", "Start a background CMake build of the current project's configured C++ plugin. Requires project C++ build permission.",
                                                R"({"type":"object","required":["generation"],"properties":{"generation":{"type":"integer","minimum":0}},"additionalProperties":false})",
                                                execute, xiiMakeDelegate(&xiiEditorMCP::StartCppBuild, this)));
  return m_Registry.RegisterTool("editor.cpp_build", "Get the latest C++ build state, elapsed time, compiler output and terminal result. Supply the build ID to check for replacement, and outputOffset to page through retained diagnostics.",
                                 R"({"type":"object","properties":{"build":{"type":"string"},"outputOffset":{"type":"integer","minimum":0}},"additionalProperties":false})",
                                 execute, xiiMakeDelegate(&xiiEditorMCP::GetCppBuild, this));
}
