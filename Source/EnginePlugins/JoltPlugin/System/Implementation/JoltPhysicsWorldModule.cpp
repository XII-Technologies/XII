/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/World/World.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/AtomicInteger.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Meshes/MeshComponent.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltStaticCollisionComponent.h>
#include <JoltPlugin/Joints/JoltJointComponent.h>
#include <JoltPlugin/Resources/Implementation/JoltCollisionMeshData.h>
#include <JoltPlugin/Resources/Implementation/JoltSceneCollisionCook.h>
#include <JoltPlugin/Shapes/JoltPrimitiveShapeComponent.h>
#include <JoltPlugin/System/JoltCollisionLayerConfig.h>
#include <JoltPlugin/System/JoltPhysicsWorldModule.h>

#include <JoltPlugin/System/Implementation/JoltJobSystem.h>

#include <Jolt/Jolt.h>

#include <Jolt/Core/FPFlushDenormals.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/ContactListener.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <Jolt/Physics/Collision/Shape/ScaledShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/StaticCompoundShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#ifdef JPH_DEBUG_RENDERER
#  include <Jolt/Renderer/DebugRendererSimple.h>
#endif

#include <optional>
#include <utility>

XII_IMPLEMENT_WORLD_MODULE(xiiJoltPhysicsWorldModule);

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiJoltPhysicsWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

namespace
{
  static_assert(static_cast<int>(xiiJoltJointMotorMode::Off) == static_cast<int>(JPH::EMotorState::Off) &&
                static_cast<int>(xiiJoltJointMotorMode::Velocity) == static_cast<int>(JPH::EMotorState::Velocity) &&
                static_cast<int>(xiiJoltJointMotorMode::Position) == static_cast<int>(JPH::EMotorState::Position));
  constexpr xiiUInt32 MaxBodies             = 4096;
  constexpr xiiUInt32 MaxBodyPairs          = 16384;
  constexpr xiiUInt32 MaxContactConstraints = 8192;
  constexpr xiiUInt32 ScratchBufferSize     = 8 * 1024 * 1024;
  constexpr xiiUInt32 CollisionLayerCount   = xiiJoltCollisionLayerConfig::LayerCount;

#ifdef JPH_DEBUG_RENDERER
  constexpr xiiUInt32 MaxDebugContactPoints = 4096;
  constexpr xiiUInt32 MaxDebugLines         = 300000;
  constexpr xiiUInt32 MaxDebugTriangles     = 100000;
  constexpr xiiUInt32 MaxDebugLabels        = 4096;
  const auto          ContactDebugModes     = xiiJoltDebugMode::Contacts | xiiJoltDebugMode::ContactNormals;
#endif
  constexpr JPH::BroadPhaseLayer StaticBroadPhase(0);
  constexpr JPH::BroadPhaseLayer MovingBroadPhase(1);

  static_assert(static_cast<int>(xiiJoltCharacterGroundState::OnGround) == static_cast<int>(JPH::CharacterBase::EGroundState::OnGround) &&
                static_cast<int>(xiiJoltCharacterGroundState::OnSteepGround) == static_cast<int>(JPH::CharacterBase::EGroundState::OnSteepGround) &&
                static_cast<int>(xiiJoltCharacterGroundState::NotSupported) == static_cast<int>(JPH::CharacterBase::EGroundState::NotSupported) &&
                static_cast<int>(xiiJoltCharacterGroundState::InAir) == static_cast<int>(JPH::CharacterBase::EGroundState::InAir));

  JPH::Vec3 ToJoltVector(const xiiVec3& vValue) { return JPH::Vec3(vValue.x, vValue.y, vValue.z); }
  JPH::Quat ToJoltRotation(const xiiQuat& qValue) { return JPH::Quat(qValue.x, qValue.y, qValue.z, qValue.w).Normalized(); }

  template <typename Vector>
  xiiVec3 FromJoltVector(const Vector& value)
  {
    return xiiVec3(static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ()));
  }

  xiiQuat FromJoltRotation(JPH::QuatArg rotation)
  {
    return xiiQuat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
  }

#ifdef JPH_DEBUG_RENDERER
  class JoltDebugCollector final : public JPH::DebugRendererSimple
  {
  public:
    void DrawLine(JPH::RVec3Arg start, JPH::RVec3Arg end, JPH::ColorArg color) override
    {
      if (m_pData == nullptr)
        return;
      if (m_pData->m_Lines.GetCount() >= MaxDebugLines)
      {
        m_pData->m_bTruncated = true;
        return;
      }
      m_pData->m_Lines.PushBack({FromJoltVector(start), FromJoltVector(end), ConvertColor(color)});
    }

    void DrawTriangle(JPH::RVec3Arg a, JPH::RVec3Arg b, JPH::RVec3Arg c, JPH::ColorArg color, ECastShadow) override
    {
      if (m_pData == nullptr)
        return;
      if (m_pData->m_Triangles.GetCount() >= MaxDebugTriangles)
      {
        m_pData->m_bTruncated = true;
        return;
      }
      m_pData->m_Triangles.PushBack({{FromJoltVector(a), FromJoltVector(b), FromJoltVector(c)}, ConvertColor(color)});
    }

    void DrawText3D(JPH::RVec3Arg position, const JPH::string_view& text, JPH::ColorArg color, float) override
    {
      if (m_pData == nullptr)
        return;
      if (m_pData->m_Labels.GetCount() >= MaxDebugLabels)
      {
        m_pData->m_bTruncated = true;
        return;
      }
      m_pData->m_Labels.PushBack({FromJoltVector(position), ConvertColor(color), xiiStringView(text.data(), text.data() + text.size())});
    }

    xiiJoltDebugDrawData* m_pData = nullptr;

  private:
    static xiiColor ConvertColor(JPH::ColorArg color)
    {
      return xiiColor(xiiColorGammaUB(color.r, color.g, color.b, color.a));
    }
  };

  // Jolt permits one renderer instance. Its immutable triangle batches can be
  // used by any world, while a synchronous draw scopes the output to one world.
  xiiMutex                          s_DebugCollectorMutex;
  std::optional<JoltDebugCollector> s_DebugCollector;

  class DebugBodyFilter final : public JPH::BodyDrawFilter
  {
  public:
    bool ShouldDraw(const JPH::Body& body) const override { return !m_bActiveOnly || body.IsActive(); }
    bool m_bActiveOnly = false;
  };
#endif

  const xiiJoltActorComponent* FindActiveActor(const xiiGameObject& object)
  {
    for (const auto* pComponent : object.GetComponents())
    {
      if (const auto* pActor = xiiDynamicCast<const xiiJoltActorComponent*>(pComponent); pActor != nullptr && pActor->IsActiveAndSimulating())
        return pActor;
    }
    return nullptr;
  }

  void CollectShapes(xiiGameObject& object, xiiDynamicArray<xiiJoltShapeComponent*>& shapes)
  {
    for (auto* c : object.GetComponents())
      if (auto* p = xiiDynamicCast<xiiJoltShapeComponent*>(c); p && p->IsActiveAndInitialized())
        shapes.PushBack(p);
    for (auto it = object.GetChildren(); it.IsValid(); ++it)
      if (!FindActiveActor(*it))
        CollectShapes(*it, shapes);
  }
  const xiiJoltActorComponent* ShapeActor(const xiiGameObject* pObject)
  {
    for (; pObject; pObject = pObject->GetParent())
      if (auto* pActor = FindActiveActor(*pObject))
        return pActor;
    return nullptr;
  }
  bool ShapeMatrix(const xiiJoltShapeComponent& shape, const xiiGameObject& actor, xiiMat4& matrix)
  {
    matrix        = xiiTransform(shape.GetLocalPosition(), shape.GetLocalRotation()).GetAsMat4();
    const auto* p = shape.GetOwner();
    for (; p && p != &actor; p = p->GetParent())
      matrix = p->GetLocalTransform().GetAsMat4() * matrix;
    if (!p)
      return false;
    matrix = xiiMat4::MakeScaling(actor.GetGlobalScaling()) * matrix;
    return matrix.IsValid();
  }
  bool DecomposeShape(const xiiMat4& matrix, xiiTransform& pose)
  {
    xiiVec3 axes[3] = {matrix.GetColumn(0).GetAsVec3(), matrix.GetColumn(1).GetAsVec3(), matrix.GetColumn(2).GetAsVec3()};
    xiiVec3 scale(axes[0].GetLength(), axes[1].GetLength(), axes[2].GetLength());
    if (!scale.IsValid() || scale.x <= 0 || scale.y <= 0 || scale.z <= 0)
      return false;
    axes[0] /= scale.x;
    axes[1] /= scale.y;
    axes[2] /= scale.z;
    if (xiiMath::Abs(axes[0].Dot(axes[1])) > 0.0001f || xiiMath::Abs(axes[0].Dot(axes[2])) > 0.0001f ||
        xiiMath::Abs(axes[1].Dot(axes[2])) > 0.0001f)
      return false;
    if (axes[0].CrossRH(axes[1]).Dot(axes[2]) < 0)
    {
      scale.x = -scale.x;
      axes[0] = -axes[0];
    }
    xiiMat3 rotation;
    for (xiiUInt32 i = 0; i < 3; ++i)
      rotation.SetColumn(i, axes[i]);
    pose = xiiTransform(matrix.GetTranslationVector(), xiiQuat::MakeFromMat3(rotation), scale);
    return pose.m_vPosition.IsValid() && pose.m_qRotation.IsValid();
  }
  JPH::Shape::ShapeResult CreatePrimitive(const xiiJoltShapeComponent& component)
  {
    if (const auto* pBox = xiiDynamicCast<const xiiJoltBoxShapeComponent*>(&component))
    {
      const auto half = pBox->GetDimensions() * 0.5f;
      if (half.IsValid() && half.x > 0 && half.y > 0 && half.z > 0)
        return JPH::BoxShapeSettings(ToJoltVector(half), xiiMath::Min(JPH::cDefaultConvexRadius, 0.5f * xiiMath::Min(half.x, half.y, half.z))).Create();
    }
    else if (const auto* pSphere = xiiDynamicCast<const xiiJoltSphereShapeComponent*>(&component))
    {
      if (xiiMath::IsFinite(pSphere->GetRadius()) && pSphere->GetRadius() > 0)
        return JPH::SphereShapeSettings(pSphere->GetRadius()).Create();
    }
    else
    {
      JPH::Shape::ShapeResult result;
      if (const auto* pCapsule = xiiDynamicCast<const xiiJoltCapsuleShapeComponent*>(&component))
      {
        if (xiiMath::IsFinite(pCapsule->GetRadius()) && xiiMath::IsFinite(pCapsule->GetHeight()) && pCapsule->GetRadius() > 0 && pCapsule->GetHeight() > 0)
          result = JPH::CapsuleShapeSettings(pCapsule->GetHeight() * 0.5f, pCapsule->GetRadius()).Create();
      }
      else if (const auto* pCylinder = xiiDynamicCast<const xiiJoltCylinderShapeComponent*>(&component))
      {
        if (xiiMath::IsFinite(pCylinder->GetRadius()) && xiiMath::IsFinite(pCylinder->GetHeight()) && pCylinder->GetRadius() > 0 && pCylinder->GetHeight() > 0)
          result = JPH::CylinderShapeSettings(pCylinder->GetHeight() * 0.5f, pCylinder->GetRadius(), xiiMath::Min(JPH::cDefaultConvexRadius, 0.25f * xiiMath::Min(pCylinder->GetRadius(), pCylinder->GetHeight()))).Create();
      }
      if (!result.IsEmpty())
      {
        if (result.HasError())
          return result;
        return JPH::RotatedTranslatedShapeSettings(JPH::Vec3::sZero(), JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::JPH_PI * 0.5f), result.Get()).Create();
      }
    }
    JPH::Shape::ShapeResult error;
    error.SetError("Collision dimensions must be finite and positive.");
    return error;
  }

  class xiiJoltCollisionFilters final :
    public JPH::BroadPhaseLayerInterface,
    public JPH::ObjectVsBroadPhaseLayerFilter,
    public JPH::ObjectLayerPairFilter
  {
  public:
    static JPH::ObjectLayer MakeObjectLayer(xiiUInt8 uiCollisionLayer, JPH::EMotionType motionType)
    {
      return static_cast<JPH::ObjectLayer>(uiCollisionLayer + (motionType == JPH::EMotionType::Static ? 0U : CollisionLayerCount));
    }

    void SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled)
    {
      m_Config.SetCollisionEnabled(uiLayerA, uiLayerB, bEnabled);
    }

    JPH::uint GetNumBroadPhaseLayers() const override { return 2; }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override
    {
      return layer < CollisionLayerCount ? StaticBroadPhase : MovingBroadPhase;
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
      return layer == StaticBroadPhase ? "Static" : "Moving";
    }
#endif

    bool ShouldCollide(JPH::ObjectLayer layer, JPH::BroadPhaseLayer broadPhaseLayer) const override
    {
      return layer >= CollisionLayerCount || broadPhaseLayer == MovingBroadPhase;
    }

    bool ShouldCollide(JPH::ObjectLayer layerA, JPH::ObjectLayer layerB) const override
    {
      if (layerA < CollisionLayerCount && layerB < CollisionLayerCount)
        return false;

      return AreLayersEnabled(static_cast<xiiUInt8>(layerA), static_cast<xiiUInt8>(layerB));
    }

    bool AreLayersEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB) const
    {
      return m_Config.IsCollisionEnabled(uiLayerA, uiLayerB);
    }

    xiiJoltCollisionLayerConfig m_Config;
  };
} // namespace

struct xiiJoltShapePreview::Cache
{
  xiiSharedPtr<xiiJoltCollisionMeshData> m_pResource;
  JPH::RefConst<JPH::Shape>              m_pShape;
  const xiiRTTI*                         m_pType       = nullptr;
  xiiVec3                                m_vDimensions = xiiVec3::MakeZero();
};

xiiJoltShapePreview::xiiJoltShapePreview()  = default;
xiiJoltShapePreview::~xiiJoltShapePreview() = default;

const xiiJoltDebugDrawData& xiiJoltShapePreview::GetGeometry() const
{
  if (m_pCache && m_pCache->m_pResource && m_pCache->m_pResource->m_pPreviewGeometry)
    return *m_pCache->m_pResource->m_pPreviewGeometry;
  return m_Geometry;
}

xiiResult xiiJoltPhysicsWorldModule::UpdateShapePreview(const xiiJoltShapeComponent& shape, xiiJoltShapePreview& preview)
{
  preview.m_sStatus.Clear();
  preview.m_bPending = false;
#ifdef JPH_DEBUG_RENDERER
  const auto*          object = shape.GetOwner();
  const xiiGameObject* actor  = nullptr;
  for (const auto* ancestor = object; ancestor && !actor; ancestor = ancestor->GetParent())
    for (const auto* component : ancestor->GetComponents())
      if (const auto* candidate = xiiDynamicCast<const xiiJoltActorComponent*>(component); candidate && candidate->IsActiveAndInitialized())
      {
        actor = ancestor;
        break;
      }
  preview.m_hActor = actor ? actor->GetHandle() : xiiGameObjectHandle();
  if (actor == nullptr)
    actor = object;
  xiiMat4      matrix;
  xiiTransform local;
  if (!ShapeMatrix(shape, *actor, matrix) || !DecomposeShape(matrix, local))
  {
    preview.m_sStatus = "Invalid, zero-scale, or sheared collider attachment.";
    return XII_FAILURE;
  }
  auto actorPose      = actor->GetGlobalTransform();
  actorPose.m_vScale  = xiiVec3(1.0f);
  preview.m_Transform = actorPose * local;
  xiiVec3 dimensions  = xiiVec3::MakeZero();
  if (const auto* box = xiiDynamicCast<const xiiJoltBoxShapeComponent*>(&shape))
    dimensions = box->GetDimensions();
  else if (const auto* sphere = xiiDynamicCast<const xiiJoltSphereShapeComponent*>(&shape))
    dimensions.x = sphere->GetRadius();
  else if (const auto* capsule = xiiDynamicCast<const xiiJoltCapsuleShapeComponent*>(&shape))
    dimensions = xiiVec3(capsule->GetRadius(), capsule->GetHeight(), 0);
  else if (const auto* cylinder = xiiDynamicCast<const xiiJoltCylinderShapeComponent*>(&shape))
    dimensions = xiiVec3(cylinder->GetRadius(), cylinder->GetHeight(), 0);

  xiiSharedPtr<xiiJoltCollisionMeshData> resourceData;
  xiiJoltCollisionMeshResourceHandle     resourceHandle;
  bool                                   mesh = false;
  if (const auto* hull = xiiDynamicCast<const xiiJoltConvexHullShapeComponent*>(&shape))
    resourceHandle = hull->GetCollisionMesh();
  else if (const auto* triangleMesh = xiiDynamicCast<const xiiJoltMeshShapeComponent*>(&shape))
  {
    resourceHandle = triangleMesh->GetCollisionMesh();
    mesh           = true;
  }
  if (mesh || xiiDynamicCast<const xiiJoltConvexHullShapeComponent*>(&shape))
  {
    if (!resourceHandle.IsValid())
    {
      preview.m_sStatus = "Assign a collision mesh resource.";
      return XII_FAILURE;
    }
    const xiiResourceLock<xiiJoltCollisionMeshResource> resource(resourceHandle, xiiResourceAcquireMode::PointerOnly);
    resourceData = resource->GetData();
    if (!resourceData)
    {
      preview.m_bPending = resource->GetLoadingState() != xiiResourceState::LoadedResourceMissing;
      preview.m_sStatus  = preview.m_bPending ? "Collision resource pending." : "Invalid collision resource. Check its transformation diagnostics.";
      if (preview.m_bPending)
        xiiResourceManager::PreloadResource(resourceHandle);
      return XII_FAILURE;
    }
    if ((resourceData->m_Type == xiiJoltCollisionGeometryType::TriangleMesh) != mesh)
    {
      preview.m_sStatus = "Collision resource geometry type does not match the collider.";
      return XII_FAILURE;
    }
  }
  if (!preview.m_pCache)
    preview.m_pCache = XII_DEFAULT_NEW(xiiJoltShapePreview::Cache);
  auto&      cache   = *preview.m_pCache;
  const bool rebuild = cache.m_pType != shape.GetDynamicRTTI() || cache.m_vDimensions != dimensions || cache.m_pResource != resourceData;
  // Validate scale on every pose change, without reconstructing an unchanged primitive.
  JPH::RefConst<JPH::Shape> nativeShape;
  if (!rebuild)
    nativeShape = cache.m_pShape;
  else if (resourceData)
    nativeShape = resourceData->m_pShape;
  else
  {
    auto result = CreatePrimitive(shape);
    if (result.HasError())
    {
      preview.m_sStatus = result.GetError().c_str();
      return XII_FAILURE;
    }
    nativeShape = result.Get();
  }
  if (!nativeShape->IsValidScale(ToJoltVector(local.m_vScale)))
  {
    preview.m_sStatus = "Jolt does not support the collider's authored scale.";
    return XII_FAILURE;
  }
  if (mesh)
  {
    for (const auto* component : actor->GetComponents())
      if (component->IsActiveAndInitialized() && (xiiDynamicCast<const xiiJoltDynamicActorComponent*>(component) || xiiDynamicCast<const xiiJoltKinematicActorComponent*>(component)))
      {
        preview.m_sStatus = "Triangle-mesh collision requires a static actor.";
        return XII_FAILURE;
      }
  }
  if (rebuild)
  {
    XII_LOCK(s_DebugCollectorMutex);
    if (!s_DebugCollector)
      s_DebugCollector.emplace();
    // All instances of one immutable cooked resource reuse its converted debug geometry.
    if (!resourceData || !resourceData->m_pPreviewGeometry)
    {
      if (resourceData)
        resourceData->m_pPreviewGeometry = XII_DEFAULT_NEW(xiiJoltDebugDrawData);
      auto& renderer = *s_DebugCollector;
      auto& data     = resourceData ? *resourceData->m_pPreviewGeometry : preview.m_Geometry;
      data.m_Lines.Clear();
      data.m_Triangles.Clear();
      data.m_Labels.Clear();
      data.m_bTruncated = false;
      renderer.m_pData  = &data;
      nativeShape->Draw(&renderer, JPH::RMat44::sTranslation(JPH::RVec3(nativeShape->GetCenterOfMass())), JPH::Vec3::sOne(), JPH::Color::sWhite, false, true);
      renderer.m_pData = nullptr;
    }
    cache.m_pShape      = nativeShape;
    cache.m_pResource   = resourceData;
    cache.m_pType       = shape.GetDynamicRTTI();
    cache.m_vDimensions = dimensions;
  }
  return XII_SUCCESS;
#else
  XII_IGNORE_UNUSED(shape);
  preview.m_sStatus = "Authored collision previews require a Debug or Dev build.";
  return XII_FAILURE;
#endif
}

struct xiiJoltPhysicsWorldModule::NativeWorld : JPH::ContactListener
{
  explicit NativeWorld(xiiAllocator* pAllocator) :
    m_ShapeSets(pAllocator),
    m_TrackedShapes(pAllocator),
    m_SubContacts(pAllocator),
    m_ContactManifolds(pAllocator),
    m_PendingBodies(pAllocator),
    m_PosesToSync(pAllocator),
    m_KinematicActors(pAllocator),
    m_TeleportedBodies(pAllocator),
    m_Characters(pAllocator),
    m_PendingCharacters(pAllocator),
    m_Contacts(pAllocator),
    m_ContactEvents(pAllocator),
    m_DispatchContactEvents(pAllocator),
    m_QueuedImpulses(pAllocator),
    m_Joints(pAllocator),
    m_PendingJoints(pAllocator),
    m_WaitingJoints(pAllocator),
    m_BreakableJoints(pAllocator),
    m_BrokenJoints(pAllocator),
    m_BodiesByComponent(pAllocator),
    m_OwnersByBody(pAllocator)
  {
    m_PosesToSync.Reserve(MaxBodies);
    m_Contacts.Reserve(MaxBodies);
    m_SubContacts.Reserve(MaxContactConstraints);
    m_ContactManifolds.Reserve(MaxBodies);
    m_ContactEvents.Reserve(MaxBodies);
    m_DispatchContactEvents.Reserve(MaxBodies);
    m_PhysicsSystem.Init(MaxBodies, 0, MaxBodyPairs, MaxContactConstraints, m_CollisionFilters, m_CollisionFilters, m_CollisionFilters);
    m_PhysicsSystem.SetGravity(JPH::Vec3(0.0f, 0.0f, -9.81f));
    m_PhysicsSystem.SetContactListener(this);
    xiiResourceManager::GetResourceEvents().AddEventHandler(xiiMakeDelegate(&NativeWorld::OnResourceEvent, this));
  }

