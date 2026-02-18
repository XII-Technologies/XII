#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <GraphicsCore/Pipeline/Extractor.h>
#include <GraphicsCore/Pipeline/RenderPipeline.h>
#include <GraphicsCore/Pipeline/RenderPipelinePass.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiCameraUsageHint, 1)
  XII_ENUM_CONSTANT(xiiCameraUsageHint::None),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::MainView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::EditorView),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::RenderTarget),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::Culling),
  XII_ENUM_CONSTANT(xiiCameraUsageHint::Thumbnail),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiView, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RenderTarget0", m_PinRenderTarget0),
    XII_MEMBER_PROPERTY("RenderTarget1", m_PinRenderTarget1),
    XII_MEMBER_PROPERTY("RenderTarget2", m_PinRenderTarget2),
    XII_MEMBER_PROPERTY("RenderTarget3", m_PinRenderTarget3),
    XII_MEMBER_PROPERTY("RenderTarget4", m_PinRenderTarget4),
    XII_MEMBER_PROPERTY("RenderTarget5", m_PinRenderTarget5),
    XII_MEMBER_PROPERTY("RenderTarget6", m_PinRenderTarget6),
    XII_MEMBER_PROPERTY("RenderTarget7", m_PinRenderTarget7),
    XII_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiView::xiiView()
{
  m_pExtractTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "", xiiTaskNesting::Never, xiiMakeDelegate(&xiiView::ExtractData, this));
}

xiiView::~xiiView() = default;

void xiiView::SetName(xiiStringView sName)
{
  m_sName.Assign(sName);

  xiiStringBuilder sb = sName;
  sb.Append(".ExtractData");
  m_pExtractTask->ConfigureTask(sb, xiiTaskNesting::Maybe);
}

void xiiView::SetWorld(xiiWorld* pWorld)
{
  if (m_pWorld != pWorld)
  {
    m_pWorld = pWorld;

    xiiRenderWorld::ResetRenderDataCache(*this);
  }
}

void xiiView::SetSwapChain(xiiGALSwapChain* pSwapChain)
{
  if (m_Data.m_pSwapChain != pSwapChain)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_pSwapChain                       = pSwapChain;
    m_Data.m_SwapChainRenderTargets.m_pRTs[0] = m_Data.m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget);
    m_Data.m_RenderTargets                    = xiiRenderTargets();
    if (m_pRenderPipeline)
    {
      xiiRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

void xiiView::SetRenderTargets(const xiiRenderTargets& renderTargets)
{
  if (m_Data.m_RenderTargets != renderTargets)
  {
    // Swap chain and render target setup are mutually exclusive.
    m_Data.m_pSwapChain             = xiiSharedPtr<xiiGALSwapChain>();
    m_Data.m_SwapChainRenderTargets = xiiRenderTargets();
    m_Data.m_RenderTargets          = renderTargets;
    if (m_pRenderPipeline)
    {
      xiiRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
    }
  }
}

const xiiRenderTargets& xiiView::GetActiveRenderTargets() const
{
  if (m_Data.m_pSwapChain)
  {
    xiiSharedPtr<xiiGALTextureView> pBackbufferRT = m_Data.m_pSwapChain->GetBackBufferTexture()->GetDefaultView(xiiGALTextureViewType::RenderTarget);

    if (pBackbufferRT != m_Data.m_SwapChainRenderTargets.m_pRTs[0])
    {
      m_Data.m_SwapChainRenderTargets.m_pRTs[0] = pBackbufferRT;
    }
    return m_Data.m_SwapChainRenderTargets;
  }
  return m_Data.m_RenderTargets;
}

void xiiView::SetRenderPipelineResource(xiiRenderPipelineResourceHandle hPipeline)
{
  if (hPipeline == m_hRenderPipeline)
    return;

  m_uiRenderPipelineResourceDescriptionCounter = 0;
  m_hRenderPipeline                            = hPipeline;

  if (m_pRenderPipeline == nullptr)
  {
    EnsureUpToDate();
  }
}

xiiRenderPipelineResourceHandle xiiView::GetRenderPipelineResource() const
{
  return m_hRenderPipeline;
}

void xiiView::SetCameraUsageHint(xiiEnum<xiiCameraUsageHint> val)
{
  m_Data.m_CameraUsageHint = val;
}

void xiiView::SetViewRenderMode(xiiEnum<xiiViewRenderMode> value)
{
  m_Data.m_ViewRenderMode = value;
}

void xiiView::SetViewport(const xiiRectFloat& viewport)
{
  m_Data.m_ViewPortRect = viewport;

  UpdateViewData(xiiRenderWorld::GetDataIndexForExtraction());
}

void xiiView::ForceUpdate()
{
  if (m_pRenderPipeline)
  {
    xiiRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());
  }
}

void xiiView::ExtractData()
{
  XII_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  xiiRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type           = xiiRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView          = this;
  extractionEvent.m_uiFrameCounter = xiiRenderWorld::GetFrameCounter();
  xiiRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

  m_pRenderPipeline->m_sName = m_sName;
  m_pRenderPipeline->ExtractData(*this);

  extractionEvent.m_Type = xiiRenderWorldExtractionEvent::Type::AfterViewExtraction;
  xiiRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);
}

