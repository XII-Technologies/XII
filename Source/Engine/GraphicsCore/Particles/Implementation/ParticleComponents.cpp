/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/ResourceManager/Implementation/ResourceHandleReflection.h>
#include <Core/World/SpatialData.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshResource.h>
#include <GraphicsCore/Particles/ParticleComponents.h>
#include <GraphicsCore/Textures/Texture2DResource.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleEmitterShape, 1)
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Point),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Sphere),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Box),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Cylinder),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Cone),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Surface),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::Volume),
  XII_ENUM_CONSTANT(xiiParticleEmitterShape::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleEmitterMode, 1)
  XII_ENUM_CONSTANT(xiiParticleEmitterMode::Continuous),
  XII_ENUM_CONSTANT(xiiParticleEmitterMode::Burst),
  XII_ENUM_CONSTANT(xiiParticleEmitterMode::Distance),
  XII_ENUM_CONSTANT(xiiParticleEmitterMode::Manual),
  XII_ENUM_CONSTANT(xiiParticleEmitterMode::EventDriven),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleForceFieldType, 1)
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Directional),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::PointAttractor),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::PointRepulsor),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Vortex),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Drag),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Turbulence),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Brownian),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::VectorField),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Electric),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::Magnetic),
  XII_ENUM_CONSTANT(xiiParticleForceFieldType::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleColliderShape, 1)
  XII_ENUM_CONSTANT(xiiParticleColliderShape::Plane),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::Sphere),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::Box),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::Capsule),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::Mesh),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::SignedDistanceField),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::DepthBuffer),
  XII_ENUM_CONSTANT(xiiParticleColliderShape::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleRendererType, 1)
  XII_ENUM_CONSTANT(xiiParticleRendererType::Billboard),
  XII_ENUM_CONSTANT(xiiParticleRendererType::StretchedBillboard),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Mesh),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Ribbon),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Trail),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Point),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Volume),
  XII_ENUM_CONSTANT(xiiParticleRendererType::Impostor),
  XII_ENUM_CONSTANT(xiiParticleRendererType::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleSimulationDomainShape, 1)
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::Box),
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::Sphere),
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::Cylinder),
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::Slab),
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::Infinite),
  XII_ENUM_CONSTANT(xiiParticleSimulationDomainShape::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiParticleSimulationDomainFlags, 1)
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::None),
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::PeriodicX),
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::PeriodicY),
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::PeriodicZ),
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::KillOutside),
  XII_BITFLAGS_CONSTANT(xiiParticleSimulationDomainFlags::WrapPosition),
