#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Float16.h>
#include <Foundation/Types/ScopeExit.h>
#include <GraphicsCore/Components/SpriteComponent.h>
#include <GraphicsCore/Components/SpriteRenderer.h>
#include <GraphicsCore/GPUResourcePool/GPUResourcePool.h>
#include <GraphicsCore/Pipeline/RenderDataBatch.h>
#include <GraphicsCore/Shader/ShaderResource.h>
#include <GraphicsCore/Utils/CommandListUtilities.h>
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

void xiiSpriteRenderer::RenderBatch(const xiiRenderViewContext& renderViewContext, xiiSharedPtr<xiiGALCommandList> pCommandList, const xiiRenderPipelinePass* pPass, const xiiRenderDataBatch& batch) const
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  const xiiSpriteRenderData* pRenderData  = batch.GetFirstData<xiiSpriteRenderData>();
  const xiiUInt32            uiBufferSize = xiiMath::RoundUp(batch.GetCount(), 128u);
  xiiSharedPtr<xiiGALBuffer> pSpriteData  = CreateSpriteDataBuffer(uiBufferSize);
  XII_SCOPE_EXIT(DeleteSpriteDataBuffer(pSpriteData));

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

    auto& spriteData = m_SpriteData.ExpandAndGetRef();

    spriteData.WorldSpacePosition = pRenderData->m_GlobalTransform.m_vPosition;
    spriteData.Size               = pRenderData->m_fSize;
    spriteData.MaxScreenSize      = pRenderData->m_fMaxScreenSize;
    spriteData.AspectRatio        = pRenderData->m_fAspectRatio;
    spriteData.ColorRG            = xiiShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_color.r, pRenderData->m_color.g));
    spriteData.ColorBA            = xiiShaderUtilities::Float2ToRG16F(xiiVec2(pRenderData->m_color.b, pRenderData->m_color.a));
    spriteData.TexCoordScale      = xiiShaderUtilities::Float2ToRG16F(pRenderData->m_texCoordScale);
    spriteData.TexCoordOffset     = xiiShaderUtilities::Float2ToRG16F(pRenderData->m_texCoordOffset);
    spriteData.GameObjectID       = pRenderData->m_uiUniqueID;
    spriteData.Reserved           = 0;
  }
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Components_Implementation_SpriteRenderer);
