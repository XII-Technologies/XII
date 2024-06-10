#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/SensorComponent.h>
#include <GraphicsCore/Debug/DebugRenderer.h>

// clang-format off
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSensorDetectedObjectsChanged);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSensorDetectedObjectsChanged, 1, xiiRTTIDefaultAllocator<xiiMsgSensorDetectedObjectsChanged>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("DetectedObjects", m_DetectedObjects),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

//////////////////////////////////////////////////////////////////////////

XII_BEGIN_ABSTRACT_COMPONENT_TYPE(xiiSensorComponent, 1)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("UpdateRate", xiiUpdateRate, m_UpdateRate),
    XII_ACCESSOR_PROPERTY("SpatialCategory", GetSpatialCategory, SetSpatialCategory)->AddAttributes(new xiiDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    XII_MEMBER_PROPERTY("TestVisibility", m_bTestVisibility)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new xiiDynamicEnumAttribute("PhysicsCollisionLayer")),    
    XII_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    XII_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new xiiDefaultValueAttribute(xiiColorScheme::LightUI(xiiColorScheme::Orange))),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCategoryAttribute("AI/Sensors"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

xiiSensorComponent::xiiSensorComponent()  = default;
xiiSensorComponent::~xiiSensorComponent() = default;

void xiiSensorComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_sSpatialCategory;
  s << m_bTestVisibility;
  s << m_uiCollisionLayer;
  s << m_UpdateRate;
  s << m_bShowDebugInfo;
  s << m_Color;
}

void xiiSensorComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_sSpatialCategory;
  s >> m_bTestVisibility;
  s >> m_uiCollisionLayer;
  s >> m_UpdateRate;
  s >> m_bShowDebugInfo;
  s >> m_Color;
}

void xiiSensorComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateSpatialCategory();
  UpdateScheduling();
  UpdateDebugInfo();
}

void xiiSensorComponent::OnDeactivated()
{
  auto pModule = GetWorld()->GetOrCreateModule<xiiSensorWorldModule>();
  pModule->RemoveComponentToSchedule(this);
  pModule->RemoveComponentForDebugRendering(this);

  SUPER::OnDeactivated();
}

void xiiSensorComponent::SetSpatialCategory(const char* szCategory)
{
  m_sSpatialCategory.Assign(szCategory);

  if (IsActiveAndInitialized())
  {
    UpdateSpatialCategory();
  }
}

const char* xiiSensorComponent::GetSpatialCategory() const
{
  return m_sSpatialCategory;
}

void xiiSensorComponent::SetUpdateRate(const xiiEnum<xiiUpdateRate>& updateRate)
{
  if (m_UpdateRate == updateRate)
    return;

  m_UpdateRate = updateRate;

  if (IsActiveAndInitialized())
  {
    UpdateScheduling();
  }
}

const xiiEnum<xiiUpdateRate>& xiiSensorComponent::GetUpdateRate() const
{
  return m_UpdateRate;
}

void xiiSensorComponent::SetShowDebugInfo(bool bShow)
{
  if (m_bShowDebugInfo == bShow)
    return;

  m_bShowDebugInfo = bShow;

  if (IsActiveAndInitialized())
  {
    UpdateDebugInfo();
  }
}

bool xiiSensorComponent::GetShowDebugInfo() const
{
  return m_bShowDebugInfo;
}

void xiiSensorComponent::SetColor(xiiColorGammaUB color)
{
  m_Color = color;
}

xiiColorGammaUB xiiSensorComponent::GetColor() const
{
  return m_Color;
}

