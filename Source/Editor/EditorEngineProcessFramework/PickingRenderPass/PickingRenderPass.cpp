#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/PickingRenderPass/PickingRenderPass.h>
#include <GraphicsCore/Lights/ClusteredDataProvider.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderContext/RenderContext.h>
#include <GraphicsFoundation/Resources/Texture.h>

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
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPickingRenderPass::xiiPickingRenderPass() :
  xiiRenderPipelinePass("EditorPickingRenderPass")
{
}

xiiPickingRenderPass::~xiiPickingRenderPass()
{
  DestroyTarget();
}

xiiGALTextureHandle xiiPickingRenderPass::GetPickingIdRT() const
{
  return m_hPickingIdRT;
}

xiiGALTextureHandle xiiPickingRenderPass::GetPickingDepthRT() const
{
  return m_hPickingDepthRT;
}

bool xiiPickingRenderPass::GetRenderTargetDescriptions(const xiiView& view, const xiiArrayPtr<xiiGALTextureCreationDescription* const> inputs, xiiArrayPtr<xiiGALTextureCreationDescription> outputs)
{
  m_TargetRect = view.GetViewport();

  return true;
}

void xiiPickingRenderPass::InitRenderPipelinePass(const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  DestroyTarget();
  CreateTarget();
}

void xiiPickingRenderPass::Execute(const xiiRenderViewContext& renderViewContext, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> inputs, const xiiArrayPtr<xiiRenderPipelinePassConnection* const> outputs)
{
  const xiiRectFloat& viewPortRect = renderViewContext.m_pViewData->m_ViewPortRect;
  m_uiWindowWidth                  = (xiiUInt32)viewPortRect.width;
  m_uiWindowHeight                 = (xiiUInt32)viewPortRect.height;

  const xiiGALTexture* pDepthTexture = xiiGALDevice::GetDefaultDevice()->GetTexture(m_hPickingDepthRT);
  XII_ASSERT_DEV(m_uiWindowWidth == pDepthTexture->GetDescription().m_uiWidth, "");
  XII_ASSERT_DEV(m_uiWindowHeight == pDepthTexture->GetDescription().m_uiHeight, "");

  xiiGALRenderingSetup renderingSetup;
  renderingSetup.m_RenderTargetSetup       = m_RenderTargetSetup;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth             = true;
  renderingSetup.m_bClearStencil           = true;

  auto pCommandEncoder = xiiRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());

  xiiViewRenderMode::Enum viewRenderMode = renderViewContext.m_pViewData->m_ViewRenderMode;
  if (viewRenderMode == xiiViewRenderMode::WireframeColor || viewRenderMode == xiiViewRenderMode::WireframeMonochrome)
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING_WIREFRAME");
  else
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_PICKING");

  // Setup clustered data
  auto pClusteredData = GetPipeline()->GetFrameDataProvider<xiiClusteredDataProvider>()->GetData(renderViewContext);
  pClusteredData->BindResources(renderViewContext.m_pRenderContext);

  // copy selection to set for faster checks
  m_SelectionSet.Clear();

  auto            batchList    = GetPipeline()->GetRenderDataBatchesWithCategory(xiiDefaultRenderDataCategories::Selection);
  const xiiUInt32 uiBatchCount = batchList.GetBatchCount();
  for (xiiUInt32 i = 0; i < uiBatchCount; ++i)
  {
    const xiiRenderDataBatch& batch = batchList.GetBatch(i);
    for (auto it = batch.GetIterator<xiiRenderData>(); it.IsValid(); ++it)
    {
      m_SelectionSet.Insert(it->m_hOwner);
    }
  }

  // filter out all selected objects
  xiiRenderDataBatch::Filter filter([&](const xiiRenderData* pRenderData) { return m_SelectionSet.Contains(pRenderData->m_hOwner); });

  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitOpaque, filter);
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitMasked, filter);

  if (m_bPickTransparent)
  {
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::LitTransparent, filter);

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
    RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleTransparent, filter);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "TRUE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("PREPARE_DEPTH", "FALSE");
  RenderDataWithCategory(renderViewContext, xiiDefaultRenderDataCategories::SimpleForeground);

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", "RENDER_PASS_FORWARD");

  // download the picking information from the GPU
  if (m_uiWindowWidth != 0 && m_uiWindowHeight != 0)
  {
    pCommandEncoder->ReadbackTexture(GetPickingDepthRT());
    pCommandEncoder->ReadbackTexture(GetPickingIdRT());

    xiiMat4 mProj;
    renderViewContext.m_pCamera->GetProjectionMatrix((float)m_uiWindowWidth / m_uiWindowHeight, mProj);
    xiiMat4 mView = renderViewContext.m_pCamera->GetViewMatrix();

    if (mProj.IsNaN())
      return;

    xiiMat4 inv = mProj * mView;
    if (inv.Invert(0).Failed())
    {
      xiiLog::Warning("Inversion of View-Projection-Matrix failed. Picking results will be wrong.");
      return;
    }

    m_mPickingInverseViewProjectionMatrix = inv;

    xiiGALSystemMemoryDescription MemDesc;
    MemDesc.m_uiRowPitch   = 4 * m_uiWindowWidth;
    MemDesc.m_uiSlicePitch = 4 * m_uiWindowWidth * m_uiWindowHeight;
    xiiArrayPtr<xiiGALSystemMemoryDescription> SysMemDescs(&MemDesc, 1);

    xiiGALTextureSubresource              sourceSubResource;
    xiiArrayPtr<xiiGALTextureSubresource> sourceSubResources(&sourceSubResource, 1);

    {
      m_PickingResultsDepth.Clear();
      m_PickingResultsDepth.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

      MemDesc.m_pData = m_PickingResultsDepth.GetData();
      pCommandEncoder->CopyTextureReadbackResult(GetPickingDepthRT(), sourceSubResources, SysMemDescs);
    }

    {
      m_PickingResultsID.Clear();
      m_PickingResultsID.SetCountUninitialized(m_uiWindowWidth * m_uiWindowHeight);

      MemDesc.m_pData = m_PickingResultsID.GetData();
      pCommandEncoder->CopyTextureReadbackResult(GetPickingIdRT(), sourceSubResources, SysMemDescs);
    }
  }
}

