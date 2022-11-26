#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiLogAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogAnimNode

public:
  xiiString m_sText;

private:
  xiiAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  xiiAnimGraphTriggerInputPin m_Input0;    // [ property ]
  xiiAnimGraphTriggerInputPin m_Input1;    // [ property ]
  xiiAnimGraphNumberInputPin  m_Input2;    // [ property ]
  xiiAnimGraphNumberInputPin  m_Input3;    // [ property ]
};
