#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>

#include <Foundation/Types/RefCounted.h>
#include <ozz/base/maths/soa_transform.h>

class xiiAnimGraphInstance;
class xiiAnimController;
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
    Bool,
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

  virtual xiiAnimGraphPin::Type GetPinType() const = 0;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

protected:
  friend class xiiAnimGraph;

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
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Trigger; }

  bool IsTriggered(xiiAnimGraphInstance& ref_graph) const;
  bool AreAllTriggered(xiiAnimGraphInstance& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphTriggerOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphTriggerOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Trigger; }

  /// \brief Sets this output pin to the triggered state for this frame.
  ///
  /// All pin states are reset before every graph update, so this only needs to be called
  /// when a pin should be set to the triggered state, but then it must be called every frame.
  void SetTriggered(xiiAnimGraphInstance& ref_graph) const;
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphNumberInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphNumberInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Number; }

  double GetNumber(xiiAnimGraphInstance& ref_graph, double fFallback = 0.0) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphNumberOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphNumberOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Number; }

  void SetNumber(xiiAnimGraphInstance& ref_graph, double value) const;
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphBoolInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoolInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Bool; }

  bool GetBool(xiiAnimGraphInstance& ref_graph, bool bFallback = false) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphBoolOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoolOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::Bool; }

  void SetBool(xiiAnimGraphInstance& ref_graph, bool bValue) const;
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphBoneWeightsInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoneWeightsInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::BoneWeights; }

  xiiAnimGraphPinDataBoneWeights* GetWeights(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphBoneWeightsOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphBoneWeightsOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::BoneWeights; }

  void SetWeights(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataBoneWeights* pWeights) const;
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::LocalPose; }

  xiiAnimGraphPinDataLocalTransforms* GetPose(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseMultiInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseMultiInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::LocalPose; }

  void GetPoses(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiDynamicArray<xiiAnimGraphPinDataLocalTransforms*>& out_poses) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphLocalPoseOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphLocalPoseOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::LocalPose; }

  void SetPose(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataLocalTransforms* pPose) const;
};

//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiAnimGraphModelPoseInputPin : public xiiAnimGraphInputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphModelPoseInputPin, xiiAnimGraphInputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::ModelPose; }

  xiiAnimGraphPinDataModelTransforms* GetPose(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph) const;
};

class XII_RENDERERCORE_DLL xiiAnimGraphModelPoseOutputPin : public xiiAnimGraphOutputPin
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAnimGraphModelPoseOutputPin, xiiAnimGraphOutputPin);

public:
  virtual xiiAnimGraphPin::Type GetPinType() const override { return xiiAnimGraphPin::ModelPose; }

  void SetPose(xiiAnimGraphInstance& ref_graph, xiiAnimGraphPinDataModelTransforms* pPose) const;
};
