/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>
#include <EditorFramework/Project/PlayerSession.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Application/Config/ProjectManifest.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/System/Process.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GameEngine/GameApplication/PlayerSessionProtocol.h>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScreen>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTimer>
#include <QUuid>
#include <QVBoxLayout>

struct xiiQtPlayerSession::Instance
{
  xiiString                                  m_sId;
  xiiString                                  m_sSession;
  xiiString                                  m_sProject;
  xiiString                                  m_sRole;
  xiiString                                  m_sConfigurationName;
  xiiString                                  m_sState = "launching";
  xiiString                                  m_sDetail;
  xiiString                                  m_sFailureDetail;
  xiiTime                                    m_Started = xiiTime::Now();
  xiiTime                                    m_StopDeadline;
  xiiTime                                    m_LastMessage       = xiiTime::Now();
  xiiUInt32                                  m_uiFirstPort       = 0;
  xiiUInt32                                  m_uiPortCount       = 0;
  xiiUInt64                                  m_uiSceneRevision   = 0;
  bool                                       m_bStopRequested    = false;
  bool                                       m_bStopSent         = false;
  bool                                       m_bConnected        = false;
  bool                                       m_bReady            = false;
  bool                                       m_bShutdownReported = false;
  bool                                       m_bFinalized        = false;
  bool                                       m_bFailure          = false;
  xiiNetworkListener                         m_Listener;
  xiiNetworkChannel                          m_Channel;
  QTemporaryDir                              m_TemporaryDirectory;
  xiiDynamicArray<xiiUniquePtr<xiiOSFile>>   m_PortLocks;
  xiiMutex                                   m_LogMutex;
  xiiStringBuilder                           m_LogTail;
  // Destroy/join the output reader before its callback's mutex and strings.
  xiiProcess m_Process;
};

xiiQtPlayerSession::xiiQtPlayerSession(QWidget* pParent) : QDialog(pParent)
{
  setWindowTitle("Player Sessions");
  resize(1000, 560);
  auto* pLayout = new QVBoxLayout(this);
  pLayout->addWidget(new QLabel("Each launch owns its instances. Stop requests are graceful, with a five-second fallback. Editor exit stops all owned Players.", this));
  m_pTable = new QTableWidget(0, 6, this);
  m_pTable->setHorizontalHeaderLabels({"Instance", "Project", "Role", "PID", "State", "Status"});
  m_pTable->setSelectionBehavior(QAbstractItemView::SelectRows);
  m_pTable->setSelectionMode(QAbstractItemView::SingleSelection);
  m_pTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
  m_pTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  m_pTable->horizontalHeader()->setStretchLastSection(true);
  pLayout->addWidget(m_pTable);
  auto* pButtons = new QHBoxLayout;
  for (int i = 0; i < 3; ++i)
  {
    auto* pButton = new QPushButton(i == 0 ? "Stop Instance" : i == 1 ? "Stop Session" :
                                                                        "Stop All",
                                    this);
    pButtons->addWidget(pButton);
    connect(pButton, &QPushButton::clicked, this, [this, i]() {
      const int row = m_pTable->currentRow();
      if (i == 2)
        StopAll();
      else if (row >= 0 && row < static_cast<int>(m_Instances.GetCount()))
      {
        if (i == 0)
          StopInstance(row);
        else
          StopSession(m_Instances[row]->m_sSession);
      }
    });
  }
  pLayout->addLayout(pButtons);
  auto* transitions = new QHBoxLayout;
  auto* scene       = new QLineEdit(this);
  scene->setPlaceholderText("Scene asset GUID or data-directory path");
  auto* change = new QPushButton("Transition Selected", this);
  auto* cancel = new QPushButton("Cancel Loading", this);
  transitions->addWidget(scene);
  transitions->addWidget(change);
  transitions->addWidget(cancel);
  pLayout->addLayout(transitions);
  connect(change, &QPushButton::clicked, this, [this, scene]() {
    const int row = m_pTable->currentRow();
    if (row >= 0 && TransitionInstance(row, scene->text().toUtf8().constData()).Failed())
      m_pLog->setPlainText("Cannot request a transition. Select a connected instance and provide a scene reference.");
  });
  connect(cancel, &QPushButton::clicked, this, [this]() {
    const int row = m_pTable->currentRow();
    if (row >= 0)
      CancelTransition(row).IgnoreResult();
  });
  m_pLog = new QPlainTextEdit(this);
  m_pLog->setReadOnly(true);
  m_pLog->setMaximumBlockCount(150);
  pLayout->addWidget(m_pLog);
  m_pTimer = new QTimer(this);
  connect(m_pTimer, &QTimer::timeout, this, [this]() { Tick(); });
  m_pTimer->start(100);
}

