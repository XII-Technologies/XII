#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsCore/Textures/TextureUtils.h>
#include <GraphicsFoundation/Resources/Texture.h>
#include <GraphicsFoundation/Tools/TextureReadback.h>
#include <GraphicsFoundation/Utilities/GraphicsUtilities.h>
#include <GraphicsFoundation/Utilities/TextureUtilities.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPickingRenderPass, 1, xiiRTTIDefaultAllocator<xiiPickingRenderPass>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("PickSelected", m_bPickSelected),
    XII_MEMBER_PROPERTY("PickTransparent", m_bPickTransparent),
    XII_MEMBER_PROPERTY("PickingPosition", m_PickingPosition),
    XII_MEMBER_PROPERTY("MarqueePickPos0", m_MarqueePickPosition0),
    XII_MEMBER_PROPERTY("MarqueePickPos1", m_MarqueePickPosition1),
    XII_MEMBER_PROPERTY("MarqueeActionID", m_uiMarqueeActionID),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Rendering")
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

static xiiRenderData::Category s_LitOpaqueWithoutSelection        = xiiRenderData::RegisterDerivedCategory("LitOpaqueWithoutSelection", xiiDefaultRenderDataCategories::LitOpaqueStatic);
static xiiRenderData::Category s_LitMaskedWithoutSelection        = xiiRenderData::RegisterDerivedCategory("LitMaskedWithoutSelection", xiiDefaultRenderDataCategories::LitMaskedStatic);
static xiiRenderData::Category s_LitMaskedDynamicWithoutSelection = xiiRenderData::RegisterDerivedCategory("LitMaskedDynamicWithoutSelection", xiiDefaultRenderDataCategories::LitMaskedDynamic);

static xiiRenderData::Category s_LitTransparentWithoutSelection    = xiiRenderData::RegisterDerivedCategory("LitTransparentWithoutSelection", xiiDefaultRenderDataCategories::LitTransparent);
static xiiRenderData::Category s_SimpleTransparentWithoutSelection = xiiRenderData::RegisterDerivedCategory("SimpleTransparentWithoutSelection", xiiDefaultRenderDataCategories::SimpleTransparent);

xiiPickingRenderPass::xiiPickingRenderPass() :
  xiiGraphicsPipelinePass("EditorPickingRenderPass", xiiRenderPipelinePassCapabilityFlags::None)
{
  m_pGridRenderDataType = xiiRTTI::FindTypeByName("xiiGridRenderData");
  XII_ASSERT_DEV(m_pGridRenderDataType != nullptr, "xiiGridRenderData type not found. Type renamed?");
}

xiiPickingRenderPass::~xiiPickingRenderPass() = default;

xiiSharedPtr<xiiGALTexture> xiiPickingRenderPass::GetPickingIdRT() const
{
  return m_pPickingIdRT;
}

xiiSharedPtr<xiiGALTexture> xiiPickingRenderPass::GetPickingDepthRT() const
{
  return m_pPickingDepthRT;
}

xiiResult xiiPickingRenderPass::GetResourceDescriptions(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassResource* const> pInputs, xiiArrayPtr<xiiRenderPipelinePassResource> pOutputs)
{
  return XII_SUCCESS;
}

xiiResult xiiPickingRenderPass::InitializeRenderPipelinePass(const xiiView& view, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pInputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> pOutputs)
{
  m_TargetRect = view.GetViewport();

  DestroyTarget();
  CreateTarget();

  if (m_uiProcessorId == xiiInvalidIndex)
  {
    m_uiProcessorId = GetPipeline()->AddRenderDataProcessor(xiiMakeDelegate(&xiiPickingRenderPass::ProcessPickingRenderData, this));
  }
  return XII_SUCCESS;
}

void xiiPickingRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  // Render result
  const xiiRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  m_uiWindowWidth                  = (xiiUInt32)viewPortRect.width;
  m_uiWindowHeight                 = (xiiUInt32)viewPortRect.height;

  XII_ASSERT_DEV(m_uiWindowWidth == m_pPickingDepthRT->GetDescription().GetWidth(), "");
  XII_ASSERT_DEV(m_uiWindowHeight == m_pPickingDepthRT->GetDescription().GetHeight(), "");

  xiiRenderingSetup renderingSetup;
  renderingSetup.AddColorAttachment({m_pPickingIdRT->GetDefaultView(xiiGALTextureViewType::RenderTarget), xiiColor::Black, xiiGALAttachmentLoadOperation::Clear})
    .SetDepthStencilAttachment({m_pPickingDepthRT->GetDefaultView(xiiGALTextureViewType::DepthStencil), 1.0f, 0U, xiiGALAttachmentLoadOperation::Clear, xiiGALAttachmentStoreOperation::Store, xiiGALAttachmentLoadOperation::Clear, xiiGALAttachmentStoreOperation::Store})
    .Build();

  auto pRenderContext = xiiRenderContext::BeginRenderingScope(renderViewContext, std::move(renderingSetup), GetName());

  xiiViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
  if (viewRenderMode == xiiViewRenderMode::WireframeColor || viewRenderMode == xiiViewRenderMode::WireframeMonochrome)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");
  }

  // Setup clustered data.
  xiiClusteredDataGPU* pClusteredData = GetPipeline()->GetFrameDataProvider<xiiClusteredDataProvider>()->GetData(renderViewContext);
  pClusteredData->BindResources(renderViewContext.m_pRenderContext);

  RenderDataWithCategory(renderViewContext, s_LitOpaqueWithoutSelection);
  RenderDataWithCategory(renderViewContext, s_LitMaskedWithoutSelection);

  if (m_bPickTransparent)
  {
    RenderDataWithCategory(renderViewContext, s_LitTransparentWithoutSelection);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitForeground);

    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitForeground);
  }

  if (m_bPickSelected)
  {
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::Selection);
  }

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleOpaque);

  if (m_bPickTransparent)
  {
    RenderDataWithCategory(renderViewContext, s_SimpleTransparentWithoutSelection);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD");

  xiiSharedPtr<xiiGALCommandList> pCommandList = renderViewContext.m_pRenderContext->GetCommandList();

  if (m_PendingReadback.m_bReadbackInProgress)
  {
    // Wait for results.
    m_PendingReadback.m_PickingReadback->WaitForNextCompleted();
    m_PendingReadback.m_PickingDepthReadback->WaitForNextCompleted();

    m_PickingResultsID.Clear();
    m_PickingResultsDepth.Clear();
    m_mPickingInverseViewProjectionMatrix = xiiMat4::MakeZero();

    xiiGALTextureMipLevelData      mipLevelData;
    xiiGALMappedTextureSubresource mappedSubResource;

    // If the resolution has changed, discard the readback result.
    if (m_uiWindowHeight == m_PendingReadback.m_uiWindowHeight && m_uiWindowWidth == m_PendingReadback.m_uiWindowWidth)
    {
      {
        m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

        auto readback = m_PendingReadback.m_PickingReadback->GetCompleted();
        pCommandList->MapTextureSubresource(readback.m_pStagingTexture, mipLevelData, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).AssertSuccess("Failed to map readback texture.");

        xiiGALTextureUtilities::CopySubresourceToMemory(readback.m_pStagingTexture->GetDescription(), mappedSubResource, mipLevelData, m_PickingResultsID.GetByteArrayPtr(), m_uiWindowWidth * sizeof(xiiUInt32));

        pCommandList->UnmapTextureSubresource(readback.m_pStagingTexture, mipLevelData).IgnoreResult();

        m_PendingReadback.m_PickingReadback->RecycleStagingTexture(std::move(readback.m_pStagingTexture));
      }
      {
        m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

        auto readback = m_PendingReadback.m_PickingDepthReadback->GetCompleted();
        pCommandList->MapTextureSubresource(readback.m_pStagingTexture, mipLevelData, xiiGALMapType::Read, xiiGALMapFlags::DoNotWait, nullptr, mappedSubResource).AssertSuccess("Failed to map readback texture.");

        xiiGALTextureUtilities::CopySubresourceToMemory(readback.m_pStagingTexture->GetDescription(), mappedSubResource, mipLevelData, m_PickingResultsDepth.GetByteArrayPtr(), m_uiWindowWidth * sizeof(float));

        pCommandList->UnmapTextureSubresource(readback.m_pStagingTexture, mipLevelData).IgnoreResult();

        m_PendingReadback.m_PickingDepthReadback->RecycleStagingTexture(std::move(readback.m_pStagingTexture));
      }
      m_mPickingInverseViewProjectionMatrix = m_PendingReadback.m_mPickingInverseViewProjectionMatrix;
    }
  }

  // Begin transferring the picking information from the GPU to the CPU.
  if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
  {
    {
      xiiGALTextureReadback::ReadbackRequest request = {};
      request.m_uiTextureID                          = m_uiMarqueeActionID;
      request.m_uiMipLevel                           = 0U;
      request.m_uiArraySlice                         = 0U;

      request.m_pTexture = GetPickingIdRT();
      m_PendingReadback.m_PickingReadback->Enqueue(pCommandList, request);

      request.m_pTexture = GetPickingDepthRT();
      m_PendingReadback.m_PickingDepthReadback->Enqueue(pCommandList, request);
    }

    xiiMat4 mProjection;
    renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProjection);
    xiiMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

    if (mProjection.IsNaN())
      return;

    xiiMat4 mInverse = mProjection * mView;
    if (mInverse.Invert(0).Failed())
    {
      xiiLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be incorrect.");
      return;
    }

    m_PendingReadback.m_mPickingInverseViewProjectionMatrix = mInverse;
    m_PendingReadback.m_uiWindowWidth                       = m_uiWindowWidth;
    m_PendingReadback.m_uiWindowHeight                      = m_uiWindowHeight;
    m_PendingReadback.m_bReadbackInProgress                 = true;
  }
}

