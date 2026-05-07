/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/World/Component.h>
#include <Foundation/Math/BoundingBoxSphere.h>
#include <Foundation/Math/Color.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/Declarations.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

struct xiiMsgUpdateLocalBounds;

using xiiParticleEmitterComponentManager           = xiiComponentManager<class xiiParticleEmitterComponent, xiiBlockStorageType::FreeList>;
using xiiParticleForceFieldComponentManager        = xiiComponentManager<class xiiParticleForceFieldComponent, xiiBlockStorageType::FreeList>;
using xiiParticleColliderComponentManager          = xiiComponentManager<class xiiParticleColliderComponent, xiiBlockStorageType::FreeList>;
using xiiParticleRendererComponentManager          = xiiComponentManager<class xiiParticleRendererComponent, xiiBlockStorageType::FreeList>;
using xiiParticleSimulationDomainComponentManager  = xiiComponentManager<class xiiParticleSimulationDomainComponent, xiiBlockStorageType::FreeList>;
using xiiParticleMolecularDynamicsComponentManager = xiiComponentManager<class xiiParticleMolecularDynamicsComponent, xiiBlockStorageType::FreeList>;

struct XII_GRAPHICSCORE_DLL xiiParticleEmitterShape
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Point = 0U,
    Sphere,
    Box,
    Cylinder,
    Cone,
    Surface,
    Volume,
    CustomGraph,

    ENUM_COUNT,

    Default = Point
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleEmitterShape);

struct XII_GRAPHICSCORE_DLL xiiParticleEmitterMode
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Continuous = 0U,
    Burst,
    Distance,
    Manual,
    EventDriven,

    ENUM_COUNT,

    Default = Continuous
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleEmitterMode);

struct XII_GRAPHICSCORE_DLL xiiParticleForceFieldType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Directional = 0U,
    PointAttractor,
    PointRepulsor,
    Vortex,
    Drag,
    Turbulence,
    Brownian,
    VectorField,
    Electric,
    Magnetic,
    CustomGraph,

    ENUM_COUNT,

    Default = Directional
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleForceFieldType);

struct XII_GRAPHICSCORE_DLL xiiParticleColliderShape
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Plane = 0U,
    Sphere,
    Box,
    Capsule,
    Mesh,
    SignedDistanceField,
    DepthBuffer,
    CustomGraph,

    ENUM_COUNT,

    Default = Plane
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleColliderShape);

struct XII_GRAPHICSCORE_DLL xiiParticleRendererType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Billboard = 0U,
    StretchedBillboard,
    Mesh,
    Ribbon,
    Trail,
    Point,
    Volume,
    Impostor,
    CustomGraph,

    ENUM_COUNT,

    Default = Billboard
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleRendererType);

struct XII_GRAPHICSCORE_DLL xiiParticleSimulationDomainShape
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Box = 0U,
    Sphere,
    Cylinder,
    Slab,
    Infinite,
    CustomGraph,

    ENUM_COUNT,

    Default = Box
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSimulationDomainShape);

struct XII_GRAPHICSCORE_DLL xiiParticleSimulationDomainFlags
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    None         = 0U,
    PeriodicX    = XII_BIT(0),
    PeriodicY    = XII_BIT(1),
    PeriodicZ    = XII_BIT(2),
    KillOutside  = XII_BIT(3),
    WrapPosition = XII_BIT(4),

    Default = KillOutside
  };

  struct Bits
  {
    StorageType PeriodicX : 1;
    StorageType PeriodicY : 1;
    StorageType PeriodicZ : 1;
    StorageType KillOutside : 1;
    StorageType WrapPosition : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiParticleSimulationDomainFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleSimulationDomainFlags);

struct XII_GRAPHICSCORE_DLL xiiParticleMolecularIntegrator
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    VelocityVerlet = 0U,
    Verlet,
    Leapfrog,
    Langevin,
    Brownian,
    CustomGraph,

    ENUM_COUNT,

    Default = VelocityVerlet
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleMolecularIntegrator);

struct XII_GRAPHICSCORE_DLL xiiParticleMolecularPotential
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    LennardJones = 0U,
    Coulomb,
    Morse,
    HarmonicBond,
    Angle,
    Dihedral,
    Tabulated,
    CustomGraph,

    ENUM_COUNT,

    Default = LennardJones
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleMolecularPotential);

