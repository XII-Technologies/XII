/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

namespace
{
  static xiiHashTable<xiiUInt64, xiiUntrackedString> s_PermutationPaths;
}

xiiShaderPermutationResourceHandle xiiShaderPermutationUtilities::PreloadSinglePermutation(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, bool bAllowFallback)
{
  xiiResourceLock<xiiShaderResource> pShader(hShader, bAllowFallback ? xiiResourceAcquireMode::AllowLoadingFallback : xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return xiiShaderPermutationResourceHandle();

  xiiTemporaryHybridArray<xiiGALPermutationVariable, 32> filteredPermutationVariables;
  xiiUInt32                                              uiPermutationHash = xiiGALShaderManager::FilterPermutationVariables(pShader->GetUsedPermutationVariables(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);
}

xiiShaderPermutationResourceHandle xiiShaderPermutationUtilities::PreloadSinglePermutationInternal(xiiStringView sResourceId, xiiUInt64 uiResourceIdHash, xiiUInt32 uiPermutationHash, xiiArrayPtr<xiiGALPermutationVariable> filteredPermutationVariables)
{
  const xiiUInt64 uiPermutationKey = (xiiUInt64)xiiHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  xiiUntrackedString& sPermutationPath = s_PermutationPaths[uiPermutationKey];
  if (sPermutationPath.IsEmpty())
  {
    xiiStringBuilder sShaderFile = xiiGALShaderManager::GetCacheDirectory();
    sShaderFile.AppendPath(xiiGALShaderManager::GetActivePlatform());
    sShaderFile.AppendPath(sResourceId);
    sShaderFile.ChangeFileExtension("");

    if (sShaderFile.EndsWith("."))
    {
      sShaderFile.Shrink(0, 1);
    }

    sShaderFile.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    sPermutationPath = sShaderFile;
  }

  xiiShaderPermutationResourceHandle hShaderPermutation = xiiResourceManager::LoadResource<xiiShaderPermutationResource>(sPermutationPath);

  {
    xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hShaderPermutation, xiiResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVariables = filteredPermutationVariables;
    }
  }

  xiiResourceManager::PreloadResource(hShaderPermutation);

  return hShaderPermutation;
}