xiiQtPlayerSession::~xiiQtPlayerSession()
{
  Shutdown();
}

xiiStatus xiiQtPlayerSession::Launch(const xiiPlayerLaunchConfig& config, xiiStringView sProject, xiiArrayPtr<const xiiString> arguments)
{
  if (m_bShuttingDown || config.Validate().Failed())
    return xiiStatus("Invalid launch configuration: use 1-16 instances, valid roles and a port block below 65536.");

  xiiStringBuilder project(sProject);
  if (!config.m_sProject.IsEmpty())
  {
    project = config.m_sProject;
    if (project.IsRelativePath())
      project.Prepend(sProject, "/");
  }
  project.MakeCleanPath();
  if (!project.IsAbsolutePath() || !xiiOSFile::ExistsFile(xiiStringBuilder(project, "/xiiProject")))
    return xiiStatus("The launch project must contain xiiProject and resolve to an absolute directory.");

  xiiProjectManifest manifest;
  const auto         projectStatus = manifest.Load(xiiStringBuilder(project, "/xiiProject"));
  if (projectStatus.Failed())
    return projectStatus;
  const auto compatibility = manifest.CheckEngineCompatibility();
  if (compatibility.Failed())
    return compatibility;

  xiiStringBuilder application(config.m_sApplication);
  if (application.IsEmpty())
  {
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    application = "xiiPlayer.exe";
#else
    application = "xiiPlayer";
#endif
  }
  if (application.IsRelativePath())
    application.Prepend(xiiOSFile::GetApplicationDirectory(), "/");
  application.MakeCleanPath();
  if (!xiiOSFile::ExistsFile(application))
    return xiiStatus(xiiFmt("Player application does not exist: '{}'", application));

  // No caller-controlled option may replace managed identity, storage or IPC. Arguments are
  // passed directly to the process API, including paths with spaces; no shell is involved.
  auto reserved = [](xiiStringView arg) {
    return arg.IsEqual_NoCase("-player-session") || arg.IsEqual_NoCase("-player-instance") ||
      arg.IsEqual_NoCase("-player-ipc") || arg.IsEqual_NoCase("-player-editor-pid") || arg.IsEqual_NoCase("-player-temp") ||
      arg.IsEqual_NoCase("-TelemetryPort") || arg.IsEqual_NoCase("-project") ||
      arg.IsEqual_NoCase("-player-index") || arg.IsEqual_NoCase("-role") || arg.IsEqual_NoCase("-connect") || arg.IsEqual_NoCase("-port");
  };
  for (const auto& arg : config.m_Arguments)
    if (reserved(arg))
      return xiiStatus(xiiFmt("Launch argument '{}' is managed by the session.", arg));

  xiiHybridArray<xiiProcessOptions, 16> options;
  for (xiiUInt32 i = 0; i < config.m_uiInstances; ++i)
  {
    auto& option               = options.ExpandAndGetRef();
    option.m_sProcess          = application;
    option.m_sWorkingDirectory = xiiOSFile::GetApplicationDirectory();

    if (i < config.m_InstanceArguments.GetCount())
    {
      xiiDynamicArray<xiiString>   instance;
      xiiDynamicArray<const char*> argv;
      xiiCommandLineUtils::SplitCommandLineString(config.m_InstanceArguments[i], false, instance, argv);
      for (const auto& arg : instance)
      {
        if (reserved(arg))
          return xiiStatus(xiiFmt("Instance argument '{}' is managed by the session.", arg));
        option.m_Arguments.PushBack(arg);
      }
    }
    for (const auto& arg : config.m_Arguments)
      option.m_Arguments.PushBack(arg);
    if (!config.m_sScene.IsEmpty())
    {
      option.AddArgument("-scene");
      option.AddArgument(config.m_sScene);
    }
    for (const auto& arg : arguments)
      option.m_Arguments.PushBack(arg);
  }

  for (const auto& option : options)
  {
    const auto validation = xiiPlayerLaunchConfig::ValidateRuntimeArguments(option.m_Arguments);
    if (validation.Failed())
      return validation;
  }

  if (!m_pControl)
  {
    auto pControl = XII_DEFAULT_NEW(xiiPlayerSessionProtocol);
    if (pControl->StartEditor(xiiMakeDelegate(&xiiQtPlayerSession::HandleStatus, this)).Failed())
      return xiiStatus("Cannot start the managed Player control host.");
    m_pControl = std::move(pControl);
  }

  const xiiString session   = QUuid::createUuid().toString(QUuid::Id128).toUtf8().constData();
  xiiUInt32       firstPort = config.m_uiBasePort;
  if (firstPort == 0)
    firstPort = 20000 + (xiiHashingUtils::StringHashTo32(xiiHashingUtils::StringHash(session)) % 1000) * 32;
  // Exclusive file leases coordinate deterministic port blocks across editor processes. They are
  // held until the last process in the session exits; closing an editor cannot steal another's lease.
  xiiDynamicArray<xiiUniquePtr<xiiOSFile>> portLocks;
  const xiiStringBuilder                   portDirectory(xiiOSFile::GetTempDataFolder(), "/xiiPlayer-ports");
  if (xiiOSFile::CreateDirectoryStructure(portDirectory).Failed())
    return xiiStatus("Cannot create the Player port lease directory.");
  for (xiiUInt32 attempt = 0; attempt < 1000; ++attempt)
  {
    portLocks.Clear();
    for (xiiUInt32 port = firstPort; port < firstPort + 2 * config.m_uiInstances; ++port)
    {
      auto             lease = XII_DEFAULT_NEW(xiiOSFile);
      xiiStringBuilder path;
      path.SetFormat("{}/{}.lock", portDirectory, port);
      if (lease->Open(path, xiiFileOpenMode::Write, xiiFileShareMode::Exclusive).Failed())
        break;
      portLocks.PushBack(std::move(lease));
    }
    if (portLocks.GetCount() == 2 * config.m_uiInstances)
      break;
    if (config.m_uiBasePort != 0)
      return xiiStatus("The port block is leased by another session. Select another base port.");
    firstPort = firstPort + 32 > 52000 ? 20000 : firstPort + 32;
  }
  if (portLocks.GetCount() != 2 * config.m_uiInstances)
    return xiiStatus("No free Player port block is available.");

  // Keep bounded terminal history; never discard a live instance or its process handle.
  for (xiiUInt32 i = m_Instances.GetCount(); i > 0 && m_Instances.GetCount() > 64; --i)
    if (m_Instances[i - 1]->m_Process.GetState() != xiiProcessState::Running && m_Instances[i - 1]->m_PortLocks.IsEmpty())
      m_Instances.RemoveAtAndCopy(i - 1);

  const QRect area    = QGuiApplication::primaryScreen() ? QGuiApplication::primaryScreen()->availableGeometry() : QRect(0, 0, 1280, 720);
  const int   columns = config.m_uiInstances > 1 ? (config.m_uiInstances > 4 ? 4 : 2) : 1;
  const int   rows    = (config.m_uiInstances + columns - 1) / columns;
  for (xiiUInt32 i = 0; i < config.m_uiInstances; ++i)
  {
    auto      instance    = XII_DEFAULT_NEW(Instance);
    Instance* pInstance   = instance;
    pInstance->m_sSession = session;
    if (i == 0)
      pInstance->m_PortLocks = std::move(portLocks);
    xiiStringBuilder id;
    id.SetFormat("{}-{}", session, i + 1);
    pInstance->m_sId                = id;
    pInstance->m_sProject           = project;
    pInstance->m_sConfigurationName = config.m_sName;
    pInstance->m_uiFirstPort        = firstPort;
    pInstance->m_uiPortCount        = config.m_uiInstances * 2;
    pInstance->m_sRole              = config.m_sRole == "server-clients" ? (i == 0 ? "server" : "client") : config.m_sRole.GetData();
    m_Instances.PushBack(std::move(instance));
    auto& option               = options[i];
    option.m_sWorkingDirectory = pInstance->m_TemporaryDirectory.path().toUtf8().constData();
    option.AddArgument("-project");
    option.AddArgument(project);
    option.AddArgument("-player-session");
    option.AddArgument(session);
    option.AddArgument("-player-instance");
    option.AddArgument(pInstance->m_sId);
    option.AddArgument("-player-index");
    option.AddArgument("{}", i);
    option.AddArgument("-player-ipc");
    option.AddArgument(pInstance->m_sId);
    option.AddArgument("-player-editor-pid");
    option.AddArgument("{}", xiiProcess::GetCurrentProcessID());
    option.AddArgument("-player-temp");
    option.AddArgument(pInstance->m_TemporaryDirectory.path().toUtf8().constData());
    option.AddArgument("-TelemetryPort");
    option.AddArgument("{}", firstPort + config.m_uiInstances + i);
    xiiStringBuilder windowTitle;
    windowTitle.SetFormat("{} - {} [{}] ({})", project.GetFileNameAndExtension(), config.m_sName, pInstance->m_sId.GetLast(8), pInstance->m_sRole);
    option.AddArgument("-window-title");
    option.AddArgument(windowTitle);
    option.AddArgument("-role");
    option.AddArgument(pInstance->m_sRole);
    option.AddArgument("-connect");
    option.AddArgument(config.m_sConnect);
    option.AddArgument("-port");
    option.AddArgument("{}", config.m_sRole == "server-clients" ? firstPort : firstPort + i);
    if (!config.m_sScene.IsEmpty())
    {
      option.AddArgument("-scene");
      option.AddArgument(config.m_sScene);
    }
    // Arrangement defaults precede explicit per-instance overrides (the command line reads first occurrence).
    if (config.m_bArrangeWindows)
    {
      QRect instanceArea = area;
      for (xiiUInt32 argument = 0; argument + 1 < option.m_Arguments.GetCount(); ++argument)
        if (option.m_Arguments[argument].IsEqual_NoCase("-display"))
        {
          xiiInt32 display = -1;
          if (xiiConversionUtils::StringToInt(option.m_Arguments[argument + 1], display).Succeeded() && display >= 0 && display < QGuiApplication::screens().size())
            instanceArea = QGuiApplication::screens()[display]->availableGeometry();
          break;
        }
      const int width  = xiiMath::Max(320, instanceArea.width() / columns - 24);
      const int height = xiiMath::Max(200, instanceArea.height() / rows - 50);
      option.AddArgument("-window-mode");
      option.AddArgument("window");
      option.AddArgument("-width");
      option.AddArgument("{}", width);
      option.AddArgument("-height");
      option.AddArgument("{}", height);
      option.AddArgument("-window-x");
      option.AddArgument("{}", instanceArea.x() + (i % columns) * (instanceArea.width() / columns));
      option.AddArgument("-window-y");
      option.AddArgument("{}", instanceArea.y() + (i / columns) * (instanceArea.height() / rows));
    }
    option.m_onStdOut = [pInstance](xiiStringView text) {
      XII_LOCK(pInstance->m_LogMutex);
      pInstance->m_LogTail.Append(text);
      const auto characters = pInstance->m_LogTail.GetCharacterCount();
      if (characters > 8192)
        pInstance->m_LogTail.Shrink(characters - 8192, 0);
    };
    option.m_onStdError = option.m_onStdOut;
    xiiNetworkLocalEndpoint endpoint;
    if (!pInstance->m_TemporaryDirectory.isValid() ||
        xiiNetworkLocalEndpoint::Make(pInstance->m_sId, endpoint).Failed() ||
        m_pControl->GetHost().Listen(endpoint, pInstance->m_Listener).Failed())
    {
      pInstance->m_bFailure = true;
      pInstance->m_sState   = "failed";
      pInstance->m_sDetail  = "Cannot create private IPC or temporary directory.";
      StopSession(session);
      return xiiStatus(pInstance->m_sDetail.GetView());
    }
    if (pInstance->m_Process.Launch(option).Failed())
    {
      CloseControlConnection(*pInstance);
      pInstance->m_bFailure = true;
      pInstance->m_sState   = "failed";
      pInstance->m_sDetail  = "Process creation failed. Check the executable, dependencies and access rights.";
      StopSession(session);
      return xiiStatus(pInstance->m_sDetail.GetView());
    }
  }
  Tick();
  show();
  raise();
  return xiiStatus(XII_SUCCESS);
}

