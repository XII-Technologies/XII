/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/CodeUtils/Preprocessor.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>

struct xiiGALShaderManager::PermutationVarConfig
{
  xiiHashedString                                m_sName;
  xiiVariant                                     m_DefaultValue;
  xiiDynamicArray<xiiGALShaderParser::EnumValue> m_EnumValues;
};

class xiiGALShaderManager::State
{
public:
  xiiMutex                                             m_Mutex;
  xiiDeque<PermutationVarConfig>                       m_PermutationConfigurationsStorage;
  xiiHashTable<xiiHashedString, PermutationVarConfig*> m_PermutationConfigurations;
  xiiHashedString                                      m_sTrue;
  xiiHashedString                                      m_sFalse;
  xiiString                                            m_sPlatform;
  xiiString                                            m_sPermutationVariableSubDirectory;
  xiiString                                            m_sShaderCacheDirectory;
  bool                                                 m_bEnableRuntimeCompilation = false;
};

xiiUniquePtr<xiiGALShaderManager::State> xiiGALShaderManager::s_pState;

void xiiGALShaderManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Shader manager started twice.");
  s_pState = XII_DEFAULT_NEW(State);
  s_pState->m_sTrue.Assign("TRUE");
  s_pState->m_sFalse.Assign("FALSE");
}

void xiiGALShaderManager::Shutdown()
{
  s_pState.Clear();
}

bool xiiGALShaderManager::IsInitialized()
{
  return s_pState != nullptr;
}

const xiiGALShaderManager::PermutationVarConfig* xiiGALShaderManager::FindConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  if (s_pState == nullptr)
    return nullptr;

  {
    XII_LOCK(s_pState->m_Mutex);
    PermutationVarConfig* pConfig = nullptr;
    if (s_pState->m_PermutationConfigurations.TryGetValue(sHashedName, pConfig))
      return pConfig;
  }

  ReloadPermutationVarConfig(sName, sHashedName);

  XII_LOCK(s_pState->m_Mutex);
  PermutationVarConfig* pConfig = nullptr;
  s_pState->m_PermutationConfigurations.TryGetValue(sHashedName, pConfig);
  return pConfig;
}

const xiiGALShaderManager::PermutationVarConfig* xiiGALShaderManager::FindConfig(const xiiHashedString& sName)
{
  return FindConfig(sName.GetData(), sName);
}

bool xiiGALShaderManager::IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue, xiiHashedString& out_sValue)
{
  if (config.m_DefaultValue.IsA<bool>())
  {
    if (sValue == s_pState->m_sTrue)
    {
      out_sValue = s_pState->m_sTrue;
      return true;
    }

    if (sValue == s_pState->m_sFalse)
    {
      out_sValue = s_pState->m_sFalse;
      return true;
    }
  }
  else
  {
    for (const auto& enumValue : config.m_EnumValues)
    {
      if (enumValue.m_sValueName == sValue)
      {
        out_sValue = enumValue.m_sValueName;
        return true;
      }
    }
  }

  return false;
}

bool xiiGALShaderManager::IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue)
{
  if (config.m_DefaultValue.IsA<bool>())
    return sValue == s_pState->m_sTrue || sValue == s_pState->m_sFalse;

  for (const auto& enumValue : config.m_EnumValues)
  {
    if (enumValue.m_sValueName == sValue)
      return true;
  }

  return false;
}

//////////////////////////////////////////////////////////////////////////

void xiiGALShaderManager::Configure(xiiStringView sActivePlatform, bool bEnableRuntimeCompilation, xiiStringView sShaderCacheDirectory, xiiStringView sPermutationVariableSubDirectory)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  if (s_pState == nullptr)
    return;

  xiiStringBuilder sb = sActivePlatform;
  sb.ToUpper();

  XII_LOCK(s_pState->m_Mutex);
  s_pState->m_sPlatform                        = sb;
  s_pState->m_bEnableRuntimeCompilation        = bEnableRuntimeCompilation;
  s_pState->m_sShaderCacheDirectory            = sShaderCacheDirectory;
  s_pState->m_sPermutationVariableSubDirectory = sPermutationVariableSubDirectory;
  s_pState->m_PermutationConfigurations.Clear();
  s_pState->m_PermutationConfigurationsStorage.Clear();
}

const xiiString& xiiGALShaderManager::GetPermutationVarSubDirectory()
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  return s_pState->m_sPermutationVariableSubDirectory;
}

const xiiString& xiiGALShaderManager::GetActivePlatform()
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  return s_pState->m_sPlatform;
}

const xiiString& xiiGALShaderManager::GetCacheDirectory()
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  return s_pState->m_sShaderCacheDirectory;
}

bool xiiGALShaderManager::IsRuntimeCompilationEnabled()
{
  return s_pState != nullptr && s_pState->m_bEnableRuntimeCompilation;
}