  ~NativeWorld()
  {
    xiiResourceManager::GetResourceEvents().RemoveEventHandler(xiiMakeDelegate(&NativeWorld::OnResourceEvent, this));
    m_JobSystem.DrainTasks();
    DrainStaticCooks();
    m_PhysicsSystem.SetContactListener(nullptr);
    ClearBodies();
  }

  struct BodyOwner
  {
    xiiGameObjectHandle      m_hObject;
    xiiComponentHandle       m_hComponent;
    xiiGameObjectHandle      m_hShapeObject;
    xiiComponentHandle       m_hShapeComponent;
    xiiUInt32                m_uiFilterId;
    bool                     m_bDynamic;
    xiiSurfaceResourceHandle m_hSurface;
    xiiHashedString          m_sCollisionInteraction;
    bool                     m_bMaterialInitialized = false;
    bool                     m_bGenerated           = false;
    float                    m_fFriction            = 0.2f;
    float                    m_fRestitution         = 0;
  };

  struct DynamicPose
  {
    xiiGameObjectHandle m_hObject;
    xiiVec3             m_vPosition;
    xiiQuat             m_qRotation;
    xiiUInt32           m_uiDepth;
    bool                m_bPositionOnly = false;
  };

  struct KinematicTarget
  {
    JPH::RVec3 m_Position = JPH::RVec3::sZero();
    JPH::Quat  m_Rotation = JPH::Quat::sIdentity();
    bool       m_bValid   = false;
  };

  struct ShapeOwner
  {
    xiiGameObjectHandle                       m_hObject;
    xiiComponentHandle                        m_hComponent;
    xiiSurfaceResourceHandle                  m_hSurfaceOverride;
    xiiSharedPtr<xiiJoltCollisionMeshData>    m_pResource;
    xiiDynamicArray<xiiSurfaceResourceHandle> m_SurfaceSlots;
    xiiDynamicArray<BodyOwner>                m_Surfaces;
    xiiTransform                              m_StaticLocal = xiiTransform::MakeIdentity();
  };
  struct ShapeSet
  {
    JPH::RefConst<JPH::Shape>   m_pRoot;
    xiiDynamicArray<ShapeOwner> m_Shapes;
  };
  struct TrackedShape
  {
    xiiComponentHandle m_hActor;
    xiiMat4            m_Matrix       = xiiMat4::MakeIdentity();
    bool               m_bInitialized = false;
    bool               m_bMatrixValid = false;
  };
  xiiHashTable<xiiUInt32, ShapeSet>              m_ShapeSets;
  xiiHashTable<xiiComponentHandle, TrackedShape> m_TrackedShapes;
  xiiAtomicInteger32                             m_GeometryDirty{1};

  static const ShapeOwner* FindShapeOwner(const ShapeSet& set, JPH::SubShapeID id)
  {
    xiiUInt32       index     = 0;
    JPH::SubShapeID remainder = id;
    if (set.m_Shapes.GetCount() > 1)
    {
      const auto& compound = *static_cast<const JPH::StaticCompoundShape*>(set.m_pRoot.GetPtr());
      const auto  child    = compound.GetSubShapeIndexFromID(id, remainder);
      if (child >= compound.GetNumSubShapes())
        return nullptr;
      index = compound.GetSubShape(child).mUserData;
    }
    return index < set.m_Shapes.GetCount() ? &set.m_Shapes[index] : nullptr;
  }

  const BodyOwner* ResolveOwner(JPH::BodyID body, JPH::SubShapeID id) const
  {
    const auto* pSet = m_ShapeSets.GetValue(body.GetIndexAndSequenceNumber());
    if (pSet == nullptr || pSet->m_Shapes.IsEmpty())
      return FindOwner(body);
    const auto* pOwner = FindShapeOwner(*pSet, id);
    if (!pOwner)
      return nullptr;
    xiiUInt32       material = 0;
    JPH::SubShapeID leafId;
    const auto*     pLeaf = pSet->m_pRoot->GetLeafShape(id, leafId);
    if (pLeaf != nullptr && pLeaf->GetSubType() == JPH::EShapeSubType::Mesh)
      material = static_cast<const JPH::MeshShape*>(pLeaf)->GetMaterialIndex(leafId);
    return material < pOwner->m_Surfaces.GetCount() ? &pOwner->m_Surfaces[material] : FindOwner(body);
  }

  /// One body per owning component. The owner handles must be live in this world.
  /// The supplied layer uses XII's collision-layer index. Motion type determines broadphase placement.
  [[nodiscard]] xiiResult CreateBody(const xiiWorld& world, xiiComponentHandle hComponent, const xiiComponent& shape, xiiUInt8 uiCollisionLayer, JPH::BodyCreationSettings& ref_settings, JPH::EActivation activation)
  {
    if (hComponent.IsInvalidated() || hComponent.GetInternalID().m_WorldIndex != world.GetIndex())
      return XII_FAILURE;

    const xiiComponent* pComponent = nullptr;
    if (!world.TryGetComponent(hComponent, pComponent) || pComponent->GetOwner() == nullptr)
      return XII_FAILURE;

    ref_settings.mObjectLayer = xiiJoltCollisionFilters::MakeObjectLayer(uiCollisionLayer, ref_settings.mMotionType);
    const auto* pPrevious     = FindOwner(FindBody(hComponent));
    if (pPrevious == nullptr && m_uiNextFilterId == xiiInvalidIndex)
      return XII_FAILURE;
    const xiiUInt32 uiFilterId = pPrevious != nullptr ? pPrevious->m_uiFilterId : m_uiNextFilterId;
    ref_settings.mCollisionGroup.SetGroupID(uiFilterId);
    auto& bodies = m_PhysicsSystem.GetBodyInterface();
    auto* pBody  = bodies.CreateBody(ref_settings);
    if (pBody == nullptr)
      return XII_FAILURE;
    if (pPrevious == nullptr)
      ++m_uiNextFilterId;

    const auto bodyId = pBody->GetID();
    RemoveBody(hComponent);
    m_BodiesByComponent.Insert(hComponent, bodyId);
    m_OwnersByBody.Insert(bodyId.GetIndexAndSequenceNumber(), BodyOwner{pComponent->GetOwner()->GetHandle(), hComponent, shape.GetOwner()->GetHandle(), shape.GetHandle(), uiFilterId, ref_settings.mMotionType == JPH::EMotionType::Dynamic});
    if (ref_settings.mMotionType == JPH::EMotionType::Kinematic)
      m_KinematicActors.Insert(hComponent, KinematicTarget{});
    bodies.AddBody(bodyId, activation);
    m_bBodiesChanged = true;
    m_MaterialsDirty = 1;
    return XII_SUCCESS;
  }

  JPH::BodyID FindBody(xiiComponentHandle hComponent) const
  {
    JPH::BodyID bodyId;
    m_BodiesByComponent.TryGetValue(hComponent, bodyId);
    return bodyId;
  }

  const BodyOwner* FindOwner(JPH::BodyID bodyId) const
  {
    return m_OwnersByBody.GetValue(bodyId.GetIndexAndSequenceNumber());
  }
  BodyOwner* FindOwnerMutable(JPH::BodyID bodyId)
  {
    return m_OwnersByBody.GetValue(bodyId.GetIndexAndSequenceNumber());
  }

  struct Contact
  {
    BodyOwner                       m_Owners[2];
    xiiVec3                         m_vPosition;
    xiiVec3                         m_vNormal;
    float                           m_fImpactSqr   = 0;
    xiiUInt64                       m_uiKey        = 0;
    xiiUInt32                       m_uiEventIndex = xiiInvalidIndex;
    xiiEnum<xiiPhysicsContactPhase> m_Phase;
    bool                            m_bReport[2];
  };
  struct SubContact
  {
    XII_DECLARE_POD_TYPE();

    xiiUInt64 m_uiBodies;
    xiiUInt64 m_uiShapes;
    bool      operator==(const SubContact&) const = default;
  };
  struct SubContactHash
  {
    static xiiUInt32 Hash(const SubContact& key) { return xiiHashingUtils::xxHash32(&key, sizeof(key)); }
    static bool      Equal(const SubContact& a, const SubContact& b) { return a == b; }
  };
  xiiHashSet<SubContact, SubContactHash> m_SubContacts;
  xiiHashTable<xiiUInt64, xiiUInt32>     m_ContactManifolds;

  static xiiUInt64 ContactKey(JPH::BodyID a, JPH::BodyID b)
  {
    return (static_cast<xiiUInt64>(a.GetIndexAndSequenceNumber()) << 32) | b.GetIndexAndSequenceNumber();
  }

  void RecordContact(const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings, xiiPhysicsContactPhase::Enum phase)
  {
    const auto* pOwnerA = ResolveOwner(a.GetID(), manifold.mSubShapeID1);
    const auto* pOwnerB = ResolveOwner(b.GetID(), manifold.mSubShapeID2);
    if (pOwnerA == nullptr || pOwnerB == nullptr)
      return;
    settings.mCombinedFriction    = static_cast<float>(xiiMath::Sqrt(double(pOwnerA->m_fFriction) * pOwnerB->m_fFriction));
    settings.mCombinedRestitution = xiiMath::Max(pOwnerA->m_fRestitution, pOwnerB->m_fRestitution);
#ifdef JPH_DEBUG_RENDERER
    if (m_bCaptureDebugContacts)
    {
      XII_LOCK(m_ContactMutex);
      for (JPH::uint i = 0; i < manifold.mRelativeContactPointsOn1.size(); ++i)
      {
        if (m_DebugContacts.GetCount() == MaxDebugContactPoints)
        {
          m_bDebugContactsTruncated = true;
          break;
        }
        m_DebugContacts.PushBack({FromJoltVector(manifold.GetWorldSpaceContactPointOn1(i)), FromJoltVector(manifold.GetWorldSpaceContactPointOn2(i)),
                                  FromJoltVector(manifold.mWorldSpaceNormal), manifold.mPenetrationDepth});
      }
    }
#endif
    if ((a.GetUserData() | b.GetUserData()) == 0 || manifold.mRelativeContactPointsOn1.empty())
      return;

    JPH::Vec3 average = JPH::Vec3::sZero();
    for (JPH::uint i = 0; i < manifold.mRelativeContactPointsOn1.size(); ++i)
      average += manifold.mRelativeContactPointsOn1[i] + manifold.mRelativeContactPointsOn2[i];
    const auto  position = manifold.mBaseOffset + average / (2.0f * static_cast<float>(manifold.mRelativeContactPointsOn1.size()));
    const auto  normal   = manifold.mWorldSpaceNormal;
    const float fSpeed   = xiiMath::Max(0.0f, (a.GetPointVelocity(position) - b.GetPointVelocity(position)).Dot(normal));

    XII_LOCK(m_ContactMutex);
    const auto       key = ContactKey(a.GetID(), b.GetID());
    const SubContact subContact{key, (xiiUInt64(manifold.mSubShapeID1.GetValue()) << 32) | manifold.mSubShapeID2.GetValue()};
    if (!m_SubContacts.Insert(subContact))
      ++m_ContactManifolds[key];
    const auto* pPrevious = m_Contacts.GetValue(key);
    if (pPrevious == nullptr)
      phase = xiiPhysicsContactPhase::Added;
    Contact contact{{*pOwnerA, *pOwnerB},
                    xiiVec3(static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ())),
                    xiiVec3(normal.GetX(), normal.GetY(), normal.GetZ()),
                    fSpeed * fSpeed,
                    key,
                    xiiInvalidIndex,
                    phase,
                    {(a.GetUserData() & 1) != 0, (b.GetUserData() & 1) != 0}};
    if (pPrevious != nullptr && pPrevious->m_uiEventIndex != xiiInvalidIndex)
    {
      contact.m_uiEventIndex = pPrevious->m_uiEventIndex;
      auto& event            = m_ContactEvents[contact.m_uiEventIndex];
      // Keep one report per pair per world update, preserving the initial Added phase and strongest impact.
      if (contact.m_fImpactSqr > event.m_fImpactSqr)
      {
        contact.m_Phase = event.m_Phase;
        event           = contact;
      }
    }
    else
    {
      contact.m_uiEventIndex = m_ContactEvents.GetCount();
      m_ContactEvents.PushBack(contact);
    }
    if (!m_Contacts.Insert(key, contact))
      m_uiTrackedContacts.Increment();
  }

  void OnContactAdded(const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
  {
    RecordContact(a, b, manifold, settings, xiiPhysicsContactPhase::Added);
  }

  void OnContactPersisted(const JPH::Body& a, const JPH::Body& b, const JPH::ContactManifold& manifold, JPH::ContactSettings& settings) override
  {
    RecordContact(a, b, manifold, settings, xiiPhysicsContactPhase::Persisted);
  }

  void OnContactRemoved(const JPH::SubShapeIDPair& pair) override
  {
    if (m_uiTrackedContacts == 0)
      return;

    XII_LOCK(m_ContactMutex);
    const auto       key = ContactKey(pair.GetBody1ID(), pair.GetBody2ID());
    const SubContact subContact{key, (xiiUInt64(pair.GetSubShapeID1().GetValue()) << 32) | pair.GetSubShapeID2().GetValue()};
    if (!m_SubContacts.Remove(subContact))
      return;
    if (auto* pCount = m_ContactManifolds.GetValue(key); pCount != nullptr && *pCount > 1)
    {
      --*pCount;
      return;
    }
    m_ContactManifolds.Remove(key);
    Contact contact;
    if (m_Contacts.Remove(ContactKey(pair.GetBody1ID(), pair.GetBody2ID()), &contact))
    {
      m_uiTrackedContacts.Decrement();
      contact.m_Phase      = xiiPhysicsContactPhase::Removed;
      contact.m_fImpactSqr = 0.0f;
      m_ContactEvents.PushBack(contact);
    }
  }

  struct QueuedImpulse
  {
    xiiComponentHandle m_hActor;
    xiiVec3            m_vPosition;
    xiiVec3            m_vImpulse;
    xiiUInt32          m_uiFilterId;
  };

  bool RemoveBody(xiiComponentHandle hComponent)
  {
    JPH::BodyID bodyId;
    if (!m_BodiesByComponent.Remove(hComponent, &bodyId))
      return false;

    const auto uiId = bodyId.GetIndexAndSequenceNumber();
#ifdef JPH_DEBUG_RENDERER
    m_DebugContacts.Clear();
    m_uiDebugFrame = xiiInvalidIndex;
#endif
    for (auto it = m_Joints.GetIterator(); it.IsValid(); ++it)
    {
      auto& joint = it.Value();
      if (joint.m_pConstraint != nullptr && (joint.m_Bodies[0] == bodyId || joint.m_Bodies[1] == bodyId))
      {
        DetachJoint(joint);
        joint.m_bRecreate = true;
        m_WaitingJoints.Insert(it.Key());
      }
    }
    m_bBodiesChanged = true;
    // A body can be removed while paused, before Jolt reports contact removals.
    if (m_uiTrackedContacts != 0)
    {
      for (auto it = m_Contacts.GetIterator(); it.IsValid();)
      {
        if (static_cast<xiiUInt32>(it.Key() >> 32) != uiId && static_cast<xiiUInt32>(it.Key()) != uiId)
        {
          ++it;
          continue;
        }
        auto contact         = it.Value();
        contact.m_Phase      = xiiPhysicsContactPhase::Removed;
        contact.m_fImpactSqr = 0.0f;
        m_ContactEvents.PushBack(contact);
        m_ContactManifolds.Remove(it.Key());
        it = m_Contacts.Remove(it);
        m_uiTrackedContacts.Decrement();
      }
    }

    for (auto it = m_SubContacts.GetIterator(); it.IsValid();)
    {
      if (static_cast<xiiUInt32>(it.Key().m_uiBodies >> 32) == uiId || static_cast<xiiUInt32>(it.Key().m_uiBodies) == uiId)
        it = m_SubContacts.Remove(it);
      else
        ++it;
    }
    m_KinematicActors.Remove(hComponent);
    m_TeleportedBodies.Remove(uiId);
    m_OwnersByBody.Remove(uiId);
    DestroyBody(bodyId);
    m_ShapeSets.Remove(bodyId.GetIndexAndSequenceNumber());
    return true;
  }

  void ClearBodies()
  {
    m_Characters.Clear();
    m_PendingCharacters.Clear();
    m_KinematicActors.Clear();
    m_TeleportedBodies.Clear();
    for (auto it = m_Joints.GetIterator(); it.IsValid(); ++it)
      DetachJoint(it.Value());
    m_Joints.Clear();
    m_PendingJoints.Clear();
    m_WaitingJoints.Clear();
    m_BreakableJoints.Clear();
    m_BrokenJoints.Clear();
    m_PendingBodies.Clear();
    m_PosesToSync.Clear();
    m_Contacts.Clear();
    m_SubContacts.Clear();
    m_ContactManifolds.Clear();
    m_uiTrackedContacts = 0;
    m_ContactEvents.Clear();
    m_DispatchContactEvents.Clear();
    m_QueuedImpulses.Clear();
#ifdef JPH_DEBUG_RENDERER
    m_DebugContacts.Clear();
    m_bDebugContactsTruncated = false;
    m_bCaptureDebugContacts   = false;
    m_bDebugContactsRequested = false;
    m_DebugData.m_Lines.Clear();
    m_DebugData.m_Triangles.Clear();
    m_DebugData.m_Labels.Clear();
    m_uiDebugFrame = xiiInvalidIndex;
#endif
    m_LastUpdateDuration = xiiTime::MakeZero();
    m_uiLastSteps        = 0;
    m_uiLastUpdateErrors = 0;
    for (auto it = m_BodiesByComponent.GetIterator(); it.IsValid(); ++it)
    {
      DestroyBody(it.Value());
    }

    m_OwnersByBody.Clear();
    m_BodiesByComponent.Clear();
    m_ShapeSets.Clear();
    m_TrackedShapes.Clear();
  }

  xiiJoltCollisionFilters                  m_CollisionFilters;
  xiiJoltJobSystem                         m_JobSystem;
  JPH::TempAllocatorImplWithMallocFallback m_TempAllocator{ScratchBufferSize};
  JPH::PhysicsSystem                       m_PhysicsSystem;
  xiiHashSet<xiiComponentHandle>           m_PendingBodies;

  struct StaticSource
  {
    xiiGameObjectHandle                        m_hObject;
    xiiComponentHandle                         m_hComponent;
    xiiTransform                               m_Local = xiiTransform::MakeIdentity();
    xiiSharedPtr<xiiJoltSceneCollisionCook>    m_pCook;
    xiiDynamicArray<xiiMaterialResourceHandle> m_MaterialOverrides;
    xiiDynamicArray<xiiMaterialResourceHandle> m_MaterialDependencies;
  };
  struct StaticGroup
  {
    xiiTransform                        m_Pose = xiiTransform::MakeIdentity();
    xiiDynamicArray<StaticSource>       m_Sources;
    xiiDynamicArray<xiiComponentHandle> m_InstalledSources;
    bool                                m_bDirty     = true;
    bool                                m_bPoseValid = true;
  };
  xiiHashTable<xiiComponentHandle, StaticGroup>                    m_StaticGroups;
  xiiHashSet<xiiComponentHandle>                                   m_PendingStaticGroups;
  xiiHashTable<xiiString, xiiSharedPtr<xiiJoltSceneCollisionCook>> m_StaticMeshCache;
  xiiDynamicArray<xiiSharedPtr<xiiJoltSceneCollisionCook>>         m_StaticCooks;
  xiiMutex                                                         m_StaticResourceMutex;
  xiiHashSet<xiiUInt64>                                            m_StaticResourceChanges;
  xiiHashSet<xiiUInt64>                                            m_StaticMeshReloads;
  xiiAtomicInteger32                                               m_bStaticResourcesChanged{0};
  xiiAtomicInteger32                                               m_StaticGroupCount{0};
  bool                                                             m_bPruneStaticMeshCache = false;

  void DrainStaticCooks()
  {
    for (const auto& task : m_StaticCooks)
      xiiTaskSystem::CancelGroup(task->m_Group).IgnoreResult();
    m_StaticCooks.Clear();
    m_StaticMeshCache.Clear();
    m_StaticGroups.Clear();
    m_PendingStaticGroups.Clear();
    m_StaticGroupCount = 0;
    XII_LOCK(m_StaticResourceMutex);
    m_StaticResourceChanges.Clear();
    m_StaticMeshReloads.Clear();
    m_bStaticResourcesChanged = 0;
  }
  xiiDynamicArray<DynamicPose>                      m_PosesToSync;
  xiiHashTable<xiiComponentHandle, KinematicTarget> m_KinematicActors;
  xiiHashSet<xiiUInt32>                             m_TeleportedBodies;

  struct CharacterContactListener final : JPH::CharacterContactListener
  {
    void OnContactAdded(const JPH::CharacterVirtual* pCharacter, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& ref_settings) override
    {
      ref_settings.mCanReceiveImpulses = contact.mMotionTypeB == JPH::EMotionType::Dynamic && !contact.mIsSensorB && pCharacter->GetMaxStrength() > 0.0f;
    }

    void OnContactPersisted(const JPH::CharacterVirtual* pCharacter, const JPH::CharacterContact& contact, JPH::CharacterContactSettings& ref_settings) override
    {
      OnContactAdded(pCharacter, contact, ref_settings);
    }
  };

  struct Character
  {
    JPH::Ref<JPH::CharacterVirtual>            m_pCharacter;
    xiiJoltCharacterControllerComponent::State m_State;
    float                                      m_fVerticalVelocity = 0.0f;
    float                                      m_fRadius           = 0.0f;
    float                                      m_fHeight           = 0.0f;
    bool                                       m_bRefreshContacts  = true;
  };

  CharacterContactListener                    m_CharacterContactListener;
  xiiHashTable<xiiComponentHandle, Character> m_Characters;
  xiiHashSet<xiiComponentHandle>              m_PendingCharacters;

  struct CharacterBodyFilter final : JPH::BodyFilter
  {
    CharacterBodyFilter(const NativeWorld& native, xiiGameObjectHandle hOwner) :
      m_Native(native), m_hOwner(hOwner)
    {
    }

    bool ShouldCollideLocked(const JPH::Body& body) const override
    {
      const auto* pOwner = m_Native.FindOwner(body.GetID());
      return pOwner != nullptr && pOwner->m_hObject != m_hOwner;
    }

    const NativeWorld&  m_Native;
    xiiGameObjectHandle m_hOwner;
  };

  struct CharacterShapeFilter final : JPH::ShapeFilter
  {
    CharacterShapeFilter(const NativeWorld& native, xiiGameObjectHandle hOwner) :
      m_Native(native), m_hOwner(hOwner)
    {
    }

    bool ShouldCollide(const JPH::Shape*, const JPH::SubShapeID&, const JPH::Shape* pShape, const JPH::SubShapeID& id) const override
    {
      const auto* pSet = m_Native.m_ShapeSets.GetValue(mBodyID2.GetIndexAndSequenceNumber());
      if (pSet == nullptr || (pSet->m_Shapes.GetCount() > 1 && pSet->m_pRoot.GetPtr() == pShape))
        return true;

      // A character object can also own one child collider of another actor's compound.
      const auto* pOwner = FindShapeOwner(*pSet, id);
      return pOwner != nullptr && pOwner->m_hObject != m_hOwner;
    }

    const NativeWorld&  m_Native;
    xiiGameObjectHandle m_hOwner;
  };
  xiiMutex                      m_ContactMutex;
  xiiBitflags<xiiJoltDebugMode> m_DebugModes;
  xiiTime                       m_LastUpdateDuration;
  xiiUInt32                     m_uiLastSteps        = 0;
  xiiUInt32                     m_uiLastUpdateErrors = 0;
#ifdef JPH_DEBUG_RENDERER
  struct DebugContact
  {
    xiiVec3 m_vPositionA;
    xiiVec3 m_vPositionB;
    xiiVec3 m_vNormal;
    float   m_fDepth;
  };
  xiiDynamicArray<DebugContact> m_DebugContacts;
  xiiJoltDebugDrawData          m_DebugData;
  xiiBitflags<xiiJoltDebugMode> m_CollectedDebugModes;
  xiiUInt32                     m_uiDebugFrame            = xiiInvalidIndex;
  bool                          m_bCaptureDebugContacts   = false;
  bool                          m_bDebugContactsRequested = false;
  bool                          m_bDebugContactsTruncated = false;
#endif
  xiiAtomicIntegerU32              m_uiTrackedContacts;
  xiiHashTable<xiiUInt64, Contact> m_Contacts;
  xiiDynamicArray<Contact>         m_ContactEvents;
  xiiDynamicArray<Contact>         m_DispatchContactEvents;
  xiiMutex                         m_ImpulseMutex;
  xiiDynamicArray<QueuedImpulse>   m_QueuedImpulses;

  struct Joint
  {
    JPH::Ref<JPH::TwoBodyConstraint> m_pConstraint;
    JPH::BodyID                      m_Bodies[2];
    float                            m_fBreakForce  = 0;
    float                            m_fBreakTorque = 0;
    bool                             m_bRecreate    = true;
  };
  void DetachJoint(Joint& joint)
  {
    if (joint.m_pConstraint != nullptr)
    {
      m_PhysicsSystem.RemoveConstraint(joint.m_pConstraint);
      for (auto id : joint.m_Bodies)
        if (!id.IsInvalid() && m_PhysicsSystem.GetBodyInterface().GetMotionType(id) == JPH::EMotionType::Dynamic)
          m_PhysicsSystem.GetBodyInterface().ActivateBody(id);
      joint.m_pConstraint = nullptr;
    }
  }
  void OnResourceEvent(const xiiResourceEvent& event)
  {
    if ((event.m_Type == xiiResourceEvent::Type::ResourceContentUpdated || event.m_Type == xiiResourceEvent::Type::ResourceContentUnloading) &&
        event.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiJoltCollisionMeshResource>())
      m_GeometryDirty = 1;
    if ((event.m_Type == xiiResourceEvent::Type::ResourceContentUpdated || event.m_Type == xiiResourceEvent::Type::ResourceContentUnloading) &&
        event.m_pResource->GetDynamicRTTI()->IsDerivedFrom<xiiSurfaceResource>())
      m_MaterialsDirty = 1;
    if (m_StaticGroupCount == 0)
      return;
    const auto* pType = event.m_pResource->GetDynamicRTTI();
    const auto  hash  = event.m_pResource->GetResourceIDHash();
    if (pType == xiiGetStaticRTTI<xiiMeshResource>() && event.m_Type == xiiResourceEvent::Type::ResourceContentUnloading)
    {
      XII_LOCK(m_StaticResourceMutex);
      m_StaticMeshReloads.Insert(hash);
      m_bStaticResourcesChanged = 1;
    }
    else if ((pType == xiiGetStaticRTTI<xiiMaterialResource>() || pType == xiiGetStaticRTTI<xiiJoltCollisionMeshResource>()) &&
             (event.m_Type == xiiResourceEvent::Type::ResourceContentUpdated || event.m_Type == xiiResourceEvent::Type::ResourceContentUnloading))
    {
      XII_LOCK(m_StaticResourceMutex);
      m_StaticResourceChanges.Insert(hash);
      m_bStaticResourcesChanged = 1;
    }
  }
  xiiHashTable<xiiComponentHandle, Joint> m_Joints;
  xiiHashSet<xiiComponentHandle>          m_PendingJoints;
  xiiHashSet<xiiComponentHandle>          m_WaitingJoints;
  xiiDynamicArray<xiiComponentHandle>     m_BreakableJoints;
  xiiDynamicArray<xiiComponentHandle>     m_BrokenJoints;
  xiiAtomicInteger32                      m_MaterialsDirty{1};
  bool                                    m_bBodiesChanged = false;

  const auto& GetBodyOwners() const { return m_OwnersByBody; }

private:
  void DestroyBody(JPH::BodyID bodyId)
  {
    auto& bodies = m_PhysicsSystem.GetBodyInterface();
    bodies.RemoveBody(bodyId);
    bodies.DestroyBody(bodyId);
  }

  xiiHashTable<xiiComponentHandle, JPH::BodyID> m_BodiesByComponent;
  xiiHashTable<xiiUInt32, BodyOwner>            m_OwnersByBody;
  xiiUInt32                                     m_uiNextFilterId = 0;
};

