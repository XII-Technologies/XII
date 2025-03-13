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

struct xiiSkeletonPoseMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    CustomPose,
    RestPose,
    Disabled,
    Default = CustomPose
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiSkeletonPoseMode);


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

  void                             SetSkeleton(const xiiSkeletonResourceHandle& hResource); // [ property ]
  const xiiSkeletonResourceHandle& GetSkeleton() const { return m_hSkeleton; }              // [ property ]

  xiiEnum<xiiSkeletonPoseMode> GetPoseMode() const { return m_PoseMode; }
  void                         SetPoseMode(xiiEnum<xiiSkeletonPoseMode> mode);

  void ResendPose();

  const xiiRangeView<xiiStringView, xiiUInt32> GetBones() const;                                         // [ property ] (exposed bones)
  void                                         SetBone(xiiStringView sKey, const xiiVariant& value);     // [ property ] (exposed bones)
  void                                         RemoveBone(xiiStringView sKey);                           // [ property ] (exposed bones)
  bool                                         GetBone(xiiStringView sKey, xiiVariant& out_value) const; // [ property ] (exposed bones)

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
