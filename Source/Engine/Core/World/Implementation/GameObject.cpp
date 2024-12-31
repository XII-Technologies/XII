#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

namespace
{
  static xiiVariantArray GetDefaultTags()
  {
    xiiVariantArray value(xiiStaticAllocatorWrapper::GetAllocator());
    value.PushBack("CastShadow");
    return value;
  }
} // namespace

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiGameObject, xiiNoBase, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Name", GetName, SetName),
    XII_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new xiiDefaultValueAttribute(true)),
    XII_ACCESSOR_PROPERTY("GlobalKey", GetGlobalKey, SetGlobalKey),
    XII_ENUM_ACCESSOR_PROPERTY("Mode", xiiObjectMode, Reflection_GetMode, Reflection_SetMode),
    XII_ACCESSOR_PROPERTY("LocalPosition", GetLocalPosition, SetLocalPosition)->AddAttributes(new xiiSuffixAttribute(" m")),
    XII_ACCESSOR_PROPERTY("LocalRotation", GetLocalRotation, SetLocalRotation),
    XII_ACCESSOR_PROPERTY("LocalScaling", GetLocalScaling, SetLocalScaling)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(1.0f, 1.0f, 1.0f))),
    XII_ACCESSOR_PROPERTY("LocalUniformScaling", GetLocalUniformScaling, SetLocalUniformScaling)->AddAttributes(new xiiDefaultValueAttribute(1.0f)),
    XII_SET_ACCESSOR_PROPERTY("Tags", GetTags, Reflection_SetTag, Reflection_RemoveTag)->AddAttributes(new xiiTagSetWidgetAttribute("Default"), new xiiDefaultValueAttribute(GetDefaultTags())),
    XII_SET_ACCESSOR_PROPERTY("Children", Reflection_GetChildren, Reflection_AddChild, Reflection_DetachChild)->AddFlags(xiiPropertyFlags::PointerOwner | xiiPropertyFlags::Hidden),
    XII_SET_ACCESSOR_PROPERTY("Components", Reflection_GetComponents, Reflection_AddComponent, Reflection_RemoveComponent)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_SCRIPT_FUNCTION_PROPERTY(IsActive),
    XII_SCRIPT_FUNCTION_PROPERTY(SetCreatedByPrefab),
    XII_SCRIPT_FUNCTION_PROPERTY(WasCreatedByPrefab),

    XII_SCRIPT_FUNCTION_PROPERTY(HasName, In, "Name"),
    XII_SCRIPT_FUNCTION_PROPERTY(HasTag, In, "TagName"),

    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_GetParent),
    XII_SCRIPT_FUNCTION_PROPERTY(FindChildByName, In, "Name", In, "Recursive")->AddFlags(xiiPropertyFlags::Const),
    XII_SCRIPT_FUNCTION_PROPERTY(FindChildByPath, In, "Path")->AddFlags(xiiPropertyFlags::Const),

    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalPosition, In, "Position"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalPosition),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalRotation, In, "Rotation"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalRotation),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalScaling, In, "Scaling"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalScaling),
    XII_SCRIPT_FUNCTION_PROPERTY(Reflection_SetGlobalTransform, In, "Transform"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalTransform),

    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirForwards),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirRight),
    XII_SCRIPT_FUNCTION_PROPERTY(GetGlobalDirUp),

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
    XII_SCRIPT_FUNCTION_PROPERTY(GetLinearVelocity),
    XII_SCRIPT_FUNCTION_PROPERTY(GetAngularVelocity),
