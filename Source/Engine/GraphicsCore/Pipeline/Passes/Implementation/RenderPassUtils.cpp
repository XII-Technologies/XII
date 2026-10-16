/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#include <Core/ResourceManager/ResourceManager.h>
#include <GraphicsCore/GraphicsCorePCH.h>
#include <GraphicsCore/Pipeline/Passes/Implementation/RenderPassUtils.h>
#include <GraphicsCore/Pipeline/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>
#include <GraphicsFoundation/Device/Device.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Resources/Texture.h>

xiiResult xiiRenderPassUtils::LoadComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& pipeline, xiiStringView shader)
{
  if (pipeline)
    return XII_SUCCESS;
  XII_SCOPE_EXIT(if (!pipeline) xiiLog::Error("Failed to create reflected compute operation pipeline '{}'.", shader));
  if (!xiiGALDevice::HasDefaultDevice() || xiiGALDevice::GetDefaultDevice()->GetFeatures().m_ComputeShaders != xiiGALDeviceFeatureState::Enabled)
    return XII_FAILURE;
  const auto                                     resource = xiiResourceManager::LoadResource<xiiShaderResource>(shader);
  xiiHashTable<xiiHashedString, xiiHashedString> variables(xiiTemporaryAllocator::Get());
  const auto                                     handle = xiiShaderPermutationUtilities::PreloadSinglePermutation(resource, variables, false);
  if (!handle.IsValid())
    return XII_FAILURE;
  xiiResourceLock<xiiShaderPermutationResource> permutation(handle, xiiResourceAcquireMode::BlockTillLoaded);
  if (!permutation.IsValid() || !permutation->IsShaderValid())
    return XII_FAILURE;
  xiiGALComputePipelineStateCreationDescription description;
  description.m_pComputeShader             = permutation->GetGALShader(xiiGALShaderType::Compute);
  description.m_pPipelineResourceSignature = permutation->GetPipelineResourceSignature();
  pipeline                                 = xiiGALPipelineCache::GetPipeline(description);
  return pipeline ? XII_SUCCESS : XII_FAILURE;
}

xiiResult xiiRenderPassUtils::LinearClampSampler(xiiSharedPtr<xiiGALSampler>& sampler)
{
  if (!sampler)
  {
    if (!xiiGALDevice::HasDefaultDevice())
      return XII_FAILURE;
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter = xiiGALFilterType::Linear;
    description.m_MagFilter = xiiGALFilterType::Linear;
    description.m_MipFilter = xiiGALFilterType::Linear;
    sampler                 = xiiGALDevice::GetDefaultDevice()->CreateSampler(description);
  }
  return sampler ? XII_SUCCESS : XII_FAILURE;
}

xiiRenderGraphBufferHandle xiiRenderPassUtils::Constants(xiiRenderGraphBuilder& builder, xiiStringView name, xiiUInt32 size)
{
  xiiGALBufferCreationDescription description;
  description.m_uiSize         = size;
  description.m_BindFlags      = xiiGALBindFlags::UniformBuffer;
  description.m_Usage          = xiiGALResourceUsage::Dynamic;
  description.m_CPUAccessFlags = xiiGALCPUAccessFlag::Write;
  return builder.WriteBuffer(name, description, xiiGALResourceStateFlags::ConstantBuffer);
}

xiiResult xiiRenderPassUtils::OutputSize(const xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle input, xiiUInt32 width, xiiUInt32 height, xiiSizeU32& out_size, bool useViewport)
{
  const auto* description = context.m_Graph.GetTextureDescription(input);
  if (!description)
    return XII_FAILURE;
  if (context.m_pView && useViewport)
  {
    const auto viewport = context.m_pView->GetViewport();
    if (!xiiMath::IsFinite(viewport.width) || !xiiMath::IsFinite(viewport.height) || viewport.width > 65536 || viewport.height > 65536)
      return XII_FAILURE;
    out_size = {width ? width : static_cast<xiiUInt32>(xiiMath::Max(viewport.width, 1.0f)), height ? height : static_cast<xiiUInt32>(xiiMath::Max(viewport.height, 1.0f))};
  }
  else
    out_size = {width ? width : description->m_Size.width, height ? height : description->m_Size.height};
  return out_size.width && out_size.height ? XII_SUCCESS : XII_FAILURE;
}

void xiiRenderPassUtils::ResourcePins(xiiDynamicArray<xiiRenderPipelinePin>& pins)
{
  pins.PushBack({"Before", xiiRenderPipelinePin::Order, true, true});
  pins.PushBack({"After", xiiRenderPipelinePin::Order, false, true});
}

xiiResult xiiRenderPassUtils::DisplayConfiguration::Resolve(const xiiRenderPipelinePassContext& context, const xiiDisplayOutputSettings& authored, bool useViewDefaults)
{
  const auto*      override = context.m_Parameters.GetValue("DisplayOutput");
  const xiiVariant value    = override ? *override : xiiVariant();
  const xiiUInt64  revision = context.m_pView && useViewDefaults ? context.m_pView->GetDisplayOutputSettingsRevision() : 0;
  if (m_bValid && context.m_pView == m_pView && revision == m_uiViewRevision && value == m_Override)
    return XII_SUCCESS;
  auto settings = context.m_pView && useViewDefaults ? context.m_pView->GetDisplayOutputSettings() : authored;
  XII_SUCCEED_OR_RETURN(xiiResolvePipelineDisplaySettings(context.m_Parameters, settings));
  m_Settings       = settings;
  m_Override       = value;
  m_pView          = context.m_pView;
  m_uiViewRevision = revision;
  m_bValid         = true;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Passes_Implementation_RenderPassUtils);

