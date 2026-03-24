#include <GraphicsCore/GraphicsCorePCH.h>

#include <Foundation/Math/Frustum.h>
#include <Foundation/Reflection/ReflectionUtils.h>
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

  EnsureUpToDate();
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
}

void xiiView::ExtractData()
{
  XII_ASSERT_DEV(IsValid(), "Cannot extract data from an invalid view");

  xiiRenderWorldExtractionEvent extractionEvent;
  extractionEvent.m_Type           = xiiRenderWorldExtractionEvent::Type::BeforeViewExtraction;
  extractionEvent.m_pView          = this;
  extractionEvent.m_uiFrameCounter = xiiRenderWorld::GetFrameCounter();
  xiiRenderWorld::s_ExtractionEvent.Broadcast(extractionEvent);

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

void xiiView::SetShaderPermutationVariable(xiiStringView sName, xiiStringView sValue)
{
  xiiHashedString sNameHash;
  sNameHash.Assign(sName);

  for (auto& permutationVariable : m_PermutationVariables)
  {
    if (permutationVariable.m_sName == sNameHash)
    {
      if (permutationVariable.m_sValue.GetView() != sValue)
      {
        permutationVariable.m_sValue.Assign(sValue);

        m_bPermutationVariablesModified = true;
      }
      return;
    }
  }

  auto& permutationVariable   = m_PermutationVariables.ExpandAndGetRef();
  permutationVariable.m_sName = sNameHash;
  permutationVariable.m_sValue.Assign(sValue);

  m_bPermutationVariablesModified = true;
}

void xiiView::SetRenderPassProperty(xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  SetProperty(m_PassProperties, sPassName, sPropertyName, value);
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
  XII_IGNORE_UNUSED(uiDataIndex);
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
  if (!m_hRenderPipeline.IsValid())
    return;

  m_bPermutationVariablesModified = false;
}

void xiiView::ApplyPermutationVariables()
{
  if (!m_bPermutationVariablesModified)
    return;

  m_bPermutationVariablesModified = false;
}

void xiiView::SetProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  bool  bExisted = false;
  auto& prop     = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName   = sPassName;
    prop.m_sPropertyName = sPropertyName;
    prop.m_bIsValid      = true;
  }

  prop.m_bIsDirty     = true;
  prop.m_CurrentValue = value;
}

void xiiView::SetReadBackProperty(xiiMap<xiiString, PropertyValue>& map, xiiStringView sPassName, xiiStringView sPropertyName, const xiiVariant& value)
{
  xiiStringBuilder sKey(sPassName, "::", sPropertyName);

  bool  bExisted = false;
  auto& prop     = map.FindOrAdd(sKey, &bExisted).Value();

  if (!bExisted)
  {
    prop.m_sObjectName   = sPassName;
    prop.m_sPropertyName = sPropertyName;
    prop.m_bIsValid      = true;
  }

  prop.m_bIsDirty     = false;
  prop.m_CurrentValue = value;
}

void xiiView::ReadBackPassProperties()
{
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
    it.Value().m_bIsDirty = false;
    it.Value().m_bIsValid = false;
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
