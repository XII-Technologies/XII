#include <EditorProcessor/EditorProcessorPCH.h>

#include <Foundation/Basics/Platform/Windows/IncludeWindows.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessorMessages.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/CodeGen/CppSettings.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/Application/Application.h>
#include <Foundation/System/CrashHandler.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <ToolsFoundation/Application/ApplicationServices.h>

// clang-format off
xiiCommandLineOptionPath   opt_OutputDir("_EditorProcessor", "-outputDir", "Output directory", "");
xiiCommandLineOptionBool   opt_SaveProfilingData("_EditorProcessor", "-profiling", "Saves performance profiling information into the output folder.", false);
xiiCommandLineOptionPath   opt_Project("_EditorProcessor", "-project", "Path to the project folder.", "");
xiiCommandLineOptionBool   opt_Resave("_EditorProcessor", "-resave", "If specified, assets will be resaved.", false);
xiiCommandLineOptionString opt_Transform("_EditorProcessor", "-transform", "If specified, assets will be transformed for the given platform profile.\n\
\n\
Example:\n\
  -transform Default\n\
",
"");
// clang-format on

class xiiEditorApplication : public xiiApplication
{
public:
  using SUPER = xiiApplication;

  xiiEditorApplication() :
    xiiApplication("xiiEditor")
  {
    EnableMemoryLeakReporting(true);
    m_pEditorEngineProcessAppDummy = XII_DEFAULT_NEW(xiiEditorEngineProcessApp);

    m_pEditorApp = new xiiQtEditorApp;
  }

  virtual xiiResult BeforeCoreSystemsStartup() override
  {
    xiiStartup::AddApplicationTag("tool");
    xiiStartup::AddApplicationTag("editor");
    xiiStartup::AddApplicationTag("editorprocessor");

    xiiQtEditorApp::GetSingleton()->InitQt(GetArgumentCount(), (char**)GetArgumentsArray());

    xiiString sUserDataFolder = xiiApplicationServices::GetSingleton()->GetApplicationUserDataFolder();
    xiiString sOutputFolder   = opt_OutputDir.GetOptionValue(xiiCommandLineOption::LogMode::Never);
    xiiCrashHandler_WriteMiniDump::g_Instance.SetDumpFilePath(sOutputFolder.IsEmpty() ? sUserDataFolder : sOutputFolder, "EditorProcessor");
    xiiCrashHandler::SetCrashHandler(&xiiCrashHandler_WriteMiniDump::g_Instance);

    return XII_SUCCESS;
  }

  virtual void AfterCoreSystemsShutdown() override
  {
    m_pEditorEngineProcessAppDummy = nullptr;

    xiiQtEditorApp::GetSingleton()->DeInitQt();

    delete m_pEditorApp;
    m_pEditorApp = nullptr;
  }

