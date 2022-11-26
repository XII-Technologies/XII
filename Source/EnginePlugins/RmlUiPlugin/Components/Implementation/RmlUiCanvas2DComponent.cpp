#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RmlUiPlugin/Components/RmlUiCanvas2DComponent.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiSingleton.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiRmlUiCanvas2DComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("RmlFile", GetRmlFile, SetRmlFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Rml_UI")),
    XII_ACCESSOR_PROPERTY("AnchorPoint", GetAnchorPoint, SetAnchorPoint)->AddAttributes(new xiiClampValueAttribute(xiiVec2(0), xiiVec2(1))),
    XII_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new xiiSuffixAttribute("px"), new xiiMinValueTextAttribute("Auto")),
    XII_ACCESSOR_PROPERTY("Offset", GetOffset, SetOffset)->AddAttributes(new xiiDefaultValueAttribute(xiiVec2::ZeroVector()), new xiiSuffixAttribute("px")),    
    XII_ACCESSOR_PROPERTY("PassInput", GetPassInput, SetPassInput)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("AutobindBlackboards", GetAutobindBlackboards, SetAutobindBlackboards)->AddAttributes(new xiiDefaultValueAttribute(true)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgRmlUiReload, OnMsgReload)
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gui/RmlUi"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiRmlUiCanvas2DComponent::xiiRmlUiCanvas2DComponent()        = default;
xiiRmlUiCanvas2DComponent::~xiiRmlUiCanvas2DComponent()       = default;
xiiRmlUiCanvas2DComponent& xiiRmlUiCanvas2DComponent::operator=(xiiRmlUiCanvas2DComponent&& rhs) = default;

void xiiRmlUiCanvas2DComponent::Initialize()
{
  SUPER::Initialize();

  UpdateAutobinding();
}

void xiiRmlUiCanvas2DComponent::Deinitialize()
{
  SUPER::Deinitialize();

  if (m_pContext != nullptr)
  {
    xiiRmlUi::GetSingleton()->DeleteContext(m_pContext);
    m_pContext = nullptr;
  }

  m_DataBindings.Clear();
}

void xiiRmlUiCanvas2DComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOrCreateRmlContext()->ShowDocument();
}

void xiiRmlUiCanvas2DComponent::OnDeactivated()
{
  m_pContext->HideDocument();

  SUPER::OnDeactivated();
}

void xiiRmlUiCanvas2DComponent::Update()
{
  if (m_pContext == nullptr)
    return;

  xiiVec2 viewSize = xiiVec2(1.0f);
  if (xiiView* pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView, GetWorld()))
  {
    viewSize.x = pView->GetViewport().width;
    viewSize.y = pView->GetViewport().height;
  }

  float fScale = 1.0f;
  if (m_vReferenceResolution.x > 0 && m_vReferenceResolution.y > 0)
  {
    fScale = viewSize.y / m_vReferenceResolution.y;
  }

  xiiVec2 size = xiiVec2(static_cast<float>(m_vSize.x), static_cast<float>(m_vSize.y)) * fScale;
  if (size.x <= 0.0f)
  {
    size.x = viewSize.x;
  }
  if (size.y <= 0.0f)
  {
    size.y = viewSize.y;
  }
  m_pContext->SetSize(xiiVec2U32(static_cast<xiiUInt32>(size.x), static_cast<xiiUInt32>(size.y)));

  xiiVec2 offset = xiiVec2(static_cast<float>(m_vOffset.x), static_cast<float>(m_vOffset.y)) * fScale;
  offset         = (viewSize - size).CompMul(m_vAnchorPoint) - offset.CompMul(m_vAnchorPoint * 2.0f - xiiVec2(1.0f));
  m_pContext->SetOffset(xiiVec2I32(static_cast<int>(offset.x), static_cast<int>(offset.y)));

  m_pContext->SetDpiScale(fScale);

  if (m_bPassInput && GetWorld()->GetWorldSimulationEnabled())
  {
    xiiVec2 mousePos;
    xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionX, &mousePos.x);
    xiiInputManager::GetInputSlotState(xiiInputSlot_MousePositionY, &mousePos.y);

    mousePos = mousePos.CompMul(viewSize) - offset;
    m_pContext->UpdateInput(mousePos);
  }

  for (auto& pDataBinding : m_DataBindings)
  {
    if (pDataBinding != nullptr)
    {
      pDataBinding->Update();
    }
  }

  m_pContext->Update();
}

