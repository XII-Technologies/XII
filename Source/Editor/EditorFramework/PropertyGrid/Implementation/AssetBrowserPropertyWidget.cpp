/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/PropertyGrid/AssetBrowserPropertyWidget.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

xiiQtAssetPropertyWidget::xiiQtAssetPropertyWidget() :
  xiiQtStandardPropertyWidget()
{
  m_uiThumbnailID = 0;

  m_pLayout = new QHBoxLayout(this);
  m_pLayout->setContentsMargins(0, 0, 0, 0);
  m_pLayout->setSpacing(0);
  setLayout(m_pLayout);

  m_pWidget = new xiiQtAssetLineEdit(this);
  m_pWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
  m_pWidget->setFocusPolicy(Qt::FocusPolicy::StrongFocus);
  m_pWidget->m_pOwner = this;
  setFocusProxy(m_pWidget);

  XII_VERIFY(connect(m_pWidget, SIGNAL(editingFinished()), this, SLOT(on_TextFinished_triggered())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pWidget, SIGNAL(textChanged(const QString&)), this, SLOT(on_TextChanged_triggered(const QString&))) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pWidget, SIGNAL(OpenAsset()), this, SLOT(OnOpenAssetDocument())) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(m_pWidget, SIGNAL(SelectAsset()), this, SLOT(on_BrowseFile_clicked())) != nullptr, "signal/slot connection failed");

  m_pButton = new QToolButton(this);
  m_pButton->setText(QStringLiteral("... "));
  m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  m_pButton->setPopupMode(QToolButton::InstantPopup);

  QMenu* pMenu = new QMenu();
  pMenu->setToolTipsVisible(true);
  m_pButton->setMenu(pMenu);

  connect(pMenu, &QMenu::aboutToShow, this, &xiiQtAssetPropertyWidget::OnShowMenu);

  m_pLayout->addWidget(m_pWidget);
  m_pLayout->addWidget(m_pButton);

  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageLoaded, this, &xiiQtAssetPropertyWidget::ThumbnailLoaded) != nullptr, "signal/slot connection failed");
  XII_VERIFY(connect(xiiQtImageCache::GetSingleton(), &xiiQtImageCache::ImageInvalidated, this, &xiiQtAssetPropertyWidget::ThumbnailInvalidated) != nullptr, "signal/slot connection failed");
}

bool xiiQtAssetPropertyWidget::IsValidAssetType(const char* szAssetReference) const
{
  xiiAssetCurator::xiiLockedSubAsset pAsset;

  if (!xiiConversionUtils::IsStringUuid(szAssetReference))
  {
    pAsset = xiiAssetCurator::GetSingleton()->FindSubAsset(szAssetReference);

    if (pAsset == nullptr)
    {
      const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();

      // if this file type is on the asset whitelist for this asset type, let it through
      return xiiAssetFileExtensionWhitelist::IsFileOnAssetWhitelist(pAssetAttribute->GetTypeFilter(), szAssetReference);
    }
  }
  else
  {
    const xiiUuid AssetGuid = xiiConversionUtils::ConvertStringToUuid(szAssetReference);

    pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);
  }

  // invalid asset in general
  if (pAsset == nullptr)
    return false;

  const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();

  if (pAssetAttribute->GetTypeFilter() == ";;") // empty type list -> allows everything
    return true;

  xiiStringBuilder sTypeFilter(";", pAsset->m_Data.m_sSubAssetsDocumentTypeName, ";");

  if (pAssetAttribute->GetTypeFilter().FindSubString_NoCase(sTypeFilter) != nullptr)
    return true;

  if (const xiiDocumentTypeDescriptor* pDesc = xiiDocumentManager::GetDescriptorForDocumentType(pAsset->m_Data.m_sSubAssetsDocumentTypeName))
  {
    for (const xiiString& comp : pDesc->m_CompatibleTypes)
    {
      sTypeFilter.Set(";", comp, ";");

      if (pAssetAttribute->GetTypeFilter().FindSubString_NoCase(sTypeFilter) != nullptr)
        return true;
    }
  }

  return false;
}

