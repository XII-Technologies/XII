#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Behavior/ParticleBehavior.h>
#include <ParticlePlugin/Emitter/ParticleEmitter.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Age.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_Volume.h>
#include <ParticlePlugin/Initializer/ParticleInitializer.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/ParticleType.h>
#include <ParticlePlugin/Type/Point/ParticleTypePoint.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiParticleSystemDescriptor, 2, xiiRTTIDefaultAllocator<xiiParticleSystemDescriptor>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Name", m_sName),
    XII_MEMBER_PROPERTY("Visible", m_bVisible)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("LifeTime", m_LifeTime)->AddAttributes(new xiiDefaultValueAttribute(xiiTime::Seconds(2)), new xiiClampValueAttribute(xiiTime::Seconds(0.0), xiiVariant())),
    XII_MEMBER_PROPERTY("LifeScaleParam", m_sLifeScaleParameter),
    XII_MEMBER_PROPERTY("OnDeathEvent", m_sOnDeathEvent),
    XII_ARRAY_MEMBER_PROPERTY("Emitters", m_EmitterFactories)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiMaxArraySizeAttribute(1)),
    XII_SET_ACCESSOR_PROPERTY("Initializers", GetInitializerFactories, AddInitializerFactory, RemoveInitializerFactory)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiPreventDuplicatesAttribute()),
    XII_SET_ACCESSOR_PROPERTY("Behaviors", GetBehaviorFactories, AddBehaviorFactory, RemoveBehaviorFactory)->AddFlags(xiiPropertyFlags::PointerOwner)->AddAttributes(new xiiPreventDuplicatesAttribute()),
    XII_SET_ACCESSOR_PROPERTY("Types", GetTypeFactories, AddTypeFactory, RemoveTypeFactory)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiParticleSystemDescriptor::xiiParticleSystemDescriptor()
{
  m_bVisible = true;
}

xiiParticleSystemDescriptor::~xiiParticleSystemDescriptor()
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();
}

