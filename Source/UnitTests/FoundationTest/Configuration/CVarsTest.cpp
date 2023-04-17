#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/CommandLineUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(Configuration);

#define xiiCVarValueDefault xiiCVarValue::Default
#define xiiCVarValueStored  xiiCVarValue::Stored
#define xiiCVarValueRestart xiiCVarValue::Restart

// Interestingly using 'xiiCVarValue::Default' directly inside a macro does not work. (?!)
#define CHECK_CVAR(var, Current, Default, Stored, Restart)        \
  XII_TEST_BOOL(var != nullptr);                                  \
  if (var != nullptr)                                             \
  {                                                               \
    XII_TEST_BOOL(var->GetValue() == Current);                    \
    XII_TEST_BOOL(var->GetValue(xiiCVarValueDefault) == Default); \
    XII_TEST_BOOL(var->GetValue(xiiCVarValueStored) == Stored);   \
    XII_TEST_BOOL(var->GetValue(xiiCVarValueRestart) == Restart); \
  }

static xiiInt32 iChangedValue   = 0;
static xiiInt32 iChangedRestart = 0;

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

static void ChangedCVar(const xiiCVarEvent& e)
{
  switch (e.m_EventType)
  {
    case xiiCVarEvent::ValueChanged:
      ++iChangedValue;
      break;
    case xiiCVarEvent::RestartValueChanged:
      ++iChangedRestart;
      break;
    default:
      break;
  }
}

#endif

XII_CREATE_SIMPLE_TEST(Configuration, CVars)
{
  iChangedValue   = 0;
  iChangedRestart = 0;

  // setup the filesystem
  // we need it to test the storing of cvars (during plugin reloading)

  xiiStringBuilder sOutputFolder1 = xiiTestFramework::GetInstance()->GetAbsOutputPath();

  XII_TEST_BOOL(xiiFileSystem::AddDataDirectory(sOutputFolder1.GetData(), "test", "output", xiiFileSystem::AllowWrites) == XII_SUCCESS);

  // Delete all cvar setting files
  {
    xiiStringBuilder sConfigFile;

    sConfigFile = ":output/CVars/CVars_" xiiFoundationTest_Plugin1 ".cfg";

    xiiFileSystem::DeleteFile(sConfigFile.GetData());

    sConfigFile = ":output/CVars/CVars_" xiiFoundationTest_Plugin2 ".cfg";

    xiiFileSystem::DeleteFile(sConfigFile.GetData());
  }

  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Int2");
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102");

  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Float2");
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102.2");

  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Double2");
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("102.22");

  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_Bool2");
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("false");

  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("-test1_String2");
  xiiCommandLineUtils::GetGlobalInstance()->InjectCustomArgument("test1c");

  xiiCVar::SetStorageFolder(":output/CVars");
  xiiCVar::LoadCVars(); // should do nothing (no settings files available)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "No Plugin Loaded")
  {
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String") == nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_String") == nullptr);
  }

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Plugin1 Loaded")
  {
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin1) == XII_SUCCESS);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String") != nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int2") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float2") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double2") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool2") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String2") != nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_String") == nullptr);

    xiiPlugin::UnloadAllPlugins();
  }

#endif

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String") == nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_String") == nullptr);
  }

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Plugin2 Loaded")
  {
    // Plugin2 should automatically load Plugin1 with it

    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2) == XII_SUCCESS);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String") != nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Int") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Float") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Double") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Bool") != nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_String") != nullptr);

    xiiPlugin::UnloadAllPlugins();
  }