namespace xiiRenderPassUtils
{
  // A pass may temporarily use transfer commands, but subsequent graph barriers still expect
  // the resource state declared by that pass. Restore it before returning from the callback.
  void UpdateBufferPreservingState(xiiGALCommandList& commandList, xiiGALBuffer* buffer, xiiUInt32 offset, xiiArrayPtr<const xiiUInt8> data)
  {
    const auto state = buffer->GetResourceState();
    commandList.UpdateBuffer(buffer, offset, data);
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = buffer;
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commandList.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  }
  static void RestoreTextureState(xiiGALCommandList& commandList, xiiGALTexture* texture, xiiBitflags<xiiGALResourceStateFlags> state)
  {
    if (texture->GetResourceState() == state)
      return;
    xiiGALStateTransitionDescription transition;
    transition.m_pResource       = texture;
    transition.m_NewState        = state;
    transition.m_TransitionFlags = xiiGALStateTransitionFlags::UpdateState;
    commandList.TransitionResourceStates(xiiMakeArrayPtr(&transition, 1));
  }
  void ClearRenderTargetPreservingState(xiiGALCommandList& commandList, xiiGALTextureView* view, const xiiColor& color)
  {
    const auto state = view->GetTexture()->GetResourceState();
    commandList.ClearRenderTargetView(view, color);
    RestoreTextureState(commandList, view->GetTexture(), state);
  }
  void ClearDepthPreservingState(xiiGALCommandList& commandList, xiiGALTextureView* view, bool depth, bool stencil, float value, xiiUInt8 stencilValue)
  {
    const auto state = view->GetTexture()->GetResourceState();
    commandList.ClearDepthStencilView(view, depth, stencil, value, stencilValue);
    RestoreTextureState(commandList, view->GetTexture(), state);
  }


} // namespace xiiRenderPassUtils

xiiResult xiiRenderPassUtils::PointClampSampler(xiiSharedPtr<xiiGALSampler>& sampler)
{
  if (!sampler)
  {
    if (!xiiGALDevice::HasDefaultDevice())
      return XII_FAILURE;
    xiiGALSamplerCreationDescription description;
    description.m_MinFilter = xiiGALFilterType::Point;
    description.m_MagFilter = xiiGALFilterType::Point;
    description.m_MipFilter = xiiGALFilterType::Point;
    sampler                 = xiiGALDevice::GetDefaultDevice()->CreateSampler(description);
  }
  return sampler ? XII_SUCCESS : XII_FAILURE;
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiRenderPassUtils::LoadGraphicsPipeline(xiiSharedPtr<xiiGALGraphicsPipelineState>& inout_pPipeline, xiiStringView sShaderPath, const xiiSharedPtr<xiiGALRenderPass>& pRenderPass, xiiUInt32 uiSubpassIndex)
{
  XII_ASSERT_DEV(pRenderPass != nullptr, "A graphics pipeline requires a compatible render pass.");

  if (inout_pPipeline != nullptr)
  {
    const xiiGALGraphicsPipelineDescription& pipelineDescription = inout_pPipeline->GetDescription().m_GraphicsPipeline;
    if (pipelineDescription.m_pRenderPass == pRenderPass && pipelineDescription.m_uiSubpassIndex == uiSubpassIndex)
      return inout_pPipeline;
  }

  xiiShaderResourceHandle hShader = xiiResourceManager::LoadResource<xiiShaderResource>(sShaderPath);

  xiiHashTable<xiiHashedString, xiiHashedString> permutationVariables(xiiTemporaryAllocator::Get());
  xiiShaderPermutationResourceHandle             hPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(hShader, permutationVariables, false);
  if (!hPermutation.IsValid())
    return {};

  xiiResourceLock<xiiShaderPermutationResource> pPermutation(hPermutation, xiiResourceAcquireMode::BlockTillLoaded);
  if (!pPermutation.IsValid() || !pPermutation->IsShaderValid())
    return nullptr;

  xiiGALGraphicsPipelineStateCreationDescription description;
  description.m_pPipelineResourceSignature            = pPermutation->GetPipelineResourceSignature();
  description.m_pVertexShader                         = pPermutation->GetGALShader(xiiGALShaderType::Vertex);
  description.m_pPixelShader                          = pPermutation->GetGALShader(xiiGALShaderType::Pixel);
  description.m_GraphicsPipeline.m_pBlendState        = pPermutation->GetBlendState();
  description.m_GraphicsPipeline.m_pRasterizerState   = pPermutation->GetRasterizerState();
  description.m_GraphicsPipeline.m_pDepthStencilState = pPermutation->GetDepthStencilState();
  description.m_GraphicsPipeline.m_pRenderPass        = pRenderPass;
  description.m_GraphicsPipeline.m_uiSubpassIndex     = static_cast<xiiUInt8>(uiSubpassIndex);
  description.m_GraphicsPipeline.m_PrimitiveTopology  = xiiGALPrimitiveTopology::TriangleList;

  inout_pPipeline = xiiGALPipelineCache::GetPipeline(description);
  if (!inout_pPipeline)
    xiiLog::Error("Failed to create reflected graphics operation pipeline '{}'.", sShaderPath);

  return inout_pPipeline;
}