void xiiParticleSystemDescriptor::ClearEmitters()
{
  for (auto pFactory : m_EmitterFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_EmitterFactories.Clear();
}

void xiiParticleSystemDescriptor::ClearInitializers()
{
  for (auto pFactory : m_InitializerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_InitializerFactories.Clear();
}

void xiiParticleSystemDescriptor::ClearBehaviors()
{
  for (auto pFactory : m_BehaviorFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_BehaviorFactories.Clear();
}

void xiiParticleSystemDescriptor::ClearTypes()
{
  for (auto pFactory : m_TypeFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_TypeFactories.Clear();
}

void xiiParticleSystemDescriptor::ClearFinalizers()
{
  for (auto pFactory : m_FinalizerFactories)
  {
    pFactory->GetDynamicRTTI()->GetAllocator()->Deallocate(pFactory);
  }

  m_FinalizerFactories.Clear();
}

void xiiParticleSystemDescriptor::SetupDefaultProcessors()
{
  // Age Behavior
  {
    xiiParticleFinalizerFactory_Age* pFactory =
      xiiParticleFinalizerFactory_Age::GetStaticRTTI()->GetAllocator()->Allocate<xiiParticleFinalizerFactory_Age>();
    pFactory->m_LifeTime            = m_LifeTime;
    pFactory->m_sOnDeathEvent       = m_sOnDeathEvent;
    pFactory->m_sLifeScaleParameter = m_sLifeScaleParameter;
    m_FinalizerFactories.PushBack(pFactory);
  }

  // Bounding Volume Update Behavior
  {
    xiiParticleFinalizerFactory_Volume* pFactory =
      xiiParticleFinalizerFactory_Volume::GetStaticRTTI()->GetAllocator()->Allocate<xiiParticleFinalizerFactory_Volume>();
    m_FinalizerFactories.PushBack(pFactory);
  }

  if (m_TypeFactories.IsEmpty())
  {
    xiiParticleTypePointFactory* pFactory = xiiParticleTypePointFactory::GetStaticRTTI()->GetAllocator()->Allocate<xiiParticleTypePointFactory>();
    m_TypeFactories.PushBack(pFactory);
  }

  xiiSet<const xiiRTTI*> finalizers;
  for (const auto* pFactory : m_InitializerFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_BehaviorFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const auto* pFactory : m_TypeFactories)
  {
    pFactory->QueryFinalizerDependencies(finalizers);
  }

  for (const xiiRTTI* pRtti : finalizers)
  {
    XII_ASSERT_DEBUG(
      pRtti->IsDerivedFrom<xiiParticleFinalizerFactory>(), "Invalid finalizer factory added as a dependency: '{0}'", pRtti->GetTypeName());
    XII_ASSERT_DEBUG(pRtti->GetAllocator()->CanAllocate(), "Finalizer factory cannot be allocated: '{0}'", pRtti->GetTypeName());

    m_FinalizerFactories.PushBack(pRtti->GetAllocator()->Allocate<xiiParticleFinalizerFactory>());
  }
}

enum class ParticleSystemVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,
  Version_3,
  Version_4, // added Types
  Version_5, // added default processors
  Version_6, // changed lifetime variance
  Version_7, // added life scale param

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


xiiTime xiiParticleSystemDescriptor::GetAvgLifetime() const
{
  xiiTime time = m_LifeTime.m_Value + m_LifeTime.m_Value * (m_LifeTime.m_fVariance * 2.0f / 3.0f);

  // we actively prevent values outside the [0;2] range for the life-time scale parameter, when it is applied
  // so this is the accurate worst case value
  // effects should be authored with the maximum lifetime, and at runtime the lifetime should only be scaled down
  if (!m_sLifeScaleParameter.IsEmpty())
  {
    time = time * 2.0f;
  }

  return time;
}

void xiiParticleSystemDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  const xiiUInt8 uiVersion = (int)ParticleSystemVersion::Version_Current;

  inout_stream << uiVersion;

  const xiiUInt32 uiNumEmitters     = m_EmitterFactories.GetCount();
  const xiiUInt32 uiNumInitializers = m_InitializerFactories.GetCount();
  const xiiUInt32 uiNumBehaviors    = m_BehaviorFactories.GetCount();
  const xiiUInt32 uiNumTypes        = m_TypeFactories.GetCount();

  xiiUInt32 uiMaxParticles = 0;
  inout_stream << m_bVisible;
  inout_stream << uiMaxParticles;
  inout_stream << m_LifeTime.m_Value;
  inout_stream << m_LifeTime.m_fVariance;
  inout_stream << m_sOnDeathEvent;
  inout_stream << m_sLifeScaleParameter;
  inout_stream << uiNumEmitters;
  inout_stream << uiNumInitializers;
  inout_stream << uiNumBehaviors;
  inout_stream << uiNumTypes;

  for (auto pEmitter : m_EmitterFactories)
  {
    inout_stream << pEmitter->GetDynamicRTTI()->GetTypeName();

    pEmitter->Save(inout_stream);
  }

  for (auto pInitializer : m_InitializerFactories)
  {
    inout_stream << pInitializer->GetDynamicRTTI()->GetTypeName();

    pInitializer->Save(inout_stream);
  }

  for (auto pBehavior : m_BehaviorFactories)
  {
    inout_stream << pBehavior->GetDynamicRTTI()->GetTypeName();

    pBehavior->Save(inout_stream);
  }

  for (auto pType : m_TypeFactories)
  {
    inout_stream << pType->GetDynamicRTTI()->GetTypeName();

    pType->Save(inout_stream);
  }
}


void xiiParticleSystemDescriptor::Load(xiiStreamReader& inout_stream)
{
  ClearEmitters();
  ClearInitializers();
  ClearBehaviors();
  ClearFinalizers();
  ClearTypes();

  xiiUInt8 uiVersion = 0;
  inout_stream >> uiVersion;
  XII_ASSERT_DEV(uiVersion <= (int)ParticleSystemVersion::Version_Current, "Unknown particle template version {0}", uiVersion);

  xiiUInt32 uiNumEmitters     = 0;
  xiiUInt32 uiNumInitializers = 0;
  xiiUInt32 uiNumBehaviors    = 0;
  xiiUInt32 uiNumTypes        = 0;

  if (uiVersion >= 3)
  {
    inout_stream >> m_bVisible;
  }

  if (uiVersion >= 2)
  {
    // now unused
    xiiUInt32 uiMaxParticles = 0;
    inout_stream >> uiMaxParticles;
  }

  if (uiVersion >= 5)
  {
    inout_stream >> m_LifeTime.m_Value;
    inout_stream >> m_LifeTime.m_fVariance;
    inout_stream >> m_sOnDeathEvent;
  }

  if (uiVersion >= 7)
  {
    inout_stream >> m_sLifeScaleParameter;
  }

  inout_stream >> uiNumEmitters;

  if (uiVersion >= 2)
  {
    inout_stream >> uiNumInitializers;
  }

  inout_stream >> uiNumBehaviors;

  if (uiVersion >= 4)
  {
    inout_stream >> uiNumTypes;
  }

  m_EmitterFactories.SetCountUninitialized(uiNumEmitters);
  m_InitializerFactories.SetCountUninitialized(uiNumInitializers);
  m_BehaviorFactories.SetCountUninitialized(uiNumBehaviors);
  m_TypeFactories.SetCountUninitialized(uiNumTypes);

  xiiStringBuilder sType;

  for (auto& pEmitter : m_EmitterFactories)
  {
    inout_stream >> sType;

    const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
    XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown emitter factory type '{0}'", sType);

    pEmitter = pRtti->GetAllocator()->Allocate<xiiParticleEmitterFactory>();

    pEmitter->Load(inout_stream);
  }

  if (uiVersion >= 2)
  {
    for (auto& pInitializer : m_InitializerFactories)
    {
      inout_stream >> sType;

      const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
      XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown initializer factory type '{0}'", sType);

      pInitializer = pRtti->GetAllocator()->Allocate<xiiParticleInitializerFactory>();

      pInitializer->Load(inout_stream);
    }
  }

  for (auto& pBehavior : m_BehaviorFactories)
  {
    inout_stream >> sType;

    const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
    XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown behavior factory type '{0}'", sType);

    pBehavior = pRtti->GetAllocator()->Allocate<xiiParticleBehaviorFactory>();

    pBehavior->Load(inout_stream);
  }

  if (uiVersion >= 4)
  {
    for (auto& pType : m_TypeFactories)
    {
      inout_stream >> sType;

      const xiiRTTI* pRtti = xiiRTTI::FindTypeByName(sType);
      XII_ASSERT_DEBUG(pRtti != nullptr, "Unknown type factory type '{0}'", sType);

      pType = pRtti->GetAllocator()->Allocate<xiiParticleTypeFactory>();

      pType->Load(inout_stream);
    }
  }

  SetupDefaultProcessors();
}

//////////////////////////////////////////////////////////////////////////

class xiiParticleSystemDescriptor_1_2 : public xiiGraphPatch
{
public:
  xiiParticleSystemDescriptor_1_2() :
    xiiGraphPatch("xiiParticleSystemDescriptor", 2)
  {
  }

  virtual void Patch(xiiGraphPatchContext& ref_context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("LifeTime").IgnoreResult();
  }
};

xiiParticleSystemDescriptor_1_2 g_xiiParticleSystemDescriptor_1_2;

XII_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_System_ParticleSystemDescriptor);
