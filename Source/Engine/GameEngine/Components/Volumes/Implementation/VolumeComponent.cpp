/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Utils/BlackboardTemplateResource.h>
#include <GameEngine/Volumes/VolumeComponent.h>

// clang-format off
XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiVolumeComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Type", GetVolumeType, SetVolumeType)->AddAttributes(new xiiDynamicStringEnumAttribute("SpatialDataCategoryEnum"), new xiiDefaultValueAttribute("GenericVolume")),
    XII_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new xiiClampValueAttribute(-64.0f, 64.0f)),
    XII_RESOURCE_ACCESSOR_PROPERTY("Template", GetTemplate, SetTemplate)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_BlackboardTemplate")),
    XII_MAP_ACCESSOR_PROPERTY("Values", Reflection_GetKeys, Reflection_GetValue, Reflection_InsertValue, Reflection_RemoveValue),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(SetValue, In, "Name", In, "Value"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetValue, In, "Name"),
  }
  XII_END_FUNCTIONS;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiVolumeComponent::xiiVolumeComponent()  = default;
xiiVolumeComponent::~xiiVolumeComponent() = default;

void xiiVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  InitializeFromTemplate();

  GetOwner()->UpdateLocalBounds();
}

void xiiVolumeComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  RemoveReloadFunction();

  GetOwner()->UpdateLocalBounds();
}

void xiiVolumeComponent::SetTemplate(const xiiBlackboardTemplateResourceHandle& hResource)
{
  RemoveReloadFunction();

  m_hTemplateResource = hResource;

  if (IsActiveAndInitialized())
  {
    ReloadTemplate();
  }
}

void xiiVolumeComponent::SetSortOrder(float fOrder)
{
  fOrder       = xiiMath::Clamp(fOrder, -64.0f, 64.0f);
  m_fSortOrder = fOrder;
}

void xiiVolumeComponent::SetVolumeType(const char* szType)
{
  m_SpatialCategory = xiiSpatialData::RegisterCategory(szType, xiiSpatialData::Flags::None);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

const char* xiiVolumeComponent::GetVolumeType() const
{
  return xiiSpatialData::GetCategoryName(m_SpatialCategory);
}

void xiiVolumeComponent::SetValue(const xiiHashedString& sName, const xiiVariant& value)
{
  m_Values.Insert(sName, value);
}

void xiiVolumeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fSortOrder;

  auto& sCategory = xiiSpatialData::GetCategoryName(m_SpatialCategory);
  s << sCategory;

  s << m_hTemplateResource;

  // Only serialize overwritten values so a template change doesn't require a re-save of all volumes
  xiiUInt32 numValues = m_OverwrittenValues.GetCount();
  s << numValues;
  for (auto& sName : m_OverwrittenValues)
  {
    xiiVariant value;
    m_Values.TryGetValue(sName, value);

    s << sName;
    s << value;
  }
}

void xiiVolumeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_fSortOrder;

  xiiHashedString sCategory;
  s >> sCategory;
  m_SpatialCategory = xiiSpatialData::RegisterCategory(sCategory, xiiSpatialData::Flags::None);

  s >> m_hTemplateResource;

  // m_OverwrittenValues is only used in editor so we don't write to it here
  xiiUInt32 numValues = 0;
  s >> numValues;
  for (xiiUInt32 i = 0; i < numValues; ++i)
  {
    xiiHashedString sName;
    xiiVariant      value;
    s >> sName;
    s >> value;

    m_Values.Insert(sName, value);
  }
}

const xiiRangeView<const xiiString&, xiiUInt32> xiiVolumeComponent::Reflection_GetKeys() const
{
  return xiiRangeView<const xiiString&, xiiUInt32>([]() -> xiiUInt32 { return 0; },
                                                   [this]() -> xiiUInt32 { return m_OverwrittenValues.GetCount(); },
                                                   [](xiiUInt32& ref_uiIt) { ++ref_uiIt; },
                                                   [this](const xiiUInt32& uiIt) -> const xiiString& { return m_OverwrittenValues[uiIt].GetString(); });
}

bool xiiVolumeComponent::Reflection_GetValue(xiiStringView sName, xiiVariant& value) const
{
  return m_Values.TryGetValue(xiiTempHashedString(sName), value);
}

