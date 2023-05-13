#pragma once

#include <Core/CoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/Variant.h>

using xiiSurfaceResourceHandle = xiiTypedResourceHandle<class xiiSurfaceResource>;
using xiiPrefabResourceHandle  = xiiTypedResourceHandle<class xiiPrefabResource>;

struct xiiSurfaceInteractionAlignment
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    SurfaceNormal,
    IncidentDirection,
    ReflectedDirection,
    ReverseSurfaceNormal,
    ReverseIncidentDirection,
    ReverseReflectedDirection,

    Default = SurfaceNormal
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiSurfaceInteractionAlignment);


struct XII_CORE_DLL xiiSurfaceInteraction
{
  void        SetPrefab(const char* szPrefab);
  const char* GetPrefab() const;

  xiiString m_sInteractionType;

  xiiPrefabResourceHandle                 m_hPrefab;
  xiiEnum<xiiSurfaceInteractionAlignment> m_Alignment;
  xiiAngle                                m_Deviation;
  float                                   m_fImpulseThreshold = 0.0f;
  float                                   m_fImpulseScale     = 1.0f;

  const xiiRangeView<const char*, xiiUInt32> GetParameters() const;                                        // [ property ] (exposed parameter)
  void                                       SetParameter(const char* szKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                       RemoveParameter(const char* szKey);                           // [ property ] (exposed parameter)
  bool                                       GetParameter(const char* szKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiSurfaceInteraction);

struct XII_CORE_DLL xiiSurfaceResourceDescriptor : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceResourceDescriptor, xiiReflectedClass);

public:
  void Load(xiiStreamReader& stream);
  void Save(xiiStreamWriter& stream) const;

  void        SetBaseSurfaceFile(const char* szFile);
  const char* GetBaseSurfaceFile() const;

  void        SetCollisionInteraction(const char* name);
  const char* GetCollisionInteraction() const;

  void        SetSlideReactionPrefabFile(const char* szFile);
  const char* GetSlideReactionPrefabFile() const;

  void        SetRollReactionPrefabFile(const char* szFile);
  const char* GetRollReactionPrefabFile() const;


  xiiSurfaceResourceHandle m_hBaseSurface;
  float                    m_fPhysicsRestitution;
  float                    m_fPhysicsFrictionStatic;
  float                    m_fPhysicsFrictionDynamic;
  xiiHashedString          m_sOnCollideInteraction;
  xiiHashedString          m_sSlideInteractionPrefab;
  xiiHashedString          m_sRollInteractionPrefab;

  xiiHybridArray<xiiSurfaceInteraction, 16> m_Interactions;
};