#endif

    XII_SCRIPT_FUNCTION_PROPERTY(SetTeamID, In, "Id"),
    XII_SCRIPT_FUNCTION_PROPERTY(GetTeamID),
  }
  XII_END_FUNCTIONS;
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(xiiMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

void xiiGameObject::Reflection_AddChild(xiiGameObject* pChild)
{
  if (IsDynamic())
  {
    pChild->MakeDynamic();
  }

  AddChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // Check whether the child object was only dynamic because of its old parent
  // If that's the case make it static now.
  pChild->ConditionalMakeStatic();
}

void xiiGameObject::Reflection_DetachChild(xiiGameObject* pChild)
{
  DetachChild(pChild->GetHandle(), TransformPreservation::PreserveLocal);

  // The child object is now a top level object, check whether it should be static now.
  pChild->ConditionalMakeStatic();
}

xiiHybridArray<xiiGameObject*, 8> xiiGameObject::Reflection_GetChildren() const
{
  ConstChildIterator it = GetChildren();

  xiiHybridArray<xiiGameObject*, 8> all;
  all.Reserve(GetChildCount());

  while (it.IsValid())
  {
    all.PushBack(it.m_pObject);
    ++it;
  }

  return all;
}

void xiiGameObject::Reflection_AddComponent(xiiComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  if (pComponent->IsDynamic())
  {
    MakeDynamic();
  }

  AddComponent(pComponent);
}

void xiiGameObject::Reflection_RemoveComponent(xiiComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  /*Don't call RemoveComponent here, Component is automatically removed when deleted.*/

  if (pComponent->IsDynamic())
  {
    ConditionalMakeStatic(pComponent);
  }
}

xiiHybridArray<xiiComponent*, xiiGameObject::NUM_INPLACE_COMPONENTS> xiiGameObject::Reflection_GetComponents() const
{
  return xiiHybridArray<xiiComponent*, xiiGameObject::NUM_INPLACE_COMPONENTS>(m_Components);
}

xiiObjectMode::Enum xiiGameObject::Reflection_GetMode() const
{
  return m_Flags.IsSet(xiiObjectFlags::ForceDynamic) ? xiiObjectMode::ForceDynamic : xiiObjectMode::Automatic;
}

void xiiGameObject::Reflection_SetMode(xiiObjectMode::Enum mode)
{
  if (Reflection_GetMode() == mode)
    return;

  if (mode == xiiObjectMode::ForceDynamic)
  {
    m_Flags.Add(xiiObjectFlags::ForceDynamic);
    MakeDynamic();
  }
  else
  {
    m_Flags.Remove(xiiObjectFlags::ForceDynamic);
    ConditionalMakeStatic();
  }
}

xiiGameObject* xiiGameObject::Reflection_GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void xiiGameObject::Reflection_SetGlobalPosition(const xiiVec3& vPosition)
{
  SetGlobalPosition(vPosition);
}

void xiiGameObject::Reflection_SetGlobalRotation(const xiiQuat& qRotation)
{
  SetGlobalRotation(qRotation);
}

void xiiGameObject::Reflection_SetGlobalScaling(const xiiVec3& vScaling)
{
  SetGlobalScaling(vScaling);
}

void xiiGameObject::Reflection_SetGlobalTransform(const xiiTransform& transform)
{
  SetGlobalTransform(transform);
}

bool xiiGameObject::DetermineDynamicMode(xiiComponent* pComponentToIgnore /*= nullptr*/) const
{
  if (m_Flags.IsSet(xiiObjectFlags::ForceDynamic))
  {
    return true;
  }

  const xiiGameObject* pParent = GetParent();
  if (pParent != nullptr && pParent->IsDynamic())
  {
    return true;
  }

  for (auto pComponent : m_Components)
  {
    if (pComponent != pComponentToIgnore && pComponent->IsDynamic())
    {
      return true;
    }
  }

  return false;
}

void xiiGameObject::ConditionalMakeStatic(xiiComponent* pComponentToIgnore /*= nullptr*/)
{
  if (!DetermineDynamicMode(pComponentToIgnore))
  {
    MakeStaticInternal();

    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->ConditionalMakeStatic();
    }
  }
}

void xiiGameObject::MakeStaticInternal()
{
  if (IsStatic())
    return;

  m_Flags.Remove(xiiObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, true);
}

void xiiGameObject::UpdateGlobalTransformAndBoundsRecursive()
{
  if (IsStatic() && GetWorld()->ReportErrorWhenStaticObjectMoves())
  {
    xiiLog::Error("Static object '{0}' was moved during runtime.", GetName());
  }

  xiiSimdTransform oldGlobalTransform = GetGlobalTransformSimd();

  m_pTransformationData->UpdateGlobalTransformNonRecursive(GetWorld()->GetUpdateCounter());

  if (xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    m_pTransformationData->UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
  else
  {
    m_pTransformationData->UpdateGlobalBounds();
  }

  if (IsStatic() && m_Flags.IsSet(xiiObjectFlags::StaticTransformChangesNotifications) && oldGlobalTransform != GetGlobalTransformSimd())
  {
    xiiMsgTransformChanged msg;
    msg.m_OldGlobalTransform = xiiSimdConversion::ToTransform(oldGlobalTransform);
    msg.m_NewGlobalTransform = GetGlobalTransform();

    SendMessage(msg);
  }

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->UpdateGlobalTransformAndBoundsRecursive();
  }
}

