#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/Pipeline/Renderer.h>

struct xiiPerSpriteData;
class xiiRenderDataBatch;
using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

/// \brief Implements rendering of sprites
class XII_GRAPHICSCORE_DLL xiiSpriteRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpriteRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiSpriteRenderer);

public:
  xiiSpriteRenderer();
  ~xiiSpriteRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void UpdateBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) override;
  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  xiiGALBufferHandle CreateSpriteDataBuffer(xiiUInt32 uiBufferSize) const;
  void               DeleteSpriteDataBuffer(xiiGALBufferHandle hBuffer) const;
  virtual void       FillSpriteData(const xiiRenderDataBatch& batch) const;

  xiiGALBufferHandle                                                    m_hSpriteData;
  xiiShaderResourceHandle                                               m_hShader;
  mutable xiiDynamicArray<xiiPerSpriteData, xiiAlignedAllocatorWrapper> m_SpriteData;
};
