/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserInspector.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QCheckBox>
#include <QDialog>
#include <QHeaderView>
#include <QLabel>
#include <QLocale>
#include <QPlainTextEdit>
#include <QResizeEvent>
#include <QTabWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
  constexpr int s_iRelatedGuid = Qt::UserRole;
  constexpr int s_iRelatedFile = Qt::UserRole + 1;

  QTreeWidgetItem* AddAssetProperty(QTreeWidget* pTree, const QString& sName, const QString& sValue)
  {
    auto* pItem = new QTreeWidgetItem(pTree, {sName, sValue});
    pItem->setToolTip(1, sValue);
    return pItem;
  }
} // namespace

xiiQtAssetBrowserInspector::xiiQtAssetBrowserInspector(xiiQtAssetBrowserWidget* pBrowser, QWidget* pParent) :
  QWidget(pParent), m_pBrowser(pBrowser)
{
  setObjectName(QStringLiteral("AssetInspector"));
  setMinimumWidth(0);
  auto* pLayout = new QVBoxLayout(this);
  pLayout->setContentsMargins(4, 3, 4, 3);
  pLayout->setSpacing(3);

  auto* pHeading  = new QHBoxLayout;
  auto* pTitle    = new QLabel(tr("Inspector"), this);
  QFont titleFont = pTitle->font();
  titleFont.setBold(true);
  pTitle->setFont(titleFont);
  pHeading->addWidget(pTitle, 1);
  auto* pClose = new QToolButton(this);
  pClose->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  pClose->setToolTip(tr("Hide asset inspector"));
  pClose->setAccessibleName(tr("Hide asset inspector"));
  pClose->setAutoRaise(true);
  connect(pClose, &QToolButton::clicked, this, [this]() { m_pBrowser->m_pInspectorAction->setChecked(false); hide(); });
  pHeading->addWidget(pClose);
  pLayout->addLayout(pHeading);

  m_pSummary = new QLabel(this);
  m_pSummary->setWordWrap(true);
  m_pSummary->setMaximumHeight(32);
  m_pSummary->setTextFormat(Qt::PlainText);
  m_pSummary->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  pLayout->addWidget(m_pSummary);

  m_pTabs = new QTabWidget(this);
  m_pTabs->setDocumentMode(true);
  m_pTabs->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
  m_pTabs->setUsesScrollButtons(true);
  pLayout->addWidget(m_pTabs, 1);

  m_pOverview           = new QWidget(m_pTabs);
  auto* pOverview       = m_pOverview;
  auto* pOverviewLayout = new QVBoxLayout(pOverview);
  pOverviewLayout->setContentsMargins(3, 4, 3, 3);
  pOverviewLayout->setSpacing(3);
  m_pPreview = new QLabel(pOverview);
  m_pPreview->setObjectName(QStringLiteral("AssetInspectorPreview"));
  m_pPreview->setAlignment(Qt::AlignCenter);
  m_pPreview->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  m_pPreview->setFixedHeight(106);
  m_pPreview->setBackgroundRole(QPalette::AlternateBase);
  m_pPreview->setAutoFillBackground(true);
  pOverviewLayout->addWidget(m_pPreview);
  m_pStatus = new QLabel(pOverview);
  m_pStatus->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  pOverviewLayout->addWidget(m_pStatus);
  m_pDetails = new QTreeWidget(pOverview);
  m_pDetails->setObjectName(QStringLiteral("AssetInspectorDetails"));
  m_pDetails->setHeaderLabels({tr("Property"), tr("Value")});
  m_pDetails->setRootIsDecorated(false);
  m_pDetails->setUniformRowHeights(true);
  m_pDetails->setAlternatingRowColors(true);
  m_pDetails->setFrameShape(QFrame::NoFrame);
  m_pDetails->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Ignored);
  m_pDetails->header()->resizeSection(0, 104);
  pOverviewLayout->addWidget(m_pDetails, 1);

  auto* pActions  = new QHBoxLayout;
  auto  addAction = [pOverview, pActions](const QString& sText, const QIcon& icon = {}) {
    auto* pButton = new QToolButton(pOverview);
    pButton->setAutoRaise(true);
    pButton->setIcon(icon);
    pButton->setText(sText);
    pButton->setToolTip(sText);
    pButton->setAccessibleName(sText);
    pButton->setToolButtonStyle(icon.isNull() ? Qt::ToolButtonTextOnly : Qt::ToolButtonIconOnly);
    pActions->addWidget(pButton);
    return pButton;
  };
  m_pOpen               = addAction(tr("Open asset"));
  m_pTransform          = addAction(tr("Transform"), QIcon(":/EditorFramework/Icons/TransformAsset.svg"));
  m_pForceTransform     = addAction(tr("Force transform / reimport"), style()->standardIcon(QStyle::SP_BrowserReload));
  m_pQuickPreviewButton = addAction(tr("Quick preview"), QIcon(":/GuiFoundation/Icons/Search.svg"));
  pActions->addStretch();
  pOverviewLayout->addLayout(pActions);
  m_pTabs->addTab(pOverview, tr("Overview"));

  auto* pDependencyPage   = new QWidget(m_pTabs);
  auto* pDependencyLayout = new QVBoxLayout(pDependencyPage);
  pDependencyLayout->setContentsMargins(3, 4, 3, 3);
  pDependencyLayout->setSpacing(3);
  m_pTransitive = new QCheckBox(tr("Include indirect relationships"), pDependencyPage);
  pDependencyLayout->addWidget(m_pTransitive);
  m_pDependencies = new QTreeWidget(pDependencyPage);
  m_pDependencies->setObjectName(QStringLiteral("AssetInspectorDependencies"));
  m_pDependencies->setHeaderHidden(true);
  m_pDependencies->setUniformRowHeights(true);
  m_pDependencies->setFrameShape(QFrame::NoFrame);
  pDependencyLayout->addWidget(m_pDependencies, 1);
  m_pTabs->addTab(pDependencyPage, tr("Dependencies"));

  m_pDiagnostics = new QPlainTextEdit(m_pTabs);
  m_pDiagnostics->setObjectName(QStringLiteral("AssetInspectorDiagnostics"));
  m_pDiagnostics->setReadOnly(true);
  m_pDiagnostics->setFrameShape(QFrame::NoFrame);
  m_pTabs->addTab(m_pDiagnostics, tr("Diagnostics"));

  connect(m_pTabs, &QTabWidget::currentChanged, this, [this]() { RefreshDependencies(); });
  connect(m_pTransitive, &QCheckBox::toggled, this, [this]() {
    m_bDependenciesDirty = true;
    RefreshDependencies();
  });
  connect(m_pDependencies, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem* pItem) {
    const auto guid = pItem->data(0, s_iRelatedGuid).value<xiiUuid>();
    if (guid.IsValid())
      m_pBrowser->NavigateToAsset(guid);
    else if (const auto sPath = pItem->data(0, s_iRelatedFile).toString(); !sPath.isEmpty())
      m_pBrowser->NavigateToFile(sPath);
  });
  connect(m_pOpen, &QToolButton::clicked, m_pBrowser, &xiiQtAssetBrowserWidget::OnListOpenAssetDocument);
  connect(m_pTransform, &QToolButton::clicked, m_pBrowser, &xiiQtAssetBrowserWidget::OnTransform);
  connect(m_pForceTransform, &QToolButton::clicked, m_pBrowser, [this]() { m_pBrowser->TransformSelection(true); });
  connect(m_pQuickPreviewButton, &QToolButton::clicked, this, &xiiQtAssetBrowserInspector::ShowQuickPreview);

  Clear();
}

