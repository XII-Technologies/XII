#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAnimGraphNode, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CustomTitle", GetCustomNodeTitle, SetCustomNodeTitle),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiAnimGraphNode::xiiAnimGraphNode()  = default;
xiiAnimGraphNode::~xiiAnimGraphNode() = default;

xiiResult xiiAnimGraphNode::SerializeNode(xiiStreamWriter& stream) const
{
  stream.WriteVersion(1);

  // no need to serialize this, not used at runtime
  // stream << m_CustomNodeTitle;

  return XII_SUCCESS;
}

xiiResult xiiAnimGraphNode::DeserializeNode(xiiStreamReader& stream)
{
  stream.ReadVersion(1);

  // no need to serialize this, not used at runtime
  // stream >> m_CustomNodeTitle;

  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_AnimationSystem_AnimGraph_Implementation_AnimGraphNode);