void xiiQtAssetPropertyWidget::OnInit()
{
  XII_ASSERT_DEV(m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>() != nullptr, "xiiQtAssetPropertyWidget was created without a xiiAssetBrowserAttribute!");
}

void xiiQtAssetPropertyWidget::UpdateThumbnail(const xiiUuid& guid, const char* szThumbnailPath)
{
  if (IsUndead())
    return;

  const QPixmap* pThumbnailPixmap = nullptr;

  if (guid.IsValid())
  {
    xiiUInt64 uiUserData1, uiUserData2;
    m_AssetGuid.GetValues(uiUserData1, uiUserData2);

    const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();
    xiiStringBuilder                sTypeFilter     = pAssetAttribute->GetTypeFilter();
    sTypeFilter.Trim(" ;");

    pThumbnailPixmap = xiiQtImageCache::GetSingleton()->QueryPixmapForType(sTypeFilter, szThumbnailPath, QModelIndex(), QVariant(uiUserData1), QVariant(uiUserData2), &m_uiThumbnailID);
  }

  if (pThumbnailPixmap)
  {
    m_pButton->setIcon(QIcon(pThumbnailPixmap->scaledToWidth(16, Qt::TransformationMode::SmoothTransformation)));
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonIconOnly);
  }
  else
  {
    m_pButton->setIcon(QIcon());
    m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);
  }
}

void xiiQtAssetPropertyWidget::InternalSetValue(const xiiVariant& value)
{
  xiiQtScopedBlockSignals b(m_pWidget);
  xiiQtScopedBlockSignals b2(m_pButton);

  if (!value.IsValid())
  {
    m_pWidget->setPlaceholderText(QStringLiteral("<Multiple Values>"));
  }
  else
  {
    xiiStringBuilder sText = value.ConvertTo<xiiString>();
    m_AssetGuid            = xiiUuid();
    xiiStringBuilder sThumbnailPath;

    if (xiiConversionUtils::IsStringUuid(sText))
    {
      if (!IsValidAssetType(sText))
      {
        m_uiThumbnailID = 0;

        m_pWidget->setText(xiiMakeQString(sText));

        m_pButton->setIcon(QIcon());
        m_pButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextOnly);

        m_Pal.setColor(QPalette::Text, Qt::red);
        m_pWidget->setPalette(m_Pal);

        return;
      }

      xiiUuid newAssetGuid = xiiConversionUtils::ConvertStringToUuid(sText);

      // If this is a thumbnail or transform dependency, make sure the target is not in our inverse hull, i.e. we don't create a circular dependency.
      const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();
      if (pAssetAttribute->GetDependencyFlags().IsAnySet(xiiDependencyFlags::Thumbnail | xiiDependencyFlags::Transform))
      {
        xiiUuid                            documentGuid = m_pObjectAccessor->GetObjectManager()->GetDocument()->GetGuid();
        xiiAssetCurator::xiiLockedSubAsset asset        = xiiAssetCurator::GetSingleton()->GetSubAsset(documentGuid);
        if (asset.isValid())
        {
          xiiSet<xiiUuid> inverseHull;
          xiiAssetCurator::GetSingleton()->GenerateInverseTransitiveHull(asset->m_pAssetInfo, inverseHull, true, true);
          if (inverseHull.Contains(newAssetGuid))
          {
            xiiQtUiServices::GetSingleton()->MessageBoxWarning("This asset can't be used here, as that would create a circular dependency.");
            return;
          }
        }
      }

      m_AssetGuid = newAssetGuid;
      auto pAsset = xiiAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid);

      if (pAsset)
      {
        pAsset->GetSubAssetIdentifier(sText);

        sThumbnailPath = pAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pAsset->m_pAssetInfo->m_Path, pAsset->m_Data.m_sName);
      }
      else
      {
        m_AssetGuid = xiiUuid();
      }
    }

    UpdateThumbnail(m_AssetGuid, sThumbnailPath);

    {
      const QColor validColor   = xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Green));
      const QColor invalidColor = xiiToQtColor(xiiColorScheme::LightUI(xiiColorScheme::Red));

      m_Pal.setColor(QPalette::Text, m_AssetGuid.IsValid() ? validColor : invalidColor);
      m_pWidget->setPalette(m_Pal);

      if (m_AssetGuid.IsValid())
        m_pWidget->setToolTip(QStringLiteral("Valid asset selected.\n\nCTRL+LMB or MMB to open the asset document.\nSHIFT+LMB to select a different asset."));
      else
        m_pWidget->setToolTip(QStringLiteral("The selected file is not a valid asset."));
    }

    m_pWidget->setPlaceholderText(QString());
    m_pWidget->setText(QString::fromUtf8(sText.GetData()));
  }
}

