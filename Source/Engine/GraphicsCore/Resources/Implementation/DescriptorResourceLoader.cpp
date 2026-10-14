/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <GraphicsCore/Decals/DecalResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>

namespace
{
  /// Native mesh, animation and decal resources consume descriptor streams. File loading removes the standard asset
  /// envelope here; custom memory loaders continue supplying descriptors directly.
  class DescriptorResourceLoader final : public xiiResourceLoaderFromFile
  {
    struct LoadedData
    {
      xiiDefaultMemoryStreamStorage m_Storage;
      xiiMemoryStreamReader         m_Reader = xiiMemoryStreamReader(&m_Storage);
    };

  public:
    xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override
    {
      xiiFileReader file;
      if (file.Open(pResource->GetResourceID()).Failed())
        return {};
      LoadedData* data = XII_DEFAULT_NEW(LoadedData);
      data->m_Storage.ReadAll(file);
      // Check the fixed prefix before AssetFileHeader::Read, which asserts on a missing tag or future version.
      char     tag[8]  = {};
      xiiUInt8 version = 0;
      if (data->m_Storage.GetStorageSize64() < 27 || data->m_Reader.ReadBytes(tag, sizeof(tag)) != sizeof(tag) || !xiiMemoryUtils::IsEqual(tag, "xiiAsset", sizeof(tag)) || data->m_Reader.ReadBytes(&version, sizeof(version)) != sizeof(version) || version != 3)
      {
        XII_DEFAULT_DELETE(data);
        return {};
      }
      data->m_Reader.SetReadPosition(19); // tag, header version, asset hash and asset type version
      xiiUInt32 generatorLength = 0;
      if (data->m_Reader.ReadDWordValue(&generatorLength).Failed() || generatorLength > data->m_Storage.GetStorageSize64() - 27)
      {
        XII_DEFAULT_DELETE(data);
        return {};
      }
      data->m_Reader.SetReadPosition(0);
      xiiAssetFileHeader header;
      if (header.Read(data->m_Reader).Failed())
      {
        XII_DEFAULT_DELETE(data);
        return {};
      }
      const auto descriptorOffset  = data->m_Reader.GetReadPosition();
      xiiUInt32  descriptorVersion = 0;
      xiiResult  versionRead       = XII_FAILURE;
      const bool packedDecalAtlas  = pResource->GetDynamicRTTI()->IsDerivedFrom<xiiDecalAtlasResource>();
      if (packedDecalAtlas || pResource->GetDynamicRTTI()->IsDerivedFrom<xiiDecalResource>() || pResource->GetDynamicRTTI()->IsDerivedFrom<xiiMeshResource>() || pResource->GetDynamicRTTI()->IsDerivedFrom<xiiMeshBufferResource>())
      {
        xiiTypeVersion wordVersion = 0;
        versionRead                = data->m_Reader.ReadWordValue(&wordVersion);
        descriptorVersion          = wordVersion;
      }
      else
      {
        versionRead = data->m_Reader.ReadDWordValue(&descriptorVersion);
      }
      if (versionRead.Failed() || descriptorVersion != (packedDecalAtlas ? 2U : 1U))
      {
        xiiLog::Error("Native descriptor asset '{}' uses an unsupported descriptor format. Re-transform its source asset.", pResource->GetResourceID());
        XII_DEFAULT_DELETE(data);
        return {};
      }
      data->m_Reader.SetReadPosition(descriptorOffset);
      xiiResourceLoadData result;
      result.m_sResourceDescription = file.GetFilePathRelative().GetView();
      result.m_pDataStream          = &data->m_Reader;
      result.m_pCustomLoaderData    = data;
#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
      xiiFileStats stats;
      if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stats).Succeeded())
        result.m_LoadedFileModificationDate = stats.m_LastModificationTime;
#endif
      return result;
    }

    void CloseDataStream(const xiiResource*, const xiiResourceLoadData& loaderData) override
    {
      auto* data = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);
      XII_DEFAULT_DELETE(data);
    }
  };

  xiiUniquePtr<DescriptorResourceLoader> s_pDescriptorResourceLoader;
} // namespace

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(GraphicsCore, DescriptorResourceLoader)
  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    s_pDescriptorResourceLoader = XII_DEFAULT_NEW(DescriptorResourceLoader);
    xiiResourceManager::SetResourceTypeLoader<xiiSkeletonResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiMeshResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiMeshBufferResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiAnimationClipResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiAnimGraphResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiDecalAtlasResource>(s_pDescriptorResourceLoader.Borrow());
    xiiResourceManager::SetResourceTypeLoader<xiiDecalResource>(s_pDescriptorResourceLoader.Borrow());
  }
  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeLoader<xiiSkeletonResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiMeshResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiMeshBufferResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiAnimationClipResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiAnimGraphResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiDecalAtlasResource>(nullptr);
    xiiResourceManager::SetResourceTypeLoader<xiiDecalResource>(nullptr);
    s_pDescriptorResourceLoader.Clear();
  }
XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Resources_Implementation_DescriptorResourceLoader);