void xiiRmlUiCanvas2DComponent::SetRmlFile(const char* szFile)
{
  xiiRmlUiResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiRmlUiResource>(szFile);
    xiiResourceManager::PreloadResource(hResource);
  }

  SetRmlResource(hResource);
}

const char* xiiRmlUiCanvas2DComponent::GetRmlFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void xiiRmlUiCanvas2DComponent::SetRmlResource(const xiiRmlUiResourceHandle& hResource)
{
  if (m_hResource != hResource)
  {
    m_hResource = hResource;

    if (m_pContext != nullptr)
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }

      UpdateCachedValues();
    }
  }
}

void xiiRmlUiCanvas2DComponent::SetOffset(const xiiVec2I32& offset)
{
  m_vOffset = offset;
}

void xiiRmlUiCanvas2DComponent::SetSize(const xiiVec2U32& size)
{
  if (m_vSize != size)
  {
    m_vSize = size;

    if (m_pContext != nullptr)
    {
      m_pContext->SetSize(m_vSize);
    }
  }
}

void xiiRmlUiCanvas2DComponent::SetAnchorPoint(const xiiVec2& anchorPoint)
{
  m_vAnchorPoint = anchorPoint;
}

void xiiRmlUiCanvas2DComponent::SetPassInput(bool bPassInput)
{
  m_bPassInput = bPassInput;
}

void xiiRmlUiCanvas2DComponent::SetAutobindBlackboards(bool bAutobind)
{
  if (m_bAutobindBlackboards != bAutobind)
  {
    m_bAutobindBlackboards = bAutobind;

    UpdateAutobinding();
  }
}

xiiUInt32 xiiRmlUiCanvas2DComponent::AddDataBinding(xiiUniquePtr<xiiRmlUiDataBinding>&& dataBinding)
{
  // Document needs to be loaded again since data bindings have to be set before document load
  if (m_pContext != nullptr)
  {
    if (dataBinding->Initialize(*m_pContext).Succeeded())
    {
      if (m_pContext->LoadDocumentFromResource(m_hResource).Succeeded() && IsActive())
      {
        m_pContext->ShowDocument();
      }
    }
  }

  for (xiiUInt32 i = 0; i < m_DataBindings.GetCount(); ++i)
  {
    if (dataBinding == nullptr)
    {
      m_DataBindings[i] = std::move(dataBinding);
      return i;
    }
  }

  xiiUInt32 uiDataBindingIndex = m_DataBindings.GetCount();
  m_DataBindings.PushBack(std::move(dataBinding));
  return uiDataBindingIndex;
}

void xiiRmlUiCanvas2DComponent::RemoveDataBinding(xiiUInt32 uiDataBindingIndex)
{
  auto& pDataBinding = m_DataBindings[uiDataBindingIndex];

  if (m_pContext != nullptr)
  {
    pDataBinding->Deinitialize(*m_pContext);
  }

  m_DataBindings[uiDataBindingIndex] = nullptr;
}

xiiUInt32 xiiRmlUiCanvas2DComponent::AddBlackboardBinding(const xiiSharedPtr<xiiBlackboard>& pBlackboard)
{
  auto pDataBinding = XII_DEFAULT_NEW(xiiRmlUiInternal::BlackboardDataBinding, pBlackboard);
  return AddDataBinding(pDataBinding);
}

void xiiRmlUiCanvas2DComponent::RemoveBlackboardBinding(xiiUInt32 uiDataBindingIndex)
{
  RemoveDataBinding(uiDataBindingIndex);
}

