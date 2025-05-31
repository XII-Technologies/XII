#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/AnimationSystem/Declarations.h>

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/maths/soa_float.h>
#include <ozz/base/maths/soa_transform.h>

XII_DEFINE_AS_POD_TYPE(ozz::math::SoaTransform);

class xiiSkeletonResource;
class xiiAnimPoseGenerator;
class xiiGameObject;

using xiiAnimationClipResourceHandle = xiiTypedResourceHandle<class xiiAnimationClipResource>;

using xiiAnimPoseGeneratorLocalPoseID = xiiUInt32;
using xiiAnimPoseGeneratorModelPoseID = xiiUInt32;
using xiiAnimPoseGeneratorCommandID   = xiiUInt32;

/// \brief The type of xiiAnimPoseGeneratorCommand
enum class xiiAnimPoseGeneratorCommandType
{
  Invalid,
  SampleTrack,
  RestPose,
  CombinePoses,
  LocalToModelPose,
  SampleEventTrack,
  AimIK,
  TwoBoneIK,
};

enum class xiiAnimPoseEventTrackSampleMode : xiiUInt8
{
  None,         ///< Don't sample the event track at all
  OnlyBetween,  ///< Sample the event track only between PrevSamplePos and SamplePos
  LoopAtEnd,    ///< Sample the event track between PrevSamplePos and End, then Start and SamplePos
  LoopAtStart,  ///< Sample the event track between PrevSamplePos and Start, then End and SamplePos
  BounceAtEnd,  ///< Sample the event track between PrevSamplePos and End, then End and SamplePos
  BounceAtStart ///< Sample the event track between PrevSamplePos and Start, then Start and SamplePos
};

/// \brief Base class for all pose generator commands
///
/// All commands have a unique command ID with which they are referenced.
/// All commands can have zero or N other commands set as *inputs*.
/// Every type of command only accepts certain types and amount of inputs.
///
/// The pose generation graph is built by allocating commands on the graph and then setting up
/// which command is an input to which other node.
/// A command can be an input to multiple other commands. It will be evaluated only once.
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommand
{
  xiiHybridArray<xiiAnimPoseGeneratorCommandID, 4> m_Inputs;

  xiiAnimPoseGeneratorCommandID   GetCommandID() const { return m_CommandID; }
  xiiAnimPoseGeneratorCommandType GetType() const { return m_Type; }

private:
  friend class xiiAnimPoseGenerator;

  bool                            m_bExecuted = false;
  xiiAnimPoseGeneratorCommandID   m_CommandID = xiiInvalidIndex;
  xiiAnimPoseGeneratorCommandType m_Type      = xiiAnimPoseGeneratorCommandType::Invalid;
};

/// \brief Returns the rest pose (also often called 'bind pose').
///
/// The command has to be added as an input to one of
/// * xiiAnimPoseGeneratorCommandCombinePoses
/// * xiiAnimPoseGeneratorCommandLocalToModelPose
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandRestPose final : public xiiAnimPoseGeneratorCommand
{
private:
  friend class xiiAnimPoseGenerator;

  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = xiiInvalidIndex;
};

/// \brief Samples an animation clip at a given time and optionally also its event track.
///
/// The command has to be added as an input to one of
/// * xiiAnimPoseGeneratorCommandCombinePoses
/// * xiiAnimPoseGeneratorCommandLocalToModelPose
///
/// If the event track shall be sampled as well, event messages are sent to the xiiGameObject for which the pose is generated.
///
/// This command can optionally have input commands of type xiiAnimPoseGeneratorCommandSampleEventTrack.
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandSampleTrack final : public xiiAnimPoseGeneratorCommand
{
  xiiAnimationClipResourceHandle m_hAnimationClip;
  float                          m_fNormalizedSamplePos;
  float                          m_fPreviousNormalizedSamplePos;

  xiiAnimPoseEventTrackSampleMode m_EventSampling = xiiAnimPoseEventTrackSampleMode::None;

private:
  friend class xiiAnimPoseGenerator;

  bool                            m_bAdditive       = false;
  xiiUInt32                       m_uiUniqueID      = 0;
  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = xiiInvalidIndex;
};

/// \brief Combines all the local space poses that are given as input into one local pose.
///
/// The input commands must be of type
/// * xiiAnimPoseGeneratorCommandSampleTrack
/// * xiiAnimPoseGeneratorCommandCombinePoses
/// * xiiAnimPoseGeneratorCommandRestPose
///
/// Every input pose gets both an overall weight, as well as optionally a per-bone weight mask.
/// If a per-bone mask is used, the respective input pose will only affect those bones.
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandCombinePoses final : public xiiAnimPoseGeneratorCommand
{
  xiiHybridArray<float, 4>                                    m_InputWeights;
  xiiHybridArray<xiiArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class xiiAnimPoseGenerator;

  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = xiiInvalidIndex;
};

