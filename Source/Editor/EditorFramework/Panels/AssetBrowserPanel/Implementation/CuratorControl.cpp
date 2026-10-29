/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>
#include <QProgressBar>

xiiQtCuratorControl::xiiQtCuratorControl(QWidget* pParent) :
  QWidget(pParent)
{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  pLayout->addWidget(m_pBackgroundProcess);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &xiiQtCuratorControl::BackgroundProcessClicked);
  m_pTransformProgress = new QProgressBar(this);
  m_pTransformProgress->setFormat(QStringLiteral("Transforming assets: %v/%m (%p%)"));
  m_pTransformProgress->hide();
  pLayout->addWidget(m_pTransformProgress);
  m_pCancelTransforms = new QToolButton(this);
  m_pCancelTransforms->setText(QStringLiteral("Cancel"));
  m_pCancelTransforms->setToolTip(QStringLiteral("Cancel queued transformations; finish active assets"));
  m_pCancelTransforms->hide();
  pLayout->addWidget(m_pCancelTransforms);
  connect(m_pCancelTransforms, &QAbstractButton::clicked, this, []() { xiiAssetProcessor::GetSingleton()->StopProcessTask(false); });
  pLayout->addSpacing(230);

  UpdateBackgroundProcessState();
  xiiAssetCurator::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::AssetCuratorEvents, this));
  xiiAssetProcessor::GetSingleton()->m_Events.AddEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::AssetProcessorEvents, this));
  xiiToolsProject::GetSingleton()->s_Events.AddEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::ProjectEvents, this));
}

xiiQtCuratorControl::~xiiQtCuratorControl()
{
  xiiAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::AssetCuratorEvents, this));
  xiiAssetProcessor::GetSingleton()->m_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::AssetProcessorEvents, this));
  xiiToolsProject::GetSingleton()->s_Events.RemoveEventHandler(xiiMakeDelegate(&xiiQtCuratorControl::ProjectEvents, this));
}

void xiiQtCuratorControl::paintEvent(QPaintEvent* e)
{
  QRect rect       = contentsRect();
  QRect rectButton = m_pBackgroundProcess->geometry();
  rect.setLeft(rectButton.right());

  QPainter painter(this);
  painter.setPen(QPen(Qt::NoPen));

  xiiUInt32                                                      uiNumAssets;
  xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT> sections;
  xiiAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);
  QColor colors[xiiAssetInfo::TransformState::COUNT];
  colors[xiiAssetInfo::TransformState::Unknown]                    = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Gray));
  colors[xiiAssetInfo::TransformState::NeedsImport]                = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Yellow));
  colors[xiiAssetInfo::TransformState::NeedsTransform]             = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Blue));
  colors[xiiAssetInfo::TransformState::NeedsThumbnail]             = xiiToQtColor(xiiColorScheme::DarkUI(float(xiiColorScheme::Blue + xiiColorScheme::Green) * 0.5f * xiiColorScheme::s_fIndexNormalizer));
  colors[xiiAssetInfo::TransformState::UpToDate]                   = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Green));
  colors[xiiAssetInfo::TransformState::MissingTransformDependency] = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Red));
  colors[xiiAssetInfo::TransformState::MissingPackageDependency]   = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Orange));
  colors[xiiAssetInfo::TransformState::MissingThumbnailDependency] = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Orange));
  colors[xiiAssetInfo::TransformState::CircularDependency]         = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Red));
  colors[xiiAssetInfo::TransformState::TransformError]             = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Red));

  painter.fillRect(rect, palette().base());
  if (uiNumAssets > 0)
  {
    const float    fTotalCount   = uiNumAssets;
    const xiiInt32 iTargetWidth  = rect.width();
    xiiInt32       iCurrentCount = 0;
    for (xiiInt32 i = 0; i < xiiAssetInfo::TransformState::COUNT; ++i)
    {
      const xiiInt32 iStartX = xiiInt32((iCurrentCount / fTotalCount) * iTargetWidth);
      iCurrentCount += sections[i];
      const xiiInt32 iEndX = xiiInt32((iCurrentCount / fTotalCount) * iTargetWidth);
      if (sections[i])
      {
        QRect area(rect.left() + iStartX, rect.bottom() - 2, iEndX - iStartX, 3);
        painter.fillRect(area, colors[i]);
      }
    }
  }

  const xiiUInt32 uiIssues  = sections[xiiAssetInfo::TransformState::MissingTransformDependency] + sections[xiiAssetInfo::TransformState::MissingThumbnailDependency] + sections[xiiAssetInfo::TransformState::MissingPackageDependency] + sections[xiiAssetInfo::TransformState::TransformError] + sections[xiiAssetInfo::TransformState::CircularDependency];
  const xiiUInt32 uiPending = sections[xiiAssetInfo::TransformState::Unknown] + sections[xiiAssetInfo::TransformState::NeedsImport] + sections[xiiAssetInfo::TransformState::NeedsTransform] + sections[xiiAssetInfo::TransformState::NeedsThumbnail];
  QString         sSummary  = tr("No assets");
  if (uiIssues > 0)
    sSummary = tr("%1 assets | %2 issues").arg(uiNumAssets).arg(uiIssues);
  else if (uiPending > 0)
    sSummary = tr("%1 assets | %2 pending").arg(uiNumAssets).arg(uiPending);
  else if (uiNumAssets > 0)
    sSummary = tr("%1 assets ready").arg(uiNumAssets);
  const QRect textRect = rect.adjusted(8, 0, -8, -3);
  painter.setPen(palette().color(QPalette::Text));
  painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, fontMetrics().elidedText(sSummary, Qt::ElideRight, textRect.width()));
}

