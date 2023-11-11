#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiSendEventAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSendEventAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSendEventAnimNode

public:
  void        SetEventName(const char* szSz) { m_sEventName.Assign(szSz); }
  const char* GetEventName() const { return m_sEventName.GetString(); }

private:
  xiiHashedString             m_sEventName; // [ property ]
  xiiAnimGraphTriggerInputPin m_InActivate; // [ property ]
};
