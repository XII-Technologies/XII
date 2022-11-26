#include <ProcGenPlugin/ProcGenPluginPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <Core/Curves/ColorGradientResource.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Foundation/CodeUtils/Expression/ExpressionByteCode.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <ProcGenPlugin/Resources/ProcGenGraphResource.h>
#include <ProcGenPlugin/Resources/ProcGenGraphSharedData.h>

namespace xiiProcGenInternal
{
  extern Pattern* GetPattern(xiiTempHashedString sName);
}

using namespace xiiProcGenInternal;

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiProcGenGraphResource, 1, xiiRTTIDefaultAllocator<xiiProcGenGraphResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiProcGenGraphResource);
// clang-format on

xiiProcGenGraphResource::xiiProcGenGraphResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiProcGenGraphResource::~xiiProcGenGraphResource() = default;

const xiiDynamicArray<xiiSharedPtr<const PlacementOutput>>& xiiProcGenGraphResource::GetPlacementOutputs() const
{
  return m_PlacementOutputs;
}

const xiiDynamicArray<xiiSharedPtr<const VertexColorOutput>>& xiiProcGenGraphResource::GetVertexColorOutputs() const
{
  return m_VertexColorOutputs;
}

xiiResourceLoadDesc xiiProcGenGraphResource::UnloadData(Unload WhatToUnload)
{
  m_PlacementOutputs.Clear();
  m_VertexColorOutputs.Clear();
  m_pSharedData = nullptr;

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiProcGenGraphResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiProcGenGraphResource::UpdateContent", GetResourceDescription().GetData());

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
  AssetHash.Read(*Stream).IgnoreResult();

  xiiUniquePtr<xiiStringDeduplicationReadContext> pStringDedupReadContext;
  if (AssetHash.GetFileVersion() >= 5)
  {
    pStringDedupReadContext = XII_DEFAULT_NEW(xiiStringDeduplicationReadContext, *Stream);
  }

  // load
  {
    xiiChunkStreamReader chunk(*Stream);
    chunk.SetEndChunkFileMode(xiiChunkStreamReader::EndChunkFileMode::JustClose);

    chunk.BeginStream();

    xiiStringBuilder sTemp;

    // skip all chunks that we don't know
    while (chunk.GetCurrentChunk().m_bValid)
    {
      if (chunk.GetCurrentChunk().m_sChunkName == "SharedData")
      {
        xiiSharedPtr<GraphSharedData> pSharedData = XII_DEFAULT_NEW(GraphSharedData);
        if (pSharedData->Load(chunk).Succeeded())
        {
          m_pSharedData = pSharedData;
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "PlacementOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 4)
        {
          xiiLog::Error("Invalid PlacementOutputs Chunk Version {0}. Expected >= 4", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        xiiUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_PlacementOutputs.Reserve(uiNumOutputs);
        for (xiiUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          xiiUniquePtr<xiiExpressionByteCode> pByteCode = XII_DEFAULT_NEW(xiiExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          xiiSharedPtr<PlacementOutput> pOutput = XII_DEFAULT_NEW(PlacementOutput);
          pOutput->m_pByteCode                  = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();

          xiiUInt64 uiNumObjectsToPlace = 0;
          chunk >> uiNumObjectsToPlace;

          for (xiiUInt32 uiObjectIndex = 0; uiObjectIndex < static_cast<xiiUInt32>(uiNumObjectsToPlace); ++uiObjectIndex)
          {
            chunk >> sTemp;
            pOutput->m_ObjectsToPlace.ExpandAndGetRef() = xiiResourceManager::LoadResource<xiiPrefabResource>(sTemp);
          }

          pOutput->m_pPattern = xiiProcGenInternal::GetPattern("Bayer");

          chunk >> pOutput->m_fFootprint;

          chunk >> pOutput->m_vMinOffset;
          chunk >> pOutput->m_vMaxOffset;

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 6)
          {
            chunk >> pOutput->m_YawRotationSnap;
          }
          chunk >> pOutput->m_fAlignToNormal;

          chunk >> pOutput->m_vMinScale;
          chunk >> pOutput->m_vMaxScale;

          chunk >> pOutput->m_fCullDistance;

          chunk >> pOutput->m_uiCollisionLayer;

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hColorGradient = xiiResourceManager::LoadResource<xiiColorGradientResource>(sTemp);
          }

          chunk >> sTemp;
          if (!sTemp.IsEmpty())
          {
            pOutput->m_hSurface = xiiResourceManager::LoadResource<xiiSurfaceResource>(sTemp);
          }

          if (chunk.GetCurrentChunk().m_uiChunkVersion >= 5)
          {
            chunk >> pOutput->m_Mode;
          }

          m_PlacementOutputs.PushBack(pOutput);
        }
      }
      else if (chunk.GetCurrentChunk().m_sChunkName == "VertexColorOutputs")
      {
        if (chunk.GetCurrentChunk().m_uiChunkVersion < 2)
        {
          xiiLog::Error("Invalid VertexColorOutputs Chunk Version {0}. Expected >= 2", chunk.GetCurrentChunk().m_uiChunkVersion);
          chunk.NextChunk();
          continue;
        }

        xiiUInt32 uiNumOutputs = 0;
        chunk >> uiNumOutputs;

        m_VertexColorOutputs.Reserve(uiNumOutputs);
        for (xiiUInt32 uiIndex = 0; uiIndex < uiNumOutputs; ++uiIndex)
        {
          xiiUniquePtr<xiiExpressionByteCode> pByteCode = XII_DEFAULT_NEW(xiiExpressionByteCode);
          if (pByteCode->Load(chunk).Failed())
          {
            break;
          }

          xiiSharedPtr<VertexColorOutput> pOutput = XII_DEFAULT_NEW(VertexColorOutput);
          pOutput->m_pByteCode                    = std::move(pByteCode);

          chunk >> pOutput->m_sName;
          chunk.ReadArray(pOutput->m_VolumeTagSetIndices).IgnoreResult();

          m_VertexColorOutputs.PushBack(pOutput);
        }
      }

      chunk.NextChunk();
    }

    chunk.EndStream();
    pStringDedupReadContext = nullptr;

    // link shared data
    if (m_pSharedData != nullptr)
    {
      for (auto& pPlacementOutput : m_PlacementOutputs)
      {
        const_cast<PlacementOutput*>(pPlacementOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }

      for (auto& pVertexColorOutput : m_VertexColorOutputs)
      {
        const_cast<VertexColorOutput*>(pVertexColorOutput.Borrow())->m_pGraphSharedData = m_pSharedData;
      }
    }
  }

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiProcGenGraphResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiProcGenGraphResource, xiiProcGenGraphResourceDescriptor)
{
  // XII_REPORT_FAILURE("This resource type does not support creating data.");

  // Missing resource

  auto pOutput = XII_DEFAULT_NEW(PlacementOutput);
  pOutput->m_sName.Assign("MissingPlacementOutput");
  pOutput->m_ObjectsToPlace.PushBack(xiiResourceManager::GetResourceTypeMissingFallback<xiiPrefabResource>());
  pOutput->m_pPattern   = xiiProcGenInternal::GetPattern("Bayer");
  pOutput->m_fFootprint = 3.0f;
  pOutput->m_vMinOffset.Set(-1.0f, -1.0f, -0.5f);
  pOutput->m_vMaxOffset.Set(1.0f, 1.0f, 0.0f);
  pOutput->m_vMinScale.Set(1.0f, 1.0f, 1.0f);
  pOutput->m_vMaxScale.Set(1.5f, 1.5f, 2.0f);

  m_PlacementOutputs.PushBack(pOutput);
  //

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}
