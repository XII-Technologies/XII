#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiLogAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogAnimNode

protected:
  xiiString                                     m_sText;             // [ property ]
  xiiAnimGraphTriggerInputPin                   m_InActivate;        // [ property ]
  xiiUInt8                                      m_uiNumberCount = 1; // [ property ]
  xiiHybridArray<xiiAnimGraphNumberInputPin, 2> m_InNumbers;         // [ property ]
};

class XII_GRAPHICSCORE_DLL xiiLogInfoAnimNode : public xiiLogAnimNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogInfoAnimNode, xiiLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiLogAnimNode

protected:
  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
};

class XII_GRAPHICSCORE_DLL xiiLogErrorAnimNode : public xiiLogAnimNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogErrorAnimNode, xiiLogAnimNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiLogAnimNode

protected:
  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
};
