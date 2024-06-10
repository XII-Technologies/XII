#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Effects/PostProcessing/PostProcessingComponent.h>
#include <GraphicsCore/Components/CameraComponent.h>
#include <GraphicsCore/Pipeline/View.h>
#include <GraphicsCore/RenderWorld/RenderWorld.h>

xiiPostProcessingComponentManager::xiiPostProcessingComponentManager(xiiWorld* pWorld) :
  xiiComponentManager(pWorld)
{
}

void xiiPostProcessingComponentManager::Initialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiPostProcessingComponentManager::UpdateComponents, this);
  desc.m_Phase = UpdateFunctionDesc::Phase::PostTransform;

  RegisterUpdateFunction(desc);
}

void xiiPostProcessingComponentManager::UpdateComponents(const UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->SampleAndSetViewProperties();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPostProcessingValueMapping, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPostProcessingValueMapping>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("RenderPass", m_sRenderPassName),
    XII_MEMBER_PROPERTY("Property", m_sPropertyName),
    XII_MEMBER_PROPERTY("VolumeValue", m_sVolumeValueName),
    XII_MEMBER_PROPERTY("DefaultValue", m_DefaultValue)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("InterpolationDuration", m_InterpolationDuration),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

xiiResult xiiPostProcessingValueMapping::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream << m_sRenderPassName;
  inout_stream << m_sPropertyName;
  inout_stream << m_sVolumeValueName;
  inout_stream << m_DefaultValue;
  inout_stream << m_InterpolationDuration;

  return XII_SUCCESS;
}

xiiResult xiiPostProcessingValueMapping::Deserialize(xiiStreamReader& inout_stream)
{
  inout_stream >> m_sRenderPassName;
  inout_stream >> m_sPropertyName;
  inout_stream >> m_sVolumeValueName;
  inout_stream >> m_DefaultValue;
  inout_stream >> m_InterpolationDuration;

  return XII_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPostProcessingComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("VolumeType", GetVolumeType, SetVolumeType)->AddAttributes(new xiiDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new xiiDefaultValueAttribute("GenericVolume")),
    XII_ARRAY_ACCESSOR_PROPERTY("Mappings", Mappings_GetCount, Mappings_GetMapping, Mappings_SetMapping, Mappings_Insert, Mappings_Remove),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiPostProcessingComponent::xiiPostProcessingComponent()                                   = default;
xiiPostProcessingComponent::xiiPostProcessingComponent(xiiPostProcessingComponent&& other) = default;
xiiPostProcessingComponent::~xiiPostProcessingComponent()                                  = default;
xiiPostProcessingComponent& xiiPostProcessingComponent::operator=(xiiPostProcessingComponent&& other) = default;

void xiiPostProcessingComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  auto& sCategory = xiiSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s.WriteArray(m_Mappings).IgnoreResult();
}

void xiiPostProcessingComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  xiiHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = xiiSpatialData::RegisterCategory(sCategory, xiiSpatialData::Flags::None);

  s.ReadArray(m_Mappings).IgnoreResult();
}

void xiiPostProcessingComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = xiiSpatialData::RegisterCategory(szType, xiiSpatialData::Flags::None);
}

const char* xiiPostProcessingComponent::GetVolumeType() const
{
  return xiiSpatialData::GetCategoryName(m_SpatialCategory);
}

void xiiPostProcessingComponent::Initialize()
{
  m_pSampler = XII_DEFAULT_NEW(xiiVolumeSampler);

  RegisterSamplerValues();
}

void xiiPostProcessingComponent::Deinitialize()
{
  m_pSampler = nullptr;
}

void xiiPostProcessingComponent::OnActivated()
{
  SUPER::OnActivated();

  xiiCameraComponent* pCameraComponent = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType(pCameraComponent))
  {
    m_hCameraComponent = pCameraComponent->GetHandle();
  }
}

void xiiPostProcessingComponent::OnDeactivated()
{
  ResetViewProperties();

  m_hCameraComponent.Invalidate();

  SUPER::OnDeactivated();
}

void xiiPostProcessingComponent::Mappings_SetMapping(xiiUInt32 i, const xiiPostProcessingValueMapping& mapping)
{
  m_Mappings.EnsureCount(i + 1);
  m_Mappings[i] = mapping;

  RegisterSamplerValues();
}

void xiiPostProcessingComponent::Mappings_Insert(xiiUInt32 uiIndex, const xiiPostProcessingValueMapping& mapping)
{
  m_Mappings.InsertAt(uiIndex, mapping);

  RegisterSamplerValues();
}

void xiiPostProcessingComponent::Mappings_Remove(xiiUInt32 uiIndex)
{
  m_Mappings.RemoveAtAndCopy(uiIndex);

  ResetViewProperties();
  RegisterSamplerValues();
}

xiiView* xiiPostProcessingComponent::FindView() const
{
  const xiiWorld* pWorld = GetWorld();
  xiiView*        pView  = nullptr;

  const xiiCameraComponent* pCameraComponent = nullptr;
  if (pWorld->TryGetComponent(m_hCameraComponent, pCameraComponent) && pCameraComponent->GetUsageHint() == xiiCameraUsageHint::RenderTarget)
  {
    xiiRenderWorld::TryGetView(pCameraComponent->GetRenderTargetView(), pView);
  }

  if (pView == nullptr)
  {
    pView = xiiRenderWorld::GetViewByUsageHint(xiiCameraUsageHint::MainView, xiiCameraUsageHint::EditorView, pWorld);
  }

  return pView;
}

void xiiPostProcessingComponent::RegisterSamplerValues()
{
  if (m_pSampler == nullptr)
    return;

  m_pSampler->DeregisterAllValues();

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sVolumeValueName.IsEmpty())
      continue;

    m_pSampler->RegisterValue(mapping.m_sVolumeValueName, mapping.m_DefaultValue, mapping.m_InterpolationDuration);
  }
}

void xiiPostProcessingComponent::ResetViewProperties()
{
  if (xiiView* pView = FindView())
  {
    pView->ResetRenderPassProperties();
  }
}

void xiiPostProcessingComponent::SampleAndSetViewProperties()
{
  xiiView* pView = FindView();
  if (pView == nullptr)
    return;

  const xiiVec3 vSamplePos = pView->GetCullingCamera()->GetCenterPosition();

  xiiWorld* pWorld = GetWorld();
  xiiTime   deltaTime;
  if (pWorld->GetWorldSimulationEnabled())
  {
    deltaTime = pWorld->GetClock().GetTimeDiff();
  }
  else
  {
    deltaTime = xiiClock::GetGlobalClock()->GetTimeDiff();
  }

  m_pSampler->SampleAtPosition(*pWorld, m_SpatialCategory, vSamplePos, deltaTime);

  for (auto& mapping : m_Mappings)
  {
    if (mapping.m_sRenderPassName.IsEmpty() || mapping.m_sPropertyName.IsEmpty())
      continue;

    xiiVariant value;
    if (mapping.m_sVolumeValueName.IsEmpty())
    {
      value = mapping.m_DefaultValue;
    }
    else
    {
      value = m_pSampler->GetValue(mapping.m_sVolumeValueName);
    }

    pView->SetRenderPassProperty(mapping.m_sRenderPassName, mapping.m_sPropertyName, value);
  }
}
