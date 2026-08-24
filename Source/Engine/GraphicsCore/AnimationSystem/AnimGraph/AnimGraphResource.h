/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Strings/HashedString.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/GraphicsCoreDLL.h>

using xiiAnimGraphResourceHandle = xiiTypedResourceHandle<class xiiAnimGraphResource>;

struct XII_GRAPHICSCORE_DLL xiiAnimGraphNodeType
{
  using StorageType = xiiUInt8;

  enum Enum : StorageType
  {
    Invalid,
    Output,
    Clip,
    Blend1D,
    Additive,
    LayeredBlend,
    StateMachine,
    TwoBoneIK,
    AimIK,

    ENUM_COUNT,

    Default = Invalid
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAnimGraphNodeType);

struct XII_GRAPHICSCORE_DLL xiiAnimGraphNodeFlags
{
  using StorageType = xiiUInt16;

  enum Enum : StorageType
  {
    None          = 0U,
    Loop          = XII_BIT(0),
    Synchronize   = XII_BIT(1),
    RootMotion    = XII_BIT(2),
    Additive      = XII_BIT(3),
    WriteSkinning = XII_BIT(4),

    Default = None
  };

  struct Bits
  {
    StorageType Loop : 1;
    StorageType Synchronize : 1;
    StorageType RootMotion : 1;
    StorageType Additive : 1;
    StorageType WriteSkinning : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiAnimGraphNodeFlags);
XII_DECLARE_REFLECTABLE_TYPE(XII_GRAPHICSCORE_DLL, xiiAnimGraphNodeFlags);

struct XII_GRAPHICSCORE_DLL xiiAnimGraphNode
{
  xiiHashedString                    m_sName;
  xiiEnum<xiiAnimGraphNodeType>      m_Type  = xiiAnimGraphNodeType::Invalid;
  xiiBitflags<xiiAnimGraphNodeFlags> m_Flags = xiiAnimGraphNodeFlags::Default;

  xiiAnimationClipResourceHandle m_hClip;
  xiiHashedString                m_sParameter;
  float                          m_fPlaybackSpeed = 1.0f;
  float                          m_fWeight        = 1.0f;
  float                          m_fThreshold     = 0.0f;
  xiiUInt16                      m_uiTargetJoint  = xiiMath::MaxValue<xiiUInt16>();

  xiiHybridArray<xiiUInt16, 4> m_Inputs;
  xiiHybridArray<float, 4>     m_InputThresholds;
  xiiHybridArray<float, 4>     m_InputWeights;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimGraphTransition
{
  xiiUInt16       m_uiFromNode = xiiMath::MaxValue<xiiUInt16>();
  xiiUInt16       m_uiToNode   = xiiMath::MaxValue<xiiUInt16>();
  xiiHashedString m_sConditionParameter;
  float           m_fConditionThreshold = 0.5f;
  xiiTime         m_BlendDuration       = xiiTime::MakeFromMilliseconds(150.0);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};

struct XII_GRAPHICSCORE_DLL xiiAnimGraphResourceDescriptor
{
  void Clear();
  void ComputeRuntimeHash();

  xiiUInt16 AddNode(const xiiAnimGraphNode& node);
  xiiUInt16 AddClipNode(xiiStringView sName, const xiiAnimationClipResourceHandle& hClip, float fSpeed = 1.0f);
  xiiUInt16 AddBlend1DNode(xiiStringView sName, xiiStringView sParameter, xiiArrayPtr<const xiiUInt16> inputs, xiiArrayPtr<const float> thresholds);
  xiiUInt16 AddLayeredBlendNode(xiiStringView sName, xiiUInt16 uiBaseNode, xiiUInt16 uiLayerNode, xiiUInt16 uiRootJoint, xiiStringView sWeightParameter = {}, float fWeight = 1.0f);
  xiiUInt16 AddStateMachineNode(xiiStringView sName, xiiArrayPtr<const xiiUInt16> states);
  void      AddTransition(xiiUInt16 uiFromNode, xiiUInt16 uiToNode, xiiStringView sConditionParameter, float fThreshold = 0.5f, xiiTime blendDuration = xiiTime::MakeFromMilliseconds(150.0));
  void      SetOutputNode(xiiUInt16 uiNodeIndex);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiHybridArray<xiiAnimGraphNode, 32>       m_Nodes;
  xiiHybridArray<xiiAnimGraphTransition, 16> m_Transitions;
  xiiUInt16                                  m_uiOutputNode  = xiiMath::MaxValue<xiiUInt16>();
  xiiUInt32                                  m_uiRuntimeHash = 0U;
};

class XII_GRAPHICSCORE_DLL xiiAnimGraphResource final : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiAnimGraphResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiAnimGraphResource, xiiAnimGraphResourceDescriptor);

public:
  xiiAnimGraphResource();
  ~xiiAnimGraphResource();

  const xiiAnimGraphResourceDescriptor& GetDescriptor() const;
  xiiUInt32                             GetRuntimeHash() const;

private:
  virtual xiiResourceLoadDescription UnloadData(Unload whatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  xiiAnimGraphResourceDescriptor m_Descriptor;
};

class XII_GRAPHICSCORE_DLL xiiAnimGraphInstance
{
public:
  void Reset();

  void                              SetGraph(const xiiAnimGraphResourceHandle& hGraph);
  const xiiAnimGraphResourceHandle& GetGraph() const;

  void  SetFloat(xiiStringView sName, float fValue);
  float GetFloat(const xiiTempHashedString& sName, float fFallback = 0.0f) const;

  void SetBool(xiiStringView sName, bool bValue);
  bool GetBool(const xiiTempHashedString& sName, bool bFallback = false) const;

  void Update(const xiiSkeletonResource& skeleton, xiiTime deltaTime, xiiAnimationPose& ref_pose);

private:
  void      EvaluateNode(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiNodeIndex, const xiiAnimGraphNode& node, const xiiSkeletonResource& skeleton, xiiAnimationPose& ref_pose);
  void      EvaluateNodeIndex(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiNodeIndex, const xiiSkeletonResource& skeleton, xiiAnimationPose& ref_pose);
  bool      IsTransitionConditionTrue(const xiiAnimGraphTransition& transition) const;
  xiiUInt16 GetActiveStateNode(const xiiAnimGraphResourceDescriptor& graph, xiiUInt16 uiStateMachineNodeIndex, const xiiAnimGraphNode& stateMachineNode);

private:
  xiiAnimGraphResourceHandle           m_hGraph;
  xiiTime                              m_Time;
  xiiHashTable<xiiHashedString, float> m_Floats;
  xiiHashTable<xiiHashedString, bool>  m_Bools;
  xiiHashTable<xiiUInt16, xiiUInt16>   m_StateMachineStates;
  xiiHashTable<xiiUInt16, xiiUInt16>   m_StateMachinePreviousStates;
  xiiHashTable<xiiUInt16, xiiTime>     m_StateMachineTransitionStarts;
  xiiHashTable<xiiUInt16, xiiTime>     m_StateMachineTransitionDurations;
};
