/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/ResourceManager/Implementation/ResourceLock.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Shader/ShaderPermutationResource.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Visibility/GpuHiZPyramid.h>
#include <GraphicsFoundation/Tools/MapHelper.h>
#include <Shaders/Visibility/GpuHiZBuildConstants.h>

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGpuHiZPyramidDescription, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiGpuHiZPyramidDescription>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("FramesInFlight", m_uiFramesInFlight)->AddAttributes(new xiiClampValueAttribute(2U, 8U)),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

namespace
{
  struct ImportPassData
  {
    xiiRenderGraphTextureHandle m_hHistory;
  };

  struct BuildPassData
  {
    xiiRenderGraphTextureHandle m_hSceneDepth;
    xiiRenderGraphTextureHandle m_hDestination;
    xiiRenderGraphBufferHandle m_hConstants;
    xiiSharedPtr<xiiGALComputePipelineState> m_pPipeline;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pSourceViews;
    xiiDynamicArray<xiiSharedPtr<xiiGALTextureView>> m_pDestinationViews;
    xiiSizeU32 m_Size;
  };

  void TransitionMip(xiiGALCommandList& commandList, xiiGALTexture* pTexture, xiiUInt32 uiMip,
    xiiBitflags<xiiGALResourceStateFlags> oldState, xiiBitflags<xiiGALResourceStateFlags> newState)
  {
    xiiGALStateTransitionDescription transition;
    transition.m_pResource = pTexture;
    transition.m_uiFirstMipLevel = uiMip;
    transition.m_uiMipLevelCount = 1U;
    transition.m_OldState = oldState;
    transition.m_NewState = newState;
    commandList.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1U));
  }
}

xiiGpuHiZPyramid::~xiiGpuHiZPyramid()
{
  Shutdown();
}

xiiResult xiiGpuHiZPyramid::Initialize(xiiGALDevice* pDevice, const xiiGpuHiZPyramidDescription& description)
{
  Shutdown();
  if (pDevice == nullptr || description.m_uiFramesInFlight < 2U)
    return XII_FAILURE;

  m_pDevice = pDevice;
  m_Description = description;
  m_pBuildPipeline = LoadComputePipeline("Shaders/Visibility/GpuHiZBuild.xiiShader");
  return m_pBuildPipeline != nullptr ? XII_SUCCESS : XII_FAILURE;
}

void xiiGpuHiZPyramid::Shutdown()
{
  m_Frames.Clear();
  m_pBuildPipeline.Clear();
  m_pDevice = nullptr;
  m_Description = {};
  m_Size = {};
  m_uiMipLevelCount = 0U;
  m_bHistoryValid = false;
}

xiiResult xiiGpuHiZPyramid::Resize(xiiUInt32 uiWidth, xiiUInt32 uiHeight)
{
  if (m_pDevice == nullptr || uiWidth == 0U || uiHeight == 0U)
    return XII_FAILURE;
  if (m_Size.width == uiWidth && m_Size.height == uiHeight && !m_Frames.IsEmpty())
    return XII_SUCCESS;

  m_Frames.Clear();
  m_Size = xiiSizeU32(uiWidth, uiHeight);
  m_uiMipLevelCount = 1U;
  for (xiiUInt32 width = uiWidth, height = uiHeight; width > 1U || height > 1U; )
  {
    width = xiiMath::Max(width >> 1U, 1U);
    height = xiiMath::Max(height >> 1U, 1U);
    ++m_uiMipLevelCount;
  }

  xiiGALTextureCreationDescription textureDescription;
  textureDescription.m_Type = xiiGALResourceDimension::Texture2D;
  textureDescription.m_Format = xiiGALResourceFormat::R32Float;
  textureDescription.m_Size = m_Size;
  textureDescription.m_uiMipLevels = m_uiMipLevelCount;
  textureDescription.m_BindFlags = xiiGALBindFlags::ShaderResource | xiiGALBindFlags::UnorderedAccess;
  textureDescription.m_Usage = xiiGALResourceUsage::Default;

  m_Frames.SetCount(m_Description.m_uiFramesInFlight);
  for (xiiUInt32 uiFrame = 0U; uiFrame < m_Frames.GetCount(); ++uiFrame)
  {
    FrameResources& frame = m_Frames[uiFrame];
    frame.m_pTexture = m_pDevice->CreateTexture(textureDescription);
    if (frame.m_pTexture == nullptr)
    {
      m_Frames.Clear();
      return XII_FAILURE;
    }

    xiiStringBuilder textureName;
    textureName.SetFormat("GPU Hi-Z History {}", uiFrame);
    frame.m_pTexture->SetDebugName(textureName);
    frame.m_pMipShaderResourceViews.SetCount(m_uiMipLevelCount);
    frame.m_pMipUnorderedAccessViews.SetCount(m_uiMipLevelCount);
    for (xiiUInt32 uiMip = 0U; uiMip < m_uiMipLevelCount; ++uiMip)
    {
      xiiGALTextureViewCreationDescription viewDescription;
      viewDescription.m_ViewType = xiiGALTextureViewType::ShaderResource;
      viewDescription.m_uiMostDetailedMip = uiMip;
      viewDescription.m_uiMipLevelCount = 1U;
      frame.m_pMipShaderResourceViews[uiMip] = frame.m_pTexture->CreateView(viewDescription);

      viewDescription.m_ViewType = xiiGALTextureViewType::UnorderedAccess;
      frame.m_pMipUnorderedAccessViews[uiMip] = frame.m_pTexture->CreateView(viewDescription);
      if (frame.m_pMipShaderResourceViews[uiMip] == nullptr || frame.m_pMipUnorderedAccessViews[uiMip] == nullptr)
      {
        m_Frames.Clear();
        return XII_FAILURE;
      }
    }
  }

  m_bHistoryValid = false;
  return XII_SUCCESS;
}

