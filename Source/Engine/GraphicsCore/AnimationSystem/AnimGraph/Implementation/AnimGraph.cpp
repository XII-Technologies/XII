#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>

xiiAnimGraph::xiiAnimGraph()
{
  Clear();
}

xiiAnimGraph::~xiiAnimGraph() = default;

void xiiAnimGraph::Clear()
{
  xiiMemoryUtils::ZeroFillArray(m_uiInputPinCounts);
  xiiMemoryUtils::ZeroFillArray(m_uiPinInstanceDataOffset);
  m_From.Clear();
  m_Nodes.Clear();
  m_bPreparedForUse = true;
  m_InstanceDataAllocator.ClearDescs();

  for (auto& r : m_OutputPinToInputPinMapping)
  {
    r.Clear();
  }
}

xiiAnimGraphNode* xiiAnimGraph::AddNode(xiiUniquePtr<xiiAnimGraphNode>&& pNode)
{
  m_bPreparedForUse = false;

  m_Nodes.PushBack(std::move(pNode));
  return m_Nodes.PeekBack().Borrow();
}

void xiiAnimGraph::AddConnection(const xiiAnimGraphNode* pSrcNode, xiiStringView sSrcPinName, xiiAnimGraphNode* pDstNode, xiiStringView sDstPinName)
{
  // TODO: assert pSrcNode and pDstNode exist

  m_bPreparedForUse = false;
  xiiStringView sIdx;

  xiiAbstractMemberProperty* pPinPropSrc = (xiiAbstractMemberProperty*)pSrcNode->GetDynamicRTTI()->FindPropertyByName(sSrcPinName);

  auto& to         = m_From[pSrcNode].m_To.ExpandAndGetRef();
  to.m_sSrcPinName = sSrcPinName;
  to.m_pDstNode    = pDstNode;
  to.m_sDstPinName = sDstPinName;
  to.m_pSrcPin     = (xiiAnimGraphPin*)pPinPropSrc->GetPropertyPointer(pSrcNode);

  if (const char* szIdx = sDstPinName.FindSubString("["))
  {
    sIdx        = xiiStringView(szIdx + 1, sDstPinName.GetEndPointer() - 1);
    sDstPinName = xiiStringView(sDstPinName.GetStartPointer(), szIdx);

    xiiAbstractArrayProperty*     pPinPropDst = (xiiAbstractArrayProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);
    const xiiDynamicPinAttribute* pDynPinAttr = pPinPropDst->GetAttributeByType<xiiDynamicPinAttribute>();

    const xiiTypedMemberProperty<xiiUInt8>* pPinSizeProp = (const xiiTypedMemberProperty<xiiUInt8>*)pDstNode->GetDynamicRTTI()->FindPropertyByName(pDynPinAttr->GetProperty());
    xiiUInt8                                uiArraySize  = pPinSizeProp->GetValue(pDstNode);
    pPinPropDst->SetCount(pDstNode, uiArraySize);

    xiiUInt32 uiIdx;
    xiiConversionUtils::StringToUInt(sIdx, uiIdx).AssertSuccess();

    to.m_pDstPin = (xiiAnimGraphPin*)pPinPropDst->GetValuePointer(pDstNode, uiIdx);
  }
  else
  {
    xiiAbstractMemberProperty* pPinPropDst = (xiiAbstractMemberProperty*)pDstNode->GetDynamicRTTI()->FindPropertyByName(sDstPinName);

    to.m_pDstPin = (xiiAnimGraphPin*)pPinPropDst->GetPropertyPointer(pDstNode);
  }
}

void xiiAnimGraph::PreparePinMapping()
{
  xiiUInt16 uiOutputPinCounts[xiiAnimGraphPin::Type::ENUM_COUNT];
  xiiMemoryUtils::ZeroFillArray(uiOutputPinCounts);

  for (const auto& consFrom : m_From)
  {
    for (const ConnectionTo& to : consFrom.Value().m_To)
    {
      uiOutputPinCounts[to.m_pSrcPin->GetPinType()]++;
    }
  }

  for (xiiUInt32 i = 0; i < xiiAnimGraphPin::ENUM_COUNT; ++i)
  {
    m_OutputPinToInputPinMapping[i].Clear();
    m_OutputPinToInputPinMapping[i].SetCount(uiOutputPinCounts[i]);
  }
}

