/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/ShaderCompiler/ShaderManager.h>

class xiiShaderPermutationUtilitiesState
{
public:
  xiiMutex                                     m_Mutex;
  xiiHashTable<xiiUInt64, xiiUntrackedString> m_PermutationPaths;
};

xiiUniquePtr<xiiShaderPermutationUtilitiesState> xiiShaderPermutationUtilities::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, ShaderPermutationUtilities)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiShaderPermutationUtilities::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiShaderPermutationUtilities::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

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
  XII_ASSERT_DEV(s_pState != nullptr, "Shader permutation utilities are not started.");
  if (s_pState == nullptr)
    return {};

  const xiiUInt64 uiPermutationKey = (xiiUInt64)xiiHashingUtils::StringHashTo32(uiResourceIdHash) << 32 | uiPermutationHash;

  xiiUntrackedString sResolvedPermutationPath;
  {
    XII_LOCK(s_pState->m_Mutex);

    xiiUntrackedString& sPermutationPath = s_pState->m_PermutationPaths[uiPermutationKey];
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

    sResolvedPermutationPath = sPermutationPath;
  }

  xiiShaderPermutationResourceHandle hShaderPermutation = xiiResourceManager::LoadResource<xiiShaderPermutationResource>(sResolvedPermutationPath);

  {
    XII_LOCK(s_pState->m_Mutex);
    xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hShaderPermutation, xiiResourceAcquireMode::PointerOnly);
    if (!pShaderPermutation->IsShaderValid())
    {
      pShaderPermutation->m_PermutationVariables = filteredPermutationVariables;
    }
  }

  xiiResourceManager::PreloadResource(hShaderPermutation);
  return hShaderPermutation;
}

void xiiShaderPermutationUtilities::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Shader permutation utilities were started twice.");
  s_pState = XII_DEFAULT_NEW(xiiShaderPermutationUtilitiesState);
}

void xiiShaderPermutationUtilities::Shutdown()
{
  s_pState.Clear();
}