XII_END_STATIC_REFLECTED_BITFLAGS;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleMolecularIntegrator, 1)
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::VelocityVerlet),
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::Verlet),
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::Leapfrog),
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::Langevin),
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::Brownian),
  XII_ENUM_CONSTANT(xiiParticleMolecularIntegrator::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleMolecularPotential, 1)
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::LennardJones),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::Coulomb),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::Morse),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::HarmonicBond),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::Angle),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::Dihedral),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::Tabulated),
  XII_ENUM_CONSTANT(xiiParticleMolecularPotential::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiParticleMolecularEnsemble, 1)
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::NVE),
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::NVT),
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::NPT),
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::Brownian),
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::GrandCanonical),
  XII_ENUM_CONSTANT(xiiParticleMolecularEnsemble::CustomGraph),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_COMPONENT_TYPE(xiiParticleEmitterComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_ACCESSOR_PROPERTY("Shape", xiiParticleEmitterShape, GetShape, SetShape),
    XII_ENUM_MEMBER_PROPERTY("Mode", xiiParticleEmitterMode, m_Mode),
    XII_MEMBER_PROPERTY("MaxParticles", m_uiMaxParticles)->AddAttributes(new xiiClampValueAttribute(0U, 16U * 1024U * 1024U)),
    XII_MEMBER_PROPERTY("SpawnRate", m_fSpawnRate)->AddAttributes(new xiiDefaultValueAttribute(1000.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("BurstCount", m_uiBurstCount)->AddAttributes(new xiiDefaultValueAttribute(1024U), new xiiClampValueAttribute(0U, 16U * 1024U * 1024U)),
    XII_MEMBER_PROPERTY("Lifetime", m_fLifetime)->AddAttributes(new xiiDefaultValueAttribute(5.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("InitialSpeed", m_fInitialSpeed)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("BoxExtents", GetBoxExtents, SetBoxExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant())),
    XII_ACCESSOR_PROPERTY("ConeAngle", GetConeAngle, SetConeAngle)->AddAttributes(new xiiDefaultValueAttribute(25.0f), new xiiClampValueAttribute(0.0f, 179.0f)),
    XII_MEMBER_PROPERTY("RandomSeed", m_uiRandomSeed),
    XII_MEMBER_PROPERTY("SurfaceOnly", m_bSurfaceOnly),
    XII_MEMBER_PROPERTY("InheritOwnerVelocity", m_bInheritOwnerVelocity),
    XII_MEMBER_PROPERTY("Deterministic", m_bDeterministic),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiBoxManipulatorAttribute("BoxExtents", 1.0f, true),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleForceFieldComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_MEMBER_PROPERTY("Type", xiiParticleForceFieldType, m_Type),
    XII_MEMBER_PROPERTY("Direction", m_vDirection)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(0.0f, 0.0f, 1.0f))),
    XII_MEMBER_PROPERTY("Strength", m_fStrength)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Falloff", m_fFalloff)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, 16.0f)),
    XII_MEMBER_PROPERTY("Drag", m_fDrag)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Frequency", m_fFrequency)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Seed", m_fSeed),
    XII_MEMBER_PROPERTY("NormalizeDirection", m_bNormalizeDirection),
    XII_MEMBER_PROPERTY("AffectsMolecularParticles", m_bAffectsMolecularParticles),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
    new xiiSphereManipulatorAttribute("Radius"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleColliderComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_ACCESSOR_PROPERTY("Shape", xiiParticleColliderShape, GetShape, SetShape),
    XII_RESOURCE_MEMBER_PROPERTY("CollisionMesh", m_hCollisionMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh", xiiDependencyFlags::Package)),
    XII_RESOURCE_MEMBER_PROPERTY("SignedDistanceField", m_hSignedDistanceField)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Texture_2D", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f)), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant())),
    XII_MEMBER_PROPERTY("CapsuleHalfHeight", m_fCapsuleHalfHeight)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Restitution", m_fRestitution)->AddAttributes(new xiiDefaultValueAttribute(0.5f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("Friction", m_fFriction)->AddAttributes(new xiiDefaultValueAttribute(0.2f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("Thickness", m_fThickness)->AddAttributes(new xiiDefaultValueAttribute(0.01f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("KillOnContact", m_bKillOnContact),
    XII_MEMBER_PROPERTY("Invert", m_bInvert),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleRendererComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_MEMBER_PROPERTY("RendererType", xiiParticleRendererType, m_RendererType),
    XII_RESOURCE_MEMBER_PROPERTY("Material", m_hMaterial)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Material", xiiDependencyFlags::Package)),
    XII_RESOURCE_MEMBER_PROPERTY("Mesh", m_hMesh)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Mesh", xiiDependencyFlags::Package)),
    XII_MEMBER_PROPERTY("Tint", m_Tint)->AddAttributes(new xiiDefaultValueAttribute(xiiColor::White)),
    XII_MEMBER_PROPERTY("Size", m_fSize)->AddAttributes(new xiiDefaultValueAttribute(0.05f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Stretch", m_fStretch)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("SoftParticleDistance", m_fSoftParticleDistance)->AddAttributes(new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("SortPriority", m_uiSortPriority)->AddAttributes(new xiiDefaultValueAttribute(128)),
    XII_MEMBER_PROPERTY("SortByDepth", m_bSortByDepth),
    XII_MEMBER_PROPERTY("ReceiveLighting", m_bReceiveLighting),
    XII_MEMBER_PROPERTY("CastShadows", m_bCastShadows),
    XII_MEMBER_PROPERTY("MotionVectors", m_bMotionVectors),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleSimulationDomainComponent, 1, xiiComponentMode::Dynamic)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_ACCESSOR_PROPERTY("Shape", xiiParticleSimulationDomainShape, GetShape, SetShape),
    XII_BITFLAGS_MEMBER_PROPERTY("Flags", xiiParticleSimulationDomainFlags, m_Flags),
    XII_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(20.0f)), new xiiClampValueAttribute(xiiVec3::MakeZero(), xiiVariant())),
    XII_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0001f, xiiVariant())),
    XII_MEMBER_PROPERTY("MaxGridCells", m_uiMaxGridCells)->AddAttributes(new xiiDefaultValueAttribute(1024U * 1024U), new xiiClampValueAttribute(1U, 16U * 1024U * 1024U)),
    XII_MEMBER_PROPERTY("WorldSpace", m_bWorldSpace),
    XII_MEMBER_PROPERTY("AdaptiveBounds", m_bAdaptiveBounds),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgUpdateLocalBounds, OnMsgUpdateLocalBounds),
  }
  XII_END_MESSAGEHANDLERS;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;

