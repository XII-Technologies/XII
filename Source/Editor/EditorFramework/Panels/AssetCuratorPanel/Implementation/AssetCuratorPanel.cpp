#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetCuratorPanel/AssetCuratorPanel.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>

xiiQtAssetCuratorFilter::xiiQtAssetCuratorFilter(QObject* pParent) :
  xiiQtAssetFilter(pParent)
{
}

void xiiQtAssetCuratorFilter::SetFilterTransitive(bool bFilterTransitive)
{
  m_bFilterTransitive = bFilterTransitive;
}

bool xiiQtAssetCuratorFilter::IsAssetFiltered(xiiStringView sDataDirParentRelativePath, bool bIsFolder, const xiiSubAsset* pInfo) const
{
  if (!pInfo)
    return true;

  if (!pInfo->m_bMainAsset)
    return true;

  if ((pInfo->m_pAssetInfo->m_TransformState != xiiAssetInfo::MissingTransformDependency) && (pInfo->m_pAssetInfo->m_TransformState != xiiAssetInfo::CircularDependency) && (pInfo->m_pAssetInfo->m_TransformState != xiiAssetInfo::MissingThumbnailDependency) && (pInfo->m_pAssetInfo->m_TransformState != xiiAssetInfo::MissingPackageDependency) && (pInfo->m_pAssetInfo->m_TransformState != xiiAssetInfo::TransformError))
  {
    return true;
  }

  if (m_bFilterTransitive)
  {
    if (pInfo->m_pAssetInfo->m_TransformState == xiiAssetInfo::MissingThumbnailDependency)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingThumbnailDeps)
      {
        if (!xiiAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        {
          return false;
        }
      }

      return true;
    }

    if (pInfo->m_pAssetInfo->m_TransformState == xiiAssetInfo::MissingPackageDependency)
    {
      for (auto& ref : pInfo->m_pAssetInfo->m_MissingPackageDeps)
      {
        if (!xiiAssetCurator::GetSingleton()->FindSubAsset(ref).isValid())
        {
          return false;
        }
      }

      return true;
    }
  }

  return false;
}

XII_IMPLEMENT_SINGLETON(xiiQtAssetCuratorPanel);

xiiQtAssetCuratorPanel::xiiQtAssetCuratorPanel() :
  xiiQtApplicationPanel("Panel.AssetCurator"), m_SingletonRegistrar(this)
{
  QWidget* pDummy = new QWidget();
  setupUi(pDummy);
  pDummy->setContentsMargins(0, 0, 0, 0);
  pDummy->layout()->setContentsMargins(0, 0, 0, 0);

  // using pDummy instead of 'this' breaks auto-connect for slots
  setWidget(pDummy);
  setIcon(xiiQtUiServices::GetCachedIconResource(":/EditorFramework/Icons/AssetCurator.svg"));
  setWindowTitle(xiiMakeQString(xiiTranslate("Panel.AssetCurator")));

  connect(ListAssets, &QTreeView::doubleClicked, this, &xiiQtAssetCuratorPanel::onListAssetsDoubleClicked);
  connect(CheckIndirect, &QCheckBox::toggled, this, &xiiQtAssetCuratorPanel::onCheckIndirectToggled);

  xiiAssetProcessor::GetSingleton()->AddLogWriter(xiiMakeDelegate(&xiiQtAssetCuratorPanel::LogWriter, this));

  m_pFilter = new xiiQtAssetCuratorFilter(this);
  m_pModel  = new xiiQtAssetBrowserModel(this, m_pFilter);
  m_pModel->SetIconMode(false);

  TransformLog->ShowControls(false);

  ListAssets->setModel(m_pModel);
  ListAssets->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
  XII_VERIFY(connect(ListAssets->selectionModel(), &QItemSelectionModel::selectionChanged, this, &xiiQtAssetCuratorPanel::OnAssetSelectionChanged) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pModel, &QAbstractItemModel::dataChanged, this,
                     [this](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QVector<int>& roles) {
                       if (m_SelectedIndex.isValid() && topLeft.row() <= m_SelectedIndex.row() && m_SelectedIndex.row() <= bottomRight.row())
                       {
                         UpdateIssueInfo();
                       }
                     }),
             "signal/slot connection failed");

  XII_VERIFY(connect(m_pModel, &QAbstractItemModel::modelReset, this,
                     [this]() {
                       m_SelectedIndex = QPersistentModelIndex();
                       UpdateIssueInfo();
                     }),
             "signal/slot connection failed");
}