xiiResult xiiQtPlayerSession::TransitionInstance(xiiUInt32 uiIndex, xiiStringView sScene, xiiStringView sPreloadCollection)
{
  if (sScene.IsEmpty() || uiIndex >= m_Instances.GetCount())
    return XII_FAILURE;
  auto& instance = *m_Instances[uiIndex];
  if (instance.m_bStopRequested || !m_pControl || !m_pControl->IsReady(instance.m_Channel))
    return XII_FAILURE;
  xiiPlayerSceneMessage message;
  message.m_sInstance          = instance.m_sId;
  message.m_sScene             = sScene;
  message.m_sPreloadCollection = sPreloadCollection;
  return m_pControl->Send(instance.m_Channel, message).GetResult();
}

xiiResult xiiQtPlayerSession::CancelTransition(xiiUInt32 uiIndex)
{
  if (uiIndex >= m_Instances.GetCount())
    return XII_FAILURE;
  auto& instance = *m_Instances[uiIndex];
  if (instance.m_bStopRequested || !m_pControl || !m_pControl->IsReady(instance.m_Channel))
    return XII_FAILURE;
  xiiPlayerSceneMessage message;
  message.m_sInstance = instance.m_sId;
  message.m_bCancel   = true;
  return m_pControl->Send(instance.m_Channel, message).GetResult();
}