void xiiPickingRenderPass::ReadBackProperties(xiiView* pView)
{
  ReadBackPropertiesSinglePick(pView);
  ReadBackPropertiesMarqueePick(pView);
}

void xiiPickingRenderPass::CreateTarget()
{
  xiiSharedPtr<xiiGALDevice> pDevice = xiiGALDevice::GetDefaultDevice();

  // Create render target for picking
  xiiGALTextureCreationDescription tcd;
  tcd.m_Type        = xiiGALResourceDimension::Texture2D;
  tcd.m_Format      = xiiGALResourceFormat::RGBA8UNormalized;
  tcd.m_Size.width  = (xiiUInt32)m_TargetRect.width;
  tcd.m_Size.height = (xiiUInt32)m_TargetRect.height;
  tcd.m_BindFlags   = xiiGALBindFlags::RenderTarget;

  m_pPickingIdRT                      = pDevice->CreateTexture(tcd);
  m_PendingReadback.m_PickingReadback = XII_DEFAULT_NEW(xiiGALTextureReadback, pDevice);

  tcd.m_Format         = xiiGALResourceFormat::D32Float;
  tcd.m_BindFlags      = xiiGALBindFlags::DepthStencil;
  tcd.m_CPUAccessFlags = xiiGALCPUAccessFlag::None;
  tcd.m_Usage          = xiiGALResourceUsage::Mutable;

  m_pPickingDepthRT                        = pDevice->CreateTexture(tcd);
  m_PendingReadback.m_PickingDepthReadback = XII_DEFAULT_NEW(xiiGALTextureReadback, pDevice);
}

void xiiPickingRenderPass::DestroyTarget()
{
  m_pPickingIdRT.Clear();
  m_pPickingDepthRT.Clear();
}