void xiiGameObject::UpdateLastGlobalTransform()
{
  m_pTransformationData->UpdateLastGlobalTransform(GetWorld()->GetUpdateCounter());
}

void xiiGameObject::ConstChildIterator::Next()
{
  m_pObject = m_pWorld->GetObjectUnchecked(m_pObject->m_uiNextSiblingIndex);
}

xiiGameObject::~xiiGameObject()
{
  // Since we are using the small array base class for components we have to cleanup ourself with the correct allocator.
  m_Components.Clear();
  m_Components.Compact(GetWorld()->GetAllocator());
}

void xiiGameObject::operator=(const xiiGameObject& other)
{
  XII_ASSERT_DEV(m_InternalId.m_WorldIndex == other.m_InternalId.m_WorldIndex, "Cannot copy between worlds.");

  m_InternalId = other.m_InternalId;
  m_Flags      = other.m_Flags;
  m_sName      = other.m_sName;

  m_uiParentIndex     = other.m_uiParentIndex;
  m_uiFirstChildIndex = other.m_uiFirstChildIndex;
  m_uiLastChildIndex  = other.m_uiLastChildIndex;

  m_uiNextSiblingIndex = other.m_uiNextSiblingIndex;
  m_uiPrevSiblingIndex = other.m_uiPrevSiblingIndex;
  m_uiChildCount       = other.m_uiChildCount;

  m_uiTeamID = other.m_uiTeamID;

  m_uiHierarchyLevel               = other.m_uiHierarchyLevel;
  m_pTransformationData            = other.m_pTransformationData;
  m_pTransformationData->m_pObject = this;

  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    pSpatialSystem->UpdateSpatialDataObject(m_pTransformationData->m_hSpatialData, this);
  }

  m_Components.CopyFrom(other.m_Components, GetWorld()->GetAllocator());
  for (xiiComponent* pComponent : m_Components)
  {
    XII_ASSERT_DEV(pComponent->m_pOwner == &other, "");
    pComponent->m_pOwner = this;
  }

  m_Tags = other.m_Tags;
}

void xiiGameObject::MakeDynamic()
{
  if (IsDynamic())
    return;

  m_Flags.Add(xiiObjectFlags::Dynamic);

  GetWorld()->RecreateHierarchyData(this, false);

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->MakeDynamic();
  }
}

void xiiGameObject::MakeStatic()
{
  XII_ASSERT_DEV(!DetermineDynamicMode(), "This object can't be static because it has a dynamic parent or dynamic component(s) attached.");

  MakeStaticInternal();
}

void xiiGameObject::SetActiveFlag(bool bEnabled)
{
  if (m_Flags.IsSet(xiiObjectFlags::ActiveFlag) == bEnabled)
    return;

  m_Flags.AddOrRemove(xiiObjectFlags::ActiveFlag, bEnabled);

  UpdateActiveState(GetParent() == nullptr ? true : GetParent()->IsActive());
}

void xiiGameObject::UpdateActiveState(bool bParentActive)
{
  const bool bSelfActive = bParentActive && m_Flags.IsSet(xiiObjectFlags::ActiveFlag);

  if (bSelfActive != m_Flags.IsSet(xiiObjectFlags::ActiveState))
  {
    m_Flags.AddOrRemove(xiiObjectFlags::ActiveState, bSelfActive);

    for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
    {
      m_Components[i]->UpdateActiveState(bSelfActive);
    }

    // recursively update all children
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      it->UpdateActiveState(bSelfActive);
    }
  }
}

void xiiGameObject::SetGlobalKey(const xiiHashedString& sName)
{
  GetWorld()->SetObjectGlobalKey(this, sName);
}

xiiStringView xiiGameObject::GetGlobalKey() const
{
  return GetWorld()->GetObjectGlobalKey(this);
}

