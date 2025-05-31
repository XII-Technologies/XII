#pragma once

#include <Core/World/ComponentManager.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/RangeView.h>
#include <GraphicsCore/AnimationSystem/EditableSkeleton.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

class xiiSkeletonPoseComponentManager : public xiiComponentManager<class xiiSkeletonPoseComponent, xiiBlockStorageType::Compact>
{
public:
  using SUPER = xiiComponentManager<xiiSkeletonPoseComponent, xiiBlockStorageType::Compact>;

  xiiSkeletonPoseComponentManager(xiiWorld* pWorld) :
    SUPER(pWorld)
  {
  }

  void Update(const xiiWorldModule::UpdateContext& context);
  void EnqueueUpdate(xiiComponentHandle hComponent);

private:
  mutable xiiMutex             m_Mutex;
  xiiDeque<xiiComponentHandle> m_RequireUpdate;

protected:
  virtual void Initialize() override;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Which pose to apply to an animated mesh.
struct xiiSkeletonPoseMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    CustomPose, ///< Set a custom pose on the mesh.
    RestPose,   ///< Set the rest pose (bind pose) on the mesh.
    Disabled,   ///< Don't set any pose.
    Default = CustomPose
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSkeletonPoseMode);

/// \brief Used in conjunction with a xiiAnimatedMeshComponent to set a specific pose for the animated mesh.
///
/// This component is used to set one, static pose for an animated mesh. The pose is applied once at startup.
/// This can be used to either just pose a mesh in a certain way, or to set a start pose that is then used
/// by other systems, for example a ragdoll component, to generate further poses.
///
/// The component needs to be attached to the same game object where the animated mesh component is attached.
class XII_GRAPHICSCORE_DLL xiiSkeletonPoseComponent : public xiiComponent
{
  XII_DECLARE_COMPONENT_TYPE(xiiSkeletonPoseComponent, xiiComponent, xiiSkeletonPoseComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // xiiComponent

public:
  virtual void SerializeComponent(xiiWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(xiiWorldReader& inout_stream) override;

protected:
  virtual void OnActivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSkeletonPoseComponent

public:
  xiiSkeletonPoseComponent();
  ~xiiSkeletonPoseComponent();

  /// \brief Sets the xiiSkeletonResource to use.
  void                             SetSkeleton(const xiiSkeletonResourceHandle& hResource); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }              // [ property ]

  /// \brief Configures which pose to apply to the animated mesh.
  void                         SetPoseMode(xiiEnum<xiiSkeletonPoseMode> mode);
  xiiEnum<xiiSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }

  const xiiRangeView<xiiStringView, xiiUInt32> GetBones() const;                                    // [ property ] (exposed bones)
  void                                         SetBone(xiiStringView, const xiiVariant& value);     // [ property ] (exposed bones)
  void                                         RemoveBone(xiiStringView);                           // [ property ] (exposed bones)
  bool                                         GetBone(xiiStringView, xiiVariant& out_value) const; // [ property ] (exposed bones)

  /// \brief Instructs the component to apply the pose to the animated mesh again.
  void ResendPose();

protected:
  void Update();
  void SendRestPose();
  void SendCustomPose();

  float                                        m_fDummy       = 0;
  xiiUInt8                                     m_uiResendPose = 0;
  xiiSkeletonResourceHandle                    m_hSkeleton;
  xiiArrayMap<xiiHashedString, xiiExposedBone> m_Bones;    // [ property ]
  xiiEnum<xiiSkeletonPoseMode>                 m_PoseMode; // [ property ]
};
