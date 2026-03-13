#pragma once

#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/Pipeline/Renderer.h>

class xiiSpriteRenderData;
struct xiiPerInstanceData;
struct xiiPerSpriteData;

class XII_GRAPHICSCORE_DLL xiiSpriteRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSpriteRenderer, xiiRenderer);

  XII_DISALLOW_COPY_AND_ASSIGN(xiiSpriteRenderer);

public:
  xiiSpriteRenderer();
  ~xiiSpriteRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiDynamicArray<const xiiRTTI*>& out_types) const override;

  virtual void RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  xiiSharedPtr<xiiGALBuffer> CreateSpriteDataBuffer(xiiUInt32 uiBufferSize) const;
  void                       DeleteSpriteDataBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer) const;
  virtual void               FillSpriteData(const xiiRenderDataBatch& batch) const;

  xiiShaderResourceHandle                                               m_hShader;
  mutable xiiDynamicArray<xiiPerSpriteData, xiiAlignedAllocatorWrapper> m_SpriteData;
};