xiiJoltPhysicsWorldModule::xiiJoltPhysicsWorldModule(xiiWorld* pWorld) :
  xiiPhysicsWorldModuleInterface(pWorld)
{
}

xiiJoltPhysicsWorldModule::~xiiJoltPhysicsWorldModule() = default;

bool xiiJoltPhysicsWorldModule::HasSimulationState() const
{
  return m_pNativeWorld && GetWorld()->GetWorldSimulationEnabled() &&
    (m_pNativeWorld->m_PhysicsSystem.GetNumBodies() != 0 || !m_pNativeWorld->m_Characters.IsEmpty());
}

xiiResult xiiJoltPhysicsWorldModule::SetSimulationStep(xiiTime fixedStep, xiiUInt32 uiMaxCatchUpSteps)
{
  const float fStepSeconds = fixedStep.AsFloatInSeconds();
  if (!xiiMath::IsFinite(fStepSeconds) || fStepSeconds <= 0.0f || !xiiMath::IsFinite(1.0f / fStepSeconds) ||
      uiMaxCatchUpSteps == 0 || !xiiMath::IsFinite(fStepSeconds * static_cast<float>(uiMaxCatchUpSteps)))
    return XII_FAILURE;

  m_FixedStep         = fixedStep;
  m_uiMaxCatchUpSteps = uiMaxCatchUpSteps;
  m_AccumulatedTime   = xiiTime::MakeZero();
  return XII_SUCCESS;
}

xiiResult xiiJoltPhysicsWorldModule::SetCollisionEnabled(xiiUInt8 uiLayerA, xiiUInt8 uiLayerB, bool bEnabled)
{
  if (!m_pNativeWorld || m_pNativeWorld->m_PhysicsSystem.GetNumBodies() != 0 || !m_pNativeWorld->m_Characters.IsEmpty())
    return XII_FAILURE;

  m_pNativeWorld->m_CollisionFilters.SetCollisionEnabled(uiLayerA, uiLayerB, bEnabled);
  return XII_SUCCESS;
}

xiiStatus xiiJoltPhysicsWorldModule::ReloadCollisionLayerConfig()
{
  if (!m_pNativeWorld || !m_pNativeWorld->m_Characters.IsEmpty())
    return xiiStatus("Jolt collision-layer changes require an empty physics world. Restart simulation to apply them.");
  if (m_pNativeWorld->m_PhysicsSystem.GetNumBodies() != 0)
  {
    // Authored preview bodies are also registered while the editor is stopped.
    // Reloading the pair table is safe when only generated static bodies exist.
    if (GetWorld()->GetWorldSimulationEnabled())
      return xiiStatus("Jolt collision-layer changes require stopping simulation first.");
    for (const auto& entry : m_pNativeWorld->GetBodyOwners())
      if (!entry.Value().m_bGenerated)
        return xiiStatus("Jolt collision-layer changes require an empty physics world. Restart simulation to apply them.");
  }
  return m_pNativeWorld->m_CollisionFilters.m_Config.Load();
}

void xiiJoltPhysicsWorldModule::Initialize()
{
  m_pNativeWorld = XII_NEW(xiiFoundation::GetAlignedAllocator(), NativeWorld, GetAllocator());
  ReloadCollisionLayerConfig().LogFailure();

  auto desc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::Simulate, this);
  desc.m_Phase                     = xiiWorldUpdatePhase::PreAsync;
  desc.m_bOnlyUpdateWhenSimulating = true;
  desc.m_fPriority                 = -100000.0f;
  RegisterUpdateFunction(desc);

  auto staticCollision        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::UpdateStaticCollision, this);
  staticCollision.m_Phase     = xiiWorldUpdatePhase::PreAsync;
  staticCollision.m_fPriority = -99999.0f;
  RegisterUpdateFunction(staticCollision);

  auto contacts    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::DispatchContacts, this);
  contacts.m_Phase = xiiWorldUpdatePhase::PostAsync;
  RegisterUpdateFunction(contacts);
}

void xiiJoltPhysicsWorldModule::Deinitialize()
{
  auto desc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::Simulate, this);
  desc.m_Phase = xiiWorldUpdatePhase::PreAsync;
  DeregisterUpdateFunction(desc);

  auto staticCollision    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::UpdateStaticCollision, this);
  staticCollision.m_Phase = xiiWorldUpdatePhase::PreAsync;
  DeregisterUpdateFunction(staticCollision);

  auto contacts    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiJoltPhysicsWorldModule::DispatchContacts, this);
  contacts.m_Phase = xiiWorldUpdatePhase::PostAsync;
  DeregisterUpdateFunction(contacts);

  m_pNativeWorld.Clear();
}

void xiiJoltPhysicsWorldModule::OnSimulationStarted()
{
  m_AccumulatedTime = xiiTime::MakeZero();
}

void xiiJoltPhysicsWorldModule::WorldClear()
{
  m_pNativeWorld->DrainStaticCooks();
  m_pNativeWorld->ClearBodies();
  m_AccumulatedTime = xiiTime::MakeZero();
}

void xiiJoltPhysicsWorldModule::SetDebugModes(xiiBitflags<xiiJoltDebugMode> modes)
{
  if (!m_pNativeWorld || m_pNativeWorld->m_DebugModes == modes)
    return;
  m_pNativeWorld->m_DebugModes = modes;
#ifdef JPH_DEBUG_RENDERER
  auto& native          = *m_pNativeWorld;
  native.m_uiDebugFrame = xiiInvalidIndex;
  if (!modes.IsAnySet(ContactDebugModes))
  {
    native.m_bDebugContactsRequested = false;
    native.m_DebugContacts.Clear();
    native.m_bDebugContactsTruncated = false;
  }
#endif
}

xiiBitflags<xiiJoltDebugMode> xiiJoltPhysicsWorldModule::GetDebugModes() const
{
  return m_pNativeWorld ? m_pNativeWorld->m_DebugModes : xiiBitflags<xiiJoltDebugMode>{};
}

bool xiiJoltPhysicsWorldModule::IsDebugDrawingSupported()
{
#ifdef JPH_DEBUG_RENDERER
  return true;
#else
  return false;
#endif
}

xiiJoltSimulationDiagnostics xiiJoltPhysicsWorldModule::GetSimulationDiagnostics() const
{
  xiiJoltSimulationDiagnostics diagnostics;
  diagnostics.m_FixedStep = m_FixedStep;
  if (!m_pNativeWorld)
    return diagnostics;

  const auto& native                       = *m_pNativeWorld;
  const auto  bodies                       = native.m_PhysicsSystem.GetBodyStats();
  diagnostics.m_uiBodies                   = bodies.mNumBodies;
  diagnostics.m_uiActiveBodies             = bodies.mNumActiveBodiesDynamic + bodies.mNumActiveBodiesKinematic + bodies.mNumActiveSoftBodies;
  diagnostics.m_uiSleepingBodies           = bodies.mNumBodiesDynamic - bodies.mNumActiveBodiesDynamic;
  diagnostics.m_uiStaticBodies             = bodies.mNumBodiesStatic;
  diagnostics.m_uiDynamicBodies            = bodies.mNumBodiesDynamic;
  diagnostics.m_uiKinematicBodies          = bodies.mNumBodiesKinematic;
  diagnostics.m_uiReportedContactManifolds = native.m_SubContacts.GetCount();
  for (auto it = native.m_Joints.GetIterator(); it.IsValid(); ++it)
    if (it.Value().m_pConstraint != nullptr)
      ++diagnostics.m_uiConstraints;
  diagnostics.m_uiCharacters           = native.m_Characters.GetCount();
  diagnostics.m_uiSteps                = native.m_uiLastSteps;
  diagnostics.m_LastUpdateDuration     = native.m_LastUpdateDuration;
  diagnostics.m_uiUpdateErrors         = native.m_uiLastUpdateErrors;
  diagnostics.m_uiReservedScratchBytes = ScratchBufferSize;
  return diagnostics;
}

const xiiJoltDebugDrawData* xiiJoltPhysicsWorldModule::GetDebugDrawData(xiiBitflags<xiiJoltDebugMode> modes)
{
#ifdef JPH_DEBUG_RENDERER
  if (!m_pNativeWorld)
    return nullptr;
  auto&      native    = *m_pNativeWorld;
  const auto modifiers = xiiJoltDebugMode::Statistics | xiiJoltDebugMode::ActiveBodiesOnly | xiiJoltDebugMode::SolidShapes;
  if ((modes.GetValue() & ~modifiers.GetValue()) == 0)
    return nullptr;

  // The renderer requests next update's samples only for worlds with a consuming view.
  native.m_bDebugContactsRequested = modes.IsAnySet(ContactDebugModes);
  if (native.m_uiDebugFrame == GetWorld()->GetUpdateCounter() && native.m_CollectedDebugModes == modes)
    return &native.m_DebugData;
  if (s_DebugCollectorMutex.TryLock().Failed())
    return nullptr;
  XII_SCOPE_EXIT(s_DebugCollectorMutex.Unlock());
  if (!s_DebugCollector)
  {
    if (JPH::DebugRenderer::sInstance != nullptr)
      return nullptr;
    s_DebugCollector.emplace();
  }

  XII_PROFILE_SCOPE("Jolt Debug Geometry");
  auto& data = native.m_DebugData;
  data.m_Lines.Clear();
  data.m_Triangles.Clear();
  data.m_Labels.Clear();
  data.m_bTruncated = modes.IsAnySet(ContactDebugModes) && native.m_bDebugContactsTruncated;
  auto& renderer    = *s_DebugCollector;
  renderer.m_pData  = &data;
  XII_SCOPE_EXIT(renderer.m_pData = nullptr);

  JPH::BodyManager::DrawSettings bodies;
  bodies.mDrawShape                 = modes.IsSet(xiiJoltDebugMode::Shapes);
  bodies.mDrawShapeWireframe        = !modes.IsSet(xiiJoltDebugMode::SolidShapes);
  bodies.mDrawShapeColor            = modes.IsSet(xiiJoltDebugMode::BodyStates) ? JPH::BodyManager::EShapeColor::SleepColor : JPH::BodyManager::EShapeColor::MotionTypeColor;
  bodies.mDrawBoundingBox           = modes.IsSet(xiiJoltDebugMode::Bounds);
  bodies.mDrawCenterOfMassTransform = modes.IsSet(xiiJoltDebugMode::CentersOfMass);
  bodies.mDrawWorldTransform        = modes.IsSet(xiiJoltDebugMode::BodyStates);
  bodies.mDrawVelocity              = modes.IsSet(xiiJoltDebugMode::Velocities);
  const bool      bActiveOnly       = modes.IsSet(xiiJoltDebugMode::ActiveBodiesOnly);
  DebugBodyFilter filter;
  filter.m_bActiveOnly = bActiveOnly;
  if (bodies.mDrawShape || bodies.mDrawBoundingBox || bodies.mDrawCenterOfMassTransform || bodies.mDrawWorldTransform || bodies.mDrawVelocity)
    native.m_PhysicsSystem.DrawBodies(bodies, &renderer, bActiveOnly ? &filter : nullptr);

  if (bodies.mDrawWorldTransform || modes.IsSet(xiiJoltDebugMode::BodyIdentifiers) || (bodies.mDrawShape && !bActiveOnly))
  {
    for (auto it = native.GetBodyOwners().GetIterator(); it.IsValid(); ++it)
    {
      const JPH::BodyLockRead lock(native.m_PhysicsSystem.GetBodyLockInterface(), JPH::BodyID(it.Key()));
      if (!lock.Succeeded() || (bActiveOnly && !lock.GetBody().IsActive()))
        continue;
      const auto& body = lock.GetBody();
      if (bodies.mDrawShape && native.m_StaticGroups.Contains(it.Value().m_hComponent))
        renderer.DrawText3D(body.GetPosition(), "Generated static collision", JPH::Color::sCyan, 0.15f);
      if (bodies.mDrawWorldTransform)
      {
        const auto color = body.IsSensor() ? JPH::Color::sYellow : body.IsStatic() ? JPH::Color::sGrey :
          !body.IsActive()                                                         ? JPH::Color::sRed :
          body.IsKinematic()                                                       ? JPH::Color::sGreen :
                                                                                     JPH::Color::sCyan;
        renderer.DrawMarker(body.GetPosition(), color, 0.08f);
      }
      if (modes.IsSet(xiiJoltDebugMode::BodyIdentifiers))
      {
        xiiStringBuilder text;
        text.SetFormat("Body {}:{}", body.GetID().GetIndex(), body.GetID().GetSequenceNumber());
        renderer.DrawText3D(body.GetPosition(), text.GetData(), JPH::Color::sWhite, 0.15f);
      }
    }
  }
  if (bodies.mDrawShape && !bActiveOnly)
    for (const auto& entry : native.m_StaticGroups)
    {
      const xiiJoltStaticCollisionComponent* pGroup = nullptr;
      if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(entry.Key(), pGroup))
        continue;
      const auto state    = pGroup->GetCollisionState();
      const auto position = pGroup->GetOwner()->GetGlobalPosition();
      if (position.IsValid() && (state == xiiJoltStaticCollisionComponent::State::PendingGeometry || state == xiiJoltStaticCollisionComponent::State::PendingResource))
        renderer.DrawText3D(JPH::RVec3(position.x, position.y, position.z), state == xiiJoltStaticCollisionComponent::State::PendingGeometry ? "Preparing static collision geometry" : "Loading static collision resource", JPH::Color::sOrange, 0.15f);
    }

  const bool bConstraints = modes.IsSet(xiiJoltDebugMode::Constraints);
  const bool bLimits      = modes.IsSet(xiiJoltDebugMode::ConstraintLimits);
  if (bConstraints)
    native.m_PhysicsSystem.DrawConstraintReferenceFrame(&renderer);
  if (bLimits)
    native.m_PhysicsSystem.DrawConstraintLimits(&renderer);
  if (bConstraints || bLimits)
    for (auto it = native.m_Joints.GetIterator(); it.IsValid(); ++it)
    {
      const auto* pConstraint = it.Value().m_pConstraint.GetPtr();
      if (pConstraint == nullptr)
      {
        if (bConstraints)
        {
          const xiiJoltJointComponent* pJoint = nullptr;
          if (GetWorld()->TryGetComponent(it.Key(), pJoint))
          {
            const auto position = JPH::RVec3(ToJoltVector(pJoint->GetOwner()->GetGlobalPosition()));
            renderer.DrawText3D(position, pJoint->IsBroken() ? "Broken joint" : "Unresolved joint", JPH::Color::sRed, 0.15f);
          }
        }
        continue;
      }
      const auto frameA = pConstraint->GetBody1()->GetCenterOfMassTransform() * pConstraint->GetConstraintToBody1Matrix();
      const auto frameB = pConstraint->GetBody2()->GetCenterOfMassTransform() * pConstraint->GetConstraintToBody2Matrix();
      const auto a      = frameA.GetTranslation();
      const auto b      = frameB.GetTranslation();
      if (bConstraints)
      {
        // DistanceConstraint's native draw uses cached solver positions. The
        // current body frames also work before the first step and while paused.
        if (pConstraint->GetSubType() != JPH::EConstraintSubType::Distance)
          pConstraint->DrawConstraint(&renderer);
        renderer.DrawLine(a, b, JPH::Color::sWhite);
        renderer.DrawMarker(a, JPH::Color::sRed, 0.08f);
        renderer.DrawMarker(b, JPH::Color::sGreen, 0.08f);
      }

      xiiStringBuilder text;
      switch (pConstraint->GetSubType())
      {
        case JPH::EConstraintSubType::Hinge:
        {
          const auto& hinge = *static_cast<const JPH::HingeConstraint*>(pConstraint);
          if (bConstraints && hinge.GetMotorState() == JPH::EMotorState::Position)
          {
            const auto axis   = frameA.GetAxisX();
            const auto target = JPH::Quat::sRotation(axis, hinge.GetTargetAngle()) * frameA.GetAxisY();
            renderer.DrawArrow(a, a + target * 0.35f, JPH::Color::sCyan, 0.04f);
          }
          else if (bConstraints && hinge.GetMotorState() == JPH::EMotorState::Velocity)
            text.SetFormat("Target {} rad/s", xiiArgF(hinge.GetTargetAngularVelocity(), 2));
          break;
        }
        case JPH::EConstraintSubType::Slider:
        {
          const auto& slider = *static_cast<const JPH::SliderConstraint*>(pConstraint);
          if (bConstraints && slider.GetMotorState() == JPH::EMotorState::Position)
            text.SetFormat("Target {} m", xiiArgF(slider.GetTargetPosition(), 2));
          else if (bConstraints && slider.GetMotorState() == JPH::EMotorState::Velocity)
            text.SetFormat("Target {} m/s", xiiArgF(slider.GetTargetVelocity(), 2));
          break;
        }
        case JPH::EConstraintSubType::Distance:
        {
          const auto& distance = *static_cast<const JPH::DistanceConstraint*>(pConstraint);
          if (bLimits)
          {
            const auto axis = JPH::Vec3(b - a).NormalizedOr(JPH::Vec3::sAxisX());
            renderer.DrawMarker(a + axis * distance.GetMinDistance(), JPH::Color::sPurple, 0.08f);
            if (distance.GetMaxDistance() < xiiMath::MaxValue<float>())
              renderer.DrawMarker(a + axis * distance.GetMaxDistance(), JPH::Color::sPurple, 0.08f);
            text.SetFormat("{} m  ({} to {} m)", xiiArgF(JPH::Vec3(b - a).Length(), 2), xiiArgF(distance.GetMinDistance(), 2), xiiArgF(distance.GetMaxDistance(), 2));
          }
          break;
        }
        default:
          break;
      }
      if (!text.IsEmpty())
        renderer.DrawText3D(b, text.GetData(), JPH::Color::sWhite, 0.15f);
    }

  if (modes.IsAnySet(ContactDebugModes))
    for (const auto& contact : native.m_DebugContacts)
    {
      const JPH::RVec3 a(ToJoltVector(contact.m_vPositionA)), b(ToJoltVector(contact.m_vPositionB));
      const auto       color = contact.m_fDepth < 0.0f ? JPH::Color::sCyan : JPH::Color::sOrange;
      if (modes.IsSet(xiiJoltDebugMode::Contacts))
      {
        renderer.DrawMarker(a, color, 0.035f);
        renderer.DrawMarker(b, color, 0.035f);
        renderer.DrawLine(a, b, color);
        xiiStringBuilder text;
        text.SetFormat("Depth {} m", xiiArgF(contact.m_fDepth, 3));
        renderer.DrawText3D(a, text.GetData(), color, 0.1f);
      }
      if (modes.IsSet(xiiJoltDebugMode::ContactNormals))
        renderer.DrawArrow(a, a + 0.25f * ToJoltVector(contact.m_vNormal), color, 0.035f);
    }

  if (modes.IsSet(xiiJoltDebugMode::Characters))
    for (auto it = native.m_Characters.GetIterator(); it.IsValid(); ++it)
    {
      const auto& character = *it.Value().m_pCharacter;
      using GroundState     = JPH::CharacterBase::EGroundState;
      const auto state      = !character.GetGroundBodyID().IsInvalid() && native.FindOwner(character.GetGroundBodyID()) == nullptr ? GroundState::InAir : character.GetGroundState();
      const auto color      = state == GroundState::OnGround ? JPH::Color::sGreen : state == GroundState::OnSteepGround ? JPH::Color::sOrange :
        state == GroundState::NotSupported                                                                              ? JPH::Color::sRed :
                                                                                                                          JPH::Color::sCyan;
      const auto position   = character.GetPosition();
      character.GetShape()->Draw(&renderer, character.GetCenterOfMassTransform(), JPH::Vec3::sOne(), color, false, true);
      renderer.DrawMarker(position, color, 0.08f);
      renderer.DrawArrow(position, position + character.GetLinearVelocity(), JPH::Color::sWhite, 0.06f);
      renderer.DrawText3D(position, JPH::CharacterBase::sToString(state), color, 0.15f);
      if (state != GroundState::InAir)
      {
        const auto ground = character.GetGroundPosition();
        renderer.DrawMarker(ground, color, 0.08f);
        renderer.DrawArrow(ground, ground + 0.35f * character.GetGroundNormal(), color, 0.06f);
      }
    }

  native.m_uiDebugFrame        = GetWorld()->GetUpdateCounter();
  native.m_CollectedDebugModes = modes;
  return &data;