void xiiVolumeComponent::Reflection_InsertValue(xiiStringView sName, const xiiVariant& value)
{
  xiiHashedString sNameHash;
  sNameHash.Assign(sName);

  // Only needed in editor
  if (GetUniqueID() != xiiInvalidIndex && m_OverwrittenValues.Contains(sNameHash) == false)
  {
    m_OverwrittenValues.PushBack(sNameHash);
  }

  m_Values.Insert(sNameHash, value);
}

void xiiVolumeComponent::Reflection_RemoveValue(xiiStringView sName)
{
  xiiHashedString sNameHash;
  sNameHash.Assign(sName);

  m_OverwrittenValues.RemoveAndCopy(sNameHash);

  m_Values.Remove(sNameHash);
}

void xiiVolumeComponent::InitializeFromTemplate()
{
  if (!m_hTemplateResource.IsValid())
    return;

  xiiResourceLock<xiiBlackboardTemplateResource> pTemplate(m_hTemplateResource, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pTemplate.GetAcquireResult() != xiiResourceAcquireResult::Final)
    return;

  for (const auto& entry : pTemplate->GetDescriptor().m_Entries)
  {
    if (m_Values.Contains(entry.m_sName) == false)
    {
      m_Values.Insert(entry.m_sName, entry.m_InitialValue);
    }
  }

  if (m_bReloadFunctionAdded == false)
  {
    GetWorld()->AddResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr,
                                          [](const xiiWorld::ResourceReloadContext& context) {
                                            xiiStaticCast<xiiVolumeComponent*>(context.m_pComponent)->ReloadTemplate();
                                          });

    m_bReloadFunctionAdded = true;
  }
}

void xiiVolumeComponent::ReloadTemplate()
{
  // Remove all values that are not overwritten
  xiiHashTable<xiiHashedString, xiiVariant> overwrittenValues;
  for (auto& sName : m_OverwrittenValues)
  {
    overwrittenValues.Insert(sName, m_Values[sName]);
  }
  m_Values.Swap(overwrittenValues);

  InitializeFromTemplate();
}

void xiiVolumeComponent::RemoveReloadFunction()
{
  if (m_bReloadFunctionAdded)
  {
    GetWorld()->RemoveResourceReloadFunction(m_hTemplateResource, GetHandle(), nullptr);

    m_bReloadFunctionAdded = false;
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVolumeSphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiSphereVisualizerAttribute("Radius", xiiColorScheme::LightUI(xiiColorScheme::Cyan)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiVolumeSphereComponent::xiiVolumeSphereComponent()  = default;
xiiVolumeSphereComponent::~xiiVolumeSphereComponent() = default;

void xiiVolumeSphereComponent::SetRadius(float fRadius)
{
  if (m_fRadius != fRadius)
  {
    m_fRadius = fRadius;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void xiiVolumeSphereComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = fFalloff;
}

void xiiVolumeSphereComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
}

void xiiVolumeSphereComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
}

void xiiVolumeSphereComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(xiiBoundingSphere::MakeFromCenterAndRadius(xiiVec3::MakeZero(), m_fRadius), m_SpatialCategory);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiVolumeBoxComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(10.0f)), new xiiClampValueAttribute(xiiVec3(0), xiiVariant())),
    XII_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.5f)), new xiiClampValueAttribute(xiiVec3(0.0f), xiiVec3(1.0f))),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
    new xiiBoxVisualizerAttribute("Extents", 1.0f, xiiColorScheme::LightUI(xiiColorScheme::Cyan)),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiVolumeBoxComponent::xiiVolumeBoxComponent()  = default;
xiiVolumeBoxComponent::~xiiVolumeBoxComponent() = default;

void xiiVolumeBoxComponent::SetExtents(const xiiVec3& vExtents)
{
  if (m_vExtents != vExtents)
  {
    m_vExtents = vExtents;

    if (IsActiveAndInitialized())
    {
      GetOwner()->UpdateLocalBounds();
    }
  }
}

void xiiVolumeBoxComponent::SetFalloff(const xiiVec3& vFalloff)
{
  m_vFalloff = vFalloff;
}

void xiiVolumeBoxComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vFalloff;
}

void xiiVolumeBoxComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vFalloff;
}

void xiiVolumeBoxComponent::OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(xiiBoundingBoxSphere(xiiBoundingBox(-m_vExtents * 0.5f, m_vExtents * 0.5f)), m_SpatialCategory);
}
