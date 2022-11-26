#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <RmlUiPlugin/Resources/RmlUiResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiRmlUiScaleMode, 1)
  XII_ENUM_CONSTANTS(xiiRmlUiScaleMode::Fixed, xiiRmlUiScaleMode::WithScreenSize)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

//////////////////////////////////////////////////////////////////////////

static xiiTypeVersion s_RmlUiDescVersion = 1;

xiiResult xiiRmlUiResourceDescriptor::Save(xiiStreamWriter& stream)
{
  // write this at the beginning so that the file can be read as an xiiDependencyFile
  m_DependencyFile.StoreCurrentTimeStamp();
  XII_SUCCEED_OR_RETURN(m_DependencyFile.WriteDependencyFile(stream));

  stream.WriteVersion(s_RmlUiDescVersion);

  stream << m_sRmlFile;
  stream << m_ScaleMode;
  stream << m_ReferenceResolution;

  return XII_SUCCESS;
}

xiiResult xiiRmlUiResourceDescriptor::Load(xiiStreamReader& stream)
{
  XII_SUCCEED_OR_RETURN(m_DependencyFile.ReadDependencyFile(stream));

  xiiTypeVersion uiVersion = stream.ReadVersion(s_RmlUiDescVersion);

  stream >> m_sRmlFile;
  stream >> m_ScaleMode;
  stream >> m_ReferenceResolution;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRmlUiResource, 1, xiiRTTIDefaultAllocator<xiiRmlUiResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRmlUiResource);
// clang-format on

xiiRmlUiResource::xiiRmlUiResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiResourceLoadDesc xiiRmlUiResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiRmlUiResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiRmlUiResourceDescriptor desc;
  xiiResourceLoadDesc        res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  // Direct loading of rml file
  if (sAbsFilePath.GetFileExtension() == "rml")
  {
    m_sRmlFile = sAbsFilePath;

    res.m_State = xiiResourceState::Loaded;
    return res;
  }

  xiiAssetFileHeader assetHeader;
  assetHeader.Read(*Stream).IgnoreResult();

  if (desc.Load(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  return CreateResource(std::move(desc));
}

void xiiRmlUiResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(*this);
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiRmlUiResource, xiiRmlUiResourceDescriptor)
{
  m_sRmlFile             = descriptor.m_sRmlFile;
  m_ScaleMode            = descriptor.m_ScaleMode;
  m_vReferenceResolution = descriptor.m_ReferenceResolution;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

//////////////////////////////////////////////////////////////////////////

bool xiiRmlUiResourceLoader::IsResourceOutdated(const xiiResource* pResource) const
{
  if (xiiResourceLoaderFromFile::IsResourceOutdated(pResource))
    return true;

  xiiStringBuilder sId = pResource->GetResourceID();
  if (sId.GetFileExtension() == "rml")
    return false;

  xiiFileReader stream;
  if (stream.Open(pResource->GetResourceID()).Failed())
    return false;

  // skip asset header
  xiiAssetFileHeader assetHeader;
  assetHeader.Read(stream).IgnoreResult();

  xiiDependencyFile dep;
  if (dep.ReadDependencyFile(stream).Failed())
    return true;

  return dep.HasAnyFileChanged();
}
