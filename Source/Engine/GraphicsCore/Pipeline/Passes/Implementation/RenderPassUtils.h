/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once

#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Pipeline/RenderPipelineConfiguration.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/Resources/Sampler.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Local names describe allocations within one reflected operation occurrence.
/// All cross-pass resources are supplied through compiled pin bindings.
class xiiRenderPassResourceScope
{
public:
  explicit xiiRenderPassResourceScope(const xiiRenderPipelinePassContext& context) : m_Graph(context.m_Graph), m_Previous(context.m_Graph.GetResourceNamespace())
  {
    xiiStringBuilder scope(m_Previous);
    scope.Append("/Operation/", context.m_sPassName);
    m_Graph.SetResourceNamespace(scope);
  }
  ~xiiRenderPassResourceScope() { m_Graph.SetResourceNamespace(m_Previous); }

private:
  xiiRenderGraph& m_Graph;
  xiiString       m_Previous;
};

namespace xiiRenderPassUtils
{
  xiiResult                                 LoadComputePipeline(xiiSharedPtr<xiiGALComputePipelineState>& pipeline, xiiStringView shader);
  xiiSharedPtr<xiiGALGraphicsPipelineState> LoadGraphicsPipeline(xiiSharedPtr<xiiGALGraphicsPipelineState>& pipeline, xiiStringView shader, const xiiSharedPtr<xiiGALRenderPass>& renderPass, xiiUInt32 subpass);
  xiiResult                                 PointClampSampler(xiiSharedPtr<xiiGALSampler>& sampler);
  void                                      UpdateBufferPreservingState(xiiGALCommandList& commands, xiiGALBuffer* buffer, xiiUInt32 offset, xiiArrayPtr<const xiiUInt8> data);
  void                                      ClearRenderTargetPreservingState(xiiGALCommandList& commands, xiiGALTextureView* view, const xiiColor& color);
  void                                      ClearDepthPreservingState(xiiGALCommandList& commands, xiiGALTextureView* view, bool depth, bool stencil, float value, xiiUInt8 stencilValue);
  xiiResult                                 LinearClampSampler(xiiSharedPtr<xiiGALSampler>& sampler);
  xiiRenderGraphBufferHandle                Constants(xiiRenderGraphBuilder& builder, xiiStringView name, xiiUInt32 size);
  xiiResult                                 OutputSize(const xiiRenderPipelinePassContext& context, xiiRenderGraphTextureHandle input, xiiUInt32 width, xiiUInt32 height, xiiSizeU32& out_size, bool useViewport = false);
  /// Per-pass cache, invalidated by authored configuration, view defaults or parameter changes.
  struct DisplayConfiguration
  {
    xiiDisplayOutputSettings m_Settings;
    xiiVariant               m_Override;
    const xiiView*           m_pView          = nullptr; // Compared only; never dereferenced outside Resolve's live context.
    xiiUInt64                m_uiViewRevision = 0;
    bool                     m_bValid         = false;
    xiiResult                Resolve(const xiiRenderPipelinePassContext& context, const xiiDisplayOutputSettings& authored, bool useViewDefaults);
  };
} // namespace xiiRenderPassUtils
