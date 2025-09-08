#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Components/SpriteRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Shader/ShaderPermutationUtilities.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundation/Resources/Buffer.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>
#include <GraphicsFoundation/Utilities/DeviceUtilities.h>

#include <Shaders/Materials/SpriteData.h>
static_assert(sizeof(xiiPerSpriteData) == 48);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSpriteRenderer, 1, xiiRTTIDefaultAllocator<xiiSpriteRenderer>)
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiSpriteRenderer::xiiSpriteRenderer()
{
  m_hShader = xiiResourceManager::LoadResource<xiiShaderResource>("Shaders/Materials/SpriteMaterial.xiiShader");
}

xiiSpriteRenderer::~xiiSpriteRenderer() = default;

void xiiSpriteRenderer::GetSupportedRenderDataTypes(xiiHybridArray<const xiiRTTI*, 8>& ref_types) const
{
  ref_types.PushBack(xiiGetStaticRTTI<xiiSpriteRenderData>());
}

void xiiSpriteRenderer::GetSupportedRenderDataCategories(xiiHybridArray<xiiRenderData::Category, 8>& ref_categories) const
{
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitMasked);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::LitTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleOpaque);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Selection);
}

void xiiSpriteRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiSpriteRenderData* pRenderData  = batch.GetFirstData<xiiSpriteRenderData>();
  const xiiUInt32            uiBufferSize = xiiMath::RoundUp(batch.GetCount(), 128u);
  xiiSharedPtr<xiiGALBuffer> pSpriteData  = CreateSpriteDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteSpriteDataBuffer(pSpriteData));

  renderViewContext.SetShaderPermutationVariable("BLEND_MODE", xiiSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  renderViewContext.SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == xiiSpriteBlendMode::ShapeIcon ? xiiMakeHashedString("TRUE") : xiiMakeHashedString("FALSE"));

  xiiSharedPtr<xiiGALGraphicsPipelineState> pGraphicsPipelineState = CreatePipelineState(renderViewContext);

  renderViewContext.m_pCommandList->SetPipelineState(pGraphicsPipelineState);
  renderViewContext.m_pCommandList->ResolveAndSetShaderResourceBufferView("xiiGlobalConstants", renderViewContext.m_CommandListData.m_pGlobalConstants->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  renderViewContext.m_pCommandList->ResolveAndSetShaderResourceBufferView("spriteData", pSpriteData->GetDefaultView(xiiGALBufferViewType::ShaderResource));
  {
    xiiResourceLock<xiiTexture2DResource> pTexture(pRenderData->m_hTexture, xiiResourceAcquireMode::AllowLoadingFallback);

    renderViewContext.m_pCommandList->ResolveAndSetShaderResourceTextureView("spriteTexture", pTexture->GetGALTexture()->GetDefaultView(xiiGALTextureViewType::ShaderResource));
    renderViewContext.m_pCommandList->ResolveAndSetSampler("spriteTexture", pTexture->GetGALSampler());
  }

  FillSpriteData(batch);

  if (!m_SpriteData.IsEmpty()) // Instance data might be empty if all render data was filtered.
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(renderViewContext.m_pCommandList.Borrow(), pSpriteData, 0U, m_SpriteData.GetByteArrayPtr()).AssertSuccess();

    const xiiUInt32 uiVertsPerPrimitive = xiiGALPrimitiveTopology::VerticesPerPrimitive(pGraphicsPipelineState->GetDescription().m_GraphicsPipeline.m_PrimitiveTopology);
    xiiUInt32       uiPrimitiveCount    = m_SpriteData.GetCount() * 2U * uiVertsPerPrimitive;
    xiiUInt32       uiInstanceCount     = renderViewContext.m_pCamera->IsStereoscopic() ? 2U : 1U;

    renderViewContext.m_pCommandList->BeginRenderPass({renderViewContext.m_CommandListData.m_pRenderPass, renderViewContext.m_CommandListData.m_pFramebuffer});
    renderViewContext.m_pCommandList->Draw({uiPrimitiveCount, uiInstanceCount});
    renderViewContext.m_pCommandList->EndRenderPass();
  }

