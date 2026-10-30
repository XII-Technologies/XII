/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <EditorFramework/Dialogs/AssetProfilesPage.moc.h>
#include <EditorFramework/Dialogs/ConfigurationDlg.moc.h>
#include <EditorFramework/Dialogs/PluginSelectionWidget.moc.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Settings/EditorPreferences.h>
#include <EditorFramework/Settings/ScenePreferences.h>
#include <Foundation/Application/Application.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>
#include <GameEngine/Configuration/GameApplicationSettings.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableTypeMenu.moc.h>
#include <QApplication>
#include <QPixmap>
#include <QTreeWidgetItemIterator>
#include <TestFramework/Utilities/TestLogInterface.h>
#include <ToolsFoundation/Application/ApplicationServices.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>
#include <ToolsFoundation/Project/ToolsProject.h>

class xiiEditorTestSettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorTestSettings, xiiSettings);

public:
  xiiInt32 m_iValue   = 3;
  bool     m_bRestart = false;
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorTestSettings, 1, xiiRTTIDefaultAllocator<xiiEditorTestSettings>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Application),
    new xiiCategoryAttribute("Diagnostics/Settings Test"),
    new xiiTitleAttribute("Test Controls"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(new xiiDefaultValueAttribute(3), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("Restart", m_bRestart)->AddAttributes(new xiiSettingsRestartAttribute()),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiEditorTestDocumentSettings : public xiiEditorTestSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorTestDocumentSettings, xiiEditorTestSettings);
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorTestDocumentSettings, 1, xiiRTTIDefaultAllocator<xiiEditorTestDocumentSettings>)
  {
    XII_BEGIN_ATTRIBUTES
    {
      new xiiSettingsScopeAttribute(xiiSettingsScope::Document),
    } XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiEditorTestProjectSettings : public xiiEditorTestSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEditorTestProjectSettings, xiiEditorTestSettings);
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiEditorTestProjectSettings, 1, xiiRTTIDefaultAllocator<xiiEditorTestProjectSettings>)
  {
    XII_BEGIN_ATTRIBUTES
    {
      new xiiSettingsScopeAttribute(xiiSettingsScope::Project),
    } XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  class SettingsTestDocument : public xiiDocument
  {
  public:
    explicit SettingsTestDocument(xiiStringView path) : xiiDocument(path, XII_DEFAULT_NEW(xiiDocumentObjectManager))
    {
      m_pDocumentInfo = CreateDocumentInfo();
    }
    xiiDocumentInfo* CreateDocumentInfo() override { return XII_DEFAULT_NEW(xiiDocumentInfo); }
  };

  xiiDocumentObject* FindSettingsObject(xiiQtConfigurationDlg& dialog, const xiiRTTI* pType)
  {
    auto* pDocument = const_cast<xiiDocument*>(dialog.Properties->GetDocument());
    for (const auto* pObject : pDocument->GetObjectManager()->GetRootObject()->GetChildren())
      if (pObject->GetType() == pType)
        return const_cast<xiiDocumentObject*>(pObject);
    return nullptr;
  }

  void EditValue(xiiQtConfigurationDlg& dialog, xiiInt32 iValue)
  {
    auto* pObject = FindSettingsObject(dialog, xiiGetStaticRTTI<xiiEditorTestSettings>());
    if (!XII_TEST_BOOL(pObject != nullptr))
      return;
    auto* pDocument = const_cast<xiiDocument*>(dialog.Properties->GetDocument());
    auto* pAccessor = pDocument->GetObjectAccessor();
    pAccessor->StartTransaction("Edit settings");
    XII_TEST_STATUS(pAccessor->SetValueByName(pObject, "Value", iValue));
    pAccessor->FinishTransaction();
    pDocument->GetSelectionManager()->SetSelection(pObject);
  }
} // namespace

