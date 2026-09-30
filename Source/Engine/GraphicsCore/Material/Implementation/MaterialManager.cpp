/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashTable.h>
#include <GraphicsCore/Material/MaterialManager.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureCubeResource.h>
#include <GraphicsFoundation/Resources/BindlessResourceTable.h>

namespace
{
  template <typename ResourceHandle>
  struct MaterialTextureBindingCacheEntry
  {
    xiiSharedPtr<xiiGALTextureView>   m_pView;
    xiiGALBindlessResourceHandle      m_hBindless;
    xiiUInt64                         m_uiLastReferencedFrame = 0ULL;
  };
} // namespace

class xiiMaterialManagerState
{
public:
  xiiUniquePtr<xiiMaterialSystem>  m_pSystem;
  xiiMaterialGpuStorageDescription m_Description;
  xiiHashTable<xiiTexture2DResourceHandle, MaterialTextureBindingCacheEntry<xiiTexture2DResourceHandle>>     m_Texture2DCache;
  xiiHashTable<xiiTextureCubeResourceHandle, MaterialTextureBindingCacheEntry<xiiTextureCubeResourceHandle>> m_TextureCubeCache;
  xiiUInt64                       m_uiFrameIndex    = 0ULL;
  bool                             m_bEngineStarted = false;
  bool                             m_bInitialized   = false;
};

namespace
{
  template <typename ResourceType, typename ResourceHandle>
  xiiUInt32 ResolveBindlessTexture(
    xiiHashTable<ResourceHandle, MaterialTextureBindingCacheEntry<ResourceHandle>>& cache,
    const ResourceHandle& hTexture, xiiUInt64 uiFrameIndex, xiiGALBindlessResourceTable& table)
  {
    if (!hTexture.IsValid())
      return xiiInvalidIndex;

    xiiResourceLock<ResourceType> texture(hTexture, xiiResourceAcquireMode::AllowLoadingFallback_NeverFail);
    if (!texture.IsValid())
      return xiiInvalidIndex;

    const xiiSharedPtr<xiiGALTexture> pTexture = texture->GetGALTexture();
    if (pTexture == nullptr)
      return xiiInvalidIndex;

    const xiiSharedPtr<xiiGALTextureView> pView = pTexture->GetDefaultView(xiiGALTextureViewType::ShaderResource);
    if (pView == nullptr)
      return xiiInvalidIndex;

    MaterialTextureBindingCacheEntry<ResourceHandle>* pEntry = nullptr;
    if (!cache.TryGetValue(hTexture, pEntry))
    {
      MaterialTextureBindingCacheEntry<ResourceHandle> entry;
      entry.m_pView                 = pView;
      entry.m_hBindless             = table.RegisterTextureSRV(pView);
      entry.m_uiLastReferencedFrame = uiFrameIndex;
      if (!entry.m_hBindless.IsValid())
        return xiiInvalidIndex;

      cache.Insert(hTexture, std::move(entry));
      XII_VERIFY(cache.TryGetValue(hTexture, pEntry), "Material texture cache insertion must be observable immediately.");
    }
    else if (pEntry->m_pView != pView)
    {
      // Descriptor contents may still be read by an older frame. Allocate a fresh slot for the
      // streamed view and defer reuse of the old index instead of mutating it in place.
      const xiiGALBindlessResourceHandle hReplacement = table.RegisterTextureSRV(pView);
      if (hReplacement.IsValid())
      {
        table.RetireTextureSRV(pEntry->m_hBindless, uiFrameIndex);
        pEntry->m_pView     = pView;
        pEntry->m_hBindless = hReplacement;
      }
    }

    pEntry->m_uiLastReferencedFrame = uiFrameIndex;
    return pEntry->m_hBindless.m_uiIndex;
  }

  template <typename ResourceHandle>
  void RetireUnreferencedTextures(
    xiiHashTable<ResourceHandle, MaterialTextureBindingCacheEntry<ResourceHandle>>& cache,
    xiiUInt64 uiFrameIndex, xiiGALBindlessResourceTable& table)
  {
    for (auto it = cache.GetIterator(); it.IsValid();)
    {
      if (it.Value().m_uiLastReferencedFrame == uiFrameIndex)
      {
        ++it;
        continue;
      }

      table.RetireTextureSRV(it.Value().m_hBindless, uiFrameIndex);
      it = cache.Remove(it);
    }
  }