void xiiPickingRenderPass::ReadBackPropertiesSinglePick(xiiView* pView)
{
  const xiiUInt32 x       = (xiiUInt32)m_PickingPosition.x;
  const xiiUInt32 y       = (xiiUInt32)m_PickingPosition.y;
  const xiiUInt32 uiIndex = (y * m_uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsDepth.GetCount() || x >= m_uiWindowWidth || y >= m_uiWindowHeight)
  {
    // xiiLog::Error("Picking position {0}, {1} is outside the available picking area of {2} * {3}", x, y, m_uiWindowWidth, m_uiWindowHeight);
    return;
  }

  m_PickingPosition.Set(-1);

  xiiVec3 vNormal(0);
  xiiVec3 vPickingRayStartPosition(0);
  xiiVec3 vPickedPosition(0);
  {
    const float fDepth = m_PickingResultsDepth[uiIndex];
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)y, fDepth), vPickedPosition).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)y, 0), vPickingRayStartPosition).IgnoreResult();

    float   fOtherDepths[4] = {fDepth, fDepth, fDepth, fDepth};
    xiiVec3 vOtherPos[4];
    xiiVec3 vNormals[4];

    if ((xiiUInt32)x + 1 < m_uiWindowWidth)
      fOtherDepths[0] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x + 1];
    if (x > 0)
      fOtherDepths[1] = m_PickingResultsDepth[(y * m_uiWindowWidth) + x - 1];
    if ((xiiUInt32)y + 1 < m_uiWindowHeight)
      fOtherDepths[2] = m_PickingResultsDepth[((y + 1) * m_uiWindowWidth) + x];
    if (y > 0)
      fOtherDepths[3] = m_PickingResultsDepth[((y - 1) * m_uiWindowWidth) + x];

    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)(x + 1), (float)y, fOtherDepths[0]), vOtherPos[0]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)(x - 1), (float)y, fOtherDepths[1]), vOtherPos[1]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(y + 1), fOtherDepths[2]), vOtherPos[2]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(y - 1), fOtherDepths[3]), vOtherPos[3]).IgnoreResult();

    vNormals[0].CalculateNormal(vPickedPosition, vOtherPos[0], vOtherPos[2]).IgnoreResult();
    vNormals[1].CalculateNormal(vPickedPosition, vOtherPos[2], vOtherPos[1]).IgnoreResult();
    vNormals[2].CalculateNormal(vPickedPosition, vOtherPos[1], vOtherPos[3]).IgnoreResult();
    vNormals[3].CalculateNormal(vPickedPosition, vOtherPos[3], vOtherPos[0]).IgnoreResult();

    vNormal = (vNormals[0] + vNormals[1] + vNormals[2] + vNormals[3]).GetNormalized();
  }

  xiiUInt32 uiPickID = m_PickingResultsID[uiIndex];
  if (uiPickID == 0)
  {
    for (xiiInt32 radius = 1; radius < 10; ++radius)
    {
      xiiInt32 left   = xiiMath::Max<xiiInt32>(x - radius, 0);
      xiiInt32 right  = xiiMath::Min<xiiInt32>(x + radius, m_uiWindowWidth - 1);
      xiiInt32 top    = xiiMath::Max<xiiInt32>(y - radius, 0);
      xiiInt32 bottom = xiiMath::Min<xiiInt32>(y + radius, m_uiWindowHeight - 1);

      for (xiiInt32 xt = left; xt <= right; ++xt)
      {
        const xiiUInt32 idxt = (top * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto Done;
      }

      for (xiiInt32 xt = left; xt <= right; ++xt)
      {
        const xiiUInt32 idxt = (bottom * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto Done;
      }
    }

  Done:;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "PickedMatrix", m_mPickingInverseViewProjectionMatrix);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedID", uiPickID);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedDepth", m_PickingResultsDepth[uiIndex]);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedNormal", vNormal);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedRayStartPosition", vPickingRayStartPosition);
  pView->SetRenderPassReadBackProperty(GetName(), "PickedPosition", vPickedPosition);
}

