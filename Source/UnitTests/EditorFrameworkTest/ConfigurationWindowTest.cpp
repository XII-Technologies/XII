/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorFramework/Dialogs/ConfigurationDlg.moc.h>
#include <EditorFramework/Dialogs/InputConfigPage.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Types/ScopeExit.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Dialogs/ShortcutEditorPage.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QApplication>
#include <QFile>
#include <QKeySequenceEdit>
#include <QTreeWidgetItemIterator>
#include <ToolsFoundation/Application/ApplicationServices.h>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Windows/IncludeWindows.h>
#  include <Foundation/Strings/StringConversion.h>
#endif
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

namespace
{
  QTreeWidgetItem* FindItem(QTreeWidget* pTree, const QString& name)
  {
    for (QTreeWidgetItemIterator it(pTree); *it; ++it)
      if ((*it)->text(0) == name)
        return *it;
    return nullptr;
  }

  QByteArray ReadFile(const QString& path)
  {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
  }

  const xiiActionMap::TreeNode* FindAction(const xiiActionMap::TreeNode* pRoot, xiiStringView name)
  {
    if (auto* pDescriptor = xiiActionManager::GetActionDescriptor(pRoot->m_Data.m_hAction); pDescriptor && pDescriptor->m_sActionName == name)
      return pRoot;
    for (auto* pChild : pRoot->GetChildren())
      if (auto* pFound = FindAction(pChild, name))
        return pFound;
    return nullptr;
  }
} // namespace

