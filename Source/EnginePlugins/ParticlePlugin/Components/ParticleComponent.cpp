#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/WorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <ParticlePlugin/Components/ParticleFinisherComponent.h>
#include <ParticlePlugin/Resources/ParticleEffectResource.h>
#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

//////////////////////////////////////////////////////////////////////////

xiiParticleComponentManager::xiiParticleComponentManager(xiiWorld* pWorld) :
  SUPER(pWorld)
{
}

void xiiParticleComponentManager::Initialize()
{
  {
    auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiParticleComponentManager::Update, this);
    desc.m_bOnlyUpdateWhenSimulating = true;
    RegisterUpdateFunction(desc);
  }
}

void xiiParticleComponentManager::Update(const xiiWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

void xiiParticleComponentManager::UpdatePfxTransformsAndBounds()
{
  for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->UpdatePfxTransform();

      // This function is called in the post-transform phase so the global bounds and transform have already been calculated at this point.
      // Therefore we need to manually update the global bounds again to ensure correct bounds for culling and rendering.
      pComponent->GetOwner()->UpdateLocalBounds();
      pComponent->GetOwner()->UpdateGlobalBounds();
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiParticleComponent, 5, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Effect", GetParticleEffectFile, SetParticleEffectFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Particle_Effect")),
    XII_MEMBER_PROPERTY("SpawnAtStart", m_bSpawnAtStart)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ENUM_MEMBER_PROPERTY("OnFinishedAction", xiiOnComponentFinishedAction2, m_OnFinishedAction),
    XII_MEMBER_PROPERTY("MinRestartDelay", m_MinRestartDelay),
    XII_MEMBER_PROPERTY("RestartDelayRange", m_RestartDelayRange),
    XII_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    XII_ENUM_MEMBER_PROPERTY("SpawnDirection", xiiBasisAxis, m_SpawnDirection)->AddAttributes(new xiiDefaultValueAttribute((xiiInt32)xiiBasisAxis::PositiveZ)),
    XII_MEMBER_PROPERTY("IgnoreOwnerRotation", m_bIgnoreOwnerRotation),
    XII_MEMBER_PROPERTY("SharedInstanceName", m_sSharedInstanceName),
    XII_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new xiiExposedParametersAttribute("Effect"), new xiiExposeColorAlphaAttribute),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("Effects"),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgSetPlaying, OnMsgSetPlaying),
    XII_MESSAGE_HANDLER(xiiMsgExtractRenderData, OnMsgExtractRenderData),
    XII_MESSAGE_HANDLER(xiiMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(StartEffect),
    XII_SCRIPT_FUNCTION_PROPERTY(StopEffect),
    XII_SCRIPT_FUNCTION_PROPERTY(InterruptEffect),
    XII_SCRIPT_FUNCTION_PROPERTY(IsEffectActive),
  }
  XII_END_FUNCTIONS;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiParticleComponent::xiiParticleComponent()  = default;
xiiParticleComponent::~xiiParticleComponent() = default;

void xiiParticleComponent::OnDeactivated()
{
  m_EffectController.Invalidate();

  xiiRenderComponent::OnDeactivated();
}

void xiiParticleComponent::SerializeComponent(xiiWorldWriter& stream) const
{
  auto& s = stream.GetStream();

  s << m_hEffectResource;
  s << m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s << bAutoRestart;
  }

  s << m_MinRestartDelay;
  s << m_RestartDelayRange;
  s << m_RestartTime;
  s << m_uiRandomSeed;
  s << m_sSharedInstanceName;

  // Version 2
  s << m_FloatParams.GetCount();
  for (xiiUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    s << m_FloatParams[i].m_sName;
    s << m_FloatParams[i].m_Value;
  }
  s << m_ColorParams.GetCount();
  for (xiiUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    s << m_ColorParams[i].m_sName;
    s << m_ColorParams[i].m_Value;
  }

  // Version 3
  s << m_OnFinishedAction;

  // version 4
  s << m_bIgnoreOwnerRotation;

  // version 5
  s << m_SpawnDirection;

  /// \todo store effect state
}