XII_BEGIN_COMPONENT_TYPE(xiiParticleMolecularDynamicsComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SystemBinding", m_sSystemBinding),
    XII_ENUM_MEMBER_PROPERTY("Integrator", xiiParticleMolecularIntegrator, m_Integrator),
    XII_ENUM_MEMBER_PROPERTY("Potential", xiiParticleMolecularPotential, m_Potential),
    XII_ENUM_MEMBER_PROPERTY("Ensemble", xiiParticleMolecularEnsemble, m_Ensemble),
    XII_MEMBER_PROPERTY("TimeStep", m_fTimeStep)->AddAttributes(new xiiDefaultValueAttribute(0.001f), new xiiClampValueAttribute(0.0f, 1.0f)),
    XII_MEMBER_PROPERTY("SubSteps", m_uiSubSteps)->AddAttributes(new xiiDefaultValueAttribute(1U), new xiiClampValueAttribute(1U, 64U)),
    XII_MEMBER_PROPERTY("CutoffRadius", m_fCutoffRadius)->AddAttributes(new xiiDefaultValueAttribute(2.5f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("NeighborSkin", m_fNeighborSkin)->AddAttributes(new xiiDefaultValueAttribute(0.3f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("TargetTemperature", m_fTargetTemperature)->AddAttributes(new xiiDefaultValueAttribute(300.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("TargetPressure", m_fTargetPressure)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("ParticleMass", m_fParticleMass)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.000001f, xiiVariant())),
    XII_MEMBER_PROPERTY("LennardJonesEpsilon", m_fLennardJonesEpsilon)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("LennardJonesSigma", m_fLennardJonesSigma)->AddAttributes(new xiiDefaultValueAttribute(1.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("CoulombScale", m_fCoulombScale)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_MEMBER_PROPERTY("UseHalfNeighborList", m_bUseHalfNeighborList),
    XII_MEMBER_PROPERTY("Deterministic", m_bDeterministic),
    XII_MEMBER_PROPERTY("EnableReadback", m_bEnableReadback),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("RenderWorld/Particles"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE;
// clang-format on

namespace
{
  static xiiSpatialData::Category GetParticleAuthoringCategory()
  {
    static xiiSpatialData::Category s_Category = xiiSpatialData::RegisterCategory("ParticleAuthoring", xiiSpatialData::Flags::FrequentChanges);
    return s_Category;
  }

  static xiiBoundingBoxSphere MakeSphereBounds(float fRadius)
  {
    const float fSafeRadius = xiiMath::Max(0.001f, fRadius);
    return xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), xiiVec3(fSafeRadius), fSafeRadius);
  }

  static xiiBoundingBoxSphere MakeBoxBounds(xiiVec3 vExtents)
  {
    const xiiVec3 vHalfExtents = (vExtents * 0.5f).CompMax(xiiVec3(0.001f));
    return xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), vHalfExtents, vHalfExtents.GetLength());
  }

  static xiiBoundingBoxSphere MakeCapsuleBounds(float fRadius, float fHalfHeight)
  {
    const float   fSafeRadius     = xiiMath::Max(0.001f, fRadius);
    const float   fSafeHalfHeight = xiiMath::Max(0.0f, fHalfHeight);
    const xiiVec3 vHalfExtents(fSafeRadius, fSafeRadius, fSafeRadius + fSafeHalfHeight);
    return xiiBoundingBoxSphere::MakeFromCenterExtents(xiiVec3::MakeZero(), vHalfExtents, vHalfExtents.GetLength());
  }
} // namespace

xiiParticleEmitterComponent::xiiParticleEmitterComponent()  = default;
xiiParticleEmitterComponent::~xiiParticleEmitterComponent() = default;

void xiiParticleEmitterComponent::SetShape(xiiEnum<xiiParticleEmitterShape> shape)
{
  if (m_Shape == shape)
    return;

  m_Shape = shape;
  UpdateLocalBounds();
}

void xiiParticleEmitterComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(0.0f, fRadius);
  UpdateLocalBounds();
}

void xiiParticleEmitterComponent::SetBoxExtents(xiiVec3 vExtents)
{
  m_vBoxExtents = vExtents.CompMax(xiiVec3::MakeZero());
  UpdateLocalBounds();
}

void xiiParticleEmitterComponent::OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  if (m_Shape == xiiParticleEmitterShape::Box || m_Shape == xiiParticleEmitterShape::Volume)
  {
    ref_msg.AddBounds(MakeBoxBounds(m_vBoxExtents), GetParticleAuthoringCategory());
  }
  else
  {
    ref_msg.AddBounds(MakeSphereBounds(m_fRadius), GetParticleAuthoringCategory());
  }
}

void xiiParticleEmitterComponent::UpdateLocalBounds()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiParticleEmitterComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_Shape;
  s << m_Mode;
  s << m_uiMaxParticles;
  s << m_uiBurstCount;
  s << m_uiRandomSeed;
  s << m_fSpawnRate;
  s << m_fLifetime;
  s << m_fInitialSpeed;
  s << m_fRadius;
  s << m_fConeAngleDegree;
  s << m_vBoxExtents;
  s << m_bSurfaceOnly;
  s << m_bInheritOwnerVelocity;
  s << m_bDeterministic;
}

void xiiParticleEmitterComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_Shape;
  s >> m_Mode;
  s >> m_uiMaxParticles;
  s >> m_uiBurstCount;
  s >> m_uiRandomSeed;
  s >> m_fSpawnRate;
  s >> m_fLifetime;
  s >> m_fInitialSpeed;
  s >> m_fRadius;
  s >> m_fConeAngleDegree;
  s >> m_vBoxExtents;
  s >> m_bSurfaceOnly;
  s >> m_bInheritOwnerVelocity;
  s >> m_bDeterministic;
}

