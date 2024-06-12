#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Math/Rect.h>
#include <GraphicsCore/Decals/DecalAtlasResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <Texture/Image/Formats/DdsFileFormat.h>
#include <Texture/Image/Image.h>

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, DecalAtlasResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "Core",
  "TextureResource"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiDecalAtlasResourceDescriptor desc;
    xiiDecalAtlasResourceHandle hFallback = xiiResourceManager::CreateResource<xiiDecalAtlasResource>("Fallback Decal Atlas", std::move(desc), "Empty Decal Atlas for loading and missing decals");

    xiiResourceManager::SetResourceTypeLoadingFallback<xiiDecalAtlasResource>(hFallback);
    xiiResourceManager::SetResourceTypeMissingFallback<xiiDecalAtlasResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeLoadingFallback<xiiDecalAtlasResource>(xiiDecalAtlasResourceHandle());
    xiiResourceManager::SetResourceTypeMissingFallback<xiiDecalAtlasResource>(xiiDecalAtlasResourceHandle());
  }

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
  }

XII_END_SUBSYSTEM_DECLARATION;
// clang-format on


//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiDecalAtlasResource, 1, xiiRTTIDefaultAllocator<xiiDecalAtlasResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiDecalAtlasResource);
// clang-format on

xiiUInt32 xiiDecalAtlasResource::s_uiDecalAtlasResources = 0;

xiiDecalAtlasResource::xiiDecalAtlasResource() :
  xiiResource(DoUpdate::OnAnyThread, 1), m_vBaseColorSize(xiiVec2U32::MakeZero()), m_vNormalSize(xiiVec2U32::MakeZero())
{
}

xiiDecalAtlasResourceHandle xiiDecalAtlasResource::GetDecalAtlasResource()
{
  return xiiResourceManager::LoadResource<xiiDecalAtlasResource>("{ ProjectDecalAtlas }");
}

xiiResourceLoadDesc xiiDecalAtlasResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiDecalAtlasResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiDecalAtlasResource::UpdateContent", GetResourceDescription().GetData());

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::LoadedResourceMissing;

  if (Stream == nullptr)
    return res;

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset header
  {
    xiiAssetFileHeader header;
    header.Read(*Stream).IgnoreResult();
  }

  {
    xiiUInt8 uiVersion = 0;
    *Stream >> uiVersion;
    XII_ASSERT_DEV(uiVersion <= 3, "Invalid decal atlas version {0}", uiVersion);

    // this version is now incompatible
    if (uiVersion < 3)
      return res;
  }

  // read the textures
  {
    xiiDdsFileFormat dds;
    xiiImage         baseColor, normal, orm;

    if (dds.ReadImage(*Stream, baseColor, "dds").Failed())
    {
      xiiLog::Error("Failed to load baseColor image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, normal, "dds").Failed())
    {
      xiiLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    if (dds.ReadImage(*Stream, orm, "dds").Failed())
    {
      xiiLog::Error("Failed to load normal image for decal atlas");
      return res;
    }

    CreateLayerTexture(baseColor, true, m_hBaseColor);
    CreateLayerTexture(normal, false, m_hNormal);
    CreateLayerTexture(orm, false, m_hORM);

    m_vBaseColorSize = xiiVec2U32(baseColor.GetWidth(), baseColor.GetHeight());
    m_vNormalSize    = xiiVec2U32(normal.GetWidth(), normal.GetHeight());
    m_vORMSize       = xiiVec2U32(orm.GetWidth(), orm.GetHeight());
  }

  ReadDecalInfo(Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiDecalAtlasResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiDecalAtlasResource) + (xiiUInt32)m_Atlas.m_Items.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiDecalAtlasResource, xiiDecalAtlasResourceDescriptor)
{
  xiiResourceLoadDesc ret;
  ret.m_uiQualityLevelsDiscardable = 0;
  ret.m_uiQualityLevelsLoadable    = 0;
  ret.m_State                      = xiiResourceState::Loaded;

  m_Atlas.Clear();

  return ret;
}

void xiiDecalAtlasResource::CreateLayerTexture(const xiiImage& img, bool bSRGB, xiiTexture2DResourceHandle& out_hTexture)
{
  xiiTexture2DResourceDescriptor td;
  td.m_SamplerDesc.m_AddressU = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
  td.m_SamplerDesc.m_AddressV = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);
  td.m_SamplerDesc.m_AddressW = xiiTextureUtils::GALTextureAddressMode(xiiImageAddressMode::Clamp);

  xiiUInt32                                        uiMemory;
  xiiHybridArray<xiiGALTextureSubResourceData, 32> initData;
  xiiTexture2DResource::FillOutDescriptor(td, &img, bSRGB, img.GetNumMipLevels(), uiMemory, initData);
  xiiTextureUtils::ConfigureSampler(xiiTextureFilterSetting::HighQuality, td.m_SamplerDesc);

  xiiStringBuilder sTexId;
  sTexId.SetFormat("{0}_Tex{1}", GetResourceID(), s_uiDecalAtlasResources);
  ++s_uiDecalAtlasResources;

  out_hTexture = xiiResourceManager::CreateResource<xiiTexture2DResource>(sTexId, std::move(td));
}

void xiiDecalAtlasResource::ReadDecalInfo(xiiStreamReader* Stream)
{
  m_Atlas.Deserialize(*Stream).IgnoreResult();
}

void xiiDecalAtlasResource::ReportResourceIsMissing()
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  // normal during development, don't care much
  xiiLog::Debug("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#else
  // should probably exist for shipped applications, report this
  xiiLog::Warning("Decal Atlas Resource is missing: '{0}' ('{1}')", GetResourceID(), GetResourceDescription());
#endif
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Decals_Implementation_DecalAtlasResource);
