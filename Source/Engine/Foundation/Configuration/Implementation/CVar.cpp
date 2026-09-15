/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Utilities/CommandLineOptions.h>
#include <Foundation/Utilities/CommandLineUtils.h>
#include <Foundation/Utilities/ConversionUtils.h>

// clang-format off
XII_ENUMERABLE_CLASS_IMPLEMENTATION(xiiCVar);

// The CVars need to be saved and loaded whenever plugins are loaded and unloaded.
// Therefore we register as early as possible (Base Startup) at the plugin system,
// to be informed about plugin changes.
XII_BEGIN_SUBSYSTEM_DECLARATION(Foundation, CVars)

  // For saving and loading we need the filesystem, so make sure we are initialized after
  // and shutdown before the filesystem is.
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // Save the CVars every time the core is shut down.
    // At this point the filesystem might already be uninitialized by the user (data dirs),
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though.
    xiiCVar::SaveCVars();

    xiiPlugin::Events().RemoveEventHandler(xiiCVar::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // Save the CVars every time the engine is shut down.
    // At this point the filesystem should usually still be configured properly.
    xiiCVar::SaveCVars();
  }

  // The user is responsible to call 'xiiCVar::SetStorageFolder' to define where the CVars are
  // actually stored. That call will automatically load all CVar states.

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


xiiString                     xiiCVar::s_sStorageFolder;
xiiEvent<const xiiCVarEvent&> xiiCVar::s_AllCVarEvents;

void xiiCVar::AssignSubSystemPlugin(xiiStringView sPluginName)
{
  xiiCVar* pCVar = xiiCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_sPluginName.IsEmpty())
      pCVar->m_sPluginName = sPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void xiiCVar::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::BeforeLoading:
    {
      // Before a new plugin is loaded, make sure all currently available CVars are assigned to the proper plugin.
      // All not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin.
      AssignSubSystemPlugin("Static");
    }
    break;

    case xiiPluginEvent::AfterLoadingBeforeInit:
    {
      // After we loaded a new plugin, but before it is initialized, find all new CVars and assign them to that new plugin.
      AssignSubSystemPlugin(EventData.m_sPluginBinary);

      // Now load the state of all CVars.
      LoadCVars();
    }
    break;

    case xiiPluginEvent::BeforeUnloading:
    {
      SaveCVars();
    }
    break;

    default:
      break;
  }
}

xiiCVar::xiiCVar(xiiStringView sName, xiiBitflags<xiiCVarFlags> Flags, xiiStringView sDescription) :
  m_sName(sName), m_sDescription(sDescription), m_Flags(Flags)
{
  XII_ASSERT_DEV(!m_sDescription.IsEmpty(), "Please add a useful description for CVar '{}'.", sName);
}

