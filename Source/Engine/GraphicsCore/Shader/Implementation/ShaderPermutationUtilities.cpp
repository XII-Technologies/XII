#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

namespace
{
  static xiiHashTable<xiiUInt64, xiiUntrackedString> s_PermutationPaths;
}

void xiiGALShaderPermutationUtilities::PreloadPermutations(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, xiiTime shouldBeAvailableIn)
{
  XII_ASSERT_NOT_IMPLEMENTED;
#if 0
  xiiResourceLock<xiiShaderResource> pShader(hShader, xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return;

  /*xiiUInt32 uiPermutationHash = */ FilterPermutationVars(pShader->GetUsedPermutationVars(), permVars);

  generator.RemoveUnusedPermutations(pShader->GetUsedPermutationVars());

  xiiHybridArray<xiiGALPermutationVariable, 16> usedPermVars;

  const xiiUInt32 uiPermutationCount = generator.GetPermutationCount();
  for (xiiUInt32 uiPermutation = 0; uiPermutation < uiPermutationCount; ++uiPermutation)
  {
    generator.GetPermutation(uiPermutation, usedPermVars);

    PreloadSingleShaderPermutation(hShader, usedPermVars, tShouldBeAvailableIn);
  }
#endif
}

xiiShaderPermutationResourceHandle xiiGALShaderPermutationUtilities::PreloadSinglePermutation(xiiShaderResourceHandle hShader, const xiiHashTable<xiiHashedString, xiiHashedString>& permVars, bool bAllowFallback)
{
  xiiResourceLock<xiiShaderResource> pShader(hShader, bAllowFallback ? xiiResourceAcquireMode::AllowLoadingFallback : xiiResourceAcquireMode::BlockTillLoaded);

  if (!pShader->IsShaderValid())
    return xiiShaderPermutationResourceHandle();

  xiiHybridArray<xiiGALPermutationVariable, 64> filteredPermutationVariables(xiiFrameAllocator::GetCurrentAllocator());
  xiiUInt32                                     uiPermutationHash = xiiGALShaderManager::FilterPermutationVariables(pShader->GetUsedPermutationVars(), permVars, filteredPermutationVariables);

  return PreloadSinglePermutationInternal(pShader->GetResourceID(), pShader->GetResourceIDHash(), uiPermutationHash, filteredPermutationVariables);

  return xiiShaderPermutationResourceHandle();
}

xiiShaderPermutationResourceHandle xiiGALShaderPermutationUtilities::PreloadSinglePermutationInternal(xiiStringView sResourceId, xiiUInt64 uiResourceIdHash, xiiUInt32 uiPermutationHash, xiiArrayPtr<xiiGALPermutationVariable> filteredPermutationVariables)
{
  const xiiUInt64 uiPermutationKey = (xiiUInt64)xiiHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  xiiUntrackedString& permutationPath = s_PermutationPaths[uiPermutationKey];
  if (permutationPath.IsEmpty())
  {
    xiiStringBuilder sShaderFile = xiiGALShaderManager::GetCacheDirectory();
    sShaderFile.AppendPath(xiiGALShaderManager::GetActivePlatform());
    sShaderFile.AppendPath(sResourceId);
    sShaderFile.ChangeFileExtension("");
    if (sShaderFile.EndsWith("."))
      sShaderFile.Shrink(0, 1);
    sShaderFile.AppendFormat("_{0}.xiiPermutation", xiiArgU(uiPermutationHash, 8, true, 16, true));

    permutationPath = sShaderFile;
  }

  xiiShaderPermutationResourceHandle hShaderPermutation = xiiResourceManager::LoadResource<xiiShaderPermutationResource>(permutationPath);

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