void xiiQtPlayerSession::StopInstance(xiiUInt32 uiIndex)
{
  if (uiIndex >= m_Instances.GetCount())
    return;
  auto& instance = *m_Instances[uiIndex];
  if (instance.m_Process.GetState() != xiiProcessState::Running || instance.m_bStopRequested)
    return;
  instance.m_bStopRequested = true;
  instance.m_sState         = "stopping";
  instance.m_StopDeadline   = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
}

void xiiQtPlayerSession::StopSession(xiiStringView sSession)
{
  for (xiiUInt32 i = 0; i < m_Instances.GetCount(); ++i)
    if (m_Instances[i]->m_sSession == sSession)
      StopInstance(i);
}

void xiiQtPlayerSession::StopAll()
{
  for (xiiUInt32 i = 0; i < m_Instances.GetCount(); ++i)
    StopInstance(i);
}

void xiiQtPlayerSession::Shutdown()
{
  if (m_bShuttingDown)
    return;
  m_bShuttingDown = true;
  m_pTimer->stop();
  StopAll();
  const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(6);
  bool       running  = true;
  while (running && xiiTime::Now() < deadline)
  {
    Tick();
    running = false;
    for (const auto& instance : m_Instances)
      running |= instance->m_Process.GetState() == xiiProcessState::Running;
    if (running)
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(10));
  }
  m_Instances.Clear(); // Owned process destructors terminate/join any remaining children.
  m_pControl.Clear();
}

