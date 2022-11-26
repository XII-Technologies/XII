#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>

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

#endif
}