bool xiiSensorComponent::RunSensorCheck(xiiPhysicsWorldModuleInterface* pPhysicsWorldModule, xiiDynamicArray<xiiGameObject*>& out_objectsInSensorVolume, xiiDynamicArray<xiiGameObjectHandle>& ref_detectedObjects, bool bPostChangeMsg) const
{
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
  m_LastOccludedObjectPositions.Clear();
#endif

  out_objectsInSensorVolume.Clear();

  GetObjectsInSensorVolume(out_objectsInSensorVolume);
  const xiiGameObject* pSensorOwner = GetOwner();

  ref_detectedObjects.Clear();

  if (m_bTestVisibility && pPhysicsWorldModule)
  {
    const xiiVec3 rayStart = pSensorOwner->GetGlobalPosition();
    for (auto pObject : out_objectsInSensorVolume)
    {
      const xiiVec3 rayEnd    = pObject->GetGlobalPosition();
      xiiVec3       rayDir    = rayEnd - rayStart;
      const float   fDistance = rayDir.GetLengthAndNormalize();

      xiiPhysicsCastResult      hitResult;
      xiiPhysicsQueryParameters params(m_uiCollisionLayer);
      params.m_bIgnoreInitialOverlap = true;
      params.m_ShapeTypes            = xiiPhysicsShapeType::Default;

      // TODO: probably best to expose the xiiPhysicsShapeType bitflags on the component
      params.m_ShapeTypes.Remove(xiiPhysicsShapeType::Rope);
      params.m_ShapeTypes.Remove(xiiPhysicsShapeType::Ragdoll);
      params.m_ShapeTypes.Remove(xiiPhysicsShapeType::Trigger);
      params.m_ShapeTypes.Remove(xiiPhysicsShapeType::Query);
      params.m_ShapeTypes.Remove(xiiPhysicsShapeType::Character);

      if (pPhysicsWorldModule->Raycast(hitResult, rayStart, rayDir, fDistance, params))
      {
        // hit something in between -> not visible
#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
        m_LastOccludedObjectPositions.PushBack(rayEnd);
#endif

        continue;
      }

      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }
  else
  {
    for (auto pObject : out_objectsInSensorVolume)
    {
      ref_detectedObjects.PushBack(pObject->GetHandle());
    }
  }

  ref_detectedObjects.Sort();
  if (ref_detectedObjects == m_LastDetectedObjects)
    return false;

  ref_detectedObjects.Swap(m_LastDetectedObjects);

  if (bPostChangeMsg)
  {
    xiiMsgSensorDetectedObjectsChanged msg;
    msg.m_DetectedObjects = m_LastDetectedObjects;
    pSensorOwner->PostEventMessage(msg, this, xiiTime::MakeZero(), xiiObjectMsgQueueType::PostAsync);
  }

  return true;
}

void xiiSensorComponent::UpdateSpatialCategory()
{
  if (!m_sSpatialCategory.IsEmpty())
  {
    m_SpatialCategory = xiiSpatialData::RegisterCategory(m_sSpatialCategory, xiiSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = xiiInvalidSpatialDataCategory;
  }
}

void xiiSensorComponent::UpdateScheduling()
{
  auto pModule = GetWorld()->GetOrCreateModule<xiiSensorWorldModule>();

  if (m_UpdateRate == xiiUpdateRate::Never)
    pModule->RemoveComponentToSchedule(this);
  else
    pModule->AddComponentToSchedule(this, m_UpdateRate);
}

void xiiSensorComponent::UpdateDebugInfo()
{
  auto pModule = GetWorld()->GetOrCreateModule<xiiSensorWorldModule>();
  if (IsActiveAndInitialized() && m_bShowDebugInfo)
  {
    pModule->AddComponentForDebugRendering(this);
  }
  else
  {
    pModule->RemoveComponentForDebugRendering(this);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSensorSphereComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiSphereManipulatorAttribute("Radius"),
    new xiiSphereVisualizerAttribute("Radius", xiiColor::White, "Color"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSensorSphereComponent::xiiSensorSphereComponent()  = default;
xiiSensorSphereComponent::~xiiSensorSphereComponent() = default;

void xiiSensorSphereComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void xiiSensorSphereComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
}

void xiiSensorSphereComponent::GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const
{
  const xiiGameObject* pOwner = GetOwner();

  const float             scale  = pOwner->GetGlobalTransformSimd().GetMaxScale();
  const xiiBoundingSphere sphere = xiiBoundingSphere(pOwner->GetGlobalPosition(), m_fRadius * scale);

  xiiSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  xiiSimdMat4f toLocalSpace  = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  xiiSimdFloat radiusSquared = m_fRadius * m_fRadius;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](xiiGameObject* pObject) {
    xiiSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<3>() <= radiusSquared;

    if (bInRadius)
    {
      out_objects.PushBack(pObject);
    }

    return xiiVisitorExecution::Continue; });
}

void xiiSensorSphereComponent::DebugDrawSensorShape() const
{
  const xiiBoundingSphere sphere = xiiBoundingSphere(xiiVec3::MakeZero(), m_fRadius);
  xiiDebugRenderer::DrawLineSphere(GetWorld(), sphere, m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSensorCylinderComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Height", m_fHeight)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
  }
  XII_END_PROPERTIES;

  XII_BEGIN_ATTRIBUTES
  {
    new xiiCylinderVisualizerAttribute(xiiBasisAxis::PositiveZ, "Height", "Radius", xiiColor::White, "Color"),
  }
  XII_END_ATTRIBUTES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSensorCylinderComponent::xiiSensorCylinderComponent()  = default;
xiiSensorCylinderComponent::~xiiSensorCylinderComponent() = default;

void xiiSensorCylinderComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fHeight;
}

void xiiSensorCylinderComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fHeight;
}

void xiiSensorCylinderComponent::GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const
{
  const xiiGameObject* pOwner = GetOwner();

  const xiiVec3 scale   = pOwner->GetGlobalScaling().Abs();
  const float   xyScale = xiiMath::Max(scale.x, scale.y);

  const float             sphereRadius = xiiVec2(m_fRadius * xyScale, m_fHeight * 0.5f * scale.z).GetLength();
  const xiiBoundingSphere sphere       = xiiBoundingSphere(pOwner->GetGlobalPosition(), sphereRadius);

  xiiSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  xiiSimdMat4f toLocalSpace  = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  xiiSimdFloat radiusSquared = m_fRadius * m_fRadius;
  xiiSimdFloat halfHeight    = m_fHeight * 0.5f;

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](xiiGameObject* pObject) {
    xiiSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const bool bInRadius = localSpacePos.GetLengthSquared<2>() <= radiusSquared;
    const bool bInHeight = localSpacePos.Abs().z() <= halfHeight;

    if (bInRadius && bInHeight)
    {
      out_objects.PushBack(pObject);
    }

    return xiiVisitorExecution::Continue; });
}

