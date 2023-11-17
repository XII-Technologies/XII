#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiRootRotationAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRootRotationAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRootRotationAnimNode

public:
  xiiRootRotationAnimNode();
  ~xiiRootRotationAnimNode();

private:
  xiiAnimGraphNumberInputPin m_InRotateX; // [ property ]
  xiiAnimGraphNumberInputPin m_InRotateY; // [ property ]
  xiiAnimGraphNumberInputPin m_InRotateZ; // [ property ]
};