struct XII_GRAPHICSCORE_DLL xiiParticleMolecularEnsemble
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    NVE = 0U,
    NVT,
    NPT,
    Brownian,
    GrandCanonical,
    CustomGraph,

    ENUM_COUNT,

    Default = NVE
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiParticleMolecularEnsemble);

class XII_GRAPHICSCORE_DLL xiiParticleEmitterComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleEmitterComponent, xiiComponent, xiiParticleEmitterComponentManager);

public:
  xiiParticleEmitterComponent();
  ~xiiParticleEmitterComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void                             SetShape(xiiEnum<xiiParticleEmitterShape> shape);
  xiiEnum<xiiParticleEmitterShape> GetShape() const { return m_Shape; }
  void                             SetRadius(float fRadius);
  float                            GetRadius() const { return m_fRadius; }
  void                             SetBoxExtents(xiiVec3 vExtents);
  xiiVec3                          GetBoxExtents() const { return m_vBoxExtents; }
  void                             SetConeAngle(float fAngleDegree) { m_fConeAngleDegree = xiiMath::Clamp(fAngleDegree, 0.0f, 179.0f); }
  float                            GetConeAngle() const { return m_fConeAngleDegree; }

protected:
  void OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

private:
  void UpdateLocalBounds();

private:
  xiiHashedString                  m_sSystemBinding;
  xiiEnum<xiiParticleEmitterShape> m_Shape;
  xiiEnum<xiiParticleEmitterMode>  m_Mode;
  xiiUInt32                        m_uiMaxParticles        = 0U;
  xiiUInt32                        m_uiBurstCount          = 1024U;
  xiiUInt32                        m_uiRandomSeed          = 0U;
  float                            m_fSpawnRate            = 1000.0f;
  float                            m_fLifetime             = 5.0f;
  float                            m_fInitialSpeed         = 1.0f;
  float                            m_fRadius               = 1.0f;
  float                            m_fConeAngleDegree      = 25.0f;
  xiiVec3                          m_vBoxExtents           = xiiVec3(1.0f);
  bool                             m_bSurfaceOnly          = false;
  bool                             m_bInheritOwnerVelocity = false;
  bool                             m_bDeterministic        = false;
};

class XII_GRAPHICSCORE_DLL xiiParticleForceFieldComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleForceFieldComponent, xiiComponent, xiiParticleForceFieldComponentManager);

public:
  xiiParticleForceFieldComponent();
  ~xiiParticleForceFieldComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void  SetRadius(float fRadius);
  float GetRadius() const { return m_fRadius; }

protected:
  void OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

private:
  void UpdateLocalBounds();

private:
  xiiHashedString                    m_sSystemBinding;
  xiiEnum<xiiParticleForceFieldType> m_Type;
  xiiVec3                            m_vDirection                 = xiiVec3(0.0f, 0.0f, 1.0f);
  float                              m_fStrength                  = 1.0f;
  float                              m_fRadius                    = 10.0f;
  float                              m_fFalloff                   = 1.0f;
  float                              m_fDrag                      = 0.0f;
  float                              m_fFrequency                 = 1.0f;
  float                              m_fSeed                      = 0.0f;
  bool                               m_bNormalizeDirection        = true;
  bool                               m_bAffectsMolecularParticles = true;
};

class XII_GRAPHICSCORE_DLL xiiParticleColliderComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleColliderComponent, xiiComponent, xiiParticleColliderComponentManager);

public:
  xiiParticleColliderComponent();
  ~xiiParticleColliderComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void                              SetShape(xiiEnum<xiiParticleColliderShape> shape);
  xiiEnum<xiiParticleColliderShape> GetShape() const { return m_Shape; }
  void                              SetRadius(float fRadius);
  float                             GetRadius() const { return m_fRadius; }
  void                              SetExtents(xiiVec3 vExtents);
  xiiVec3                           GetExtents() const { return m_vExtents; }

protected:
  void OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

private:
  void UpdateLocalBounds();

