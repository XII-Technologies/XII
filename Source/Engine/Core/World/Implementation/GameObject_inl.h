/// Copyright (c) Theophilus Eriata. All Rights Reserved.

XII_ALWAYS_INLINE xiiGameObject::ConstChildIterator::ConstChildIterator(xiiGameObject* pObject, const xiiWorld* pWorld) :
  m_pObject(pObject), m_pWorld(pWorld)
{
}

XII_ALWAYS_INLINE const xiiGameObject& xiiGameObject::ConstChildIterator::operator*() const
{
  return *m_pObject;
}

XII_ALWAYS_INLINE const xiiGameObject* xiiGameObject::ConstChildIterator::operator->() const
{
  return m_pObject;
}

XII_ALWAYS_INLINE xiiGameObject::ConstChildIterator::operator const xiiGameObject*() const
{
  return m_pObject;
}

XII_ALWAYS_INLINE bool xiiGameObject::ConstChildIterator::IsValid() const
{
  return m_pObject != nullptr;
}

XII_ALWAYS_INLINE void xiiGameObject::ConstChildIterator::operator++()
{
  Next();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE xiiGameObject::ChildIterator::ChildIterator(xiiGameObject* pObject, const xiiWorld* pWorld) :
  ConstChildIterator(pObject, pWorld)
{
}

XII_ALWAYS_INLINE xiiGameObject& xiiGameObject::ChildIterator::operator*()
{
  return *m_pObject;
}

XII_ALWAYS_INLINE xiiGameObject* xiiGameObject::ChildIterator::operator->()
{
  return m_pObject;
}

XII_ALWAYS_INLINE xiiGameObject::ChildIterator::operator xiiGameObject*()
{
  return m_pObject;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

inline xiiGameObject::xiiGameObject() = default;

XII_ALWAYS_INLINE xiiGameObject::xiiGameObject(const xiiGameObject& other)
{
  *this = other;
}

XII_ALWAYS_INLINE xiiGameObjectHandle xiiGameObject::GetHandle() const
{
  return xiiGameObjectHandle(m_InternalId);
}

XII_ALWAYS_INLINE bool xiiGameObject::IsDynamic() const
{
  return m_Flags.IsSet(xiiObjectFlags::Dynamic);
}

XII_ALWAYS_INLINE bool xiiGameObject::IsStatic() const
{
  return !m_Flags.IsSet(xiiObjectFlags::Dynamic);
}

XII_ALWAYS_INLINE bool xiiGameObject::GetActiveFlag() const
{
  return m_Flags.IsSet(xiiObjectFlags::ActiveFlag);
}

XII_ALWAYS_INLINE bool xiiGameObject::IsActive() const
{
  return m_Flags.IsSet(xiiObjectFlags::ActiveState);
}

XII_ALWAYS_INLINE void xiiGameObject::SetName(xiiStringView sName)
{
  m_sName.Assign(sName);
}

XII_ALWAYS_INLINE void xiiGameObject::SetName(const xiiHashedString& sName)
{
  m_sName = sName;
}

XII_ALWAYS_INLINE void xiiGameObject::SetGlobalKey(xiiStringView sKey)
{
  xiiHashedString sGlobalKey;
  sGlobalKey.Assign(sKey);
  SetGlobalKey(sGlobalKey);
}

XII_ALWAYS_INLINE xiiStringView xiiGameObject::GetName() const
{
  return m_sName.GetView();
}

XII_ALWAYS_INLINE const xiiHashedString& xiiGameObject::GetNameHashed() const
{
  return m_sName;
}

XII_ALWAYS_INLINE bool xiiGameObject::HasName(const xiiTempHashedString& sName) const
{
  return m_sName == sName;
}

XII_ALWAYS_INLINE void xiiGameObject::EnableChildChangesNotifications()
{
  m_Flags.Add(xiiObjectFlags::ChildChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::DisableChildChangesNotifications()
{
  m_Flags.Remove(xiiObjectFlags::ChildChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::EnableParentChangesNotifications()
{
  m_Flags.Add(xiiObjectFlags::ParentChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::DisableParentChangesNotifications()
{
  m_Flags.Remove(xiiObjectFlags::ParentChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::AddChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiTransformPreservation::Enum preserve)
{
  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    AddChild(children[i], preserve);
  }
}

XII_ALWAYS_INLINE void xiiGameObject::DetachChildren(const xiiArrayPtr<const xiiGameObjectHandle>& children, xiiTransformPreservation::Enum preserve)
{
  for (xiiUInt32 i = 0; i < children.GetCount(); ++i)
  {
    DetachChild(children[i], preserve);
  }
}

XII_ALWAYS_INLINE xiiUInt32 xiiGameObject::GetChildCount() const
{
  return m_uiChildCount;
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalPosition(xiiVec3 vPosition)
{
  SetLocalPosition(xiiSimdConversion::ToVec3(vPosition));
}

XII_ALWAYS_INLINE xiiVec3 xiiGameObject::GetLocalPosition() const
{
  return xiiSimdConversion::ToVec3(m_pTransformationData->m_localPosition);
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalRotation(xiiQuat qRotation)
{
  SetLocalRotation(xiiSimdConversion::ToQuat(qRotation));
}

XII_ALWAYS_INLINE xiiQuat xiiGameObject::GetLocalRotation() const
{
  return xiiSimdConversion::ToQuat(m_pTransformationData->m_localRotation);
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalScaling(xiiVec3 vScaling)
{
  SetLocalScaling(xiiSimdConversion::ToVec3(vScaling));
}

XII_ALWAYS_INLINE xiiVec3 xiiGameObject::GetLocalScaling() const
{
  return xiiSimdConversion::ToVec3(m_pTransformationData->m_localScaling);
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalUniformScaling(float fScaling)
{
  SetLocalUniformScaling(xiiSimdFloat(fScaling));
}

XII_ALWAYS_INLINE float xiiGameObject::GetLocalUniformScaling() const
{
  return m_pTransformationData->m_localScaling.w();
}

XII_ALWAYS_INLINE xiiTransform xiiGameObject::GetLocalTransform() const
{
  return xiiSimdConversion::ToTransform(GetLocalTransformSimd());
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalPosition(const xiiVec3& vPosition)
{
  SetGlobalPosition(xiiSimdConversion::ToVec3(vPosition));
}

XII_ALWAYS_INLINE xiiVec3 xiiGameObject::GetGlobalPosition() const
{
  return xiiSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Position);
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalRotation(const xiiQuat& qRotation)
{
  SetGlobalRotation(xiiSimdConversion::ToQuat(qRotation));
}

XII_ALWAYS_INLINE xiiQuat xiiGameObject::GetGlobalRotation() const
{
  return xiiSimdConversion::ToQuat(m_pTransformationData->m_globalTransform.m_Rotation);
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalScaling(const xiiVec3& vScaling)
{
  SetGlobalScaling(xiiSimdConversion::ToVec3(vScaling));
}

XII_ALWAYS_INLINE xiiVec3 xiiGameObject::GetGlobalScaling() const
{
  return xiiSimdConversion::ToVec3(m_pTransformationData->m_globalTransform.m_Scale);
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalTransform(const xiiTransform& transform)
{
  SetGlobalTransform(xiiSimdConversion::ToTransform(transform));
}

XII_ALWAYS_INLINE xiiTransform xiiGameObject::GetGlobalTransform() const
{
  return xiiSimdConversion::ToTransform(m_pTransformationData->m_globalTransform);
}

XII_ALWAYS_INLINE xiiTransform xiiGameObject::GetLastGlobalTransform() const
{
  return xiiSimdConversion::ToTransform(GetLastGlobalTransformSimd());
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalPosition(const xiiSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localPosition = vPosition;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdVec4f& xiiGameObject::GetLocalPositionSimd() const
{
  return m_pTransformationData->m_localPosition;
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalRotation(const xiiSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localRotation = qRotation;

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdQuat& xiiGameObject::GetLocalRotationSimd() const
{
  return m_pTransformationData->m_localRotation;
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalScaling(const xiiSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior)
{
  xiiSimdFloat uniformScale             = m_pTransformationData->m_localScaling.w();
  m_pTransformationData->m_localScaling = vScaling;
  m_pTransformationData->m_localScaling.SetW(uniformScale);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdVec4f& xiiGameObject::GetLocalScalingSimd() const
{
  return m_pTransformationData->m_localScaling;
}


XII_ALWAYS_INLINE void xiiGameObject::SetLocalUniformScaling(const xiiSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior)
{
  m_pTransformationData->m_localScaling.SetW(fScaling);

  if (IsStatic() && updateBehavior == UpdateBehaviorIfStatic::UpdateImmediately)
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE xiiSimdFloat xiiGameObject::GetLocalUniformScalingSimd() const
{
  return m_pTransformationData->m_localScaling.w();
}

XII_ALWAYS_INLINE xiiSimdTransform xiiGameObject::GetLocalTransformSimd() const
{
  const xiiSimdVec4f vScale = m_pTransformationData->m_localScaling * m_pTransformationData->m_localScaling.w();
  return xiiSimdTransform(m_pTransformationData->m_localPosition, m_pTransformationData->m_localRotation, vScale);
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalPosition(const xiiSimdVec4f& vPosition)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Position = vPosition;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdVec4f& xiiGameObject::GetGlobalPositionSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Position;
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalRotation(const xiiSimdQuat& qRotation)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Rotation = qRotation;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdQuat& xiiGameObject::GetGlobalRotationSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Rotation;
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalScaling(const xiiSimdVec4f& vScaling)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform.m_Scale = vScaling;

  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdVec4f& xiiGameObject::GetGlobalScalingSimd() const
{
  return m_pTransformationData->m_globalTransform.m_Scale;
}


XII_ALWAYS_INLINE void xiiGameObject::SetGlobalTransform(const xiiSimdTransform& transform)
{
  UpdateLastGlobalTransform();

  m_pTransformationData->m_globalTransform = transform;

  // xiiTransformTemplate<Type>::SetLocalTransform will produce NaNs in w components
  // of pos and scale if scale.w is not set to 1 here. This only affects builds that
  // use XII_SIMD_IMPLEMENTATION_FPU, e.g. arm atm.
  m_pTransformationData->m_globalTransform.m_Scale.SetW(1.0f);
  m_pTransformationData->UpdateLocalTransform();

  if (IsStatic())
  {
    UpdateGlobalTransformAndBoundsRecursive();
  }
}

XII_ALWAYS_INLINE const xiiSimdTransform& xiiGameObject::GetGlobalTransformSimd() const
{
  return m_pTransformationData->m_globalTransform;
}

XII_ALWAYS_INLINE const xiiSimdTransform& xiiGameObject::GetLastGlobalTransformSimd() const
{
#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  return m_pTransformationData->m_lastGlobalTransform;
#else
  return m_pTransformationData->m_globalTransform;
#endif
}

XII_ALWAYS_INLINE void xiiGameObject::EnableStaticTransformChangesNotifications()
{
  m_Flags.Add(xiiObjectFlags::StaticTransformChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::DisableStaticTransformChangesNotifications()
{
  m_Flags.Remove(xiiObjectFlags::StaticTransformChangesNotifications);
}

XII_ALWAYS_INLINE xiiBoundingBoxSphere xiiGameObject::GetLocalBounds() const
{
  return xiiSimdConversion::ToBBoxSphere(m_pTransformationData->m_localBounds);
}

XII_ALWAYS_INLINE xiiBoundingBoxSphere xiiGameObject::GetGlobalBounds() const
{
  return xiiSimdConversion::ToBBoxSphere(m_pTransformationData->m_globalBounds);
}

XII_ALWAYS_INLINE const xiiSimdBBoxSphere& xiiGameObject::GetLocalBoundsSimd() const
{
  return m_pTransformationData->m_localBounds;
}

XII_ALWAYS_INLINE const xiiSimdBBoxSphere& xiiGameObject::GetGlobalBoundsSimd() const
{
  return m_pTransformationData->m_globalBounds;
}

XII_ALWAYS_INLINE xiiSpatialDataHandle xiiGameObject::GetSpatialData() const
{
  return m_pTransformationData->m_hSpatialData;
}

XII_ALWAYS_INLINE void xiiGameObject::EnableComponentChangesNotifications()
{
  m_Flags.Add(xiiObjectFlags::ComponentChangesNotifications);
}

XII_ALWAYS_INLINE void xiiGameObject::DisableComponentChangesNotifications()
{
  m_Flags.Remove(xiiObjectFlags::ComponentChangesNotifications);
}

template <typename T>
XII_ALWAYS_INLINE bool xiiGameObject::TryGetComponentOfBaseType(T*& out_pComponent)
{
  return TryGetComponentOfBaseType(xiiGetStaticRTTI<T>(), (xiiComponent*&)out_pComponent);
}

template <typename T>
XII_ALWAYS_INLINE bool xiiGameObject::TryGetComponentOfBaseType(const T*& out_pComponent) const
{
  return TryGetComponentOfBaseType(xiiGetStaticRTTI<T>(), (const xiiComponent*&)out_pComponent);
}

template <typename T>
void xiiGameObject::TryGetComponentsOfBaseType(xiiDynamicArray<T*>& out_components)
{
  out_components.Clear();

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<T*>(pComponent));
    }
  }
}

template <typename T>
void xiiGameObject::TryGetComponentsOfBaseType(xiiDynamicArray<const T*>& out_components) const
{
  out_components.Clear();

  for (xiiUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    xiiComponent* pComponent = m_Components[i];
    if (pComponent->IsInstanceOf<T>())
    {
      out_components.PushBack(static_cast<const T*>(pComponent));
    }
  }
}

XII_ALWAYS_INLINE xiiArrayPtr<xiiComponent* const> xiiGameObject::GetComponents()
{
  return m_Components;
}

XII_ALWAYS_INLINE xiiArrayPtr<const xiiComponent* const> xiiGameObject::GetComponents() const
{
  return xiiMakeArrayPtr(const_cast<const xiiComponent* const*>(m_Components.GetData()), m_Components.GetCount());
}

XII_ALWAYS_INLINE xiiUInt16 xiiGameObject::GetComponentVersion() const
{
  return m_Components.GetUserData<ComponentUserData>().m_uiVersion;
}

XII_ALWAYS_INLINE bool xiiGameObject::SendMessage(xiiMessage& ref_msg)
{
  return SendMessageInternal(ref_msg, false);
}

XII_ALWAYS_INLINE bool xiiGameObject::SendMessage(xiiMessage& ref_msg) const
{
  return SendMessageInternal(ref_msg, false);
}

XII_ALWAYS_INLINE bool xiiGameObject::SendMessageRecursive(xiiMessage& ref_msg)
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

XII_ALWAYS_INLINE bool xiiGameObject::SendMessageRecursive(xiiMessage& ref_msg) const
{
  return SendMessageRecursiveInternal(ref_msg, false);
}

XII_ALWAYS_INLINE const xiiTagSet& xiiGameObject::GetTags() const
{
  return m_Tags;
}

XII_ALWAYS_INLINE bool xiiGameObject::HasTag(const xiiTempHashedString& sTagName) const
{
  return m_Tags.IsSetByName(sTagName);
}

XII_ALWAYS_INLINE xiiUInt32 xiiGameObject::GetStableRandomSeed() const
{
  return m_pTransformationData->m_uiStableRandomSeed;
}

XII_ALWAYS_INLINE void xiiGameObject::SetStableRandomSeed(xiiUInt32 uiSeed)
{
  m_pTransformationData->m_uiStableRandomSeed = uiSeed;
}

//////////////////////////////////////////////////////////////////////////

XII_ALWAYS_INLINE void xiiGameObject::TransformationData::UpdateGlobalTransformWithoutParent(xiiUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  m_globalTransform.m_Position = m_localPosition;
  m_globalTransform.m_Rotation = m_localRotation;
  m_globalTransform.m_Scale    = m_localScaling * m_localScaling.w();
}

XII_ALWAYS_INLINE void xiiGameObject::TransformationData::UpdateGlobalTransformWithParent(xiiUInt32 uiUpdateCounter)
{
  UpdateLastGlobalTransform(uiUpdateCounter);

  const xiiSimdVec4f     vScale = m_localScaling * m_localScaling.w();
  const xiiSimdTransform localTransform(m_localPosition, m_localRotation, vScale);
  m_globalTransform = xiiSimdTransform::MakeGlobalTransform(m_pParentData->m_globalTransform, localTransform);
}

XII_FORCE_INLINE void xiiGameObject::TransformationData::UpdateGlobalBounds()
{
  m_globalBounds = m_localBounds;
  m_globalBounds.Transform(m_globalTransform);
}

XII_ALWAYS_INLINE void xiiGameObject::TransformationData::UpdateLastGlobalTransform(xiiUInt32 uiUpdateCounter)
{
#if XII_ENABLED(XII_GAMEOBJECT_VELOCITY)
  if (m_uiLastGlobalTransformUpdateCounter != uiUpdateCounter)
  {
    m_lastGlobalTransform                = m_globalTransform;
    m_uiLastGlobalTransformUpdateCounter = uiUpdateCounter;
  }
#endif
}