void xiiGALShaderManager::ReloadPermutationVarConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
{
  XII_ASSERT_DEV(s_pState != nullptr, "The shader manager subsystem is not started.");
  if (s_pState == nullptr)
    return;

  // clear earlier data
  {
    XII_LOCK(s_pState->m_Mutex);

    s_pState->m_PermutationConfigurations.Remove(sHashedName);
  }

  xiiStringBuilder sPath;
  sPath.SetFormat("{0}/{1}.xiiPermVar", s_pState->m_sPermutationVariableSubDirectory, sName);

  xiiStringBuilder sTemp = s_pState->m_sPlatform;
  sTemp.Append(" 1");

  xiiPreprocessor pp;
  pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetView()).IgnoreResult();

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    xiiLog::Error("Could not read shader permutation variable '{0}' from file '{1}'.", sName, sPath);
  }

  xiiVariant                         defaultValue;
  xiiGALShaderParser::EnumDefinition enumDefinition;

  xiiGALShaderParser::ParsePermutationVariableConfiguration(sTemp, defaultValue, enumDefinition);
  if (defaultValue.IsValid())
  {
    XII_LOCK(s_pState->m_Mutex);

    auto pConfig = &s_pState->m_PermutationConfigurationsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(sName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues   = enumDefinition.m_Values;

    s_pState->m_PermutationConfigurations.Insert(pConfig->m_sName, pConfig);
  }
}

bool xiiGALShaderManager::IsPermutationValueAllowed(xiiStringView sName, const xiiTempHashedString& sHashedName, const xiiTempHashedString& sValue, xiiHashedString& out_sName, xiiHashedString& out_sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName, sHashedName);
  if (pConfig == nullptr)
  {
    xiiLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  out_sName = pConfig->m_sName;

  if (!IsValueAllowed(*pConfig, sValue, out_sValue))
  {
    if (!s_pState->m_bEnableRuntimeCompilation)
    {
      return false;
    }

    xiiLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue.GetHash());
    ReloadPermutationVarConfig(sName, sHashedName);
    pConfig = FindConfig(sName, sHashedName);

    if (pConfig == nullptr || !IsValueAllowed(*pConfig, sValue, out_sValue))
    {
      xiiLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue.GetHash());
      return false;
    }
  }

  return true;
}

bool xiiGALShaderManager::IsPermutationValueAllowed(const xiiHashedString& sName, const xiiHashedString& sValue)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
  {
    xiiLog::Error("Permutation variable '{0}' does not exist", sName);
    return false;
  }

  if (!IsValueAllowed(*pConfig, sValue))
  {
    if (!s_pState->m_bEnableRuntimeCompilation)
    {
      return false;
    }

    xiiLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);
    pConfig = FindConfig(sName);

    if (pConfig == nullptr || !IsValueAllowed(*pConfig, sValue))
    {
      xiiLog::Error("Invalid Shader Permutation: '{0}' cannot be set to value '{1}'", sName, sValue);
      return false;
    }
  }

  return true;
}

void xiiGALShaderManager::GetPermutationValues(const xiiHashedString& sName, xiiDynamicArray<xiiHashedString>& out_values)
{
  out_values.Clear();

  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig == nullptr)
    return;

  if (pConfig->m_DefaultValue.IsA<bool>())
  {
    out_values.PushBack(s_pState->m_sTrue);
    out_values.PushBack(s_pState->m_sFalse);
  }
  else
  {
    for (const auto& val : pConfig->m_EnumValues)
    {
      out_values.PushBack(val.m_sValueName);
    }
  }
}

xiiArrayPtr<const xiiGALShaderParser::EnumValue> xiiGALShaderManager::GetPermutationEnumValues(const xiiHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return {};
}

xiiUInt32 xiiGALShaderManager::FilterPermutationVariables(xiiArrayPtr<const xiiHashedString> usedVariables, const xiiHashTable<xiiHashedString, xiiHashedString>& permutationVariables, xiiDynamicArray<xiiGALPermutationVariable>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVariables)
  {
    auto& var   = out_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permutationVariables.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfiguration = FindConfig(sName);
      if (pConfiguration == nullptr)
        continue;

      const xiiVariant& defaultValue = pConfiguration->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue = defaultValue.Get<bool>() ? s_pState->m_sTrue : s_pState->m_sFalse;
      }
      else
      {
        xiiUInt32 uiDefaultValue = defaultValue.Get<xiiUInt32>();
        var.m_sValue             = pConfiguration->m_EnumValues[uiDefaultValue].m_sValueName;
      }
    }
  }

  return xiiGALPermutationVariable::CalculateHash(out_FilteredPermutationVariables);
}

XII_STATICLINK_FILE(GraphicsFoundation, GraphicsFoundation_ShaderCompiler_Implementation_ShaderManager);