void xiiAnimGraph::AssignInputPinIndices()
{
  xiiMemoryUtils::ZeroFillArray(m_uiInputPinCounts);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      // there may be multiple connections to this pin
      // only assign the index the first time we see a connection to this pin
      // otherwise only count up the number of connections

      if (to.m_pDstPin->m_iPinIndex == -1)
      {
        to.m_pDstPin->m_iPinIndex = m_uiInputPinCounts[to.m_pDstPin->GetPinType()]++;
      }

      ++to.m_pDstPin->m_uiNumConnections;
    }
  }
}

void xiiAnimGraph::AssignOutputPinIndices()
{
  xiiInt16 iPinTypeCount[xiiAnimGraphPin::Type::ENUM_COUNT];
  xiiMemoryUtils::ZeroFillArray(iPinTypeCount);

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      const xiiUInt8 pinType = to.m_pSrcPin->GetPinType();

      // there may be multiple connections from this pin
      // only assign the index the first time we see a connection from this pin

      if (to.m_pSrcPin->m_iPinIndex == -1)
      {
        to.m_pSrcPin->m_iPinIndex = iPinTypeCount[pinType]++;
      }

      // store the indices of all the destination pins
      m_OutputPinToInputPinMapping[pinType][to.m_pSrcPin->m_iPinIndex].PushBack(to.m_pDstPin->m_iPinIndex);
    }
  }
}

xiiUInt16 xiiAnimGraph::ComputeNodePriority(const xiiAnimGraphNode* pNode, xiiMap<const xiiAnimGraphNode*, xiiUInt16>& inout_Prios, xiiUInt16& inout_uiOutputPrio) const
{
  auto itPrio = inout_Prios.Find(pNode);
  if (itPrio.IsValid())
  {
    // priority already computed -> return it
    return itPrio.Value();
  }

  const auto itConsFrom = m_From.Find(pNode);

  xiiUInt16 uiOwnPrio = 0xFFFF;

  if (itConsFrom.IsValid())
  {
    // look at all outgoing priorities and take the smallest dst priority - 1
    for (const ConnectionTo& to : itConsFrom.Value().m_To)
    {
      uiOwnPrio = xiiMath::Min<xiiUInt16>(uiOwnPrio, ComputeNodePriority(to.m_pDstNode, inout_Prios, inout_uiOutputPrio) - 1);
    }
  }
  else
  {
    // has no outgoing connections at all -> max priority value
    uiOwnPrio = inout_uiOutputPrio;
    inout_uiOutputPrio -= 64;
  }

  XII_ASSERT_DEBUG(uiOwnPrio != 0xFFFF, "");

  inout_Prios[pNode] = uiOwnPrio;
  return uiOwnPrio;
}

void xiiAnimGraph::SortNodesByPriority()
{
  // this is important so that we can step all nodes in linear order,
  // and have them generate their output such that it is ready before
  // dependent nodes are stepped

  xiiUInt16                                  uiOutputPrio = 0xFFFE;
  xiiMap<const xiiAnimGraphNode*, xiiUInt16> prios;
  for (const auto& pNode : m_Nodes)
  {
    ComputeNodePriority(pNode.Borrow(), prios, uiOutputPrio);
  }

  m_Nodes.Sort([&](const auto& lhs, const auto& rhs) -> bool { return prios[lhs.Borrow()] < prios[rhs.Borrow()]; });
}

void xiiAnimGraph::PrepareForUse()
{
  if (m_bPreparedForUse)
    return;

  m_bPreparedForUse = true;

  for (auto& consFrom : m_From)
  {
    for (ConnectionTo& to : consFrom.Value().m_To)
    {
      to.m_pSrcPin->m_iPinIndex        = -1;
      to.m_pSrcPin->m_uiNumConnections = 0;
      to.m_pDstPin->m_iPinIndex        = -1;
      to.m_pDstPin->m_uiNumConnections = 0;
    }
  }

  SortNodesByPriority();
  PreparePinMapping();
  AssignInputPinIndices();
  AssignOutputPinIndices();

  m_InstanceDataAllocator.ClearDescs();
  for (const auto& pNode : m_Nodes)
  {
    xiiInstanceDataDesc desc;
    if (pNode->GetInstanceDataDesc(desc))
    {
      pNode->m_uiInstanceDataOffset = m_InstanceDataAllocator.AddDesc(desc);
    }
  }

  // EXTEND THIS if a new type is introduced
  {
    xiiInstanceDataDesc desc;
    desc.m_uiTypeAlignment                                    = XII_ALIGNMENT_OF(xiiInt8);
    desc.m_uiTypeSize                                         = sizeof(xiiInt8) * m_uiInputPinCounts[xiiAnimGraphPin::Type::Trigger];
    m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Trigger] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    xiiInstanceDataDesc desc;
    desc.m_uiTypeAlignment                                   = XII_ALIGNMENT_OF(double);
    desc.m_uiTypeSize                                        = sizeof(double) * m_uiInputPinCounts[xiiAnimGraphPin::Type::Number];
    m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Number] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    xiiInstanceDataDesc desc;
    desc.m_uiTypeAlignment                                 = XII_ALIGNMENT_OF(bool);
    desc.m_uiTypeSize                                      = sizeof(bool) * m_uiInputPinCounts[xiiAnimGraphPin::Type::Bool];
    m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Bool] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    xiiInstanceDataDesc desc;
    desc.m_uiTypeAlignment                                        = XII_ALIGNMENT_OF(xiiUInt16);
    desc.m_uiTypeSize                                             = sizeof(xiiUInt16) * m_uiInputPinCounts[xiiAnimGraphPin::Type::BoneWeights];
    m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::BoneWeights] = m_InstanceDataAllocator.AddDesc(desc);
  }
  {
    xiiInstanceDataDesc desc;
    desc.m_uiTypeAlignment                                      = XII_ALIGNMENT_OF(xiiUInt16);
    desc.m_uiTypeSize                                           = sizeof(xiiUInt16) * m_uiInputPinCounts[xiiAnimGraphPin::Type::ModelPose];
    m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::ModelPose] = m_InstanceDataAllocator.AddDesc(desc);
  }
}

