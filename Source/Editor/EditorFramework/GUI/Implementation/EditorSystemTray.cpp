/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/GUI/EditorSystemTray.h>

#if QT_CONFIG(systemtrayicon)

#  include <EditorFramework/Assets/AssetProcessor.h>
#  include <EditorFramework/EditorApp/EditorApp.moc.h>
#  include <GuiFoundation/UIServices/UIServices.moc.h>
#  include <QAction>
#  include <QApplication>
#  include <QDir>
#  include <QEvent>
#  include <QFileInfo>
#  include <QIcon>
#  include <QStringList>
#  include <QThread>

xiiQtEditorSystemTray::xiiQtEditorSystemTray(xiiQtEditorApp& editor, QWidget& window, xiiProgress& progress) :
  QObject(&editor), m_Editor(editor), m_pWindow(&window), m_Progress(progress), m_pProcessor(xiiAssetProcessor::GetSingleton()), m_Icon(this)
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Tray integration requires the Qt GUI thread.");
  auto* pShow = m_Menu.addAction(QStringLiteral("Show XII Editor"), this, &xiiQtEditorSystemTray::ShowEditor);
  m_Menu.setDefaultAction(pShow);
  m_Menu.addSeparator();
  m_pRecentProjects = m_Menu.addMenu(QStringLiteral("Recent Projects"));
  m_pRecentProjects->setToolTipsVisible(true);
  m_pOpenProject = m_Menu.addAction(QStringLiteral("Open Project..."));
  connect(m_pOpenProject, &QAction::triggered, this, [this]() {
    if (CanChangeProject())
    {
      ShowEditor();
      m_Editor.GuiOpenDashboard();
    } }, Qt::QueuedConnection);
  m_Menu.addSeparator();
  m_pExit = m_Menu.addAction(QStringLiteral("Exit XII Editor"));
  connect(m_pExit, &QAction::triggered, this, [this]() {
    if (CanChangeProject())
    {
      ShowEditor();
      // The normal container close path owns unsaved-document prompts, vetoes, and layout saving.
      m_pWindow->close();
    } }, Qt::QueuedConnection);
  connect(&m_Menu, &QMenu::aboutToShow, this, &xiiQtEditorSystemTray::RefreshMenu);
  m_Icon.setContextMenu(&m_Menu);
  connect(&m_Icon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
      ShowEditor();
  });
  connect(qApp, &QCoreApplication::aboutToQuit, &m_Icon, &QSystemTrayIcon::hide);
  connect(&window, &QObject::destroyed, this, [this]() {
    m_Icon.hide();
    m_Menu.setEnabled(false);
  });
  qApp->installEventFilter(this);

  connect(&editor, &xiiQtEditorApp::ToolExecutionStarted, this, [this](const QString& sOperation) {
    QString sName = sOperation.simplified();
    if (sName.isEmpty())
      sName = QStringLiteral("External tool");
    ++m_Tools[sName];
    UpdateToolTip();
  });
  connect(&editor, &xiiQtEditorApp::ToolExecutionFinished, this, [this](bool, const QString& sOperation) {
    QString sName = sOperation.simplified();
    if (sName.isEmpty())
      sName = QStringLiteral("External tool");
    auto it = m_Tools.find(sName);
    if (it != m_Tools.end() && --it.value() == 0)
      m_Tools.erase(it);
    UpdateToolTip();
  });
  m_Progress.m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorSystemTray::ProgressEventHandler, this));
  if (m_pProcessor)
  {
    m_pProcessor->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtEditorSystemTray::ProcessorEventHandler, this));
    xiiAssetProcessorEvent event;
    event.m_Type = xiiAssetProcessorEvent::Type::ProcessTaskStateChanged;
    ProcessorEventHandler(event);
  }
  RefreshIcon();
  RefreshMenu();
  UpdateToolTip();
  // Keep the requested visibility even if the notification area is currently unavailable.
  // Qt registers the entry when support appears and restores it after Explorer restarts.
  m_Icon.show();
}

xiiQtEditorSystemTray::~xiiQtEditorSystemTray()
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Tray integration must be destroyed on the Qt GUI thread.");
  m_Icon.hide();
  m_Icon.setContextMenu(nullptr);
  m_Progress.m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorSystemTray::ProgressEventHandler, this));
  if (m_pProcessor)
    m_pProcessor->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtEditorSystemTray::ProcessorEventHandler, this));
  qApp->removeEventFilter(this);
}

void xiiQtEditorSystemTray::ProgressEventHandler(const xiiProgressEvent& e)
{
  const bool    bActive   = e.m_Type != xiiProgressEvent::Type::ProgressEnded;
  const QString sText     = bActive ? xiiMakeQString(e.m_pProgressbar->GetMainDisplayText()).simplified() : QString();
  const bool    bCanceled = e.m_pProgressbar->WasCanceled();
  // Copy producer-owned text before returning; queued updates never borrow a progress range.
  QMetaObject::invokeMethod(this, [this, bActive, sText, bCanceled]() {
    m_bProgressActive = bActive;
    m_sForeground = bActive ? (sText.isEmpty() ? QStringLiteral("Working") : sText) : QString();
    if (bActive && bCanceled)
      m_sForeground.prepend(QStringLiteral("Cancelling: "));
    UpdateToolTip(); }, Qt::AutoConnection);
}

