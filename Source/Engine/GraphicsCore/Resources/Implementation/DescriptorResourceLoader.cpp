/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/MemoryStream.h>
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
    struct LoadedData : xiiStreamReader
    {
      xiiFileReader m_File;
      xiiUInt8      m_Prefix[4]     = {};
      xiiUInt32     m_uiPrefixBytes = 0U, m_uiPrefixPosition = 0U;

      xiiUInt64 ReadBytes(void* buffer, xiiUInt64 bytes) override
      {
        const auto prefix = xiiMath::Min<xiiUInt64>(bytes, m_uiPrefixBytes - m_uiPrefixPosition);
        xiiMemoryUtils::Copy(static_cast<xiiUInt8*>(buffer), m_Prefix + m_uiPrefixPosition, static_cast<size_t>(prefix));
        m_uiPrefixPosition += static_cast<xiiUInt32>(prefix);
        if (prefix == bytes)
          return prefix;
        return prefix + m_File.ReadBytes(static_cast<xiiUInt8*>(buffer) + prefix, bytes - prefix);
      }

      xiiUInt64 SkipBytes(xiiUInt64 bytes) override
      {
        const auto prefix = xiiMath::Min<xiiUInt64>(bytes, m_uiPrefixBytes - m_uiPrefixPosition);
        m_uiPrefixPosition += static_cast<xiiUInt32>(prefix);
        return prefix + m_File.SkipBytes(bytes - prefix);
      }
    };

  public:
    xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override
    {
      xiiUniquePtr<LoadedData> data = XII_DEFAULT_NEW(LoadedData);
      auto&                    file = data->m_File;
      if (file.Open(pResource->GetResourceID()).Failed())
        return {};
      // Validate the envelope without materializing the asset payload. The open
      // file remains the resource-update stream; page payloads can be skipped.
      xiiUInt8 prefix[23] = {};
      if (file.GetFileSize() < 27U || file.ReadBytes(prefix, sizeof(prefix)) != sizeof(prefix) ||
          !xiiMemoryUtils::IsEqual(prefix, reinterpret_cast<const xiiUInt8*>("xiiAsset"), 8U) || prefix[8] != 3U)
        return {};
      xiiRawMemoryStreamReader prefixReader(prefix, sizeof(prefix));
      prefixReader.SkipBytes(19U);
      xiiUInt32 generatorLength = 0U;
      if (prefixReader.ReadDWordValue(&generatorLength).Failed() || generatorLength > 65536U || generatorLength > file.GetFileSize() - 27U)
        return {};
      xiiDynamicArray<xiiUInt8> envelope;
      envelope.SetCountUninitialized(23U + generatorLength);
      xiiMemoryUtils::Copy(envelope.GetData(), prefix, sizeof(prefix));
      if (file.ReadBytes(envelope.GetData() + sizeof(prefix), generatorLength) != generatorLength)
        return {};
      xiiRawMemoryStreamReader headerReader(envelope.GetData(), envelope.GetCount());
      xiiAssetFileHeader       header;
      if (header.Read(headerReader).Failed())
        return {};
      const auto descriptorOffset = envelope.GetCount();
      const bool packedDecalAtlas = pResource->GetDynamicRTTI()->IsDerivedFrom<xiiDecalAtlasResource>();
      const bool wordVersion      = packedDecalAtlas || pResource->GetDynamicRTTI()->IsDerivedFrom<xiiDecalResource>() ||
        pResource->GetDynamicRTTI()->IsDerivedFrom<xiiMeshResource>() || pResource->GetDynamicRTTI()->IsDerivedFrom<xiiMeshBufferResource>();
      data->m_uiPrefixBytes = wordVersion ? sizeof(xiiTypeVersion) : sizeof(xiiUInt32);
      if (file.ReadBytes(data->m_Prefix, data->m_uiPrefixBytes) != data->m_uiPrefixBytes)
        return {};
      xiiRawMemoryStreamReader versionReader(data->m_Prefix, data->m_uiPrefixBytes);
      xiiUInt32                descriptorVersion = 0U;
      if (wordVersion)
      {
        xiiTypeVersion version = 0U;
        if (versionReader.ReadWordValue(&version).Failed())
          return {};
        descriptorVersion = version;
      }
      else if (versionReader.ReadDWordValue(&descriptorVersion).Failed())
        return {};
      const bool meshBuffer = pResource->GetDynamicRTTI()->IsDerivedFrom<xiiMeshBufferResource>();
      if ((meshBuffer ? descriptorVersion != 1U && descriptorVersion != 4U : descriptorVersion != (packedDecalAtlas ? 2U : 1U)))
      {
        xiiLog::Error("Native descriptor asset '{}' uses an unsupported descriptor format. Re-transform its source asset.", pResource->GetResourceID());
        return {};
      }
      xiiResourceLoadData result;
      result.m_sResourceDescription = file.GetFilePathRelative().GetView();
      result.m_sSourceFile          = file.GetFilePathAbsolute().GetView();
      result.m_uiSourceOffset       = descriptorOffset;
      result.m_uiSourceHash         = header.GetFileHash();
      result.m_uiSourceVersion      = header.GetFileVersion();
      result.m_pDataStream          = data.Borrow();
      result.m_pCustomLoaderData    = data.Release();
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
