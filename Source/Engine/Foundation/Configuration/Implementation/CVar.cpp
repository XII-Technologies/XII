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

  // for saving and loading we need the filesystem, so make sure we are initialized after
  // and shutdown before the filesystem is
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "FileSystem"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiPlugin::Events().AddEventHandler(xiiCVar::PluginEventHandler);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    // save the CVars every time the core is shut down
    // at this point the filesystem might already be uninitialized by the user (data dirs)
    // in that case the variables cannot be saved, but it will fail silently
    // if it succeeds, the most recent state will be serialized though
    xiiCVar::SaveCVars();

    xiiPlugin::Events().RemoveEventHandler(xiiCVar::PluginEventHandler);
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    // save the CVars every time the engine is shut down
    // at this point the filesystem should usually still be configured properly
    xiiCVar::SaveCVars();
  }

  // The user is responsible to call 'xiiCVar::SetStorageFolder' to define where the CVars are
  // actually stored. That call will automatically load all CVar states.

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


xiiString                     xiiCVar::s_sStorageFolder;
xiiEvent<const xiiCVarEvent&> xiiCVar::s_AllCVarEvents;

void xiiCVar::AssignSubSystemPlugin(const char* szPluginName)
{
  xiiCVar* pCVar = xiiCVar::GetFirstInstance();

  while (pCVar)
  {
    if (pCVar->m_szPluginName == nullptr)
      pCVar->m_szPluginName = szPluginName;

    pCVar = pCVar->GetNextInstance();
  }
}