xiiParticleForceFieldComponent::xiiParticleForceFieldComponent()  = default;
xiiParticleForceFieldComponent::~xiiParticleForceFieldComponent() = default;

void xiiParticleForceFieldComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(0.0f, fRadius);
  UpdateLocalBounds();
}

void xiiParticleForceFieldComponent::OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  ref_msg.AddBounds(MakeSphereBounds(m_fRadius), GetParticleAuthoringCategory());
}

void xiiParticleForceFieldComponent::UpdateLocalBounds()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiParticleForceFieldComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_Type;
  s << m_vDirection;
  s << m_fStrength;
  s << m_fRadius;
  s << m_fFalloff;
  s << m_fDrag;
  s << m_fFrequency;
  s << m_fSeed;
  s << m_bNormalizeDirection;
  s << m_bAffectsMolecularParticles;
}

void xiiParticleForceFieldComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_Type;
  s >> m_vDirection;
  s >> m_fStrength;
  s >> m_fRadius;
  s >> m_fFalloff;
  s >> m_fDrag;
  s >> m_fFrequency;
  s >> m_fSeed;
  s >> m_bNormalizeDirection;
  s >> m_bAffectsMolecularParticles;
}

xiiParticleColliderComponent::xiiParticleColliderComponent()  = default;
xiiParticleColliderComponent::~xiiParticleColliderComponent() = default;