void xiiQtCuratorControl::UpdateBackgroundProcessState()
{
  xiiAssetProcessor::ProcessTaskState state = xiiAssetProcessor::GetSingleton()->GetProcessTaskState();
  switch (state)
  {
    case xiiAssetProcessor::ProcessTaskState::Stopped:
      m_pBackgroundProcess->setToolTip("Start background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart.svg"));
      break;
    case xiiAssetProcessor::ProcessTaskState::Running:
      m_pBackgroundProcess->setToolTip("Stop background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause.svg"));
      break;
    case xiiAssetProcessor::ProcessTaskState::Stopping:
      m_pBackgroundProcess->setToolTip("Force stop background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingForceStop.svg"));
      break;
    default:
      break;
  }

  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(state == xiiAssetProcessor::ProcessTaskState::Running);
  m_pBackgroundProcess->setAccessibleName(m_pBackgroundProcess->toolTip());
}

void xiiQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  xiiAssetProcessor::ProcessTaskState state = xiiAssetProcessor::GetSingleton()->GetProcessTaskState();

  if (state == xiiAssetProcessor::ProcessTaskState::Stopped)
  {
    xiiAssetCurator::GetSingleton()->QueueCheckFileSystem();
    xiiAssetProcessor::GetSingleton()->StartProcessTask();
  }
  else
  {
    bool bForce = state == xiiAssetProcessor::ProcessTaskState::Stopping;
    xiiAssetProcessor::GetSingleton()->StopProcessTask(bForce);
  }
}

