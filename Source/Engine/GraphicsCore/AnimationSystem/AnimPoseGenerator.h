#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

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
  ModelPoseToOutput,
  SampleEventTrack,
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
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommand
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
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandRestPose final : public xiiAnimPoseGeneratorCommand
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
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandSampleTrack final : public xiiAnimPoseGeneratorCommand
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
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandCombinePoses final : public xiiAnimPoseGeneratorCommand
{
  xiiHybridArray<float, 4>                                    m_InputWeights;
  xiiHybridArray<xiiArrayPtr<const ozz::math::SimdFloat4>, 4> m_InputBoneWeights;

private:
  friend class xiiAnimPoseGenerator;

  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseOutput = xiiInvalidIndex;
};

/// \brief Accepts a single input in local space and converts it to model space.
///
/// The input command must be of type
/// * xiiAnimPoseGeneratorCommandSampleTrack
/// * xiiAnimPoseGeneratorCommandCombinePoses
/// * xiiAnimPoseGeneratorCommandRestPose
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandLocalToModelPose final : public xiiAnimPoseGeneratorCommand
{
  xiiGameObject* m_pSendLocalPoseMsgTo = nullptr;

private:
  friend class xiiAnimPoseGenerator;

  xiiAnimPoseGeneratorModelPoseID m_ModelPoseOutput = xiiInvalidIndex;
};

/// \brief Accepts a single input command that outputs a model space pose and forwards it to the xiiGameObject for which the pose is generated.
///
/// The input command must be of type
/// * xiiAnimPoseGeneratorCommandLocalToModelPose
///
/// Every graph should have exactly one of these nodes. Commands that are not (indirectly) connected to an
/// output node will not be evaluated and won't have any effect.
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandModelPoseToOutput final : public xiiAnimPoseGeneratorCommand
{
};

/// \brief Samples the event track of an animation clip but doesn't generate an animation pose.
///
/// Commands of this type can be added as inputs to commands of type
/// * xiiAnimPoseGeneratorCommandSampleTrack
/// * xiiAnimPoseGeneratorCommandSampleEventTrack
///
/// They are used to sample event tracks only.
struct XII_RENDERERCORE_DLL xiiAnimPoseGeneratorCommandSampleEventTrack final : public xiiAnimPoseGeneratorCommand
{
  xiiAnimationClipResourceHandle m_hAnimationClip;
  float                          m_fNormalizedSamplePos;
  float                          m_fPreviousNormalizedSamplePos;

  xiiAnimPoseEventTrackSampleMode m_EventSampling = xiiAnimPoseEventTrackSampleMode::None;

private:
  friend class xiiAnimPoseGenerator;

  xiiUInt32 m_uiUniqueID = 0;
};

class XII_RENDERERCORE_DLL xiiAnimPoseGenerator final
{
public:
  xiiAnimPoseGenerator();
  ~xiiAnimPoseGenerator();

  void Reset(const xiiSkeletonResource* pSkeleton);

  xiiAnimPoseGeneratorCommandSampleTrack&       AllocCommandSampleTrack(xiiUInt32 uiDeterministicID);
  xiiAnimPoseGeneratorCommandRestPose&          AllocCommandRestPose();
  xiiAnimPoseGeneratorCommandCombinePoses&      AllocCommandCombinePoses();
  xiiAnimPoseGeneratorCommandLocalToModelPose&  AllocCommandLocalToModelPose();
  xiiAnimPoseGeneratorCommandModelPoseToOutput& AllocCommandModelPoseToOutput();
  xiiAnimPoseGeneratorCommandSampleEventTrack&  AllocCommandSampleEventTrack();

  const xiiAnimPoseGeneratorCommand& GetCommand(xiiAnimPoseGeneratorCommandID id) const;
  xiiAnimPoseGeneratorCommand&       GetCommand(xiiAnimPoseGeneratorCommandID id);

  xiiArrayPtr<xiiMat4> GeneratePose(const xiiGameObject* pSendAnimationEventsTo);

private:
  void Validate() const;

  void Execute(xiiAnimPoseGeneratorCommand& cmd, const xiiGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandSampleTrack& cmd, const xiiGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandRestPose& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandCombinePoses& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandLocalToModelPose& cmd, const xiiGameObject* pSendAnimationEventsTo);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandModelPoseToOutput& cmd);
  void ExecuteCmd(xiiAnimPoseGeneratorCommandSampleEventTrack& cmd, const xiiGameObject* pSendAnimationEventsTo);
  void SampleEventTrack(const xiiAnimationClipResource* pResource, xiiAnimPoseEventTrackSampleMode mode, const xiiGameObject* pSendAnimationEventsTo, float fPrevPos, float fCurPos);

  xiiArrayPtr<ozz::math::SoaTransform> AcquireLocalPoseTransforms(xiiAnimPoseGeneratorLocalPoseID id);
  xiiArrayPtr<xiiMat4>                 AcquireModelPoseTransforms(xiiAnimPoseGeneratorModelPoseID id);

  const xiiSkeletonResource* m_pSkeleton = nullptr;

  xiiAnimPoseGeneratorLocalPoseID m_LocalPoseCounter = 0;
  xiiAnimPoseGeneratorModelPoseID m_ModelPoseCounter = 0;

  xiiArrayPtr<xiiMat4> m_OutputPose;

  xiiHybridArray<xiiArrayPtr<ozz::math::SoaTransform>, 8>                 m_UsedLocalTransforms;
  xiiHybridArray<xiiDynamicArray<xiiMat4, xiiAlignedAllocatorWrapper>, 2> m_UsedModelTransforms;

  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleTrack, 4>       m_CommandsSampleTrack;
  xiiHybridArray<xiiAnimPoseGeneratorCommandRestPose, 1>          m_CommandsRestPose;
  xiiHybridArray<xiiAnimPoseGeneratorCommandCombinePoses, 1>      m_CommandsCombinePoses;
  xiiHybridArray<xiiAnimPoseGeneratorCommandLocalToModelPose, 1>  m_CommandsLocalToModelPose;
  xiiHybridArray<xiiAnimPoseGeneratorCommandModelPoseToOutput, 1> m_CommandsModelPoseToOutput;
  xiiHybridArray<xiiAnimPoseGeneratorCommandSampleEventTrack, 2>  m_CommandsSampleEventTrack;

  xiiArrayMap<xiiUInt32, ozz::animation::SamplingJob::Context*> m_SamplingCaches;
};