void xiiSensorCylinderComponent::DebugDrawSensorShape() const
{
  xiiTransform pt = GetOwner()->GetGlobalTransform();

  xiiQuat r;
  r.SetFromAxisAndAngle(xiiVec3(0, 1, 0), xiiAngle::MakeFromDegree(-90.0f));
  xiiTransform t = xiiTransform(xiiVec3(0, 0, -0.5f * m_fHeight * pt.m_vScale.z), r, xiiVec3(pt.m_vScale.z, pt.m_vScale.y, pt.m_vScale.x));

  pt.m_vScale.Set(1);
  t = pt * t;

  xiiColor solidColor = xiiColor::Black.WithAlpha(0.0f); // lines only
  xiiDebugRenderer::DrawCylinder(GetWorld(), m_fRadius, m_fRadius, m_fHeight, solidColor, m_Color, t);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_BEGIN_COMPONENT_TYPE(xiiSensorConeComponent, 1, xiiComponentMode::Static)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("NearDistance", m_fNearDistance)->AddAttributes(new xiiDefaultValueAttribute(0.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("FarDistance", m_fFarDistance)->AddAttributes(new xiiDefaultValueAttribute(10.0f), new xiiClampValueAttribute(0.0f, xiiVariant())),
    XII_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(90.0f)), new xiiClampValueAttribute(0.0f, xiiAngle::MakeFromDegree(180.0f))),
  }
  XII_END_PROPERTIES;
}
XII_END_COMPONENT_TYPE
// clang-format on

xiiSensorConeComponent::xiiSensorConeComponent()  = default;
xiiSensorConeComponent::~xiiSensorConeComponent() = default;

void xiiSensorConeComponent::SerializeComponent(xiiWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fNearDistance;
  s << m_fFarDistance;
  s << m_Angle;
}

void xiiSensorConeComponent::DeserializeComponent(xiiWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const xiiUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fNearDistance;
  s >> m_fFarDistance;
  s >> m_Angle;
}

void xiiSensorConeComponent::GetObjectsInSensorVolume(xiiDynamicArray<xiiGameObject*>& out_objects) const
{
  const xiiGameObject* pOwner = GetOwner();

  const float             scale  = pOwner->GetGlobalTransformSimd().GetMaxScale();
  const xiiBoundingSphere sphere = xiiBoundingSphere(pOwner->GetGlobalPosition(), m_fFarDistance * scale);

  xiiSpatialSystem::QueryParams params;
  params.m_uiCategoryBitmask = m_SpatialCategory.GetBitmask();

  xiiSimdMat4f       toLocalSpace = pOwner->GetGlobalTransformSimd().GetAsMat4().GetInverse();
  const xiiSimdFloat nearSquared  = m_fNearDistance * m_fNearDistance;
  const xiiSimdFloat farSquared   = m_fFarDistance * m_fFarDistance;
  const xiiSimdFloat cosAngle     = xiiMath::Cos(m_Angle * 0.5f);

  GetWorld()->GetSpatialSystem()->FindObjectsInSphere(sphere, params, [&](xiiGameObject* pObject) {
    xiiSimdVec4f localSpacePos = toLocalSpace.TransformPosition(pObject->GetGlobalPositionSimd());
    const xiiSimdFloat fDistanceSquared = localSpacePos.GetLengthSquared<3>();
    const bool bInDistance = fDistanceSquared >= nearSquared && fDistanceSquared <= farSquared;

    const xiiSimdVec4f normalizedPos = localSpacePos * fDistanceSquared.GetInvSqrt();
    const bool bInAngle = normalizedPos.x() >= cosAngle;

    if (bInDistance && bInAngle)
    {
      out_objects.PushBack(pObject);
    }

    return xiiVisitorExecution::Continue; });
}

