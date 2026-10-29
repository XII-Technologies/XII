/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>
#include <EditorFrameworkTest/EditorWorkspaceTest.h>

#include <EditorFramework/Assets/AssetBrowserFilter.moc.h>
#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Panels/AssetBrowserPanel/AssetBrowserPanel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/ActionViews/QtProxy.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <GuiFoundation/Models/LogModel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/CollapsibleGroupBox.moc.h>
#include <GuiFoundation/Widgets/CommandPalette.moc.h>
#include <GuiFoundation/Widgets/LogWidget.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <QApplication>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QScreen>
#include <QScrollBar>
#include <QToolButton>
#include <QTreeWidget>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ads/AutoHideDockContainer.h>

void TestEditorWorkspace(xiiSceneDocument& scene, xiiQtEngineDocumentWindow& window, xiiQtEngineViewWidget& view, const xiiUuid& selectedObject, xiiStringView sOutputDirectory)
{
  const auto* pPreviousBlock = xiiTestFramework::s_szTestBlockName;
  XII_SCOPE_EXIT(xiiTestFramework::s_szTestBlockName = pPreviousBlock);
  auto*       pContainer = window.GetContainerWindow();
  auto*       pAccessor  = scene.GetObjectAccessor();
  const auto* pSelected  = pAccessor->GetObject(selectedObject);
  scene.GetSelectionManager()->SetSelection(pSelected);
  QApplication::processEvents();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Outliner search, Inspector navigation and native undo command")
  {
    auto* pOutliner = window.findChild<xiiQtGameObjectWidget*>();
    if (XII_TEST_BOOL(pOutliner != nullptr))
    {
      pOutliner->GetFilterWidget().setText(QStringLiteral("does not match any object"));
      XII_TEST_BOOL(scene.GetSelectionManager()->GetCurrentObject() == pSelected);
      pOutliner->GetFilterWidget().setText(QString());
      XII_TEST_BOOL(scene.GetSelectionManager()->GetCurrentObject() == pSelected);
    }
    auto* pGrid = window.findChild<xiiQtPropertyGridWidget*>();
    if (XII_TEST_BOOL(pGrid != nullptr))
    {
      pGrid->SetSelection(scene.GetSelectionManager()->GetSelection());
      QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
      auto*      pSearch     = pGrid->findChild<xiiQtSearchWidget*>();
      const auto uiUndoCount = scene.GetCommandHistory()->GetUndoStackSize();
      pSearch->setText(QStringLiteral("LocalPosition"));
      bool bFound = false;
      for (auto* pLabel : pGrid->findChildren<QLabel*>())
        bFound |= pLabel->text().contains(QLatin1Char('/'));
      XII_TEST_BOOL(bFound);
      XII_TEST_INT(scene.GetCommandHistory()->GetUndoStackSize(), uiUndoCount);
      pSearch->setText(QString());
      const auto groups = pGrid->findChildren<xiiQtCollapsibleGroupBox*>();
      if (XII_TEST_BOOL(!groups.isEmpty()))
      {
        auto* pGroup = groups.front();
        pGroup->SetCollapseState(true);
        QKeyEvent space(QEvent::KeyPress, Qt::Key_Space, Qt::NoModifier);
        QApplication::sendEvent(pGroup->GetHeader(), &space);
        XII_TEST_BOOL(!pGroup->GetCollapseState());
      }
      bool bEditedName = false;
      for (auto* pProperty : pGrid->findChildren<xiiQtPropertyWidget*>())
      {
        if (!pProperty->GetProperty() || pProperty->GetProperty()->GetPropertyName() != "Name")
          continue;
        auto* pEdit = pProperty->findChild<QLineEdit*>();
        if (!pEdit)
          continue;
        bEditedName = true;
        xiiVariant oldName;
        XII_TEST_STATUS(pAccessor->GetValueByName(pSelected, "Name", oldName));
        pEdit->setText(QStringLiteral("Workspace acceptance object"));
        XII_TEST_BOOL(QMetaObject::invokeMethod(pEdit, "editingFinished", Qt::DirectConnection));
        xiiVariant newName;
        XII_TEST_STATUS(pAccessor->GetValueByName(pSelected, "Name", newName));
        XII_TEST_STRING(newName.Get<xiiString>(), "Workspace acceptance object");
        const auto actions = xiiQtProxy::GetContextActions(&scene, &view);
        const auto globals = xiiQtProxy::GetContextActions(nullptr, nullptr);
        QAction*   pUndo   = nullptr;
        for (auto* pAction : actions)
          if (pAction->shortcut() == QKeySequence::Undo)
            pUndo = pAction;
        if (XII_TEST_BOOL(pUndo != nullptr))
        {
          XII_TEST_BOOL(!globals.contains(pUndo));
          XII_TEST_BOOL(pUndo->isEnabled());
          pUndo->trigger();
          XII_TEST_STATUS(pAccessor->GetValueByName(pSelected, "Name", newName));
          XII_TEST_BOOL(newName == oldName);
        }
        break;
      }
      XII_TEST_BOOL(bEditedName);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Content history preserves search and constrained type filters")
  {
    auto* pPanel = xiiQtAssetBrowserPanel::GetSingleton();
    if (XII_TEST_BOOL(pPanel != nullptr))
    {
      auto*           pBrowser   = pPanel->AssetBrowserWidget;
      auto*           pFilter    = pBrowser->GetAssetBrowserFilter();
      const xiiString oldPath    = pFilter->GetPathFilter();
      const xiiString oldType    = pFilter->GetTypeFilter();
      const QString   oldSearch  = pBrowser->SearchWidget->text();
      const bool      bWasDrawer = pPanel->isAutoHide();
      if (!bWasDrawer)
        pPanel->setAutoHide(true, ads::SideBarBottom);
      if (auto* pDrawer = pPanel->autoHideDockContainer())
      {
        pDrawer->collapseView(true);
        pPanel->EnsureVisible();
        XII_TEST_BOOL(!pDrawer->isHidden());
      }
      pFilter->SetPathFilter("Base");
      pFilter->SetPathFilter("Base/Meshes");
      pBrowser->SearchWidget->setText(QStringLiteral("mesh"));
      auto* pNavigation = pBrowser->findChild<QToolBar*>(QStringLiteral("ContentNavigation"));
      if (XII_TEST_BOOL(pNavigation != nullptr))
      {
        pNavigation->actions()[0]->trigger();
        XII_TEST_STRING(pFilter->GetPathFilter(), "Base");
        pNavigation->actions()[1]->trigger();
        XII_TEST_STRING(pFilter->GetPathFilter(), "Base/Meshes");
        pNavigation->actions()[2]->trigger();
        XII_TEST_STRING(pFilter->GetPathFilter(), "Base");
        XII_TEST_BOOL(pBrowser->SearchWidget->text() == QStringLiteral("mesh"));
        XII_TEST_STRING(pFilter->GetTypeFilter(), oldType);
      }
      pBrowser->ButtonListMode->click();
      XII_TEST_BOOL(!pBrowser->GetAssetBrowserModel()->GetIconMode());
      pBrowser->ButtonIconMode->click();
      XII_TEST_BOOL(pBrowser->GetAssetBrowserModel()->GetIconMode());
      pFilter->SetPathFilter(oldPath);
      pBrowser->SearchWidget->setText(oldSearch);
      if (!bWasDrawer)
        pPanel->setAutoHide(false);
      else if (auto* pDrawer = pPanel->autoHideDockContainer())
        pDrawer->collapseView(true);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Named workspace round trip and safe-mode guard")
  {
    const bool bPrevious = xiiQtDocumentWindow::s_bAllowRestoreWindowLayout;
    XII_SCOPE_EXIT(xiiQtDocumentWindow::s_bAllowRestoreWindowLayout = bPrevious);
    xiiQtDocumentWindow::s_bAllowRestoreWindowLayout = true;
    XII_TEST_BOOL(pContainer->SaveWorkspace(QString()).Failed());
    XII_TEST_RESULT(pContainer->SaveWorkspace(QStringLiteral("Authoring / Test")));
    XII_TEST_BOOL(pContainer->GetWorkspaceNames().contains(QStringLiteral("Authoring / Test")));
    XII_TEST_RESULT(pContainer->RestoreWorkspace(QStringLiteral("Authoring / Test")));
    XII_TEST_BOOL(pContainer->RestoreWorkspace(QStringLiteral("Missing workspace")).Failed());
    xiiQtDocumentWindow::s_bAllowRestoreWindowLayout = false;
    XII_TEST_BOOL(pContainer->RestoreWorkspace(QStringLiteral("Authoring / Test")).Failed());
    XII_TEST_RESULT(pContainer->RemoveWorkspace(QStringLiteral("Authoring / Test")));
    XII_TEST_BOOL(!pContainer->GetWorkspaceNames().contains(QStringLiteral("Authoring / Test")));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Log filters and explicit following preserve selection")
  {
    xiiQtLogWidget log(nullptr);
    log.resize(700, 280);
    for (int i = 0; i < 120; ++i)
    {
      xiiLogEntry entry;
      entry.m_Type = i == 0 ? xiiLogMsgType::ErrorMsg : (i == 1 ? xiiLogMsgType::WarningMsg : xiiLogMsgType::InfoMsg);
      entry.m_sMsg = "Workspace diagnostic acceptance";
      log.GetLog()->AddLogMsg(entry);
    }
    QApplication::processEvents();
    auto* pErrors   = log.findChild<QToolButton*>(QStringLiteral("LogErrors"));
    auto* pWarnings = log.findChild<QToolButton*>(QStringLiteral("LogWarnings"));
    auto* pFollow   = log.findChild<QToolButton*>(QStringLiteral("LogFollow"));
    XII_TEST_INT(log.GetLog()->rowCount(), 120);
    pErrors->click();
    XII_TEST_INT(log.GetLog()->rowCount(), 1);
    pWarnings->click();
    XII_TEST_INT(log.GetLog()->rowCount(), 2);
    log.SetLogLevel(xiiLogMsgType::InfoMsg);
    log.ListViewLog->selectionModel()->select(log.GetLog()->index(0, 0), QItemSelectionModel::ClearAndSelect);
    log.ListViewLog->verticalScrollBar()->triggerAction(QAbstractSlider::SliderSingleStepSub);
    XII_TEST_BOOL(!pFollow->isChecked());
    xiiLogEntry entry;
    entry.m_Type = xiiLogMsgType::InfoMsg;
    entry.m_sMsg = "A later message";
    log.GetLog()->AddLogMsg(entry);
    QApplication::processEvents();
    XII_TEST_INT(log.ListViewLog->selectionModel()->selectedRows().size(), 1);
    pFollow->click();
    XII_TEST_BOOL(pFollow->isChecked());
    xiiStringBuilder capture(sOutputDirectory, "/EditorDiagnostics.png");
    XII_TEST_BOOL(log.grab().save(xiiMakeQString(capture)));
    log.GetLog()->Clear();
    XII_TEST_INT(log.GetLog()->GetNumErrors(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native workspace chrome and real command captures")
  {
    auto* pToolbar = window.findChild<xiiQtToolBarActionMapView*>(QStringLiteral("ViewportControls"));
    if (XII_TEST_BOOL(pToolbar != nullptr))
    {
      int iReadableMenus = 0;
      for (auto* pButton : pToolbar->findChildren<QToolButton*>())
        if (pButton->menu() && pButton->toolButtonStyle() == Qt::ToolButtonTextBesideIcon && !pButton->text().isEmpty())
          ++iReadableMenus;
      XII_TEST_BOOL(iReadableMenus >= 2);
    }
    // The renderer regression above uses a fixed 1280x720 surface. Release that
    // test-only constraint while inspecting normal responsive editor layouts.
    QWidget* pSurface = nullptr;
    for (auto* pChild : view.findChildren<QWidget*>())
      if (pChild->testAttribute(Qt::WA_PaintOnScreen) && pChild->testAttribute(Qt::WA_NativeWindow))
        pSurface = pChild;
    const auto oldResolution    = xiiQtEngineViewWidget::s_FixedResolution;
    const auto oldContainerSize = pContainer->size();
    const auto oldWindowState   = pContainer->windowState();
    const auto oldMinimum       = pSurface ? pSurface->minimumSize() : QSize();
    const auto oldMaximum       = pSurface ? pSurface->maximumSize() : QSize();
    XII_SCOPE_EXIT(
      xiiQtEngineViewWidget::s_FixedResolution = oldResolution;
      if (pSurface) { pSurface->setMinimumSize(oldMinimum); pSurface->setMaximumSize(oldMaximum); } pContainer->resize(oldContainerSize);
      pContainer->setWindowState(oldWindowState));
    xiiQtEngineViewWidget::s_FixedResolution = {};
    if (pSurface)
    {
      pSurface->setMinimumSize(64, 64);
      pSurface->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    pContainer->setWindowState(Qt::WindowNoState);
    const auto  available = QApplication::primaryScreen()->availableGeometry().size();
    const QSize wideSize(qMin(1280, qMax(800, available.width() - 32)), qMin(800, qMax(450, available.height() - 64)));
    pContainer->resize(wideSize);
    pContainer->layout()->activate();
    QApplication::processEvents();
    xiiStringBuilder capture(sOutputDirectory, "/EditorWorkspace.png");
    XII_TEST_BOOL(pContainer->grab().save(xiiMakeQString(capture)));
    pContainer->resize(qMin(960, wideSize.width()), qMin(640, wideSize.height()));
    pContainer->layout()->activate();
    QApplication::processEvents();
    capture.Set(sOutputDirectory, "/EditorWorkspaceCompact.png");
    XII_TEST_BOOL(pContainer->grab().save(xiiMakeQString(capture)));
    if (auto* pPanel = xiiQtAssetBrowserPanel::GetSingleton())
    {
      pPanel->EnsureVisible();
      QApplication::processEvents();
      capture.Set(sOutputDirectory, "/EditorContent.png");
      XII_TEST_BOOL(pPanel->widget()->grab().save(xiiMakeQString(capture)));
      if (auto* pDrawer = pPanel->autoHideDockContainer())
        pDrawer->collapseView(true);
    }
    xiiQtCommandPalette palette(pContainer, xiiQtProxy::GetContextActions(&scene, &view));
    palette.findChild<QLineEdit*>()->setText(QStringLiteral("selection"));
    capture.Set(sOutputDirectory, "/EditorCommands.png");
    XII_TEST_BOOL(palette.grab().save(xiiMakeQString(capture)));
  }
}
