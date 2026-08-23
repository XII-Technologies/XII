/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/Components/StateMachine/StateMachineResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiStateMachineResource, 1, xiiRTTIDefaultAllocator<xiiStateMachineResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiStateMachineResource);
// clang-format on

xiiStateMachineResource::xiiStateMachineResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiStateMachineResource::~xiiStateMachineResource() = default;

xiiUniquePtr<xiiStateMachineInstance> xiiStateMachineResource::CreateInstance(xiiReflectedClass& ref_owner)
{
  if (m_pDescription != nullptr)
  {
    return XII_DEFAULT_NEW(xiiStateMachineInstance, ref_owner, m_pDescription);
  }

  return nullptr;
}

xiiResourceLoadDescription xiiStateMachineResource::UnloadData(Unload WhatToUnload)
{
  m_pDescription = nullptr;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDescription xiiStateMachineResource::UpdateContent(xiiStreamReader* pStream)
{
  XII_LOG_BLOCK("xiiStateMachineResource::UpdateContent", GetResourceDescription().GetData());

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

  xiiUniquePtr<xiiStateMachineDescription> pDescription = XII_DEFAULT_NEW(xiiStateMachineDescription);
  if (pDescription->Deserialize(*pStream).Failed())
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  m_pDescription = std::move(pDescription);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiStateMachineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineResource);