void TestConfigurationNavigation(xiiStringView sProjectDirectory)
{
  using Section              = xiiQtConfigurationDlg::Section;
  auto*       pEditor        = xiiQtEditorApp::GetSingleton();
  const auto* pActionTree    = xiiActionMapManager::GetActionMap("SettingsTabMenuBar")->BuildActionTree();
  const auto* pEdit          = FindAction(pActionTree, "G.Edit");
  const auto* pConfiguration = FindAction(pActionTree, "G.Edit.Configuration");
  if (XII_TEST_BOOL(pEdit && pConfiguration))
  {
    XII_TEST_BOOL(pConfiguration->GetParent() == pEdit);
    XII_TEST_INT(pConfiguration->GetChildren().GetCount(), 3);
    const char* names[] = {"Project.Settings", "Editor.Preferences", "Project.PluginSelection"};
    for (xiiUInt32 i = 0; i < 3; ++i)
      XII_TEST_STRING(xiiActionManager::GetActionDescriptor(pConfiguration->GetChildren()[i]->m_Data.m_hAction)->m_sActionName, names[i]);
  }
  XII_TEST_BOOL(FindAction(pActionTree, "G.Project.Settings") == nullptr);
  XII_TEST_BOOL(FindAction(pActionTree, "Project.InputConfig") == nullptr);
  XII_TEST_BOOL(FindAction(pActionTree, "Editor.Shortcuts") == nullptr);

  const QString projectPath  = xiiMakeQString(sProjectDirectory);
  const QString inputPath    = projectPath + QStringLiteral("/RuntimeConfigs/InputConfig.ddl");
  const QString windowPath   = projectPath + QStringLiteral("/RuntimeConfigs/Window.ddl");
  const auto    inputBefore  = ReadFile(inputPath);
  const auto    windowBefore = ReadFile(windowPath);
  {
    auto*                 pUser    = xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>();
    const float           previous = pUser->m_fTranslationSnapValue;
    xiiQtConfigurationDlg dialog(nullptr);
    auto*                 pDraft = const_cast<xiiDocument*>(dialog.Properties->GetDocument());
    for (const auto* pObject : pDraft->GetObjectManager()->GetRootObject()->GetChildren())
    {
      if (pObject->GetType() != xiiGetStaticRTTI<xiiEditorPreferencesUser>())
        continue;
      auto* pAccessor = pDraft->GetObjectAccessor();
      pAccessor->StartTransaction("Stage snapping preference");
      XII_TEST_STATUS(pAccessor->SetValueByName(pObject, "TranslationSnap", 0.75f));
      pAccessor->FinishTransaction();
      XII_TEST_FLOAT(xiiSnapProvider::GetTranslationSnapValue(), previous, 0.0001f);
      dialog.ButtonCancel->click();
      XII_TEST_FLOAT(pUser->m_fTranslationSnapValue, previous, 0.0001f);
      break;
    }
  }
  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, Section::ProjectSettings);
    XII_TEST_INT(dialog.Tree->topLevelItemCount(), 3);
    XII_TEST_STRING(dialog.Tree->topLevelItem(0)->text(0).toUtf8().constData(), "Project Settings");
    XII_TEST_STRING(dialog.Tree->topLevelItem(1)->text(0).toUtf8().constData(), "Editor Preferences");
    XII_TEST_STRING(dialog.Tree->topLevelItem(2)->text(0).toUtf8().constData(), "Plugins");
    dialog.SelectSection(Section::ProjectSettings, "Window Configuration");
    XII_TEST_BOOL(ReadFile(windowPath) == windowBefore);
    dialog.SelectSection(Section::ProjectSettings, "Input Actions");
    auto* pInput = dialog.findChild<xiiQtInputConfigPage*>();
    if (XII_TEST_BOOL(pInput && pInput->TreeActions->topLevelItemCount() > 0))
    {
      auto* pSet = pInput->TreeActions->topLevelItem(0);
      if (XII_TEST_BOOL(pSet->childCount() > 0))
        pSet->child(0)->setText(0, QStringLiteral("CancelledConfigurationAction"));
    }
    dialog.SelectSection(Section::EditorPreferences);
    dialog.SelectSection(Section::ProjectSettings, "Input Actions");
    XII_TEST_BOOL(dialog.findChild<xiiQtInputConfigPage*>() == pInput);
    dialog.ButtonCancel->click();
    XII_TEST_BOOL(ReadFile(inputPath) == inputBefore);
    XII_TEST_BOOL(ReadFile(windowPath) == windowBefore);
  }
  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, Section::ProjectSettings);
    dialog.SelectSection(Section::ProjectSettings, "Input Actions");
    auto* pInput = dialog.findChild<xiiQtInputConfigPage*>();
    auto* pSet   = pInput->TreeActions->topLevelItem(0);
    if (XII_TEST_BOOL(pSet->childCount() > 0))
    {
      pSet->child(0)->setText(0, QStringLiteral("AppliedConfigurationAction"));
      XII_TEST_RESULT(dialog.ApplyAllChanges());
      XII_TEST_BOOL(ReadFile(inputPath).contains("AppliedConfigurationAction"));
      pSet->child(0)->setText(0, QStringLiteral("DiscardedAfterApply"));
      dialog.ButtonCancel->click();
      XII_TEST_BOOL(!ReadFile(inputPath).contains("DiscardedAfterApply"));
    }
  }

  // Keyboard bindings stay local until the common Apply operation.
  const auto hAction     = xiiActionManager::GetActionHandle("Editor", "Editor.FindAssets");
  auto*      pDescriptor = const_cast<xiiActionDescriptor*>(xiiActionManager::GetActionDescriptor(hAction));
  if (XII_TEST_BOOL(pDescriptor != nullptr))
  {
    const xiiString previous = pDescriptor->m_sShortcut;
    XII_SCOPE_EXIT(pDescriptor->m_sShortcut = previous; pDescriptor->UpdateExistingActions(); xiiActionManager::SaveShortcutAssignment());
    xiiQtConfigurationDlg dialog(nullptr);
    dialog.SelectSection(Section::EditorPreferences, "Keyboard Shortcuts");
    auto* pShortcuts = dialog.findChild<xiiQtShortcutEditorPage*>();
    auto* pItem      = FindItem(pShortcuts->Shortcuts, QStringLiteral("Editor.FindAssets"));
    if (XII_TEST_BOOL(pItem != nullptr))
    {
      pShortcuts->Shortcuts->setCurrentItem(pItem);
      pShortcuts->KeyEditor->setKeySequence(QKeySequence(QStringLiteral("Ctrl+Alt+9")));
      pShortcuts->ButtonAssign->click();
      XII_TEST_STRING(pDescriptor->m_sShortcut, previous);
#if XII_ENABLED(XII_PLATFORM_WINDOWS)
      xiiActionManager::SaveShortcutAssignment();
      const xiiStringBuilder path(xiiApplicationServices::GetSingleton()->GetApplicationPreferencesFolder(), "/Settings/Shortcuts.ddl");
      {
        const HANDLE file = CreateFileW(xiiStringWChar(path).GetData(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (XII_TEST_BOOL(file != INVALID_HANDLE_VALUE))
        {
          XII_SCOPE_EXIT(CloseHandle(file));
          XII_TEST_BOOL(dialog.ApplyAllChanges().Failed());
          XII_TEST_STRING(pDescriptor->m_sShortcut, previous);
        }
      }
#endif
      XII_TEST_RESULT(dialog.ApplyAllChanges());
      XII_TEST_BOOL(pDescriptor->m_sShortcut != previous);
    }
  }

  // One modeless window is reused by all three real action entry points.
  xiiActionContext context;
  XII_TEST_RESULT(xiiActionManager::ExecuteAction("Project", "Project.Settings", context));
  QPointer<xiiQtConfigurationDlg> pShared;
  for (auto* pWidget : QApplication::topLevelWidgets())
    if (auto* pDialog = qobject_cast<xiiQtConfigurationDlg*>(pWidget); pDialog && pDialog->isVisible())
      pShared = pDialog;
  if (XII_TEST_BOOL(pShared != nullptr))
  {
    XII_TEST_BOOL(!pShared->isModal());
    XII_TEST_RESULT(xiiActionManager::ExecuteAction("Editor", "Editor.Preferences", context));
    XII_TEST_RESULT(xiiActionManager::ExecuteAction("Project", "Project.PluginSelection", context));
    XII_TEST_STRING(pShared->Heading->text().toUtf8().constData(), "Plugins");
    QApplication::processEvents();
    XII_TEST_BOOL(pShared->grab().save(projectPath + QStringLiteral("/ConfigurationPlugins.png")));
    pShared->close();
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    XII_TEST_BOOL(pShared == nullptr);
  }

  // Availability is a snapshot; editing it never loads arbitrary DLLs.
  auto original = pEditor->GetPluginBundles();
  XII_SCOPE_EXIT(pEditor->GetPluginBundles() = original);
  auto& bundle          = pEditor->GetPluginBundles().m_Plugins["ConfigurationTest"];
  bundle.m_sDisplayName = "Configuration Test Plugin";
  bundle.m_sDescription = "Editor-only metadata fixture";
  bundle.m_EditorPlugins.PushBack("MetadataFixture");
  const QString pluginsPath   = projectPath + QStringLiteral("/Editor/PluginSelection.ddl");
  const auto    pluginsBefore = ReadFile(pluginsPath);
  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, Section::Plugins);
    auto*                 pPlugins = dialog.findChild<xiiQtPluginSelectionWidget*>();
    auto                  items    = pPlugins->PluginsList->findItems(QStringLiteral("Configuration Test Plugin"), Qt::MatchExactly);
    if (XII_TEST_INT(items.count(), 1))
      items[0]->setCheckState(Qt::Checked);
    dialog.ButtonCancel->click();
    XII_TEST_BOOL(!pEditor->GetPluginBundles().m_Plugins["ConfigurationTest"].m_bSelected);
    XII_TEST_BOOL(ReadFile(pluginsPath) == pluginsBefore);
  }
  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, Section::Plugins);
    auto*                 pPlugins = dialog.findChild<xiiQtPluginSelectionWidget*>();
    auto                  items    = pPlugins->PluginsList->findItems(QStringLiteral("Configuration Test Plugin"), Qt::MatchExactly);
    if (XII_TEST_INT(items.count(), 1))
      items[0]->setCheckState(Qt::Checked);
    XII_TEST_RESULT(dialog.ApplyAllChanges());
    XII_TEST_BOOL(pEditor->GetPluginBundles().m_Plugins["ConfigurationTest"].m_bSelected);
    XII_TEST_BOOL(ReadFile(pluginsPath).contains("ConfigurationTest"));
    XII_TEST_BOOL(pEditor->GetRestartRequiredReasons().Find("The set of active plugins has changed.").IsValid());
  }
  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, Section::ProjectSettings);
    dialog.SelectSection(Section::ProjectSettings, "Platform and Asset Profiles");
    QPointer<QWidget> pPage = dialog.Pages->currentWidget();
    xiiPlugin::BeginPluginChanges();
    XII_TEST_BOOL(dialog.Properties == nullptr);
    XII_TEST_BOOL(pPage == nullptr);
    xiiPlugin::EndPluginChanges();
  }
}

