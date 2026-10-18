/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once
#include <GraphicsCore/Lighting/ShadowCascade.h>
#include <GraphicsCore/Lighting/VirtualShadowMap.h>
#include <GraphicsCore/Pipeline/RenderGraph.h>
#include <GraphicsCore/Pipeline/RenderPipelineFrameData.h>

/// Derived camera/light projection values, not authoritative scene transforms.
class XII_GRAPHICSCORE_DLL xiiShadowCascadeSnapshot final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiShadowCascadeSnapshot, xiiRenderPipelineFrameData);

public:
  xiiShadowCascadeSnapshot(xiiUInt64 setup, const xiiStaticArray<xiiShadowCascadeDescription, 4>& cascades, xiiUInt32 resolution, xiiRenderGraphBufferHandle constants) :
    xiiRenderPipelineFrameData(setup), m_Cascades(cascades), m_uiResolution(resolution), m_hConstants(constants) {}
  const xiiStaticArray<xiiShadowCascadeDescription, 4> m_Cascades;
  const xiiUInt32                                      m_uiResolution;
  const xiiRenderGraphBufferHandle                     m_hConstants;
};

/// Frame-local GPU bindings and addressing, never serialized scene identities.
struct xiiVirtualShadowTableBindings : xiiVirtualShadowGpuLayout
{
  xiiRenderGraphBufferHandle  m_hPhysicalPageTable, m_hVirtualPageTable;
  xiiRenderGraphTextureHandle m_hPhysicalAtlas;
};

class XII_GRAPHICSCORE_DLL xiiVirtualShadowResidencySnapshot final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowResidencySnapshot, xiiRenderPipelineFrameData);

public:
  xiiVirtualShadowResidencySnapshot(xiiUInt64 setup, const xiiVirtualShadowTableBindings& bindings, xiiSharedPtr<const xiiVirtualShadowTableUpload> upload) :
    xiiRenderPipelineFrameData(setup), m_Bindings(bindings), m_Upload(std::move(upload)) {}
  const xiiVirtualShadowTableBindings                   m_Bindings;
  const xiiSharedPtr<const xiiVirtualShadowTableUpload> m_Upload;
};

/// A bounded immutable device-page work batch with explicit residency provenance.
/// It owns no world objects, components or scene hierarchy.
class XII_GRAPHICSCORE_DLL xiiVirtualShadowRasterBatch final : public xiiRenderPipelineFrameData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiVirtualShadowRasterBatch, xiiRenderPipelineFrameData);

public:
  xiiVirtualShadowRasterBatch(xiiUInt64 setup, xiiSharedPtr<const xiiVirtualShadowResidencySnapshot> residency, xiiDynamicArray<xiiVirtualShadowRasterPage>&& pages) :
    xiiRenderPipelineFrameData(setup), m_Residency(std::move(residency)), m_Pages(std::move(pages)) {}
  const xiiSharedPtr<const xiiVirtualShadowResidencySnapshot> m_Residency;
  const xiiDynamicArray<xiiVirtualShadowRasterPage>           m_Pages;
};