void xiiQtAssetPropertyWidget::showEvent(QShowEvent* event)
{
  // Use of style sheets (ADS) breaks previously set palette.
  m_pWidget->setPalette(m_Pal);
  xiiQtStandardPropertyWidget::showEvent(event);
}

void xiiQtAssetPropertyWidget::FillAssetMenu(QMenu& menu)
{
  if (!menu.isEmpty())
    menu.addSeparator();

  const bool bAsset = m_AssetGuid.IsValid();
  menu.setDefaultAction(menu.addAction(QIcon(), QLatin1String("Select Asset"), this, SLOT(on_BrowseFile_clicked())));
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Document.svg")), QLatin1String("Open Asset"), this, SLOT(OnOpenAssetDocument()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Select in Asset Browser"), this, SLOT(OnSelectInAssetBrowser()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/OpenFolder.svg")), QLatin1String("Open in Explorer"), this, SLOT(OnOpenExplorer()))->setEnabled(bAsset);
  menu.addAction(QIcon(QLatin1String(":/GuiFoundation/Icons/Guid.svg")), QLatin1String("Copy Asset Guid"), this, SLOT(OnCopyAssetGuid()))->setEnabled(bAsset);
  menu.addAction(QIcon(), QLatin1String("Create New Asset"), this, SLOT(OnCreateNewAsset()));
  menu.addAction(QIcon(":/GuiFoundation/Icons/Clear.svg"), QLatin1String("Clear Asset Reference"), this, SLOT(OnClearReference()))->setEnabled(bAsset);
}

void xiiQtAssetPropertyWidget::on_TextFinished_triggered()
{
  xiiStringBuilder sText = m_pWidget->text().toUtf8().data();

  auto pAsset = xiiAssetCurator::GetSingleton()->FindSubAsset(sText);

  if (pAsset)
  {
    xiiConversionUtils::ToString(pAsset->m_Data.m_Guid, sText);
  }

  BroadcastValueChanged(sText.GetData());
}

void xiiQtAssetPropertyWidget::on_TextChanged_triggered(const QString& value)
{
  if (!hasFocus())
    on_TextFinished_triggered();
}

void xiiQtAssetPropertyWidget::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const xiiUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  if (guid == m_AssetGuid)
  {
    UpdateThumbnail(guid, sPath.toUtf8().data());
  }
}

void xiiQtAssetPropertyWidget::ThumbnailInvalidated(QString sPath, xiiUInt32 uiImageID)
{
  if (m_uiThumbnailID == uiImageID)
  {
    UpdateThumbnail(xiiUuid(), "");
  }
}

void xiiQtAssetPropertyWidget::OnOpenAssetDocument()
{
  if (!m_AssetGuid.IsValid())
    return;

  if (auto asset = xiiAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid))
  {
    xiiQtEditorApp::GetSingleton()->OpenDocumentQueued(asset->m_pAssetInfo->m_Path.GetAbsolutePath(), GetSelection()[0].m_pObject);
  }
}

