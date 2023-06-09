#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiEventAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEventAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiEventAnimNode

public:
  void        SetEventName(const char* szSz) { m_sEventName.Assign(szSz); }
  const char* GetEventName() const { return m_sEventName.GetString(); }

private:
  xiiHashedString             m_sEventName;
  xiiAnimGraphTriggerInputPin m_ActivePin; // [ property ]
};