#else
  XII_IGNORE_UNUSED(modes);
  return nullptr;
#endif
}

void xiiJoltPhysicsWorldModule::ShutdownDebugRenderer()
{
#ifdef JPH_DEBUG_RENDERER
  s_DebugCollector.reset();
#endif
}

void xiiJoltPhysicsWorldModule::QueueBodyUpdate(xiiComponentHandle hActor)
{
  if (m_pNativeWorld)
  {
    m_pNativeWorld->m_PendingBodies.Insert(hActor);
    m_pNativeWorld->m_bBodiesChanged = true;
  }
}

void xiiJoltPhysicsWorldModule::SetBodyContactReporting(xiiComponentHandle hActor, bool bEnabled)
{
  if (m_pNativeWorld)
  {
    const auto id = m_pNativeWorld->FindBody(hActor);
    if (!id.IsInvalid())
    {
      auto& bodies = m_pNativeWorld->m_PhysicsSystem.GetBodyInterface();
      bodies.SetUserData(id, (bodies.GetUserData(id) & ~JPH::uint64{1}) | (bEnabled ? 1 : 0));
    }
  }
}

void xiiJoltPhysicsWorldModule::RemoveBody(xiiComponentHandle hActor)
{
  if (m_pNativeWorld)
  {
    m_pNativeWorld->m_PendingBodies.Remove(hActor);
    m_pNativeWorld->RemoveBody(hActor);
  }
}

JPH::BodyInterface* xiiJoltPhysicsWorldModule::FindDynamicBody(xiiComponentHandle hActor, JPH::BodyID& out_bodyId) const
{
  if (!m_pNativeWorld)
    return nullptr;

  out_bodyId   = m_pNativeWorld->FindBody(hActor);
  auto& bodies = m_pNativeWorld->m_PhysicsSystem.GetBodyInterface();
  return !out_bodyId.IsInvalid() && bodies.GetMotionType(out_bodyId) == JPH::EMotionType::Dynamic ? &bodies : nullptr;
}

void xiiJoltPhysicsWorldModule::RebuildBody(xiiJoltActorComponent& actor)
{
  auto& nativeWorld = *m_pNativeWorld;
  auto& object      = *actor.GetOwner();
  if (FindActiveActor(object) != &actor)
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt object '{}' supports one active actor.", object.GetName());
    return;
  }

  xiiDynamicArray<xiiJoltShapeComponent*> shapes(GetAllocator());
  CollectShapes(object, shapes);
  if (shapes.IsEmpty())
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    return;
  }
  object.UpdateGlobalTransform();
  xiiTransform actorPose = object.GetGlobalTransform();
  actorPose.m_vScale     = xiiVec3(1.0f);
  if (!actorPose.m_vPosition.IsValid() || !actorPose.m_qRotation.IsValid())
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Jolt actor '{}' has an invalid pose.", object.GetName());
    return;
  }
  const auto*                      pDynamic   = xiiDynamicCast<const xiiJoltDynamicActorComponent*>(&actor);
  const auto*                      pKinematic = xiiDynamicCast<const xiiJoltKinematicActorComponent*>(&actor);
  const auto                       motionType = pDynamic ? JPH::EMotionType::Dynamic : pKinematic ? JPH::EMotionType::Kinematic :
                                                                                                    JPH::EMotionType::Static;
  NativeWorld::ShapeSet            shapeSet;
  JPH::StaticCompoundShapeSettings compound;
  for (auto* pShape : shapes)
  {
    xiiMat4      matrix;
    xiiTransform local;
    if (!ShapeMatrix(*pShape, object, matrix) || !DecomposeShape(matrix, local))
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt collider '{}' has an invalid, zero-scale, or sheared attachment transform.", pShape->GetOwner()->GetName());
      return;
    }
    JPH::RefConst<JPH::Shape>              nativeShape;
    xiiSharedPtr<xiiJoltCollisionMeshData> resourceData;
    xiiJoltCollisionMeshResourceHandle     resourceHandle;
    bool                                   bMesh = false;
    if (const auto* pHull = xiiDynamicCast<const xiiJoltConvexHullShapeComponent*>(pShape))
      resourceHandle = pHull->GetCollisionMesh();
    else if (const auto* pMesh = xiiDynamicCast<const xiiJoltMeshShapeComponent*>(pShape))
    {
      resourceHandle = pMesh->GetCollisionMesh();
      bMesh          = true;
    }
    if (bMesh || xiiDynamicCast<const xiiJoltConvexHullShapeComponent*>(pShape))
    {
      if (!resourceHandle.IsValid() || (bMesh && motionType != JPH::EMotionType::Static))
      {
        nativeWorld.RemoveBody(actor.GetHandle());
        xiiLog::Warning("Jolt collider '{}' requires a collision resource; triangle meshes require static actors.", pShape->GetOwner()->GetName());
        return;
      }
      const xiiResourceLock<xiiJoltCollisionMeshResource> resource(resourceHandle, xiiResourceAcquireMode::PointerOnly);
      resourceData = resource->GetData();
      if (!resourceData)
      {
        xiiResourceManager::PreloadResource(resourceHandle);
        // A resource replacement can be pending while a valid old body still owns its immutable snapshot.
        // Never preserve geometry whose contributing components have been removed or reparented.
        const auto* pOld     = nativeWorld.m_ShapeSets.GetValue(nativeWorld.FindBody(actor.GetHandle()).GetIndexAndSequenceNumber());
        bool        preserve = pOld != nullptr;
        if (pOld)
          for (const auto& old : pOld->m_Shapes)
          {
            const xiiJoltShapeComponent* pComponent = nullptr;
            preserve &= static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(old.m_hComponent, pComponent) &&
              pComponent->IsActiveAndInitialized() && ShapeActor(pComponent->GetOwner()) == &actor;
          }
        if (!preserve)
          nativeWorld.RemoveBody(actor.GetHandle());
        return;
      }
      if ((resourceData->m_Type == xiiJoltCollisionGeometryType::TriangleMesh) != bMesh)
      {
        nativeWorld.RemoveBody(actor.GetHandle());
        xiiLog::Warning("Jolt collider '{}' references the wrong collision geometry type.", pShape->GetOwner()->GetName());
        return;
      }
      nativeShape = resourceData->m_pShape;
    }
    else
    {
      auto result = CreatePrimitive(*pShape);
      if (result.HasError())
      {
        nativeWorld.RemoveBody(actor.GetHandle());
        xiiLog::Warning("Jolt collider '{}': {}", pShape->GetOwner()->GetName(), result.GetError().c_str());
        return;
      }
      nativeShape = result.Get();
    }
    if (!nativeShape->IsValidScale(ToJoltVector(local.m_vScale)))
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt collider '{}' does not support its authored scale.", pShape->GetOwner()->GetName());
      return;
    }
    if (local.m_vScale != xiiVec3(1.0f))
      nativeShape = new JPH::ScaledShape(nativeShape, ToJoltVector(local.m_vScale));
    const auto childIndex = shapeSet.m_Shapes.GetCount();
    compound.AddShape(ToJoltVector(local.m_vPosition), ToJoltRotation(local.m_qRotation), nativeShape, childIndex);
    auto& owner              = shapeSet.m_Shapes.ExpandAndGetRef();
    owner.m_hObject          = pShape->GetOwner()->GetHandle();
    owner.m_hComponent       = pShape->GetHandle();
    owner.m_hSurfaceOverride = pShape->GetSurface();
    owner.m_pResource        = resourceData;
    owner.m_Surfaces.SetCount(resourceData ? resourceData->m_Surfaces.GetCount() : 1);
  }
  auto result = compound.Create(nativeWorld.m_TempAllocator);
  if (result.HasError())
  {
    nativeWorld.RemoveBody(actor.GetHandle());
    xiiLog::Warning("Could not assemble Jolt actor '{}': {}", object.GetName(), result.GetError().c_str());
    return;
  }
  JPH::RefConst<JPH::Shape> pNativeShape = result.Get();
  shapeSet.m_pRoot                       = pNativeShape;
  auto* pShape                           = shapes[0];

  JPH::BodyCreationSettings settings(pNativeShape.GetPtr(),
                                     JPH::RVec3(actorPose.m_vPosition.x, actorPose.m_vPosition.y, actorPose.m_vPosition.z),
                                     ToJoltRotation(actorPose.m_qRotation), motionType, 0);
  auto                      activation = JPH::EActivation::DontActivate;
  if (pDynamic != nullptr)
  {
    settings.mGravityFactor  = pDynamic->GetGravityFactor();
    settings.mLinearDamping  = pDynamic->GetLinearDamping();
    settings.mAngularDamping = pDynamic->GetAngularDamping();
    settings.mAllowSleeping  = pDynamic->GetAllowSleeping();
    auto massProperties      = pNativeShape->GetMassProperties();
    massProperties.ScaleToMass(massProperties.mMass * pDynamic->GetDensity() / 1000.0f);
    if (pDynamic->GetMass() > 0.0f)
      massProperties.ScaleToMass(pDynamic->GetMass());

    const auto inertia = massProperties.mInertia.GetDiagonal3();
    if (!xiiMath::IsFinite(massProperties.mMass) || massProperties.mMass <= 0.0f || !xiiMath::IsFinite(1.0f / massProperties.mMass) ||
        !xiiMath::IsFinite(inertia.GetX()) || !xiiMath::IsFinite(inertia.GetY()) || !xiiMath::IsFinite(inertia.GetZ()) || inertia.ReduceMin() <= 0.0f)
    {
      nativeWorld.RemoveBody(actor.GetHandle());
      xiiLog::Warning("Jolt actor '{}' has invalid mass properties.", object.GetName());
      return;
    }
    settings.mOverrideMassProperties = JPH::EOverrideMassProperties::MassAndInertiaProvided;
    settings.mMassPropertiesOverride = massProperties;

    const auto oldBody = nativeWorld.FindBody(actor.GetHandle());
    if (!oldBody.IsInvalid())
    {
      const auto& bodies = nativeWorld.m_PhysicsSystem.GetBodyInterface();
      bodies.GetPositionAndRotation(oldBody, settings.mPosition, settings.mRotation);
      bodies.GetLinearAndAngularVelocity(oldBody, settings.mLinearVelocity, settings.mAngularVelocity);
      const auto oldCenter = bodies.GetShape(oldBody)->GetCenterOfMass();
      settings.mLinearVelocity += settings.mAngularVelocity.Cross(settings.mRotation * (pNativeShape->GetCenterOfMass() - oldCenter));
      if (bodies.IsActive(oldBody) || !settings.mAllowSleeping)
        activation = JPH::EActivation::Activate;
    }
    else
    {
      settings.mLinearVelocity  = ToJoltVector(pDynamic->GetInitialLinearVelocity());
      settings.mAngularVelocity = ToJoltVector(pDynamic->GetInitialAngularVelocity());
      if (!pDynamic->GetStartAsleep() || !settings.mAllowSleeping)
        activation = JPH::EActivation::Activate;
    }
  }

  if (pKinematic)
  {
    // Rebuilding geometry must not move the body directly to a pending authored target.
    const auto oldBody = nativeWorld.FindBody(actor.GetHandle());
    if (!oldBody.IsInvalid())
      nativeWorld.m_PhysicsSystem.GetBodyInterface().GetPositionAndRotation(oldBody, settings.mPosition, settings.mRotation);
  }
  settings.mUserData = actor.GetReportContacts() ? 1 : 0;
  if (nativeWorld.CreateBody(*GetWorld(), actor.GetHandle(), *pShape, actor.GetCollisionLayer(), settings, activation).Failed())
    xiiLog::Warning("Jolt could not allocate a body for '{}'.", object.GetName());
  else
  {
    const auto id        = nativeWorld.FindBody(actor.GetHandle());
    auto&      bodyOwner = *nativeWorld.FindOwnerMutable(id);
    bodyOwner.m_hSurface = actor.GetSurface();
    for (auto& child : shapeSet.m_Shapes)
      for (auto& surface : child.m_Surfaces)
      {
        surface                   = bodyOwner;
        surface.m_hShapeObject    = child.m_hObject;
        surface.m_hShapeComponent = child.m_hComponent;
      }
    nativeWorld.m_ShapeSets.Insert(id.GetIndexAndSequenceNumber(), std::move(shapeSet));
  }
}

bool xiiJoltPhysicsWorldModule::GetKinematicState(const xiiJoltKinematicActorComponent& actor, xiiJoltKinematicActorComponent::BodyState& out_state) const
{
  if (!m_pNativeWorld)
    return false;

  const auto        id = m_pNativeWorld->FindBody(actor.GetHandle());
  JPH::BodyLockRead lock(m_pNativeWorld->m_PhysicsSystem.GetBodyLockInterface(), id);
  if (!lock.Succeeded() || !lock.GetBody().IsKinematic())
    return false;

  const auto& body             = lock.GetBody();
  out_state.m_vPosition        = FromJoltVector(body.GetPosition());
  out_state.m_qRotation        = FromJoltRotation(body.GetRotation());
  out_state.m_vLinearVelocity  = FromJoltVector(body.GetLinearVelocity());
  out_state.m_vAngularVelocity = FromJoltVector(body.GetAngularVelocity());
  return true;
}

void xiiJoltPhysicsWorldModule::ApplyKinematicTeleports()
{
  auto& native = *m_pNativeWorld;
  auto& bodies = native.m_PhysicsSystem.GetBodyInterfaceNoLock();
  for (auto it = native.m_KinematicActors.GetIterator(); it.IsValid(); ++it)
  {
    const auto                      hActor = it.Key();
    xiiJoltKinematicActorComponent* pActor = nullptr;
    if (!GetWorld()->TryGetComponent(hActor, pActor) || !pActor->m_bTeleportPending)
      continue;

    const auto id = native.FindBody(hActor);
    bodies.SetPositionAndRotation(id, JPH::RVec3(ToJoltVector(pActor->m_vTeleportPosition)), ToJoltRotation(pActor->m_qTeleportRotation), JPH::EActivation::Activate);
    bodies.SetLinearAndAngularVelocity(id, JPH::Vec3::sZero(), JPH::Vec3::sZero());
    bodies.InvalidateContactCache(id);
    native.m_TeleportedBodies.Insert(id.GetIndexAndSequenceNumber());
#ifdef JPH_DEBUG_RENDERER
    native.m_DebugContacts.Clear();
    native.m_bDebugContactsTruncated = false;
    native.m_uiDebugFrame            = xiiInvalidIndex;
#endif
    pActor->m_bTeleportPending = false;
  }
}

void xiiJoltPhysicsWorldModule::CaptureKinematicTargets()
{
  auto& native = *m_pNativeWorld;
  for (auto it = native.m_KinematicActors.GetIterator(); it.IsValid(); ++it)
  {
    xiiJoltKinematicActorComponent* pActor = nullptr;
    if (!GetWorld()->TryGetComponent(it.Key(), pActor) || pActor->m_bUseVelocity)
      continue;

    auto* pOwner = pActor->GetOwner();
    pOwner->UpdateGlobalTransform();
    const auto& transform = pOwner->GetGlobalTransform();
    auto&       target    = it.Value();
    target.m_bValid       = transform.m_vPosition.IsValid() && transform.m_qRotation.IsValid();
    if (target.m_bValid)
    {
      target.m_Position = JPH::RVec3(ToJoltVector(transform.m_vPosition));
      target.m_Rotation = ToJoltRotation(transform.m_qRotation);
    }
  }
}

void xiiJoltPhysicsWorldModule::UpdateKinematicBodies(float fTimeToTarget)
{
  auto& native = *m_pNativeWorld;
  auto& bodies = native.m_PhysicsSystem.GetBodyInterfaceNoLock();
  for (auto it = native.m_KinematicActors.GetIterator(); it.IsValid(); ++it)
  {
    const auto                      hActor = it.Key();
    xiiJoltKinematicActorComponent* pActor = nullptr;
    if (!GetWorld()->TryGetComponent(hActor, pActor))
      continue;

    const auto id = native.FindBody(hActor);
    if (pActor->m_bUseVelocity)
    {
      bodies.SetLinearAndAngularVelocity(id, ToJoltVector(pActor->m_vLinearVelocity), ToJoltVector(pActor->m_vAngularVelocity));
      continue;
    }

    const auto& target = it.Value();
    const auto* pBody  = native.m_PhysicsSystem.GetBodyLockInterfaceNoLock().TryGetBody(id);
    if (!pBody)
      continue;

    if (target.m_bValid)
    {
      const auto targetCenter = target.m_Position + target.m_Rotation * pBody->GetShape()->GetCenterOfMass();
      const auto delta        = JPH::Vec3(targetCenter - pBody->GetCenterOfMassPosition());
      if (xiiMath::IsFinite((delta / fTimeToTarget).LengthSq()) && xiiMath::IsFinite(xiiMath::Square(JPH::JPH_PI / fTimeToTarget)))
      {
        bodies.MoveKinematic(id, target.m_Position, target.m_Rotation, fTimeToTarget);
        continue;
      }
    }
    bodies.SetLinearAndAngularVelocity(id, JPH::Vec3::sZero(), JPH::Vec3::sZero());
  }
}

void xiiJoltPhysicsWorldModule::QueueCharacterUpdate(xiiComponentHandle hCharacter)
{
  if (m_pNativeWorld)
    m_pNativeWorld->m_PendingCharacters.Insert(hCharacter);
}

void xiiJoltPhysicsWorldModule::RemoveCharacter(xiiComponentHandle hCharacter)
{
  if (m_pNativeWorld)
  {
    m_pNativeWorld->m_PendingCharacters.Remove(hCharacter);
    m_pNativeWorld->m_Characters.Remove(hCharacter);
  }
}