#ifdef CORE_ENABLE
  pContext->BindShader(m_hShader);

  xiiGALCommandListUtilities::BindBuffer(pCommandList, "spriteData", pSpriteData);
  xiiGALCommandListUtilities::BindTexture2D(pCommandList, "SpriteTexture", pRenderData->m_hTexture);

  renderViewContext.SetShaderPermutationVariable("BLEND_MODE", xiiSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  renderViewContext.SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == xiiSpriteBlendMode::ShapeIcon ? xiiMakeHashedString("TRUE") : xiiMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(pContext->GetCommandList(), pSpriteData, 0, m_SpriteData.GetByteArrayPtr()).AssertSuccess();

    pContext->BindMeshBuffer(nullptr, nullptr, nullptr, xiiGALPrimitiveTopology::TriangleList, m_SpriteData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
#endif
}

xiiSharedPtr<xiiGALBuffer> xiiSpriteRenderer::CreateSpriteDataBuffer(xiiUInt32 uiBufferSize) const
{
  xiiGALBufferCreationDescription bufferDescription;
  bufferDescription.m_uiElementByteStride = sizeof(xiiPerSpriteData);
  bufferDescription.m_uiSize              = bufferDescription.m_uiElementByteStride * uiBufferSize;
  bufferDescription.m_BindFlags           = xiiGALBindFlags::ShaderResource;
  bufferDescription.m_Usage               = xiiGALResourceUsage::Dynamic;
  bufferDescription.m_CPUAccessFlags      = xiiGALCPUAccessFlag::Write;
  bufferDescription.m_Mode                = xiiGALBufferMode::Structured;

  return xiiGPUResourcePool::GetDefaultInstance()->GetBuffer(bufferDescription);
}

void xiiSpriteRenderer::DeleteSpriteDataBuffer(xiiSharedPtr<xiiGALBuffer> hBuffer) const
{
  xiiGPUResourcePool::GetDefaultInstance()->ReturnBuffer(hBuffer);
}

void xiiSpriteRenderer::FillSpriteData(const xiiRenderDataBatch& batch) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(batch.GetCount());

  for (auto it = batch.GetIterator<xiiSpriteRenderData>(); it.IsValid(); ++it)
  {
    const xiiSpriteRenderData* pRenderData = it;

    xiiPerSpriteData& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.Size               = pRenderData->m_fSize;
    spriteData.MaxScreenSize      = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio        = pRenderData->m_fAspectRatio;
    spriteData.ColorRG            = xiiGALShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_Colour.r, pRenderData->m_Colour.g));
    spriteData.ColorBA            = xiiGALShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_Colour.b, pRenderData->m_Colour.a));
    spriteData.TexCoordScale      = xiiGALShaderUtilities::Float2ToRG16F(pRenderData->m_vTextureCoordScale);
    spriteData.TexCoordOffset     = xiiGALShaderUtilities::Float2ToRG16F(pRenderData->m_vTextureCoordOffset);
    spriteData.GameObjectID       = pRenderData->m_uiUniqueID;
    spriteData.Reserved           = 0U;
  }
}

xiiSharedPtr<xiiGALGraphicsPipelineState> xiiSpriteRenderer::CreatePipelineState(const xiiRenderViewContext& renderViewContext) const
{
  xiiSharedPtr<xiiGALDevice>         pDevice            = xiiGALDevice::GetDefaultDevice();
  xiiShaderPermutationResourceHandle hShaderPermutation = xiiShaderPermutationUtilities::PreloadSinglePermutation(m_hShader, renderViewContext.GetPermutationVariables(), true);

  xiiGALGraphicsPipelineStateCreationDescription graphicsPipelineStateDescription;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRenderPass       = renderViewContext.m_CommandListData.m_pRenderPass;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_PrimitiveTopology = xiiGALPrimitiveTopology::TriangleList;
  graphicsPipelineStateDescription.m_GraphicsPipeline.m_uiSampleMask      = 0xFFFFFFFFU;

  {
    xiiResourceLock<xiiShaderPermutationResource> pShaderPermutation(hShaderPermutation, xiiResourceAcquireMode::BlockTillLoaded);

    graphicsPipelineStateDescription.m_pPipelineResourceSignature = pShaderPermutation->GetPipelineResourceSignature();
    graphicsPipelineStateDescription.m_pVertexShader              = pShaderPermutation->GetGALShader(xiiGALShaderType::Vertex);
    graphicsPipelineStateDescription.m_pGeometryShader            = pShaderPermutation->GetGALShader(xiiGALShaderType::Geometry);
    graphicsPipelineStateDescription.m_pPixelShader               = pShaderPermutation->GetGALShader(xiiGALShaderType::Pixel);

    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pBlendState        = pShaderPermutation->GetBlendState();
    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pRasterizerState   = pShaderPermutation->GetRasterizerState();
    graphicsPipelineStateDescription.m_GraphicsPipeline.m_pDepthStencilState = pShaderPermutation->GetDepthStencilState();
  }

  return pDevice->CreateGraphicsPipelineState(graphicsPipelineStateDescription);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_SpriteRenderer);