void xiiView::ComputeCullingFrustum(xiiFrustum& out_frustum) const
{
  const xiiCamera* pCamera              = GetCullingCamera();
  const float      fViewportAspectRatio = m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height;

  xiiMat4 viewMatrix = pCamera->GetViewMatrix();

  xiiMat4 projectionMatrix;
  pCamera->GetProjectionMatrix(fViewportAspectRatio, projectionMatrix);

  out_frustum = xiiFrustum::MakeFromMVP(projectionMatrix * viewMatrix);
}

void xiiView::SetRenderPassProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  SetProperty(m_PassProperties, sPassName, sPropertyName, value);
}

void xiiView::SetExtractorProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  SetProperty(m_ExtractorProperties, sPassName, sPropertyName, value);
}

void xiiView::ResetRenderPassProperties()
{
  for (auto it : m_PassProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty     = true;
    }
  }
}

void xiiView::ResetExtractorProperties()
{
  for (auto it : m_ExtractorProperties)
  {
    auto& prop = it.Value();
    if (prop.m_bIsValid)
    {
      prop.m_CurrentValue = prop.m_DefaultValue;
      prop.m_bIsDirty     = true;
    }
  }
}

void xiiView::SetRenderPassReadBackProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  SetReadBackProperty(m_PassReadBackProperties, sPassName, sPropertyName, value);
}

xiiVariant xiiView::GetRenderPassReadBackProperty(xiiStringView sPassName, xiiStringView sPropertyName)
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  if (it.IsValid())
  {
    return it.Value().m_CurrentValue;
  }

  xiiLog::Warning("Unknown read-back property '{0}::{1}'", sPassName, sPropertyName);
  return xiiVariant();
}

bool xiiView::IsRenderPassReadBackPropertyExisting(xiiStringView sPassName, xiiStringView sPropertyName) const
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  auto it = m_PassReadBackProperties.Find(sKey);
  return it.IsValid();
}

void xiiView::UpdateViewData(xiiUInt32 uiDataIndex)
{
  if (m_pRenderPipeline != nullptr)
  {
    m_pRenderPipeline->UpdateViewData(*this, uiDataIndex);
  }
}

void xiiView::UpdateCachedMatrices() const
{
  const xiiCamera* pCamera = GetCamera();

  bool bUpdateVP = false;

  if (m_uiLastCameraOrientationModification != pCamera->GetOrientationModificationCounter())
  {
    bUpdateVP                             = true;
    m_uiLastCameraOrientationModification = pCamera->GetOrientationModificationCounter();

    m_Data.m_ViewMatrix[0] = pCamera->GetViewMatrix(xiiCameraEye::Left);
    m_Data.m_ViewMatrix[1] = pCamera->GetViewMatrix(xiiCameraEye::Right);

    // Some of our matrices contain very small values so that the matrix inversion will fall below the default epsilon.
    // We pass zero as epsilon here since all view and projection matrices are invertible.
    m_Data.m_InverseViewMatrix[0] = m_Data.m_ViewMatrix[0].GetInverse(0.0f);
    m_Data.m_InverseViewMatrix[1] = m_Data.m_ViewMatrix[1].GetInverse(0.0f);
  }

  const float fViewportAspectRatio = m_Data.m_ViewPortRect.HasNonZeroArea() ? m_Data.m_ViewPortRect.width / m_Data.m_ViewPortRect.height : 1.0f;
  if (m_uiLastCameraSettingsModification != pCamera->GetSettingsModificationCounter() || m_fLastViewportAspectRatio != fViewportAspectRatio)
  {
    bUpdateVP                          = true;
    m_uiLastCameraSettingsModification = pCamera->GetSettingsModificationCounter();
    m_fLastViewportAspectRatio         = fViewportAspectRatio;

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[0], xiiCameraEye::Left);
    m_Data.m_InverseProjectionMatrix[0] = m_Data.m_ProjectionMatrix[0].GetInverse(0.0f);

    pCamera->GetProjectionMatrix(m_fLastViewportAspectRatio, m_Data.m_ProjectionMatrix[1], xiiCameraEye::Right);
    m_Data.m_InverseProjectionMatrix[1] = m_Data.m_ProjectionMatrix[1].GetInverse(0.0f);
  }

  if (bUpdateVP)
  {
    for (int i = 0; i < 2; ++i)
    {
      m_Data.m_ViewProjectionMatrix[i]        = m_Data.m_ProjectionMatrix[i] * m_Data.m_ViewMatrix[i];
      m_Data.m_InverseViewProjectionMatrix[i] = m_Data.m_ViewProjectionMatrix[i].GetInverse(0.0f);
    }
  }
}