XII_CREATE_SIMPLE_TEST_GROUP(Configuration);
XII_CREATE_SIMPLE_TEST(Configuration, PluginDependenciesAndFiltering)
{
  xiiPluginBundleSet bundles;
  auto&              a = bundles.m_Plugins["A"];
  a.m_sDisplayName     = "Editor Physics";
  a.m_sDescription     = "Simulation tools";
  a.m_EditorPlugins.PushBack("EditorPhysics");
  a.m_bSelected = true;
  a.m_RequiredBundles.PushBack("B");
  auto& b          = bundles.m_Plugins["B"];
  b.m_sDisplayName = "Runtime Physics";
  b.m_RuntimePlugins.PushBack("Physics");
  b.m_RequiredBundles.PushBack("A");
  xiiPluginBundleSet         empty;
  xiiQtPluginSelectionWidget widget(nullptr);
  widget.SetPluginSet(&bundles);
  widget.SetPluginSet(&bundles);
  XII_TEST_INT(widget.PluginsList->count(), 2);
  xiiStringBuilder reason;
  XII_TEST_RESULT(widget.ValidateSelection(reason));
  widget.Filter(QStringLiteral("Simulation"));
  XII_TEST_BOOL(!widget.PluginsList->item(0)->isHidden());
  XII_TEST_BOOL(widget.PluginsList->item(1)->isHidden());
  b.m_bMissing = true;
  XII_TEST_BOOL(widget.ValidateSelection(reason).Failed());
  b.m_bMissing = false;
  a.m_RequiredBundles.PushBack("Unavailable");
  XII_TEST_BOOL(widget.ValidateSelection(reason).Failed());
  a.m_RequiredBundles.PopBack();
  a.m_ExclusiveFeatures.PushBack("Physics");
  b.m_ExclusiveFeatures.PushBack("Physics");
  XII_TEST_BOOL(widget.ValidateSelection(reason).Failed());
  widget.SetReadOnly(true);
  XII_TEST_BOOL(!widget.PluginsList->item(0)->flags().testFlag(Qt::ItemIsUserCheckable));
  widget.PluginsList->setCurrentRow(0);
  XII_TEST_BOOL(!widget.DescriptionText->toPlainText().isEmpty());
  widget.Filter(QStringLiteral("no matching plugin"));
  XII_TEST_BOOL(widget.PluginsList->currentItem() == nullptr);
  XII_TEST_BOOL(widget.DescriptionText->toPlainText().isEmpty());
  XII_TEST_BOOL(!widget.LoadCopy->isEnabled());
  widget.SetPluginSet(&empty);
  widget.SetReadOnly(true);
  widget.Filter(QStringLiteral("unavailable"));
  XII_TEST_INT(widget.PluginsList->count(), 0);
  XII_TEST_RESULT(widget.ValidateSelection(reason));
  const xiiStringView      ddl = "PluginState { string %ID { \"OtherProject\" } bool %Selected { true } }";
  xiiRawMemoryStreamReader stream(ddl.GetStartPointer(), ddl.GetElementCount());
  xiiOpenDdlReader         reader;
  XII_TEST_RESULT(reader.ParseDocument(stream));
  a.m_bSelected = true;
  a.m_bLoadCopy = true;
  a.ReadStateFromDDL(reader, "A");
  XII_TEST_BOOL(!a.m_bSelected && !a.m_bLoadCopy);
}