  template <typename ResourceHandle>
  void RetireAllTextures(
    xiiHashTable<ResourceHandle, MaterialTextureBindingCacheEntry<ResourceHandle>>& cache,
    xiiUInt64 uiFrameIndex, xiiGALBindlessResourceTable& table)
  {
    for (auto it = cache.GetIterator(); it.IsValid(); ++it)
      table.RetireTextureSRV(it.Value().m_hBindless, uiFrameIndex);
    cache.Clear();
  }
} // namespace

xiiUniquePtr<xiiMaterialManagerState> xiiMaterialManager::s_pState;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, MaterialManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "BindlessResourceTable"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiMaterialManager::Startup();
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    xiiMaterialManager::EngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    xiiMaterialManager::EngineShutdown();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiMaterialManager::Shutdown();
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

xiiResult xiiMaterialManager::Configure(const xiiMaterialGpuStorageDescription& description)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Material manager is not started.");
  if (s_pState == nullptr || description.m_uiMaxMaterials == 0U || description.m_uiMaxParameterBytes == 0U || description.m_uiFramesInFlight == 0U)
    return XII_FAILURE;

  if (s_pState->m_bInitialized && s_pState->m_pSystem->GetGpuStorage().GetStatistics().m_uiActiveMaterials != 0U)
  {
    XII_ASSERT_DEV(false, "The material manager cannot be reconfigured while material slots are active.");
    return XII_FAILURE;
  }

  const bool bSameConfiguration =
    s_pState->m_Description.m_uiMaxMaterials == description.m_uiMaxMaterials &&
    s_pState->m_Description.m_uiMaxParameterBytes == description.m_uiMaxParameterBytes &&
    s_pState->m_Description.m_uiFramesInFlight == description.m_uiFramesInFlight;
  if (bSameConfiguration && s_pState->m_bInitialized)
    return XII_SUCCESS;

  s_pState->m_Description = description;
  return s_pState->m_bEngineStarted ? ApplyConfiguration() : XII_SUCCESS;
}

bool xiiMaterialManager::IsInitialized()
{
  return s_pState != nullptr && s_pState->m_bInitialized;
}

void xiiMaterialManager::BeginFrame(xiiUInt64 uiFrameIndex, xiiUInt64 uiCompletedFrame)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before BeginFrame().");
  s_pState->m_uiFrameIndex = uiFrameIndex;

  if (xiiGALBindlessResourceTable* pTable = xiiGALBindlessResourceTable::GetSingleton(); pTable != nullptr && pTable->IsInitialized())
  {
    xiiDynamicArray<xiiSharedPtr<xiiMaterialInstance>> materials;
    s_pState->m_pSystem->GetGpuStorage().GetActiveMaterials(materials);

    for (const xiiSharedPtr<xiiMaterialInstance>& pMaterial : materials)
    {
      xiiMaterialInstanceSnapshot snapshot;
      pMaterial->CreateSnapshot(snapshot);
      if (snapshot.m_pSchema == nullptr)
        continue;

      const auto textureDefinitions = snapshot.m_pSchema->GetTextures();
      const xiiUInt32 uiTextureCount = xiiMath::Min(textureDefinitions.GetCount(), snapshot.m_ResourceBindings.GetCount());
      for (xiiUInt32 uiTexture = 0U; uiTexture < uiTextureCount; ++uiTexture)
      {
        const xiiMaterialTextureDefinition& definition = textureDefinitions[uiTexture];
        if (!definition.m_bBindless)
          continue;

        const xiiMaterialResourceBinding& binding = snapshot.m_ResourceBindings[uiTexture];
        xiiUInt32 uiBindlessIndex = xiiInvalidIndex;
        if (definition.m_TextureType == xiiGALShaderTextureType::Texture2D || definition.m_TextureType == xiiGALShaderTextureType::Texture2DArray)
        {
          if (!binding.m_hTexture2D.IsValid())
            continue; // No resource handle means an explicitly assigned descriptor is caller-owned.
          uiBindlessIndex = ResolveBindlessTexture<xiiTexture2DResource>(s_pState->m_Texture2DCache, binding.m_hTexture2D, uiFrameIndex, *pTable);
        }
        else if (definition.m_TextureType == xiiGALShaderTextureType::TextureCube || definition.m_TextureType == xiiGALShaderTextureType::TextureCubeArray)
        {
          if (!binding.m_hTextureCube.IsValid())
            continue;
          uiBindlessIndex = ResolveBindlessTexture<xiiTextureCubeResource>(s_pState->m_TextureCubeCache, binding.m_hTextureCube, uiFrameIndex, *pTable);
        }

        pMaterial->SetBindlessIndex(binding.m_Id, uiBindlessIndex).IgnoreResult();
      }
    }

    RetireUnreferencedTextures(s_pState->m_Texture2DCache, uiFrameIndex, *pTable);
    RetireUnreferencedTextures(s_pState->m_TextureCubeCache, uiFrameIndex, *pTable);
  }

  s_pState->m_pSystem->BeginFrame(uiFrameIndex, uiCompletedFrame);
}

