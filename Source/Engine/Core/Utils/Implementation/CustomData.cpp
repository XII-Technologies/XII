/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Utils/CustomData.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Utilities/AssetFileHeader.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomData, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void xiiCustomData::Load(xiiAbstractObjectGraph& ref_graph, xiiRttiConverterContext& ref_context, const xiiAbstractObjectNode* pRootNode)
{
  xiiRttiConverterReader convRead(&ref_graph, &ref_context);
  convRead.ApplyPropertiesToObject(pRootNode, GetDynamicRTTI(), this);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCustomDataResourceBase, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiCustomDataResourceBase::xiiCustomDataResourceBase() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiCustomDataResourceBase::~xiiCustomDataResourceBase() = default;

xiiResourceLoadDescription xiiCustomDataResourceBase::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;
  return res;
}

xiiResourceLoadDescription xiiCustomDataResourceBase::UpdateContent_Internal(xiiStreamReader* pStream, const xiiRTTI& rtti)
{
  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (pStream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*pStream) >> sAbsFilePath;
  }

  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*pStream).IgnoreResult();

  xiiAbstractObjectGraph  graph;
  xiiRttiConverterContext context;

  xiiAbstractGraphBinarySerializer::Read(*pStream, &graph);

  const xiiAbstractObjectNode* pRootNode = graph.GetNodeByName("root");

  if (pRootNode != nullptr && pRootNode->GetType() != rtti.GetTypeName())
  {
    xiiLog::Error("Expected xiiCustomData type '{}' but resource is of type '{}' ('{}')", rtti.GetTypeName(), pRootNode->GetType(), GetResourceIdOrDescription());

    // Ensure that we create a default-initialized object and do not deserialize data that happens to match.
    pRootNode = nullptr;
  }

  CreateAndLoadData(graph, context, pRootNode);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

XII_STATICLINK_FILE(Core, Core_Utils_Implementation_CustomData);