void xiiGameObject::SetParent(const xiiGameObjectHandle& hParent, xiiGameObject::TransformPreservation preserve)
{
  xiiWorld* pWorld = GetWorld();

  xiiGameObject* pParent = nullptr;
  bool           _       = pWorld->TryGetObject(hParent, pParent);
  XII_IGNORE_UNUSED(_);
  pWorld->SetParent(this, pParent, preserve);
}

xiiGameObject* xiiGameObject::GetParent()
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

const xiiGameObject* xiiGameObject::GetParent() const
{
  return GetWorld()->GetObjectUnchecked(m_uiParentIndex);
}

void xiiGameObject::AddChild(const xiiGameObjectHandle& hChild, xiiGameObject::TransformPreservation preserve)
{
  xiiWorld* pWorld = GetWorld();

  xiiGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    pWorld->SetParent(pChild, this, preserve);
  }
}

void xiiGameObject::DetachChild(const xiiGameObjectHandle& hChild, xiiGameObject::TransformPreservation preserve)
{
  xiiWorld* pWorld = GetWorld();

  xiiGameObject* pChild = nullptr;
  if (pWorld->TryGetObject(hChild, pChild))
  {
    if (pChild->GetParent() == this)
    {
      pWorld->SetParent(pChild, nullptr, preserve);
    }
  }
}

xiiGameObject::ChildIterator xiiGameObject::GetChildren()
{
  xiiWorld* pWorld = GetWorld();
  return ChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

xiiGameObject::ConstChildIterator xiiGameObject::GetChildren() const
{
  const xiiWorld* pWorld = GetWorld();
  return ConstChildIterator(pWorld->GetObjectUnchecked(m_uiFirstChildIndex), pWorld);
}

xiiGameObject* xiiGameObject::FindChildByName(const xiiTempHashedString& sName, bool bRecursive /*= true*/)
{
  /// \test Needs a unit test

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == sName)
    {
      return &(*it);
    }
  }

  if (bRecursive)
  {
    for (auto it = GetChildren(); it.IsValid(); ++it)
    {
      xiiGameObject* pChild = it->FindChildByName(sName, bRecursive);

      if (pChild != nullptr)
        return pChild;
    }
  }

  return nullptr;
}

xiiGameObject* xiiGameObject::FindChildByPath(xiiStringView sPath)
{
  /// \test Needs a unit test

  if (sPath.IsEmpty())
    return this;

  const char* szSep      = sPath.FindSubString("/");
  xiiUInt64   uiNameHash = 0;

  if (szSep == nullptr)
    uiNameHash = xiiHashingUtils::StringHash(sPath);
  else
    uiNameHash = xiiHashingUtils::StringHash(xiiStringView(sPath.GetStartPointer(), szSep));

  xiiGameObject* pNextChild = FindChildByName(xiiTempHashedString(uiNameHash), false);

  if (szSep == nullptr || pNextChild == nullptr)
    return pNextChild;

  return pNextChild->FindChildByPath(xiiStringView(szSep + 1, sPath.GetEndPointer()));
}


xiiGameObject* xiiGameObject::SearchForChildByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent /*= nullptr*/)
{
  /// \test Needs a unit test

  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      xiiComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return nullptr;
    }

    return this;
  }

  const char*   szSep = sObjectSequence.FindSubString("/");
  xiiStringView sNextSequence;
  xiiUInt64     uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = xiiHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash    = xiiHashingUtils::StringHash(xiiStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = xiiStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const xiiTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      xiiGameObject* res = it->SearchForChildByNameSequence(sNextSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name)
    {
      xiiGameObject* res = it->SearchForChildByNameSequence(sObjectSequence, pExpectedComponent);
      if (res != nullptr)
        return res;
    }
  }

  return nullptr;
}