void xiiQtAssetPropertyWidget::OnSelectInAssetBrowser()
{
  xiiQtAssetBrowserPanel::GetSingleton()->AssetBrowserWidget->SetSelectedAsset(m_AssetGuid);
  xiiQtAssetBrowserPanel::GetSingleton()->raise();
}

void xiiQtAssetPropertyWidget::OnOpenExplorer()
{
  xiiString sPath;

  if (m_AssetGuid.IsValid())
  {
    sPath = xiiAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
  }
  else
  {
    sPath = m_pWidget->text().toUtf8().data();
    if (!xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath))
      return;
  }

  xiiQtUiServices::OpenInExplorer(sPath, true);
}

void xiiQtAssetPropertyWidget::OnCopyAssetGuid()
{
  xiiStringBuilder sGuid;

  if (m_AssetGuid.IsValid())
  {
    xiiConversionUtils::ToString(m_AssetGuid, sGuid);
  }
  else
  {
    sGuid = m_pWidget->text().toUtf8().data();
    if (!xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sGuid))
      return;
  }

  QClipboard* clipboard = QApplication::clipboard();
  QMimeData*  mimeData  = new QMimeData();
  mimeData->setText(QString::fromUtf8(sGuid.GetData()));
  clipboard->setMimeData(mimeData);

  xiiQtUiServices::GetSingleton()->ShowAllDocumentsTemporaryStatusBarMessage(xiiFmt("Copied asset GUID: {}", sGuid), xiiTime::MakeFromSeconds(5));
}

void xiiQtAssetPropertyWidget::OnCreateNewAsset()
{
  xiiString sPath;

  // try to pick a good path
  {
    if (m_AssetGuid.IsValid())
    {
      sPath = xiiAssetCurator::GetSingleton()->GetSubAsset(m_AssetGuid)->m_pAssetInfo->m_Path.GetAbsolutePath();
    }
    else
    {
      sPath = m_pWidget->text().toUtf8().data();

      if (sPath.IsEmpty())
      {
        sPath = ":project/";
      }

      xiiQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sPath);
    }
  }

  const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();
  xiiStringBuilder                sTypeFilter     = pAssetAttribute->GetTypeFilter();

  xiiHybridArray<xiiString, 4> allowedTypes;
  sTypeFilter.Split(false, allowedTypes, ";");

  xiiStringBuilder tmp;

  for (xiiString& type : allowedTypes)
  {
    tmp = type;
    tmp.Trim(" ");
    type = tmp;
  }

  struct info
  {
    xiiAssetDocumentManager*         pAssetMan = nullptr;
    const xiiDocumentTypeDescriptor* pDocType  = nullptr;
  };

  xiiMap<xiiString, info> typesToUse;

  {
    const xiiHybridArray<xiiDocumentManager*, 16>& managers = xiiDocumentManager::GetAllDocumentManagers();

    for (xiiDocumentManager* pMan : managers)
    {
      if (auto pAssetMan = xiiDynamicCast<xiiAssetDocumentManager*>(pMan))
      {
        xiiHybridArray<const xiiDocumentTypeDescriptor*, 4> documentTypes;
        pAssetMan->GetSupportedDocumentTypes(documentTypes);

        for (const xiiDocumentTypeDescriptor* pType : documentTypes)
        {
          if (allowedTypes.IndexOf(pType->m_sDocumentTypeName) == xiiInvalidIndex)
          {
            for (const xiiString& compType : pType->m_CompatibleTypes)
            {
              if (allowedTypes.IndexOf(compType) != xiiInvalidIndex)
                goto allowed;
            }

            continue;
          }

        allowed:

          auto& toUse = typesToUse[pType->m_sDocumentTypeName];

          toUse.pAssetMan = pAssetMan;
          toUse.pDocType  = pType;
        }
      }
    }
  }

  if (typesToUse.IsEmpty())
    return;

  xiiStringBuilder sFilter;
  QString          sSelectedFilter;

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    const xiiString sAssetType = ttu.pDocType->m_sDocumentTypeName;
    const xiiString sExtension = ttu.pDocType->m_sFileExtension;

    sFilter.AppendWithSeparator(";;", sAssetType, " (*.", sExtension, ")");

    if (sSelectedFilter.isEmpty())
    {
      sSelectedFilter = sExtension.GetData();
    }
  }


  xiiStringBuilder sOutput = sPath;
  {

    QString sStartDir = sOutput.GetFileDirectory().GetData(tmp);
    sOutput           = QFileDialog::getSaveFileName(QApplication::activeWindow(), "Create Asset", sStartDir, sFilter.GetData(), &sSelectedFilter, QFileDialog::Option::DontResolveSymlinks).toUtf8().data();

    if (sOutput.IsEmpty())
      return;
  }

  sFilter = sOutput.GetFileExtension();

  for (auto it : typesToUse)
  {
    const auto& ttu = it.Value();

    if (sFilter.IsEqual_NoCase(ttu.pDocType->m_sFileExtension))
    {
      xiiDocument* pDoc = nullptr;

      const xiiStatus res = ttu.pAssetMan->CreateDocument(ttu.pDocType->m_sDocumentTypeName, sOutput, pDoc, xiiDocumentFlags::RequestWindow | xiiDocumentFlags::AddToRecentFilesList);

      xiiQtUiServices::GetSingleton()->MessageBoxStatus(res, "Creating the document failed.");

      if (res.Succeeded())
      {
        // if this is an asset, make sure it gets transformed, so that the output file exists
        // and make sure the filesystem knows about it (the asset lookup table is written)
        // so that redirections inside the resource manager will work right away
        // otherwise they may only work after a while (the world gets set up again) which would be irritating
        if (xiiAssetDocument* pAsset = xiiDynamicCast<xiiAssetDocument*>(pDoc))
        {
          xiiAssetCurator::GetSingleton()->NotifyOfAssetChange(pAsset->GetGuid());

          if (pAsset->TransformAsset(xiiTransformFlags::Default).Failed())
          {
            xiiLog::Error("Failed to transform newly created asset '{}'", pDoc->GetDocumentPath());
            break;
          }

          xiiAssetCurator::GetSingleton()->MainThreadTick(false);
          xiiAssetCurator::GetSingleton()->WriteAssetTables(nullptr, true).IgnoreResult();
        }

        pDoc->EnsureVisible();

        InternalSetValue(sOutput.GetData());
        on_TextFinished_triggered();
      }
      break;
    }
  }
}