void xiiJoltPhysicsWorldModule::UpdateCharacterConfigurations()
{
  auto& native = *m_pNativeWorld;
  for (const auto hCharacter : native.m_PendingCharacters)
  {
    xiiJoltCharacterControllerComponent* pComponent = nullptr;
    if (!GetWorld()->TryGetComponent(hCharacter, pComponent) || !pComponent->IsActiveAndSimulating() || !pComponent->GetSimulationEnabled())
    {
      native.m_Characters.Remove(hCharacter);
      continue;
    }

    pComponent->GetOwner()->UpdateGlobalTransform();
    const auto*   pPrevious    = native.m_Characters.GetValue(hCharacter);
    const xiiVec3 vPosition    = pComponent->m_bTeleportPending ? pComponent->m_vTeleportPosition :
      pPrevious                                                 ? pPrevious->m_State.m_vPosition :
                                                                  pComponent->GetOwner()->GetGlobalPosition();
    const float   fRadius      = pComponent->GetRadius();
    const float   fHalfSegment = 0.5f * pComponent->GetHeight() - fRadius;
    if (fHalfSegment < 0.0f || pComponent->GetContactPadding() >= fRadius || !vPosition.IsValid())
    {
      xiiLog::Warning("Jolt character '{}' requires a valid foot position, height at least twice the radius, and contact padding smaller than its radius.",
                      pComponent->GetOwner()->GetName());
      native.m_Characters.Remove(hCharacter);
      continue;
    }

    const bool                bShapeChanged = !pPrevious || pPrevious->m_fRadius != fRadius || pPrevious->m_fHeight != pComponent->GetHeight();
    JPH::RefConst<JPH::Shape> shape;
    if (bShapeChanged)
    {
      auto capsule = JPH::CapsuleShapeSettings(fHalfSegment, fRadius).Create();
      if (capsule.HasError())
      {
        xiiLog::Warning("Jolt could not create character capsule: {}", capsule.GetError().c_str());
        continue;
      }

      auto result = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0.0f, 0.0f, 0.5f * pComponent->GetHeight()),
                                                        JPH::Quat::sRotation(JPH::Vec3::sAxisX(), JPH::JPH_PI * 0.5f), capsule.Get())
                      .Create();
      if (result.HasError())
      {
        xiiLog::Warning("Jolt could not place character capsule: {}", result.GetError().c_str());
        continue;
      }
      shape = result.Get();
    }
    else
      shape = pPrevious->m_pCharacter->GetShape();

    JPH::CharacterVirtualSettings settings;
    const bool                    bCheckClearance = pPrevious && bShapeChanged && !pComponent->m_bTeleportPending;
    settings.mShape                               = bCheckClearance ? pPrevious->m_pCharacter->GetShape() : shape.GetPtr();
    settings.mUp                                  = JPH::Vec3::sAxisZ();
    settings.mSupportingVolume                    = JPH::Plane(settings.mUp, -fRadius);
    settings.mMaxSlopeAngle                       = pComponent->GetMaxSlope().GetRadian();
    settings.mCollisionTolerance                  = pComponent->GetContactTolerance();
    settings.mPredictiveContactDistance           = xiiMath::Max(settings.mPredictiveContactDistance, pComponent->GetContactPadding());
    settings.mCharacterPadding                    = pComponent->GetContactPadding();
    settings.mMaxCollisionIterations              = pComponent->GetMaxCollisionIterations();
    settings.mEnhancedInternalEdgeRemoval         = true;

    NativeWorld::Character entry;
    entry.m_pCharacter = new JPH::CharacterVirtual(&settings, JPH::RVec3(ToJoltVector(vPosition)), JPH::Quat::sIdentity(), &native.m_PhysicsSystem);
    if (bCheckClearance)
    {
      const auto layer  = xiiJoltCollisionFilters::MakeObjectLayer(pComponent->GetCollisionLayer(), JPH::EMotionType::Kinematic);
      const auto hOwner = pComponent->GetOwner()->GetHandle();
      if (!entry.m_pCharacter->SetShape(shape, settings.mCollisionTolerance, native.m_PhysicsSystem.GetDefaultBroadPhaseLayerFilter(layer),
                                        native.m_PhysicsSystem.GetDefaultLayerFilter(layer), NativeWorld::CharacterBodyFilter(native, hOwner),
                                        NativeWorld::CharacterShapeFilter(native, hOwner), native.m_TempAllocator))
      {
        xiiLog::Warning("Jolt character '{}' cannot change its collision shape at the current position.", pComponent->GetOwner()->GetName());
        continue;
      }
    }
    entry.m_pCharacter->SetListener(&native.m_CharacterContactListener);
    entry.m_State.m_vPosition = vPosition;
    entry.m_fRadius           = fRadius;
    entry.m_fHeight           = pComponent->GetHeight();
    if (pPrevious)
      entry.m_fVerticalVelocity = pPrevious->m_fVerticalVelocity;
    native.m_Characters.Insert(hCharacter, std::move(entry));
  }
  native.m_PendingCharacters.Clear();
}

bool xiiJoltPhysicsWorldModule::GetCharacterState(xiiComponentHandle hCharacter, xiiJoltCharacterControllerComponent::State& out_state) const
{
  const auto* pEntry = m_pNativeWorld ? m_pNativeWorld->m_Characters.GetValue(hCharacter) : nullptr;
  if (!pEntry)
    return false;

  out_state     = pEntry->m_State;
  const auto id = pEntry->m_pCharacter->GetGroundBodyID();
  if (!id.IsInvalid() && (!m_pNativeWorld->FindOwner(id) || m_pNativeWorld->m_TeleportedBodies.Contains(id.GetIndexAndSequenceNumber())))
  {
    out_state.m_GroundState = xiiJoltCharacterGroundState::InAir;
    out_state.m_hGroundObject.Invalidate();
    out_state.m_vGroundNormal.SetZero();
    out_state.m_vGroundVelocity.SetZero();
  }
  return true;
}

void xiiJoltPhysicsWorldModule::StepCharacters(float fStep)
{
  auto& native = *m_pNativeWorld;
  auto& system = native.m_PhysicsSystem;
  for (auto it = native.m_Characters.GetIterator(); it.IsValid(); ++it)
  {
    xiiJoltCharacterControllerComponent* pComponent = nullptr;
    if (!GetWorld()->TryGetComponent(it.Key(), pComponent) || !pComponent->GetSimulationEnabled())
      continue;

    auto&                                   entry       = it.Value();
    auto&                                   character   = *entry.m_pCharacter;
    const auto                              layer       = xiiJoltCollisionFilters::MakeObjectLayer(pComponent->GetCollisionLayer(), JPH::EMotionType::Kinematic);
    const auto                              broadPhase  = system.GetDefaultBroadPhaseLayerFilter(layer);
    const auto                              objectLayer = system.GetDefaultLayerFilter(layer);
    const auto                              hOwner      = pComponent->GetOwner()->GetHandle();
    const NativeWorld::CharacterBodyFilter  bodyFilter(native, hOwner);
    const NativeWorld::CharacterShapeFilter shapeFilter(native, hOwner);

    character.SetMaxSlopeAngle(pComponent->GetMaxSlope().GetRadian());
    character.SetMass(pComponent->GetMass());
    character.SetMaxStrength(pComponent->GetMaxStrength());
    character.SetPenetrationRecoverySpeed(pComponent->GetPenetrationRecoverySpeed());

    const auto id = character.GetGroundBodyID();
    if (pComponent->m_bTeleportPending)
    {
      character.SetPosition(JPH::RVec3(ToJoltVector(pComponent->m_vTeleportPosition)));
      character.SetLinearVelocity(JPH::Vec3::sZero());
      pComponent->m_bTeleportPending = false;
      entry.m_fVerticalVelocity      = 0.0f;
      entry.m_bRefreshContacts       = true;
    }
    if (entry.m_bRefreshContacts || (!id.IsInvalid() && (!native.FindOwner(id) || native.m_TeleportedBodies.Contains(id.GetIndexAndSequenceNumber()))))
    {
      character.RefreshContacts(broadPhase, objectLayer, bodyFilter, shapeFilter, native.m_TempAllocator);
      entry.m_bRefreshContacts = false;
    }

    const bool bGrounded = character.GetGroundState() == JPH::CharacterBase::EGroundState::OnGround && entry.m_fVerticalVelocity <= 0.0f;
    if (bGrounded)
      character.UpdateGroundVelocity();
    const auto groundVelocity = bGrounded ? character.GetGroundVelocity() : JPH::Vec3::sZero();
    const bool bJump          = bGrounded && pComponent->m_fJumpSpeed > 0.0f;
    if (bJump)
      entry.m_fVerticalVelocity = pComponent->m_fJumpSpeed + groundVelocity.GetZ();
    else if (bGrounded)
      entry.m_fVerticalVelocity = 0.0f;
    pComponent->m_fJumpSpeed = 0.0f;

    const auto gravity = pComponent->GetGravityFactor() * system.GetGravity();
    entry.m_fVerticalVelocity += gravity.GetZ() * fStep;
    auto velocity = ToJoltVector(pComponent->m_vDesiredVelocity) + JPH::Vec3(0.0f, 0.0f, entry.m_fVerticalVelocity);
    if (bGrounded)
      velocity += bJump ? JPH::Vec3(groundVelocity.GetX(), groundVelocity.GetY(), 0.0f) : groundVelocity;
    if (!xiiMath::IsFinite(velocity.LengthSq()) || !xiiMath::IsFinite((velocity * fStep).LengthSq()))
    {
      entry.m_fVerticalVelocity = 0.0f;
      velocity                  = JPH::Vec3::sZero();
    }
    character.SetLinearVelocity(velocity);
    const auto previous = character.GetPosition();

    JPH::CharacterVirtual::ExtendedUpdateSettings settings;
    settings.mStickToFloorStepDown = bJump || entry.m_fVerticalVelocity > 0.0f ? JPH::Vec3::sZero() : JPH::Vec3(0.0f, 0.0f, -pComponent->GetStepDownDistance());
    settings.mWalkStairsStepUp     = bGrounded && !bJump ? JPH::Vec3(0.0f, 0.0f, pComponent->GetStepUpHeight()) : JPH::Vec3::sZero();
    character.ExtendedUpdate(fStep, gravity, settings, broadPhase, objectLayer, bodyFilter, shapeFilter, native.m_TempAllocator);

    auto& state       = entry.m_State;
    state.m_vPosition = FromJoltVector(character.GetPosition());
    state.m_vVelocity = FromJoltVector(JPH::Vec3(character.GetPosition() - previous) / fStep);
    // Discard upward momentum lost to ceilings. Continue integrating gravity on unwalkable slopes.
    if (entry.m_fVerticalVelocity > 0.0f)
      entry.m_fVerticalVelocity = xiiMath::Clamp(state.m_vVelocity.z, 0.0f, entry.m_fVerticalVelocity);

    state.m_GroundState     = static_cast<xiiJoltCharacterGroundState::Enum>(character.GetGroundState());
    state.m_vGroundNormal   = FromJoltVector(character.GetGroundNormal());
    state.m_vGroundVelocity = FromJoltVector(character.GetGroundVelocity());
    state.m_hGroundObject.Invalidate();
    if (const auto* pGround = native.FindOwner(character.GetGroundBodyID()))
      state.m_hGroundObject = pGround->m_hObject;

    state.m_bTouchingDynamicBody = false;
    for (const auto& contact : character.GetActiveContacts())
    {
      if (contact.mHadCollision && !contact.mWasDiscarded && !contact.mIsSensorB && contact.mMotionTypeB == JPH::EMotionType::Dynamic)
      {
        state.m_bTouchingDynamicBody = true;
        break;
      }
    }
  }
  native.m_TeleportedBodies.Clear();
}

void xiiJoltPhysicsWorldModule::SynchronizeMovedObjects()
{
  auto&       nativeWorld = *m_pNativeWorld;
  auto&       poses       = nativeWorld.m_PosesToSync;
  const auto& bodies      = nativeWorld.m_PhysicsSystem.GetBodyInterfaceNoLock();
  for (auto it = nativeWorld.GetBodyOwners().GetIterator(); it.IsValid(); ++it)
  {
    const auto& owner = it.Value();
    if (!owner.m_bDynamic && !nativeWorld.m_KinematicActors.Contains(owner.m_hComponent))
      continue;

    if (!owner.m_bDynamic)
    {
      const xiiJoltKinematicActorComponent* pActor = nullptr;
      if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(owner.m_hComponent, pActor) || !pActor->m_bUseVelocity)
        continue;
    }

    xiiGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(owner.m_hObject, pObject))
      continue;

    JPH::RVec3 position;
    JPH::Quat  rotation;
    bodies.GetPositionAndRotation(JPH::BodyID(it.Key()), position, rotation);
    auto& pose           = poses.ExpandAndGetRef();
    pose.m_hObject       = owner.m_hObject;
    pose.m_vPosition     = xiiVec3(static_cast<float>(position.GetX()), static_cast<float>(position.GetY()), static_cast<float>(position.GetZ()));
    pose.m_qRotation     = xiiQuat(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
    pose.m_bPositionOnly = false;
    pose.m_uiDepth       = 0;
    for (auto* pParent = pObject->GetParent(); pParent != nullptr; pParent = pParent->GetParent())
      ++pose.m_uiDepth;
  }

  for (auto it = nativeWorld.m_Characters.GetIterator(); it.IsValid(); ++it)
  {
    const xiiJoltCharacterControllerComponent* pComponent = nullptr;
    if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(it.Key(), pComponent))
      continue;

    auto& pose           = poses.ExpandAndGetRef();
    pose.m_hObject       = pComponent->GetOwner()->GetHandle();
    pose.m_vPosition     = it.Value().m_State.m_vPosition;
    pose.m_bPositionOnly = true;
    pose.m_uiDepth       = 0;
    for (auto* pParent = pComponent->GetOwner()->GetParent(); pParent != nullptr; pParent = pParent->GetParent())
      ++pose.m_uiDepth;
  }

  poses.Sort([](const auto& a, const auto& b) { return a.m_uiDepth < b.m_uiDepth; });
  for (const auto& pose : poses)
  {
    xiiGameObject* pObject = nullptr;
    if (!GetWorld()->TryGetObject(pose.m_hObject, pObject))
      continue;

    if (pose.m_uiDepth != 0)
      pObject->UpdateGlobalTransform();

    auto transform = pObject->GetGlobalTransform();
    if (pose.m_bPositionOnly)
    {
      if (transform.m_vPosition != pose.m_vPosition)
        pObject->SetGlobalPosition(pose.m_vPosition);
      continue;
    }
    const xiiQuat qOpposite(-pose.m_qRotation.x, -pose.m_qRotation.y, -pose.m_qRotation.z, -pose.m_qRotation.w);
    if (transform.m_vPosition == pose.m_vPosition && (transform.m_qRotation == pose.m_qRotation || transform.m_qRotation == qOpposite))
      continue;

    transform.m_vPosition = pose.m_vPosition;
    transform.m_qRotation = pose.m_qRotation;
    pObject->SetGlobalTransform(transform);
  }
  poses.Clear();
}

void xiiJoltPhysicsWorldModule::Simulate(const UpdateContext&)
{
  auto& nativeWorld                = *m_pNativeWorld;
  nativeWorld.m_LastUpdateDuration = xiiTime::MakeZero();
  nativeWorld.m_uiLastSteps        = 0;
  nativeWorld.m_uiLastUpdateErrors = 0;
#ifdef JPH_DEBUG_RENDERER
  const bool bCaptureContacts = std::exchange(nativeWorld.m_bDebugContactsRequested, false);
#endif
  UpdateShapes();
  auto& pendingBodies = m_pNativeWorld->m_PendingBodies;
  for (const auto hActor : pendingBodies)
  {
    xiiJoltActorComponent* pActor = nullptr;
    if (GetWorld()->TryGetComponent(hActor, pActor) && pActor->IsActiveAndSimulating())
      RebuildBody(*pActor);
  }
  pendingBodies.Clear();
  UpdateJoints();
  UpdateMaterials();
  ApplyKinematicTeleports();
  UpdateCharacterConfigurations();
  ApplyQueuedImpulses();

  const xiiClock& clock         = GetWorld()->GetClock();
  const double    fDeltaSeconds = clock.GetTimeDiff().GetSeconds();
  if (clock.GetPaused() || fDeltaSeconds <= 0.0 || !xiiMath::IsFinite(fDeltaSeconds) ||
      (m_pNativeWorld->m_PhysicsSystem.GetNumBodies() == 0 && m_pNativeWorld->m_Characters.IsEmpty()))
    return;

  m_AccumulatedTime += xiiTime::MakeFromSeconds(fDeltaSeconds);
  if (m_AccumulatedTime < m_FixedStep)
    return;

  const double fStepSeconds  = m_FixedStep.GetSeconds();
  const double fElapsedSteps = xiiMath::Floor(m_AccumulatedTime.GetSeconds() / fStepSeconds);
  const auto   uiSteps       = static_cast<xiiUInt32>(xiiMath::Min(fElapsedSteps, static_cast<double>(m_uiMaxCatchUpSteps)));

  // Drop whole steps beyond the catch-up limit and retain the fractional remainder.
  if (fElapsedSteps > m_uiMaxCatchUpSteps)
  {
    m_AccumulatedTime = xiiTime::MakeFromSeconds(xiiMath::Mod(m_AccumulatedTime.GetSeconds(), fStepSeconds));
  }
  else
  {
    m_AccumulatedTime = xiiTime::MakeFromSeconds(xiiMath::Max(0.0, m_AccumulatedTime.GetSeconds() - fStepSeconds * uiSteps));
  }

  [[maybe_unused]] const JPH::FPFlushDenormals flushDenormals;
  auto                                         errors = JPH::EPhysicsUpdateError::None;
  const float                                  fStep  = m_FixedStep.AsFloatInSeconds();
  XII_PROFILE_SCOPE("Jolt Physics");
#ifdef JPH_DEBUG_RENDERER
  nativeWorld.m_DebugContacts.Clear();
  nativeWorld.m_bDebugContactsTruncated = false;
  nativeWorld.m_bCaptureDebugContacts   = bCaptureContacts;
  if (bCaptureContacts)
    nativeWorld.m_DebugContacts.Reserve(MaxDebugContactPoints);
#endif
  CaptureKinematicTargets();
  for (xiiUInt32 uiStep = 0; uiStep < uiSteps; ++uiStep)
  {
    // Spread the authored target across the executed steps and let characters read the
    // current platform velocities before Jolt integrates the rigid bodies.
    UpdateKinematicBodies(fStep * static_cast<float>(uiSteps - uiStep));
    StepCharacters(fStep);
    if (nativeWorld.m_PhysicsSystem.GetNumBodies() != 0)
    {
      const xiiTime start = xiiTime::Now();
      errors |= nativeWorld.m_PhysicsSystem.Update(fStep, 1, &nativeWorld.m_TempAllocator, &nativeWorld.m_JobSystem);
      nativeWorld.m_LastUpdateDuration += xiiTime::Now() - start;
      CheckJointBreakage(fStep);
    }
  }
  nativeWorld.m_uiLastSteps        = uiSteps;
  nativeWorld.m_uiLastUpdateErrors = static_cast<xiiUInt32>(errors);
#ifdef JPH_DEBUG_RENDERER
  nativeWorld.m_bCaptureDebugContacts = false;
#endif

  SynchronizeMovedObjects();

  if (errors != JPH::EPhysicsUpdateError::None)
  {
    xiiLog::Warning("Jolt simulation in world '{}' exceeded its contact capacity (flags {}).", GetWorld()->GetName(), static_cast<xiiUInt32>(errors));
  }
}

void xiiJoltPhysicsWorldModule::QueueImpulse(xiiComponentHandle hActor, const xiiMsgPhysicsAddImpulse& msg) const
{
  if (!m_pNativeWorld || msg.m_uiImpulseType != 0 || !msg.m_vImpulse.IsValid() || !msg.m_vGlobalPosition.IsValid())
    return;

  const auto* pOwner = m_pNativeWorld->FindOwner(m_pNativeWorld->FindBody(hActor));
  if (pOwner == nullptr || !pOwner->m_bDynamic ||
      (msg.m_uiObjectFilterID != xiiInvalidIndex && msg.m_uiObjectFilterID != pOwner->m_uiFilterId))
    return;

  XII_LOCK(m_pNativeWorld->m_ImpulseMutex);
  m_pNativeWorld->m_QueuedImpulses.PushBack({hActor, msg.m_vGlobalPosition, msg.m_vImpulse, pOwner->m_uiFilterId});
}

void xiiJoltPhysicsWorldModule::ApplyQueuedImpulses()
{
  // PreAsync owns the world; all producers from the preceding frame have finished.
  auto& native = *m_pNativeWorld;
  auto& bodies = native.m_PhysicsSystem.GetBodyInterface();
  for (const auto& impulse : native.m_QueuedImpulses)
  {
    const auto  id     = native.FindBody(impulse.m_hActor);
    const auto* pOwner = native.FindOwner(id);
    if (pOwner != nullptr && pOwner->m_bDynamic && pOwner->m_uiFilterId == impulse.m_uiFilterId)
      bodies.AddImpulse(id, ToJoltVector(impulse.m_vImpulse), JPH::RVec3(ToJoltVector(impulse.m_vPosition)));
  }
  native.m_QueuedImpulses.Clear();
}

void xiiJoltPhysicsWorldModule::DispatchContacts(const UpdateContext&)
{
  auto& native = *m_pNativeWorld;
  for (auto hJoint : native.m_BrokenJoints)
  {
    xiiJoltJointComponent* pJoint = nullptr;
    if (GetWorld()->TryGetComponent(hJoint, pJoint) && pJoint->IsActiveAndSimulating())
    {
      xiiMsgPhysicsJointBroke msg;
      msg.m_hJointObject = pJoint->GetOwner()->GetHandle();
      pJoint->GetOwner()->PostEventMessage(msg, pJoint, xiiTime::MakeZero(), xiiObjectMsgQueueType::PostTransform);
    }
  }
  native.m_BrokenJoints.Clear();
  // Surface interactions may instantiate scene objects. Keep events appended by lifecycle changes separate.
  native.m_DispatchContactEvents.Swap(native.m_ContactEvents);
  for (const auto& contact : native.m_DispatchContactEvents)
  {
    if (auto* pCached = native.m_Contacts.GetValue(contact.m_uiKey))
      pCached->m_uiEventIndex = xiiInvalidIndex;

    if (contact.m_Phase == xiiPhysicsContactPhase::Added && contact.m_fImpactSqr > 0.0f)
    {
      // Prefer the stationary surface as the interaction target, using the striking object's interaction name.
      const xiiUInt32 uiTarget    = !contact.m_Owners[0].m_bDynamic ? 0 : 1;
      const auto&     target      = contact.m_Owners[uiTarget];
      const auto&     source      = contact.m_Owners[1 - uiTarget];
      const auto      hSurface    = target.m_hSurface.IsValid() ? target.m_hSurface : source.m_hSurface;
      const auto&     interaction = source.m_hSurface.IsValid() ? source.m_sCollisionInteraction : target.m_sCollisionInteraction;
      const auto*     pCurrentA   = native.FindOwner(native.FindBody(contact.m_Owners[0].m_hComponent));
      const auto*     pCurrentB   = native.FindOwner(native.FindBody(contact.m_Owners[1].m_hComponent));
      if (hSurface.IsValid() && !interaction.IsEmpty() && pCurrentA != nullptr && pCurrentB != nullptr &&
          pCurrentA->m_uiFilterId == contact.m_Owners[0].m_uiFilterId && pCurrentB->m_uiFilterId == contact.m_Owners[1].m_uiFilterId)
      {
        const xiiResourceLock<xiiSurfaceResource> surface(hSurface, xiiResourceAcquireMode::PointerOnly);
        if (surface->GetLoadingState() == xiiResourceState::Loaded)
        {
          const xiiVec3 vNormal = uiTarget == 0 ? contact.m_vNormal : -contact.m_vNormal;
          surface->InteractWithSurface(GetWorld(), target.m_hObject, contact.m_vPosition, vNormal, -vNormal, interaction, nullptr, contact.m_fImpactSqr);
        }
      }
    }

    for (xiiUInt32 i = 0; i < 2; ++i)
    {
      if (!contact.m_bReport[i])
        continue;

      const auto&         owner      = contact.m_Owners[i];
      const xiiComponent* pComponent = nullptr;
      if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(owner.m_hComponent, pComponent))
        continue;
      const auto* pActor = xiiDynamicCast<const xiiJoltActorComponent*>(pComponent);
      const auto* pGroup = xiiDynamicCast<const xiiJoltStaticCollisionComponent*>(pComponent);
      if (!(pActor && pActor->IsActiveAndSimulating() && pActor->GetReportContacts()) &&
          !(pGroup && pGroup->IsActiveAndInitialized() && pGroup->GetReportContacts()))
        continue;

      if (contact.m_Phase != xiiPhysicsContactPhase::Removed)
      {
        const auto* pCurrent = native.FindOwner(native.FindBody(owner.m_hComponent));
        if (pCurrent == nullptr || pCurrent->m_uiFilterId != owner.m_uiFilterId)
          continue;
      }

      xiiMsgPhysicContact msg;
      msg.m_vGlobalPosition       = contact.m_vPosition;
      msg.m_vNormal               = i == 0 ? contact.m_vNormal : -contact.m_vNormal;
      msg.m_fImpactSqr            = contact.m_fImpactSqr;
      msg.m_Phase                 = contact.m_Phase;
      const xiiGameObject* pOther = nullptr;
      const auto&          other  = contact.m_Owners[1 - i];
      if (static_cast<const xiiWorld*>(GetWorld())->TryGetObject(other.m_bGenerated ? other.m_hShapeObject : other.m_hObject, pOther))
        msg.m_hOtherObject = pOther->GetHandle();

      GetWorld()->PostMessage(owner.m_hObject, msg, xiiTime::MakeZero(), xiiObjectMsgQueueType::PostTransform);
    }
  }
  native.m_DispatchContactEvents.Clear();
}