void xiiParticleColliderComponent::SetShape(xiiEnum<xiiParticleColliderShape> shape)
{
  if (m_Shape == shape)
    return;

  m_Shape = shape;
  UpdateLocalBounds();
}

void xiiParticleColliderComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(0.0f, fRadius);
  UpdateLocalBounds();
}

void xiiParticleColliderComponent::SetExtents(xiiVec3 vExtents)
{
  m_vExtents = vExtents.CompMax(xiiVec3::MakeZero());
  UpdateLocalBounds();
}

void xiiParticleColliderComponent::OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  if (m_Shape == xiiParticleColliderShape::Sphere)
  {
    ref_msg.AddBounds(MakeSphereBounds(m_fRadius), GetParticleAuthoringCategory());
  }
  else if (m_Shape == xiiParticleColliderShape::Capsule)
  {
    ref_msg.AddBounds(MakeCapsuleBounds(m_fRadius, m_fCapsuleHalfHeight), GetParticleAuthoringCategory());
  }
  else
  {
    ref_msg.AddBounds(MakeBoxBounds(m_vExtents), GetParticleAuthoringCategory());
  }
}

void xiiParticleColliderComponent::UpdateLocalBounds()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiParticleColliderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_Shape;
  s << m_hCollisionMesh;
  s << m_hSignedDistanceField;
  s << m_uiCollisionLayer;
  s << m_fRadius;
  s << m_fCapsuleHalfHeight;
  s << m_vExtents;
  s << m_fRestitution;
  s << m_fFriction;
  s << m_fThickness;
  s << m_bKillOnContact;
  s << m_bInvert;
}

void xiiParticleColliderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_Shape;
  s >> m_hCollisionMesh;
  s >> m_hSignedDistanceField;
  s >> m_uiCollisionLayer;
  s >> m_fRadius;
  s >> m_fCapsuleHalfHeight;
  s >> m_vExtents;
  s >> m_fRestitution;
  s >> m_fFriction;
  s >> m_fThickness;
  s >> m_bKillOnContact;
  s >> m_bInvert;
}

xiiParticleRendererComponent::xiiParticleRendererComponent()  = default;
xiiParticleRendererComponent::~xiiParticleRendererComponent() = default;

void xiiParticleRendererComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_RendererType;
  s << m_hMaterial;
  s << m_hMesh;
  s << m_Tint;
  s << m_fSize;
  s << m_fStretch;
  s << m_fSoftParticleDistance;
  s << m_uiSortPriority;
  s << m_bSortByDepth;
  s << m_bReceiveLighting;
  s << m_bCastShadows;
  s << m_bMotionVectors;
}

void xiiParticleRendererComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_RendererType;
  s >> m_hMaterial;
  s >> m_hMesh;
  s >> m_Tint;
  s >> m_fSize;
  s >> m_fStretch;
  s >> m_fSoftParticleDistance;
  s >> m_uiSortPriority;
  s >> m_bSortByDepth;
  s >> m_bReceiveLighting;
  s >> m_bCastShadows;
  s >> m_bMotionVectors;
}

