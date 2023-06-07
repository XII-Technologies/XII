#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <RendererCore/Pipeline/Renderer.h>

struct SpriteData;
class xiiRenderDataBatch;
using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

/// \brief Implements rendering of sprites
class XII_RENDERERCORE_DLL xiiSpriteRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpriteRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSpriteRenderer);

public:
  xiiSpriteRenderer();
  ~xiiSpriteRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(
    const xiiRenderViewContext&  renderContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;

protected:
  xiiGALBufferHandle CreateSpriteDataBuffer() const;
  void               DeleteSpriteDataBuffer(xiiGALBufferHandle hBuffer) const;
  virtual void       FillSpriteData(const xiiRenderDataBatch& batch, xiiUInt32 uiStartIndex, xiiUInt32 uiCount) const;

  xiiShaderResourceHandle                                         m_hShader;
  mutable xiiDynamicArray<SpriteData, xiiAlignedAllocatorWrapper> m_SpriteData;
};