void xiiGpuHiZPyramid::Invalidate()
{
  m_bHistoryValid = false;
}

xiiRenderGraphTextureHandle xiiGpuHiZPyramid::ImportPrevious(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex)
{
  if (!m_bHistoryValid || m_Frames.IsEmpty() || uiFrameIndex == 0U)
    return {};

  const xiiUInt32 uiSlot = static_cast<xiiUInt32>((uiFrameIndex - 1U) % m_Frames.GetCount());
  auto import = graph.AddPass<ImportPassData>(
    "Import Previous GPU Hi-Z", xiiGALCommandQueueFlags::Compute,
    [this, uiSlot](ImportPassData& data, xiiRenderGraphBuilder& builder) {
      xiiStringBuilder name;
      name.SetFormat("GPU Hi-Z History {}", uiSlot);
      data.m_hHistory = builder.ImportTexture(name, m_Frames[uiSlot].m_pTexture, m_Frames[uiSlot].m_pTexture->GetResourceState());
    },
    [](const ImportPassData&, xiiRenderGraphPassContext&) {});
  return import.first->m_hHistory;
}

void xiiGpuHiZPyramid::AddBuildPass(xiiRenderGraph& graph, xiiUInt64 uiFrameIndex, xiiRenderGraphTextureHandle hSceneDepth, bool bAsyncCompute)
{
  if (m_Frames.IsEmpty() || !hSceneDepth.IsValid())
    return;

  const xiiUInt32 uiSlot = static_cast<xiiUInt32>(uiFrameIndex % m_Frames.GetCount());
  const xiiBitflags<xiiGALCommandQueueFlags> queue = bAsyncCompute ? xiiGALCommandQueueFlags::Compute : xiiGALCommandQueueFlags::Graphics;
  auto build = graph.AddPass<BuildPassData>(
    "Build GPU Hi-Z History", queue,
    [this, uiSlot, hSceneDepth](BuildPassData& data, xiiRenderGraphBuilder& builder) {
      data.m_hSceneDepth = builder.ReadTexture(hSceneDepth, xiiGALResourceStateFlags::DepthRead);
      xiiStringBuilder name;
      name.SetFormat("GPU Hi-Z History {}", uiSlot);
      const xiiRenderGraphTextureHandle hImported = builder.ImportTexture(name, m_Frames[uiSlot].m_pTexture, m_Frames[uiSlot].m_pTexture->GetResourceState());
      data.m_hDestination = builder.WriteTexture(hImported, xiiGALResourceStateFlags::UnorderedAccess);

      xiiGALBufferCreationDescription constantsDescription;
      constantsDescription.m_uiSize = sizeof(xiiGpuHiZBuildConstants);
      constantsDescription.m_BindFlags = xiiGALBindFlags::UniformBuffer;
      constantsDescription.m_Usage = xiiGALResourceUsage::Dynamic;
      constantsDescription.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
      data.m_hConstants = builder.WriteBuffer("GPU Hi-Z Build Constants", constantsDescription, xiiGALResourceStateFlags::ConstantBuffer);
      builder.SetPassSideEffects(true);
      builder.SetPassAllowMerge(false);
    },
    [this](const BuildPassData& data, xiiRenderGraphPassContext& context) {
      xiiGALCommandList& commandList = context.GetCommandList();
      xiiGALTexture* pDestination = context.GetTexture(data.m_hDestination);
      xiiGALBuffer* pConstants = context.GetBuffer(data.m_hConstants);
      // Whole-resource state tracking cannot represent the deliberately mixed per-mip layouts
      // used while reducing the pyramid. Unknown hands subresource ownership to this pass until
      // all mips have been restored to UnorderedAccess below.
      pDestination->SetResourceState(xiiGALResourceStateFlags::Unknown);
      commandList.SetPipelineState(data.m_pPipeline.Borrow());
      commandList.ResolveAndSetConstantBuffer("xiiGpuHiZBuildConstants", pConstants, xiiGALShaderType::Compute);

      xiiUInt32 uiSourceWidth = data.m_Size.width;
      xiiUInt32 uiSourceHeight = data.m_Size.height;
      for (xiiUInt32 uiMip = 0U; uiMip < data.m_pDestinationViews.GetCount(); ++uiMip)
      {
        const bool bCopyDepth = uiMip == 0U;
        const xiiUInt32 uiDestinationWidth = bCopyDepth ? uiSourceWidth : xiiMath::Max(uiSourceWidth >> 1U, 1U);
        const xiiUInt32 uiDestinationHeight = bCopyDepth ? uiSourceHeight : xiiMath::Max(uiSourceHeight >> 1U, 1U);

        {
          xiiGALMapHelper<xiiGpuHiZBuildConstants> constants(commandList, pConstants, xiiGALMapType::Write, xiiGALMapFlags::Discard);
          constants->SrcSize = xiiVec2U32(uiSourceWidth, uiSourceHeight);
          constants->DstSize = xiiVec2U32(uiDestinationWidth, uiDestinationHeight);
          constants->Reduce = bCopyDepth ? 0U : 1U;
        }
        xiiGALStateTransitionDescription constantsTransition;
        constantsTransition.m_pResource = pConstants;
        constantsTransition.m_OldState = xiiGALResourceStateFlags::CopyDestination;
        constantsTransition.m_NewState = xiiGALResourceStateFlags::ConstantBuffer;
        constantsTransition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
        commandList.TransitionResourceStates(xiiMakeArrayPtr(&constantsTransition, 1U));

        xiiSharedPtr<xiiGALTextureView> pSource = bCopyDepth
          ? context.GetTexture(data.m_hSceneDepth)->GetDefaultView(xiiGALTextureViewType::ShaderResource)
          : data.m_pSourceViews[uiMip - 1U];
        commandList.ResolveAndSetShaderResourceTextureView("g_HiZSource", pSource.Borrow(), xiiGALShaderType::Compute);
        commandList.ResolveAndSetUnorderedAccessTextureView("g_HiZDestination", data.m_pDestinationViews[uiMip].Borrow(), xiiGALShaderType::Compute);
        commandList.CommitShaderResources(xiiGALStateTransitionMode::None).AssertSuccess();
        commandList.DispatchCompute({(uiDestinationWidth + 7U) / 8U, (uiDestinationHeight + 7U) / 8U, 1U});

        if (uiMip + 1U < data.m_pDestinationViews.GetCount())
          TransitionMip(commandList, pDestination, uiMip, xiiGALResourceStateFlags::UnorderedAccess, xiiGALResourceStateFlags::ShaderResource);

        uiSourceWidth = uiDestinationWidth;
        uiSourceHeight = uiDestinationHeight;
      }

      // Restore every sampled mip to GENERAL/UnorderedAccess. The texture's global GAL state
      // remains UnorderedAccess throughout; the temporary transitions intentionally do not use
      // UpdateState because only individual subresources changed layout.
      for (xiiUInt32 uiMip = 0U; uiMip + 1U < data.m_pDestinationViews.GetCount(); ++uiMip)
        TransitionMip(commandList, pDestination, uiMip, xiiGALResourceStateFlags::ShaderResource, xiiGALResourceStateFlags::UnorderedAccess);
      pDestination->SetResourceState(xiiGALResourceStateFlags::UnorderedAccess);

      // Promote temporal history only after the render graph actually records this pass. Merely
      // scheduling a build is insufficient: graph compilation may fail or execution may be
      // skipped, in which case exposing this ring slot next frame would sample unwritten data.
      m_bHistoryValid = true;
    }, true);

  build.first->m_pPipeline = m_pBuildPipeline;
  build.first->m_pSourceViews = m_Frames[uiSlot].m_pMipShaderResourceViews;
  build.first->m_pDestinationViews = m_Frames[uiSlot].m_pMipUnorderedAccessViews;
  build.first->m_Size = m_Size;
}

xiiSharedPtr<xiiGALComputePipelineState> xiiGpuHiZPyramid::LoadComputePipeline(xiiStringView sShaderPath)
{
  const xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);
  xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
  const xiiShaderPermutationResourceHandle hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, variables, true);
  xiiResourceLock<xiiShaderPermutationResource> permutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid())
    return nullptr;

  xiiGALComputePipelineStateCreationDescription description;
  description.m_pComputeShader = permutation->GetGALShader(xiiGALShaderType::Compute);
  description.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
  return xiiGALPipelineCache::GetPipeline(description);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Visibility_Implementation_GpuHiZPyramid);
