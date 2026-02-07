#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Pipeline/Renderer/SpriteRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/GPUResourcePool/PipelineStateCache.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Pipeline/ViewData.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
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
  ref_categories.PushBack(xiiDefaultRenderDataCategories::SimpleTransparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Transparent);
  ref_categories.PushBack(xiiDefaultRenderDataCategories::Foreground);
}

void xiiSpriteRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiGraphicsPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiSpriteRenderData* pRenderData  = batch.GetFirstData<xiiSpriteRenderData>();
  const xiiUInt32            uiBufferSize = xiiMath::RoundUp(batch.GetCount(), 128u);
  xiiSharedPtr<xiiGALBuffer> pSpriteData  = CreateSpriteDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteSpriteDataBuffer(pSpriteData));

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLEND_MODE", xiiSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == xiiSpriteBlendMode::ShapeIcon ? xiiMakeHashedString("TRUE") : xiiMakeHashedString("FALSE"));
  renderViewContext.m_pRenderContext->BindShader(m_hShader);
  renderViewContext.m_pRenderContext->BindBuffer("spriteData", pSpriteData);
  renderViewContext.m_pRenderContext->BindTexture2D("spriteTexture", pRenderData->m_hTexture);

  FillSpriteData(batch);

  if (!m_SpriteData.IsEmpty()) // Instance data might be empty if all render data was filtered.
  {
    xiiGALDeviceUtilities::MapAndUpdateBuffer(renderViewContext.m_pRenderContext->GetCommandList(), pSpriteData, 0U, m_SpriteData.GetByteArrayPtr()).AssertSuccess();

    renderViewContext.m_pRenderContext->BindNullMeshBuffer(xiiGALPrimitiveTopology::TriangleList, m_SpriteData.GetCount() * 2U);
    renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
  }
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

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_Renderer_SpriteRenderer);