xiiRmlUiContext* xiiRmlUiCanvas2DComponent::GetOrCreateRmlContext()
{
  if (m_pContext != nullptr)
  {
    return m_pContext;
  }

  xiiStringBuilder sName = "Context_";
  if (m_hResource.IsValid())
  {
    sName.Append(m_hResource.GetResourceID().GetView());
  }
  sName.AppendFormat("_{}", xiiArgP(this));

  m_pContext = xiiRmlUi::GetSingleton()->CreateContext(sName, m_vSize);

  for (auto& pDataBinding : m_DataBindings)
  {
    pDataBinding->Initialize(*m_pContext).IgnoreResult();
  }

  m_pContext->LoadDocumentFromResource(m_hResource).IgnoreResult();

  UpdateCachedValues();

  return m_pContext;
}

void xiiRmlUiCanvas2DComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  xiiStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_vOffset;
  s << m_vSize;
  s << m_vAnchorPoint;
  s << m_bPassInput;
  s << m_bAutobindBlackboards;
}

void xiiRmlUiCanvas2DComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32  uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = stream.GetStream();

  s >> m_hResource;
  s >> m_vOffset;
  s >> m_vSize;
  s >> m_vAnchorPoint;
  s >> m_bPassInput;

  if (uiVersion >= 2)
  {
    s >> m_bAutobindBlackboards;
  }
}

xiiResult xiiRmlUiCanvas2DComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  bAlwaysVisible = true;
  return XII_SUCCESS;
}

void xiiRmlUiCanvas2DComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  if (msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::EditorView && msg.m_pView->GetCameraUsageHint() != xiiCameraUsageHint::Thumbnail)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != xiiInvalidRenderDataCategory)
    return;

  if (m_pContext != nullptr)
  {
    xiiRmlUi::GetSingleton()->ExtractContext(*m_pContext, msg);
  }
}

void xiiRmlUiCanvas2DComponent::OnMsgReload(xiiMsgRmlUiReload& msg)
{
  if (m_pContext != nullptr)
  {
    m_pContext->ReloadDocumentFromResource(m_hResource).IgnoreResult();
    m_pContext->ShowDocument();

    UpdateCachedValues();
  }
}

void xiiRmlUiCanvas2DComponent::UpdateCachedValues()
{
  m_ResourceEventUnsubscriber.Unsubscribe();
  m_vReferenceResolution.SetZero();

  if (m_hResource.IsValid())
  {
    {
      xiiResourceLock pResource(m_hResource, xiiResourceAcquireMode::BlockTillLoaded);

      if (pResource->GetScaleMode() == xiiRmlUiScaleMode::WithScreenSize)
      {
        m_vReferenceResolution = pResource->GetReferenceResolution();
      }
    }

    {
      xiiResourceLock pResource(m_hResource, xiiResourceAcquireMode::PointerOnly);

      pResource->m_ResourceEvents.AddEventHandler(
        [hComponent = GetHandle(), pWorld = GetWorld()](const xiiResourceEvent& e) {
          if (e.m_Type == xiiResourceEvent::Type::ResourceContentUnloading)
          {
            pWorld->PostMessage(hComponent, xiiMsgRmlUiReload(), xiiTime::Zero());
          }
        },
        m_ResourceEventUnsubscriber);
    }
  }
}

void xiiRmlUiCanvas2DComponent::UpdateAutobinding()
{
  for (xiiUInt32 uiIndex : m_AutoBindings)
  {
    RemoveDataBinding(uiIndex);
  }

  m_AutoBindings.Clear();

  if (m_bAutobindBlackboards)
  {
    xiiHybridArray<xiiBlackboardComponent*, 4> blackboardComponents;

    xiiGameObject* pObject = GetOwner();
    while (pObject != nullptr)
    {
      pObject->TryGetComponentsOfBaseType(blackboardComponents);
      for (auto pBlackboardComponent : blackboardComponents)
      {
        m_AutoBindings.PushBack(AddBlackboardBinding(pBlackboardComponent->GetBoard()));
      }

      pObject = pObject->GetParent();
    }
  }
}