void xiiQtEditorSystemTray::ProcessorEventHandler(const xiiAssetProcessorEvent& e)
{
  if (e.m_Type != xiiAssetProcessorEvent::Type::ProcessTaskStateChanged)
    return;
  const auto state = m_pProcessor->GetProcessTaskState();
  QMetaObject::invokeMethod(this, [this, state]() {
    switch (state)
    {
      case xiiAssetProcessor::ProcessTaskState::Stopped:
        m_sBackground.clear();
        break;
      case xiiAssetProcessor::ProcessTaskState::Running:
        m_sBackground = QStringLiteral("Background asset processing enabled");
        break;
      case xiiAssetProcessor::ProcessTaskState::Stopping:
        m_sBackground = QStringLiteral("Finishing background assets");
        break;
    }
    UpdateToolTip(); }, Qt::QueuedConnection);
}

void xiiQtEditorSystemTray::UpdateToolTip()
{
  XII_ASSERT_DEV(QThread::currentThread() == qApp->thread(), "Tray updates require the Qt GUI thread.");
  QStringList work;
  if (!m_sForeground.isEmpty())
    work.push_back(m_sForeground);
  for (auto it = m_Tools.cbegin(); it != m_Tools.cend(); ++it)
    work.push_back(it.value() > 1 ? QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()) : it.key());
  if (!m_sBackground.isEmpty())
    work.push_back(m_sBackground);
  QString sTip = QStringLiteral("XII Editor");
  if (!work.isEmpty())
    sTip += QLatin1Char('\n') + work.join(QStringLiteral("; "));
  // Windows accepts at most 127 UTF-16 code units. Do not split a surrogate pair when eliding.
  if (sTip.size() > 127)
  {
    sTip.truncate(126);
    if (sTip.back().isHighSurrogate())
      sTip.chop(1);
    sTip += QChar(0x2026);
  }
  if (m_Icon.toolTip() != sTip)
    m_Icon.setToolTip(sTip);
}

bool xiiQtEditorSystemTray::CanChangeProject() const
{
  return !m_Editor.IsStartingUp() && m_pWindow && m_pWindow->isEnabled() && QApplication::activeModalWidget() == nullptr &&
    !m_bProgressActive && m_Tools.isEmpty() && !m_Editor.IsProgressBarProcessingEvents();
}

void xiiQtEditorSystemTray::RefreshMenu()
{
  const bool bCanChange = CanChangeProject();
  m_pRecentProjects->clear();
  for (const auto& recent : m_Editor.GetRecentProjectsList().GetFileList())
  {
    const QString   sProject = xiiMakeQString(recent.m_File);
    const QFileInfo info(sProject);
    if (!info.isAbsolute() || info.fileName().compare(QStringLiteral("xiiProject"), Qt::CaseInsensitive) != 0 || !info.isFile() || !info.isReadable())
      continue;
    auto* pAction = m_pRecentProjects->addAction(info.dir().dirName().replace(QLatin1Char('&'), QStringLiteral("&&")));
    pAction->setToolTip(sProject);
    pAction->setData(sProject);
    connect(pAction, &QAction::triggered, this, [this, sProject]() {
      if (CanChangeProject())
      {
        ShowEditor();
        m_Editor.OpenProject(sProject.toUtf8().constData()).IgnoreResult();
      } }, Qt::QueuedConnection);
  }
  m_pRecentProjects->setEnabled(bCanChange && !m_pRecentProjects->actions().isEmpty());
  m_pOpenProject->setEnabled(bCanChange);
  m_pExit->setEnabled(bCanChange);
}

void xiiQtEditorSystemTray::ShowEditor()
{
  xiiQtEditorApp::ShowEditorWindow(m_pWindow);
}

void xiiQtEditorSystemTray::RefreshIcon()
{
  QIcon icon = m_pWindow ? m_pWindow->windowIcon() : QIcon();
  if (icon.isNull())
    icon = qApp->windowIcon();
  if (icon.isNull())
    icon = xiiQtUiServices::GetCachedIconResource(":/GuiFoundation/XII-Logo.svg");
  m_Icon.setIcon(icon);
}

bool xiiQtEditorSystemTray::eventFilter(QObject* pObject, QEvent* pEvent)
{
  if (pObject != qApp && pObject != m_pWindow)
    return false;
  switch (pEvent->type())
  {
    case QEvent::WindowIconChange:
    case QEvent::ApplicationWindowIconChange:
    case QEvent::ApplicationPaletteChange:
    case QEvent::ThemeChange:
    case QEvent::StyleChange:
    case QEvent::DevicePixelRatioChange:
      RefreshIcon();
      break;
    default:
      break;
  }
  return false;
}

#endif