namespace
{
  bool IsPositive(float fValue)
  {
    return xiiMath::IsFinite(fValue) && fValue > 0.0f;
  }

  bool IsValidQueryShape(const xiiTransform& transform)
  {
    return transform.m_vScale.IsValid() && transform.m_qRotation.IsValid();
  }

  bool GetTravel(const xiiVec3& vDir, float fDistance, JPH::Vec3& out_travel)
  {
    if (!vDir.IsValid() || !IsPositive(fDistance))
      return false;

    // Rescale before normalizing to avoid overflow or underflow for valid finite directions.
    const auto  magnitude = vDir.Abs();
    const float fMax      = xiiMath::Max(magnitude.x, magnitude.y, magnitude.z);
    if (fMax == 0.0f)
      return false;

    const auto direction = JPH::Vec3(vDir.x / fMax, vDir.y / fMax, vDir.z / fMax);
    out_travel           = direction.Normalized() * fDistance;
    return out_travel != JPH::Vec3::sZero();
  }

  template <typename Native>
  struct QueryFilters final : JPH::BroadPhaseLayerFilter, JPH::ObjectLayerFilter, JPH::BodyFilter, JPH::ShapeFilter
  {
    QueryFilters(const Native& native, const xiiWorld& world, const xiiPhysicsQueryParameters& params) :
      m_Native(native), m_World(world), m_Params(params)
    {
    }

    bool ShouldCollide(JPH::BroadPhaseLayer layer) const override
    {
      return m_Params.m_ShapeTypes.IsSet(layer == StaticBroadPhase ? xiiPhysicsShapeType::Static : xiiPhysicsShapeType::Dynamic);
    }

    bool ShouldCollide(JPH::ObjectLayer layer) const override
    {
      return m_Params.m_uiCollisionLayer < CollisionLayerCount &&
        m_Native.m_CollisionFilters.AreLayersEnabled(static_cast<xiiUInt8>(m_Params.m_uiCollisionLayer), static_cast<xiiUInt8>(layer));
    }

    bool ShouldCollideLocked(const JPH::Body& body) const override
    {
      if (m_Params.m_uiIgnoreObjectFilterID == body.GetCollisionGroup().GetGroupID())
        return false;

      const auto*                  pOwner = m_Native.FindOwner(body.GetID());
      const xiiJoltActorComponent* pActor = nullptr;
      return pOwner != nullptr && m_World.TryGetComponent(pOwner->m_hComponent, pActor) && pActor->IsActiveAndSimulating();
    }

    bool ShouldCollide(const JPH::Shape* pShape, const JPH::SubShapeID& id) const override
    {
      const auto* pSet = m_Native.m_ShapeSets.GetValue(mBodyID2.GetIndexAndSequenceNumber());
      if (pSet == nullptr || pSet->m_Shapes.IsEmpty())
        return false;
      if (pSet->m_Shapes.GetCount() > 1 && pShape == pSet->m_pRoot.GetPtr())
        return true;
      const auto*                  pOwner     = Native::FindShapeOwner(*pSet, id);
      const xiiJoltShapeComponent* pComponent = nullptr;
      return pOwner && m_World.TryGetComponent(pOwner->m_hComponent, pComponent) && pComponent->IsActiveAndInitialized();
    }

    bool ShouldCollide(const JPH::Shape*, const JPH::SubShapeID&, const JPH::Shape* pShape, const JPH::SubShapeID& id) const override
    {
      return ShouldCollide(pShape, id);
    }

    const Native&                   m_Native;
    const xiiWorld&                 m_World;
    const xiiPhysicsQueryParameters m_Params;
  };

  template <typename Hit>
  class RayCollector final : public JPH::CastRayCollector
  {
  public:
    RayCollector(Hit& ref_hit, bool bIgnoreInitial) :
      m_Hit(ref_hit), m_bIgnoreInitial(bIgnoreInitial)
    {
    }

    void AddHit(const JPH::RayCastResult& hit) override
    {
      if (!m_bIgnoreInitial || hit.mFraction > 0.0f)
        m_Hit(hit, *this);
    }

  private:
    Hit& m_Hit;
    bool m_bIgnoreInitial;
  };

  template <typename Owner, typename Result>
  void FillQueryOwner(const Owner& owner, Result& ref_result)
  {
    ref_result.m_hActorObject     = owner.m_hObject;
    ref_result.m_hShapeObject     = owner.m_hShapeObject;
    ref_result.m_uiObjectFilterID = owner.m_uiFilterId;
    if constexpr (requires { ref_result.m_hSurface; })
      ref_result.m_hSurface = owner.m_hSurface;
  }

  xiiTransform GetAxialQueryPose(const xiiTransform& transform)
  {
    auto pose        = transform;
    pose.m_qRotation = pose.m_qRotation * xiiQuat::MakeFromAxisAndAngle(xiiVec3(1.0f, 0.0f, 0.0f), xiiAngle::MakeFromDegree(90.0f));
    return pose;
  }
} // namespace

xiiUInt32 xiiJoltPhysicsWorldModule::GetCollisionLayerByName(xiiStringView sName) const
{
  if (m_pNativeWorld)
  {
    const auto layer = m_pNativeWorld->m_CollisionFilters.m_Config.FindLayer(sName);
    if (layer != xiiInvalidIndex)
      return layer;
  }

  xiiUInt32   uiLayer = 0;
  const char* szEnd   = nullptr;
  if (xiiConversionUtils::StringToUInt(sName, uiLayer, &szEnd).Succeeded() && szEnd == sName.GetEndPointer() && uiLayer < CollisionLayerCount)
    return uiLayer;
  return xiiInvalidIndex;
}

xiiUInt8 xiiJoltPhysicsWorldModule::GetWeightCategoryByName(xiiStringView) const
{
  return xiiMath::MaxValue<xiiUInt8>(); // XII has no weight category table.
}

xiiUInt8 xiiJoltPhysicsWorldModule::GetImpulseTypeByName(xiiStringView) const
{
  return xiiMath::MaxValue<xiiUInt8>(); // Only untyped impulses are supported.
}

xiiVec3 xiiJoltPhysicsWorldModule::GetGravity() const
{
  return FromJoltVector(m_pNativeWorld->m_PhysicsSystem.GetGravity());
}

bool xiiJoltPhysicsWorldModule::Raycast(xiiPhysicsCastResult& out_result, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  JPH::Vec3 travel;
  if (!m_pNativeWorld || !vStart.IsValid() || !GetTravel(vDir, fDistance, travel))
    return false;

  const JPH::RRayCast  ray(JPH::RVec3(ToJoltVector(vStart)), travel);
  const QueryFilters   filters(*m_pNativeWorld, *GetWorld(), params);
  JPH::RayCastSettings settings;
  settings.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;
  bool bFound                  = false;
  auto onHit                   = [&](const JPH::RayCastResult& hit, JPH::CastRayCollector& collector) {
    const auto* pOwner = m_pNativeWorld->ResolveOwner(hit.mBodyID, hit.mSubShapeID2);
    if (pOwner == nullptr || hit.mFraction >= collector.GetEarlyOutFraction())
      return;

    const auto position = ray.GetPointOnRay(hit.mFraction);
    out_result          = {};
    FillQueryOwner(*pOwner, out_result);
    out_result.m_vPosition = FromJoltVector(position);
    out_result.m_vNormal   = hit.mFraction == 0.0f ? FromJoltVector(-travel / fDistance) :
                                                     FromJoltVector(collector.GetContext()->GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, position));
    out_result.m_fDistance = hit.mFraction * fDistance;
    bFound                 = true;
    if (collection == xiiPhysicsHitCollection::Any)
      collector.ForceEarlyOut();
    else
      collector.UpdateEarlyOutFraction(hit.mFraction);
  };
  RayCollector collector(onHit, params.m_bIgnoreInitialOverlap);
  m_pNativeWorld->m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, filters, filters, filters, filters);
  return bFound;
}

bool xiiJoltPhysicsWorldModule::RaycastAll(xiiPhysicsCastResultArray& out_results, const xiiVec3& vStart, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();
  JPH::Vec3 travel;
  if (!m_pNativeWorld || !vStart.IsValid() || !GetTravel(vDir, fDistance, travel))
    return false;

  const JPH::RRayCast  ray(JPH::RVec3(ToJoltVector(vStart)), travel);
  const QueryFilters   filters(*m_pNativeWorld, *GetWorld(), params);
  JPH::RayCastSettings settings;
  settings.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;
  auto onHit                   = [&](const JPH::RayCastResult& hit, const JPH::CastRayCollector& collector) {
    const auto* pOwner = m_pNativeWorld->ResolveOwner(hit.mBodyID, hit.mSubShapeID2);
    if (pOwner == nullptr)
      return;
    const auto position = ray.GetPointOnRay(hit.mFraction);
    auto&      result   = out_results.m_Results.ExpandAndGetRef();
    result              = {};
    FillQueryOwner(*pOwner, result);
    result.m_vPosition = FromJoltVector(position);
    result.m_vNormal   = hit.mFraction == 0.0f ? FromJoltVector(-travel / fDistance) :
                                                 FromJoltVector(collector.GetContext()->GetWorldSpaceSurfaceNormal(hit.mSubShapeID2, position));
    result.m_fDistance = hit.mFraction * fDistance;
  };
  RayCollector collector(onHit, params.m_bIgnoreInitialOverlap);
  m_pNativeWorld->m_PhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, settings, collector, filters, filters, filters, filters);
  out_results.m_Results.Sort([](const auto& a, const auto& b) { return a.m_fDistance < b.m_fDistance; });
  return !out_results.m_Results.IsEmpty();
}

bool xiiJoltPhysicsWorldModule::SweepShape(xiiPhysicsCastResult& out_result, const JPH::Shape& shape, const xiiTransform& pose, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  JPH::Vec3 travel;
  if (!m_pNativeWorld || !pose.m_vPosition.IsValid() || !pose.m_qRotation.IsValid() || !GetTravel(vDir, fDistance, travel))
    return false;

  class SweepCollector final : public JPH::CastShapeCollector
  {
  public:
    SweepCollector(bool bIgnoreInitial, bool bAny) : m_bIgnoreInitial(bIgnoreInitial), m_bAny(bAny) {}
    void AddHit(const JPH::ShapeCastResult& hit) override
    {
      if ((m_bIgnoreInitial && hit.mFraction <= 0.0f) || hit.GetEarlyOutFraction() >= GetEarlyOutFraction())
        return;
      m_Result = hit;
      m_bFound = true;
      if (m_bAny)
        ForceEarlyOut();
      else
        UpdateEarlyOutFraction(hit.GetEarlyOutFraction());
    }

    JPH::ShapeCastResult m_Result;
    bool                 m_bFound = false;
    bool                 m_bIgnoreInitial;
    bool                 m_bAny;
  };

  const auto             origin    = JPH::RVec3(ToJoltVector(pose.m_vPosition));
  const auto             transform = JPH::RMat44::sRotationTranslation(ToJoltRotation(pose.m_qRotation), origin);
  const auto             cast      = JPH::RShapeCast::sFromWorldTransform(&shape, JPH::Vec3::sOne(), transform, travel);
  const QueryFilters     filters(*m_pNativeWorld, *GetWorld(), params);
  JPH::ShapeCastSettings settings;
  settings.mBackFaceModeConvex = JPH::EBackFaceMode::CollideWithBackFaces;
  settings.mReturnDeepestPoint = !params.m_bIgnoreInitialOverlap;
  SweepCollector collector(params.m_bIgnoreInitialOverlap, collection == xiiPhysicsHitCollection::Any);
  m_pNativeWorld->m_PhysicsSystem.GetNarrowPhaseQuery().CastShape(cast, settings, origin, collector, filters, filters, filters, filters);
  if (!collector.m_bFound)
    return false;

  const auto& hit    = collector.m_Result;
  const auto* pOwner = m_pNativeWorld->ResolveOwner(hit.mBodyID2, hit.mSubShapeID2);
  if (pOwner == nullptr)
    return false;

  out_result = {};
  FillQueryOwner(*pOwner, out_result);
  out_result.m_vPosition = FromJoltVector(origin + hit.mContactPointOn2);
  out_result.m_vNormal   = FromJoltVector((-hit.mPenetrationAxis).NormalizedOr(-travel / fDistance));
  out_result.m_fDistance = hit.mFraction * fDistance;
  return true;
}

bool xiiJoltPhysicsWorldModule::OverlapShape(xiiPhysicsOverlapResultArray* pResults, const JPH::Shape& shape, const xiiTransform& pose, const xiiPhysicsQueryParameters& params) const
{
  if (!m_pNativeWorld || !pose.m_vPosition.IsValid() || !pose.m_qRotation.IsValid())
    return false;

  const auto         origin    = JPH::RVec3(ToJoltVector(pose.m_vPosition));
  const auto         transform = JPH::RMat44::sRotationTranslation(ToJoltRotation(pose.m_qRotation), origin).PreTranslated(shape.GetCenterOfMass());
  const QueryFilters filters(*m_pNativeWorld, *GetWorld(), params);
  const auto&        query = m_pNativeWorld->m_PhysicsSystem.GetNarrowPhaseQuery();
  if (pResults == nullptr)
  {
    JPH::AnyHitCollisionCollector<JPH::CollideShapeCollector> collector;
    query.CollideShape(&shape, JPH::Vec3::sOne(), transform, {}, origin, collector, filters, filters, filters, filters);
    return collector.HadHit();
  }

  class OverlapCollector final : public JPH::CollideShapeCollector
  {
  public:
    OverlapCollector(const NativeWorld& native, xiiPhysicsOverlapResultArray& ref_results) :
      m_Native(native), m_Results(ref_results)
    {
    }

    void OnBody(const JPH::Body& body) override
    {
      m_Body    = body.GetID();
      m_vCenter = FromJoltVector(body.GetCenterOfMassPosition());
    }

    void AddHit(const JPH::CollideShapeResult& hit) override
    {
      const auto* pOwner = m_Native.ResolveOwner(m_Body, hit.mSubShapeID2);
      if (pOwner == nullptr)
        return;
      for (const auto& existing : m_Results.m_Results)
        if (existing.m_hActorObject == pOwner->m_hObject && existing.m_hShapeObject == pOwner->m_hShapeObject)
          return;
      auto& result = m_Results.m_Results.ExpandAndGetRef();
      result       = {};
      FillQueryOwner(*pOwner, result);
      result.m_vCenterPosition = m_vCenter;
    }

  private:
    const NativeWorld&            m_Native;
    xiiPhysicsOverlapResultArray& m_Results;
    JPH::BodyID                   m_Body;
    xiiVec3                       m_vCenter;
  };

  OverlapCollector collector(*m_pNativeWorld, *pResults);
  query.CollideShape(&shape, JPH::Vec3::sOne(), transform, {}, origin, collector, filters, filters, filters, filters);
  return !pResults->m_Results.IsEmpty();
}

bool xiiJoltPhysicsWorldModule::SweepTestSphere(xiiPhysicsCastResult& out_result, float fRadius, const xiiVec3& vPosition, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (!IsPositive(fRadius))
    return false;

  const JPH::SphereShape shape(fRadius);
  return SweepShape(out_result, shape, xiiTransform(vPosition), vDir, fDistance, params, collection);
}