void xiiQtPlayerSession::CloseControlConnection(Instance& instance)
{
  if (instance.m_Channel.IsValid())
    m_pControl->GetHost().Disconnect(instance.m_Channel.m_Connection, false).IgnoreResult();
  if (instance.m_Listener.IsValid())
    m_pControl->GetHost().CloseListener(instance.m_Listener).IgnoreResult();
  instance.m_Channel  = {};
  instance.m_Listener = {};
}

void xiiQtPlayerSession::ProcessControlEvents()
{
  if (!m_pControl)
    return;

  auto& host = m_pControl->GetHost();
  m_pControl->Update();
  xiiNetworkEvent event;
  for (xiiUInt32 uiEvent = 0; uiEvent < 256 && host.TryGetEvent(event); ++uiEvent)
  {
    const auto connection = event.m_Connection;
    if (event.m_Type == xiiNetworkEventType::Connected)
    {
      Instance* pInstance = nullptr;
      for (const auto& instance : m_Instances)
        if (instance->m_Listener.IsValid() && instance->m_sId == event.m_LocalServiceEndpoint.GetName())
        {
          pInstance = instance.Borrow();
          break;
        }
      if (pInstance == nullptr || pInstance->m_Channel.IsValid() || event.m_pSessionInfo == nullptr ||
          event.m_pSessionInfo->m_ConnectionInfo.m_uiLocalPeerProcessId == 0 ||
          event.m_pSessionInfo->m_ConnectionInfo.m_uiLocalPeerProcessId != pInstance->m_Process.GetProcessID())
      {
        host.Disconnect(connection, false).IgnoreResult();
        continue;
      }
      pInstance->m_Channel = event.m_pSessionInfo->m_DefaultChannel;
      host.CloseListener(pInstance->m_Listener).IgnoreResult();
      pInstance->m_Listener = {};
    }
    const bool bClosed = event.m_Type == xiiNetworkEventType::Closed;
    if (m_pControl->ProcessEvent(std::move(event)).Failed() || bClosed)
    {
      for (auto& instance : m_Instances)
        if (instance->m_Channel.IsValid() && instance->m_Channel.m_Connection == connection)
        {
          if (!bClosed)
            host.Disconnect(connection, false).IgnoreResult();
          instance->m_Channel = {};
          break;
        }
    }
  }
}