void xiiPickingRenderPass::ReadBackPropertiesMarqueePick(xiiView* pView)
{
  const xiiUInt32 x0       = (xiiUInt32)m_MarqueePickPosition0.x;
  const xiiUInt32 y0       = (xiiUInt32)m_MarqueePickPosition0.y;
  const xiiUInt32 x1       = (xiiUInt32)m_MarqueePickPosition1.x;
  const xiiUInt32 y1       = (xiiUInt32)m_MarqueePickPosition1.y;
  const xiiUInt32 uiIndex1 = (y0 * m_uiWindowWidth) + x0;
  const xiiUInt32 uiIndex2 = (y0 * m_uiWindowWidth) + x0;

  if ((uiIndex1 >= m_PickingResultsDepth.GetCount() || x0 >= m_uiWindowWidth || y0 >= m_uiWindowHeight) || (uiIndex2 >= m_PickingResultsDepth.GetCount() || x1 >= m_uiWindowWidth || y1 >= m_uiWindowHeight))
    return;

  m_MarqueePickPosition0.Set(-1);
  m_MarqueePickPosition1.Set(-1);
  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeActionID", m_uiMarqueeActionID);

  xiiHybridArray<xiiUInt32, 32> IDs;
  xiiVariantArray               resArray;

  const xiiUInt32 lowX  = xiiMath::Min(x0, x1);
  const xiiUInt32 highX = xiiMath::Max(x0, x1);
  const xiiUInt32 lowY  = xiiMath::Min(y0, y1);
  const xiiUInt32 highY = xiiMath::Max(y0, y1);

  xiiUInt32 offset = 0;

  for (xiiUInt32 y = lowY; y < highY; y += 1)
  {
    for (xiiUInt32 x = lowX + offset; x < highX; x += 2)
    {
      const xiiUInt32 uiIndex = (y * m_uiWindowWidth) + x;

      const xiiUInt32 id = m_PickingResultsID[uiIndex];

      // prevent duplicates
      if (IDs.Contains(id))
        continue;

      IDs.PushBack(id);
      resArray.PushBack(id);
    }

    // only evaluate every second pixel, in a checker board pattern
    offset = (offset + 1) % 2;
  }

  pView->SetRenderPassReadBackProperty(GetName(), "MarqueeResult", resArray);
}

void xiiPickingRenderPass::ProcessPickingRenderData(xiiExtractedRenderData& extractedRenderData)
{
  // Copy selection to set for faster checks.
  m_SelectionSet.Clear();
  {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(xiiDefaultRenderDataCategories::Selection);
    for (auto& sortableRenderData : renderDataList)
    {
      m_SelectionSet.Insert(sortableRenderData.m_pRenderData->m_hOwner);
    }
  }

  auto Filter = [&](xiiRenderData::Category originalCategory, xiiRenderData::Category filteredCategory) {
    auto renderDataList = extractedRenderData.GetRawRenderDataWithCategory(originalCategory);
    for (auto& sortableRenderData : renderDataList)
    {
      auto pRenderData = sortableRenderData.m_pRenderData;
      if (m_SelectionSet.Contains(pRenderData->m_hOwner) || pRenderData->IsInstanceOf(m_pGridRenderDataType))
        continue;

      extractedRenderData.AddRenderData(pRenderData, filteredCategory);
    }
  };

  Filter(xiiDefaultRenderDataCategories::LitOpaqueStatic, s_LitOpaqueWithoutSelection);
  Filter(xiiDefaultRenderDataCategories::LitOpaqueDynamic, s_LitOpaqueWithoutSelection);

  Filter(xiiDefaultRenderDataCategories::LitMaskedStatic, s_LitMaskedWithoutSelection);
  Filter(xiiDefaultRenderDataCategories::LitMaskedDynamic, s_LitOpaqueWithoutSelection);

  if (m_bPickTransparent)
  {
    Filter(xiiDefaultRenderDataCategories::LitTransparent, s_LitTransparentWithoutSelection);
    Filter(xiiDefaultRenderDataCategories::SimpleTransparent, s_SimpleTransparentWithoutSelection);
  }
}