XII_CREATE_SIMPLE_TEST_GROUP(EditorSettings);
void TestConfigurationNavigation(xiiStringView sProjectDirectory);
XII_CREATE_SIMPLE_TEST(EditorSettings, DiscoveryEditingAndPersistence)
{
  auto*           pEditor      = xiiQtEditorApp::GetSingleton();
  auto*           pApplication = xiiApplication::GetApplicationInstance();
  const xiiString previousName = pApplication->GetApplicationName();
  pApplication->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(pApplication->SetApplicationName(previousName));
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/UnifiedSettings");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(root));
  pEditor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, root);
  XII_SCOPE_EXIT(pEditor->ShutdownEditor());

  {
    xiiQtConfigurationDlg dialog(nullptr, nullptr, xiiQtConfigurationDlg::Section::Plugins);
    auto*                 pPlugins = dialog.findChild<xiiQtPluginSelectionWidget*>();
    XII_TEST_BOOL(pPlugins != nullptr && !pPlugins->Template->isEnabled());
    XII_TEST_INT(dialog.Tree->topLevelItem(0)->childCount(), 0);
    dialog.ButtonCancel->click();
  }

  xiiDynamicArray<xiiEditorSettings::Settings> discovered;
  xiiEditorSettings::GatherSettings(discovered);
  bool bFound = false;
  for (const auto& entry : discovered)
  {
    bFound |= entry.m_pSettings->GetDynamicRTTI() == xiiGetStaticRTTI<xiiEditorTestSettings>();
    XII_TEST_BOOL(entry.m_Domain == xiiEditorSettings::Domain::Application);
  }
  XII_TEST_BOOL(bFound);
  XII_TEST_BOOL(xiiEditorSettings::QuerySettings<xiiEditorTestProjectSettings>() == nullptr);
  XII_TEST_BOOL(xiiEditorSettings::QuerySettings<xiiEditorTestDocumentSettings>() == nullptr);
  auto* pLive                                        = xiiEditorSettings::QuerySettings<xiiEditorTestSettings>();
  pLive->m_iValue                                    = 3;
  xiiUInt32                            notifications = 0;
  xiiEvent<xiiSettings*>::Unsubscriber subscription;
  pLive->m_SettingsChangedEvent.AddEventHandler([&](xiiSettings*) { ++notifications; }, subscription);

  {
    xiiQtConfigurationDlg dialog(nullptr);
    for (QTreeWidgetItemIterator it(dialog.Tree); *it; ++it)
      if ((*it)->text(0) == QStringLiteral("General"))
        dialog.Tree->setCurrentItem(*it);
    dialog.show();
    QApplication::processEvents();
    XII_TEST_BOOL(dialog.grab().save(QString::fromUtf8(xiiStringBuilder(root, "/SettingsDialog.png").GetData())));
    XII_TEST_INT(notifications, 0);
    EditValue(dialog, 8);
    XII_TEST_INT(pLive->m_iValue, 3);
    XII_TEST_INT(notifications, 0);
    dialog.ButtonCancel->click();
    XII_TEST_INT(pLive->m_iValue, 3);
  }
  {
    xiiQtConfigurationDlg dialog(nullptr);
    EditValue(dialog, 7);
    dialog.ButtonApply->click();
    XII_TEST_BOOL(xiiQtTypeMenu::s_pRecentList == &xiiEditorSettings::QuerySettings<xiiEditorPreferencesUser>()->m_RecentlyCreatedTypes);
    XII_TEST_INT(pLive->m_iValue, 7);
    XII_TEST_INT(notifications, 1);
    XII_TEST_RESULT(dialog.ApplyAllChanges());
    XII_TEST_INT(notifications, 1);
    EditValue(dialog, 8);
    dialog.ButtonCancel->click();
    XII_TEST_INT(pLive->m_iValue, 7);
    XII_TEST_INT(notifications, 1);
    dialog.Search->setText(QStringLiteral("Diagnostics Value"));
    int visibleLeaves = 0;
    for (QTreeWidgetItemIterator it(dialog.Tree); *it; ++it)
      visibleLeaves += !(*it)->isHidden() && (*it)->childCount() == 0;
    XII_TEST_INT(visibleLeaves, 1);
    dialog.Search->setText(QStringLiteral("no matching category"));
    for (int i = 0; i < dialog.Tree->topLevelItemCount(); ++i)
      XII_TEST_BOOL(dialog.Tree->topLevelItem(i)->isHidden());
  }
  {
    xiiQtConfigurationDlg dialog(nullptr);
    EditValue(dialog, 99);
    XII_TEST_BOOL(dialog.ApplyAllChanges().Failed());
    XII_TEST_INT(pLive->m_iValue, 7);
    XII_TEST_INT(notifications, 1);
    for (QTreeWidgetItemIterator it(dialog.Tree); *it; ++it)
      if ((*it)->text(0) == QStringLiteral("Test Controls"))
        dialog.Tree->setCurrentItem(*it);
    dialog.ButtonReset->click();
    const auto* pObject = FindSettingsObject(dialog, xiiGetStaticRTTI<xiiEditorTestSettings>());
    XII_TEST_INT(pObject->GetTypeAccessor().GetValue("Value").Get<xiiInt32>(), 3);
    XII_TEST_INT(pLive->m_iValue, 7);
    dialog.ButtonCancel->click();
    XII_TEST_INT(pLive->m_iValue, 7);
  }
  subscription.Unsubscribe();
  {
    xiiQtConfigurationDlg dialog(nullptr);
    dialog.ButtonOk->click();
    XII_TEST_INT(dialog.result(), QDialog::Accepted);
    XII_TEST_INT(pLive->m_iValue, 7);
  }
  {
    xiiQtConfigurationDlg dialog(nullptr);
    xiiPlugin::BeginPluginChanges();
    XII_TEST_BOOL(dialog.Properties == nullptr);
    XII_TEST_BOOL(!dialog.ButtonApply->isEnabled());
    xiiPlugin::EndPluginChanges();
  }
  {
    xiiFileReader    file;
    xiiStringBuilder path(root, "/Settings_xiiEditorTestSettings.pref");
    XII_TEST_RESULT(file.Open(path));
    xiiEditorTestSettings restored;
    XII_TEST_RESULT(restored.ReadFromDDL(file));
    XII_TEST_INT(restored.m_iValue, 7);
  }

  xiiStringBuilder project(root, "/Project/xiiProject");
  if (!XII_TEST_RESULT(pEditor->CreateOrOpenProject(true, project)))
    return;
  {
    xiiEditorTestProjectSettings previous;
    previous.m_iValue = 6;
    const xiiStringBuilder oldPath(xiiApplicationServices::GetSingleton()->GetProjectPreferencesFolder(), "/Settings_xiiEditorTestProjectSettings.pref");
    {
      xiiFileWriter file;
      XII_TEST_RESULT(file.Open(oldPath));
      xiiReflectionSerializer::WriteObjectToDDL(file, previous.GetDynamicRTTI(), &previous);
    }
    auto* pShared = xiiEditorSettings::QuerySettings<xiiEditorTestProjectSettings>();
    XII_TEST_INT(pShared->m_iValue, 6);
    XII_TEST_RESULT(xiiEditorSettings::SaveSettings(pShared));
    const xiiStringBuilder sharedPath(project.GetFileDirectory(), "/Editor/Settings/Settings_xiiEditorTestProjectSettings.pref");
    XII_TEST_BOOL(xiiOSFile::ExistsFile(sharedPath));
    XII_TEST_BOOL(xiiOSFile::ExistsFile(oldPath));
  }
  TestConfigurationNavigation(project.GetFileDirectory());
  {
    xiiQtAssetProfilesPage dialog(nullptr);
    xiiPlugin::BeginPluginChanges();
    XII_TEST_BOOL(dialog.Tree == nullptr && dialog.Properties == nullptr);
    xiiPlugin::EndPluginChanges();
  }
  SettingsTestDocument first(xiiStringBuilder(root, "/First.asset"));
  SettingsTestDocument second(xiiStringBuilder(root, "/Second.asset"));
  auto*                pFirst  = xiiEditorSettings::QuerySettings<xiiEditorTestDocumentSettings>(&first);
  auto*                pSecond = xiiEditorSettings::QuerySettings<xiiEditorTestDocumentSettings>(&second);
  XII_TEST_BOOL(pFirst != nullptr && pSecond != nullptr && pFirst != pSecond);
  pFirst->m_iValue = 9;
  XII_TEST_INT(pSecond->m_iValue, 3);
  XII_TEST_RESULT(xiiEditorSettings::SaveSettings(pFirst));
  xiiEditorSettings::ClearDocumentSettings(&first);
  pFirst = xiiEditorSettings::QuerySettings<xiiEditorTestDocumentSettings>(&first);
  XII_TEST_INT(pFirst->m_iValue, 9);
  xiiEditorSettings::GatherSettings(discovered, &first);
  for (const auto& entry : discovered)
  {
    XII_TEST_BOOL(entry.m_pDocument == nullptr || entry.m_pDocument == &first);
    XII_TEST_BOOL(entry.m_pSettings->GetDynamicRTTI() != xiiGetStaticRTTI<xiiScenePreferencesUser>());
  }
  auto* pProject = xiiEditorSettings::QuerySettings<xiiEditorTestProjectSettings>();
  XII_TEST_BOOL(pProject != nullptr);
  pProject->m_iValue = 4;
  {
    xiiQtConfigurationDlg dialog(nullptr, &first);
    xiiEditorSettings::ClearDocumentSettings(&second);
    XII_TEST_BOOL(dialog.Properties != nullptr);
    xiiEditorSettings::ClearDocumentSettings(&first);
    XII_TEST_BOOL(dialog.Properties == nullptr);
    XII_TEST_INT(dialog.Tree->topLevelItemCount(), 0);
    XII_TEST_BOOL(dialog.ApplyAllChanges().Failed());
  }
  xiiEditorSettings::ClearDocumentSettings(&second);
  {
    const xiiStringBuilder path(xiiApplicationServices::GetSingleton()->GetDocumentPreferencesFolder(&first), "/Settings_xiiEditorTestDocumentSettings.pref");
    {
      xiiFileWriter file;
      XII_TEST_RESULT(file.Open(path));
      xiiAbstractObjectGraph graph;
      xiiAbstractGraphDdlSerializer::Write(file, &graph);
    }
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log);
    log.ExpectMessage("Invalid settings", xiiLogMsgType::WarningMsg);
    auto* pRecovered = xiiEditorSettings::QuerySettings<xiiEditorTestDocumentSettings>(&first);
    XII_TEST_INT(pRecovered->m_iValue, 3);
    XII_TEST_BOOL(xiiEditorSettings::SaveSettings(pRecovered).Failed());
    xiiEditorSettings::ClearDocumentSettings(&first);
  }
  {
    xiiQtConfigurationDlg dialog(nullptr);
    dialog.SelectSection(xiiQtConfigurationDlg::Section::ProjectSettings, "Input Actions");
    QPointer<QWidget> pPage = dialog.Pages->currentWidget();
    xiiToolsProject::CloseProject();
    XII_TEST_BOOL(dialog.Properties == nullptr && pPage == nullptr);
  }
  xiiStringBuilder otherProject(root, "/OtherProject/xiiProject");
  // The editor deliberately launches a separate process for a second project. Exercise the
  // preferences owner's scope transition directly without launching another user-facing editor.
  if (XII_TEST_STATUS(xiiToolsProject::CreateProject(otherProject)))
  {
    pProject = xiiEditorSettings::QuerySettings<xiiEditorTestProjectSettings>();
    if (XII_TEST_BOOL(pProject != nullptr))
      XII_TEST_INT(pProject->m_iValue, 3);
    xiiToolsProject::CloseProject();
  }
}