void xiiPickingRenderPass::ReadBackProperties(xiiView* pView)
{
  ReadBackPropertiesSinglePick(pView);
  ReadBackPropertiesMarqueePick(pView);
}

void xiiPickingRenderPass::CreateTarget()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  // Create render target for picking
  xiiGALTextureCreationDescription tcd;
  tcd.m_bAllowDynamicMipGeneration = false;
  tcd.m_bAllowShaderResourceView   = false;
  tcd.m_bAllowUAV                  = false;
  tcd.m_bCreateRenderTarget        = true;
  tcd.m_Format                     = xiiGALResourceFormat::RGBAUByteNormalized;
  tcd.m_ResourceAccess.m_bReadBack = true;
  tcd.m_Type                       = xiiGALTextureType::Texture2D;
  tcd.m_uiWidth                    = (xiiUInt32)m_TargetRect.width;
  tcd.m_uiHeight                   = (xiiUInt32)m_TargetRect.height;

  m_hPickingIdRT = pDevice->CreateTexture(tcd);

  tcd.m_Format                     = xiiGALResourceFormat::DFloat;
  tcd.m_ResourceAccess.m_bReadBack = true;

  m_hPickingDepthRT = pDevice->CreateTexture(tcd);

  m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(m_hPickingIdRT)).SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(m_hPickingDepthRT));
}

void xiiPickingRenderPass::DestroyTarget()
{
  xiiGALDevice* pDevice = xiiGALDevice::GetDefaultDevice();

  m_RenderTargetSetup.DestroyAllAttachedViews();
  if (!m_hPickingIdRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingIdRT);
    m_hPickingIdRT.Invalidate();
  }

  if (!m_hPickingDepthRT.IsInvalidated())
  {
    pDevice->DestroyTexture(m_hPickingDepthRT);
    m_hPickingDepthRT.Invalidate();
  }
}

void xiiPickingRenderPass::ReadBackPropertiesSinglePick(xiiView* pView)
{
  const xiiUInt32 x       = (xiiUInt32)m_PickingPosition.x;
  const xiiUInt32 y       = (xiiUInt32)m_PickingPosition.y;
  const xiiUInt32 uiIndex = (y * m_uiWindowWidth) + x;

  if (uiIndex >= m_PickingResultsDepth.GetCount() || x >= m_uiWindowWidth || y >= m_uiWindowHeight)
  {
    // xiiLog::Error("Picking position {0}, {1} is outside the available picking area of {2} * {3}", x, y, m_uiWindowWidth,
    // m_uiWindowHeight);
    return;
  }

  m_PickingPosition.Set(-1);

  xiiVec3 vNormal(0);
  xiiVec3 vPickingRayStartPosition(0);
  xiiVec3 vPickedPosition(0);
  {
    const float fDepth = m_PickingResultsDepth[uiIndex];
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(m_uiWindowHeight - y), fDepth), vPickedPosition).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(m_uiWindowHeight - y), 0), vPickingRayStartPosition).IgnoreResult();

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

    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)(x + 1), (float)(m_uiWindowHeight - y), fOtherDepths[0]), vOtherPos[0]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)(x - 1), (float)(m_uiWindowHeight - y), fOtherDepths[1]), vOtherPos[1]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(m_uiWindowHeight - (y + 1)), fOtherDepths[2]), vOtherPos[2]).IgnoreResult();
    xiiGraphicsUtils::ConvertScreenPosToWorldPos(m_mPickingInverseViewProjectionMatrix, 0, 0, m_uiWindowWidth, m_uiWindowHeight, xiiVec3((float)x, (float)(m_uiWindowHeight - (y - 1)), fOtherDepths[3]), vOtherPos[3]).IgnoreResult();

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
          goto done;
      }

      for (xiiInt32 xt = left; xt <= right; ++xt)
      {
        const xiiUInt32 idxt = (bottom * m_uiWindowWidth) + xt;

        uiPickID = m_PickingResultsID[idxt];

        if (uiPickID != 0)
          goto done;
      }
    }

  done:;
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
  {
    return;
  }

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