bool xiiJoltPhysicsWorldModule::OverlapTestSphere(float fRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const
{
  if (!IsPositive(fRadius))
    return false;

  const JPH::SphereShape shape(fRadius);
  return OverlapShape(nullptr, shape, xiiTransform(vPosition), params);
}

void xiiJoltPhysicsWorldModule::QueryShapesInSphere(xiiPhysicsOverlapResultArray& out_results, float fRadius, const xiiVec3& vPosition, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();
  if (!IsPositive(fRadius))
    return;

  const JPH::SphereShape shape(fRadius);
  OverlapShape(&out_results, shape, xiiTransform(vPosition), params);
}

bool xiiJoltPhysicsWorldModule::SweepTestBox(xiiPhysicsCastResult& out_result, const xiiVec3& vExtents, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (!IsValidQueryShape(transform) || !vExtents.IsValid())
    return false;

  const auto half = vExtents.CompMul(transform.m_vScale.Abs()) * 0.5f;
  if (!half.IsValid() || half.x <= 0.0f || half.y <= 0.0f || half.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(ToJoltVector(half), 0.0f);
  return SweepShape(out_result, shape, transform, vDir, fDistance, params, collection);
}

bool xiiJoltPhysicsWorldModule::OverlapTestBox(const xiiVec3& vExtents, const xiiVec3& vPosition, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  if (!IsValidQueryShape(transform) || !vExtents.IsValid())
    return false;

  const auto half = vExtents.CompMul(transform.m_vScale.Abs()) * 0.5f;
  if (!half.IsValid() || half.x <= 0.0f || half.y <= 0.0f || half.z <= 0.0f)
    return false;

  const JPH::BoxShape shape(ToJoltVector(half), 0.0f);
  return OverlapShape(nullptr, shape, xiiTransform(vPosition, transform.m_qRotation, transform.m_vScale), params);
}

void xiiJoltPhysicsWorldModule::QueryShapesInBox(xiiPhysicsOverlapResultArray& out_results, const xiiVec3& vExtents, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();
  if (!IsValidQueryShape(transform) || !vExtents.IsValid())
    return;

  const auto half = vExtents.CompMul(transform.m_vScale.Abs()) * 0.5f;
  if (!half.IsValid() || half.x <= 0.0f || half.y <= 0.0f || half.z <= 0.0f)
    return;

  const JPH::BoxShape shape(ToJoltVector(half), 0.0f);
  OverlapShape(&out_results, shape, transform, params);
}

bool xiiJoltPhysicsWorldModule::SweepTestCapsule(xiiPhysicsCastResult& out_result, float fRadius, float fHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (!IsValidQueryShape(transform))
    return false;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return false;

  const JPH::CapsuleShape shape(fHalfHeight, fScaledRadius);
  return SweepShape(out_result, shape, GetAxialQueryPose(transform), vDir, fDistance, params, collection);
}

bool xiiJoltPhysicsWorldModule::OverlapTestCapsule(float fRadius, float fHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  if (!IsValidQueryShape(transform))
    return false;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return false;

  const JPH::CapsuleShape shape(fHalfHeight, fScaledRadius);
  return OverlapShape(nullptr, shape, GetAxialQueryPose(transform), params);
}

void xiiJoltPhysicsWorldModule::QueryShapesInCapsule(xiiPhysicsOverlapResultArray& out_results, float fRadius, float fHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();
  if (!IsValidQueryShape(transform))
    return;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return;

  const JPH::CapsuleShape shape(fHalfHeight, fScaledRadius);
  OverlapShape(&out_results, shape, GetAxialQueryPose(transform), params);
}

bool xiiJoltPhysicsWorldModule::SweepTestCylinder(xiiPhysicsCastResult& out_result, float fRadius, float fHeight, const xiiTransform& transform, const xiiVec3& vDir, float fDistance, const xiiPhysicsQueryParameters& params, xiiPhysicsHitCollection collection) const
{
  if (!IsValidQueryShape(transform))
    return false;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return false;

  const JPH::CylinderShape shape(fHalfHeight, fScaledRadius, 0.0f);
  return SweepShape(out_result, shape, GetAxialQueryPose(transform), vDir, fDistance, params, collection);
}

bool xiiJoltPhysicsWorldModule::OverlapTestCylinder(float fRadius, float fHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  if (!IsValidQueryShape(transform))
    return false;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return false;

  const JPH::CylinderShape shape(fHalfHeight, fScaledRadius, 0.0f);
  return OverlapShape(nullptr, shape, GetAxialQueryPose(transform), params);
}

void xiiJoltPhysicsWorldModule::QueryShapesInCylinder(xiiPhysicsOverlapResultArray& out_results, float fRadius, float fHeight, const xiiTransform& transform, const xiiPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();
  if (!IsValidQueryShape(transform))
    return;

  const auto  scale         = transform.m_vScale.Abs();
  const float fHalfHeight   = 0.5f * fHeight * scale.z;
  const float fScaledRadius = fRadius * xiiMath::Max(scale.x, scale.y);
  if (!IsPositive(fScaledRadius) || !IsPositive(fHalfHeight))
    return;

  const JPH::CylinderShape shape(fHalfHeight, fScaledRadius, 0.0f);
  OverlapShape(&out_results, shape, GetAxialQueryPose(transform), params);
}

void xiiJoltPhysicsWorldModule::AddStaticCollisionBox(xiiGameObject* pOwner, xiiVec3 vBoxSize)
{
  if (pOwner == nullptr || pOwner->GetWorld() != GetWorld() || !vBoxSize.IsValid() || vBoxSize.x <= 0 || vBoxSize.y <= 0 || vBoxSize.z <= 0)
    return;

  for (const auto* pComponent : pOwner->GetComponents())
  {
    if (pComponent->IsInstanceOf<xiiJoltActorComponent>() || pComponent->IsInstanceOf<xiiJoltShapeComponent>())
      return;
  }

  xiiJoltStaticActorComponent* pActor = nullptr;
  xiiJoltStaticActorComponent::CreateComponent(pOwner, pActor);
  if (pActor == nullptr)
    return;

  xiiJoltBoxShapeComponent* pShape = nullptr;
  xiiJoltBoxShapeComponent::CreateComponent(pOwner, pShape);
  if (pShape != nullptr)
    pShape->SetDimensions(vBoxSize);
  else
    pActor->DeleteComponent();
}

xiiBoundingBoxSphere xiiJoltPhysicsWorldModule::GetWorldSpaceBounds(xiiGameObject* pOwner, xiiUInt32 uiCollisionLayer, xiiBitflags<xiiPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const
{
  auto bounds = xiiBoundingBoxSphere::MakeInvalid();
  if (pOwner == nullptr || pOwner->GetWorld() != GetWorld() || !m_pNativeWorld || uiCollisionLayer >= CollisionLayerCount)
    return bounds;

  const xiiPhysicsQueryParameters params(uiCollisionLayer, shapeTypes);
  const QueryFilters              filters(*m_pNativeWorld, *GetWorld(), params);
  const auto                      addBounds = [&](auto&& recurse, const xiiGameObject& object) -> void {
    for (const auto* pComponent : object.GetComponents())
    {
      if (!pComponent->IsInstanceOf<xiiJoltActorComponent>())
        continue;

      JPH::BodyLockRead lock(m_pNativeWorld->m_PhysicsSystem.GetBodyLockInterface(), m_pNativeWorld->FindBody(pComponent->GetHandle()));
      if (!lock.Succeeded())
        continue;
      const auto& body = lock.GetBody();
      if (filters.ShouldCollide(body.GetBroadPhaseLayer()) && filters.ShouldCollide(body.GetObjectLayer()) && filters.ShouldCollideLocked(body))
      {
        const auto& box = body.GetWorldSpaceBounds();
        bounds.ExpandToInclude(xiiBoundingBoxSphere::MakeFromBox(xiiBoundingBox::MakeFromMinMax(FromJoltVector(box.mMin), FromJoltVector(box.mMax))));
      }
    }
    if (bIncludeChildObjects)
    {
      for (auto it = object.GetChildren(); it.IsValid(); ++it)
        recurse(recurse, *it);
    }
  };
  addBounds(addBounds, *pOwner);
  return bounds;
}


/// Updates are queued until synchronous PreAsync, after actor creation and before native stepping.
void xiiJoltPhysicsWorldModule::QueueJointUpdate(xiiComponentHandle hJoint, bool bRecreate)
{
  if (!m_pNativeWorld)
    return;
  auto& joint = m_pNativeWorld->m_Joints[hJoint];
  joint.m_bRecreate |= bRecreate;
  m_pNativeWorld->m_PendingJoints.Insert(hJoint);
}

void xiiJoltPhysicsWorldModule::RemoveJoint(xiiComponentHandle hJoint)
{
  auto& native = *m_pNativeWorld;
  if (auto* pJoint = native.m_Joints.GetValue(hJoint))
    native.DetachJoint(*pJoint);
  native.m_Joints.Remove(hJoint);
  native.m_PendingJoints.Remove(hJoint);
  native.m_WaitingJoints.Remove(hJoint);
  native.m_BreakableJoints.RemoveAndSwap(hJoint);
}

namespace
{
  bool ValidJointFrame(const xiiTransform& frame)
  {
    return frame.m_vPosition.IsValid() && frame.m_qRotation.IsValid() &&
      frame.m_vScale.IsEqual(xiiVec3(1.0f), 0.0001f);
  }
  bool NonNegative(float value) { return xiiMath::IsFinite(value) && value >= 0.0f; }

  bool ValidJointSettings(const xiiJoltJointComponent& joint)
  {
    if (!NonNegative(joint.GetBreakForce()))
      return false;
    if (const auto* p = xiiDynamicCast<const xiiJoltHingeJointComponent*>(&joint))
      return (!p->GetLimitsEnabled() || (p->GetLowerLimit().GetRadian() >= -JPH::JPH_PI && p->GetLowerLimit().GetRadian() <= 0 && p->GetUpperLimit().GetRadian() >= 0 && p->GetUpperLimit().GetRadian() <= JPH::JPH_PI)) &&
        NonNegative(p->GetMaxMotorTorque()) && NonNegative(p->GetMotorFrequency()) && NonNegative(p->GetMotorDamping()) &&
        xiiMath::IsFinite(p->GetTargetVelocity()) && xiiMath::IsFinite(p->GetTargetAngle().GetRadian()) &&
        p->GetMotorMode().GetValue() <= xiiJoltJointMotorMode::Position;
    if (const auto* p = xiiDynamicCast<const xiiJoltSliderJointComponent*>(&joint))
      return (!p->GetLimitsEnabled() || (xiiMath::IsFinite(p->GetLowerLimit()) && p->GetLowerLimit() <= 0 && xiiMath::IsFinite(p->GetUpperLimit()) && p->GetUpperLimit() >= 0)) &&
        NonNegative(p->GetMaxMotorForce()) && NonNegative(p->GetMotorFrequency()) && NonNegative(p->GetMotorDamping()) &&
        xiiMath::IsFinite(p->GetTargetVelocity()) && xiiMath::IsFinite(p->GetTargetPosition()) &&
        p->GetMotorMode().GetValue() <= xiiJoltJointMotorMode::Position && NonNegative(p->GetBreakTorque());
    if (const auto* p = xiiDynamicCast<const xiiJoltDistanceJointComponent*>(&joint))
      return NonNegative(p->GetMinDistance()) && NonNegative(p->GetMaxDistance()) && p->GetMinDistance() <= p->GetMaxDistance() &&
        NonNegative(p->GetSpringFrequency()) && NonNegative(p->GetSpringDamping());
    if (const auto* p = xiiDynamicCast<const xiiJoltFixedJointComponent*>(&joint))
      return NonNegative(p->GetBreakTorque());
    return xiiDynamicCast<const xiiJoltPointJointComponent*>(&joint) != nullptr;
  }
} // namespace

void xiiJoltPhysicsWorldModule::UpdateJoints()
{
  auto& native = *m_pNativeWorld;
  if (native.m_bBodiesChanged)
  {
    for (auto hJoint : native.m_WaitingJoints)
      native.m_PendingJoints.Insert(hJoint);
    native.m_bBodiesChanged = false;
  }
  for (auto hJoint : native.m_PendingJoints)
  {
    xiiJoltJointComponent* pComponent = nullptr;
    if (!GetWorld()->TryGetComponent(hJoint, pComponent) || !pComponent->IsActiveAndSimulating() || pComponent->IsBroken())
      continue;
    auto&       joint  = native.m_Joints[hJoint];
    const auto& config = pComponent->GetJointConfig();
    native.m_WaitingJoints.Remove(hJoint);
    if (!ValidJointSettings(*pComponent) || !ValidJointFrame(config.m_LocalFrameA) || !ValidJointFrame(config.m_LocalFrameB) ||
        config.m_hActorB.IsInvalidated() || (!config.m_hActorA.IsInvalidated() && config.m_hActorA == config.m_hActorB))
    {
      xiiLog::Warning("Jolt joint '{}' has invalid actors, frames, limits, or motor settings.", pComponent->GetOwner()->GetName());
      continue;
    }
    if (joint.m_bRecreate || joint.m_pConstraint == nullptr)
    {
      JPH::Body*                bodies[2] = {&JPH::Body::sFixedToWorld, nullptr};
      const xiiGameObjectHandle actors[2] = {config.m_hActorA, config.m_hActorB};
      xiiVec3                   scales[2] = {xiiVec3(1.0f), xiiVec3(1.0f)};
      JPH::BodyID               ids[2];
      bool                      bWaiting = false;
      bool                      bInvalid = false;
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        if (i == 0 && actors[i].IsInvalidated())
          continue;
        if (actors[i].GetInternalID().m_WorldIndex != GetWorld()->GetIndex())
        {
          bInvalid = true;
          break;
        }
        xiiGameObject* pObject = nullptr;
        if (!GetWorld()->TryGetObject(actors[i], pObject))
        {
          bWaiting = true;
          break;
        }
        const auto* pActor = FindActiveActor(*pObject);
        if (pActor == nullptr)
        {
          bWaiting = true;
          break;
        }
        ids[i]    = native.FindBody(pActor->GetHandle());
        bodies[i] = native.m_PhysicsSystem.GetBodyLockInterfaceNoLock().TryGetBody(ids[i]);
        if (bodies[i] == nullptr)
        {
          bWaiting = true;
          break;
        }
        pObject->UpdateGlobalTransform();
        scales[i]         = pObject->GetGlobalTransform().m_vScale;
        const auto& frame = i == 0 ? config.m_LocalFrameA : config.m_LocalFrameB;
        if (!scales[i].IsValid() || !frame.m_vPosition.CompMul(scales[i]).IsValid())
        {
          bInvalid = true;
          break;
        }
      }
      if (bWaiting)
      {
        native.DetachJoint(joint);
        native.m_WaitingJoints.Insert(hJoint);
        continue;
      }
      if (bInvalid || bodies[1] == nullptr || bodies[0] == bodies[1] || (!bodies[0]->IsDynamic() && !bodies[1]->IsDynamic()))
      {
        native.DetachJoint(joint);
        xiiLog::Warning("Jolt joint '{}' requires distinct same-world bodies with at least one dynamic actor.", pComponent->GetOwner()->GetName());
        continue;
      }

      const xiiTransform frames[2] = {config.m_LocalFrameA, config.m_LocalFrameB};
      JPH::RVec3         points[2];
      JPH::Vec3          axesX[2], axesY[2];
      for (xiiUInt32 i = 0; i < 2; ++i)
      {
        // XII frames are relative to the actor origin; Jolt local frames are relative to its center of mass.
        points[i]           = JPH::RVec3(ToJoltVector(frames[i].m_vPosition.CompMul(scales[i])) - bodies[i]->GetShape()->GetCenterOfMass());
        const auto rotation = ToJoltRotation(frames[i].m_qRotation);
        axesX[i]            = rotation * JPH::Vec3::sAxisX();
        axesY[i]            = rotation * JPH::Vec3::sAxisY();
      }
      JPH::Ref<JPH::TwoBodyConstraint> pConstraint;
      if (xiiDynamicCast<xiiJoltFixedJointComponent*>(pComponent))
      {
        JPH::FixedConstraintSettings settings;
        settings.mSpace  = JPH::EConstraintSpace::LocalToBodyCOM;
        settings.mPoint1 = points[0];
        settings.mPoint2 = points[1];
        settings.mAxisX1 = axesX[0];
        settings.mAxisX2 = axesX[1];
        settings.mAxisY1 = axesY[0];
        settings.mAxisY2 = axesY[1];
        pConstraint      = settings.Create(*bodies[0], *bodies[1]);
      }
      else if (xiiDynamicCast<xiiJoltPointJointComponent*>(pComponent))
      {
        JPH::PointConstraintSettings settings;
        settings.mSpace  = JPH::EConstraintSpace::LocalToBodyCOM;
        settings.mPoint1 = points[0];
        settings.mPoint2 = points[1];
        pConstraint      = settings.Create(*bodies[0], *bodies[1]);
      }
      else if (xiiDynamicCast<xiiJoltHingeJointComponent*>(pComponent))
      {
        JPH::HingeConstraintSettings settings;
        settings.mSpace       = JPH::EConstraintSpace::LocalToBodyCOM;
        settings.mPoint1      = points[0];
        settings.mPoint2      = points[1];
        settings.mHingeAxis1  = axesX[0];
        settings.mHingeAxis2  = axesX[1];
        settings.mNormalAxis1 = axesY[0];
        settings.mNormalAxis2 = axesY[1];
        pConstraint           = settings.Create(*bodies[0], *bodies[1]);
      }
      else if (xiiDynamicCast<xiiJoltSliderJointComponent*>(pComponent))
      {
        JPH::SliderConstraintSettings settings;
        settings.mSpace       = JPH::EConstraintSpace::LocalToBodyCOM;
        settings.mPoint1      = points[0];
        settings.mPoint2      = points[1];
        settings.mSliderAxis1 = axesX[0];
        settings.mSliderAxis2 = axesX[1];
        settings.mNormalAxis1 = axesY[0];
        settings.mNormalAxis2 = axesY[1];
        pConstraint           = settings.Create(*bodies[0], *bodies[1]);
      }
      else
      {
        JPH::DistanceConstraintSettings settings;
        const auto*                     pDistance = xiiStaticCast<const xiiJoltDistanceJointComponent*>(pComponent);
        settings.mSpace                           = JPH::EConstraintSpace::LocalToBodyCOM;
        settings.mPoint1                          = points[0];
        settings.mPoint2                          = points[1];
        settings.mMinDistance                     = pDistance->GetMinDistance();
        settings.mMaxDistance                     = pDistance->GetMaxDistance();
        pConstraint                               = settings.Create(*bodies[0], *bodies[1]);
      }
      native.DetachJoint(joint);
      joint.m_pConstraint = pConstraint;
      joint.m_Bodies[0]   = ids[0];
      joint.m_Bodies[1]   = ids[1];
      joint.m_bRecreate   = false;
      native.m_PhysicsSystem.AddConstraint(pConstraint);
    }

    joint.m_fBreakForce  = pComponent->GetBreakForce();
    joint.m_fBreakTorque = (xiiDynamicCast<xiiJoltFixedJointComponent*>(pComponent) != nullptr || xiiDynamicCast<xiiJoltSliderJointComponent*>(pComponent) != nullptr) ? pComponent->m_fBreakTorque : 0.0f;
    if (const auto* pHinge = xiiDynamicCast<xiiJoltHingeJointComponent*>(pComponent))
    {
      auto& hinge = *static_cast<JPH::HingeConstraint*>(joint.m_pConstraint.GetPtr());
      hinge.SetLimits(pHinge->GetLimitsEnabled() ? pHinge->GetLowerLimit().GetRadian() : -JPH::JPH_PI,
                      pHinge->GetLimitsEnabled() ? pHinge->GetUpperLimit().GetRadian() : JPH::JPH_PI);
      auto& motor           = hinge.GetMotorSettings();
      motor.mSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, pHinge->GetMotorFrequency(), pHinge->GetMotorDamping());
      motor.SetTorqueLimit(pHinge->GetMaxMotorTorque());
      hinge.SetMotorState(static_cast<JPH::EMotorState>(pHinge->GetMotorMode().GetValue()));
      hinge.SetTargetAngularVelocity(pHinge->GetTargetVelocity());
      hinge.SetTargetAngle(pHinge->GetTargetAngle().GetRadian());
    }
    else if (const auto* pSlider = xiiDynamicCast<xiiJoltSliderJointComponent*>(pComponent))
    {
      auto& slider = *static_cast<JPH::SliderConstraint*>(joint.m_pConstraint.GetPtr());
      slider.SetLimits(pSlider->GetLimitsEnabled() ? pSlider->GetLowerLimit() : -FLT_MAX,
                       pSlider->GetLimitsEnabled() ? pSlider->GetUpperLimit() : FLT_MAX);
      auto& motor           = slider.GetMotorSettings();
      motor.mSpringSettings = JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, pSlider->GetMotorFrequency(), pSlider->GetMotorDamping());
      motor.SetForceLimit(pSlider->GetMaxMotorForce());
      slider.SetMotorState(static_cast<JPH::EMotorState>(pSlider->GetMotorMode().GetValue()));
      slider.SetTargetVelocity(pSlider->GetTargetVelocity());
      slider.SetTargetPosition(pSlider->GetTargetPosition());
    }
    else if (const auto* pDistance = xiiDynamicCast<xiiJoltDistanceJointComponent*>(pComponent))
    {
      auto& distance = *static_cast<JPH::DistanceConstraint*>(joint.m_pConstraint.GetPtr());
      distance.SetDistance(pDistance->GetMinDistance(), pDistance->GetMaxDistance());
      distance.SetLimitsSpringSettings(JPH::SpringSettings(JPH::ESpringMode::FrequencyAndDamping, pDistance->GetSpringFrequency(), pDistance->GetSpringDamping()));
    }
    native.m_BreakableJoints.RemoveAndSwap(hJoint);
    if (joint.m_fBreakForce > 0 || joint.m_fBreakTorque > 0)
      native.m_BreakableJoints.PushBack(hJoint);
    for (auto id : joint.m_Bodies)
      if (!id.IsInvalid() && native.m_PhysicsSystem.GetBodyInterface().GetMotionType(id) == JPH::EMotionType::Dynamic)
        native.m_PhysicsSystem.GetBodyInterface().ActivateBody(id);
  }
  native.m_PendingJoints.Clear();
}

void xiiJoltPhysicsWorldModule::CheckJointBreakage(float fStepSeconds)
{
  auto& native = *m_pNativeWorld;
  for (xiiUInt32 i = 0; i < native.m_BreakableJoints.GetCount();)
  {
    const auto hJoint = native.m_BreakableJoints[i];
    auto*      pJoint = native.m_Joints.GetValue(hJoint);
    if (pJoint == nullptr || pJoint->m_pConstraint == nullptr || !pJoint->m_pConstraint->IsActive())
    {
      ++i;
      continue;
    }
    double      fLinearImpulse = 0, fAngularImpulse = 0;
    const auto* pConstraint = pJoint->m_pConstraint.GetPtr();
    switch (pConstraint->GetSubType())
    {
      case JPH::EConstraintSubType::Fixed:
      {
        const auto& c   = *static_cast<const JPH::FixedConstraint*>(pConstraint);
        fLinearImpulse  = c.GetTotalLambdaPosition().Length();
        fAngularImpulse = c.GetTotalLambdaRotation().Length();
        break;
      }
      case JPH::EConstraintSubType::Point:
        fLinearImpulse = static_cast<const JPH::PointConstraint*>(pConstraint)->GetTotalLambdaPosition().Length();
        break;
      case JPH::EConstraintSubType::Hinge:
      {
        const auto& c  = *static_cast<const JPH::HingeConstraint*>(pConstraint);
        fLinearImpulse = c.GetTotalLambdaPosition().Length();
        // Alignment lambdas use a non-orthonormal basis. Hinge torque breaking is intentionally unavailable.
        break;
      }
      case JPH::EConstraintSubType::Slider:
      {
        const auto&  c     = *static_cast<const JPH::SliderConstraint*>(pConstraint);
        const double axial = double(c.GetTotalLambdaMotor()) + c.GetTotalLambdaPositionLimits();
        fLinearImpulse     = xiiMath::Sqrt(double(c.GetTotalLambdaPosition().LengthSq()) + axial * axial);
        fAngularImpulse    = c.GetTotalLambdaRotation().Length();
        break;
      }
      case JPH::EConstraintSubType::Distance:
        fLinearImpulse = xiiMath::Abs(static_cast<const JPH::DistanceConstraint*>(pConstraint)->GetTotalLambdaPosition());
        break;
      default:
        break;
    }
    // Native velocity-solver lambdas are impulses. Compare to F*dt and torque*dt for this fixed step.
    if ((pJoint->m_fBreakForce > 0 && fLinearImpulse > double(pJoint->m_fBreakForce) * fStepSeconds) ||
        (pJoint->m_fBreakTorque > 0 && fAngularImpulse > double(pJoint->m_fBreakTorque) * fStepSeconds))
    {
      native.DetachJoint(*pJoint);
      xiiJoltJointComponent* pComponent = nullptr;
      if (GetWorld()->TryGetComponent(hJoint, pComponent))
        pComponent->m_bBroken = true;
      native.m_BrokenJoints.PushBack(hJoint);
      native.m_BreakableJoints.RemoveAtAndSwap(i);
    }
    else
      ++i;
  }
}

void xiiJoltPhysicsWorldModule::AddFixedJointComponent(xiiGameObject* pOwner, const FixedJointConfig& cfg)
{
  xiiJoltFixedJointComponent* pJoint = nullptr;
  GetWorld()->GetOrCreateComponentManager<xiiJoltFixedJointComponentManager>()->CreateComponent(pOwner, pJoint);
  pJoint->SetJointConfig(cfg);
}

void xiiJoltPhysicsWorldModule::SetBodySurface(xiiComponentHandle hActor, const xiiSurfaceResourceHandle& hSurface)
{
  if (auto* pOwner = m_pNativeWorld->FindOwnerMutable(m_pNativeWorld->FindBody(hActor)))
    pOwner->m_hSurface = hSurface;
  m_pNativeWorld->m_MaterialsDirty = 1;
}

void xiiJoltPhysicsWorldModule::UpdateMaterials()
{
  auto& native = *m_pNativeWorld;
  if (native.m_MaterialsDirty.Set(0) == 0)
    return;
  auto refresh = [](NativeWorld::BodyOwner& owner, const xiiSurfaceResourceHandle& handle) {
    owner.m_hSurface     = handle;
    owner.m_fFriction    = 0.2f;
    owner.m_fRestitution = 0;
    owner.m_sCollisionInteraction.Clear();
    if (!handle.IsValid())
      return;
    XII_LOCK(xiiResourceManager::GetMutex());
    const xiiResourceLock<xiiSurfaceResource> surface(handle, xiiResourceAcquireMode::PointerOnly);
    if (surface->GetLoadingState() == xiiResourceState::Loaded)
    {
      const auto& descriptor = surface->GetDescriptor();
      if (NonNegative(descriptor.m_fPhysicsFrictionDynamic))
        owner.m_fFriction = descriptor.m_fPhysicsFrictionDynamic;
      if (xiiMath::IsFinite(descriptor.m_fPhysicsRestitution))
        owner.m_fRestitution = xiiMath::Clamp(descriptor.m_fPhysicsRestitution, 0.0f, 1.0f);
      owner.m_sCollisionInteraction = descriptor.m_sOnCollideInteraction;
    }
    else if (surface->GetLoadingState() == xiiResourceState::Unloaded)
      xiiResourceManager::PreloadResource(handle);
  };
  for (auto it = native.GetBodyOwners().GetIterator(); it.IsValid(); ++it)
  {
    auto&       owner       = *native.FindOwnerMutable(JPH::BodyID(it.Key()));
    const float oldFriction = owner.m_fFriction, oldRestitution = owner.m_fRestitution;
    refresh(owner, owner.m_hSurface);
    bool changed      = owner.m_bMaterialInitialized && (oldFriction != owner.m_fFriction || oldRestitution != owner.m_fRestitution);
    bool interactions = !owner.m_sCollisionInteraction.IsEmpty();
    if (auto* pSet = native.m_ShapeSets.GetValue(it.Key()))
      for (auto& child : pSet->m_Shapes)
        for (xiiUInt32 i = 0; i < child.m_Surfaces.GetCount(); ++i)
        {
          auto&       surface = child.m_Surfaces[i];
          const float oldF = surface.m_fFriction, oldR = surface.m_fRestitution;
          const bool  initialized = surface.m_bMaterialInitialized;
          auto        handle      = child.m_hSurfaceOverride;
          if (!handle.IsValid() && i < child.m_SurfaceSlots.GetCount())
            handle = child.m_SurfaceSlots[i];
          if (!handle.IsValid() && child.m_pResource && i < child.m_pResource->m_Surfaces.GetCount())
            handle = child.m_pResource->m_Surfaces[i];
          if (!handle.IsValid())
            handle = owner.m_hSurface;
          surface                   = owner;
          surface.m_hShapeObject    = child.m_hObject;
          surface.m_hShapeComponent = child.m_hComponent;
          refresh(surface, handle);
          surface.m_bMaterialInitialized = true;
          changed |= initialized && (oldF != surface.m_fFriction || oldR != surface.m_fRestitution);
          interactions |= !surface.m_sCollisionInteraction.IsEmpty();
        }
    auto&             bodies = native.m_PhysicsSystem.GetBodyInterface();
    const JPH::BodyID id(it.Key());
    bodies.SetFriction(id, owner.m_fFriction);
    bodies.SetRestitution(id, owner.m_fRestitution);
    if (changed && owner.m_bDynamic)
      bodies.ActivateBody(id);
    owner.m_bMaterialInitialized = true;
    bodies.SetUserData(id, (bodies.GetUserData(id) & 1) | (interactions ? 2 : 0));
  }
}