void xiiQtPlayerSession::HandleStatus(xiiNetworkChannel channel, const xiiPlayerStatusMessage& status)
{
  for (auto& instance : m_Instances)
  {
    if (instance->m_Channel != channel || instance->m_sId != status.m_sInstance)
      continue;
    instance->m_LastMessage     = xiiTime::Now();
    instance->m_uiSceneRevision = status.m_uiSceneRevision;
    instance->m_bReady |= status.m_sStage == "running";
    if (status.m_sStage == "failed")
    {
      instance->m_bFailure       = true;
      instance->m_sFailureDetail = status.m_sDetail;
    }
    else if (status.m_sStage == "running")
    {
      instance->m_bFailure = false;
      instance->m_sFailureDetail.Clear();
    }
    if (status.m_sStage == "stopping" || status.m_sStage == "exited")
    {
      instance->m_bShutdownReported = true;
      if (!instance->m_StopDeadline.IsPositive())
        instance->m_StopDeadline = xiiTime::Now() + xiiTime::MakeFromSeconds(5);
    }
    else if (instance->m_bStopRequested && !instance->m_bShutdownReported)
    {
      // A status after stop admission means the child may have refreshed its schemas during
      // startup. Reissue this idempotent command after status confirms the child can communicate.
      instance->m_bStopSent = false;
    }
    if (!instance->m_bStopRequested)
      instance->m_sState = status.m_sStage == "exited" ? xiiStringView("stopping") : status.m_sStage.GetView();
    instance->m_sDetail = status.m_sDetail;
    if (status.m_fProgress >= 0.0f && status.m_fProgress <= 1.0f)
    {
      xiiStringBuilder detail;
      detail.SetFormat("{} ({}%%)", status.m_sDetail, xiiMath::RoundToInt(status.m_fProgress * 100.0f));
      instance->m_sDetail = detail;
    }
    break;
  }
}

