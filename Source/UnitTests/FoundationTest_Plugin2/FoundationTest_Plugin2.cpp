/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>

static xiiInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

XII_PLUGIN_DEPENDENCY(xiiFoundationTest_Plugin1);

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

xiiCVarInt    CVar_TestInt("test2_Int", 22, xiiCVarFlags::None, "Desc: test2_Int");
xiiCVarFloat  CVar_TestFloat("test2_Float", 2.2f, xiiCVarFlags::Default, "Desc: test2_Float");
xiiCVarDouble CVar_TestDouble("test2_Double", 22.22, xiiCVarFlags::Default, "Desc: test2_Double");
xiiCVarBool   CVar_TestBool("test2_Bool", true, xiiCVarFlags::Save, "Desc: test2_Bool");
xiiCVarString CVar_TestString("test2_String", "test2", xiiCVarFlags::RequiresRestart, "Desc: test2_String");

xiiCVarBool CVar_TestInited("test2_Inited", false, xiiCVarFlags::None, "Desc: test2_Inited");

void OnLoadPlugin()
{
  XII_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  xiiCVarInt* pCVar = (xiiCVarInt*)xiiCVar::FindCVarByName("TestPlugin2InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  xiiCVarBool* pCVarDep = (xiiCVarBool*)xiiCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are available (ie. plugin1 is already loaded)
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Double") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = true;
}

void OnUnloadPlugin()
{
  XII_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  xiiCVarInt* pCVar = (xiiCVarInt*)xiiCVar::FindCVarByName("TestPlugin2UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  xiiCVarBool* pCVarDep = (xiiCVarBool*)xiiCVar::FindCVarByName("TestPlugin2FoundDependencies");

  if (pCVarDep)
  {
    *pCVarDep = true;

    // check that all CVars from plugin1 are STILL available (ie. plugin1 is not yet unloaded)
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Int") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Float") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Double") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_Bool") != nullptr);
    *pCVarDep = *pCVarDep && (xiiCVar::FindCVarByName("test1_String") != nullptr);
  }

  CVar_TestInited = false;
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin2, TestSubSystem2)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "PluginGroup_Plugin1"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on