XII_CREATE_SIMPLE_TEST(EditorSettings, LegacyPreferences)
{
  xiiScenePreferencesUser preferences;
  xiiAbstractObjectGraph  graph;
  auto*                   pRoot = graph.AddNode(xiiUuid::MakeUuid(), "xiiScenePreferencesUser", 1, "root");
  pRoot->AddProperty("ShowGrid", false);
  pRoot->AddProperty("CameraSpeed", 8);
  pRoot->AddProperty("RemovedLegacyOption", 123);
  XII_TEST_RESULT(preferences.ReadFromGraph(graph));
  XII_TEST_BOOL(!preferences.GetShowGrid());
  XII_TEST_INT(preferences.GetCameraSpeed(), 8);
  pRoot->ChangeProperty("CameraSpeed", 30);
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  xiiAbstractGraphDdlSerializer::Write(writer, &graph);
  xiiMemoryStreamReader reader(&storage);
  XII_TEST_RESULT(preferences.ReadFromDDL(reader));
  XII_TEST_INT(preferences.GetCameraSpeed(), 24);
  XII_TEST_BOOL(!preferences.GetShowGrid());
}

XII_CREATE_SIMPLE_TEST(EditorSettings, RuntimeCVarMigration)
{
  xiiStringBuilder root(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/SettingsMigration");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(root));
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(root, "SettingsMigration", "settings-migration", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("SettingsMigration"));
  const char* path = ":settings-migration/Presentation.ddl";
  xiiFileSystem::DeleteFile(path);
  XII_TEST_BOOL(!xiiFileSystem::ExistsFile(path));
  const bool previousVSync = xiiGameApplication::cvar_AppVSync.GetValue();
  XII_SCOPE_EXIT(xiiGameApplication::cvar_AppVSync = previousVSync);
  {
    xiiFileWriter legacy;
    XII_TEST_RESULT(legacy.Open(":settings-migration/CVars_Static.cfg"));
    const xiiStringView contents = "App.VSync = false\n";
    XII_TEST_RESULT(legacy.WriteBytes(contents.GetStartPointer(), contents.GetElementCount()));
  }
  xiiCVar::LoadCVarsFromFile(xiiStringView(":settings-migration/CVars_Static.cfg"), false);
  XII_TEST_BOOL(!xiiGameApplication::cvar_AppVSync.GetValue());
  xiiGameApplicationSettings settings;
  XII_TEST_RESULT(settings.Load(path, xiiGameApplication::cvar_AppVSync.GetValue()));
  XII_TEST_BOOL(!settings.m_bVSync);
  XII_TEST_RESULT(settings.Save(path));
  xiiGameApplicationSettings restored;
  XII_TEST_RESULT(restored.Load(path, true));
  XII_TEST_BOOL(!restored.m_bVSync);
  restored.m_bVSync = true;
  XII_TEST_RESULT(restored.Save(path));
  XII_TEST_RESULT(settings.Load(path, false));
  XII_TEST_BOOL(settings.m_bVSync);
  {
    xiiFileWriter file;
    XII_TEST_RESULT(file.Open(path));
    xiiAbstractObjectGraph graph;
    // Valid DDL with no settings root must be recoverable without assertions.
    xiiAbstractGraphDdlSerializer::Write(file, &graph);
  }
  XII_TEST_BOOL(settings.Load(path, false).Failed());
  XII_TEST_BOOL(settings.m_bVSync);
}