private:
  xiiHashedString                   m_sSystemBinding;
  xiiEnum<xiiParticleColliderShape> m_Shape;
  xiiMeshResourceHandle             m_hCollisionMesh;
  xiiTexture2DResourceHandle        m_hSignedDistanceField;
  xiiUInt32                         m_uiCollisionLayer   = 0U;
  float                             m_fRadius            = 1.0f;
  float                             m_fCapsuleHalfHeight = 1.0f;
  xiiVec3                           m_vExtents           = xiiVec3(1.0f);
  float                             m_fRestitution       = 0.5f;
  float                             m_fFriction          = 0.2f;
  float                             m_fThickness         = 0.01f;
  bool                              m_bKillOnContact     = false;
  bool                              m_bInvert            = false;
};

class XII_GRAPHICSCORE_DLL xiiParticleRendererComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleRendererComponent, xiiComponent, xiiParticleRendererComponentManager);

public:
  xiiParticleRendererComponent();
  ~xiiParticleRendererComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  xiiHashedString                  m_sSystemBinding;
  xiiEnum<xiiParticleRendererType> m_RendererType;
  xiiMaterialResourceHandle        m_hMaterial;
  xiiMeshResourceHandle            m_hMesh;
  xiiColor                         m_Tint                  = xiiColor::White;
  float                            m_fSize                 = 0.05f;
  float                            m_fStretch              = 1.0f;
  float                            m_fSoftParticleDistance = 0.0f;
  xiiUInt8                         m_uiSortPriority        = 128U;
  bool                             m_bSortByDepth          = false;
  bool                             m_bReceiveLighting      = false;
  bool                             m_bCastShadows          = false;
  bool                             m_bMotionVectors        = true;
};

class XII_GRAPHICSCORE_DLL xiiParticleSimulationDomainComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleSimulationDomainComponent, xiiComponent, xiiParticleSimulationDomainComponentManager);

public:
  xiiParticleSimulationDomainComponent();
  ~xiiParticleSimulationDomainComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  void                                      SetShape(xiiEnum<xiiParticleSimulationDomainShape> shape);
  xiiEnum<xiiParticleSimulationDomainShape> GetShape() const { return m_Shape; }
  void                                      SetRadius(float fRadius);
  float                                     GetRadius() const { return m_fRadius; }
  void                                      SetExtents(xiiVec3 vExtents);
  xiiVec3                                   GetExtents() const { return m_vExtents; }

protected:
  void OnMsgUpdateLocalBounds(xiiMsgUpdateLocalBounds& ref_msg) const;

private:
  void UpdateLocalBounds();

private:
  xiiHashedString                               m_sSystemBinding;
  xiiEnum<xiiParticleSimulationDomainShape>     m_Shape;
  xiiBitflags<xiiParticleSimulationDomainFlags> m_Flags;
  xiiVec3                                       m_vExtents        = xiiVec3(20.0f);
  float                                         m_fRadius         = 10.0f;
  float                                         m_fCellSize       = 1.0f;
  xiiUInt32                                     m_uiMaxGridCells  = 1024U * 1024U;
  bool                                          m_bWorldSpace     = true;
  bool                                          m_bAdaptiveBounds = false;
};

class XII_GRAPHICSCORE_DLL xiiParticleMolecularDynamicsComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiParticleMolecularDynamicsComponent, xiiComponent, xiiParticleMolecularDynamicsComponentManager);

public:
  xiiParticleMolecularDynamicsComponent();
  ~xiiParticleMolecularDynamicsComponent();

  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

private:
  xiiHashedString                         m_sSystemBinding;
  xiiEnum<xiiParticleMolecularIntegrator> m_Integrator;
  xiiEnum<xiiParticleMolecularPotential>  m_Potential;
  xiiEnum<xiiParticleMolecularEnsemble>   m_Ensemble;
  float                                   m_fTimeStep            = 0.001f;
  xiiUInt8                                m_uiSubSteps           = 1U;
  float                                   m_fCutoffRadius        = 2.5f;
  float                                   m_fNeighborSkin        = 0.3f;
  float                                   m_fTargetTemperature   = 300.0f;
  float                                   m_fTargetPressure      = 1.0f;
  float                                   m_fParticleMass        = 1.0f;
  float                                   m_fLennardJonesEpsilon = 1.0f;
  float                                   m_fLennardJonesSigma   = 1.0f;
  float                                   m_fCoulombScale        = 1.0f;
  bool                                    m_bUseHalfNeighborList = true;
  bool                                    m_bDeterministic       = true;
  bool                                    m_bEnableReadback      = false;
};
