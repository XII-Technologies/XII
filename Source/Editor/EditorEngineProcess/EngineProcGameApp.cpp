#include <EditorEngineProcess/EditorEngineProcessPCH.h>

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/System/SystemInformation.h>

#include <Core/Console/QuakeConsole.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessApp.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/Gizmos/GizmoRenderer.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

xiiEngineProcessGameApplication::xiiEngineProcessGameApplication() :
  xiiGameApplication("xiiEditorEngineProcess", nullptr)
{
  m_LongOpWorkerManager.Startup(&m_IPC);
}

xiiEngineProcessGameApplication::~xiiEngineProcessGameApplication() = default;

xiiResult xiiEngineProcessGameApplication::BeforeCoreSystemsStartup()
{
  m_pApp = CreateEngineProcessApp();
  xiiStartup::AddApplicationTag("editorengineprocess");

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP) || XII_ENABLED(XII_PLATFORM_LINUX)
  // Make sure to disable the fileserve plugin
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_off");
#endif

  return SUPER::BeforeCoreSystemsStartup();
}

void xiiEngineProcessGameApplication::AfterCoreSystemsStartup()
{
  // skip project creation at this point
  // SUPER::AfterCoreSystemsStartup();

#if XII_DISABLED(XII_PLATFORM_WINDOWS_DESKTOP) && XII_DISABLED(XII_PLATFORM_LINUX)
  {
    // on all 'mobile' platforms, we assume we are in remote mode
    xiiEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#else
  if (xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-remote", false))
  {
    xiiEditorEngineProcessApp::GetSingleton()->SetRemoteMode();
  }
#endif

  WaitForDebugger();

  DisableErrorReport();

  xiiTaskSystem::SetTargetFrameTime(xiiTime::Seconds(1.0 / 20.0));

  ConnectToHost();
}


void xiiEngineProcessGameApplication::ConnectToHost()
{
  XII_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerIPC, this));

  // wait indefinitely (not necessary anymore, should work regardless)
  // m_IPC.WaitForMessage(xiiGetStaticRTTI<xiiSetupProjectMsgToEngine>(), xiiTime());
}

void xiiEngineProcessGameApplication::DisableErrorReport()
{
#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
}

void xiiEngineProcessGameApplication::WaitForDebugger()
{
  if (xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption("-WaitForDebugger"))
  {
    while (!xiiSystemInformation::IsDebuggerAttached())
    {
      xiiThreadUtils::Sleep(xiiTime::Milliseconds(10));
    }
  }
}

void xiiEngineProcessGameApplication::BeforeCoreSystemsShutdown()
{
  m_pApp = nullptr;

  m_LongOpWorkerManager.Shutdown();

  m_IPC.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerIPC, this));

  SUPER::BeforeCoreSystemsShutdown();
}

xiiApplication::Execution xiiEngineProcessGameApplication::Run()
{
  xiiRenderWorld::ClearMainViews();
  bool bPendingOpInProgress = false;
  do
  {
    bPendingOpInProgress = xiiEngineProcessDocumentContext::PendingOperationsInProgress();
    if (ProcessIPCMessages(bPendingOpInProgress))
    {
      xiiEngineProcessDocumentContext::UpdateDocumentContexts();
    }
  } while (!bPendingOpInProgress && m_uiRedrawCountExecuted == m_uiRedrawCountReceived);

  m_uiRedrawCountExecuted = m_uiRedrawCountReceived;
  return SUPER::Run();
}

void xiiEngineProcessGameApplication::LogWriter(const xiiLoggingEventData& e)
{
  xiiLogMsgToEditor msg;
  msg.m_Entry = xiiLogEntry(e);

  // the editor does not care about flushing caches, so no need to send this over
  if (msg.m_Entry.m_Type == xiiLogMsgType::Flush)
    return;

  if (msg.m_Entry.m_sTag == "IPC")
    return;

  // Prevent infinite recursion by disabeling logging until we are done sending the message
  XII_LOG_BLOCK_MUTE();

  m_IPC.SendMessage(&msg);
}

static bool EmptyAssertHandler(const char* szSourceFile, xiiUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg)
{
  return false;
}

