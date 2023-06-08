#pragma once

#include <RendererCore/RendererCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class xiiAnimGraph;
class xiiStreamWriter;
class xiiStreamReader;
struct xiiAnimGraphPinDataBoneWeights;
struct xiiAnimGraphPinDataLocalTransforms;
struct xiiAnimGraphPinDataModelTransforms;

struct xiiAnimGraphSharedBoneWeights : public xiiRefCounted
{
  xiiDynamicArray<ozz::math::SimdFloat4, xiiAlignedAllocatorWrapper> m_Weights;
};

using xiiAnimPoseGeneratorLocalPoseID = xiiUInt32;
using xiiAnimPoseGeneratorModelPoseID = xiiUInt32;
using xiiAnimPoseGeneratorCommandID   = xiiUInt32;

class XII_RENDERERCORE_DLL xiiAnimGraphPin : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphPin, xiiReflectedClass);

public:
  enum Type : xiiUInt8
  {
    Invalid,
    Trigger,
    Number,
    BoneWeights,
    LocalPose,
    ModelPose,
    // EXTEND THIS if a new type is introduced

    ENUM_COUNT
  };

  bool IsConnected() const
  {
    return m_iPinIndex != -1;
  }

  xiiResult Serialize(xiiStreamWriter& ref_stream) const;
  xiiResult Deserialize(xiiStreamReader& ref_stream);

protected:
  xiiInt16 m_iPinIndex        = -1;
  xiiUInt8 m_uiNumConnections = 0;
};

class XII_RENDERERCORE_DLL xiiAnimGraphInputPin : public xiiAnimGraphPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphInputPin, xiiAnimGraphPin);

public:
};

class XII_RENDERERCORE_DLL xiiAnimGraphOutputPin : public xiiAnimGraphPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphOutputPin, xiiAnimGraphPin);

public:
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphTriggerInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphTriggerInputPin, xiiAnimGraphInputPin);

public:
  bool IsTriggered(xiiAnimGraph& ref_graph) const;
  bool AreAllTriggered(xiiAnimGraph& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphTriggerOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphTriggerOutputPin, xiiAnimGraphOutputPin);

public:
  /// \brief Sets this output pin to the triggered or untriggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(xiiAnimGraph& ref_graph, bool bTriggered);
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphNumberInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphNumberInputPin, xiiAnimGraphInputPin);

public:
  double GetNumber(xiiAnimGraph& ref_graph, double fFallback = 0.0) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphNumberOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphNumberOutputPin, xiiAnimGraphOutputPin);

public:
  void SetNumber(xiiAnimGraph& ref_graph, double value);
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphBoneWeightsInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoneWeightsInputPin, xiiAnimGraphInputPin);

public:
  xiiAnimGraphPinDataBoneWeights* GetWeights(xiiAnimGraph& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphBoneWeightsOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoneWeightsOutputPin, xiiAnimGraphOutputPin);

public:
  void SetWeights(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataBoneWeights* pWeights);
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseInputPin, xiiAnimGraphInputPin);

public:
  xiiAnimGraphPinDataLocalTransforms* GetPose(xiiAnimGraph& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseMultiInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseMultiInputPin, xiiAnimGraphInputPin);

public:
  void GetPoses(xiiAnimGraph& ref_graph, xiiDynamicArray<xiiAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseOutputPin, xiiAnimGraphOutputPin);

public:
  void SetPose(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataLocalTransforms* pPose);
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphModelPoseInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphModelPoseInputPin, xiiAnimGraphInputPin);

public:
  xiiAnimGraphPinDataModelTransforms* GetPose(xiiAnimGraph& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphModelPoseOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphModelPoseOutputPin, xiiAnimGraphOutputPin);

public:
  void SetPose(xiiAnimGraph& ref_graph, xiiAnimGraphPinDataModelTransforms* pPose);
};
