/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Components/Gameplay/TimedDeathComponent.h>

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiTimedDeathComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant()), new xiiDefaultValueAttribute(xiiTime::MakeFromSeconds(1.0))),
    XII_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new xiiClampValueAttribute(xiiTime(), xiiVariant())),
    XII_RESOURCE_MEMBER_PROPERTY("TimeoutPrefab", m_hTimeoutPrefab)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Prefab", xiiDependencyFlags::Package)),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgComponentInternalTrigger, OnTriggered),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Gameplay"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiTimedDeathComponent::xiiTimedDeathComponent()  = default;
xiiTimedDeathComponent::~xiiTimedDeathComponent() = default;

void xiiTimedDeathComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_hTimeoutPrefab;
}

void xiiTimedDeathComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_hTimeoutPrefab;
}

void xiiTimedDeathComponent::OnSimulationStarted()
{
  xiiMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("Suicide");

  xiiWorld* pWorld = GetWorld();

  const xiiTime tKill = xiiTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleMinMax(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);

  // make sure the prefab is available when the component dies
  if (m_hTimeoutPrefab.IsValid())
  {
    xiiResourceManager::PreloadResource(m_hTimeoutPrefab);
  }
}

void xiiTimedDeathComponent::OnTriggered(xiiMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != xiiTempHashedString("Suicide"))
    return;

  if (m_hTimeoutPrefab.IsValid())
  {
    xiiResourceLock<xiiPrefabResource> pPrefab(m_hTimeoutPrefab, xiiResourceAcquireMode::AllowLoadingFallback);

    xiiPrefabInstantiationOptions options;
    options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

    pPrefab->InstantiatePrefab(*GetWorld(), GetOwner()->GetGlobalTransform(), options);
  }

  GetWorld()->DeleteObjectDelayed(GetOwner()->GetHandle());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class xiiTimedDeathComponentPatch_1_2 : public xiiGraphPatch
{
public:
  xiiTimedDeathComponentPatch_1_2() :
    xiiGraphPatch("xiiTimedDeathComponent", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
    pNode->RenameProperty("Timeout Prefab", "TimeoutPrefab");
  }
};

xiiTimedDeathComponentPatch_1_2 g_xiiTimedDeathComponentPatch_1_2;

XII_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_TimedDeathComponent);
