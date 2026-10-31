/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorFrameworkTest/EditorFrameworkTestPCH.h>

#include <Core/Configuration/PlatformProfile.h>
#include <EditorFramework/CodeGen/CppProject.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/Project/ProjectExport.h>
#include <EditorFrameworkTest/CppProjectTest.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST_GROUP(CodeGen);

XII_CREATE_SIMPLE_TEST(CodeGen, CppSettings)
{
  XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "CppSettingsTest", ":", xiiDataDirUsage::AllowWrites));
  XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("CppSettingsTest"));
  xiiStringBuilder path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CppSettings.ddl");
  xiiCppSettings   settings;
  settings.m_sPluginName = "My_Project2";
  XII_TEST_RESULT(settings.Save(path));
  settings.m_sPluginName = "stale";
  XII_TEST_RESULT(settings.Load(path));
  XII_TEST_STRING(settings.m_sPluginName, "My_Project2");
  for (xiiStringView invalid : {"", "../Other", "My Plugin", "2Project", "A/B", "A;B", "A-B"})
  {
    XII_TEST_BOOL(!xiiCppSettings::IsValidPluginName(invalid));
    settings.m_sPluginName = invalid;
    XII_TEST_BOOL(settings.Save(path).Failed());
  }
  XII_TEST_RESULT(settings.Load(path));
  XII_TEST_STRING(settings.m_sPluginName, "My_Project2");
  {
    xiiFileWriter writer;
    XII_TEST_RESULT(writer.Open(path));
    const xiiStringView malformed = "Target %Default {}";
    XII_TEST_RESULT(writer.WriteBytes(malformed.GetStartPointer(), malformed.GetElementCount()));
  }
  XII_TEST_BOOL(settings.Load(path).Failed());
  XII_TEST_BOOL(settings.m_sPluginName.IsEmpty());
  XII_TEST_RESULT(xiiOSFile::DeleteFile(path));

  const xiiStringBuilder project(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Project With Spaces");
  const xiiStringBuilder expected(project, "/Output/Bin/", BUILDSYSTEM_BUILDTYPE);
  XII_TEST_STRING(xiiCppProject::GetPluginOutputDir(project), expected);
}