void xiiCVar::PluginEventHandler(const xiiPluginEvent& EventData)
{
  switch (EventData.m_EventType)
  {
    case xiiPluginEvent::BeforeLoading:
    {
      // before a new plugin is loaded, make sure all currently available CVars
      // are assigned to the proper plugin
      // all not-yet assigned cvars cannot be in any plugin, so assign them to the 'static' plugin
      AssignSubSystemPlugin("Static");
    }
    break;

    case xiiPluginEvent::AfterLoadingBeforeInit:
    {
      // after we loaded a new plugin, but before it is initialized,
      // find all new CVars and assign them to that new plugin
      AssignSubSystemPlugin(EventData.m_szPluginBinary);

      // now load the state of all CVars
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

xiiCVar::xiiCVar(const char* szName, xiiBitflags<xiiCVarFlags> Flags, const char* szDescription)
{
  m_szPluginName        = nullptr; // will be filled out when plugins are loaded
  m_bHasNeverBeenLoaded = true;    // next time 'LoadCVars' is called, its state will be changed

  m_szName        = szName;
  m_Flags         = Flags;
  m_szDescription = szDescription;

  // 'RequiresRestart' only works together with 'Save'
  if (m_Flags.IsAnySet(xiiCVarFlags::RequiresRestart))
    m_Flags.Add(xiiCVarFlags::Save);

  XII_ASSERT_DEV(!xiiStringUtils::IsNullOrEmpty(m_szDescription), "Please add a useful description for CVar '{}'.", szName);
}

xiiCVar* xiiCVar::FindCVarByName(const char* szName)
{
  xiiCVar* pCVar = xiiCVar::GetFirstInstance();

  while (pCVar)
  {
    if (xiiStringUtils::IsEqual(pCVar->GetName(), szName))
      return pCVar;

    pCVar = pCVar->GetNextInstance();
  }

  return nullptr;
}

void xiiCVar::SetStorageFolder(const char* szFolder)
{
  s_sStorageFolder = szFolder;
}

xiiCommandLineOptionBool opt_NoFileCVars("cvar", "-no-file-cvars", "Disables loading CVar values from the user-specific, persisted configuration file.", false);

void xiiCVar::SaveCVars()
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  // first gather all the cvars by plugin
  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>> PluginCVars;

  {
    xiiCVar* pCVar = xiiCVar::GetFirstInstance();
    while (pCVar)
    {
      // only store cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
      {
        if (pCVar->m_szPluginName != nullptr)
          PluginCVars[pCVar->m_szPluginName].PushBack(pCVar);
        else
          PluginCVars["Static"].PushBack(pCVar);
      }

      pCVar = pCVar->GetNextInstance();
    }
  }

  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

  xiiStringBuilder sTemp;

  // now save all cvars in their plugin specific file
  while (it.IsValid())
  {
    // create the plugin specific file
    sTemp.Format("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

    xiiFileWriter File;
    if (File.Open(sTemp.GetData()) == XII_SUCCESS)
    {
      // write one line for each cvar, to save its current value
      for (xiiUInt32 var = 0; var < it.Value().GetCount(); ++var)
      {
        xiiCVar* pCVar = it.Value()[var];

        switch (pCVar->GetType())
        {
          case xiiCVarType::Int:
          {
            xiiCVarInt* pInt = (xiiCVarInt*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pInt->GetValue(xiiCVarValue::Restart));
          }
          break;
          case xiiCVarType::Bool:
          {
            xiiCVarBool* pBool = (xiiCVarBool*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pBool->GetValue(xiiCVarValue::Restart) ? "true" : "false");
          }
          break;
          case xiiCVarType::Float:
          {
            xiiCVarFloat* pFloat = (xiiCVarFloat*)pCVar;
            sTemp.Format("{0} = {1}\n", pCVar->GetName(), pFloat->GetValue(xiiCVarValue::Restart));
          }
          break;
          case xiiCVarType::String:
          {
            xiiCVarString* pString = (xiiCVarString*)pCVar;
            sTemp.Format("{0} = \"{1}\"\n", pCVar->GetName(), pString->GetValue(xiiCVarValue::Restart));
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

    // continue with the next plugin
    ++it;
  }
}

void xiiCVar::LoadCVars(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  LoadCVarsFromCommandLine(bOnlyNewOnes, bSetAsCurrentValue);
  LoadCVarsFromFile(bOnlyNewOnes, bSetAsCurrentValue);
}

static xiiResult ReadLine(xiiStreamReader& Stream, xiiStringBuilder& sLine)
{
  sLine.Clear();

  char c[2];
  c[0] = '\0';
  c[1] = '\0';

  // read the first character
  if (Stream.ReadBytes(c, 1) == 0)
    return XII_FAILURE;

  // skip all white-spaces at the beginning
  // also skip all empty lines
  while ((c[0] == '\n' || c[0] == '\r' || c[0] == ' ' || c[0] == '\t') && (Stream.ReadBytes(c, 1) > 0))
  {
  }

  // we found something that is not empty, so now read till the end of the line
  while (c[0] != '\0' && c[0] != '\n')
  {
    // skip all tabs and carriage returns
    if (c[0] != '\r' && c[0] != '\t')
    {
      sLine.Append(c);
    }

    // stop if we reached the end of the file
    if (Stream.ReadBytes(c, 1) == 0)
      break;
  }

  if (sLine.IsEmpty())
    return XII_FAILURE;

  return XII_SUCCESS;
}

static xiiResult ParseLine(const xiiStringBuilder& sLine, xiiStringBuilder& VarName, xiiStringBuilder& VarValue)
{
  const char* szSign = sLine.FindSubString("=");

  if (szSign == nullptr)
    return XII_FAILURE;

  {
    xiiStringView sSubString(sLine.GetData(), szSign);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);

    VarName = sSubString;
  }

  {
    xiiStringView sSubString(szSign + 1);

    // remove all spaces
    while (sSubString.StartsWith(" "))
      sSubString.Shrink(1, 0);

    // remove all trailing spaces
    while (sSubString.EndsWith(" "))
      sSubString.Shrink(0, 1);


    // remove " and start and end

    if (sSubString.StartsWith("\""))
      sSubString.Shrink(1, 0);

    if (sSubString.EndsWith("\""))
      sSubString.Shrink(0, 1);

    VarValue = sSubString;
  }

  return XII_SUCCESS;
}

void xiiCVar::LoadCVarsFromFile(bool bOnlyNewOnes, bool bSetAsCurrentValue)
{
  if (s_sStorageFolder.IsEmpty())
    return;

  // this command line disables loading and saving CVars to and from files
  if (opt_NoFileCVars.GetOptionValue(xiiCommandLineOption::LogMode::FirstTimeIfSpecified))
    return;

  xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>> PluginCVars;

  // first gather all the cvars by plugin
  {
    for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
    {
      // only load cvars that should be saved
      if (pCVar->GetFlags().IsAnySet(xiiCVarFlags::Save))
      {
        if (!bOnlyNewOnes || pCVar->m_bHasNeverBeenLoaded)
        {
          if (pCVar->m_szPluginName != nullptr)
            PluginCVars[pCVar->m_szPluginName].PushBack(pCVar);
          else
            PluginCVars["Static"].PushBack(pCVar);
        }
      }

      // it doesn't matter whether the CVar could be loaded from file, either it works the first time, or it stays at its current value
      pCVar->m_bHasNeverBeenLoaded = false;
    }
  }

  // now load all cvars from their plugin specific file
  {
    xiiMap<xiiString, xiiHybridArray<xiiCVar*, 128>>::Iterator it = PluginCVars.GetIterator();

    xiiStringBuilder sTemp;

    while (it.IsValid())
    {
      // create the plugin specific file
      sTemp.Format("{0}/CVars_{1}.cfg", s_sStorageFolder, it.Key());

      xiiFileReader File;
      if (File.Open(sTemp.GetData()) == XII_SUCCESS)
      {
        xiiStringBuilder sLine, sVarName, sVarValue;
        while (ReadLine(File, sLine) == XII_SUCCESS)
        {
          if (ParseLine(sLine, sVarName, sVarValue) == XII_FAILURE)
            continue;

          // now find a variable with the same name
          for (xiiUInt32 var = 0; var < it.Value().GetCount(); ++var)
          {
            xiiCVar* pCVar = it.Value()[var];

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

            if (bSetAsCurrentValue)
              pCVar->SetToRestartValue();
          }
        }
      }

      // continue with the next plugin
      ++it;
    }
  }
}

xiiCommandLineOptionDoc opt_CVar("cvar", "-CVarName", "<value>", "Forces a CVar to the given value.\n\
Overrides persisted settings.\n\
Examples:\n\
-MyIntVar 42\n\
-MyStringVar \"Hello\"\n\
",
                                 nullptr);

void xiiCVar::LoadCVarsFromCommandLine(bool bOnlyNewOnes /*= true*/, bool bSetAsCurrentValue /*= true*/)
{
  xiiStringBuilder sTemp;

  for (xiiCVar* pCVar = xiiCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    if (bOnlyNewOnes && !pCVar->m_bHasNeverBeenLoaded)
      continue;

    sTemp.Set("-", pCVar->GetName());

    if (xiiCommandLineUtils::GetGlobalInstance()->GetOptionIndex(sTemp) != -1)
    {
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
        pCVar->SetToRestartValue();
    }
  }
}

void xiiCVar::ListOfCVarsChanged(const char* szSetPluginNameTo)
{
  AssignSubSystemPlugin(szSetPluginNameTo);

  LoadCVars();

  xiiCVarEvent e(nullptr);
  e.m_EventType = xiiCVarEvent::Type::ListOfVarsChanged;

  s_AllCVarEvents.Broadcast(e);
}


XII_STATICLINK_FILE(Foundation, Foundation_Configuration_Implementation_CVar);