/// \brief Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * xiiAnimPoseGeneratorCommandSampleTrack
/// * xiiAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandSampleEventTrack final : public xiiAnimPoseGeneratorCommand
{
  xiiAnimationClipResourceHandle m_hAnimationClip;
  float                          m_fNormalizedSamplePos;
  float                          m_fPreviousNormalizedSamplePos;

  xiiAnimPoseEventTrackSampleMode m_EventSampling = xiiAnimPoseEventTrackSampleMode::None;

private:
  friend class xiiAnimPoseGenerator;

  xiiUInt32 m_uiUniqueID = 0;
};

/// \brief Base class for commands that produce or update a model pose.
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandModelPose : public xiiAnimPoseGeneratorCommand
{
protected:
  friend class xiiAnimPoseGenerator;

  xiiAnimPoseGeneratorModelPoseID m_ModelPoseOutput = xiiInvalidIndex;
  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = xiiInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * xiiAnimPoseGeneratorCommandSampleTrack
/// * xiiAnimPoseGeneratorCommandCombinePoses
/// * xiiAnimPoseGeneratorCommandRestPose
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandLocalToModelPose final : public xiiAnimPoseGeneratorCommandModelPose
{
  xiiGameObject* m_pSendLocalPoseMsgTo = nullptr;
};

/// \brief Accepts a single input in model space and applies aim IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * xiiAnimPoseGeneratorCommandLocalToModelPose
/// * xiiAnimPoseGeneratorCommandAimIK
/// * xiiAnimPoseGeneratorCommandTwoBoneIK
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandAimIK final : public xiiAnimPoseGeneratorCommandModelPose
{
  xiiVec3   m_vTargetPosition;                                    ///< The position for the bone to point at. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  xiiUInt16 m_uiJointIdx;                                         ///< The index of the joint to aim.
  xiiUInt16 m_uiRecalcModelPoseToJointIdx = xiiInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  float     m_fWeight                     = 1.0f;                 ///< Factor between 0 and 1 for how much to apply the IK.
  xiiVec3   m_vForwardVector              = xiiVec3::MakeAxisX(); ///< The local joint direction that should aim at the target. Typically there is a convention to use +X, +Y or +Z.
  xiiVec3   m_vUpVector                   = xiiVec3::MakeAxisZ(); ///< The local joint direction that should point towards the pole vector. Must be orthogonal to the forward vector.
  xiiVec3   m_vPoleVector                 = xiiVec3::MakeAxisY(); ///< In the same space as the target position, a position that the up vector of the joint should (roughly) point towards. Used to have bones point into the right direction, for example to make an elbow point properly sideways.
};

/// \brief Accepts a single input in model space and applies two-bone IK (inverse kinematics) on it. Updates the model pose in place.
///
/// The input command must be of type
/// * xiiAnimPoseGeneratorCommandLocalToModelPose
/// * xiiAnimPoseGeneratorCommandAimIK
/// * xiiAnimPoseGeneratorCommandTwoBoneIK
struct XII_GRAPHICSCORE_DLL xiiAnimPoseGeneratorCommandTwoBoneIK final : public xiiAnimPoseGeneratorCommandModelPose
{
  xiiVec3   m_vTargetPosition;                                    ///< The position for the 'end' joint to try to reach. Must be in model space of the skeleton, ie even the m_RootTransform must have been removed.
  xiiUInt16 m_uiJointIdxStart;                                    ///< Index of the top joint in a chain of three joints. The IK result may freely rotate around this joint into any (unnatural direction).
  xiiUInt16 m_uiJointIdxMiddle;                                   ///< Index of the middle joint in a chain of three joints. The IK result will bend at this joint around the joint local mid-axis.
  xiiUInt16 m_uiJointIdxEnd;                                      ///< Index of the end joint that is supposed to reach the target.
  xiiUInt16 m_uiRecalcModelPoseToJointIdx = xiiInvalidJointIndex; ///< Optimization hint to prevent unnecessary recalculation of model poses for joints that get updated later again.
  xiiVec3   m_vMidAxis                    = xiiVec3::MakeAxisZ(); ///< The local joint direction around which to bend the middle joint. Typically there is a convention to use +X, +Y or +Z to bend around.
  xiiVec3   m_vPoleVector                 = xiiVec3::MakeAxisY(); ///< In the same space as the target position, a position that the middle joint should (roughly) point towards. Used to have bones point into the right direction, for example to make a knee point properly forwards.
  float     m_fWeight                     = 1.0f;                 ///< Factor between 0 and 1 for how much to apply the IK.
  float     m_fSoften                     = 1.0f;                 ///< Factor between 0 and 1. See OZZ for details.
  xiiAngle  m_TwistAngle;                                         ///< After IK how much to rotate the chain. Seems to be redundant with the pole vector. See OZZ for details.
};

/// \brief Low-level infrastructure to generate animation poses from animation clips and other inputs.
///
/// Even though instances of this class should be reused over frames, it is assumed that all commands are recreated every frame, to build a new pose.
/// Some commands take predecessor commands as inputs to combine. If a command turns out not be actually needed, it won't be evaluated.
class XII_GRAPHICSCORE_DLL xiiAnimPoseGenerator final
{
public:
  xiiAnimPoseGenerator();
  ~xiiAnimPoseGenerator();

  void Reset(const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget);

  const xiiSkeletonResource* GetSkeleton() const { return m_pSkeleton; }
  const xiiGameObject*       GetTargetObject() const { return m_pTargetGameObject; }

  xiiAnimPoseGeneratorCommandSampleTrack&      AllocCommandSampleTrack(xiiUInt32 uiDeterministicID);
  xiiAnimPoseGeneratorCommandRestPose&         AllocCommandRestPose();
  xiiAnimPoseGeneratorCommandCombinePoses&     AllocCommandCombinePoses();
  xiiAnimPoseGeneratorCommandLocalToModelPose& AllocCommandLocalToModelPose();
  xiiAnimPoseGeneratorCommandSampleEventTrack& AllocCommandSampleEventTrack();
  xiiAnimPoseGeneratorCommandAimIK&            AllocCommandAimIK();
  xiiAnimPoseGeneratorCommandTwoBoneIK&        AllocCommandTwoBoneIK();

  const xiiAnimPoseGeneratorCommand& GetCommand(xiiAnimPoseGeneratorCommandID id) const;
  xiiAnimPoseGeneratorCommand&       GetCommand(xiiAnimPoseGeneratorCommandID id);

  void UpdatePose(bool bRequestExternalPoseGeneration);

  xiiArrayPtr<xiiMat4> GetCurrentPose() const { return m_OutputPose; }

  void                          SetFinalCommand(xiiAnimPoseGeneratorCommandID cmdId) { m_FinalCommand = cmdId; }
  xiiAnimPoseGeneratorCommandID GetFinalCommand() const { return m_FinalCommand; }

private:
  void Validate() const;

  void Execute(xiiAnimPoseGeneratorCommand& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandSampleTrack& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandLocalToModelPose& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandSampleEventTrack& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandAimIK& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandTwoBoneIK& cmd);
  void SampleEventTrack(const xiiAnimationClipResource* pResource, xiiAnimPoseEventTrackSampleMode mode, float fPrevPos, float fCurPos);

  xiiArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(xiiAnimPoseGeneratorLocalPoseID id);
  xiiArrayPtr<xiiMat4>                 AcquireModelPoseTransforms(xiiAnimPoseGeneratorModelPoseID id);

  xiiGameObject*             m_pTargetGameObject = nullptr;
  const xiiSkeletonResource* m_pSkeleton         = nullptr;

  xiiArrayPtr<xiiMat4> m_OutputPose;

  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  xiiAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  xiiAnimPoseGeneratorCommandID m_FinalCommand = 0;

  xiiHybridArray<xiiArrayPtr<ozz::math::SoaTransform>, 8>                 m_UsedLocalTransforms;
  xiiHybridArray<xiiDynamicArray<xiiMat4, xiiAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleTrack, 4>      m_CommandsSampleTrack;
  xiiHybridArray<xiiAnimPoseGeneratorCommandRestPose, 1>         m_CommandsRestPose;
  xiiHybridArray<xiiAnimPoseGeneratorCommandCombinePoses, 1>     m_CommandsCombinePoses;
  xiiHybridArray<xiiAnimPoseGeneratorCommandLocalToModelPose, 1> m_CommandsLocalToModelPose;
  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleEventTrack, 2> m_CommandsSampleEventTrack;
  xiiHybridArray<xiiAnimPoseGeneratorCommandAimIK, 2>            m_CommandsAimIK;
  xiiHybridArray<xiiAnimPoseGeneratorCommandTwoBoneIK, 2>        m_CommandsTwoBoneIK;

  xiiArrayMap<xiiUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