void xiiQtAssetBrowserInspector::Clear()
{
  delete m_pQuickPreview.data();
  delete m_pSpecializedPreview.data();
  SetSelection({});
}

void xiiQtAssetBrowserInspector::SetSelection(const QModelIndexList& selection)
{
  const QModelIndex selected = selection.size() == 1 ? selection.constFirst() : QModelIndex();
  if (selected != m_Selected)
  {
    delete m_pQuickPreview.data();
    delete m_pSpecializedPreview.data();
    m_Selected            = selected;
    m_pSpecializedPreview = CreateSpecializedPreview(m_pOverview, false);
    if (m_pSpecializedPreview)
    {
      m_pSpecializedPreview->setMaximumHeight(m_pPreview->height());
      static_cast<QVBoxLayout*>(m_pOverview->layout())->insertWidget(0, m_pSpecializedPreview);
    }
    m_pPreview->setVisible(m_pSpecializedPreview.isNull());
  }
  if (m_Selected.isValid())
  {
    m_pTabs->show();
    RefreshAsset();
  }
  else
  {
    m_pSummary->setText(selection.isEmpty() ? tr("Select an item to inspect it.") : tr("%n items selected", nullptr, selection.size()));
    m_pTabs->hide();
    m_pDetails->clear();
    m_pDependencies->clear();
    m_pDiagnostics->clear();
    m_pPreview->clear();
  }
}

