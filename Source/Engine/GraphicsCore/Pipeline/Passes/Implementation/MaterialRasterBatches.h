/// Copyright (c) Theophilus Eriata. All Rights Reserved.
#pragma once
#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <GraphicsCore/Material/MaterialShaderBinding.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsFoundation/States/PipelineState.h>

/// Per-occurrence native material scheduling. GPU visibility is partitioned once
/// per stream into linear storage; it never reads visibility back to the CPU.
struct xiiMaterialRasterBatch
{
  xiiMaterialShaderBinding   m_Binding;
  xiiRenderGraphBufferHandle m_Constants, m_ViewConstants;
};
class xiiMaterialRasterBatches
{
public:
  xiiResult Build(xiiRenderPipelinePassContext& context, xiiRenderGraphBufferHandle visible, xiiRenderGraphBufferHandle count,
                  xiiDynamicArray<xiiMaterialRasterBatch>& batches, xiiRenderGraphBufferHandle& sortedVisible, xiiRenderGraphBufferHandle& draws, bool bDepth = false, bool bTransparent = false,
                  const xiiHashTable<xiiHashedString, xiiHashedString>* pViewPermutations = nullptr, bool bPicking = false);

private:
  struct CachedBinding
  {
    xiiSharedPtr<xiiMaterialInstance> m_Instance;
    xiiUInt32                         m_uiRevision = 0;
    xiiMaterialShaderBinding          m_Binding;
  };
  xiiMap<xiiUInt32, CachedBinding>         m_Bindings;
  xiiSharedPtr<xiiGALComputePipelineState> m_Count, m_Prefix, m_Scatter;
};