void xiiSensorConeComponent::DebugDrawSensorShape() const
{
  constexpr xiiUInt32 MIN_SEGMENTS    = 3;
  constexpr xiiUInt32 MAX_SEGMENTS    = 16;
  constexpr xiiUInt32 CIRCLE_SEGMENTS = MAX_SEGMENTS * 2;
  constexpr xiiUInt32 NUM_LINES       = MAX_SEGMENTS * 4 + CIRCLE_SEGMENTS * 2 + 4;

  xiiDebugRenderer::Line lines[NUM_LINES];
  xiiUInt32              curLine = 0;

  const xiiUInt32 numSegments     = xiiMath::Clamp(static_cast<xiiUInt32>(m_Angle / xiiAngle::MakeFromDegree(180) * MAX_SEGMENTS), MIN_SEGMENTS, MAX_SEGMENTS);
  const xiiAngle  stepAngle       = m_Angle / static_cast<float>(numSegments);
  const xiiAngle  circleStepAngle = xiiAngle::MakeFromDegree(360.0f / CIRCLE_SEGMENTS);

  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    xiiAngle curAngle = m_Angle * -0.5f;

    xiiQuat q;
    float   fX            = xiiMath::Cos(curAngle);
    float   fCircleRadius = xiiMath::Sin(curAngle);

    if (i == 0)
    {
      q.SetIdentity();
      fX *= m_fNearDistance;
      fCircleRadius *= m_fNearDistance;
    }
    else
    {
      q.SetFromAxisAndAngle(xiiVec3::MakeAxisX(), xiiAngle::MakeFromDegree(90));
      fX *= m_fFarDistance;
      fCircleRadius *= m_fFarDistance;
    }

    for (xiiUInt32 s = 0; s < numSegments; ++s)
    {
      const xiiAngle nextAngle = curAngle + stepAngle;

      const float fCos1 = xiiMath::Cos(curAngle);
      const float fCos2 = xiiMath::Cos(nextAngle);

      const float fSin1 = xiiMath::Sin(curAngle);
      const float fSin2 = xiiMath::Sin(nextAngle);

      curAngle = nextAngle;

      const xiiVec3 p1 = q * xiiVec3(fCos1, fSin1, 0.0f);
      const xiiVec3 p2 = q * xiiVec3(fCos2, fSin2, 0.0f);

      lines[curLine].m_start = p1 * m_fNearDistance;
      lines[curLine].m_end   = p2 * m_fNearDistance;
      ++curLine;

      lines[curLine].m_start = p1 * m_fFarDistance;
      lines[curLine].m_end   = p2 * m_fFarDistance;
      ++curLine;

      if (s == 0)
      {
        lines[curLine].m_start = p1 * m_fNearDistance;
        lines[curLine].m_end   = p1 * m_fFarDistance;
        ++curLine;
      }
      else if (s == numSegments - 1)
      {
        lines[curLine].m_start = p2 * m_fNearDistance;
        lines[curLine].m_end   = p2 * m_fFarDistance;
        ++curLine;
      }
    }

    curAngle = xiiAngle::MakeFromDegree(0.0f);
    for (xiiUInt32 s = 0; s < CIRCLE_SEGMENTS; ++s)
    {
      const xiiAngle nextAngle = curAngle + circleStepAngle;

      const float fCos1 = xiiMath::Cos(curAngle);
      const float fCos2 = xiiMath::Cos(nextAngle);

      const float fSin1 = xiiMath::Sin(curAngle);
      const float fSin2 = xiiMath::Sin(nextAngle);

      curAngle = nextAngle;

      const xiiVec3 p1 = xiiVec3(fX, fCos1 * fCircleRadius, fSin1 * fCircleRadius);
      const xiiVec3 p2 = xiiVec3(fX, fCos2 * fCircleRadius, fSin2 * fCircleRadius);

      lines[curLine].m_start = p1;
      lines[curLine].m_end   = p2;
      ++curLine;
    }
  }

  XII_ASSERT_DEV(curLine <= NUM_LINES, "");
  xiiDebugRenderer::DrawLines(GetWorld(), xiiMakeArrayPtr(lines, curLine), m_Color, GetOwner()->GetGlobalTransform());
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
XII_IMPLEMENT_WORLD_MODULE(xiiSensorWorldModule);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiSensorWorldModule, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiSensorWorldModule::xiiSensorWorldModule(xiiWorld* pWorld) :
  xiiWorldModule(pWorld)
{
}

