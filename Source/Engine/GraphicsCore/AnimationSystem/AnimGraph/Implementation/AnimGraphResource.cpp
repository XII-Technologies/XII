#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimationClipMapping, 1, xiiRTTIDefaultAllocator<xiiAnimationClipMapping>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ClipName", GetClipName, SetClipName)->AddAttributes(new xiiDynamicStringEnumAttribute("AnimationClipMappingEnum")),
    XII_ACCESSOR_PROPERTY("Clip", GetClip, SetClip)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
  }
    XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphResource, 1, xiiRTTIDefaultAllocator<xiiAnimGraphResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiAnimGraphResource);
// clang-format on

const char* xiiAnimationClipMapping::GetClip() const
{
  if (m_hClip.IsValid())
    return m_hClip.GetResourceID();

  return "";
}

void xiiAnimationClipMapping::SetClip(const char* szName)
{
  xiiAnimationClipResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szName))
  {
    hResource = xiiResourceManager::LoadResource<xiiAnimationClipResource>(szName);
  }

  m_hClip = hResource;
}

xiiAnimGraphResource::xiiAnimGraphResource() :
  xiiResource(xiiResource::DoUpdate::OnAnyThread, 0)
{
}

xiiAnimGraphResource::~xiiAnimGraphResource() = default;

xiiResourceLoadDesc xiiAnimGraphResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc d;
  d.m_State                      = xiiResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable    = 0;
  return d;
}

xiiResourceLoadDesc xiiAnimGraphResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).AssertSuccess();

  {
    const auto uiVersion = Stream->ReadVersion(2);
    Stream->ReadArray(m_IncludeGraphs).AssertSuccess();

    if (uiVersion >= 2)
    {
      xiiUInt32 uiNum = 0;
      *Stream >> uiNum;

      m_AnimationClipMapping.SetCount(uiNum);
      for (xiiUInt32 i = 0; i < uiNum; ++i)
      {
        *Stream >> m_AnimationClipMapping[i].m_sClipName;
        *Stream >> m_AnimationClipMapping[i].m_hClip;
      }
    }
  }

  if (m_AnimGraph.Deserialize(*Stream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  m_AnimGraph.PrepareForUse();

  res.m_State = xiiResourceState::Loaded;

  return res;
}

void xiiAnimGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraphResource);
