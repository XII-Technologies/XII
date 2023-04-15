#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RendererCore/Meshes/MeshBufferResource.h>
#include <RendererCore/Pipeline/Renderer.h>

using xiiShaderResourceHandle = xiiTypedResourceHandle<class xiiShaderResource>;

class XII_RMLUIPLUGIN_DLL xiiRmlUiRenderer : public xiiRenderer
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiRenderer, xiiRenderer);
  XII_DISALLOW_COPY_AND_ASSIGN(xiiRmlUiRenderer);

public:
  xiiRmlUiRenderer();
  ~xiiRmlUiRenderer();

  // xiiRenderer implementation
  virtual void GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const override;
  virtual void GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const override;

  virtual void RenderBatch(
    const xiiRenderViewContext&  renderViewContext,
    const xiiRenderPipelinePass* pPass,
    const xiiRenderDataBatch&    batch) const override;

private:
  void SetScissorRect(const xiiRenderViewContext& renderViewContext, const xiiRectFloat& rect, bool bEnable, bool bTransformRect) const;
  void PrepareStencil(const xiiRenderViewContext& renderViewContext, const xiiRectFloat& rect) const;

  xiiShaderResourceHandle        m_hShader;
  xiiConstantBufferStorageHandle m_hConstantBuffer;

  xiiGALBufferHandle m_hQuadIndexBuffer;

  xiiVertexDeclarationInfo m_VertexDeclarationInfo;

  mutable xiiMat4      m_mLastTransform = xiiMat4::IdentityMatrix();
  mutable xiiRectFloat m_LastRect       = xiiRectFloat(0, 0);
};