xiiQtAssetCuratorPanel::~xiiQtAssetCuratorPanel()
{
  xiiAssetProcessor::GetSingleton()->RemoveLogWriter(xiiMakeDelegate(&xiiQtAssetCuratorPanel::LogWriter, this));
}

void xiiQtAssetCuratorPanel::OnAssetSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  if (selected.isEmpty())
    m_SelectedIndex = QModelIndex();
  else
    m_SelectedIndex = selected.indexes()[0];

  UpdateIssueInfo();
}

void xiiQtAssetCuratorPanel::onListAssetsDoubleClicked(const QModelIndex& index)
{
  QString sAbsPath = m_pModel->data(index, xiiQtAssetBrowserModel::UserRoles::AbsolutePath).toString();

  xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(sAbsPath.toUtf8().data());
}

void xiiQtAssetCuratorPanel::onCheckIndirectToggled(bool checked)
{
  m_pFilter->SetFilterTransitive(!checked);
  m_pModel->resetModel();
}

void xiiQtAssetCuratorPanel::LogWriter(const xiiLoggingEventData& e)
{
  // Can be called from a different thread, but AddLogMsg is thread safe.
  xiiLogEntry msg(e);
  CuratorLog->GetLog()->AddLogMsg(msg);
}

void xiiQtAssetCuratorPanel::UpdateIssueInfo()
{
  if (!m_SelectedIndex.isValid())
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  xiiUuid assetGuid = m_pModel->data(m_SelectedIndex, xiiQtAssetBrowserModel::UserRoles::AssetGuid).value<xiiUuid>();
  auto    pSubAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (pSubAsset == nullptr)
  {
    TransformLog->GetLog()->Clear();
    return;
  }

  TransformLog->GetLog()->Clear();

  xiiAssetInfo* pAssetInfo = pSubAsset->m_pAssetInfo;

  auto getNiceName = [](const xiiString& sDep) -> xiiStringBuilder {
    if (xiiConversionUtils::IsStringUuid(sDep))
    {
      xiiUuid guid         = xiiConversionUtils::ConvertStringToUuid(sDep);
      auto    assetInfoDep = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
      if (assetInfoDep)
      {
        return assetInfoDep->m_pAssetInfo->m_Path.GetDataDirParentRelativePath();
      }

      xiiUInt64 uiLow;
      xiiUInt64 uiHigh;
      guid.GetValues(uiLow, uiHigh);
      xiiStringBuilder sTmp;
      sTmp.SetFormat("{} - u4{{},{}}", sDep, uiLow, uiHigh);

      return sTmp;
    }

    return sDep;
  };

  xiiLogEntryDelegate logger(([this](xiiLogEntry& ref_entry) -> void { TransformLog->GetLog()->AddLogMsg(std::move(ref_entry)); }));
  xiiStringBuilder    text;
  if (pAssetInfo->m_TransformState == xiiAssetInfo::MissingTransformDependency)
  {
    xiiLog::Error(&logger, "Missing Transform Dependency:");
    for (const xiiString& dep : pAssetInfo->m_MissingTransformDeps)
    {
      xiiStringBuilder m_sNiceName = getNiceName(dep);
      xiiLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == xiiAssetInfo::CircularDependency)
  {
    xiiLog::Error(&logger, "Circular Dependency:");
    for (const xiiString& ref : pAssetInfo->m_CircularDependencies)
    {
      xiiStringBuilder m_sNiceName = getNiceName(ref);
      xiiLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == xiiAssetInfo::MissingThumbnailDependency)
  {
    xiiLog::Error(&logger, "Missing Thumbnail Dependency:");
    for (const xiiString& ref : pAssetInfo->m_MissingThumbnailDeps)
    {
      xiiStringBuilder m_sNiceName = getNiceName(ref);
      xiiLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == xiiAssetInfo::MissingPackageDependency)
  {
    xiiLog::Error(&logger, "Missing Package Dependency:");
    for (const xiiString& ref : pAssetInfo->m_MissingPackageDeps)
    {
      xiiStringBuilder m_sNiceName = getNiceName(ref);
      xiiLog::Error(&logger, "{0}", m_sNiceName);
    }
  }
  else if (pAssetInfo->m_TransformState == xiiAssetInfo::TransformError)
  {
    xiiLog::Error(&logger, "Transform Error:");
    for (const xiiLogEntry& logEntry : pAssetInfo->m_LogEntries)
    {
      TransformLog->GetLog()->AddLogMsg(logEntry);
    }
  }
}
