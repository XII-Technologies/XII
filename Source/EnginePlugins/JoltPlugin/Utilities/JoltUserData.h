#pragma once

#include <JoltPlugin/JoltPluginDLL.h>

class xiiSurfaceResource;
class xiiComponent;
class xiiJoltDynamicActorComponent;
class xiiJoltStaticActorComponent;
class xiiJoltTriggerComponent;
class xiiJoltCharacterControllerComponent;
class xiiJoltShapeComponent;
class xiiJoltQueryShapeActorComponent;
class xiiJoltRagdollComponent;
class xiiJoltRopeComponent;
class xiiJoltActorComponent;

class xiiJoltUserData
{
public:
  XII_DECLARE_POD_TYPE();

  xiiJoltUserData() = default;
  ~xiiJoltUserData() { Invalidate(); }

  XII_ALWAYS_INLINE void Init(xiiJoltDynamicActorComponent* pObject)
  {
    m_Type    = Type::DynamicActorComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltStaticActorComponent* pObject)
  {
    m_Type    = Type::StaticActorComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltTriggerComponent* pObject)
  {
    m_Type    = Type::TriggerComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltCharacterControllerComponent* pObject)
  {
    m_Type    = Type::CharacterComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltShapeComponent* pObject)
  {
    m_Type    = Type::ShapeComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltQueryShapeActorComponent* pObject)
  {
    m_Type    = Type::QueryShapeActorComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiSurfaceResource* pObject)
  {
    m_Type    = Type::SurfaceResource;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltRagdollComponent* pObject)
  {
    m_Type    = Type::RagdollComponent;
    m_pObject = pObject;
  }

  XII_ALWAYS_INLINE void Init(xiiJoltRopeComponent* pObject)
  {
    m_Type    = Type::RopeComponent;
    m_pObject = pObject;
  }

  XII_FORCE_INLINE void Invalidate()
  {
    m_Type    = Type::Invalid;
    m_pObject = nullptr;
  }

  XII_FORCE_INLINE static xiiComponent* GetComponent(const void* pUserData)
  {
    const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
    if (pJoltUserData == nullptr ||
        pJoltUserData->m_Type == Type::Invalid ||
        pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return nullptr;
    }

    return static_cast<xiiComponent*>(pJoltUserData->m_pObject);
  }

  XII_FORCE_INLINE static xiiJoltActorComponent* GetActorComponent(const void* pUserData)
  {
    const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr &&
        (pJoltUserData->m_Type == Type::DynamicActorComponent ||
         pJoltUserData->m_Type == Type::StaticActorComponent ||
         pJoltUserData->m_Type == Type::QueryShapeActorComponent))
    {
      return static_cast<xiiJoltActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  XII_FORCE_INLINE static xiiJoltDynamicActorComponent* GetDynamicActorComponent(const void* pUserData)
  {
    const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::DynamicActorComponent)
    {
      return static_cast<xiiJoltDynamicActorComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  //XII_FORCE_INLINE static xiiJoltShapeComponent* GetShapeComponent(const void* pUserData)
  //{
  //  const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
  //  if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::ShapeComponent)
  //  {
  //    return static_cast<xiiJoltShapeComponent*>(pJoltUserData->m_pObject);
  //  }

  //  return nullptr;
  //}

  XII_FORCE_INLINE static xiiJoltTriggerComponent* GetTriggerComponent(const void* pUserData)
  {
    const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::TriggerComponent)
    {
      return static_cast<xiiJoltTriggerComponent*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

  XII_FORCE_INLINE static const xiiSurfaceResource* GetSurfaceResource(const void* pUserData)
  {
    const xiiJoltUserData* pJoltUserData = static_cast<const xiiJoltUserData*>(pUserData);
    if (pJoltUserData != nullptr && pJoltUserData->m_Type == Type::SurfaceResource)
    {
      return static_cast<const xiiSurfaceResource*>(pJoltUserData->m_pObject);
    }

    return nullptr;
  }

private:
  enum class Type
  {
    Invalid,
    DynamicActorComponent,
    StaticActorComponent,
    TriggerComponent,
    CharacterComponent,
    ShapeComponent,
    BreakableSheetComponent,
    SurfaceResource,
    QueryShapeActorComponent,
    RagdollComponent,
    RopeComponent,
  };

  Type  m_Type    = Type::Invalid;
  void* m_pObject = nullptr;
};