bool xiiEngineProcessGameApplication::ProcessIPCMessages(bool bPendingOpInProgress)
{
  XII_PROFILE_SCOPE("ProcessIPCMessages");

  if (!m_IPC.IsHostAlive()) // check whether the host crashed
  {
    // The problem here is, that the editor process crashed (or was terminated through Visual Studio),
    // but our process depends on it for cleanup!
    // That means, this process created rendering resources through a device that is bound to a window handle, which belonged to the editor
    // process. So now we can't clean up, and therefore we can only crash. Therefore we try to crash as silently as possible.

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    // Make sure that Windows doesn't show a default message box when we call abort
    _set_abort_behavior(0, _WRITE_ABORT_MSG);
    TerminateProcess(GetCurrentProcess(), 0);
#endif

    // The OS will still call destructors for our objects (even though we called abort ... what a pointless design).
    // Our code might assert on destruction, so make sure our assert handler doesn't show anything.
    xiiSetAssertHandler(EmptyAssertHandler);
    std::abort();
  }
  else
  {
    // if an operation is still pending or this process is a remote process, we do NOT want to block
    // remote processes shall run as fast as they can
    if (bPendingOpInProgress || xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    {
      m_IPC.ProcessMessages();
    }
    else
    {
      XII_PROFILE_SCOPE("WaitForMessages");
      // Only suspend and wait if no more pending ops need to be done.
      m_IPC.WaitForMessages();
    }
    return true;
  }
}

void xiiEngineProcessGameApplication::SendProjectReadyMessage()
{
  xiiProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}

void xiiEngineProcessGameApplication::SendReflectionInformation()
{
  if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
    return;

  xiiSet<const xiiRTTI*> types;

  xiiReflectionUtils::GatherTypesDerivedFromClass(xiiGetStaticRTTI<xiiReflectedClass>(), types, true);

  xiiDynamicArray<const xiiRTTI*> sortedTypes;
  xiiReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    xiiUpdateReflectionTypeMsgToEditor TypeMsg;
    xiiToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void xiiEngineProcessGameApplication::EventHandlerIPC(const xiiEngineProcessCommunicationChannel::Event& e)
{
  if (const auto* pMsg = xiiDynamicCast<const xiiSyncWithProcessMsgToEngine*>(e.m_pMessage))
  {
    xiiSyncWithProcessMsgToEditor msg;
    msg.m_uiRedrawCount     = pMsg->m_uiRedrawCount;
    m_uiRedrawCountReceived = msg.m_uiRedrawCount;
    m_IPC.SendMessage(&msg);
    return;
  }

  if (const auto* pMsg = xiiDynamicCast<const xiiShutdownProcessMsgToEngine*>(e.m_pMessage))
  {
    // in non-remote mode, the process needs to be properly killed, to prevent error messages
    // this is taken care of by the editor process
    if (xiiEditorEngineProcessApp::GetSingleton()->IsRemoteMode())
      RequestQuit();

    return;
  }

  // Project Messages:
  if (const auto* pMsg = xiiDynamicCast<const xiiSetupProjectMsgToEngine*>(e.m_pMessage))
  {
    if (!m_sProjectDirectory.IsEmpty())
    {
      // ignore this message, if it is for the same project
      if (m_sProjectDirectory == pMsg->m_sProjectDir)
        return;

      xiiLog::Error("Engine Process must restart to switch to another project ('{0}' -> '{1}').", m_sProjectDirectory, pMsg->m_sProjectDir);
      return;
    }

    m_sProjectDirectory      = pMsg->m_sProjectDir;
    m_CustomFileSystemConfig = pMsg->m_FileSystemConfig;
    m_CustomPluginConfig     = pMsg->m_PluginConfig;

    if (!pMsg->m_sAssetProfile.IsEmpty())
    {
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-profile");
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sAssetProfile);
    }

    if (!pMsg->m_sFileserveAddress.IsEmpty())
    {
      // we have no link dependency on the fileserve plugin here, it might not be loaded (yet / at all)
      // but we can pass the address to the command line, then it will pick it up, if necessary
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-fs_server");
      xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument(pMsg->m_sFileserveAddress);
    }

    // now that we know which project to initialize, do the delayed project setup
    {
      ExecuteInitFunctions();

      xiiStartup::StartupHighLevelSystems();

      xiiRenderContext::GetDefaultInstance()->SetAllowAsyncShaderLoading(true);
    }

    // after the xiiSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection
    // information over
    SendReflectionInformation();

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (const auto* pMsg1 = xiiDynamicCast<const xiiReloadResourceMsgToEngine*>(e.m_pMessage))
  {
    XII_PROFILE_SCOPE("ReloadResource");

    const xiiRTTI* pType = xiiResourceManager::FindResourceForAssetType(pMsg1->m_sResourceType);
    if (auto hResource = xiiResourceManager::GetExistingResourceByType(pType, pMsg1->m_sResourceID); hResource.IsValid())
    {
      xiiResourceManager::ReloadResource(pType, hResource, false);
    }
  }
  else if (const auto* pMsg1 = xiiDynamicCast<const xiiSimpleConfigMsgToEngine*>(e.m_pMessage))
  {
    if (pMsg1->m_sWhatToDo == "ChangeActivePlatform")
    {
      xiiStringBuilder sRedirFile("AssetCache/", pMsg1->m_sPayload, ".xiiAidlt");

      xiiDataDirectory::FolderType::s_sRedirectionFile = sRedirFile;

      xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();

      m_PlatformProfile.m_sName = pMsg1->m_sPayload;
      Init_PlatformProfile_LoadForRuntime();

      xiiResourceManager::ReloadAllResources(false);
      xiiRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadAssetLUT")
    {
      xiiFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg1->m_sWhatToDo == "ReloadResources")
    {
      xiiResourceManager::ReloadAllResources(false);
      xiiRenderWorld::DeleteAllCachedRenderData();
    }
    else if (pMsg1->m_sWhatToDo == "SaveProfiling")
    {
      xiiProfilingSystem::ProfilingData profilingData;
      xiiProfilingSystem::Capture(profilingData);
      xiiFileWriter fileWriter;
      if (fileWriter.Open(pMsg1->m_sPayload) == XII_SUCCESS)
      {
        profilingData.Write(fileWriter).IgnoreResult();
        xiiLog::Info("Engine profiling capture saved to '{0}'.", fileWriter.GetFilePathAbsolute().GetData());
      }
      else
      {
        xiiLog::Error("Could not write profiling capture to '{0}'.", pMsg1->m_sPayload);
      }

      xiiSaveProfilingResponseToEditor response;
      xiiStringBuilder                 sAbsPath;
      if (xiiFileSystem::ResolvePath(pMsg1->m_sPayload, &sAbsPath, nullptr).Succeeded())
      {
        response.m_sProfilingFile = sAbsPath;
      }
      m_IPC.SendMessage(&response);
    }
    else
      xiiLog::Warning("Unknown xiiSimpleConfigMsgToEngine '{0}'", pMsg1->m_sWhatToDo);
  }
  else if (const auto* pMsg2 = xiiDynamicCast<const xiiResourceUpdateMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceUpdateMsg(*pMsg2);
  }
  else if (const auto* pMsg2a = xiiDynamicCast<const xiiRestoreResourceMsgToEngine*>(e.m_pMessage))
  {
    HandleResourceRestoreMsg(*pMsg2a);
  }
  else if (const auto* pMsg2b = xiiDynamicCast<const xiiGlobalSettingsMsgToEngine*>(e.m_pMessage))
  {
    xiiGizmoRenderer::s_fGizmoScale = pMsg2b->m_fGizmoScale;
  }
  else if (const auto* pMsg3 = xiiDynamicCast<const xiiChangeCVarMsgToEngine*>(e.m_pMessage))
  {
    if (xiiCVar* pCVar = xiiCVar::FindCVarByName(pMsg3->m_sCVarName))
    {
      if (pCVar->GetType() == xiiCVarType::Int && pMsg3->m_NewValue.CanConvertTo<xiiInt32>())
      {
        *static_cast<xiiCVarInt*>(pCVar) = pMsg3->m_NewValue.ConvertTo<xiiInt32>();
      }
      else if (pCVar->GetType() == xiiCVarType::Float && pMsg3->m_NewValue.CanConvertTo<float>())
      {
        *static_cast<xiiCVarFloat*>(pCVar) = pMsg3->m_NewValue.ConvertTo<float>();
      }
      else if (pCVar->GetType() == xiiCVarType::Double && pMsg3->m_NewValue.CanConvertTo<double>())
      {
        *static_cast<xiiCVarDouble*>(pCVar) = pMsg3->m_NewValue.ConvertTo<double>();
      }
      else if (pCVar->GetType() == xiiCVarType::Bool && pMsg3->m_NewValue.CanConvertTo<bool>())
      {
        *static_cast<xiiCVarBool*>(pCVar) = pMsg3->m_NewValue.ConvertTo<bool>();
      }
      else if (pCVar->GetType() == xiiCVarType::String && pMsg3->m_NewValue.CanConvertTo<xiiString>())
      {
        *static_cast<xiiCVarString*>(pCVar) = pMsg3->m_NewValue.ConvertTo<xiiString>();
      }
      else
      {
        xiiLog::Warning("xiiChangeCVarMsgToEngine: New value for CVar '{0}' is incompatible with CVar type", pMsg3->m_sCVarName);
      }
    }
    else
      xiiLog::Warning("xiiChangeCVarMsgToEngine: Unknown CVar '{0}'", pMsg3->m_sCVarName);
  }
  else if (const auto* pMsg4 = xiiDynamicCast<const xiiConsoleCmdMsgToEngine*>(e.m_pMessage))
  {
    if (m_pConsole->GetCommandInterpreter())
    {
      xiiCommandInterpreterState s;
      s.m_sInput = pMsg4->m_sCommand;

      xiiStringBuilder tmp;

      if (pMsg4->m_iType == 1)
      {
        m_pConsole->GetCommandInterpreter()->AutoComplete(s);
        tmp.AppendFormat(";;00||<{}", s.m_sInput);
      }
      else
        m_pConsole->GetCommandInterpreter()->Interpret(s);

      for (auto l : s.m_sOutput)
      {
        tmp.AppendFormat(";;{}||{}", xiiArgI((int)l.m_Type, 2, true), l.m_sText);
      }

      xiiConsoleCmdResultMsgToEditor r;
      r.m_sResult = tmp;

      m_IPC.SendMessage(&r);
    }
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<xiiEditorEngineDocumentMsg>())
    return;

  const xiiEditorEngineDocumentMsg* pDocMsg = (const xiiEditorEngineDocumentMsg*)e.m_pMessage;

  xiiEngineProcessDocumentContext* pDocumentContext = xiiEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (const auto* pMsg5 = xiiDynamicCast<const xiiDocumentOpenMsgToEngine*>(e.m_pMessage)) // Document was opened or closed
  {
    if (pMsg5->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg5);
      XII_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '{0}'", pMsg5->m_sDocumentType);
    }
    else
    {
      xiiEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  if (const auto* pMsg6 = xiiDynamicCast<const xiiDocumentClearMsgToEngine*>(e.m_pMessage))
  {
    pDocumentContext = xiiEngineProcessDocumentContext::GetDocumentContext(pMsg6->m_DocumentGuid);

    if (pDocumentContext)
    {
      pDocumentContext->ClearExistingObjects();
    }
    return;
  }

  // can be null if the asset was deleted on disk manually
  if (pDocumentContext)
  {
    pDocumentContext->HandleMessage(pDocMsg);
  }
}

xiiEngineProcessDocumentContext* xiiEngineProcessGameApplication::CreateDocumentContext(const xiiDocumentOpenMsgToEngine* pMsg)
{
  xiiDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid                                  = pMsg->m_DocumentGuid;
  xiiEngineProcessDocumentContext* pDocumentContext = xiiEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr)
  {
    xiiRTTI* pRtti = xiiRTTI::GetFirstInstance();
    while (pRtti)
    {
      if (pRtti->IsDerivedFrom<xiiEngineProcessDocumentContext>())
      {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == xiiPropertyCategory::Constant)
        {
          const xiiStringBuilder sDocTypes(";", static_cast<xiiAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<xiiString>(), ";");
          const xiiStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sDocTypes.FindSubString(sRequestedType) != nullptr)
          {
            xiiLog::Dev("Created Context of type '{0}' for '{1}'", pRtti->GetTypeName(), pMsg->m_sDocumentType);
            for (xiiAbstractFunctionProperty* pFunc : pRtti->GetFunctions())
            {
              if (xiiStringUtils::IsEqual(pFunc->GetPropertyName(), "AllocateContext"))
              {
                xiiVariant                    res;
                xiiHybridArray<xiiVariant, 1> params;
                params.PushBack(pMsg);
                pFunc->Execute(nullptr, params, res);
                if (res.IsA<xiiEngineProcessDocumentContext*>())
                {
                  pDocumentContext = res.Get<xiiEngineProcessDocumentContext*>();
                }
                else
                {
                  xiiLog::Error("Failed to call custom allocator '{}::{}'.", pRtti->GetTypeName(), pFunc->GetPropertyName());
                }
              }
            }

            if (!pDocumentContext)
            {
              pDocumentContext = pRtti->GetAllocator()->Allocate<xiiEngineProcessDocumentContext>();
            }

            xiiEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pMsg->m_DocumentMetaData, pDocumentContext, &m_IPC, pMsg->m_sDocumentType);
            break;
          }
        }
      }

      pRtti = pRtti->GetNextInstance();
    }
  }
  else
  {
    pDocumentContext->Reset();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}

void xiiEngineProcessGameApplication::Init_LoadProjectPlugins()
{
  m_CustomPluginConfig.m_Plugins.Sort([](const xiiApplicationPluginConfig::PluginConfig& lhs, const xiiApplicationPluginConfig::PluginConfig& rhs) -> bool {
    const bool isEnginePluginLhs = lhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;
    const bool isEnginePluginRhs = rhs.m_sAppDirRelativePath.FindSubString_NoCase("EnginePlugin") != nullptr;

    if (isEnginePluginLhs != isEnginePluginRhs)
    {
      // make sure the "engine plugins" end up at the back of the list
      // the reason for this is, that the engine plugins often have a link dependency on runtime plugins and pull their reflection data in right away
      // but then the xiiPlugin system doesn't know that certain reflected types actually come from some runtime plugin
      // by loading the editor engine plugins last, this solves that problem
      return isEnginePluginRhs;
    }

    return lhs.m_sAppDirRelativePath.Compare_NoCase(rhs.m_sAppDirRelativePath) < 0; });

  m_CustomPluginConfig.Apply();
}

xiiString xiiEngineProcessGameApplication::FindProjectDirectory() const
{
  return m_sProjectDirectory;
}

void xiiEngineProcessGameApplication::Init_FileSystem_ConfigureDataDirs()
{
  xiiStringBuilder sAppDir   = ">sdk/Data/Tools/EditorEngineProcess";
  xiiStringBuilder sUserData = ">user/XII/Engine/EditorEngineProcess";

  // make sure these directories exist
  xiiFileSystem::CreateDirectoryStructure(sAppDir).IgnoreResult();
  xiiFileSystem::CreateDirectoryStructure(sUserData).IgnoreResult();

  xiiFileSystem::AddDataDirectory("", "EngineProcess", ":", xiiFileSystem::AllowWrites).IgnoreResult();                   // for absolute paths
  xiiFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "bin", xiiFileSystem::ReadOnly).IgnoreResult();            // writing to the binary directory
  xiiFileSystem::AddDataDirectory(">appdir/", "EngineProcess", "shadercache", xiiFileSystem::AllowWrites).IgnoreResult(); // for shader files
  xiiFileSystem::AddDataDirectory(sAppDir.GetData(), "EngineProcess", "app").IgnoreResult();                              // app specific data
  xiiFileSystem::AddDataDirectory(sUserData, "EngineProcess", "appdata", xiiFileSystem::AllowWrites).IgnoreResult();      // for writing app user data

  m_CustomFileSystemConfig.Apply();
}

