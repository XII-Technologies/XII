/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Types/ScopeExit.h>

xiiCVarInt CVar_TestPlugin1InitializedCount("TestPlugin1InitCount", 0, xiiCVarFlags::None, "How often Plugin1 has been initialized.");
xiiCVarInt CVar_TestPlugin1UninitializedCount("TestPlugin1UninitCount", 0, xiiCVarFlags::None, "How often Plugin1 has been uninitialized.");
xiiCVarInt CVar_TestPlugin1Reloaded("TestPlugin1Reloaded", 0, xiiCVarFlags::None, "How often Plugin1 has been reloaded (counts init AND de-init).");

xiiCVarInt  CVar_TestPlugin2InitializedCount("TestPlugin2InitCount", 0, xiiCVarFlags::None, "How often Plugin2 has been initialized.");
xiiCVarInt  CVar_TestPlugin2UninitializedCount("TestPlugin2UninitCount", 0, xiiCVarFlags::None, "How often Plugin2 has been uninitialized.");
xiiCVarInt  CVar_TestPlugin2Reloaded("TestPlugin2Reloaded", 0, xiiCVarFlags::None, "How often Plugin2 has been reloaded (counts init AND de-init).");
xiiCVarBool CVar_TestPlugin2FoundDependencies("TestPlugin2FoundDependencies", false, xiiCVarFlags::None, "Whether Plugin2 found all its dependencies (other plugins).");

XII_CREATE_SIMPLE_TEST(Configuration, Plugin)
{
  CVar_TestPlugin1InitializedCount   = 0;
  CVar_TestPlugin1UninitializedCount = 0;
  CVar_TestPlugin1Reloaded           = 0;
  CVar_TestPlugin2InitializedCount   = 0;
  CVar_TestPlugin2UninitializedCount = 0;
  CVar_TestPlugin2Reloaded           = 0;
  CVar_TestPlugin2FoundDependencies  = false;

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LoadPlugin")
  {
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2) == XII_SUCCESS);
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2, xiiPluginLoadFlags::PluginIsOptional) == XII_SUCCESS); // loading already loaded plugin is always a success

    XII_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    XII_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    XII_TEST_INT(CVar_TestPlugin1UninitializedCount, 0);
    XII_TEST_INT(CVar_TestPlugin2UninitializedCount, 0);

    XII_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    XII_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    XII_TEST_BOOL(CVar_TestPlugin2FoundDependencies);

    // this will fail the FoundationTests, as it logs an error
    // XII_TEST_BOOL(xiiPlugin::LoadPlugin("Test") == XII_FAILURE); // plugin does not exist
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "UnloadPlugin")
  {
    CVar_TestPlugin2FoundDependencies = false;
    xiiPlugin::UnloadAllPlugins();

    XII_TEST_INT(CVar_TestPlugin1InitializedCount, 1);
    XII_TEST_INT(CVar_TestPlugin2InitializedCount, 1);

    XII_TEST_INT(CVar_TestPlugin1UninitializedCount, 1);
    XII_TEST_INT(CVar_TestPlugin2UninitializedCount, 1);

    XII_TEST_INT(CVar_TestPlugin1Reloaded, 0);
    XII_TEST_INT(CVar_TestPlugin2Reloaded, 0);

    XII_TEST_BOOL(CVar_TestPlugin2FoundDependencies);
    XII_TEST_BOOL(xiiPlugin::LoadPlugin("Test", xiiPluginLoadFlags::PluginIsOptional) == XII_FAILURE); // plugin does not exist
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Explicit plugin directory and copy lifecycle")
  {
    xiiStringBuilder directory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/Project Plugins");
    XII_TEST_RESULT(xiiOSFile::CreateDirectoryStructure(directory));
    xiiStringBuilder original, copy, source, unused;
    xiiPlugin::GetPluginPaths(xiiFoundationTest_Plugin2, source, unused, 0);
    xiiPlugin::GetPluginPaths(xiiFoundationTest_Plugin2, original, copy, 0, directory);
    XII_TEST_BOOL(original.StartsWith(directory));
    XII_TEST_BOOL(copy.StartsWith(directory));
    // A same-name binary next to the application must not mask a missing project binary.
    XII_TEST_BOOL(!xiiPlugin::ExistsPluginFile(xiiFoundationTest_Plugin2, directory));
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2, xiiPluginLoadFlags::PluginIsOptional, directory).Failed());
    XII_TEST_RESULT(xiiOSFile::CopyFile(source, original));
    XII_SCOPE_EXIT(xiiPlugin::UnloadAllPlugins(); xiiOSFile::DeleteFolder(directory).IgnoreResult());
    XII_TEST_RESULT(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2, xiiPluginLoadFlags::LoadCopy, directory));
    XII_TEST_BOOL(CVar_TestPlugin2FoundDependencies);
    XII_TEST_INT(CVar_TestPlugin2InitializedCount, 2);
    XII_TEST_RESULT(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2));
    XII_TEST_INT(CVar_TestPlugin2InitializedCount, 2);
    if (xiiPlugin::PlatformNeedsPluginCopy())
    {
      XII_TEST_BOOL(xiiOSFile::ExistsFile(copy));
      XII_TEST_RESULT(xiiOSFile::DeleteFile(original));
    }
    xiiPlugin::UnloadAllPlugins();
    XII_TEST_BOOL(!xiiOSFile::ExistsFile(copy));
    // Direct loading must never delete an unrelated .loaded file on unload.
    XII_TEST_RESULT(xiiOSFile::CopyFile(source, original));
    XII_TEST_RESULT(xiiOSFile::CopyFile(source, copy));
    XII_TEST_RESULT(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2, xiiPluginLoadFlags::Default, directory));
    xiiPlugin::UnloadAllPlugins();
    XII_TEST_BOOL(xiiOSFile::ExistsFile(original));
    XII_TEST_BOOL(xiiOSFile::ExistsFile(copy));
  }

#endif

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Plugin config directory round trip and legacy defaults")
  {
    XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "PluginConfigTest", ":", xiiDataDirUsage::AllowWrites));
    XII_SCOPE_EXIT(xiiFileSystem::RemoveDataDirectoryGroup("PluginConfigTest"));
    xiiStringBuilder                         path(xiiTestFramework::GetInstance()->GetAbsOutputPath(), "/PluginConfig.ddl");
    xiiApplicationPluginConfig               config;
    xiiApplicationPluginConfig::PluginConfig project;
    project.m_sAppDirRelativePath = "ProjectPlugin";
    project.m_bLoadCopy           = true;
    project.m_sPluginDirectory    = ">project/Output/Bin/Dev";
    XII_TEST_BOOL(config.AddPlugin(project));
    xiiApplicationPluginConfig::PluginConfig legacy;
    legacy.m_sAppDirRelativePath = "EnginePlugin";
    XII_TEST_BOOL(config.AddPlugin(legacy));
    XII_TEST_RESULT(config.Save(path));
    xiiApplicationPluginConfig loaded;
    loaded.Load(path);
    XII_TEST_INT(loaded.m_Plugins.GetCount(), 2);
    if (loaded.m_Plugins.GetCount() == 2)
    {
      XII_TEST_BOOL(loaded.m_Plugins[0].m_sPluginDirectory.IsEmpty());
      XII_TEST_STRING(loaded.m_Plugins[1].m_sPluginDirectory, project.m_sPluginDirectory);
      XII_TEST_BOOL(loaded.m_Plugins[1].m_bLoadCopy);
    }
    XII_TEST_RESULT(xiiOSFile::DeleteFile(path));
  }
}