xiiParticleSimulationDomainComponent::xiiParticleSimulationDomainComponent()  = default;
xiiParticleSimulationDomainComponent::~xiiParticleSimulationDomainComponent() = default;

void xiiParticleSimulationDomainComponent::SetShape(xiiEnum<xiiParticleSimulationDomainShape> shape)
{
  if (m_Shape == shape)
    return;

  m_Shape = shape;
  UpdateLocalBounds();
}

void xiiParticleSimulationDomainComponent::SetRadius(float fRadius)
{
  m_fRadius = xiiMath::Max(0.0f, fRadius);
  UpdateLocalBounds();
}

void xiiParticleSimulationDomainComponent::SetExtents(xiiVec3 vExtents)
{
  m_vExtents = vExtents.CompMax(xiiVec3::MakeZero());
  UpdateLocalBounds();
}

void xiiParticleSimulationDomainComponent::OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const
{
  if (m_Shape == xiiParticleSimulationDomainShape::Infinite)
  {
    return;
  }

  if (m_Shape == xiiParticleSimulationDomainShape::Sphere)
  {
    ref_msg.AddBounds(MakeSphereBounds(m_fRadius), GetParticleAuthoringCategory());
  }
  else
  {
    ref_msg.AddBounds(MakeBoxBounds(m_vExtents), GetParticleAuthoringCategory());
  }
}

void xiiParticleSimulationDomainComponent::UpdateLocalBounds()
{
  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void xiiParticleSimulationDomainComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_Shape;
  s << m_Flags;
  s << m_vExtents;
  s << m_fRadius;
  s << m_fCellSize;
  s << m_uiMaxGridCells;
  s << m_bWorldSpace;
  s << m_bAdaptiveBounds;
}

void xiiParticleSimulationDomainComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_Shape;
  s >> m_Flags;
  s >> m_vExtents;
  s >> m_fRadius;
  s >> m_fCellSize;
  s >> m_uiMaxGridCells;
  s >> m_bWorldSpace;
  s >> m_bAdaptiveBounds;
}

xiiParticleMolecularDynamicsComponent::xiiParticleMolecularDynamicsComponent()  = default;
xiiParticleMolecularDynamicsComponent::~xiiParticleMolecularDynamicsComponent() = default;

void xiiParticleMolecularDynamicsComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  xiiStreamWriter& s = inout_stream.GetStream();

  s << m_sSystemBinding;
  s << m_Integrator;
  s << m_Potential;
  s << m_Ensemble;
  s << m_fTimeStep;
  s << m_uiSubSteps;
  s << m_fCutoffRadius;
  s << m_fNeighborSkin;
  s << m_fTargetTemperature;
  s << m_fTargetPressure;
  s << m_fParticleMass;
  s << m_fLennardJonesEpsilon;
  s << m_fLennardJonesSigma;
  s << m_fCoulombScale;
  s << m_bUseHalfNeighborList;
  s << m_bDeterministic;
  s << m_bEnableReadback;
}

void xiiParticleMolecularDynamicsComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  xiiStreamReader& s = inout_stream.GetStream();

  s >> m_sSystemBinding;
  s >> m_Integrator;
  s >> m_Potential;
  s >> m_Ensemble;
  s >> m_fTimeStep;
  s >> m_uiSubSteps;
  s >> m_fCutoffRadius;
  s >> m_fNeighborSkin;
  s >> m_fTargetTemperature;
  s >> m_fTargetPressure;
  s >> m_fParticleMass;
  s >> m_fLennardJonesEpsilon;
  s >> m_fLennardJonesSigma;
  s >> m_fCoulombScale;
  s >> m_bUseHalfNeighborList;
  s >> m_bDeterministic;
  s >> m_bEnableReadback;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Particles_Implementation_ParticleComponents);