XII_CREATE_SIMPLE_TEST(CodeGen, CppProject)
{
  auto* editor = xiiQtEditorApp::GetSingleton();
  if (!XII_TEST_BOOL(editor != nullptr))
    return;
  xiiStartup::StartupCoreSystems();
  auto*           application  = xiiApplication::GetApplicationInstance();
  const xiiString previousName = application->GetApplicationName();
  application->SetApplicationName("xiiEditor");
  XII_SCOPE_EXIT(application->SetApplicationName(previousName));
  xiiStringBuilder preferences(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CppProjectPreferences");
  XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(preferences));
  editor->StartupEditor(xiiQtEditorApp::StartupFlags::UnitTest | xiiQtEditorApp::StartupFlags::NoRecent | xiiQtEditorApp::StartupFlags::SafeMode, preferences);
  XII_SCOPE_EXIT(editor->ShutdownEditor());
  xiiStringBuilder project(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CppProject/xiiProject");
  if (!XII_TEST_RESULT(editor->CreateOrOpenProject(true, project)))
    return;
  XII_SCOPE_EXIT(editor->CloseProject());
  TestCppProjectConfiguration(project.GetFileDirectory());
}

void TestCppProjectConfiguration(xiiStringView sProjectDirectory)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "C++ project presets, migration, and runtime plugin paths")
  {
    xiiCppSettings settings;
    settings.m_sPluginName = "CppPathTest";
    XII_TEST_RESULT(settings.Save());
    xiiStringBuilder settingsPath(sProjectDirectory, "/Editor/CppProject.ddl");
    XII_SCOPE_EXIT(xiiOSFile::DeleteFile(settingsPath).IgnoreResult());
    auto json = xiiCppProject::CreateEmptyCMakeUserPresetsJson(settings);
    XII_TEST_BOOL(xiiCppProject::ModifyCMakeUserPresetsJson(settings, json) == xiiCppProject::ModifyResult::NOT_MODIFIED);
    auto& presets                 = json["configurePresets"].GetWritable<xiiVariantArray>();
    auto& engine                  = presets[0].GetWritable<xiiVariantDictionary>();
    auto& variables               = engine["cacheVariables"].GetWritable<xiiVariantDictionary>();
    variables["CMAKE_C_COMPILER"] = "obsolete-compiler";
    XII_TEST_BOOL(xiiCppProject::ModifyCMakeUserPresetsJson(settings, json) == xiiCppProject::ModifyResult::MODIFIED);
    XII_TEST_BOOL(xiiCppProject::ModifyCMakeUserPresetsJson(settings, json) == xiiCppProject::ModifyResult::NOT_MODIFIED);

    auto invalid = json;
    invalid["configurePresets"].GetWritable<xiiVariantArray>().Clear();
    XII_TEST_BOOL(xiiCppProject::ModifyCMakeUserPresetsJson(settings, invalid) == xiiCppProject::ModifyResult::FAILURE);
    auto       duplicate        = json;
    auto&      duplicatePresets = duplicate["configurePresets"].GetWritable<xiiVariantArray>();
    const auto duplicatePreset  = duplicatePresets[0];
    duplicatePresets.PushBack(duplicatePreset);
    XII_TEST_BOOL(xiiCppProject::ModifyCMakeUserPresetsJson(settings, duplicate) == xiiCppProject::ModifyResult::FAILURE);

    xiiStringBuilder cachePath(xiiCppProject::GetBuildDir(settings), "/CMakeCache.txt");
    XII_SCOPE_EXIT(xiiOSFile::DeleteFile(cachePath).IgnoreResult());
    auto WriteCache = [&](xiiStringView sVersion) {
      xiiDeferredFileWriter writer;
      writer.SetOutput(cachePath);
      xiiStringBuilder text;
      if (!sVersion.IsEmpty())
        text.Append("XII_PROJECT_OUTPUT_LAYOUT_VERSION:INTERNAL=", sVersion, "\n");
      const bool multiConfig = xiiCppProject::GetCMakeGeneratorName(settings).StartsWith("Visual Studio");
      text.Append(multiConfig ? "CMAKE_CONFIGURATION_TYPES:STRING=" : "CMAKE_BUILD_TYPE:STRING=", BUILDSYSTEM_BUILDTYPE);
      XII_TEST_RESULT(writer.WriteBytes(text.GetData(), text.GetElementCount()));
      XII_TEST_RESULT(writer.Close());
    };
    WriteCache({});
    XII_TEST_BOOL(xiiCppProject::CheckCMakeCache(settings).Failed());
    WriteCache("1");
    XII_TEST_RESULT(xiiCppProject::CheckCMakeCache(settings));
    WriteCache("0");
    XII_TEST_BOOL(xiiCppProject::CheckCMakeCache(settings).Failed());

    auto& bundles      = xiiQtEditorApp::GetSingleton()->GetPluginBundles().m_Plugins;
    auto& bundle       = bundles["CppPathTestPlugin"];
    bundle.m_bSelected = true;
    bundle.m_bLoadCopy = true;
    bundle.m_RuntimePlugins.PushBack("CppPathTestPlugin");
    XII_SCOPE_EXIT(bundles.Remove("CppPathTestPlugin"));
    const auto config = xiiQtEditorApp::GetSingleton()->GetRuntimePluginConfig(true);
    bool       found  = false;
    for (const auto& plugin : config.m_Plugins)
    {
      if (plugin.m_sAppDirRelativePath == "CppPathTestPlugin")
      {
        found = true;
        XII_TEST_STRING(plugin.m_sPluginDirectory, ">project/Output/Bin/" BUILDSYSTEM_BUILDTYPE);
        XII_TEST_BOOL(plugin.m_bLoadCopy);
      }
    }
    XII_TEST_BOOL(found);
    XII_TEST_STRING(xiiCppProject::GetPluginBinaryDirectory("CppPathTestPlugin"), xiiCppProject::GetPluginOutputDir(sProjectDirectory));
    XII_TEST_BOOL(xiiCppProject::GetPluginBinaryDirectory("SomeOtherPlugin").IsEmpty());

    // Exercise production export with a project binary, a package dependency, and development-only output.
    // Export only copies these fixtures; they are never loaded into an engine process.
    bundle.m_bLoadCopy = false;
    bundle.m_PackageDependencies.PushBack("ProjectDependency.dll");
    const xiiString  output = xiiCppProject::GetPluginOutputDir();
    xiiStringBuilder binary, copy;
    xiiPlugin::GetPluginPaths("CppPathTestPlugin", binary, copy, 0, output);
    xiiStringBuilder dependency(output, "/ProjectDependency.dll");
    xiiStringBuilder marker(output, "/DoNotExport.txt");
    xiiStringBuilder sdkMarker(output, "/xiiSdkRoot.txt");
    auto             WriteFixture = [](xiiStringView path) {
      xiiDeferredFileWriter writer;
      writer.SetOutput(path);
      const xiiStringView bytes = "export fixture";
      XII_TEST_RESULT(writer.WriteBytes(bytes.GetStartPointer(), bytes.GetElementCount()));
      XII_TEST_RESULT(writer.Close());
    };
    WriteFixture(binary);
    WriteFixture(dependency);
    WriteFixture(copy);
    WriteFixture(marker);
    WriteFixture(sdkMarker);
    XII_SCOPE_EXIT(xiiOSFile::DeleteFile(binary).IgnoreResult(); xiiOSFile::DeleteFile(dependency).IgnoreResult(); xiiOSFile::DeleteFile(copy).IgnoreResult(); xiiOSFile::DeleteFile(marker).IgnoreResult(); xiiOSFile::DeleteFile(sdkMarker).IgnoreResult());
    // This fixture covers binary packaging. Source assets are deliberately excluded;
    // the export dialog normally transforms them before calling ExportProject().
    xiiStringBuilder filterPath(sProjectDirectory, "/ProjectData.xiiExportFilter");
    const bool       hadFilter = xiiOSFile::ExistsFile(filterPath);
    xiiStringBuilder previousFilter;
    if (hadFilter)
    {
      xiiFileReader reader;
      if (!XII_TEST_RESULT(reader.Open(filterPath)))
        return;
      previousFilter.ReadAll(reader);
    }
    auto RestoreFilter = [&]() {
      if (hadFilter)
      {
        xiiDeferredFileWriter writer;
        writer.SetOutput(filterPath);
        writer.WriteBytes(previousFilter.GetData(), previousFilter.GetElementCount()).IgnoreResult();
        writer.Close().IgnoreResult();
      }
      else
        xiiOSFile::DeleteFile(filterPath).IgnoreResult();
    };
    XII_SCOPE_EXIT(RestoreFilter());
    {
      xiiDeferredFileWriter writer;
      writer.SetOutput(filterPath);
      const xiiStringView dataFilter = "#include <CommonData.xiiExportFilter>\n[EXCLUDE]\n*.xii*\n";
      XII_TEST_RESULT(writer.WriteBytes(dataFilter.GetStartPointer(), dataFilter.GetElementCount()));
      XII_TEST_RESULT(writer.Close());
    }

    xiiPlatformProfile profile;
    profile.SetConfigName("CppExportTest");
    xiiStringBuilder lookup(sProjectDirectory, "/AssetCache/CppExportTest.xiiAidlt");
    WriteFixture(lookup);
    XII_SCOPE_EXIT(xiiOSFile::DeleteFile(lookup).IgnoreResult());
    xiiApplicationFileSystemConfig filesystem;
    auto&                          project = filesystem.m_DataDirs.ExpandAndGetRef();
    project.m_sDataDirSpecialPath          = ">project";
    project.m_sRootName                    = "project";
    xiiStringBuilder exportPath(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CppPluginExport");
    XII_TEST_RESULT(xiiProjectExport::ExportProject(exportPath, &profile, filesystem));
    xiiStringBuilder exportedBinary(exportPath, "/Bin/", binary.GetFileNameAndExtension());
    XII_TEST_BOOL(xiiOSFile::ExistsFile(exportedBinary));
    XII_TEST_BOOL(xiiOSFile::ExistsFile(xiiStringBuilder(exportPath, "/Bin/ProjectDependency.dll")));
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(xiiStringBuilder(exportPath, "/Bin/", copy.GetFileNameAndExtension())));
    XII_TEST_BOOL(!xiiOSFile::ExistsDirectory(xiiStringBuilder(exportPath, "/Data/project/Output")));
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(xiiStringBuilder(exportPath, "/Bin/DoNotExport.txt")));
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(xiiStringBuilder(exportPath, "/Bin/xiiSdkRoot.txt")));
    xiiProjectManifest exportedManifest;
    XII_TEST_STATUS(exportedManifest.Load(xiiStringBuilder(exportPath, "/Data/project/xiiProject")));
    XII_TEST_BOOL(exportedManifest.m_ProjectID == xiiToolsProject::GetSingleton()->GetManifest().m_ProjectID);
    XII_TEST_STRING(exportedManifest.m_sDataDirectories, "RuntimeConfigs/DataDirectories.ddl");
    XII_TEST_STRING(exportedManifest.m_sRuntimePlugins, "RuntimeConfigs/Plugins.ddl");
    xiiApplicationPluginConfig packaged;
    packaged.Load(xiiStringBuilder(exportPath, "/Data/project/RuntimeConfigs/Plugins.ddl"));
    bool packagedPlugin = false;
    for (const auto& plugin : packaged.m_Plugins)
    {
      XII_TEST_BOOL(plugin.m_sPluginDirectory.IsEmpty());
      XII_TEST_BOOL(!plugin.m_bLoadCopy);
      packagedPlugin |= plugin.m_sAppDirRelativePath == "CppPathTestPlugin";
    }
    XII_TEST_BOOL(packagedPlugin);
    XII_TEST_RESULT(xiiOSFile::DeleteFile(binary));
    {
      xiiTestLogInterface   log;
      xiiTestLogSystemScope scope(&log);
      log.ExpectMessage("is missing. Build the C++ project before exporting.", xiiLogMsgType::ErrorMsg);
      xiiStringBuilder missingExport(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/CppPluginMissingExport");
      XII_TEST_BOOL(xiiProjectExport::ExportProject(missingExport, &profile, filesystem).Failed());
    }
  }
}