bool xiiEngineProcessGameApplication::Run_ProcessApplicationInput()
{
  // override the escape action to not shut down the app, but instead close the play-the-game window
  if (xiiInputManager::GetInputActionState("GameApp", "CloseApp") != xiiKeyState::Up)
  {
    if (m_pGameState)
    {
      m_pGameState->RequestQuit();
    }
  }
  else
  {
    return SUPER::Run_ProcessApplicationInput();
  }

  return true;
}

xiiUniquePtr<xiiEditorEngineProcessApp> xiiEngineProcessGameApplication::CreateEngineProcessApp()
{
  return XII_DEFAULT_NEW(xiiEditorEngineProcessApp);
}

void xiiEngineProcessGameApplication::BaseInit_ConfigureLogging()
{
  SUPER::BaseInit_ConfigureLogging();

  xiiGlobalLog::AddLogWriter(xiiMakeDelegate(&xiiEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  xiiCVar::s_AllCVarEvents.AddEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerCVar, this));
  xiiPlugin::Events().AddEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerCVarPlugin, this));
}

void xiiEngineProcessGameApplication::Deinit_ShutdownLogging()
{
  xiiGlobalLog::RemoveLogWriter(xiiMakeDelegate(&xiiEngineProcessGameApplication::LogWriter, this));

  // used for sending CVar changes over to the editor
  xiiCVar::s_AllCVarEvents.RemoveEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerCVar, this));
  xiiPlugin::Events().RemoveEventHandler(xiiMakeDelegate(&xiiEngineProcessGameApplication::EventHandlerCVarPlugin, this));

  SUPER::Deinit_ShutdownLogging();
}

