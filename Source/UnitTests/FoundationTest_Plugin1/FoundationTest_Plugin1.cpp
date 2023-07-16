#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Plugin.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Reflection.h>

static xiiInt32 g_iPluginState = -1;

void OnLoadPlugin();
void OnUnloadPlugin();

xiiCVarInt    CVar_TestInt("test1_Int", 11, xiiCVarFlags::Save, "Desc: test1_Int");
xiiCVarFloat  CVar_TestFloat("test1_Float", 1.1f, xiiCVarFlags::RequiresRestart, "Desc: test1_Float");
xiiCVarDouble CVar_TestDouble("test1_Double", 12.12, xiiCVarFlags::RequiresRestart, "Desc: test1_Double");
xiiCVarBool   CVar_TestBool("test1_Bool", false, xiiCVarFlags::None, "Desc: test1_Bool");
xiiCVarString CVar_TestString("test1_String", "test1", xiiCVarFlags::Default, "Desc: test1_String");

xiiCVarInt    CVar_TestInt2("test1_Int2", 21, xiiCVarFlags::Default, "Desc: test1_Int2");
xiiCVarFloat  CVar_TestFloat2("test1_Float2", 2.1f, xiiCVarFlags::Default, "Desc: test1_Float2");
xiiCVarDouble CVar_TestDouble2("test1_Double2", 122.122, xiiCVarFlags::Default, "Desc: test1_Double2");
xiiCVarBool   CVar_TestBool2("test1_Bool2", true, xiiCVarFlags::Default, "Desc: test1_Bool2");
xiiCVarString CVar_TestString2("test1_String2", "test1b", xiiCVarFlags::Default, "Desc: test1_String2");

XII_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

XII_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void OnLoadPlugin()
{
  XII_TEST_BOOL_MSG(g_iPluginState == -1, "Plugin is in an invalid state.");
  g_iPluginState = 1;

  xiiCVarInt* pCVar = (xiiCVarInt*)xiiCVar::FindCVarByName("TestPlugin1InitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;

  xiiCVarBool* pCVarPlugin2Inited = (xiiCVarBool*)xiiCVar::FindCVarByName("test2_Inited");
  if (pCVarPlugin2Inited)
  {
    XII_TEST_BOOL(*pCVarPlugin2Inited == false); // Although Plugin2 is present, it should not yet have been initialized
  }
}

void OnUnloadPlugin()
{
  XII_TEST_BOOL_MSG(g_iPluginState == 1, "Plugin is in an invalid state.");
  g_iPluginState = 2;

  xiiCVarInt* pCVar = (xiiCVarInt*)xiiCVar::FindCVarByName("TestPlugin1UninitCount");

  if (pCVar)
    *pCVar = *pCVar + 1;
}

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(PluginGroup_Plugin1, TestSubSystem1)

  //BEGIN_SUBSYSTEM_DEPENDENCIES
  //  "PluginGroup_Plugin1"
  //END_SUBSYSTEM_DEPENDENCIES

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

struct xiiTestStruct2
{
  float m_fFloat2;

  xiiTestStruct2() { m_fFloat2 = 42.0f; }
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTestStruct2);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTestStruct2, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTestStruct2>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Float2", m_fFloat2),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on