void xiiParticleComponent::DeserializeComponent(xiiWorldReader& stream)
{
  auto&           s         = stream.GetStream();
  const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_hEffectResource;
  s >> m_bSpawnAtStart;

  // Version 1
  {
    bool bAutoRestart = false;
    s >> bAutoRestart;
  }

  s >> m_MinRestartDelay;
  s >> m_RestartDelayRange;
  s >> m_RestartTime;
  s >> m_uiRandomSeed;
  s >> m_sSharedInstanceName;

  if (uiVersion >= 2)
  {
    xiiUInt32 numFloats, numColors;

    s >> numFloats;
    m_FloatParams.SetCountUninitialized(numFloats);

    for (xiiUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      s >> m_FloatParams[i].m_sName;
      s >> m_FloatParams[i].m_Value;
    }

    m_bFloatParamsChanged = numFloats > 0;

    s >> numColors;
    m_ColorParams.SetCountUninitialized(numColors);

    for (xiiUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      s >> m_ColorParams[i].m_sName;
      s >> m_ColorParams[i].m_Value;
    }

    m_bColorParamsChanged = numColors > 0;
  }

  if (uiVersion >= 3)
  {
    s >> m_OnFinishedAction;
  }

  if (uiVersion >= 4)
  {
    s >> m_bIgnoreOwnerRotation;
  }

  if (uiVersion >= 5)
  {
    s >> m_SpawnDirection;
  }
}

bool xiiParticleComponent::StartEffect()
{
  // stop any previous effect
  m_EffectController.Invalidate();

  if (m_hEffectResource.IsValid())
  {
    xiiParticleWorldModule* pModule = GetWorld()->GetOrCreateModule<xiiParticleWorldModule>();

    m_EffectController.Create(m_hEffectResource, pModule, m_uiRandomSeed, m_sSharedInstanceName, this, m_FloatParams, m_ColorParams);

    UpdatePfxTransform();

    m_bFloatParamsChanged = false;
    m_bColorParamsChanged = false;

    return true;
  }

  return false;
}

void xiiParticleComponent::StopEffect()
{
  m_EffectController.Invalidate();
}

void xiiParticleComponent::InterruptEffect()
{
  m_EffectController.StopImmediate();
}

bool xiiParticleComponent::IsEffectActive() const
{
  return m_EffectController.IsAlive();
}


void xiiParticleComponent::OnMsgSetPlaying(xiiMsgSetPlaying& msg)
{
  if (msg.m_bPlay)
  {
    StartEffect();
  }
  else
  {
    StopEffect();
  }
}

void xiiParticleComponent::SetParticleEffect(const xiiParticleEffectResourceHandle& hEffect)
{
  m_EffectController.Invalidate();

  m_hEffectResource = hEffect;

  TriggerLocalBoundsUpdate();
}


void xiiParticleComponent::SetParticleEffectFile(const char* szFile)
{
  xiiParticleEffectResourceHandle hEffect;

  if (!xiiStringUtils::IsNullOrEmpty(szFile))
  {
    hEffect = xiiResourceManager::LoadResource<xiiParticleEffectResource>(szFile);
  }

  SetParticleEffect(hEffect);
}


const char* xiiParticleComponent::GetParticleEffectFile() const
{
  if (!m_hEffectResource.IsValid())
    return "";

  return m_hEffectResource.GetResourceID();
}


xiiResult xiiParticleComponent::GetLocalBounds(xiiBoundingBoxSphere& bounds, bool& bAlwaysVisible, xiiMsgUpdateLocalBounds& msg)
{
  if (m_EffectController.IsAlive())
  {
    xiiBoundingBoxSphere volume;
    volume.SetInvalid();

    m_EffectController.GetBoundingVolume(volume);

    if (volume.IsValid())
    {
      if (m_SpawnDirection != xiiBasisAxis::PositiveZ)
      {
        const xiiQuat qRot = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveZ, m_SpawnDirection);
        volume.Transform(qRot.GetAsMat4());
      }

      if (m_bIgnoreOwnerRotation)
      {
        volume.Transform((-GetOwner()->GetGlobalRotation()).GetAsMat4());
      }

      bounds = volume;
      return XII_SUCCESS;
    }
  }

  return XII_FAILURE;
}


void xiiParticleComponent::OnMsgExtractRenderData(xiiMsgExtractRenderData& msg) const
{
  // do not extract particles during shadow map rendering
  if (msg.m_pView->GetCameraUsageHint() == xiiCameraUsageHint::Shadow)
    return;

  m_EffectController.ExtractRenderData(msg, GetPfxTransform());
}