void xiiQtAssetPropertyWidget::OnClearReference()
{
  InternalSetValue("");
  on_TextFinished_triggered();
}

void xiiQtAssetPropertyWidget::OnShowMenu()
{
  m_pButton->menu()->clear();
  FillAssetMenu(*m_pButton->menu());
}

void xiiQtAssetPropertyWidget::on_BrowseFile_clicked()
{
  xiiStringBuilder                sFile           = m_pWidget->text().toUtf8().data();
  const xiiAssetBrowserAttribute* pAssetAttribute = m_pProp->GetAttributeByType<xiiAssetBrowserAttribute>();

  xiiQtAssetBrowserDlg dlg(this, m_AssetGuid, pAssetAttribute->GetTypeFilter(), {}, pAssetAttribute->GetRequiredTag());
  if (dlg.exec() == 0)
    return;

  xiiUuid assetGuid = dlg.GetSelectedAssetGuid();
  if (assetGuid.IsValid())
    xiiConversionUtils::ToString(assetGuid, sFile);

  if (sFile.IsEmpty())
  {
    sFile = dlg.GetSelectedAssetPathRelative();

    if (sFile.IsEmpty())
    {
      sFile = dlg.GetSelectedAssetPathAbsolute();

      xiiQtEditorApp::GetSingleton()->MakePathDataDirectoryRelative(sFile);
    }
  }

  if (sFile.IsEmpty())
    return;

  InternalSetValue(sFile.GetData());

  on_TextFinished_triggered();
}
