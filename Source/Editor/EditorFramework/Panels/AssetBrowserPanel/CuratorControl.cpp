#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/CuratorControl.moc.h>

xiiQtCuratorControl::xiiQtCuratorControl(QWidget* pParent) :
  QWidget(pParent), m_bScheduled(false), m_pBackgroundProcess(nullptr)
{
  QHBoxLayout* pLayout = new QHBoxLayout();
  setLayout(pLayout);
  layout()->setContentsMargins(0, 0, 0, 0);
  m_pBackgroundProcess = new QToolButton(this);
  pLayout->addWidget(m_pBackgroundProcess);
  connect(m_pBackgroundProcess, &QAbstractButton::clicked, this, &xiiQtCuratorControl::BackgroundProcessClicked);
  pLayout->addSpacing(200);

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
  colors[xiiAssetInfo::TransformState::MissingThumbnailDependency] = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Orange));
  colors[xiiAssetInfo::TransformState::CircularDependency]         = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Red));
  colors[xiiAssetInfo::TransformState::TransformError]             = xiiToQtColor(xiiColorScheme::DarkUI(xiiColorScheme::Red));

  const float    fTotalCount   = uiNumAssets;
  const xiiInt32 iTargetWidth  = rect.width();
  xiiInt32       iCurrentCount = 0;
  for (xiiInt32 i = 0; i < xiiAssetInfo::TransformState::COUNT; ++i)
  {
    xiiInt32 iStartX = xiiInt32((iCurrentCount / fTotalCount) * iTargetWidth);
    iCurrentCount += sections[i];
    xiiInt32 iEndX = xiiInt32((iCurrentCount / fTotalCount) * iTargetWidth);

    if (sections[i])
    {
      QRect area = rect;
      area.setLeft(rect.left() + iStartX);
      area.setRight(rect.left() + iEndX);
      painter.setBrush(QBrush(colors[i]));
      painter.drawRect(area);
    }
  }

  xiiStringBuilder s;
  s.Format("[Un: {0}, Imp: {4}, Tr: {1}, Th: {2}, Err: {3}]", sections[xiiAssetInfo::TransformState::Unknown],
           sections[xiiAssetInfo::TransformState::NeedsTransform], sections[xiiAssetInfo::TransformState::NeedsThumbnail],
           sections[xiiAssetInfo::TransformState::MissingTransformDependency] + sections[xiiAssetInfo::TransformState::MissingThumbnailDependency] +
             sections[xiiAssetInfo::TransformState::TransformError] + sections[xiiAssetInfo::TransformState::CircularDependency],
           sections[xiiAssetInfo::TransformState::NeedsImport]);

  painter.setPen(QPen(Qt::white));
  painter.drawText(rect, s.GetData(), QTextOption(Qt::AlignCenter));
}

void xiiQtCuratorControl::UpdateBackgroundProcessState()
{
  xiiAssetProcessor::ProcessTaskState state = xiiAssetProcessor::GetSingleton()->GetProcessTaskState();
  switch (state)
  {
    case xiiAssetProcessor::ProcessTaskState::Stopped:
      m_pBackgroundProcess->setToolTip("Start background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingStart16.png"));
      break;
    case xiiAssetProcessor::ProcessTaskState::Running:
      m_pBackgroundProcess->setToolTip("Stop background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingPause16.png"));
      break;
    case xiiAssetProcessor::ProcessTaskState::Stopping:
      m_pBackgroundProcess->setToolTip("Force stop background asset processing");
      m_pBackgroundProcess->setIcon(xiiQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorFramework/Icons/AssetProcessingForceStop16.png"));
      break;
    default:
      break;
  }

  m_pBackgroundProcess->setCheckable(true);
  m_pBackgroundProcess->setChecked(state == xiiAssetProcessor::ProcessTaskState::Running);
}

void xiiQtCuratorControl::BackgroundProcessClicked(bool checked)
{
  xiiAssetProcessor::ProcessTaskState state = xiiAssetProcessor::GetSingleton()->GetProcessTaskState();

  if (state == xiiAssetProcessor::ProcessTaskState::Stopped)
  {
    xiiAssetCurator::GetSingleton()->CheckFileSystem();
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
    s.Format("Unknown: {0}\nImport Needed: {1}\nTransform Needed: {2}\nThumbnail Needed: {3}\nMissing Dependency: {4}\nMissing Reference: {5}\nCircular Dependency: {6}\nFailed Transform: {7}",
             sections[xiiAssetInfo::TransformState::Unknown],
             sections[xiiAssetInfo::TransformState::NeedsImport],
             sections[xiiAssetInfo::TransformState::NeedsTransform],
             sections[xiiAssetInfo::TransformState::NeedsThumbnail],
             sections[xiiAssetInfo::TransformState::MissingTransformDependency],
             sections[xiiAssetInfo::TransformState::MissingThumbnailDependency],
             sections[xiiAssetInfo::TransformState::CircularDependency],
             sections[xiiAssetInfo::TransformState::TransformError]);
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
    default:
      break;
  }
}

void xiiQtCuratorControl::AssetProcessorEvents(const xiiAssetProcessorEvent& e)
{
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