void xiiEngineProcessGameApplication::EventHandlerCVar(const xiiCVarEvent& e)
{
  if (e.m_EventType == xiiCVarEvent::ValueChanged)
  {
    TransmitCVar(e.m_pCVar);
  }

  if (e.m_EventType == xiiCVarEvent::ListOfVarsChanged)
  {
    // currently no way to remove CVars from the editor UI

    for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void xiiEngineProcessGameApplication::EventHandlerCVarPlugin(const xiiPluginEvent& e)
{
  if (e.m_EventType == xiiPluginEvent::Type::AfterLoadingBeforeInit)
  {
    for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      TransmitCVar(pCVar);
    }
  }
}

void xiiEngineProcessGameApplication::TransmitCVar(const xiiCVar* pCVar)
{
  xiiCVarMsgToEditor msg;
  msg.m_sName        = pCVar->GetName();
  msg.m_sPlugin      = pCVar->GetPluginName();
  msg.m_sDescription = pCVar->GetDescription();

  switch (pCVar->GetType())
  {
    case xiiCVarType::Int:
      msg.m_Value = ((xiiCVarInt*)pCVar)->GetValue();
      break;
    case xiiCVarType::Float:
      msg.m_Value = ((xiiCVarFloat*)pCVar)->GetValue();
      break;
    case xiiCVarType::Double:
      msg.m_Value = ((xiiCVarDouble*)pCVar)->GetValue();
      break;
    case xiiCVarType::Bool:
      msg.m_Value = ((xiiCVarBool*)pCVar)->GetValue();
      break;
    case xiiCVarType::String:
      msg.m_Value = ((xiiCVarString*)pCVar)->GetValue();
      break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  m_IPC.SendMessage(&msg);
}

void xiiEngineProcessGameApplication::HandleResourceUpdateMsg(const xiiResourceUpdateMsgToEngine& msg)
{
  const xiiRTTI* pRtti = xiiResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    xiiLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  xiiTypelessResourceHandle hResource = xiiResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    xiiStringBuilder sResourceDesc;
    sResourceDesc.Set(msg.m_sResourceType, "LiveUpdate");

    xiiUniquePtr<xiiResourceLoaderFromMemory> loader(XII_DEFAULT_NEW(xiiResourceLoaderFromMemory));
    loader->m_ModificationTimestamp = xiiTimestamp::CurrentTimestamp();
    loader->m_sResourceDescription  = sResourceDesc;

    xiiMemoryStreamWriter memoryWriter(&loader->m_CustomData);
    memoryWriter.WriteBytes(msg.m_Data.GetData(), msg.m_Data.GetCount()).IgnoreResult();

    xiiResourceManager::UpdateResourceWithCustomLoader(hResource, std::move(loader));

    xiiResourceManager::ForceLoadResourceNow(hResource);
  }
}

void xiiEngineProcessGameApplication::HandleResourceRestoreMsg(const xiiRestoreResourceMsgToEngine& msg)
{
  const xiiRTTI* pRtti = xiiResourceManager::FindResourceForAssetType(msg.m_sResourceType);

  if (pRtti == nullptr)
  {
    xiiLog::Error("Resource Type '{}' is unknown.", msg.m_sResourceType);
    return;
  }

  xiiTypelessResourceHandle hResource = xiiResourceManager::GetExistingResourceByType(pRtti, msg.m_sResourceID);

  if (hResource.IsValid())
  {
    xiiResourceManager::RestoreResource(hResource);
  }
}
