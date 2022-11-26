#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <TypeScriptPlugin/Resources/ScriptCompendiumResource.h>

// clang-format off
XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiScriptCompendiumResource);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiScriptCompendiumResource, 1, xiiRTTIDefaultAllocator<xiiScriptCompendiumResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiScriptCompendiumResource::xiiScriptCompendiumResource() :
  xiiResource(xiiResource::DoUpdate::OnAnyThread, 1)
{
}

xiiScriptCompendiumResource::~xiiScriptCompendiumResource() = default;

xiiResourceLoadDesc xiiScriptCompendiumResource::UnloadData(Unload WhatToUnload)
{
  xiiResourceLoadDesc ld;
  ld.m_State                      = xiiResourceState::Unloaded;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;

  m_Desc.m_PathToSource.Clear();

  return ld;
}

xiiResourceLoadDesc xiiScriptCompendiumResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiResourceLoadDesc ld;
  ld.m_uiQualityLevelsDiscardable = 0;
  ld.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    ld.m_State = xiiResourceState::LoadedResourceMissing;
    return ld;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiString sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  m_Desc.Deserialize(*pStream).IgnoreResult();

  ld.m_State = xiiResourceState::Loaded;

  return ld;
}

void xiiScriptCompendiumResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = (xiiUInt32)sizeof(xiiScriptCompendiumResource) + (xiiUInt32)m_Desc.m_PathToSource.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiScriptCompendiumResourceDesc::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(2);

  XII_SUCCEED_OR_RETURN(stream.WriteMap(m_PathToSource));
  XII_SUCCEED_OR_RETURN(stream.WriteMap(m_AssetGuidToInfo));

  return XII_SUCCESS;
}

xiiResult xiiScriptCompendiumResourceDesc::Deserialize(xiiStreamReader& stream)
{
  xiiTypeVersion version = stream.ReadVersion(2);

  XII_SUCCEED_OR_RETURN(stream.ReadMap(m_PathToSource));

  if (version >= 2)
  {
    XII_SUCCEED_OR_RETURN(stream.ReadMap(m_AssetGuidToInfo));
  }

  return XII_SUCCESS;
}

xiiResult xiiScriptCompendiumResourceDesc::ComponentTypeInfo::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  XII_SUCCEED_OR_RETURN(stream.WriteString(m_sComponentTypeName));
  XII_SUCCEED_OR_RETURN(stream.WriteString(m_sComponentFilePath));
  return XII_SUCCESS;
}

xiiResult xiiScriptCompendiumResourceDesc::ComponentTypeInfo::Deserialize(xiiStreamReader& stream)
{
  xiiTypeVersion version = stream.ReadVersion(1);

  XII_SUCCEED_OR_RETURN(stream.ReadString(m_sComponentTypeName));
  XII_SUCCEED_OR_RETURN(stream.ReadString(m_sComponentFilePath));
  return XII_SUCCESS;
}