void xiiGameObject::SearchForChildrenByNameSequence(xiiStringView sObjectSequence, const xiiRTTI* pExpectedComponent, xiiHybridArray<xiiGameObject*, 8>& out_objects)
{
  /// \test Needs a unit test

  if (sObjectSequence.IsEmpty())
  {
    // in case we are searching for a specific component type, verify that it exists on this object
    if (pExpectedComponent != nullptr)
    {
      xiiComponent* pComp = nullptr;
      if (!TryGetComponentOfBaseType(pExpectedComponent, pComp))
        return;
    }

    out_objects.PushBack(this);
    return;
  }

  const char*   szSep = sObjectSequence.FindSubString("/");
  xiiStringView sNextSequence;
  xiiUInt64     uiNameHash = 0;

  if (szSep == nullptr)
  {
    uiNameHash = xiiHashingUtils::StringHash(sObjectSequence);
  }
  else
  {
    uiNameHash    = xiiHashingUtils::StringHash(xiiStringView(sObjectSequence.GetStartPointer(), szSep));
    sNextSequence = xiiStringView(szSep + 1, sObjectSequence.GetEndPointer());
  }

  const xiiTempHashedString name(uiNameHash);

  // first go through all direct children an see if any of them actually matches the current name
  // if so, continue the recursion from there and give them the remaining search path to continue
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName == name)
    {
      it->SearchForChildrenByNameSequence(sNextSequence, pExpectedComponent, out_objects);
    }
  }

  // if no direct child fulfilled the requirements, just recurse with the full name sequence
  // however, we can skip any child that already fulfilled the next sequence name,
  // because that's definitely a lost cause
  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    if (it->m_sName != name) // TODO: in this function it is actually debatable whether to skip these or not
    {
      it->SearchForChildrenByNameSequence(sObjectSequence, pExpectedComponent, out_objects);
    }
  }
}

xiiWorld* xiiGameObject::GetWorld()
{
  return xiiWorld::GetWorld(m_InternalId.m_WorldIndex);
}

const xiiWorld* xiiGameObject::GetWorld() const
{
  return xiiWorld::GetWorld(m_InternalId.m_WorldIndex);
}

xiiVec3 xiiGameObject::GetGlobalDirForwards() const
{
  xiiCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vForwardDir;
}

xiiVec3 xiiGameObject::GetGlobalDirRight() const
{
  xiiCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vRightDir;
}

xiiVec3 xiiGameObject::GetGlobalDirUp() const
{
  xiiCoordinateSystem coordinateSystem;
  GetWorld()->GetCoordinateSystem(GetGlobalPosition(), coordinateSystem);

  return GetGlobalRotation() * coordinateSystem.m_vUpDir;
}

#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
void xiiGameObject::SetLastGlobalTransform(const xiiSimdTransform& transform)
{
  m_pTransformationData->m_lastGlobalTransform                = transform;
  m_pTransformationData->m_uiLastGlobalTransformUpdateCounter = GetWorld()->GetUpdateCounter();
}

xiiVec3 xiiGameObject::GetLinearVelocity() const
{
  const xiiSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const xiiSimdVec4f linearVelocity  = (m_pTransformationData->m_globalTransform.m_Position - m_pTransformationData->m_lastGlobalTransform.m_Position) * invDeltaSeconds;
  return xiiSimdConversion::ToVec3(linearVelocity);
}

xiiVec3 xiiGameObject::GetAngularVelocity() const
{
  const xiiSimdFloat invDeltaSeconds = GetWorld()->GetInvDeltaSeconds();
  const xiiSimdQuat  q               = m_pTransformationData->m_globalTransform.m_Rotation * -m_pTransformationData->m_lastGlobalTransform.m_Rotation;
  xiiSimdVec4f       angularVelocity = xiiSimdVec4f::MakeZero();

  xiiSimdVec4f axis;
  xiiSimdFloat angle;
  if (q.GetRotationAxisAndAngle(axis, angle).Succeeded())
  {
    angularVelocity = axis * (angle * invDeltaSeconds);
  }
  return xiiSimdConversion::ToVec3(angularVelocity);
}
#endif

void xiiGameObject::UpdateGlobalTransform()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
}