#endif

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "No Plugin Loaded (2)")
  {
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test1_String") == nullptr);

    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Int") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Float") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Double") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_Bool") == nullptr);
    XII_TEST_BOOL(xiiCVar::FindCVarByName("test2_String") == nullptr);
  }

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Default Value Test")
  {
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2) == XII_SUCCESS);

    // CVars from Plugin 1
    {
      xiiCVarInt* pInt = (xiiCVarInt*)xiiCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 11, 11, 11, 11);

      if (pInt)
      {
        XII_TEST_BOOL(pInt->GetType() == xiiCVarType::Int);
        XII_TEST_BOOL(pInt->GetName() == "test1_Int");
        XII_TEST_BOOL(pInt->GetDescription() == "Desc: test1_Int");

        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 12;
        CHECK_CVAR(pInt, 12, 11, 11, 12);
        XII_TEST_INT(iChangedValue, 1);
        XII_TEST_INT(iChangedRestart, 0);

        // no change
        *pInt = 12;
        XII_TEST_INT(iChangedValue, 1);
        XII_TEST_INT(iChangedRestart, 0);
      }

      xiiCVarFloat* pFloat = (xiiCVarFloat*)xiiCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.1f);

      if (pFloat)
      {
        XII_TEST_BOOL(pFloat->GetType() == xiiCVarType::Float);
        XII_TEST_BOOL(pFloat->GetName() == "test1_Float");
        XII_TEST_BOOL(pFloat->GetDescription() == "Desc: test1_Float");

        pFloat->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pFloat = 1.2f;
        CHECK_CVAR(pFloat, 1.1f, 1.1f, 1.1f, 1.2f);

        XII_TEST_INT(iChangedValue, 1);
        XII_TEST_INT(iChangedRestart, 1);

        // no change
        *pFloat = 1.2f;
        XII_TEST_INT(iChangedValue, 1);
        XII_TEST_INT(iChangedRestart, 1);

        pFloat->SetToRestartValue();
        CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.1f, 1.2f);

        XII_TEST_INT(iChangedValue, 2);
        XII_TEST_INT(iChangedRestart, 1);
      }

      xiiCVarDouble* pDouble = (xiiCVarDouble*)xiiCVar::FindCVarByName("test1_Double");
      CHECK_CVAR(pDouble, 12.12, 12.12, 12.12, 12.12);

      if (pDouble)
      {
        XII_TEST_BOOL(pDouble->GetType() == xiiCVarType::Double);
        XII_TEST_BOOL(pDouble->GetName() == "test1_Double");
        XII_TEST_BOOL(pDouble->GetDescription() == "Desc: test1_Double");

        pDouble->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pDouble = 12.11;
        CHECK_CVAR(pDouble, 12.12, 12.12, 12.12, 12.11);

        XII_TEST_INT(iChangedValue, 2);
        XII_TEST_INT(iChangedRestart, 2);

        // no change
        *pDouble = 12.11;
        XII_TEST_INT(iChangedValue, 2);
        XII_TEST_INT(iChangedRestart, 2);

        pDouble->SetToRestartValue();
        CHECK_CVAR(pDouble, 12.11, 12.12, 12.12, 12.11);

        XII_TEST_INT(iChangedValue, 3);
        XII_TEST_INT(iChangedRestart, 2);
      }

      xiiCVarBool* pBool = (xiiCVarBool*)xiiCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      if (pBool)
      {
        XII_TEST_BOOL(pBool->GetType() == xiiCVarType::Bool);
        XII_TEST_BOOL(pBool->GetName() == "test1_Bool");
        XII_TEST_BOOL(pBool->GetDescription() == "Desc: test1_Bool");

        *pBool = true;
        CHECK_CVAR(pBool, true, false, false, true);
      }

      xiiCVarString* pString = (xiiCVarString*)xiiCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");

      if (pString)
      {
        XII_TEST_BOOL(pString->GetType() == xiiCVarType::String);
        XII_TEST_BOOL(pString->GetName() == "test1_String");
        XII_TEST_BOOL(pString->GetDescription() == "Desc: test1_String");

        *pString = "test1_value2";
        CHECK_CVAR(pString, "test1_value2", "test1", "test1", "test1_value2");
      }
    }

    // CVars from Plugin 2
    {
      xiiCVarInt* pInt = (xiiCVarInt*)xiiCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      if (pInt)
      {
        pInt->m_CVarEvents.AddEventHandler(ChangedCVar);

        *pInt = 23;
        CHECK_CVAR(pInt, 23, 22, 22, 23);
        XII_TEST_INT(iChangedValue, 4);
        XII_TEST_INT(iChangedRestart, 2);
      }

      xiiCVarFloat* pFloat = (xiiCVarFloat*)xiiCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      if (pFloat)
      {
        *pFloat = 2.3f;
        CHECK_CVAR(pFloat, 2.3f, 2.2f, 2.2f, 2.3f);
      }

      xiiCVarDouble* pDouble = (xiiCVarDouble*)xiiCVar::FindCVarByName("test2_Double");
      CHECK_CVAR(pDouble, 22.22, 22.22, 22.22, 22.22);

      if (pDouble)
      {
        *pDouble = 22.11;
        CHECK_CVAR(pDouble, 22.11, 22.22, 22.22, 22.11);
      }

      xiiCVarBool* pBool = (xiiCVarBool*)xiiCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, true, true, true, true);

      if (pBool)
      {
        *pBool = false;
        CHECK_CVAR(pBool, false, true, true, false);
      }

      xiiCVarString* pString = (xiiCVarString*)xiiCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2", "test2", "test2", "test2");

      if (pString)
      {
        *pString = "test2_value2";
        CHECK_CVAR(pString, "test2", "test2", "test2", "test2_value2");

        pString->SetToRestartValue();
        CHECK_CVAR(pString, "test2_value2", "test2", "test2", "test2_value2");
      }
    }

    xiiPlugin::UnloadAllPlugins();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Loaded Value Test")
  {
    XII_TEST_BOOL(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin2) == XII_SUCCESS);

    // CVars from Plugin 1
    {
      xiiCVarInt* pInt = (xiiCVarInt*)xiiCVar::FindCVarByName("test1_Int");
      CHECK_CVAR(pInt, 12, 11, 12, 12);

      xiiCVarFloat* pFloat = (xiiCVarFloat*)xiiCVar::FindCVarByName("test1_Float");
      CHECK_CVAR(pFloat, 1.2f, 1.1f, 1.2f, 1.2f);

      xiiCVarDouble* pDouble = (xiiCVarDouble*)xiiCVar::FindCVarByName("test1_Double");
      CHECK_CVAR(pDouble, 12.11, 12.12, 12.11, 12.11);

      xiiCVarBool* pBool = (xiiCVarBool*)xiiCVar::FindCVarByName("test1_Bool");
      CHECK_CVAR(pBool, false, false, false, false);

      xiiCVarString* pString = (xiiCVarString*)xiiCVar::FindCVarByName("test1_String");
      CHECK_CVAR(pString, "test1", "test1", "test1", "test1");
    }

    // CVars from Plugin 1, overridden by command line
    {
      xiiCVarInt* pInt = (xiiCVarInt*)xiiCVar::FindCVarByName("test1_Int2");
      CHECK_CVAR(pInt, 102, 21, 102, 102);

      xiiCVarFloat* pFloat = (xiiCVarFloat*)xiiCVar::FindCVarByName("test1_Float2");
      CHECK_CVAR(pFloat, 102.2f, 2.1f, 102.2f, 102.2f);

      xiiCVarDouble* pDouble = (xiiCVarDouble*)xiiCVar::FindCVarByName("test1_Double2");
      CHECK_CVAR(pDouble, 102.22, 122.122, 102.22, 102.22);

      xiiCVarBool* pBool = (xiiCVarBool*)xiiCVar::FindCVarByName("test1_Bool2");
      CHECK_CVAR(pBool, false, true, false, false);

      xiiCVarString* pString = (xiiCVarString*)xiiCVar::FindCVarByName("test1_String2");
      CHECK_CVAR(pString, "test1c", "test1b", "test1c", "test1c");
    }

    // CVars from Plugin 2
    {
      xiiCVarInt* pInt = (xiiCVarInt*)xiiCVar::FindCVarByName("test2_Int");
      CHECK_CVAR(pInt, 22, 22, 22, 22);

      xiiCVarFloat* pFloat = (xiiCVarFloat*)xiiCVar::FindCVarByName("test2_Float");
      CHECK_CVAR(pFloat, 2.2f, 2.2f, 2.2f, 2.2f);

      xiiCVarDouble* pDouble = (xiiCVarDouble*)xiiCVar::FindCVarByName("test2_Double");
      CHECK_CVAR(pDouble, 22.22, 22.22, 22.22, 22.22);

      xiiCVarBool* pBool = (xiiCVarBool*)xiiCVar::FindCVarByName("test2_Bool");
      CHECK_CVAR(pBool, false, true, false, false);

      xiiCVarString* pString = (xiiCVarString*)xiiCVar::FindCVarByName("test2_String");
      CHECK_CVAR(pString, "test2_value2", "test2", "test2_value2", "test2_value2");
    }

    xiiPlugin::UnloadAllPlugins();
  }

#endif

  xiiFileSystem::ClearAllDataDirectories();
}