void xiiJoltPhysicsWorldModule::RegisterShape(xiiComponentHandle hShape)
{
  m_pNativeWorld->m_TrackedShapes[hShape] = {};
}

void xiiJoltPhysicsWorldModule::UnregisterShape(xiiComponentHandle hShape)
{
  auto& native = *m_pNativeWorld;
  if (const auto* pShape = native.m_TrackedShapes.GetValue(hShape))
    if (!pShape->m_hActor.IsInvalidated())
      QueueBodyUpdate(pShape->m_hActor);
  native.m_TrackedShapes.Remove(hShape);
}

void xiiJoltPhysicsWorldModule::QueueStaticCollision(xiiComponentHandle hGroup)
{
  auto& native = *m_pNativeWorld;
  if (!native.m_StaticGroups.Contains(hGroup))
    native.m_StaticGroupCount.Increment();
  native.m_StaticGroups[hGroup].m_bDirty = true;
  native.m_PendingStaticGroups.Insert(hGroup);
}

void xiiJoltPhysicsWorldModule::RemoveStaticCollision(xiiComponentHandle hGroup)
{
  auto& native = *m_pNativeWorld;
  if (native.m_StaticGroups.Remove(hGroup))
  {
    native.m_StaticGroupCount.Decrement();
    native.m_bPruneStaticMeshCache = true;
  }
  native.m_PendingStaticGroups.Remove(hGroup);
  native.RemoveBody(hGroup);
}

void xiiJoltPhysicsWorldModule::InvalidateStaticCollisionDescendants(const xiiGameObject& object)
{
  auto& native = *m_pNativeWorld;
  for (auto& entry : native.m_StaticGroups)
  {
    const xiiJoltStaticCollisionComponent* pGroup = nullptr;
    if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(entry.Key(), pGroup))
      continue;
    for (auto* pParent = pGroup->GetOwner(); pParent; pParent = pParent->GetParent())
      if (pParent == &object)
      {
        entry.Value().m_bDirty = true;
        native.m_PendingStaticGroups.Insert(entry.Key());
        break;
      }
  }
}

void xiiJoltPhysicsWorldModule::UpdateStaticCollision(const UpdateContext&)
{
  using State                = xiiJoltStaticCollisionComponent::State;
  auto&      native          = *m_pNativeWorld;
  const bool bNeedsMaterials = !GetWorld()->GetWorldSimulationEnabled() && !native.m_StaticGroups.IsEmpty();
  if (native.m_PendingStaticGroups.IsEmpty() && native.m_StaticCooks.IsEmpty() && !native.m_bPruneStaticMeshCache &&
      native.m_bStaticResourcesChanged == 0)
  {
    if (bNeedsMaterials)
      UpdateMaterials();
    return;
  }

  xiiHashSet<xiiUInt64> changed, reloaded;
  if (native.m_bStaticResourcesChanged.Set(0) != 0)
  {
    XII_LOCK(native.m_StaticResourceMutex);
    changed.Swap(native.m_StaticResourceChanges);
    reloaded.Swap(native.m_StaticMeshReloads);
  }
  if (!reloaded.IsEmpty())
    for (auto it = native.m_StaticMeshCache.GetIterator(); it.IsValid();)
      if (reloaded.Contains(it.Value()->m_hMesh.GetResourceIDHash()))
        it = native.m_StaticMeshCache.Remove(it);
      else
        ++it;
  if (!changed.IsEmpty() || !reloaded.IsEmpty())
    for (auto it = native.m_StaticGroups.GetIterator(); it.IsValid(); ++it)
    {
      const xiiJoltStaticCollisionComponent* pGroup = nullptr;
      if (!static_cast<const xiiWorld*>(GetWorld())->TryGetComponent(it.Key(), pGroup))
        continue;
      bool bUpdate = pGroup->GetCollisionMesh().IsValid() && changed.Contains(pGroup->GetCollisionMesh().GetResourceIDHash());
      for (const auto& source : it.Value().m_Sources)
      {
        if (source.m_pCook && source.m_pCook->m_hMesh.IsValid() && reloaded.Contains(source.m_pCook->m_hMesh.GetResourceIDHash()))
        {
          it.Value().m_bDirty = true;
          bUpdate             = true;
        }
        for (const auto& material : source.m_MaterialDependencies)
          if (changed.Contains(material.GetResourceIDHash()))
            bUpdate = true;
      }
      if (bUpdate)
        native.m_PendingStaticGroups.Insert(it.Key());
    }

  const auto schedule = [&native](const xiiSharedPtr<xiiJoltSceneCollisionCook>& task) {
    task->m_Group = xiiTaskSystem::StartSingleTask(task, xiiTaskPriority::LongRunning);
    native.m_StaticCooks.PushBack(task);
  };
  bool bInstalled = false;
  for (auto pending = native.m_PendingStaticGroups.GetIterator(); pending.IsValid();)
  {
    const auto                       hGroup = pending.Key();
    xiiJoltStaticCollisionComponent* pGroup = nullptr;
    if (!GetWorld()->TryGetComponent(hGroup, pGroup) || !pGroup->IsActiveAndInitialized())
    {
      pending = native.m_PendingStaticGroups.Remove(pending);
      continue;
    }
    auto& group = native.m_StaticGroups[hGroup];
    auto& root  = *pGroup->GetOwner();
    if (std::exchange(group.m_bDirty, false))
    {
      xiiDynamicArray<NativeWorld::StaticSource> sources;
      root.UpdateGlobalTransform();
      group.m_Pose          = root.GetGlobalTransform();
      group.m_bPoseValid    = group.m_Pose.m_vPosition.IsValid() && group.m_Pose.m_qRotation.IsValid();
      const auto rootScale  = group.m_Pose.m_vScale;
      group.m_Pose.m_vScale = xiiVec3(1.0f);
      auto worldToBody      = xiiMat4::MakeIdentity();
      if (group.m_bPoseValid)
      {
        worldToBody        = group.m_Pose.GetAsMat4();
        group.m_bPoseValid = worldToBody.Invert().Succeeded();
      }

      const auto firstGroup = [](const xiiGameObject& object) -> const xiiJoltStaticCollisionComponent* {
        for (auto* pComponent : object.GetComponents())
          if (const auto* pOther = xiiDynamicCast<const xiiJoltStaticCollisionComponent*>(pComponent); pOther && pOther->IsActive())
            return pOther;
        return nullptr;
      };
      const auto hasActor = [](const xiiGameObject& object) {
        for (auto* pComponent : object.GetComponents())
          if (pComponent->IsActive() && xiiDynamicCast<const xiiJoltActorComponent*>(pComponent))
            return true;
        return false;
      };
      const auto hasCollisionBoundary = [&](const xiiGameObject& object) {
        return !object.IsStatic() || hasActor(object) || firstGroup(object) != nullptr;
      };

      bool bEligible = root.IsStatic() && firstGroup(root) == pGroup;
      for (auto* pObject = &root; pObject; pObject = pObject->GetParent())
      {
        // Observe edits above the group as well. Explicit actors own their complete subtree.
        pObject->EnableComponentChangesNotifications();
        pObject->EnableChildChangesNotifications();
        bEligible &= !hasActor(*pObject);
      }

      if (bEligible && group.m_bPoseValid && pGroup->GetCollisionMesh().IsValid())
      {
        // A baked resource has no source-object map. Its contract is to represent
        // this root's geometry independently of any child instances.
        auto& source            = sources.ExpandAndGetRef();
        source.m_hObject        = root.GetHandle();
        source.m_hComponent     = hGroup;
        source.m_Local.m_vScale = rootScale;
      }
      else if (bEligible && group.m_bPoseValid)
      {
        const auto collect = [&](auto&& self, xiiGameObject& object) -> void {
          object.EnableChildChangesNotifications();
          object.EnableComponentChangesNotifications();
          object.EnableStaticTransformChangesNotifications();
          if (!object.IsActive() || (&object != &root && hasCollisionBoundary(object)))
            return;
          object.UpdateGlobalTransform();
          for (auto* pComponent : object.GetComponents())
          {
            if (!pComponent->IsActiveAndInitialized())
              continue;
            if (auto* pMesh = xiiDynamicCast<xiiStaticMeshComponent*>(pComponent); pMesh && pMesh->GetMesh().IsValid())
            {
              xiiTransform local;
              if (!DecomposeShape(worldToBody * object.GetGlobalTransform().GetAsMat4(), local))
              {
                group.m_bPoseValid = false;
                continue;
              }
              xiiStringBuilder key;
              key.SetFormat("{}#{}", pMesh->GetMesh().GetResourceID(), pMesh->GetSectionIndex());
              auto& task = native.m_StaticMeshCache[xiiString(key)];
              if (!task)
              {
                task              = XII_DEFAULT_NEW(xiiJoltSceneCollisionCook);
                task->m_hMesh     = pMesh->GetMesh();
                task->m_uiSection = pMesh->GetSectionIndex();
                schedule(task);
              }
              auto& source               = sources.ExpandAndGetRef();
              source.m_hObject           = object.GetHandle();
              source.m_hComponent        = pComponent->GetHandle();
              source.m_Local             = local;
              source.m_pCook             = task;
              source.m_MaterialOverrides = pMesh->GetMaterialOverrides();
            }
            else
            {
              xiiMsgBuildStaticMesh message;
              if (!pComponent->HandlesMessage(message))
                continue;
              xiiUniquePtr<xiiSmcDescription> geometry = XII_DEFAULT_NEW(xiiSmcDescription);
              message.m_pStaticMeshDescription         = geometry.Borrow();
              pComponent->SendMessage(message);
              if (geometry->m_Triangles.IsEmpty())
                continue;
              for (auto& vertex : geometry->m_Vertices)
                vertex = worldToBody.TransformPosition(vertex);
              auto task         = XII_DEFAULT_NEW(xiiJoltSceneCollisionCook);
              task->m_pGeometry = std::move(geometry);
              schedule(task);
              auto& source        = sources.ExpandAndGetRef();
              source.m_hObject    = object.GetHandle();
              source.m_hComponent = pComponent->GetHandle();
              source.m_pCook      = std::move(task);
            }
          }
          for (auto child = object.GetChildren(); child.IsValid(); ++child)
            self(self, *child);
        };
        collect(collect, root);
      }

      // Old geometry may remain while a replacement loads, but never if it contains
      // a contributor that was removed, deactivated or claimed by another group.
      xiiHashSet<xiiComponentHandle> newSources(GetAllocator());
      newSources.Reserve(sources.GetCount());
      for (const auto& source : sources)
        newSources.Insert(source.m_hComponent);
      for (const auto hInstalled : group.m_InstalledSources)
      {
        if (!newSources.Contains(hInstalled))
        {
          native.RemoveBody(hGroup);
          group.m_InstalledSources.Clear();
          break;
        }
      }
      group.m_Sources                = std::move(sources);
      native.m_bPruneStaticMeshCache = true;
    }

    State                                  state = !group.m_bPoseValid ? State::Failed : group.m_Sources.IsEmpty() ? State::Unsupported :
                                                                                                                     State::Ready;
    xiiSharedPtr<xiiJoltCollisionMeshData> cookedResource;
    if (state == State::Ready && pGroup->GetCollisionMesh().IsValid())
    {
      const auto&                                         handle = pGroup->GetCollisionMesh();
      const xiiResourceLock<xiiJoltCollisionMeshResource> resource(handle, xiiResourceAcquireMode::PointerOnly);
      cookedResource = resource->GetData();
      if (!cookedResource)
      {
        if (resource->GetLoadingState() == xiiResourceState::LoadedResourceMissing)
          state = State::Failed;
        else
        {
          xiiResourceManager::PreloadResource(handle);
          state = State::PendingResource;
        }
      }
      else if (cookedResource->m_Type != xiiJoltCollisionGeometryType::TriangleMesh)
        state = State::Failed;
    }
    if (state == State::Ready && !cookedResource)
      for (const auto& source : group.m_Sources)
      {
        if (!xiiTaskSystem::IsTaskGroupFinished(source.m_pCook->m_Group))
          state = State::PendingGeometry;
        else if (!source.m_pCook->m_pResult)
        {
          state = State::Failed;
          break;
        }
      }

    NativeWorld::ShapeSet shapeSet;
    if (state == State::Ready)
      for (auto& source : group.m_Sources)
      {
        auto data = cookedResource ? cookedResource : source.m_pCook->m_pResult;
        if (!data->m_pShape->IsValidScale(ToJoltVector(source.m_Local.m_vScale)))
        {
          state = State::Failed;
          break;
        }
        auto& owner         = shapeSet.m_Shapes.ExpandAndGetRef();
        owner.m_hObject     = source.m_hObject;
        owner.m_hComponent  = source.m_hComponent;
        owner.m_pResource   = data;
        owner.m_StaticLocal = source.m_Local;
        owner.m_Surfaces.SetCount(data->m_Surfaces.GetCount());

        if (!cookedResource && !source.m_pCook->m_MaterialSlots.IsEmpty())
        {
          source.m_MaterialDependencies.Clear();
          owner.m_SurfaceSlots.SetCount(source.m_pCook->m_MaterialSlots.GetCount());
          for (xiiUInt32 i = 0; i < owner.m_SurfaceSlots.GetCount(); ++i)
          {
            const auto                slot = source.m_pCook->m_MaterialSlots[i];
            xiiMaterialResourceHandle material;
            if (slot < source.m_MaterialOverrides.GetCount())
              material = source.m_MaterialOverrides[slot];
            const auto& defaults = source.m_pCook->m_Materials;
            if (!material.IsValid() && slot < defaults.GetCount() && !defaults[slot].IsEmpty())
              material = xiiResourceManager::LoadResource<xiiMaterialResource>(defaults[slot]);
            for (xiiUInt32 depth = 0; material.IsValid() && depth < 32; ++depth)
            {
              XII_LOCK(xiiResourceManager::GetMutex());
              const xiiResourceLock<xiiMaterialResource> resource(material, xiiResourceAcquireMode::PointerOnly);
              if (!source.m_MaterialDependencies.Contains(material))
                source.m_MaterialDependencies.PushBack(material);
              if (resource->GetLoadingState() != xiiResourceState::Loaded)
              {
                if (resource->GetLoadingState() == xiiResourceState::Unloaded)
                  xiiResourceManager::PreloadResource(material);
                break;
              }
              const auto& description = resource->GetCurrentDescription();
              if (!description.m_sSurface.IsEmpty())
              {
                owner.m_SurfaceSlots[i] = xiiResourceManager::LoadResource<xiiSurfaceResource>(description.m_sSurface.GetView());
                break;
              }
              if (material == description.m_hBaseMaterial)
                break;
              material = description.m_hBaseMaterial;
            }
          }
        }
      }

    if (state == State::Ready)
    {
      auto  id            = native.FindBody(hGroup);
      auto* pOld          = native.m_ShapeSets.GetValue(id.GetIndexAndSequenceNumber());
      bool  bSameGeometry = pOld && pOld->m_Shapes.GetCount() == shapeSet.m_Shapes.GetCount();
      if (bSameGeometry)
        for (xiiUInt32 i = 0; i < shapeSet.m_Shapes.GetCount(); ++i)
        {
          const auto& old   = pOld->m_Shapes[i];
          const auto& child = shapeSet.m_Shapes[i];
          bSameGeometry &= old.m_pResource == child.m_pResource && old.m_hComponent == child.m_hComponent &&
            old.m_StaticLocal.m_vPosition.IsEqual(child.m_StaticLocal.m_vPosition, 0.00001f) &&
            old.m_StaticLocal.m_vScale.IsEqual(child.m_StaticLocal.m_vScale, 0.00001f) &&
            old.m_StaticLocal.m_qRotation.IsEqualRotation(child.m_StaticLocal.m_qRotation, 0.00001f);
        }
      const auto&      position = group.m_Pose.m_vPosition;
      const JPH::RVec3 nativePosition(position.x, position.y, position.z);
      const auto       rotation = ToJoltRotation(group.m_Pose.m_qRotation);
      if (bSameGeometry)
      {
        shapeSet.m_pRoot = pOld->m_pRoot;
      }
      else
      {
        JPH::StaticCompoundShapeSettings compound;
        for (xiiUInt32 i = 0; i < shapeSet.m_Shapes.GetCount(); ++i)
        {
          const auto& child = shapeSet.m_Shapes[i];
          const auto& local = child.m_StaticLocal;
          auto        shape = child.m_pResource->m_pShape;
          if (local.m_vScale != xiiVec3(1.0f))
            shape = new JPH::ScaledShape(shape, ToJoltVector(local.m_vScale));
          compound.AddShape(ToJoltVector(local.m_vPosition), ToJoltRotation(local.m_qRotation), shape, i);
        }
        auto result = compound.Create(native.m_TempAllocator);
        if (result.HasError())
        {
          state = State::Failed;
          if (!pGroup->m_bReportedFailure)
          {
            xiiLog::Warning("Could not assemble Jolt static collision for '{}': {}", root.GetName(), result.GetError().c_str());
            pGroup->m_bReportedFailure = true;
          }
        }
        else
          shapeSet.m_pRoot = result.Get();
      }
      if (state == State::Ready)
      {
        auto& bodies = native.m_PhysicsSystem.GetBodyInterface();
        if (id.IsInvalid() || !bSameGeometry)
        {
          // A different compound can reuse subshape IDs for different children. Replace
          // the body so its existing contacts retire through the body registry.
          JPH::BodyCreationSettings settings(shapeSet.m_pRoot.GetPtr(), nativePosition, rotation, JPH::EMotionType::Static, 0);
          settings.mUserData = pGroup->GetReportContacts() ? 1 : 0;
          if (native.CreateBody(*GetWorld(), hGroup, *pGroup, pGroup->GetCollisionLayer(), settings, JPH::EActivation::DontActivate).Failed())
            state = State::Failed;
          id = native.FindBody(hGroup);
        }
        else
        {
          const auto layer = xiiJoltCollisionFilters::MakeObjectLayer(pGroup->GetCollisionLayer(), JPH::EMotionType::Static);
          if (bodies.GetObjectLayer(id) != layer)
            bodies.SetObjectLayer(id, layer);
          bodies.SetPositionAndRotationWhenChanged(id, nativePosition, rotation, JPH::EActivation::DontActivate);
          SetBodyContactReporting(hGroup, pGroup->GetReportContacts());
        }
        if (state == State::Ready)
        {
          auto& owner        = *native.FindOwnerMutable(id);
          owner.m_hSurface   = pGroup->GetSurface();
          owner.m_bGenerated = true;
          group.m_InstalledSources.Clear();
          for (auto& child : shapeSet.m_Shapes)
          {
            group.m_InstalledSources.PushBack(child.m_hComponent);
            for (auto& surface : child.m_Surfaces)
            {
              surface                   = owner;
              surface.m_hShapeObject    = child.m_hObject;
              surface.m_hShapeComponent = child.m_hComponent;
            }
          }
          native.m_ShapeSets.Insert(id.GetIndexAndSequenceNumber(), std::move(shapeSet));
          bInstalled = true;
        }
      }
    }

    if (state == State::Failed && !pGroup->m_bReportedFailure)
      xiiLog::Warning("Could not build Jolt static collision for '{}'. Check mesh data, scale, collision resource type, and body capacity.", root.GetName());
    if (state == State::Failed || state == State::Ready)
      pGroup->m_bReportedFailure = state == State::Failed;
    pGroup->m_State = state;
    if (state == State::PendingGeometry || state == State::PendingResource)
      ++pending;
    else
      pending = native.m_PendingStaticGroups.Remove(pending);
  }

  for (xiiUInt32 i = native.m_StaticCooks.GetCount(); i > 0; --i)
  {
    const auto& task = native.m_StaticCooks[i - 1];
    if (xiiTaskSystem::IsTaskGroupFinished(task->m_Group))
    {
      native.m_StaticCooks.RemoveAtAndSwap(i - 1);
      native.m_bPruneStaticMeshCache = true;
    }
  }
  if (native.m_bPruneStaticMeshCache)
  {
    native.m_bPruneStaticMeshCache = false;
    for (auto it = native.m_StaticMeshCache.GetIterator(); it.IsValid();)
      if (it.Value()->GetRefCount() == 1)
        it = native.m_StaticMeshCache.Remove(it);
      else
        ++it;
  }
  if (bInstalled || bNeedsMaterials)
    UpdateMaterials();
}

void xiiJoltPhysicsWorldModule::UpdateShapes()
{
  auto&      native          = *m_pNativeWorld;
  const bool geometryChanged = native.m_GeometryDirty.Set(0) != 0;
  for (auto it = native.m_TrackedShapes.GetIterator(); it.IsValid(); ++it)
  {
    auto&                  tracked = it.Value();
    xiiJoltShapeComponent* pShape  = nullptr;
    if (!GetWorld()->TryGetComponent(it.Key(), pShape) || !pShape->IsActiveAndInitialized())
      continue;
    const auto* pActor = ShapeActor(pShape->GetOwner());
    const auto  hActor = pActor ? pActor->GetHandle() : xiiComponentHandle{};
    xiiMat4     matrix = xiiMat4::MakeIdentity();
    bool        valid  = false;
    if (pActor)
    {
      xiiGameObject* pObject = nullptr;
      if (GetWorld()->TryGetObject(pActor->GetOwner()->GetHandle(), pObject))
      {
        pObject->UpdateGlobalTransform();
        valid = ShapeMatrix(*pShape, *pObject, matrix);
      }
    }
    const bool resourceShape = xiiDynamicCast<xiiJoltMeshShapeComponent*>(pShape) || xiiDynamicCast<xiiJoltConvexHullShapeComponent*>(pShape);
    if (!tracked.m_bInitialized || tracked.m_hActor != hActor || tracked.m_bMatrixValid != valid ||
        (valid && !tracked.m_Matrix.IsEqual(matrix, 0.00001f)) || (geometryChanged && resourceShape))
    {
      if (!tracked.m_hActor.IsInvalidated())
        QueueBodyUpdate(tracked.m_hActor);
      if (!hActor.IsInvalidated())
        QueueBodyUpdate(hActor);
      tracked.m_hActor       = hActor;
      tracked.m_Matrix       = matrix;
      tracked.m_bInitialized = true;
      tracked.m_bMatrixValid = valid;
    }
  }
}


XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltPhysicsWorldModule);