void xiiGameObject::UpdateLocalBounds()
{
  xiiMsgUpdateLocalBounds msg;
  msg.m_ResultingLocalBounds = xiiBoundingBoxSphere::MakeInvalid();

  SendMessage(msg);

  const bool bIsAlwaysVisible     = m_pTransformationData->m_localBounds.m_BoxHalfExtents.w() != xiiSimdFloat::MakeZero();
  bool       bRecreateSpatialData = false;

  if (m_pTransformationData->m_hSpatialData.IsInvalidated() == false)
  {
    // force spatial data re-creation if categories have changed
    bRecreateSpatialData |= m_pTransformationData->m_uiSpatialDataCategoryBitmask != msg.m_uiSpatialDataCategoryBitmask;

    // force spatial data re-creation if always visible flag has changed
    bRecreateSpatialData |= bIsAlwaysVisible != msg.m_bAlwaysVisible;

    // delete old spatial data if bounds are now invalid
    bRecreateSpatialData |= msg.m_bAlwaysVisible == false && msg.m_ResultingLocalBounds.IsValid() == false;
  }

  m_pTransformationData->m_localBounds = xiiSimdConversion::ToBBoxSphere(msg.m_ResultingLocalBounds);
  m_pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(msg.m_bAlwaysVisible ? 1.0f : 0.0f);
  m_pTransformationData->m_uiSpatialDataCategoryBitmask = msg.m_uiSpatialDataCategoryBitmask;

  xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
  if (pSpatialSystem != nullptr && (bRecreateSpatialData || m_pTransformationData->m_hSpatialData.IsInvalidated()))
  {
    m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
  }

  if (IsStatic())
  {
    m_pTransformationData->UpdateGlobalBounds(pSpatialSystem);
  }
}

void xiiGameObject::UpdateGlobalTransformAndBounds()
{
  m_pTransformationData->UpdateGlobalTransformRecursive(GetWorld()->GetUpdateCounter());
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

void xiiGameObject::UpdateGlobalBounds()
{
  m_pTransformationData->UpdateGlobalBounds(GetWorld()->GetSpatialSystem());
}

bool xiiGameObject::TryGetComponentOfBaseType(const xiiRTTI* pType, xiiComponent*& out_pComponent)
{
  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}

bool xiiGameObject::TryGetComponentOfBaseType(const xiiRTTI* pType, const xiiComponent*& out_pComponent) const
{
  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_pComponent = pComponent;
      return true;
    }
  }

  out_pComponent = nullptr;
  return false;
}


void xiiGameObject::TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<xiiComponent*>& out_components)
{
  out_components.Clear();

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void xiiGameObject::TryGetComponentsOfBaseType(const xiiRTTI* pType, xiiDynamicArray<const xiiComponent*>& out_components) const
{
  out_components.Clear();

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf(pType))
    {
      out_components.PushBack(pComponent);
    }
  }
}

void xiiGameObject::SetTeamID(xiiUInt16 uiId)
{
  m_uiTeamID = uiId;

  for (auto it = GetChildren(); it.IsValid(); ++it)
  {
    it->SetTeamID(uiId);
  }
}

xiiVisibilityState xiiGameObject::GetVisibilityState(xiiUInt32 uiNumFramesBeforeInvisible) const
{
  if (!m_pTransformationData->m_hSpatialData.IsInvalidated())
  {
    const xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem();
    return pSpatialSystem->GetVisibilityState(m_pTransformationData->m_hSpatialData, uiNumFramesBeforeInvisible);
  }

  return xiiVisibilityState::Direct;
}

void xiiGameObject::OnMsgDeleteGameObject(xiiMsgDeleteGameObject& msg)
{
  GetWorld()->DeleteObjectNow(GetHandle(), msg.m_bDeleteEmptyParents);
}

