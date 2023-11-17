#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/InstanceDataAllocator.h>
#include <Foundation/Types/UniquePtr.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiAnimGraph
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiAnimGraph);

public:
  xiiAnimGraph();
  ~xiiAnimGraph();

  void Clear();

  xiiAnimGraphNode* AddNode(xiiUniquePtr<xiiAnimGraphNode>&& pNode);
  void              AddConnection(const xiiAnimGraphNode* pSrcNode, xiiStringView sSrcPinName, xiiAnimGraphNode* pDstNode, xiiStringView sDstPinName);

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  const xiiInstanceDataAllocator&                   GetInstanceDataAlloator() const { return m_InstanceDataAllocator; }
  xiiArrayPtr<const xiiUniquePtr<xiiAnimGraphNode>> GetNodes() const { return m_Nodes; }

  void PrepareForUse();

private:
  friend class xiiAnimGraphInstance;

  struct ConnectionTo
  {
    xiiString               m_sSrcPinName;
    const xiiAnimGraphNode* m_pDstNode = nullptr;
    xiiString               m_sDstPinName;
    xiiAnimGraphPin*        m_pSrcPin = nullptr;
    xiiAnimGraphPin*        m_pDstPin = nullptr;
  };

  struct ConnectionsTo
  {
    xiiHybridArray<ConnectionTo, 2> m_To;
  };

  void      SortNodesByPriority();
  void      PreparePinMapping();
  void      AssignInputPinIndices();
  void      AssignOutputPinIndices();
  xiiUInt16 ComputeNodePriority(const xiiAnimGraphNode* pNode, xiiMap<const xiiAnimGraphNode*, xiiUInt16>& inout_Prios, xiiUInt16& inout_uiOutputPrio) const;

  bool                                           m_bPreparedForUse = true;
  xiiUInt32                                      m_uiInputPinCounts[xiiAnimGraphPin::Type::ENUM_COUNT];
  xiiUInt32                                      m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::ENUM_COUNT];
  xiiMap<const xiiAnimGraphNode*, ConnectionsTo> m_From;

  xiiDynamicArray<xiiUniquePtr<xiiAnimGraphNode>> m_Nodes;
  xiiDynamicArray<xiiHybridArray<xiiUInt16, 1>>   m_OutputPinToInputPinMapping[xiiAnimGraphPin::ENUM_COUNT];
  xiiInstanceDataAllocator                        m_InstanceDataAllocator;

  friend class xiiAnimGraphTriggerOutputPin;
  friend class xiiAnimGraphNumberOutputPin;
  friend class xiiAnimGraphBoolOutputPin;
  friend class xiiAnimGraphBoneWeightsOutputPin;
  friend class xiiAnimGraphLocalPoseOutputPin;
  friend class xiiAnimGraphModelPoseOutputPin;
};
