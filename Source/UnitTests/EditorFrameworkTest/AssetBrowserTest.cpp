/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetBrowserWidget.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProcessor.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFrameworkTest/AssetTestDocument.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>

#include <QApplication>
#include <QLabel>
#include <QTabWidget>
#include <QTreeWidget>
#include <ads/DockManager.h>
#include <ads/DockWidget.h>
#include <ads/FloatingDockContainer.h>

XII_CREATE_SIMPLE_TEST_GROUP(AssetBrowser);

XII_CREATE_SIMPLE_TEST(AssetBrowser, IndexedWorkspaceAndPickers)
{
  auto&           editor        = *xiiQtEditorApp::GetSingleton();
  auto*           pApplication  = xiiApplication::GetApplicationInstance();
  const xiiString sPreviousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(sPreviousName));

  const xiiStringBuilder sPreferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/AssetBrowserPreferences");
  editor.StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, sPreferences);
  XII_SCOPE_EXIT(editor.CloseProject(); QApplication::processEvents(QEventLoop::AllEvents, 10); editor.ShutdownEditor());
  const xiiStringBuilder sProject(sPreferences, "/Project/xiiProject");
  if (!XII_TEST_RESULT(editor.CreateOrOpenProject(true, sProject)))
    return;

  xiiAssetProcessor::GetSingleton()->StopProcessTask(false, true);
  auto*            pCurator = xiiAssetCurator::GetSingleton();
  xiiStringBuilder sFolder(sProject.GetFileDirectory());
  sFolder.AppendPath("BrowserRegression");
  if (!XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(sFolder)))
    return;
  xiiStringBuilder sSource(sFolder, "/Industrial Source.xiiResaveTest");
  xiiStringBuilder sConsumer(sFolder, "/Industrial Consumer.xiiResaveTest");
  xiiStringBuilder sFile(sFolder, "/Source.txt");
  {
    xiiOSFile file;
    if (!XII_TEST_RESULT(file.Open(sFile, xiiFileOpenMode::Write)))
      return;
    XII_TEST_RESULT(file.Write("source", 6));
  }

  auto* pSource   = editor.CreateDocument(sSource, xiiDocumentFlags::None);
  auto* pConsumer = xiiDynamicCast<ResaveTestDocument*>(editor.CreateDocument(sConsumer, xiiDocumentFlags::None));
  if (!XII_TEST_BOOL(pSource && pConsumer))
    return;
  const auto                                             source   = pSource->GetGuid();
  const auto                                             consumer = pConsumer->GetGuid();
  const auto                                             child    = ResaveTestManager::GetSubAssetId(source);
  xiiEvent<xiiAssetBrowserPreviewRequest&>::Unsubscriber previewSubscription;
  xiiQtAssetBrowserWidget::s_PreviewRequests.AddEventHandler([consumer](xiiAssetBrowserPreviewRequest& request) {
    if (request.m_Guid != consumer || request.m_sAssetType != "ResaveTest" || request.m_pPreview)
      return;
    auto* pPreview = new QLabel(QStringLiteral("Custom preview"), request.m_pParent);
    pPreview->setObjectName(QStringLiteral("TestAssetBrowserPreview"));
    request.m_pPreview = pPreview;
  },
                                                             previewSubscription);
  xiiStringBuilder sSourceGuid;
  xiiConversionUtils::ToString(child, sSourceGuid);
  auto* pAccessor = pConsumer->GetObjectAccessor();
  pAccessor->StartTransaction("Reference source sub-asset");
  XII_TEST_STATUS(pAccessor->SetValueByName(pConsumer->GetPropertyObject(), "Package", sSourceGuid.GetView()));
  pAccessor->FinishTransaction();
  XII_TEST_STATUS(pConsumer->SaveDocument());
  pCurator->NotifyOfFileChange(sSource);
  pCurator->NotifyOfFileChange(sConsumer);
  pCurator->NotifyOfFileChange(sFile);
  pCurator->MainThreadTick(true);

  xiiQtAssetBrowserWidget browser(nullptr);
  browser.resize(1050, 510);
  browser.show();
  auto* pFilter = browser.GetAssetBrowserFilter();
  auto* pModel  = browser.GetAssetBrowserModel();
  using Model   = xiiQtAssetBrowserModel;
  using Scope   = xiiQtAssetBrowserFilter::Scope;

  // A folder reset can arrive in a separate event batch from the file reset during startup.
  QApplication::processEvents();
  xiiUInt32                   uiModelResets    = 0;
  const auto                  resetConnection  = QObject::connect(pModel, &QAbstractItemModel::modelReset, pModel, [&uiModelResets]() { ++uiModelResets; });
  const xiiInt32              iRowsBeforeReset = pModel->rowCount();
  const xiiFolderChangedEvent folderReset({}, xiiFolderChangedEvent::Type::ModelReset);
  xiiFileSystemModel::GetSingleton()->m_FolderChangedEvents.Broadcast(folderReset);
  QCoreApplication::sendPostedEvents(pModel, QEvent::MetaCall);
  QObject::disconnect(resetConnection);
  XII_TEST_INT(uiModelResets, 1);
  XII_TEST_INT(pModel->rowCount(), iRowsBeforeReset);

  xiiStringBuilder sRelativeFolder = sFolder;
  if (!XII_TEST_BOOL(editor.MakePathDataDirectoryParentRelative(sRelativeFolder)))
    return;

  pFilter->SetPathFilter(sRelativeFolder);
  XII_TEST_BOOL(pModel->FindAssetIndex(source) >= 0);
  XII_TEST_BOOL(pModel->FindAssetIndex(child) >= 0);
  xiiStringBuilder sQuery;
  sQuery.SetFormat("type:ResaveTest path:{}/* ref:{} name:Consumer", sRelativeFolder, sSourceGuid);
  pFilter->SetTextFilter(sQuery);
  XII_TEST_BOOL(pFilter->GetQueryError().isEmpty());
  XII_TEST_INT(pModel->rowCount(), 1);
  XII_TEST_BOOL(pModel->FindAssetIndex(consumer) >= 0);
  pFilter->SetTextFilter("status:Impossible");
  XII_TEST_BOOL(!pFilter->GetQueryError().isEmpty());
  XII_TEST_INT(pModel->rowCount(), 0);
  pFilter->SetTextFilter("");
  pFilter->SetTextFilter("Ind");
  const QPersistentModelIndex matchingAsset(pModel->index(pModel->FindAssetIndex(source), Model::NameColumn));
  pFilter->SetTextFilter("Industrial Source");
  XII_TEST_BOOL(matchingAsset.isValid());
  XII_TEST_INT(pModel->rowCount(), 2);
  pFilter->SetTextFilter("");
  pFilter->SetFavorite(source, true);
  pFilter->SetScope(Scope::Favorites);
  XII_TEST_INT(pModel->rowCount(), 1);
  XII_TEST_BOOL(pModel->FindAssetIndex(source) >= 0);
  pFilter->SetPathFilter(sRelativeFolder);

  browser.SetSelectedAsset(consumer);
  QApplication::processEvents();
  const QPersistentModelIndex selected(browser.ListAssets->selectionModel()->currentIndex());
  XII_TEST_BOOL(selected.data(Model::SubAssetGuid).value<xiiUuid>() == consumer);
  XII_TEST_BOOL(browser.AssetViews->currentWidget() == browser.ListAssets);
  browser.ButtonListMode->click();
  XII_TEST_BOOL(browser.ListAssets->viewMode() == QListView::ListMode);
  browser.ButtonDetailsMode->click();
  XII_TEST_BOOL(browser.AssetViews->currentWidget() == browser.DetailsAssets);
  XII_TEST_BOOL(browser.DetailsAssets->selectionModel()->currentIndex().data(Model::SubAssetGuid).value<xiiUuid>() == consumer);
  XII_TEST_BOOL(!QModelIndex(selected).siblingAtColumn(Model::TypeColumn).data().toString().isEmpty());
  XII_TEST_BOOL(!QModelIndex(selected).siblingAtColumn(Model::ModifiedColumn).data().toString().isEmpty());
  pModel->sort(Model::NameColumn, Qt::DescendingOrder);
  XII_TEST_BOOL(browser.DetailsAssets->selectionModel()->currentIndex().data(Model::SubAssetGuid).value<xiiUuid>() == consumer);

  auto* pInspector    = browser.findChild<QWidget*>("AssetInspector");
  auto* pDetails      = browser.findChild<QTreeWidget*>("AssetInspectorDetails");
  auto* pDependencies = browser.findChild<QTreeWidget*>("AssetInspectorDependencies");
  if (!XII_TEST_BOOL(pInspector && pDetails && pDependencies))
    return;
  XII_TEST_BOOL(pInspector->isVisible());
  XII_TEST_BOOL(pInspector->findChild<QLabel*>("TestAssetBrowserPreview") != nullptr);
  QVariant   fileSize;
  const auto timeout = xiiTime::Now() + xiiTime::MakeFromSeconds(3);
  while (!(fileSize = selected.data(Model::FileSize)).isValid() && xiiTime::Now() < timeout)
  {
    QApplication::processEvents();
    xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(5));
  }
  XII_TEST_BOOL(fileSize.isValid() && fileSize.toLongLong() > 0);
  XII_TEST_BOOL(!pDetails->findItems(xiiMakeQString(sConsumer), Qt::MatchExactly, 1).isEmpty());
  pInspector->findChild<QTabWidget*>()->setCurrentIndex(1);
  auto* pPackage = pDependencies->topLevelItem(0);
  if (XII_TEST_BOOL(pPackage && pPackage->childCount() == 1))
  {
    Q_EMIT pDependencies->itemActivated(pPackage->child(0), 0);
    QApplication::processEvents();
    XII_TEST_BOOL(browser.ListAssets->selectionModel()->currentIndex().data(Model::SubAssetGuid).value<xiiUuid>() == child);
    XII_TEST_BOOL(pInspector->findChild<QLabel*>("TestAssetBrowserPreview") == nullptr);
  }

  browser.resize(350, 440);
  QApplication::processEvents();
  XII_TEST_BOOL(browser.SearchWidget->isVisible());
  XII_TEST_BOOL(browser.ButtonOptions->isVisible());
  XII_TEST_INT(browser.width(), 350);
  XII_TEST_BOOL(!pInspector->isVisible());

  xiiQtAssetBrowserWidget assetPicker(nullptr);
  assetPicker.SetMode(xiiQtAssetBrowserWidget::Mode::AssetPicker);
  assetPicker.ShowOnlyTheseTypeFilters("ResaveTest");
  assetPicker.SetSelectedAsset(child);
  QApplication::processEvents();
  XII_TEST_BOOL(assetPicker.ListAssets->selectionModel()->currentIndex().data(Model::SubAssetGuid).value<xiiUuid>() == child);
  XII_TEST_BOOL(assetPicker.GetAssetBrowserModel()->FindIndex(sFile) < 0);

  xiiQtAssetBrowserWidget filePicker(nullptr);
  filePicker.SetMode(xiiQtAssetBrowserWidget::Mode::FilePicker);
  filePicker.UseFileExtensionFilters("*.txt");
  filePicker.SetSelectedFile(sFile);
  QApplication::processEvents();
  XII_TEST_BOOL(filePicker.ListAssets->selectionModel()->currentIndex().data(Model::AbsolutePath).toString() == xiiMakeQString(sFile));
  XII_TEST_BOOL(filePicker.GetAssetBrowserModel()->FindAssetIndex(source) < 0);

  pFilter->SetPathFilter(sRelativeFolder);
  const auto iSourceRow = pModel->FindAssetIndex(source);
  if (!XII_TEST_BOOL(iSourceRow >= 0))
    return;
  const auto sourceIndex = pModel->index(iSourceRow, Model::NameColumn);
  XII_TEST_BOOL(pModel->setData(sourceIndex, QStringLiteral("Renamed Source"), Qt::EditRole));
  QApplication::processEvents();
  const auto iRenamedRow = pModel->FindAssetIndex(source);
  if (XII_TEST_BOOL(iRenamedRow >= 0))
  {
    xiiStringBuilder sRenamed(sFolder, "/Renamed Source.xiiResaveTest");
    XII_TEST_BOOL(pModel->index(iRenamedRow, Model::NameColumn).data(Model::AbsolutePath).toString() == xiiMakeQString(sRenamed));
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sRenamed) && !xiiOSFile::ExistsFile(sSource));
    xiiSet<xiiUuid> references;
    pCurator->FindAllUses(source, references, false);
    XII_TEST_BOOL(references.Contains(consumer));
  }

  auto checkDockLayout = [](ads::DockWidgetArea area) {
    ads::CDockManager manager;
    auto*             pCenter = new ads::CDockWidget(&manager, QStringLiteral("Center"));
    pCenter->setWidget(new QWidget);
    manager.addDockWidget(ads::CenterDockWidgetArea, pCenter);

    auto* pDock    = new ads::CDockWidget(&manager, QStringLiteral("Asset Browser"));
    auto* pBrowser = new xiiQtAssetBrowserWidget(nullptr);
    pDock->setWidget(pBrowser, ads::CDockWidget::ForceScrollArea);
    manager.resize(1300, 620);
    manager.show();

    QWidget* pResizeTarget = &manager;
    if (area == ads::NoDockWidgetArea)
    {
      auto* pFloating = manager.addDockWidgetFloating(pDock);
      pFloating->resize(1080, 510);
      pFloating->show();
      pResizeTarget = pFloating;
    }
    else
    {
      auto* pArea = manager.addDockWidget(area, pDock);
      manager.setSplitterSizes(pArea, area == ads::BottomDockWidgetArea ? QList<int>{210, 380} : QList<int>{950, 320});
    }
    QApplication::processEvents();

    const auto checkGeometry = [pBrowser]() {
      auto* pToolbar   = pBrowser->Toolbar;
      auto* pWorkspace = pBrowser->splitter;
      auto* pStatus    = pBrowser->StatusBar;
      auto* pInspector = pBrowser->findChild<QWidget*>(QStringLiteral("AssetInspector"));
      XII_TEST_BOOL(pBrowser->isVisible());
      XII_TEST_INT(pToolbar->y(), 0);
      XII_TEST_INT(pToolbar->height(), pToolbar->sizeHint().height());
      XII_TEST_INT(pWorkspace->y(), pToolbar->geometry().bottom() + 1);
      XII_TEST_INT(pStatus->y(), pWorkspace->geometry().bottom() + 1);
      XII_TEST_INT(pStatus->height(), pStatus->sizeHint().height());
      XII_TEST_INT(pStatus->geometry().bottom() + 1, pBrowser->height());
      XII_TEST_INT(pBrowser->WorkspaceHeader->height(), pBrowser->WorkspaceHeader->sizeHint().height());
      XII_TEST_INT(pBrowser->AssetViews->geometry().bottom() + 1, pBrowser->Workspace->height());
      for (auto* pPane : {pBrowser->Sidebar, pBrowser->Workspace, pInspector})
      {
        if (pPane && pPane->isVisible())
        {
          XII_TEST_INT(pPane->y(), 0);
          XII_TEST_INT(pPane->height(), pWorkspace->height());
        }
      }
    };
    checkGeometry();
    const int iInitialHeight          = pBrowser->height();
    const int iInitialWorkspaceHeight = pBrowser->splitter->height();
    pResizeTarget->resize(pResizeTarget->width(), pResizeTarget->height() + 190);
    QApplication::processEvents();
    checkGeometry();
    XII_TEST_BOOL(pBrowser->height() > iInitialHeight);
    XII_TEST_INT(pBrowser->splitter->height() - iInitialWorkspaceHeight, pBrowser->height() - iInitialHeight);
  };

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bottom dock height")
  {
    checkDockLayout(ads::BottomDockWidgetArea);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Side dock height")
  {
    checkDockLayout(ads::LeftDockWidgetArea);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Floating dock height")
  {
    checkDockLayout(ads::NoDockWidgetArea);
  }
}