xiiCVar* xiiCVar::FindCVarByName(xiiStringView sName)
{
  xiiCVar* pCVar = xiiCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->GetName().IsEqual_NoCase(sName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void xiiCVar::SetStorageFolder(xiiStringView sFolder)
{
  s_sStorageFolder = sFolder;
}

xiiCommandLineOptionBool opt_NoFileCVars("cvar", "-no-file-cvars", "Disables loading CVar values from the user-specific, persisted configuration file.", false);

void xiiCVar::SaveCVarsToFile(xiiStringView sPath, bool bIgnoreSaveFlag)
{
  xiiTemporaryHybridArray<xiiCVar*, 128> allCVars;

  for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
    {
      allCVars.PushBack(pCVar);
    }
  }

  SaveCVarsToFileInternal(sPath, allCVars);
}

void xiiCVar::SaveCVars()
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // This command line disables loading and saving CVars to and from files.
  if (opt_NoFileCVars.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  // First gather all the cvars by plugin.
  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>> PluginCVars;

  {
    xiiCVar* pCVar = xiiCVar::GetFirstInstance();
    while (pCVar)
    {
      // Only store cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
      {
        if (!pCVar->m_sPluginName.IsEmpty())
          PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
        else
          PluginCVars["Static"].PushBack(pCVar);
      }

      pCVar = pCVar->GetNextInstance();
    }
  }

  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

  xiiStringBuilder sTemp;

  // Now save all cvars in their plugin specific file.
  while (it.IsValid())
  {
    // Create the plugin specific file.
    sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

    SaveCVarsToFileInternal(sTemp, it.Value());

    // continue with the next plugin
    ++it;
  }
}

void xiiCVar::SaveCVarsToFileInternal(xiiStringView path, const xiiDynamicArray<xiiCVar*>& vars)
{
  xiiStringBuilder sTemp;
  xiiFileWriter    File;
  if (File.Open(path.GetData(sTemp)) == XII_SUCCESS)
  {
    // write one line for each cvar, to save its current value
    for (xiiUInt32 var = 0; var < vars.GetCount(); ++var)
    {
      xiiCVar* pCVar = vars[var];

      switch (pCVar->GetType())
      {
        case xiiCVarType::Int:
        {
          xiiCVarInt* pInt = (xiiCVarInt*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(xiiCVarValue::DelayedSync));
        }
        break;
        case xiiCVarType::Bool:
        {
          xiiCVarBool* pBool = (xiiCVarBool*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(xiiCVarValue::DelayedSync) ? "true" : "false");
        }
        break;
        case xiiCVarType::Float:
        {
          xiiCVarFloat* pFloat = (xiiCVarFloat*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(xiiCVarValue::DelayedSync));
        }
        break;
        case xiiCVarType::Double:
        {
          xiiCVarDouble* pDouble = (xiiCVarDouble*)pCVar;
          sTemp.SetFormat("{0} = {1}\n", pCVar->GetName(), pDouble->GetValue(xiiCVarValue::DelayedSync));
        }
        break;
        case xiiCVarType::String:
        {
          xiiCVarString* pString = (xiiCVarString*)pCVar;
          sTemp.SetFormat("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(xiiCVarValue::DelayedSync));
        }
        break;
        default:
          XII_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      // add the one line for that cvar to the config file
      File.WriteBytes(sTemp.GetData(), sTemp.GetElementCount()).IgnoreResult();
    }
  }
}

void xiiCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static xiiResult ParseLine(const xiiString& sLine, xiiStringBuilder& out_sVarName, xiiStringBuilder& out_sVarValue)
{
  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return XII_FAILURE;

  {
    xiiStringView sSubString(sLine.GetData(), szSign);

    // Remove all trailing spaces.
    while (sSubString.EndsWith(" "))
    {
      sSubString.Shrink(0, 1);
    }

    out_sVarName = sSubString;
  }

  {
    xiiStringView sSubString(szSign + 1);

    // Remove all spaces.
    while (sSubString.StartsWith(" "))
    {
      sSubString.Shrink(1, 0);
    }

    // Remove all trailing spaces.
    while (sSubString.EndsWith(" "))
    {
      sSubString.Shrink(0, 1);
    }

    // Remove " and start and end.

    if (sSubString.StartsWith("\""))
    {
      sSubString.Shrink(1, 0);
    }

    if (sSubString.EndsWith("\""))
    {
      sSubString.Shrink(0, 1);
    }

    out_sVarValue = sSubString;
  }

  return XII_SUCCESS;
}

void xiiCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue, xiiDynamicArray<xiiCVar*>* pOutCVars)
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // This command line disables loading and saving CVars to and from files.
  if (opt_NoFileCVars.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>> PluginCVars;

  // First gather all the cvars by plugin.
  {
    for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      // Only load cvars that should be saved.
      if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
      {
        if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
        {
          if (!pCVar->m_sPluginName.IsEmpty())
          {
            PluginCVars[pCVar->m_sPluginName].PushBack(pCVar);
          }
          else
          {
            PluginCVars["Static"].PushBack(pCVar);
          }
        }
      }

      // It doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value.
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  {
    xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    xiiStringBuilder sTemp;

    while (it.IsValid())
    {
      // Create the plugin specific file.
      sTemp.SetFormat("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

      LoadCVarsFromFileInternal(sTemp.GetView(), it.Value(), bSetAsCurrentValue, pOutCVars);

      // continue with the next plugin
      ++it;
    }
  }
}

void xiiCVar::LoadCVarsFromFile(xiiStringView sPath, bool bOnlyNewOnes, bool bSetAsCurrentValue, bool bIgnoreSaveFlag, xiiDynamicArray<xiiCVar*>* pOutCVars)
{
  xiiTemporaryHybridArray<xiiCVar*, 128> allCVars;

  for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bIgnoreSaveFlag || pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
    {
      if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
      {
        allCVars.PushBack(pCVar);
      }
    }

    // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
    pCVar->m_bHasNeverBeenLoaded = false;
  }

  LoadCVarsFromFileInternal(sPath, allCVars, bSetAsCurrentValue, pOutCVars);
}

void xiiCVar::LoadCVarsFromFileInternal(xiiStringView path, const xiiDynamicArray<xiiCVar*>& vars, bool bSetAsCurrentValue, xiiDynamicArray<xiiCVar*>* pOutCVars)
{
  xiiFileReader    File;
  xiiStringBuilder sTemp;

  if (File.Open(path.GetData(sTemp)) == XII_SUCCESS)
  {
    xiiStringBuilder sContent;
    sContent.ReadAll(File);

    xiiDynamicArray<xiiString> Lines;
    sContent.ReplaceAll("\r", ""); // remove carriage return

    // splits the string at occurrence of '\n' and adds each line to the 'Lines' container
    sContent.Split(true, Lines, "\n");

    xiiStringBuilder sVarName;
    xiiStringBuilder sVarValue;

    for (const xiiString& sLine : Lines)
    {
      if (ParseLine(sLine, sVarName, sVarValue) == XII_FAILURE)
        continue;

      // now find a variable with the same name
      for (xiiUInt32 var = 0; var < vars.GetCount(); ++var)
      {
        xiiCVar* pCVar = vars[var];

        if (!sVarName.IsEqual(pCVar->GetName()))
          continue;

        // found the cvar, now convert the text into the proper value *sigh*
        switch (pCVar->GetType())
        {
          case xiiCVarType::Int:
          {
            xiiInt32 Value = 0;
            if (xiiConversionUtils::StringToInt(sVarValue, Value).Succeeded())
            {
              xiiCVarInt* pTyped                     = (xiiCVarInt*)pCVar;
              pTyped->m_Values[xiiCVarValue::Stored] = Value;
              *pTyped                                = Value;
            }
          }
          break;
          case xiiCVarType::Bool:
          {
            bool Value = sVarValue.IsEqual_NoCase("true");

            xiiCVarBool* pTyped                    = (xiiCVarBool*)pCVar;
            pTyped->m_Values[xiiCVarValue::Stored] = Value;
            *pTyped                                = Value;
          }
          break;
          case xiiCVarType::Float:
          {
            double Value = 0.0;
            if (xiiConversionUtils::StringToFloat(sVarValue, Value).Succeeded())
            {
              xiiCVarFloat* pTyped                   = (xiiCVarFloat*)pCVar;
              pTyped->m_Values[xiiCVarValue::Stored] = static_cast<float>(Value);
              *pTyped                                = static_cast<float>(Value);
            }
          }
          break;
          case xiiCVarType::Double:
          {
            double Value = 0.0;
            if (xiiConversionUtils::StringToFloat(sVarValue, Value).Succeeded())
            {
              xiiCVarDouble* pTyped                  = (xiiCVarDouble*)pCVar;
              pTyped->m_Values[xiiCVarValue::Stored] = Value;
              *pTyped                                = Value;
            }
          }
          break;
          case xiiCVarType::String:
          {
            const char* Value = sVarValue.GetData();

            xiiCVarString* pTyped                  = (xiiCVarString*)pCVar;
            pTyped->m_Values[xiiCVarValue::Stored] = Value;
            *pTyped                                = Value;
          }
          break;
          default:
            XII_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
            break;
        }

        if (pOutCVars)
        {
          pOutCVars->PushBack(pCVar);
        }

        if (bSetAsCurrentValue)
        {
          pCVar->SetToDelayedSyncValue();
        }
      }
    }
  }
}

// clang-format off
xiiCommandLineOptionDoc opt_CVar("cvar", "-CVarName", "<value>", "Forces a CVar to the given value.\n\
Overrides persisted settings.\n\
Examples:\n\
-MyIntVar 42\n\
-MyStringVar \"Hello\"\n\
",
nullptr);
// clang-format on

void xiiCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/, xiiDynamicArray<xiiCVar*>* pOutCVars /*= nullptr*/)
{
  xiiStringBuilder sTemp;

  for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (xiiCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
      if (pOutCVars)
      {
        pOutCVars->PushBack(pCVar);
      }

      // has been specified on the command line -> mark it as 'has been loaded'
      pCVar->m_bHasNeverBeenLoaded = false;

      switch (pCVar->GetType())
      {
        case xiiCVarType::Int:
        {
          xiiCVarInt* pTyped = (xiiCVarInt*)pCVar;
          xiiInt32    Value  = pTyped->m_Values[xiiCVarValue::Stored];
          Value              = xiiCommandLineUtils::GetGlobalInstance()->GetIntOption(sTemp, Value);

          pTyped->m_Values[xiiCVarValue::Stored] = Value;
          *pTyped                                = Value;
        }
        break;
        case xiiCVarType::Bool:
        {
          xiiCVarBool* pTyped = (xiiCVarBool*)pCVar;
          bool         Value  = pTyped->m_Values[xiiCVarValue::Stored];
          Value               = xiiCommandLineUtils::GetGlobalInstance()->GetBoolOption(sTemp, Value);

          pTyped->m_Values[xiiCVarValue::Stored] = Value;
          *pTyped                                = Value;
        }
        break;
        case xiiCVarType::Float:
        {
          xiiCVarFloat* pTyped = (xiiCVarFloat*)pCVar;
          double        Value  = pTyped->m_Values[xiiCVarValue::Stored];
          Value                = xiiCommandLineUtils::GetGlobalInstance()->GetFloatOption(sTemp, Value);

          pTyped->m_Values[xiiCVarValue::Stored] = static_cast<float>(Value);
          *pTyped                                = static_cast<float>(Value);
        }
        break;
        case xiiCVarType::Double:
        {
          xiiCVarDouble* pTyped = (xiiCVarDouble*)pCVar;
          double         Value  = pTyped->m_Values[xiiCVarValue::Stored];
          Value                 = xiiCommandLineUtils::GetGlobalInstance()->GetFloatOption(sTemp, Value);

          pTyped->m_Values[xiiCVarValue::Stored] = Value;
          *pTyped                                = Value;
        }
        break;
        case xiiCVarType::String:
        {
          xiiCVarString* pTyped = (xiiCVarString*)pCVar;
          xiiString      Value  = xiiCommandLineUtils::GetGlobalInstance()->GetStringOption(sTemp, 0, pTyped->m_Values[xiiCVarValue::Stored]);

          pTyped->m_Values[xiiCVarValue::Stored] = Value;
          *pTyped                                = Value;
        }
        break;
        default:
          XII_REPORT_FAILURE("Unknown CVar Type: {0}", pCVar->GetType());
          break;
      }

      if (bSetAsCurrentValue)
        pCVar->SetToDelayedSyncValue();
    }
  }
}

void xiiCVar::ListOfCVarsChanged(xiiStringView sSetPluginNameTo)
{
  AssignSubSystemPlugin(sSetPluginNameTo);

  LoadCVars();

  xiiCVarEvent e(nullptr);
  e.m_EventType = xiiCVarEvent::Type::ListOfVarsChanged;

  s_AllCVarEvents.Broadcast(e);
}

XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