void xiiQtAssetBrowserInspector::RefreshAsset()
{
  if (!m_Selected.isValid())
    return;

  const int                                   iRow      = m_Selected.row();
  const auto                                  guid      = m_Selected.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>();
  const auto                                  sType     = m_Selected.sibling(iRow, xiiQtAssetBrowserModel::TypeColumn).data().toString();
  const auto                                  sModified = m_Selected.sibling(iRow, xiiQtAssetBrowserModel::ModifiedColumn).data().toString();
  const auto                                  sAbsolute = m_Selected.data(xiiQtAssetBrowserModel::AbsolutePath).toString();
  const auto                                  sRelative = m_Selected.data(xiiQtAssetBrowserModel::RelativePath).toString();
  const xiiBitflags<xiiAssetBrowserItemFlags> flags     = static_cast<xiiAssetBrowserItemFlags::Enum>(m_Selected.data(xiiQtAssetBrowserModel::ItemFlags).toInt());
  const bool                                  bIsFile   = flags.IsSet(xiiAssetBrowserItemFlags::File) || guid.IsValid();
  m_pSummary->setText(m_Selected.data().toString() + (sType.isEmpty() ? QString() : QStringLiteral("\n") + sType));
  m_pDetails->clear();
  AddAssetProperty(m_pDetails, tr("Source path"), sAbsolute);
  AddAssetProperty(m_pDetails, tr("Content path"), sRelative);
  if (bIsFile)
  {
    auto* pSize = AddAssetProperty(m_pDetails, tr("File size"), tr("Loading…"));
    pSize->setData(0, Qt::UserRole, true);
    if (!sModified.isEmpty())
      AddAssetProperty(m_pDetails, tr("Modified"), sModified);
  }
  QStringList diagnostics;
  bool        bCanTransform = false;
  {
    auto* pCurator = xiiAssetCurator::GetSingleton();
    if (const auto asset = pCurator->GetSubAsset(guid))
    {
      const auto* pInfo = asset->m_pAssetInfo;
      bCanTransform     = !pInfo->m_pDocumentTypeDescriptor->m_AssetDocumentFlags.IsSet(xiiAssetDocumentFlags::DisableTransform);
      xiiStringBuilder sGuid;
      AddAssetProperty(m_pDetails, tr("GUID"), xiiMakeQString(xiiConversionUtils::ToString(guid, sGuid)));
      xiiStringBuilder sIdentifier;
      asset->GetSubAssetIdentifier(sIdentifier);
      AddAssetProperty(m_pDetails, tr("Asset identifier"), xiiMakeQString(sIdentifier));
      AddAssetProperty(m_pDetails, tr("Type"), sType);
      if (const auto* pProfile = pCurator->GetActiveAssetProfile())
      {
        AddAssetProperty(m_pDetails, tr("Profile"), xiiMakeQString(pProfile->GetConfigName()));
        AddAssetProperty(m_pDetails, tr("Target"), xiiMakeQString(pProfile->GetTargetPlatform()));
      }
      const auto& tags = pInfo->m_Info->GetAssetsDocumentTags();
      if (!tags.IsEmpty())
        AddAssetProperty(m_pDetails, tr("Tags"), xiiMakeQString(tags));
      pInfo->m_Info->VisitMetadataProperties({}, [this](xiiStringView sName, xiiStringView sValue) {
        AddAssetProperty(m_pDetails, xiiMakeQString(sName), xiiMakeQString(sValue));
        return true;
      });

      auto appendIssues = [&diagnostics](const QString& sHeading, const xiiSet<xiiString>& entries) {
        for (const auto& sEntry : entries)
          diagnostics.push_back(sHeading.arg(xiiMakeQString(sEntry)));
      };
      appendIssues(tr("Missing transform dependency · %1"), pInfo->m_MissingTransformDeps);
      appendIssues(tr("Missing thumbnail dependency · %1"), pInfo->m_MissingThumbnailDeps);
      appendIssues(tr("Missing package dependency · %1"), pInfo->m_MissingPackageDeps);
      appendIssues(tr("Circular dependency · %1"), pInfo->m_CircularDependencies);
      for (const auto& log : pInfo->m_LogEntries)
      {
        if (log.m_Type >= xiiLogMsgType::ErrorMsg && log.m_Type <= xiiLogMsgType::DebugMsg)
          diagnostics.push_back(xiiMakeQString(log.m_sMsg));
      }
    }
  }
  m_pDiagnostics->setPlainText(diagnostics.isEmpty() ? tr("No processing diagnostics for this item.") : diagnostics.join(QLatin1Char('\n')));
  m_pOpen->setEnabled(bIsFile);
  m_pTransform->setEnabled(bCanTransform);
  m_pForceTransform->setEnabled(bCanTransform);
  m_pQuickPreviewButton->setEnabled(guid.IsValid());
  RefreshProcessingStatus();
  RefreshPreview();

  RefreshFileSize();
  m_bDependenciesDirty = true;
  RefreshDependencies();
}

