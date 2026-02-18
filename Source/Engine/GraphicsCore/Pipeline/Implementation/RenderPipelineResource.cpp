#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Pipeline/Implementation/RenderPipelineResourceLoader.h>
#include <GraphicsCore/Pipeline/Passes/CreateTexturePass.h>
#include <GraphicsCore/Pipeline/Passes/TargetPass.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelineResource.h>

void xiiRenderPipelineResourceDescriptor::CreateFromRenderPipeline(const xiiRenderPipeline* pPipeline)
{
  xiiRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pPipeline, *this);
}

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiRenderPipelineResource, 1, xiiRTTIDefaultAllocator<xiiRenderPipelineResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiRenderPipelineResource);

xiiRenderPipelineResource::xiiRenderPipelineResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiInternal::NewInstance<xiiRenderPipeline> xiiRenderPipelineResource::CreateRenderPipeline() const
{
  if (GetLoadingState() != xiiResourceState::Loaded)
  {
    xiiLog::Error("Can't create render pipeline '{0}', the resource is not loaded!", GetResourceID());
    return xiiInternal::NewInstance<xiiRenderPipeline>(nullptr, nullptr);
  }

  return xiiRenderPipelineResourceLoader::CreateRenderPipeline(m_Description);
}

// static
xiiRenderPipelineResourceHandle xiiRenderPipelineResource::CreateMissingPipeline()
{
  xiiUniquePtr<xiiRenderPipeline> pRenderPipeline = XII_DEFAULT_NEW(xiiRenderPipeline);

  xiiCreateColourAttachmentPass* pColourSourcePass = nullptr;
  {
    xiiUniquePtr<xiiCreateColourAttachmentPass> pPass = XII_DEFAULT_NEW(xiiCreateColourAttachmentPass, "ColourSource");
    pColourSourcePass                                 = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  xiiTargetPass* pTargetPass = nullptr;
  {
    xiiUniquePtr<xiiTargetPass> pPass = XII_DEFAULT_NEW(xiiTargetPass);
    pTargetPass                       = pPass.Borrow();
    pRenderPipeline->AddPass(std::move(pPass));
  }

  XII_VERIFY(pRenderPipeline->Connect(pColourSourcePass, "Output", pTargetPass, "Colour0"), "Connect failed!");

  xiiRenderPipelineResourceDescriptor desc;
  xiiRenderPipelineResourceLoader::CreateRenderPipelineResourceDescriptor(pRenderPipeline.Borrow(), desc);

  return xiiResourceManager::CreateResource<xiiRenderPipelineResource>("MissingRenderPipeline", std::move(desc), "MissingRenderPipeline");
}

xiiResourceLoadDesc xiiRenderPipelineResource::UnloadData(Unload WhatToUnload)
{
  m_Description.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  return res;
}

xiiResourceLoadDesc xiiRenderPipelineResource::UpdateContent(xiiStreamReader* Stream)
{
  m_Description.Clear();

  xiiResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  xiiStringBuilder sAbsFilePath;
  (*Stream) >> sAbsFilePath;

  if (sAbsFilePath.HasExtension("xiiBinRenderPipeline"))
  {
    xiiStringBuilder sTemp, sTemp2;

    xiiAssetFileHeader AssetHash;
    AssetHash.Read(*Stream).IgnoreResult();

    xiiUInt8 uiVersion = 0;
    (*Stream) >> uiVersion;

    // Version 1 was using old tooling serialization. Code path removed.
    if (uiVersion == 1)
    {
      res.m_State = xiiResourceState::LoadedResourceMissing;
      xiiLog::Error("Failed to load old xiiRenderPipelineResource '{}'. Needs re-transform.", sAbsFilePath);
      return res;
    }
    XII_ASSERT_DEV(uiVersion == 2, "Unknown xiiBinRenderPipeline version {0}.", uiVersion);

    xiiUInt32 uiSize = 0;
    (*Stream) >> uiSize;

    m_Description.m_SerializedPipeline.SetCountUninitialized(uiSize);
    Stream->ReadBytes(m_Description.m_SerializedPipeline.GetData(), uiSize);

    XII_ASSERT_DEV(uiSize > 0, "RenderPipeline resource contains no pipeline data!");
  }
  else
  {
    XII_REPORT_FAILURE("The file '{0}' is unsupported, only '.xiiBinRenderPipeline' files can be loaded as xiiRenderPipelineResource.", sAbsFilePath);
  }

  return res;
}

void xiiRenderPipelineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = sizeof(xiiRenderPipelineResource) + (xiiUInt32)(m_Description.m_SerializedPipeline.GetCount());
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiRenderPipelineResource, xiiRenderPipelineResourceDescriptor)
{
  m_Description = descriptor;

  xiiResourceLoadDesc res;
  res.m_State                      = xiiResourceState::Loaded;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  return res;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_RenderPipelineResource);