void xiiQtCuratorControl::SlotUpdateTransformStats()
{
  m_bScheduled = false;

  xiiUInt32                                                      uiNumAssets;
  xiiHybridArray<xiiUInt32, xiiAssetInfo::TransformState::COUNT> sections;
  xiiAssetCurator::GetSingleton()->GetAssetTransformStats(uiNumAssets, sections);

  xiiStringBuilder s;

  if (uiNumAssets > 0)
  {
    s.SetFormat("Unknown: {}\nImport Needed: {}\nTransform Needed: {}\nThumbnail Needed: {}\nMissing Dependency: {}\nCircular Dependency: {}\nFailed Transform: {}", sections[xiiAssetInfo::TransformState::Unknown], sections[xiiAssetInfo::TransformState::NeedsImport], sections[xiiAssetInfo::TransformState::NeedsTransform], sections[xiiAssetInfo::TransformState::NeedsThumbnail], sections[xiiAssetInfo::TransformState::MissingTransformDependency] + sections[xiiAssetInfo::TransformState::MissingThumbnailDependency] + sections[xiiAssetInfo::TransformState::MissingPackageDependency], sections[xiiAssetInfo::TransformState::CircularDependency], sections[xiiAssetInfo::TransformState::TransformError]);
    setToolTip(s.GetData());
  }
  else
  {
    setToolTip("");
  }
  update();
}

void xiiQtCuratorControl::ScheduleUpdateTransformStats()
{
  if (m_bScheduled)
    return;

  m_bScheduled = true;

  QTimer::singleShot(200, this, SLOT(SlotUpdateTransformStats()));
}

void xiiQtCuratorControl::AssetCuratorEvents(const xiiAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case xiiAssetCuratorEvent::Type::AssetUpdated:
      ScheduleUpdateTransformStats();
      break;
    case xiiAssetCuratorEvent::Type::FileSystemCheckStarted:
    case xiiAssetCuratorEvent::Type::AssetListReset:
      QMetaObject::invokeMethod(this, [this]() { UpdateProgress(); ScheduleUpdateTransformStats(); }, Qt::QueuedConnection);
      break;
    default:
      break;
  }
}

void xiiQtCuratorControl::AssetProcessorEvents(const xiiAssetProcessorEvent& e)
{
  if (e.m_Type == xiiAssetProcessorEvent::Type::TransformProgress || e.m_Type == xiiAssetProcessorEvent::Type::TransformsFinished)
  {
    QMetaObject::invokeMethod(this, [this]() { UpdateProgress(); }, Qt::QueuedConnection);
    return;
  }
  switch (e.m_Type)
  {
    case xiiAssetProcessorEvent::Type::ProcessTaskStateChanged:
    {
      QMetaObject::invokeMethod(this, "UpdateBackgroundProcessState", Qt::QueuedConnection);
    }
    break;
    default:
      break;
  }
}

void xiiQtCuratorControl::UpdateProgress()
{
  const auto progress = xiiAssetProcessor::GetSingleton()->GetTransformProgress();
  const bool checking = xiiAssetCurator::GetSingleton()->IsCheckingFileSystem();
  if (progress.m_bActive)
  {
    m_pTransformProgress->setFormat(QStringLiteral("Transforming assets: %v/%m (%p%)"));
    m_pTransformProgress->setRange(0, static_cast<int>(progress.m_uiTotal));
    m_pTransformProgress->setValue(static_cast<int>(progress.m_uiCompleted + progress.m_uiCanceled));
  }
  else if (checking)
  {
    m_pTransformProgress->setFormat(QStringLiteral("Checking assets..."));
    m_pTransformProgress->setRange(0, 0);
  }
  m_pTransformProgress->setVisible(progress.m_bActive || checking);
  m_pTransformProgress->setToolTip(checking ? QStringLiteral("Checking files and resolving asset metadata") : QString());
  m_pCancelTransforms->setVisible(progress.m_bActive);
  m_pCancelTransforms->setEnabled(!progress.m_bCancelRequested);
}

void xiiQtCuratorControl::ProjectEvents(const xiiToolsProjectEvent& e)
{
  switch (e.m_Type)
  {
    case xiiToolsProjectEvent::Type::ProjectClosing:
    case xiiToolsProjectEvent::Type::ProjectClosed:
    case xiiToolsProjectEvent::Type::ProjectOpened:
      ScheduleUpdateTransformStats();
      break;

    default:
      break;
  }
}
