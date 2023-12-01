#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <GraphicsCore/Shader/Implementation/Helper.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/ShaderCompiler/ShaderManager.h>
#include <GraphicsCore/ShaderCompiler/ShaderParser.h>

bool      xiiShaderManager::s_bEnableRuntimeCompilation = false;
xiiString xiiShaderManager::s_sPlatform;
xiiString xiiShaderManager::s_sPermVarSubDir;
xiiString xiiShaderManager::s_sShaderCacheDirectory;

namespace
{
  struct PermutationVarConfig
  {
    xiiHashedString                                                        m_sName;
    xiiVariant                                                             m_DefaultValue;
    xiiDynamicArray<xiiShaderParser::EnumValue, xiiStaticAllocatorWrapper> m_EnumValues;
  };

  static xiiDeque<PermutationVarConfig, xiiStaticAllocatorWrapper> s_PermutationVarConfigsStorage;
  static xiiHashTable<xiiHashedString, PermutationVarConfig*>      s_PermutationVarConfigs;
  static xiiMutex                                                  s_PermutationVarConfigsMutex;

  const PermutationVarConfig* FindConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
  {
    XII_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig))
    {
      xiiShaderManager::ReloadPermutationVarConfig(sName, sHashedName);
      s_PermutationVarConfigs.TryGetValue(sHashedName, pConfig);
    }

    return pConfig;
  }

  const PermutationVarConfig* FindConfig(const xiiHashedString& sName)
  {
    XII_LOCK(s_PermutationVarConfigsMutex);

    PermutationVarConfig* pConfig = nullptr;
    if (!s_PermutationVarConfigs.TryGetValue(sName, pConfig))
    {
      xiiShaderManager::ReloadPermutationVarConfig(sName.GetData(), sName);
      s_PermutationVarConfigs.TryGetValue(sName, pConfig);
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

  static xiiHashTable<xiiUInt64, xiiString> s_PermutationPaths;
} // namespace

//////////////////////////////////////////////////////////////////////////

void xiiShaderManager::Configure(xiiStringView sActivePlatform, bool bEnableRuntimeCompilation, xiiStringView sShaderCacheDirectory, xiiStringView sPermVarSubDirectory)
{
  s_sShaderCacheDirectory = sShaderCacheDirectory;
  s_sPermVarSubDir        = sPermVarSubDirectory;

  xiiStringBuilder s = sActivePlatform;
  s.ToUpper();

  s_bEnableRuntimeCompilation = bEnableRuntimeCompilation;
  s_sPlatform                 = s;
}

void xiiShaderManager::ReloadPermutationVarConfig(xiiStringView sName, const xiiTempHashedString& sHashedName)
{
  // clear earlier data
  {
    XII_LOCK(s_PermutationVarConfigsMutex);

    s_PermutationVarConfigs.Remove(sHashedName);
  }

  xiiStringBuilder sPath;
  sPath.Format("{0}/{1}.xiiPermVar", s_sPermVarSubDir, sName);

  xiiStringBuilder sTemp = s_sPlatform;
  sTemp.Append(" 1");

  xiiPreprocessor pp;
  pp.SetLogInterface(xiiLog::GetThreadLocalLogSystem());
  pp.SetPassThroughLine(false);
  pp.SetPassThroughPragma(false);
  pp.AddCustomDefine(sTemp.GetData()).IgnoreResult();

  if (pp.Process(sPath, sTemp, false).Failed())
  {
    xiiLog::Error("Could not read shader permutation variable '{0}' from file '{1}'.", sName, sPath);
  }

  xiiVariant                      defaultValue;
  xiiShaderParser::EnumDefinition enumDef;

  xiiShaderParser::ParsePermutationVarConfig(sTemp, defaultValue, enumDef);
  if (defaultValue.IsValid())
  {
    XII_LOCK(s_PermutationVarConfigsMutex);

    auto pConfig = &s_PermutationVarConfigsStorage.ExpandAndGetRef();
    pConfig->m_sName.Assign(sName);
    pConfig->m_DefaultValue = defaultValue;
    pConfig->m_EnumValues   = enumDef.m_Values;

    s_PermutationVarConfigs.Insert(pConfig->m_sName, pConfig);
  }
}

bool xiiShaderManager::IsPermutationValueAllowed(xiiStringView sName, const xiiTempHashedString& sHashedName, const xiiTempHashedString& sValue, xiiHashedString& out_sName, xiiHashedString& out_sValue)
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

bool xiiShaderManager::IsPermutationValueAllowed(const xiiHashedString& sName, const xiiHashedString& sValue)
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

void xiiShaderManager::GetPermutationValues(const xiiHashedString& sName, xiiDynamicArray<xiiHashedString>& out_values)
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

xiiArrayPtr<const xiiShaderParser::EnumValue> xiiShaderManager::GetPermutationEnumValues(const xiiHashedString& sName)
{
  const PermutationVarConfig* pConfig = FindConfig(sName);
  if (pConfig != nullptr)
  {
    return pConfig->m_EnumValues;
  }

  return {};
}

void xiiShaderManager::PreloadPermutations(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, xiiTime shouldBeAvailableIn)
{
  XII_ASSERT_NOT_IMPLEMENTED;
#if 0
  xiiResourceLock<xiiShaderResource> pShader(hShader, xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*xiiUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  xiiHybridArray<xiiPermutationVar, 16> usedPermVars;

  const xiiUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (xiiUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

xiiShaderPermutationResourceHandle xiiShaderManager::PreloadSinglePermutation(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, bool bAllowFallback)
{
  xiiResourceLock<xiiShaderResource> pShader(hShader, bAllowFallback ? xiiResourceAcquireMode::AllowLoadingFallback : xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return xiiShaderPermutationResourceHandle();

  xiiHybridArray<xiiPermutationVar, 64> filteredPermutationVariables(xiiFrameAllocator::GetCurrentAllocator());
  xiiUInt32                             uiPermutationHash = FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}


xiiUInt32 xiiShaderManager::FilterPermutationVars(xiiArrayPtr<const xiiHashedString> usedVars, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, xiiDynamicArray<xiiPermutationVar>& out_FilteredPermutationVariables)
{
  for (auto& sName : usedVars)
  {
    auto& var   = out_FilteredPermutationVariables.ExpandAndGetRef();
    var.m_sName = sName;

    if (!permVars.TryGetValue(sName, var.m_sValue))
    {
      const PermutationVarConfig* pConfig = FindConfig(sName);
      if (pConfig == nullptr)
        continue;

      const xiiVariant& defaultValue = pConfig->m_DefaultValue;
      if (defaultValue.IsA<bool>())
      {
        var.m_sValue = defaultValue.Get<bool>() ? s_sTrue : s_sFalse;
      }
      else
      {
        xiiUInt32 uiDefaultValue = defaultValue.Get<xiiUInt32>();
        var.m_sValue             = pConfig->m_EnumValues[uiDefaultValue].m_sValueName;
      }
    }
  }

  return xiiShaderHelper::CalculateHash(out_FilteredPermutationVariables);
}

xiiShaderPermutationResourceHandle xiiShaderManager::PreloadSinglePermutationInternal(xiiStringView sResourceId, xiiUInt64 uiResourceIdHash, xiiUInt32 uiPermutationHash, xiiArrayPtr<xiiPermutationVar> filteredPermutationVariables)
{
  const xiiUInt64 uiPermutationKey = (xiiUInt64)xiiHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  xiiString* pPermutationPath = &s_PermutationPaths[uiPermutationKey];
  if (pPermutationPath->IsEmpty())
  {
    xiiStringBuilder sShaderFile = GetCacheDirectory();
    sShaderFile.AppendPath(GetActivePlatform().GetData());
    sShaderFile.AppendPath(sResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    *pPermutationPath = sShaderFile;
  }

  xiiShaderPermutationResourceHandle hShaderPermutation = xiiResourceManager::LoadResource<xiiShaderPermutationResource>(pPermutationPath->GetData());

  {
    xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hShaderPermutation, xiiResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVars = filteredPermutationVariables;
    }
  }

  xiiResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_ShaderCompiler_Implementation_ShaderManager);
