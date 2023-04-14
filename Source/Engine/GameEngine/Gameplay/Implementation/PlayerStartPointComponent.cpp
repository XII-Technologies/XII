#include <GameEngine/GameEnginePCH.h>

#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/PlayerStartPointComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiPlayerStartPointComponent, 2, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("PlayerPrefab", GetPlayerPrefabFile, SetPlayerPrefabFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("PlayerPrefab")),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
    new xiiDirectionVisualizerAttribute(xiiBasisAxis::PositiveX, 0.5f, xiiColor::DarkSlateBlue),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiPlayerStartPointComponent::xiiPlayerStartPointComponent()  = default;
xiiPlayerStartPointComponent::~xiiPlayerStartPointComponent() = default;

void xiiPlayerStartPointComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_hPlayerPrefab;

  xiiPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), stream, m_Parameters);
}

void xiiPlayerStartPointComponent::DeserializeComponent(xiiWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_hPlayerPrefab;

  if (uiVersion >= 2)
  {
    xiiPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, stream);
  }
}

void xiiPlayerStartPointComponent::SetPlayerPrefabFile(const char* szFile)
{
  xiiPrefabResourceHandle hResource;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = xiiResourceManager::LoadResource<xiiPrefabResource>(szFile);
  }

  SetPlayerPrefab(hResource);
}

const char* xiiPlayerStartPointComponent::GetPlayerPrefabFile() const
{
  if (!m_hPlayerPrefab.IsValid())
    return "";

  return m_hPlayerPrefab.GetResourceID();
}

void xiiPlayerStartPointComponent::SetPlayerPrefab(const xiiPrefabResourceHandle& hPrefab)
{
  m_hPlayerPrefab = hPrefab;
}

const xiiPrefabResourceHandle& xiiPlayerStartPointComponent::GetPlayerPrefab() const
{
  return m_hPlayerPrefab;
}

const xiiRangeView<const char*, xiiUInt32> xiiPlayerStartPointComponent::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([]() -> xiiUInt32 { return 0; }, [this]() -> xiiUInt32 { return m_Parameters.GetCount(); }, [](xiiUInt32& it) { ++it; }, [this](const xiiUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void xiiPlayerStartPointComponent::SetParameter(const char* szKey, const xiiVariant& value)
{
  xiiHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != xiiInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void xiiPlayerStartPointComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(xiiTempHashedString(szKey));
}

bool xiiPlayerStartPointComponent::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  xiiUInt32 it = m_Parameters.Find(szKey);

  if (it == xiiInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}


XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_PlayerStartPointComponent);