void xiiQtAssetBrowserInspector::RefreshProcessingStatus()
{
  if (!m_Selected.isValid())
    return;

  const auto state = m_Selected.data(xiiQtAssetBrowserModel::TransformState);
  QString    sStatus;
  if (state.isValid())
  {
    const auto guid = m_Selected.data(xiiQtAssetBrowserModel::AssetGuid).value<xiiUuid>();
    sStatus         = xiiAssetCurator::GetSingleton()->IsAssetBeingTransformed(guid) ? tr("Processing") : xiiQtAssetBrowserModel::TransformStateLabel(state.toInt());
  }
  m_pStatus->setText(sStatus);
  m_pStatus->setVisible(!sStatus.isEmpty());
}

void xiiQtAssetBrowserInspector::RefreshPreview()
{
  if (!m_Selected.isValid() || (m_pSpecializedPreview && !m_pQuickPreviewImage))
    return;

  const auto pixmap     = m_Selected.data(xiiQtAssetBrowserModel::Thumbnail).value<QPixmap>();
  const auto icon       = m_Selected.data(xiiQtAssetBrowserModel::AssetIcon).value<QIcon>();
  auto       setPreview = [&pixmap, &icon](QLabel* pLabel, int iSize) {
    if (!pixmap.isNull())
      pLabel->setPixmap(pixmap.scaled(iSize, iSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    else
      pLabel->setPixmap(icon.pixmap(iSize / 2));
  };
  if (!m_pSpecializedPreview)
    setPreview(m_pPreview, qMax(24, m_pPreview->height() - 4));
  if (m_pQuickPreviewImage)
    setPreview(m_pQuickPreviewImage, 320);
}

void xiiQtAssetBrowserInspector::RefreshFileSize()
{
  const auto    size   = m_Selected.data(xiiQtAssetBrowserModel::FileSize);
  const qint64  iBytes = size.isValid() ? size.toLongLong() : -1;
  const QString sSize  = !size.isValid() ? tr("Loading…") : iBytes >= 0 ? QLocale().formattedDataSize(iBytes) :
                                                                          tr("Unavailable");
  for (int i = 0; i < m_pDetails->topLevelItemCount(); ++i)
  {
    auto* pItem = m_pDetails->topLevelItem(i);
    if (!pItem->data(0, Qt::UserRole).toBool())
      continue;

    pItem->setText(1, sSize);
    pItem->setToolTip(1, iBytes >= 0 ? tr("%1 bytes").arg(iBytes) : sSize);
    return;
  }
}

void xiiQtAssetBrowserInspector::ShowTab(int iTab)
{
  m_pTabs->setCurrentIndex(iTab);
  RefreshDependencies();
}

void xiiQtAssetBrowserInspector::RefreshDependencies()
{
  if (!m_bDependenciesDirty || m_pTabs->currentIndex() != Dependencies || !m_Selected.isValid())
    return;

  m_bDependenciesDirty = false;
  m_pDependencies->clear();
  const auto        guid        = m_Selected.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>();
  const bool        bTransitive = m_pTransitive->isChecked();
  auto*             pCurator    = xiiAssetCurator::GetSingleton();
  xiiSet<xiiString> dependencies[3];
  xiiSet<xiiString> missing[3];
  xiiSet<xiiString> allDependencies;
  xiiSet<xiiUuid>   uses;
  xiiStringBuilder  sGuid;
  xiiConversionUtils::ToString(guid, sGuid);
  {
    const auto asset = pCurator->GetSubAsset(guid);
    if (asset)
    {
      const auto* pInfo = asset->m_pAssetInfo;
      dependencies[0]   = pInfo->m_Info->m_TransformDependencies;
      dependencies[1]   = pInfo->m_Info->m_ThumbnailDependencies;
      dependencies[2]   = pInfo->m_Info->m_PackageDependencies;
      missing[0]        = pInfo->m_MissingTransformDeps;
      missing[1]        = pInfo->m_MissingThumbnailDeps;
      missing[2]        = pInfo->m_MissingPackageDeps;
    }
  }
  if (guid.IsValid())
  {
    pCurator->FindAllUses(guid, uses, bTransitive);
    if (bTransitive)
      pCurator->GenerateTransitiveHull(sGuid, allDependencies, true, true, true);
  }
  else
  {
    const auto sPath = qtToXIIString(m_Selected.data(xiiQtAssetBrowserModel::AbsolutePath).toString());
    pCurator->FindAllUses(sPath, uses);
  }

  auto addDependency = [this, pCurator](QTreeWidgetItem* pParent, xiiStringView sReference, bool bMissing) {
    const auto asset = pCurator->FindSubAsset(sReference);
    const auto sName = asset ? xiiMakeQString(asset->GetName()) : xiiMakeQString(sReference);
    auto*      pItem = new QTreeWidgetItem(pParent, {bMissing ? tr("%1 (missing)").arg(sName) : sName});
    pItem->setToolTip(0, xiiMakeQString(sReference));
    if (asset)
    {
      pItem->setData(0, s_iRelatedGuid, QVariant::fromValue(asset->m_Data.m_Guid));
      pItem->setToolTip(0, xiiMakeQString(asset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath()));
    }
    else if (!bMissing)
    {
      xiiDataDirPath path;
      if (xiiFileSystemModel::GetSingleton()->ResolveFilePath(sReference, path).Succeeded())
        pItem->setData(0, s_iRelatedFile, xiiMakeQString(path.GetAbsolutePath()));
    }
  };

  const QStringList groups = {tr("Transform dependencies"), tr("Thumbnail dependencies"), tr("Package dependencies")};
  allDependencies.Remove(sGuid);
  for (int i = 0; i < groups.size(); ++i)
  {
    if (dependencies[i].IsEmpty())
      continue;

    auto* pGroup = new QTreeWidgetItem(m_pDependencies, {tr("%1 (%2)").arg(groups[i]).arg(dependencies[i].GetCount())});
    for (const auto& sDependency : dependencies[i])
    {
      addDependency(pGroup, sDependency, missing[i].Contains(sDependency));
      allDependencies.Remove(sDependency);
    }
    pGroup->setExpanded(true);
  }

  if (!allDependencies.IsEmpty())
  {
    auto* pGroup = new QTreeWidgetItem(m_pDependencies, {tr("Indirect dependencies (%1)").arg(allDependencies.GetCount())});
    for (const auto& sDependency : allDependencies)
      addDependency(pGroup, sDependency, false);
    pGroup->setExpanded(true);
  }

  uses.Remove(guid);
  if (!uses.IsEmpty())
  {
    auto* pGroup = new QTreeWidgetItem(m_pDependencies, {tr("Referenced by (%1)").arg(uses.GetCount())});
    for (const auto& id : uses)
    {
      xiiConversionUtils::ToString(id, sGuid);
      addDependency(pGroup, sGuid, false);
    }
    pGroup->setExpanded(true);
  }
  if (m_pDependencies->topLevelItemCount() == 0)
    new QTreeWidgetItem(m_pDependencies, {tr("No indexed relationships")});
}

void xiiQtAssetBrowserInspector::resizeEvent(QResizeEvent* pEvent)
{
  QWidget::resizeEvent(pEvent);
  if (!m_pPreview)
    return;

  const int iHeight = qBound(42, (pEvent->size().height() - 100) / 3, 144);
  m_pPreview->setFixedHeight(iHeight);
  if (m_pSpecializedPreview)
    m_pSpecializedPreview->setMaximumHeight(iHeight);
  RefreshPreview();
}

QWidget* xiiQtAssetBrowserInspector::CreateSpecializedPreview(QWidget* pParent, bool bQuickPreview)
{
  const auto guid = m_Selected.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>();
  if (!guid.IsValid())
    return nullptr;

  xiiAssetBrowserPreviewRequest request;
  request.m_Guid          = guid;
  request.m_pParent       = pParent;
  request.m_bQuickPreview = bQuickPreview;
  {
    const auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(guid);
    if (!asset)
      return nullptr;
    request.m_sAssetType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetView();
  }

  xiiQtAssetBrowserWidget::s_PreviewRequests.Broadcast(request);
  if (request.m_pPreview)
    request.m_pPreview->setParent(pParent);
  return request.m_pPreview;
}

void xiiQtAssetBrowserInspector::ShowQuickPreview()
{
  if (!m_Selected.data(xiiQtAssetBrowserModel::SubAssetGuid).value<xiiUuid>().IsValid())
    return;

  if (m_pQuickPreview)
  {
    m_pQuickPreview->raise();
    m_pQuickPreview->activateWindow();
    return;
  }

  auto* pDialog = new QDialog(m_pBrowser);
  pDialog->setAttribute(Qt::WA_DeleteOnClose);
  pDialog->setWindowTitle(m_Selected.data().toString());
  auto* pLayout = new QVBoxLayout(pDialog);
  if (auto* pPreview = CreateSpecializedPreview(pDialog, true))
    pLayout->addWidget(pPreview);
  else
  {
    auto* pImage = new QLabel(pDialog);
    pImage->setAlignment(Qt::AlignCenter);
    pImage->setMinimumSize(320, 320);
    pLayout->addWidget(pImage);
    m_pQuickPreviewImage = pImage;
  }
  m_pQuickPreview = pDialog;
  RefreshPreview();
  pDialog->show();
}