xiiMaterialGpuHandle xiiMaterialManager::RegisterMaterial(xiiSharedPtr<xiiMaterialInstance> pInstance)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before registering materials.");
  return IsInitialized() ? s_pState->m_pSystem->RegisterMaterial(std::move(pInstance)) : xiiMaterialGpuHandle{};
}

void xiiMaterialManager::UnregisterMaterial(xiiMaterialGpuHandle handle)
{
  if (IsInitialized())
    s_pState->m_pSystem->UnregisterMaterial(handle);
}

xiiRenderGraphBufferHandle xiiMaterialManager::AddUploadPass(xiiRenderGraph& graph)
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager must be initialized before adding upload passes.");
  return IsInitialized() ? s_pState->m_pSystem->AddUploadPass(graph) : xiiRenderGraphBufferHandle{};
}

xiiResult xiiMaterialManager::ExtractRenderData(xiiMaterialGpuHandle handle, xiiMaterialRenderData& out_renderData)
{
  return IsInitialized() ? s_pState->m_pSystem->ExtractRenderData(handle, out_renderData) : XII_FAILURE;
}

xiiMaterialGpuStorage& xiiMaterialManager::GetGpuStorage()
{
  XII_ASSERT_DEV(IsInitialized(), "Material manager GPU storage is not initialized.");
  return s_pState->m_pSystem->GetGpuStorage();
}

xiiResult xiiMaterialManager::CreateRuntimeMaterial(const xiiMaterialSchemaDescription& description, const xiiMaterialRuntimeState& runtimeState, xiiSharedPtr<xiiMaterialSchema>& out_pSchema, xiiSharedPtr<xiiMaterialInstance>& out_pInstance, xiiStringBuilder* out_pError)
{
  XII_ASSERT_DEV(s_pState != nullptr, "Material manager is not started.");
  return xiiMaterialSystem::CreateRuntimeMaterial(description, runtimeState, out_pSchema, out_pInstance, out_pError);
}

void xiiMaterialManager::Startup()
{
  XII_ASSERT_DEV(s_pState == nullptr, "Material manager started twice.");
  s_pState = XII_DEFAULT_NEW(xiiMaterialManagerState);
}

void xiiMaterialManager::EngineStartup()
{
  XII_ASSERT_DEV(s_pState != nullptr, "Core startup must precede material manager engine startup.");
  s_pState->m_bEngineStarted = true;
  ApplyConfiguration().IgnoreResult();
}

void xiiMaterialManager::EngineShutdown()
{
  if (s_pState == nullptr)
    return;

  if (xiiGALBindlessResourceTable* pTable = xiiGALBindlessResourceTable::GetSingleton(); pTable != nullptr && pTable->IsInitialized())
  {
    RetireAllTextures(s_pState->m_Texture2DCache, s_pState->m_uiFrameIndex, *pTable);
    RetireAllTextures(s_pState->m_TextureCubeCache, s_pState->m_uiFrameIndex, *pTable);
  }
  else
  {
    s_pState->m_Texture2DCache.Clear();
    s_pState->m_TextureCubeCache.Clear();
  }

  if (s_pState->m_pSystem != nullptr)
    s_pState->m_pSystem->Shutdown();
  s_pState->m_pSystem.Clear();
  s_pState->m_uiFrameIndex     = 0ULL;
  s_pState->m_bInitialized   = false;
  s_pState->m_bEngineStarted = false;
}

void xiiMaterialManager::Shutdown()
{
  EngineShutdown();
  s_pState.Clear();
}

xiiResult xiiMaterialManager::ApplyConfiguration()
{
  const xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();
  if (pDevice == nullptr)
    return XII_FAILURE;

  if (s_pState->m_pSystem == nullptr)
    s_pState->m_pSystem = XII_DEFAULT_NEW(xiiMaterialSystem);
  else if (s_pState->m_bInitialized)
    s_pState->m_pSystem->Shutdown();

  s_pState->m_bInitialized = s_pState->m_pSystem->Initialize(pDevice.Borrow(), s_pState->m_Description).Succeeded();
  return s_pState->m_bInitialized ? XII_SUCCESS : XII_FAILURE;
}