void xiiParticleComponent::OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg)
{
  xiiOnComponentFinishedAction2::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void xiiParticleComponent::Update()
{
  if (!m_EffectController.IsAlive() && m_bSpawnAtStart)
  {
    if (StartEffect())
    {
      m_bSpawnAtStart = false;

      if (m_EffectController.IsContinuousEffect())
      {
        if (m_bIfContinuousStopRightAway)
        {
          StopEffect();
        }
        else
        {
          m_bSpawnAtStart = true;
        }
      }
    }
  }

  if (!m_EffectController.IsAlive() && (m_OnFinishedAction == xiiOnComponentFinishedAction2::Restart))
  {
    const xiiTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (m_RestartTime == xiiTime())
    {
      const xiiTime tDiff = xiiTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(m_MinRestartDelay.GetSeconds(), m_RestartDelayRange.GetSeconds()));

      m_RestartTime = tNow + tDiff;
    }
    else if (m_RestartTime <= tNow)
    {
      m_RestartTime.SetZero();
      StartEffect();
    }
  }

  if (m_EffectController.IsAlive())
  {
    if (m_bFloatParamsChanged)
    {
      m_bFloatParamsChanged = false;

      for (xiiUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
      {
        const auto& e = m_FloatParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    if (m_bColorParamsChanged)
    {
      m_bColorParamsChanged = false;

      for (xiiUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
      {
        const auto& e = m_ColorParams[i];
        m_EffectController.SetParameter(e.m_sName, e.m_Value);
      }
    }

    m_EffectController.UpdateWindSamples();
  }
  else
  {
    xiiOnComponentFinishedAction2::HandleFinishedAction(this, m_OnFinishedAction);
  }
}

const xiiRangeView<const char*, xiiUInt32> xiiParticleComponent::GetParameters() const
{
  return xiiRangeView<const char*, xiiUInt32>([this]() -> xiiUInt32 { return 0; }, [this]() -> xiiUInt32 { return m_FloatParams.GetCount() + m_ColorParams.GetCount(); }, [this](xiiUInt32& it) { ++it; },
                                              [this](const xiiUInt32& it) -> const char* {
                                                if (it < m_FloatParams.GetCount())
                                                  return m_FloatParams[it].m_sName.GetData();
                                                else
                                                  return m_ColorParams[it - m_FloatParams.GetCount()].m_sName.GetData();
                                              });
}

void xiiParticleComponent::SetParameter(const char* szKey, const xiiVariant& var)
{
  const xiiTempHashedString th(szKey);
  if (var.CanConvertTo<float>())
  {
    float value = var.ConvertTo<float>();

    for (xiiUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
    {
      if (m_FloatParams[i].m_sName == th)
      {
        if (m_FloatParams[i].m_Value != value)
        {
          m_bFloatParamsChanged    = true;
          m_FloatParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bFloatParamsChanged = true;
    auto& e               = m_FloatParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }

  if (var.CanConvertTo<xiiColor>())
  {
    xiiColor value = var.ConvertTo<xiiColor>();

    for (xiiUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
    {
      if (m_ColorParams[i].m_sName == th)
      {
        if (m_ColorParams[i].m_Value != value)
        {
          m_bColorParamsChanged    = true;
          m_ColorParams[i].m_Value = value;
        }
        return;
      }
    }

    m_bColorParamsChanged = true;
    auto& e               = m_ColorParams.ExpandAndGetRef();
    e.m_sName.Assign(szKey);
    e.m_Value = value;

    return;
  }
}

void xiiParticleComponent::RemoveParameter(const char* szKey)
{
  const xiiTempHashedString th(szKey);

  for (xiiUInt32 i = 0; i < m_FloatParams.GetCount(); ++i)
  {
    if (m_FloatParams[i].m_sName == th)
    {
      m_FloatParams.RemoveAtAndSwap(i);
      return;
    }
  }

  for (xiiUInt32 i = 0; i < m_ColorParams.GetCount(); ++i)
  {
    if (m_ColorParams[i].m_sName == th)
    {
      m_ColorParams.RemoveAtAndSwap(i);
      return;
    }
  }
}

bool xiiParticleComponent::GetParameter(const char* szKey, xiiVariant& out_value) const
{
  const xiiTempHashedString th(szKey);

  for (const auto& e : m_FloatParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  for (const auto& e : m_ColorParams)
  {
    if (e.m_sName == th)
    {
      out_value = e.m_Value;
      return true;
    }
  }
  return false;
}

xiiTransform xiiParticleComponent::GetPfxTransform() const
{
  xiiTransform transform = GetOwner()->GetGlobalTransform();

  const xiiQuat qRot = xiiBasisAxis::GetBasisRotation(xiiBasisAxis::PositiveZ, m_SpawnDirection);

  if (m_bIgnoreOwnerRotation)
  {
    transform.m_qRotation = qRot;
  }
  else
  {
    transform.m_qRotation = transform.m_qRotation * qRot;
  }

  return transform;
}

void xiiParticleComponent::UpdatePfxTransform()
{
  m_EffectController.SetTransform(GetPfxTransform(), GetOwner()->GetVelocity());
}

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Components_ParticleComponent);