void xiiView::EnsureUpToDate()
{
  if (m_hRenderPipeline.IsValid())
  {
    xiiResourceLock<xiiRenderPipelineResource> pPipeline(m_hRenderPipeline, xiiResourceAcquireMode::BlockTillLoaded);

    xiiUInt32 uiCounter = pPipeline->GetCurrentResourceChangeCounter();

    if (m_uiRenderPipelineResourceDescriptionCounter != uiCounter)
    {
      m_uiRenderPipelineResourceDescriptionCounter = uiCounter;

      m_pRenderPipeline = pPipeline->CreateRenderPipeline();
      xiiRenderWorld::AddRenderPipelineToRebuild(m_pRenderPipeline, GetHandle());

      ResetAllPropertyStates(m_PassProperties);
      ResetAllPropertyStates(m_ExtractorProperties);
    }

    ApplyRenderPassProperties();
    ApplyExtractorProperties();
  }
}

void xiiView::SetProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  bool           bExisted      = false;
  PropertyValue& propertyValue = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    propertyValue.m_sObjectName   = sPassName;
    propertyValue.m_sPropertyName = sPropertyName;
    propertyValue.m_bIsValid      = true;
  }

  propertyValue.m_bIsDirty     = true;
  propertyValue.m_CurrentValue = value;
}

void xiiView::SetReadBackProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  bool           bExisted      = false;
  PropertyValue& propertyValue = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    propertyValue.m_sObjectName   = sPassName;
    propertyValue.m_sPropertyName = sPropertyName;
    propertyValue.m_bIsValid      = true;
  }

  propertyValue.m_bIsDirty     = false;
  propertyValue.m_CurrentValue = value;
}

void xiiView::ReadBackPassProperties()
{
  xiiHybridArray<xiiRenderPipelinePassBase*, 16U> passes;

  m_pRenderPipeline->GetPasses(passes);

  for (auto pPass : passes)
  {
    pPass->ReadBackProperties(this);
  }
}

void xiiView::ResetAllPropertyStates(xiiMap<xiiString, PropertyValue>& map)
{
  for (auto it = map.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_bIsDirty = true;
    it.Value().m_bIsValid = true;
  }
}

void xiiView::ApplyRenderPassProperties()
{
  for (auto it = m_PassProperties.GetIterator(); it.IsValid(); ++it)
  {
    PropertyValue& propertyValue = it.Value();

    if (!propertyValue.m_bIsValid || !propertyValue.m_bIsDirty)
      continue;

    propertyValue.m_bIsDirty = false;

    xiiReflectedClass* pObject = nullptr;
    const char*        szDot   = propertyValue.m_sObjectName.FindSubString(".");
    if (szDot != nullptr)
    {
      XII_REPORT_FAILURE("Setting renderer properties is not possible anymore");
    }
    else
    {
      pObject = m_pRenderPipeline->GetPassByName(propertyValue.m_sObjectName);
    }

    if (pObject == nullptr)
    {
      xiiLog::Error("The render pass '{0}' does not exist. Property '{1}' cannot be applied.", propertyValue.m_sObjectName, propertyValue.m_sPropertyName);

      propertyValue.m_bIsValid = false;
      continue;
    }

    ApplyProperty(pObject, propertyValue, "render pass");
  }
}

void xiiView::ApplyExtractorProperties()
{
  for (auto it = m_ExtractorProperties.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Value().m_bIsValid || !it.Value().m_bIsDirty)
      continue;

    it.Value().m_bIsDirty = false;

    xiiExtractor* pExtractor = m_pRenderPipeline->GetExtractorByName(it.Value().m_sObjectName);
    if (pExtractor == nullptr)
    {
      xiiLog::Error("The extractor '{0}' does not exist. Property '{1}' cannot be applied.", it.Value().m_sObjectName, it.Value().m_sPropertyName);

      it.Value().m_bIsValid = false;
      continue;
    }

    ApplyProperty(pExtractor, it.Value(), "extractor");
  }
}

void xiiView::ApplyProperty(xiiReflectedClass* pObject, PropertyValue& data, xiiStringView sTypeName)
{
  const xiiAbstractProperty* pAbstractProperty = pObject->GetDynamicRTTI()->FindPropertyByName(data.m_sPropertyName);
  if (pAbstractProperty == nullptr)
  {
    xiiLog::Error("The {0} '{1}' does not have a property called '{2}', it cannot be applied.", sTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  if (pAbstractProperty->GetCategory() != xiiPropertyCategory::Member)
  {
    xiiLog::Error("The {0} property '{1}::{2}' is not a member property, it cannot be applied.", sTypeName, data.m_sObjectName, data.m_sPropertyName);

    data.m_bIsValid = false;
    return;
  }

  auto pMemberProperty = static_cast<const xiiAbstractMemberProperty*>(pAbstractProperty);
  if (data.m_DefaultValue.IsValid() == false)
  {
    data.m_DefaultValue = xiiReflectionUtils::GetMemberPropertyValue(pMemberProperty, pObject);
  }

  xiiReflectionUtils::SetMemberPropertyValue(pMemberProperty, pObject, data.m_CurrentValue);
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Pipeline_Implementation_View);