xiiResult xiiAnimGraph::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(10);

  const xiiUInt32 uiNumNodes = m_Nodes.GetCount();
  inout_stream << uiNumNodes;

  xiiMap<const xiiAnimGraphNode*, xiiUInt32> nodeToIdx;

  for (xiiUInt32 n = 0; n < m_Nodes.GetCount(); ++n)
  {
    const xiiAnimGraphNode* pNode = m_Nodes[n].Borrow();

    nodeToIdx[pNode] = n;

    inout_stream << pNode->GetDynamicRTTI()->GetTypeName();
    XII_SUCCEED_OR_RETURN(pNode->SerializeNode(inout_stream));
  }

  inout_stream << m_From.GetCount();
  for (auto itFrom : m_From)
  {
    inout_stream << nodeToIdx[itFrom.Key()];

    const auto& toAll = itFrom.Value().m_To;
    inout_stream << toAll.GetCount();

    for (const auto& to : toAll)
    {
      inout_stream << to.m_sSrcPinName;
      inout_stream << nodeToIdx[to.m_pDstNode];
      inout_stream << to.m_sDstPinName;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiAnimGraph::Deserialize(xiiStreamReader& inout_stream)
{
  Clear();

  const xiiTypeVersion version = inout_stream.ReadVersion(10);

  if (version < 10)
    return XII_FAILURE;

  xiiUInt32 uiNumNodes = 0;
  inout_stream >> uiNumNodes;

  xiiDynamicArray<xiiAnimGraphNode*> idxToNode;
  idxToNode.SetCount(uiNumNodes);

  xiiStringBuilder sTypeName;

  for (xiiUInt32 n = 0; n < uiNumNodes; ++n)
  {
    inout_stream >> sTypeName;
    xiiUniquePtr<xiiAnimGraphNode> pNode = xiiRTTI::FindTypeByName(sTypeName)->GetAllocator()->Allocate<xiiAnimGraphNode>();
    XII_SUCCEED_OR_RETURN(pNode->DeserializeNode(inout_stream));

    idxToNode[n] = AddNode(std::move(pNode));
  }

  xiiUInt32 uiNumConnectionsFrom = 0;
  inout_stream >> uiNumConnectionsFrom;

  xiiStringBuilder sPinSrc, sPinDst;

  for (xiiUInt32 cf = 0; cf < uiNumConnectionsFrom; ++cf)
  {
    xiiUInt32 nodeIdx;
    inout_stream >> nodeIdx;
    const xiiAnimGraphNode* ptrNodeFrom = idxToNode[nodeIdx];

    xiiUInt32 uiNumConnectionsTo = 0;
    inout_stream >> uiNumConnectionsTo;

    for (xiiUInt32 ct = 0; ct < uiNumConnectionsTo; ++ct)
    {
      inout_stream >> sPinSrc;

      inout_stream >> nodeIdx;
      xiiAnimGraphNode* ptrNodeTo = idxToNode[nodeIdx];

      inout_stream >> sPinDst;

      AddConnection(ptrNodeFrom, sPinSrc, ptrNodeTo, sPinDst);
    }
  }

  m_bPreparedForUse = false;
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraph);
