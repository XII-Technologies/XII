#include <GraphicsFoundation/GraphicsFoundationDLL.h>

#include <Foundation/CodeUtils/Preprocessor.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderParser.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>

bool      xiiGALShaderManager::s_bEnableRuntimeCompilation = false;
xiiString xiiGALShaderManager::s_sPlatform;
xiiString xiiGALShaderManager::s_sPermutationVariableSubDirectory;
xiiString xiiGALShaderManager::s_sShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    xiiHashedString                                                           m_sName;
    xiiVariant                                                                m_DefaultValue;
    xiiDynamicArray<xiiGALShaderParser::EnumValue, xiiStaticAllocatorWrapper> m_EnumValues;
  };

  static xiiDeque<PermutationVarConfig, xiiStaticAllocatorWrapper> s_PermutationVarConfigurationsStorage;
  static xiiHashTable<xiiHashedString, PermutationVarConfig*>      s_PermutationVarConfigurations;
  static xiiMutex                                                  s_PermutationVarConfigurationsMutex;

  const PermutationVarConfig* FindConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
  {
    XII_LOCK(s_PermutationVarConfigurationsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigurations.TryGetValue(sHashedName, pConfig))
    {
      xiiGALShaderManager::ReloadPermutationVarConfig(sName, sHashedName);
      s_PermutationVarConfigurations.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const xiiHashedString& sName)
  {
    XII_LOCK(s_PermutationVarConfigurationsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigurations.TryGetValue(sName, pConfig))
    {
      xiiGALShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigurations.TryGetValue(sName, pConfig);
    }

    return pConfig;
  }

  static xiiHashedString s_sTrue  = xiiMakeHashedString("TRUE");
  static xiiHashedString s_sFalse = xiiMakeHashedString("FALSE");

  bool IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue, xiiHashedString& out_sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      if (sValue == s_sTrue)
      {
        out_sValue = s_sTrue;
        return true;
      }

      if (sValue == s_sFalse)
      {
        out_sValue = s_sFalse;
        return true;
      }
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
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

  bool IsValueAllowed(const PermutationVarConfig& config, const xiiTempHashedString& sValue)
  {
    if (config.m_DefaultValue.IsA<bool>())
    {
      return sValue == s_sTrue || sValue == s_sFalse;
    }
    else
    {
      for (auto& enumValue : config.m_EnumValues)
      {
        if (enumValue.m_sValueName == sValue)
          return true;
      }
    }

    return false;
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

void xiiGALShaderManager::Configure(xiiStringView sActivePlatform, bool bEnableRuntimeCompilation, xiiStringView sShaderCacheDirectory, xiiStringView sPermutationVariableSubDirectory)
{
  xiiStringBuilder sb = sActivePlatform;
  sb.ToUpper();

  s_sPlatform                        = sb;
  s_bEnableRuntimeCompilation        = bEnableRuntimeCompilation;
  s_sShaderCacheDirectory            = sShaderCacheDirectory;
  s_sPermutationVariableSubDirectory = sPermutationVariableSubDirectory;
}

void xiiGALShaderManager::ReloadPermutationVarConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
{
  // clear earlier data
  {
    XII_LOCK(s_PermutationVarConfigurationsMutex);

    s_PermutationVarConfigurations.Remove(sHashedName);
  }

  xiiStringBuilder sPath;
  sPath.SetFormat("{0}/{1}.xiiPermVar", s_sPermutationVariableSubDirectory, sName);

  xiiStringBuilder sTemp = s_sPlatform;
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
    XII_LOCK(s_PermutationVarConfigurationsMutex);

    auto pConfig = &s_PermutationVarConfigurationsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(sName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues   = enumDefinition.m_Values;

    s_PermutationVarConfigurations.Insert(pConfig->m_sName, pConfig);
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
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    xiiLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue.GetHash());
    ReloadPermutationVarConfig(sName, sHashedName);

    if (!IsValueAllowed(*pConfig, sValue, out_sValue))
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
    if (!s_bEnableRuntimeCompilation)
    {
      return false;
    }

    xiiLog::Debug("Invalid Shader Permutation: '{0}' cannot be set to value '{1}' -> reloading config for variable", sName, sValue);
    ReloadPermutationVarConfig(sName, sName);

    if (!IsValueAllowed(*pConfig, sValue))
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
    out_values.PushBack(s_sTrue);
    out_values.PushBack(s_sFalse);
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
        var.m_sValue = defaultValue.Get<bool>() ? s_sTrue : s_sFalse;
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