void xiiGameObject::AddComponent(xiiComponent* pComponent)
{
  XII_ASSERT_DEV(pComponent->m_pOwner == nullptr, "Component must not be added twice.");
  XII_ASSERT_DEV(IsDynamic() || !pComponent->IsDynamic(), "Cannot attach a dynamic component to a static object. Call MakeDynamic() first.");

  pComponent->m_pOwner = this;
  m_Components.PushBack(pComponent, GetWorld()->GetAllocator());
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  pComponent->UpdateActiveState(IsActive());

  if (m_Flags.IsSet(xiiObjectFlags::ComponentChangesNotifications))
  {
    xiiMsgComponentsChanged msg;
    msg.m_Type       = xiiMsgComponentsChanged::Type::ComponentAdded;
    msg.m_hOwner     = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

void xiiGameObject::RemoveComponent(xiiComponent* pComponent)
{
  xiiUInt32 uiIndex = m_Components.IndexOf(pComponent);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "Component not found");

  pComponent->m_pOwner = nullptr;
  m_Components.RemoveAtAndSwap(uiIndex);
  m_Components.GetUserData<ComponentUserData>().m_uiVersion++;

  if (m_Flags.IsSet(xiiObjectFlags::ComponentChangesNotifications))
  {
    xiiMsgComponentsChanged msg;
    msg.m_Type       = xiiMsgComponentsChanged::Type::ComponentRemoved;
    msg.m_hOwner     = GetHandle();
    msg.m_hComponent = pComponent->GetHandle();

    SendNotificationMessage(msg);
  }
}

bool xiiGameObject::SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    xiiLog::Warning("xiiGameObject::SendMessage: None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool xiiGameObject::SendMessageInternal(xiiMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const xiiComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (!bSentToAny && msg.GetDebugMessageRouting())
  {
    xiiLog::Warning("xiiGameObject::SendMessage (const): None of the target object's components had a handler for messages of type {0}.", msg.GetId());
  }
#endif

  return bSentToAny;
}

bool xiiGameObject::SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg)
{
  bool bSentToAny = false;

  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    xiiLog::Warning("xiiGameObject::SendMessageRecursive: None of the target object's components had a handler for messages of type {0}.",
  //    msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

bool xiiGameObject::SendMessageRecursiveInternal(xiiMessage& msg, bool bWasPostedMsg) const
{
  bool bSentToAny = false;

  const xiiRTTI* pRtti = xiiGetStaticRTTI<xiiGameObject>();
  bSentToAny |= pRtti->DispatchMessage(this, msg);

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    // forward only to 'const' message handlers
    const xiiComponent* pComponent = m_Components[i];
    bSentToAny |= pComponent->SendMessageInternal(msg, bWasPostedMsg);
  }

  for (auto childIt = GetChildren(); childIt.IsValid(); ++childIt)
  {
    bSentToAny |= childIt->SendMessageRecursiveInternal(msg, bWasPostedMsg);
  }

  // should only be evaluated at the top function call
  // #if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  //  if (!bSentToAny && msg.GetDebugMessageRouting())
  //  {
  //    xiiLog::Warning("xiiGameObject::SendMessageRecursive(const): None of the target object's components had a handler for messages of type
  //    {0}.", msg.GetId());
  //  }
  // #endif
  // #
  return bSentToAny;
}

void xiiGameObject::Reflection_SetTag(xiiStringView sTagName)
{
  if (sTagName.IsEmpty())
    return;

  const xiiTag& tag = xiiTagRegistry::GetGlobalRegistry().RegisterTag(sTagName);

  SetTag(tag);
}

void xiiGameObject::Reflection_RemoveTag(xiiStringView sTagName)
{
  if (sTagName.IsEmpty())
    return;

  if (const xiiTag* pTag = xiiTagRegistry::GetGlobalRegistry().GetTagByName(xiiTempHashedString(sTagName)))
  {
    RemoveTag(*pTag);
  }
}

void xiiGameObject::PostMessage(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

void xiiGameObject::PostMessageRecursive(const xiiMessage& msg, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessageRecursive(GetHandle(), msg, delay, queueType);
}

bool xiiGameObject::SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent)
{
  if (auto pEventMsg = xiiDynamicCast<xiiEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  xiiHybridArray<xiiComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  if (ref_msg.GetDebugMessageRouting())
  {
    if (eventMsgHandlers.IsEmpty())
    {
      xiiLog::Warning("xiiGameObject::SendEventMessage: None of the target object's components had a handler for messages of type {0}.", ref_msg.GetId());
    }
  }
#endif

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

bool xiiGameObject::SendEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent) const
{
  if (auto pEventMsg = xiiDynamicCast<xiiEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  xiiHybridArray<const xiiComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  bool bResult = false;
  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    bResult |= pEventMsgHandler->SendMessage(ref_msg);
  }
  return bResult;
}

void xiiGameObject::PostEventMessage(xiiMessage& ref_msg, const xiiComponent* pSenderComponent, xiiTime delay, xiiObjectMsgQueueType::Enum queueType) const
{
  if (auto pEventMsg = xiiDynamicCast<xiiEventMessage*>(&ref_msg))
  {
    pEventMsg->FillFromSenderComponent(pSenderComponent);
  }

  xiiHybridArray<const xiiComponent*, 4> eventMsgHandlers;
  GetWorld()->FindEventMsgHandlers(ref_msg, this, eventMsgHandlers);

  for (auto pEventMsgHandler : eventMsgHandlers)
  {
    pEventMsgHandler->PostMessage(ref_msg, delay, queueType);
  }
}

void xiiGameObject::SetTags(const xiiTagSet& tags)
{
  if (xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags != tags)
    {
      m_Tags = tags;
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags = tags;
  }
}

void xiiGameObject::SetTag(const xiiTag& tag)
{
  if (xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag) == false)
    {
      m_Tags.Set(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Set(tag);
  }
}

void xiiGameObject::RemoveTag(const xiiTag& tag)
{
  if (xiiSpatialSystem* pSpatialSystem = GetWorld()->GetSpatialSystem())
  {
    if (m_Tags.IsSet(tag))
    {
      m_Tags.Remove(tag);
      m_pTransformationData->RecreateSpatialData(*pSpatialSystem);
    }
  }
  else
  {
    m_Tags.Remove(tag);
  }
}

void xiiGameObject::FixComponentPointer(xiiComponent* pOldPtr, xiiComponent* pNewPtr)
{
  xiiUInt32 uiIndex = m_Components.IndexOf(pOldPtr);
  XII_ASSERT_DEV(uiIndex != xiiInvalidIndex, "Memory corruption?");
  m_Components[uiIndex] = pNewPtr;
}

void xiiGameObject::SendNotificationMessage(xiiMessage& msg)
{
  xiiGameObject* pObject = this;
  while (pObject != nullptr)
  {
    pObject->SendMessage(msg);

    pObject = pObject->GetParent();
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiGameObject::TransformationData::UpdateLocalTransform()
{
  xiiSimdTransform tLocal;

  if (m_pParentData != nullptr)
  {
    tLocal = xiiSimdTransform::MakeLocalTransform(m_pParentData->m_globalTransform, m_globalTransform);
  }
  else
  {
    tLocal = m_globalTransform;
  }

  m_localPosition = tLocal.m_Position;
  m_localRotation = tLocal.m_Rotation;
  m_localScaling  = tLocal.m_Scale;
  m_localScaling.SetW(1.0f);
}

void xiiGameObject::TransformationData::UpdateGlobalTransformNonRecursive(xiiUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void xiiGameObject::TransformationData::UpdateGlobalTransformRecursive(xiiUInt32 uiUpdateCounter)
{
  if (m_pParentData != nullptr)
  {
    m_pParentData->UpdateGlobalTransformRecursive(uiUpdateCounter);
    UpdateGlobalTransformWithParent(uiUpdateCounter);
  }
  else
  {
    UpdateGlobalTransformWithoutParent(uiUpdateCounter);
  }
}

void xiiGameObject::TransformationData::UpdateGlobalBounds(xiiSpatialSystem* pSpatialSystem)
{
  if (pSpatialSystem == nullptr)
  {
    UpdateGlobalBounds();
  }
  else
  {
    UpdateGlobalBoundsAndSpatialData(*pSpatialSystem);
  }
}

void xiiGameObject::TransformationData::UpdateGlobalBoundsAndSpatialData(xiiSpatialSystem& ref_spatialSystem)
{
  xiiSimdBBoxSphere oldGlobalBounds = m_globalBounds;

  UpdateGlobalBounds();

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != xiiSimdFloat::MakeZero();
  if (m_hSpatialData.IsInvalidated() == false && bIsAlwaysVisible == false && m_globalBounds != oldGlobalBounds)
  {
    ref_spatialSystem.UpdateSpatialDataBounds(m_hSpatialData, m_globalBounds);
  }
}

void xiiGameObject::TransformationData::RecreateSpatialData(xiiSpatialSystem& ref_spatialSystem)
{
  if (m_hSpatialData.IsInvalidated() == false)
  {
    ref_spatialSystem.DeleteSpatialData(m_hSpatialData);
    m_hSpatialData.Invalidate();
  }

  const bool bIsAlwaysVisible = m_localBounds.m_BoxHalfExtents.w() != xiiSimdFloat::MakeZero();
  if (bIsAlwaysVisible)
  {
    m_hSpatialData = ref_spatialSystem.CreateSpatialDataAlwaysVisible(m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
  else if (m_localBounds.IsValid())
  {
    UpdateGlobalBounds();
    m_hSpatialData = ref_spatialSystem.CreateSpatialData(m_globalBounds, m_pObject, m_uiSpatialDataCategoryBitmask, m_pObject->m_Tags);
  }
}

XII_STATICLINK_FILE(Core, Core_World_Implementation_GameObject);
