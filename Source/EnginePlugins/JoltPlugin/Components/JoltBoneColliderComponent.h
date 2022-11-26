#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>

struct xiiMsgAnimationPoseUpdated;

using xiiSkeletonResourceHandle = xiiTypedResourceHandle<class xiiSkeletonResource>;

using xiiJoltBoneColliderComponentManager = xiiComponentManager<class xiiJoltBoneColliderComponent, xiiBlockStorageType::Compact>;

class XII_JOLTPLUGIN_DLL xiiJoltBoneColliderComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiJoltBoneColliderComponent, xiiComponent, xiiJoltBoneColliderComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiJoltBoneColliderComponent

public:
  xiiJoltBoneColliderComponent();
  ~xiiJoltBoneColliderComponent();

  xiiUInt32 GetObjectFilterID() const { return m_uiObjectFilterID; } // [ scriptable ]

  bool    m_bQueryShapeOnly = true; // [ property ]
  xiiTime m_UpdateThreshold;        // [ property ]

  void OnAnimationPoseUpdated(xiiMsgAnimationPoseUpdated& msg); // [ msg handler ]

  /// \brief Destroys the current shape objects and creates new ones.
  ///
  /// This can be used to update the shapes when for example the object was scaled.
  /// Be aware that all child objects that were attached to the previous physics shape objects will
  /// be deleted as well. So any 'attachments' will disappear.
  void RecreatePhysicsShapes(); // [ scriptable ]

protected:
  void CreatePhysicsShapes(const xiiSkeletonResourceHandle& hSkeleton);
  void DestroyPhysicsShapes();

  struct Shape
  {
    xiiUInt16 m_uiAttachedToBone = 0xFFFF;
    xiiVec3   m_vOffsetPos;
    xiiQuat   m_qOffsetRot;

    xiiGameObjectHandle m_hActorObject;
  };

  xiiUInt32              m_uiObjectFilterID = xiiInvalidIndex;
  xiiTime                m_LastUpdate;
  xiiDynamicArray<Shape> m_Shapes;
};