void xiiQtPlayerSession::Tick()
{
  ProcessControlEvents();
  m_pTable->setRowCount(m_Instances.GetCount());
  for (xiiUInt32 i = 0; i < m_Instances.GetCount(); ++i)
  {
    auto& instance = *m_Instances[i];
    const bool running = instance.m_Process.GetState() == xiiProcessState::Running;
    if (running)
    {
      const bool connected = m_pControl && m_pControl->IsReady(instance.m_Channel);
      if (connected && instance.m_bStopRequested && !instance.m_bStopSent)
      {
        xiiPlayerStopMessage stop;
        stop.m_sInstance     = instance.m_sId;
        instance.m_bStopSent = m_pControl->Send(instance.m_Channel, stop).Succeeded();
      }
      if ((instance.m_bConnected && !instance.m_Channel.IsValid() && !instance.m_bStopRequested && !instance.m_bShutdownReported && xiiTime::Now() - instance.m_LastMessage > xiiTime::MakeFromSeconds(2)) ||
          (!instance.m_bConnected && xiiTime::Now() - instance.m_Started > xiiTime::MakeFromSeconds(30)))
      {
        if (!instance.m_bFailure)
          instance.m_sDetail = "Player control connection was lost or startup timed out.";
        instance.m_bFailure = true;
        StopInstance(i);
      }
      if (connected && !instance.m_bStopRequested && xiiTime::Now() - instance.m_LastMessage > xiiTime::MakeFromSeconds(30))
      {
        instance.m_sState  = "failed";
        instance.m_sDetail = "Player has not reported status for 30 seconds. Stop remains available with a bounded fallback.";
      }
      instance.m_bConnected |= connected;
      if ((instance.m_bStopRequested || instance.m_bShutdownReported) && xiiTime::Now() >= instance.m_StopDeadline)
      {
        if (instance.m_Process.Terminate().Failed())
          instance.m_sDetail = "Player did not stop and process termination failed.";
        else
          instance.m_sDetail = "Player exceeded the graceful-stop timeout and was terminated.";
        instance.m_bFailure = true;
      }
    }
    else if (instance.m_Process.GetState() == xiiProcessState::Finished)
    {
      const auto exitCode = instance.m_Process.GetExitCode();
      if (!instance.m_bFinalized)
      {
        instance.m_bFinalized = true;
        QFile assertion(instance.m_TemporaryDirectory.filePath("xiiDefaultAssertHandlerOutput.txt"));
        if (assertion.open(QIODevice::ReadOnly))
        {
          const auto text           = assertion.read(16384);
          instance.m_sFailureDetail = text.constData();
        }
        if (!instance.m_TemporaryDirectory.remove())
          xiiLog::Warning("Cannot remove private Player temporary directory '{}'.", instance.m_sId);
      }
      instance.m_bFailure |= exitCode != 0 || (!instance.m_bReady && !instance.m_bStopRequested);
      instance.m_sState = instance.m_bFailure ? "failed" : "exited";
      if (!instance.m_sDetail.StartsWith("Exit code"))
      {
        xiiStringBuilder detail;
        detail.SetFormat("Exit code {}. {}", exitCode, instance.m_sFailureDetail.IsEmpty() ? instance.m_sDetail : instance.m_sFailureDetail);
        instance.m_sDetail = detail;
      }
      CloseControlConnection(instance);
    }
    if (!instance.m_PortLocks.IsEmpty())
    {
      bool anyRunning = false;
      for (const auto& peer : m_Instances)
        if (peer->m_sSession == instance.m_sSession)
          anyRunning |= peer->m_Process.GetState() == xiiProcessState::Running;
      if (!anyRunning)
        instance.m_PortLocks.Clear();
    }
    const xiiStringBuilder shortId(instance.m_sId.GetLast(8));
    const QString          cells[] = {QString::fromUtf8(shortId.GetData()), QString::fromUtf8(instance.m_sProject.GetData()), QString::fromUtf8(instance.m_sRole.GetData()),
                                      instance.m_Process.GetState() == xiiProcessState::NotStarted ? QString() : QString::number(instance.m_Process.GetProcessID()),
                                      QString::fromUtf8(instance.m_sState.GetData()), QString::fromUtf8(instance.m_sDetail.GetData())};
    for (int column = 0; column < 6; ++column)
    {
      auto* item = m_pTable->item(i, column);
      if (!item)
      {
        item = new QTableWidgetItem;
        m_pTable->setItem(i, column, item);
      }
      item->setText(cells[column]);
      item->setToolTip(QString::fromUtf8(instance.m_sId.GetData()));
    }
    if (m_pTable->currentRow() == static_cast<int>(i))
    {
      XII_LOCK(instance.m_LogMutex);
      const QString log = QString::fromUtf8(instance.m_LogTail.GetData());
      if (m_pLog->toPlainText() != log)
        m_pLog->setPlainText(log);
    }
  }
}

xiiStringView xiiQtPlayerSession::GetInstanceState(xiiUInt32 uiIndex) const { return m_Instances[uiIndex]->m_sState; }
xiiStringView xiiQtPlayerSession::GetInstanceId(xiiUInt32 uiIndex) const { return m_Instances[uiIndex]->m_sId; }
xiiStringView xiiQtPlayerSession::GetInstanceDetail(xiiUInt32 uiIndex) const { return m_Instances[uiIndex]->m_sDetail; }
xiiStringView xiiQtPlayerSession::GetSessionId(xiiUInt32 uiIndex) const { return m_Instances[uiIndex]->m_sSession; }

xiiString xiiQtPlayerSession::GetInstanceLog(xiiUInt32 uiIndex) const
{
  XII_LOCK(m_Instances[uiIndex]->m_LogMutex);
  return m_Instances[uiIndex]->m_LogTail;
}

xiiUInt64 xiiQtPlayerSession::GetInstanceSceneRevision(xiiUInt32 uiIndex) const { return m_Instances[uiIndex]->m_uiSceneRevision; }