  void EventHandlerIPC(const xiiProcessCommunicationChannel::Event& e)
  {
    if (const xiiProcessAssetMsg* pMsg = xiiDynamicCast<const xiiProcessAssetMsg*>(e.m_pMessage))
    {
      if (pMsg->m_sAssetPath.HasExtension("xiiPrefab") || pMsg->m_sAssetPath.HasExtension("xiiScene"))
      {
        xiiQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
      }

      xiiProcessAssetResponseMsg msg;
      {
        xiiLogEntryDelegate logger([&msg](xiiLogEntry& ref_entry) -> void { msg.m_LogEntries.PushBack(std::move(ref_entry)); }, xiiLogMsgType::WarningMsg);
        xiiLogSystemScope   logScope(&logger);

        const xiiUInt32 uiPlatform = xiiAssetCurator::GetSingleton()->FindAssetProfileByName(pMsg->m_sPlatform);

        if (uiPlatform == xiiInvalidIndex)
        {
          xiiLog::Error("Asset platform config '{0}' is unknown", pMsg->m_sPlatform);
        }
        else
        {
          xiiUInt64 uiAssetHash = 0;
          xiiUInt64 uiThumbHash = 0;

          // TODO: there is currently no 'nice' way to switch the active platform for the asset processors it is also not clear whether this is actually safe to execute here
          xiiAssetCurator::GetSingleton()->SetActiveAssetProfileByIndex(uiPlatform);
          // First, force checking for file system changes for the asset and the transitive hull of all dependencies and runtime references. This needs to be done as this EditorProcessor instance might not know all the files yet as some might just have been written. We can't rely on the filesystem watcher as it is not instant and also might just miss some events.
          for (const xiiString& sDepOrRef : pMsg->m_DepRefHull)
          {
            if (sDepOrRef.IsAbsolutePath())
            {
              xiiAssetCurator::GetSingleton()->NotifyOfFileChange(sDepOrRef);
            }
            else
            {
              xiiStringBuilder sTemp = sDepOrRef;
              if (xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sTemp))
              {
                xiiAssetCurator::GetSingleton()->NotifyOfFileChange(sTemp);
              }
            }
          }
          xiiAssetCurator::GetSingleton()->NotifyOfFileChange(pMsg->m_sAssetPath);

          // Next, we force checking that the asset is up to date. This EditorProcessor instance might not have observed the generation of the output files of various dependencies yet and incorrectly assume that some dependencies still need to be transformed. To prevent this, we force checking the asset and all its dependencies via the filesystem, ignoring the caching.
          xiiAssetInfo::TransformState state = xiiAssetCurator::GetSingleton()->IsAssetUpToDate(pMsg->m_AssetGuid, xiiAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform), nullptr, uiAssetHash, uiThumbHash, true);

          if (uiAssetHash != pMsg->m_AssetHash || uiThumbHash != pMsg->m_ThumbHash)
          {
            xiiLog::Warning("Asset '{}' of state '{}' in processor with hashes '{}{}' differs from the state in the editor with hashes '{}{}'", pMsg->m_sAssetPath, (int)state, uiAssetHash, uiThumbHash, pMsg->m_AssetHash, pMsg->m_ThumbHash);
          }

          if (state == xiiAssetInfo::NeedsThumbnail || state == xiiAssetInfo::NeedsTransform)
          {
            msg.m_Status = xiiAssetCurator::GetSingleton()->TransformAsset(pMsg->m_AssetGuid, xiiTransformFlags::BackgroundProcessing, xiiAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));

            if (msg.m_Status.Failed())
            {
              // make sure the result message ends up in the log
              xiiLog::Error("{}", msg.m_Status.m_sMessage);
            }
          }
          else if (state == xiiAssetInfo::UpToDate)
          {
            msg.m_Status = xiiTransformStatus();
            xiiLog::Warning("Asset already up to date: '{}'", pMsg->m_sAssetPath);
          }
          else
          {
            msg.m_Status = xiiTransformStatus(xiiFmt("Asset {} is in state {}, can't process asset.", pMsg->m_sAssetPath, (int)state)); // TODO nicer state to string
            xiiLog::Error("{}", msg.m_Status.m_sMessage);
          }
        }
      }
      m_IPC.SendMessage(&msg);
    }
  }

  virtual Execution Run() override
  {
    {
      xiiStringBuilder cmdHelp;
      if (xiiCommandLineOption::LogAvailableOptionsToBuffer(cmdHelp, xiiCommandLineOption::LogAvailableModes::IfHelpRequested, "_EditorProcessor;cvar"))
      {
        xiiQtUiServices::GetSingleton()->MessageBoxInformation(cmdHelp);
        return xiiApplication::Execution::Quit;
      }
    }

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
    // this also speeds up process termination significantly (down to less than a second)
    DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
    SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
    const xiiString                                 sTransformProfile = opt_Transform.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool                                      bResave           = opt_Resave.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified);
    const bool                                      bBackgroundMode   = sTransformProfile.IsEmpty() && !bResave;
    const xiiString                                 sOutputDir        = opt_OutputDir.GetOptionValue(xiiCommandLineOption::LogMode::Always);
    const xiiBitflags<xiiQtEditorApp::StartupFlags> startupFlags      = bBackgroundMode ? xiiQtEditorApp::StartupFlags::Headless | xiiQtEditorApp::StartupFlags::Background : xiiQtEditorApp::StartupFlags::Headless;
    xiiQtEditorApp::GetSingleton()->StartupEditor(startupFlags, sOutputDir);
    xiiQtUiServices::SetHeadless(true);

    const xiiStringBuilder sProject = opt_Project.GetOptionValue(xiiCommandLineOption::LogMode::Always);

    if (!sTransformProfile.IsEmpty())
    {
      if (xiiQtEditorApp::GetSingleton()->OpenProject(sProject).Failed())
      {
        SetReturnCode(2);
        return xiiApplication::Execution::Quit;
      }

      // before we transform any assets, make sure the C++ code is properly built
      {
        xiiCppSettings cppSettings;
        if (cppSettings.Load().Succeeded())
        {
          if (xiiCppProject::BuildCodeIfNecessary(cppSettings).Failed())
          {
            SetReturnCode(3);
            return xiiApplication::Execution::Quit;
          }

          xiiQtEditorApp::GetSingleton()->RestartEngineProcessIfPluginsChanged(true);
        }
      }

      bool bTransform = true;

      xiiQtEditorApp::GetSingleton()->connect(xiiQtEditorApp::GetSingleton(), &xiiQtEditorApp::IdleEvent, xiiQtEditorApp::GetSingleton(), [this, &bTransform, &sTransformProfile]() {
        if (!bTransform)
          return;

        bTransform = false;

        const xiiUInt32 uiPlatform = xiiAssetCurator::GetSingleton()->FindAssetProfileByName(sTransformProfile);

        if (uiPlatform == xiiInvalidIndex)
        {
          xiiLog::Error("Asset platform config '{0}' is unknown", sTransformProfile);
        }
        else
        {
          xiiStatus status = xiiAssetCurator::GetSingleton()->TransformAllAssets(xiiTransformFlags::TriggeredManually, xiiAssetCurator::GetSingleton()->GetAssetProfile(uiPlatform));
          if (status.Failed())
          {
            status.LogFailure();
            SetReturnCode(1);
          }

          if (opt_SaveProfilingData.GetOptionValue(xiiCommandLineOption::LogMode::Always))
          {
            xiiActionContext context;
            xiiActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
          }
        }

        QApplication::quit(); });

      const xiiInt32 iReturnCode = xiiQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else if (opt_Resave.GetOptionValue(xiiCommandLineOption::LogMode::AlwaysIfSpecified))
    {
      xiiQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();

      xiiQtEditorApp::GetSingleton()->connect(xiiQtEditorApp::GetSingleton(), &xiiQtEditorApp::IdleEvent, xiiQtEditorApp::GetSingleton(), [this]() {
        xiiAssetCurator::GetSingleton()->ResaveAllAssets();

        if (opt_SaveProfilingData.GetOptionValue(xiiCommandLineOption::LogMode::Always))
        {
          xiiActionContext context;
          xiiActionManager::ExecuteAction("Engine", "Editor.SaveProfiling", context).IgnoreResult();
        }

        QApplication::quit();
      });

      const xiiInt32 iReturnCode = xiiQtEditorApp::GetSingleton()->RunEditor();
      if (iReturnCode != 0)
        SetReturnCode(iReturnCode);
    }
    else
    {
      xiiResult res = m_IPC.ConnectToHostProcess();
      if (res.Succeeded())
      {
        m_IPC.m_Events.AddEventHandler(xiiMakeDelegate(&xiiEditorApplication::EventHandlerIPC, this));

        xiiQtEditorApp::GetSingleton()->OpenProject(sProject).IgnoreResult();
        xiiQtEditorApp::GetSingleton()->connect(xiiQtEditorApp::GetSingleton(), &xiiQtEditorApp::IdleEvent, xiiQtEditorApp::GetSingleton(), [this]() {
          static bool bRecursionBlock = false;
          if (bRecursionBlock)
            return;
          bRecursionBlock = true;

          if (!m_IPC.IsHostAlive())
            QApplication::quit();

          m_IPC.WaitForMessages();

          bRecursionBlock = false;
        });

        const xiiInt32 iReturnCode = xiiQtEditorApp::GetSingleton()->RunEditor();
        SetReturnCode(iReturnCode);
      }
      else
      {
        xiiLog::Error("Failed to connect with host process");
      }
    }

    xiiQtEditorApp::GetSingleton()->ShutdownEditor();

    return xiiApplication::Execution::Quit;
  }

private:
  xiiQtEditorApp*                         m_pEditorApp = nullptr;
  xiiEngineProcessCommunicationChannel    m_IPC;
  xiiUniquePtr<xiiEditorEngineProcessApp> m_pEditorEngineProcessAppDummy;
};

XII_APPLICATION_ENTRY_POINT(xiiEditorApplication);
