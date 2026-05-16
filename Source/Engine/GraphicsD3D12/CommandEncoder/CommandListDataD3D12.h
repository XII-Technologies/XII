/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsD3D12/GraphicsD3D12DLL.h>

#include <GraphicsD3D12/Pools/DescriptorSetPoolD3D12.h>
#include <GraphicsD3D12/Pools/DynamicBufferPoolD3D12.h>
#include <GraphicsD3D12/Pools/StagingBufferPoolD3D12.h>
#include <GraphicsD3D12/Resources/BufferD3D12.h>
#include <GraphicsD3D12/Resources/BufferViewD3D12.h>
#include <GraphicsD3D12/Resources/SamplerD3D12.h>
#include <GraphicsD3D12/Resources/TextureViewD3D12.h>
#include <GraphicsD3D12/Resources/TopLevelASD3D12.h>
#include <GraphicsFoundation/CommandEncoder/CommandList.h>

struct XII_GRAPHICSD3D12_DLL xiiGALCommandListDataD3D12
{
  struct ResourceSetBindings
  {
    XII_ALWAYS_INLINE ResourceSetBindings()  = default;
    XII_ALWAYS_INLINE ~ResourceSetBindings() = default;
    XII_ALWAYS_INLINE ResourceSetBindings(ResourceSetBindings&& other) noexcept
    {
      m_pBoundConstantBuffers                     = std::move(other.m_pBoundConstantBuffers);
      m_pBoundBufferResourceViews                 = std::move(other.m_pBoundBufferResourceViews);
      m_pBoundTextureResourceViews                = std::move(other.m_pBoundTextureResourceViews);
      m_pBoundAccelerationStructures              = std::move(other.m_pBoundAccelerationStructures);
      m_pBoundUnorderedAccessBufferResourceViews  = std::move(other.m_pBoundUnorderedAccessBufferResourceViews);
      m_pBoundUnorderedAccessTextureResourceViews = std::move(other.m_pBoundUnorderedAccessTextureResourceViews);
      m_pBoundSamplerStates                       = std::move(other.m_pBoundSamplerStates);
    }

    xiiDynamicArray<xiiGALBufferD3D12*>      m_pBoundConstantBuffers;
    xiiDynamicArray<xiiGALBufferViewD3D12*>  m_pBoundBufferResourceViews;
    xiiDynamicArray<xiiGALTextureViewD3D12*> m_pBoundTextureResourceViews;
    xiiDynamicArray<xiiGALTopLevelASD3D12*>  m_pBoundAccelerationStructures;
    xiiDynamicArray<xiiGALBufferViewD3D12*>  m_pBoundUnorderedAccessBufferResourceViews;
    xiiDynamicArray<xiiGALTextureViewD3D12*> m_pBoundUnorderedAccessTextureResourceViews;
    xiiDynamicArray<xiiGALSamplerD3D12*>     m_pBoundSamplerStates;
  };

  XII_ALWAYS_INLINE xiiGALCommandListDataD3D12()  = default;
  XII_ALWAYS_INLINE ~xiiGALCommandListDataD3D12() = default;
  XII_ALWAYS_INLINE xiiGALCommandListDataD3D12(xiiGALCommandListDataD3D12&& other) noexcept
  {
    m_pBoundRenderTargets         = std::move(other.m_pBoundRenderTargets);
    m_pBoundDepthStencilTarget    = std::move(other.m_pBoundDepthStencilTarget);
    m_uiBoundRenderTargetCount    = other.m_uiBoundRenderTargetCount;
    m_uiSubpassIndex              = other.m_uiSubpassIndex;
    m_AttachmentClearValues       = std::move(other.m_AttachmentClearValues);
    m_bPipelineStateModified      = other.m_bPipelineStateModified;
    m_ResourceSets                = std::move(other.m_ResourceSets);
    m_uiActiveQueriesCounter      = other.m_uiActiveQueriesCounter;
  }

  XII_ALWAYS_INLINE xiiGALCommandListDataD3D12& operator=(xiiGALCommandListDataD3D12&& other) noexcept
  {
    m_pBoundRenderTargets         = std::move(other.m_pBoundRenderTargets);
    m_pBoundDepthStencilTarget    = std::move(other.m_pBoundDepthStencilTarget);
    m_uiBoundRenderTargetCount    = other.m_uiBoundRenderTargetCount;
    m_uiSubpassIndex              = other.m_uiSubpassIndex;
    m_AttachmentClearValues       = std::move(other.m_AttachmentClearValues);
    m_bPipelineStateModified      = other.m_bPipelineStateModified;
    m_ResourceSets                = std::move(other.m_ResourceSets);

    m_uiActiveQueriesCounter      = other.m_uiActiveQueriesCounter;

    return *this;
  }

  XII_ALWAYS_INLINE void Invalidate()
  {
    for (ResourceSetBindings& setBindings : m_ResourceSets)
    {
      setBindings.m_pBoundConstantBuffers.Clear();
      setBindings.m_pBoundBufferResourceViews.Clear();
      setBindings.m_pBoundTextureResourceViews.Clear();
      setBindings.m_pBoundAccelerationStructures.Clear();
      setBindings.m_pBoundUnorderedAccessBufferResourceViews.Clear();
      setBindings.m_pBoundUnorderedAccessTextureResourceViews.Clear();
      setBindings.m_pBoundSamplerStates.Clear();
    }

    m_bPipelineStateModified = true;

    m_pBoundRenderTargets.Clear();
    m_pBoundDepthStencilTarget.Clear();
    m_uiBoundRenderTargetCount = 0U;

    m_uiSubpassIndex = 0U;
    m_AttachmentClearValues.Clear();
  }

  XII_ALWAYS_INLINE void Reset()
  {
    Invalidate();
  }

  xiiHybridArray<xiiSharedPtr<xiiGALTextureViewD3D12>, 2U> m_pBoundRenderTargets;
  xiiSharedPtr<xiiGALTextureViewD3D12>                     m_pBoundDepthStencilTarget;
  xiiUInt32                                                m_uiBoundRenderTargetCount = 0U;

  xiiUInt32                             m_uiSubpassIndex = 0U;
  xiiHybridArray<D3D12_CLEAR_VALUE, 2U> m_AttachmentClearValues;

  bool m_bPipelineStateModified = false;

  xiiHybridArray<ResourceSetBindings, 1U>  m_ResourceSets;

  xiiUInt32 m_uiActiveQueriesCounter = 0U;
};
