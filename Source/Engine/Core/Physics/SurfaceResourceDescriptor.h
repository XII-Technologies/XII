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
  void          SetPrefab(xiiStringView sPrefab);
  xiiStringView GetPrefab() const;

  xiiString m_sInteractionType;

  xiiPrefabResourceHandle                 m_hPrefab;
  xiiEnum<xiiSurfaceInteractionAlignment> m_Alignment;
  xiiAngle                                m_Deviation;
  float                                   m_fImpulseThreshold = 0.0f;
  float                                   m_fImpulseScale     = 1.0f;

  const xiiRangeView<xiiStringView, xiiUInt32> GetParameters() const;                                         // [ property ] (exposed parameter)
  void                                         SetParameter(xiiStringView sKey, const xiiVariant& value);     // [ property ] (exposed parameter)
  void                                         RemoveParameter(xiiStringView sKey);                           // [ property ] (exposed parameter)
  bool                                         GetParameter(xiiStringView sKey, xiiVariant& out_value) const; // [ property ] (exposed parameter)

  xiiArrayMap<xiiHashedString, xiiVariant> m_Parameters;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiSurfaceInteraction);

struct XII_CORE_DLL xiiSurfaceResourceDescriptor : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSurfaceResourceDescriptor, xiiReflectedClass);

public:
  void Load(xiiStreamReader& ref_stream);
  void Save(xiiStreamWriter& ref_stream) const;

  void          SetBaseSurfaceFile(xiiStringView sFile);
  xiiStringView GetBaseSurfaceFile() const;

  void          SetCollisionInteraction(xiiStringView sName);
  xiiStringView GetCollisionInteraction() const;

  void          SetSlideReactionPrefabFile(xiiStringView sFile);
  xiiStringView GetSlideReactionPrefabFile() const;

  void          SetRollReactionPrefabFile(xiiStringView sFile);
  xiiStringView GetRollReactionPrefabFile() const;


  xiiSurfaceResourceHandle m_hBaseSurface;
  float                    m_fPhysicsRestitution;
  float                    m_fPhysicsFrictionStatic;
  float                    m_fPhysicsFrictionDynamic;
  xiiHashedString          m_sOnCollideInteraction;
  xiiHashedString          m_sSlideInteractionPrefab;
  xiiHashedString          m_sRollInteractionPrefab;

  xiiHybridArray<xiiSurfaceInteraction, 16> m_Interactions;
};
