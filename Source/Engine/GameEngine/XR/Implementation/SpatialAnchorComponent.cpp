#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/SpatialAnchorComponent.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSpatialAnchorComponent, 2, xiiComponentMode::Dynamic)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("XR"),
    new xiiInDevelopmentAttribute(xiiInDevelopmentAttribute::Phase::Beta),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSpatialAnchorComponent::xiiSpatialAnchorComponent() = default;
xiiSpatialAnchorComponent::~xiiSpatialAnchorComponent()
{
  if (xiiXRSpatialAnchorsInterface* pXR = xiiSingletonRegistry::GetSingletonInstance<xiiXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = xiiXRSpatialAnchorID();
    }
  }
}

void xiiSpatialAnchorComponent::SerializeComponent(xiiWorldWriter& ref_stream) const
{
  SUPER::SerializeComponent(ref_stream);
  xiiStreamWriter& s = ref_stream.GetStream();
}

void xiiSpatialAnchorComponent::DeserializeComponent(xiiWorldReader& ref_stream)
{
  SUPER::DeserializeComponent(ref_stream);
  const xiiUInt32  uiVersion = ref_stream.GetComponentTypeVersion(GetStaticRTTI());
  xiiStreamReader& s         = ref_stream.GetStream();
  if (uiVersion == 1)
  {
    xiiString sAnchorName;
    s >> sAnchorName;
  }
}

xiiResult xiiSpatialAnchorComponent::RecreateAnchorAt(const xiiTransform& position)
{
  if (xiiXRSpatialAnchorsInterface* pXR = xiiSingletonRegistry::GetSingletonInstance<xiiXRSpatialAnchorsInterface>())
  {
    if (!m_AnchorID.IsInvalidated())
    {
      pXR->DestroyAnchor(m_AnchorID).IgnoreResult();
      m_AnchorID = xiiXRSpatialAnchorID();
    }

    m_AnchorID = pXR->CreateAnchor(position);
    return m_AnchorID.IsInvalidated() ? XII_FAILURE : XII_SUCCESS;
  }

  return XII_SUCCESS;
}

void xiiSpatialAnchorComponent::Update()
{
  if (IsActiveAndSimulating())
  {
    if (xiiXRSpatialAnchorsInterface* pXR = xiiSingletonRegistry::GetSingletonInstance<xiiXRSpatialAnchorsInterface>())
    {
      if (!m_AnchorID.IsInvalidated())
      {
        xiiTransform globalTransform;
        if (pXR->TryGetAnchorTransform(m_AnchorID, globalTransform).Succeeded())
        {
          globalTransform.m_vScale = GetOwner()->GetGlobalScaling();
          GetOwner()->SetGlobalTransform(globalTransform);
        }
      }
      else
      {
        m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
      }
    }
  }
}

void xiiSpatialAnchorComponent::OnSimulationStarted()
{
  if (xiiXRSpatialAnchorsInterface* pXR = xiiSingletonRegistry::GetSingletonInstance<xiiXRSpatialAnchorsInterface>())
  {
    m_AnchorID = pXR->CreateAnchor(GetOwner()->GetGlobalTransform());
  }
}

XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_SpatialAnchorComponent);
