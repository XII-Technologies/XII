#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Components/SpriteRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsFoundation/Shader/ShaderUtils.h>

#include <Shaders/Materials/SpriteData.h>
XII_CHECK_AT_COMPILETIME(sizeof(xiiPerSpriteData) == 48);

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

void xiiSpriteRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiGALDevice*     pDevice  = xiiGALDevice::GetDefaultDevice();
  xiiRenderContext* pContext = renderViewContext.m_pRenderContext;

  const xiiSpriteRenderData* pRenderData = batch.GetFirstData<xiiSpriteRenderData>();

  const xiiUInt32    uiBufferSize = xiiMath::RoundUp(batch.GetCount(), 128u);
  xiiGALBufferHandle hSpriteData  = CreateSpriteDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", xiiSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));
  pContext->SetShaderPermutationVariable("SHAPE_ICON", pRenderData->m_BlendMode == xiiSpriteBlendMode::ShapeIcon ? xiiMakeHashedString("TRUE") : xiiMakeHashedString("FALSE"));

  FillSpriteData(batch);

  if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
  {
    pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr());

    pContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::TriangleList, m_SpriteData.GetCount() * 2);
    pContext->DrawMeshBuffer().IgnoreResult();
  }
}

xiiGALBufferHandle xiiSpriteRenderer::CreateSpriteDataBuffer(xiiUInt32 uiBufferSize) const
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = sizeof(xiiPerSpriteData);
  desc.m_uiTotalSize                 = desc.m_uiStructSize * uiBufferSize;
  desc.m_BufferType                  = xiiGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer      = true;
  desc.m_bAllowShaderResourceView    = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  return xiiGPUResourcePool::GetDefaultInstance()->GetBuffer(desc);
}

void xiiSpriteRenderer::DeleteSpriteDataBuffer(xiiGALBufferHandle hBuffer) const
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

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.Size               = pRenderData->m_fSize;
    spriteData.MaxScreenSize      = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio        = pRenderData->m_fAspectRatio;
    spriteData.ColorRG            = xiiShaderUtils::Float2ToRG16F(xiiVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.ColorBA            = xiiShaderUtils::Float2ToRG16F(xiiVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.TexCoordScale      = xiiShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.TexCoordOffset     = xiiShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.GameObjectID       = pRenderData->m_uiUniqueID;
    spriteData.Reserved           = 0;
  }
}



XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_SpriteRenderer);
