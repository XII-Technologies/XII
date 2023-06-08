#include <RendererCore/RendererCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Components/SpriteRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/RenderDataBatch.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

struct alignas(16) SpriteData
{
  xiiVec3   m_worldSpacePosition;
  float     m_size;
  float     m_maxScreenSize;
  float     m_aspectRatio;
  xiiUInt32 m_colorRG;
  xiiUInt32 m_colorBA;
  xiiUInt32 m_texCoordScale;
  xiiUInt32 m_texCoordOffset;
  xiiUInt32 m_gameObjectID;
  xiiUInt32 m_reserved;
};

XII_CHECK_AT_COMPILETIME(sizeof(SpriteData) == 48);

namespace
{
  enum
  {
    MAX_SPRITE_DATA_PER_BATCH = 1024
  };
}

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

  xiiGALBufferHandle hSpriteData = CreateSpriteDataBuffer();
  XII_SCOPE_EXIT(DeleteSpriteDataBuffer(hSpriteData));

  pContext->BindShader(m_hShader);
  pContext->BindBuffer("spriteData", pDevice->GetDefaultResourceView(hSpriteData));
  pContext->BindTexture2D("SpriteTexture", pRenderData->m_hTexture);

  pContext->SetShaderPermutationVariable("BLEND_MODE", xiiSpriteBlendMode::GetPermutationValue(pRenderData->m_BlendMode));

  xiiUInt32 uiStartIndex = 0;
  while (uiStartIndex < batch.GetCount())
  {
    const xiiUInt32 uiCount = xiiMath::Min(batch.GetCount() - uiStartIndex, (xiiUInt32)MAX_SPRITE_DATA_PER_BATCH);

    FillSpriteData(batch, uiStartIndex, uiCount);
    if (m_SpriteData.GetCount() > 0) // Instance data might be empty if all render data was filtered.
    {
      pContext->GetCommandEncoder()->UpdateBuffer(hSpriteData, 0, m_SpriteData.GetByteArrayPtr());

      pContext->BindMeshBuffer(xiiGALBufferHandle(), xiiGALBufferHandle(), nullptr, xiiGALPrimitiveTopology::Triangles, uiCount * 2);
      pContext->DrawMeshBuffer().IgnoreResult();
    }

    uiStartIndex += uiCount;
  }
}

xiiGALBufferHandle xiiSpriteRenderer::CreateSpriteDataBuffer() const
{
  xiiGALBufferCreationDescription desc;
  desc.m_uiStructSize                = sizeof(SpriteData);
  desc.m_uiTotalSize                 = desc.m_uiStructSize * MAX_SPRITE_DATA_PER_BATCH;
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

void xiiSpriteRenderer::FillSpriteData(const xiiRenderDataBatch& batch, xiiUInt32 uiStartIndex, xiiUInt32 uiCount) const
{
  m_SpriteData.Clear();
  m_SpriteData.Reserve(uiCount);

  for (auto it = batch.GetIterator<xiiSpriteRenderData>(uiStartIndex, uiCount); it.IsValid(); ++it)
  {
    const xiiSpriteRenderData* pRenderData = it;

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.m_worldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.m_size               = pRenderData->m_fSize;
    spriteData.m_maxScreenSize      = pRenderData->m_fMaxScreenSize;
    spriteData.m_aspectRatio        = pRenderData->m_fAspectRatio;
    spriteData.m_colorRG            = xiiShaderUtils::Float2ToRG16F(xiiVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.m_colorBA            = xiiShaderUtils::Float2ToRG16F(xiiVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.m_texCoordScale      = xiiShaderUtils::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.m_texCoordOffset     = xiiShaderUtils::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.m_gameObjectID       = pRenderData->m_uiUniqueID;
    spriteData.m_reserved           = 0;
  }
}



XII_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteRenderer);
