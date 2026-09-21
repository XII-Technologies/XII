/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/World/World.h>

struct xiiMsgUpdateLocalBounds;
struct xiiMsgExtractRenderData;

struct XII_GAMEENGINE_DLL xiiGrabbableItemGrabPoint
{
  xiiVec3 m_vLocalPosition;
  xiiQuat m_qLocalRotation;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiGrabbableItemGrabPoint);

//////////////////////////////////////////////////////////////////////////

using xiiGrabbableItemComponentManager = xiiComponentManager<class xiiGrabbableItemComponent, xiiBlockStorageType::Compact>;

/// Used to define 'grab points' on an object where a player can pick up and hold the item
///
/// The grabbable item component is typically added to objects with a dynamic physics actor to mark it as an item that can be
/// picked up, and to define the anchor points at which the object can be held.
/// Of course a game can utilize this information without a physical actor and physically holding objects as well.
///
/// Each grab point defines how the object would be oriented when held.
///
/// The component only holds data, it doesn't add any custom behavior. It is the responsibility of other components to use this
/// data in a sensible way.
class XII_GAMEENGINE_DLL xiiGrabbableItemComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiGrabbableItemComponent, xiiComponent, xiiGrabbableItemComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiGrabbableItemComponent

public:
  xiiGrabbableItemComponent();
  ~xiiGrabbableItemComponent();

  void SetDebugShowPoints(bool bShow); // [ property ]
  bool GetDebugShowPoints() const;     // [ property ]

  static void DebugDrawGrabPoint(const xiiWorld& world, const xiiTransform& globalGrabPointTransform);

  xiiDynamicArray<xiiGrabbableItemGrabPoint> m_GrabPoints; // [ property ]

protected:
  void OnUpdateLocalBounds(xiiMsgUpdateLocalBounds& msg) const;
  void OnExtractRenderData(xiiMsgExtractRenderData& msg) const;
};
