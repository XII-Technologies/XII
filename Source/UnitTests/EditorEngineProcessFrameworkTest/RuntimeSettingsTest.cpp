/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/System/Window.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Threading/ThreadUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <GameEngine/GameApplication/GameApplication.h>

namespace
{
  class SettingsApplication final : public xiiGameApplication
  {
  public:
    SettingsApplication() : xiiGameApplication("Player settings test", ">sdk/") {}
    using xiiGameApplication::Init_ConfigureCVars;
  };
} // namespace

XII_CREATE_SIMPLE_TEST(Player, RuntimeSettings)
{
  const xiiStringBuilder directory(xiiTestFramework::GetInstance()->GetAbsOutputPath());
  for (const char* folder : {"SettingsProject", "SettingsUser", "OtherInstance"})
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(xiiStringBuilder(directory, "/", folder)));
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiStringBuilder(directory, "/SettingsProject"), "PlayerSettingsTest", "project", xiiDataDirUsage::AllowWrites));
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiStringBuilder(directory, "/SettingsUser"), "PlayerSettingsTest", "appdata", xiiDataDirUsage::AllowWrites));
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiStringBuilder(directory, "/OtherInstance"), "PlayerSettingsTest", "other-user", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PlayerSettingsTest"));
  auto       previousArguments = xiiCommandLineUtils::GetGlobalInstance()->GetCommandLineArray();
  const auto previousVSync     = xiiGameApplication::cvar_AppVSync.GetValue();
  auto*      cap               = static_cast<xiiCVarInt*>(xiiCVar::FindCVarByName("App.FrameLimit"));
  const auto previousCap       = cap->GetValue();
  XII_SCOPE_EXIT(xiiCommandLineUtils::GetGlobalInstance()->SetCommandLine(previousArguments); xiiGameApplication::cvar_AppVSync = previousVSync; *cap = previousCap; xiiCVar::SetStorageFolder({}));
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Project defaults, user preferences and explicit launch precedence")
  {
    xiiGameApplicationSettings project;
    project.m_bVSync       = false;
    project.m_uiFrameLimit = 45;
    project.m_fRenderScale = 0.8f;
    XII_TEST_RESULT(project.Save(":project/RuntimeConfigs/Presentation.ddl"));
    xiiGameApplicationSettings user;
    user.m_bVSync       = true;
    user.m_uiFrameLimit = 90;
    user.m_fRenderScale = 0.7f;
    XII_TEST_RESULT(user.Save(":appdata/RuntimeConfigs/Presentation.ddl"));
    XII_TEST_RESULT(user.Save(":other-user/RuntimeConfigs/Presentation.ddl"));
    xiiDynamicArray<xiiString> arguments;
    for (const char* argument : {"SettingsTest", "-App.VSync", "false", "-App.FrameLimit", "144", "-render-scale", "0.6"})
      arguments.PushBack(argument);
    xiiCommandLineUtils::GetGlobalInstance()->SetCommandLine(arguments);
    SettingsApplication application;
    application.Init_ConfigureCVars();
    XII_TEST_BOOL(!application.GetSettings().m_bVSync);
    XII_TEST_INT(application.GetSettings().m_uiFrameLimit, 144);
    XII_TEST_FLOAT(application.GetSettings().m_fRenderScale, 0.6f, 0.001f);
    xiiGameApplicationSettings unchanged;
    XII_TEST_RESULT(unchanged.Load(":appdata/RuntimeConfigs/Presentation.ddl", false));
    XII_TEST_INT(unchanged.m_uiFrameLimit, 90);
    XII_TEST_FLOAT(unchanged.m_fRenderScale, 0.7f, 0.001f);
    xiiGameApplicationSettings changed;
    changed.Apply(application.GetSettings()).IgnoreResult();
    changed.m_uiFrameLimit = 30;
    XII_TEST_RESULT(application.ApplySettings(changed));
    xiiGameApplicationSettings saved;
    XII_TEST_RESULT(saved.Load(":appdata/RuntimeConfigs/Presentation.ddl", true));
    XII_TEST_INT(saved.m_uiFrameLimit, 30);
    XII_TEST_RESULT(saved.Load(":other-user/RuntimeConfigs/Presentation.ddl", false));
    XII_TEST_INT(saved.m_uiFrameLimit, 90);
  }
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native resize preserves the window and rejects invalid display previews")
  {
    xiiWindowCreationDescription description;
    description.m_Title                = "Player display API validation";
    description.m_WindowMode           = xiiWindowMode::WindowResizable;
    description.m_Resolution           = xiiSizeU32(640, 360);
    description.m_bClipMouseCursor     = false;
    description.m_bShowMouseCursor     = true;
    description.m_bSetForegroundOnInit = false;
    xiiWindow window;
    if (!XII_TEST_RESULT(window.Initialize(description)))
      return;
    const auto handle        = window.GetNativeWindowHandle();
    description.m_Resolution = xiiSizeU32(800, 450);
    XII_TEST_RESULT(window.ApplyWindowSettings(description));
    const auto deadline = xiiTime::Now() + xiiTime::MakeFromSeconds(2);
    while (window.GetClientAreaSize() != description.m_Resolution && xiiTime::Now() < deadline)
    {
      window.ProcessWindowMessages();
      xiiThreadUtils::Sleep(xiiTime::MakeFromMilliseconds(1));
    }
    XII_TEST_INT(window.GetClientAreaSize().width, 800);
    XII_TEST_INT(window.GetClientAreaSize().height, 450);
    XII_TEST_BOOL(window.GetNativeWindowHandle() == handle);
    description.m_iMonitor = 127;
    XII_TEST_BOOL(window.ApplyWindowSettings(description).Failed());
    description.m_iMonitor   = 0;
    description.m_Resolution = xiiSizeU32(640, 360);
    XII_TEST_RESULT(window.ApplyWindowSettings(description));
    window.ProcessWindowMessages();
    XII_TEST_RESULT(window.Destroy());
  }
}
