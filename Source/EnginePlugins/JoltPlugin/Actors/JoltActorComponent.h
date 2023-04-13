#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <Foundation/Types/Bitflags.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>

namespace JPH
{
  class Shape;
  class BodyCreationSettings;
} // namespace JPH

class XII_JOLTPLUGIN_DLL xiiJoltActorComponent : public xiiComponent
{
  XII_DECLARE_ABSTRACT_COMPONENT_TYPE(xiiJoltActorComponent, xiiComponent);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltActorComponent

public:
  xiiJoltActorComponent();
  ~xiiJoltActorComponent();

  xiiUInt8 m_uiCollisionLayer = 0; // [ property ]

  const xiiJoltUserData* GetUserData() const;

  /// \brief Sets the object filter ID to use. This can only be set right after creation, before the component gets activated.
  void SetInitialObjectFilterID(xiiUInt32 uiObjectFilterID);

  /// \brief The object filter ID can be used to ignore collisions specifically with this one object.
  xiiUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; }

protected:
  void ExtractSubShapeGeometry(const xiiGameObject* pObject, xiiMsgExtractGeometry& msg) const;

  static void GatherShapes(xiiDynamicArray<xiiJoltSubShape>& shapes, xiiGameObject* pObject, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial);
  xiiResult   CreateShape(JPH::BodyCreationSettings* pSettings, float fDensity, const xiiJoltMaterial* pMaterial);

  virtual void CreateShapes(xiiDynamicArray<xiiJoltSubShape>& out_Shapes, const xiiTransform& rootTransform, float fDensity, const xiiJoltMaterial* pMaterial) {}

  xiiUInt32 m_uiUserDataIndex  = xiiInvalidIndex;
  xiiUInt32 m_uiJoltBodyID     = xiiInvalidIndex;
  xiiUInt32 m_uiObjectFilterID = xiiInvalidIndex;
};