void xiiSensorWorldModule::Initialize()
{
  SUPER::Initialize();

  {
    auto updateDesc                        = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiSensorWorldModule::UpdateSensors, this);
    updateDesc.m_Phase                     = xiiWorldModule::UpdateFunctionDesc::Phase::Async;
    updateDesc.m_bOnlyUpdateWhenSimulating = true;

    RegisterUpdateFunction(updateDesc);
  }

  {
    auto updateDesc    = XII_CREATE_MODULE_UPDATE_FUNCTION_DESC(xiiSensorWorldModule::DebugDrawSensors, this);
    updateDesc.m_Phase = xiiWorldModule::UpdateFunctionDesc::Phase::PostTransform;

    RegisterUpdateFunction(updateDesc);
  }

  m_pPhysicsWorldModule = GetWorld()->GetOrCreateModule<xiiPhysicsWorldModuleInterface>();
}

void xiiSensorWorldModule::AddComponentToSchedule(xiiSensorComponent* pComponent, xiiUpdateRate::Enum updateRate)
{
  XII_ASSERT_DEBUG(updateRate != xiiUpdateRate::Never, "Invalid update rate for scheduling");
  m_Scheduler.AddOrUpdateWork(pComponent->GetHandle(), xiiUpdateRate::GetInterval(updateRate));
}

void xiiSensorWorldModule::RemoveComponentToSchedule(xiiSensorComponent* pComponent)
{
  m_Scheduler.RemoveWork(pComponent->GetHandle());
}

void xiiSensorWorldModule::AddComponentForDebugRendering(xiiSensorComponent* pComponent)
{
  xiiComponentHandle hComponent = pComponent->GetHandle();
  if (m_DebugComponents.Contains(hComponent) == false)
  {
    m_DebugComponents.PushBack(hComponent);
  }
}

void xiiSensorWorldModule::RemoveComponentForDebugRendering(xiiSensorComponent* pComponent)
{
  m_DebugComponents.RemoveAndSwap(pComponent->GetHandle());
}

void xiiSensorWorldModule::UpdateSensors(const xiiWorldModule::UpdateContext& context)
{
  if (m_pPhysicsWorldModule == nullptr)
    return;

  const xiiTime deltaTime = GetWorld()->GetClock().GetTimeDiff();
  m_Scheduler.Update(deltaTime, [this](const xiiComponentHandle& hComponent, xiiTime deltaTime) {
    const xiiWorld*           pWorld           = GetWorld();
    const xiiSensorComponent* pSensorComponent = nullptr;
    XII_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

    pSensorComponent->RunSensorCheck(m_pPhysicsWorldModule, m_ObjectsInSensorVolume, m_DetectedObjects, true);
    //
  });
}

void xiiSensorWorldModule::DebugDrawSensors(const xiiWorldModule::UpdateContext& context)
{
  xiiHybridArray<xiiDebugRenderer::Line, 256> lines;
  const xiiWorld*                             pWorld = GetWorld();

  for (xiiComponentHandle hComponent : m_DebugComponents)
  {
    lines.Clear();

    const xiiSensorComponent* pSensorComponent = nullptr;
    XII_VERIFY(pWorld->TryGetComponent(hComponent, pSensorComponent), "Invalid component handle");

    pSensorComponent->DebugDrawSensorShape();

    const xiiVec3 sensorPos = pSensorComponent->GetOwner()->GetGlobalPosition();
    for (xiiGameObjectHandle hObject : pSensorComponent->m_LastDetectedObjects)
    {
      const xiiGameObject* pObject = nullptr;
      if (pWorld->TryGetObject(hObject, pObject) == false)
        continue;

      lines.PushBack({sensorPos, pObject->GetGlobalPosition(), xiiColor::Lime});
    }

#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT)
    for (const xiiVec3& occludedPos : pSensorComponent->m_LastOccludedObjectPositions)
    {
      lines.PushBack({sensorPos, occludedPos, xiiColor::Red});
    }
#endif

    xiiDebugRenderer::DrawLines(pWorld, lines, xiiColor::White);
  }
}


XII_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_SensorComponent);
