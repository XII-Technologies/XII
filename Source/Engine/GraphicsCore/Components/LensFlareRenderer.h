#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/Pipeline/Renderer.h>

struct xiiPerLensFlareData;
class xiiRenderDataBatch;
using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

/// \brief Implements rendering of lens flares
class XII_GRAPHICSCORE_DLL xiiLensFlareRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLensFlareRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiLensFlareRenderer);

public:
  xiiLensFlareRenderer();
  ~xiiLensFlareRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;
  virtual void RenderBatch(const xiiRenderViewContext& renderContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const override;

protected:
  xiiSharedPtr<xiiGALBuffer> CreateLensFlareDataBuffer(xiiUInt32 uiBufferSize) const;
  void                       DeleteLensFlareDataBuffer(xiiSharedPtr<xiiGALBuffer> pBuffer) const;
  virtual void               FillLensFlareData(const xiiRenderDataBatch& batch) const;

  xiiShaderResourceHandle                                                  m_hShader;
  mutable xiiDynamicArray<xiiPerLensFlareData, xiiAlignedAllocatorWrapper> m_LensFlareData;
};
