#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiControllerInputAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiControllerInputAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

private:
  xiiAnimGraphTriggerOutputPin m_ButtonA; // [ property ]
  xiiAnimGraphTriggerOutputPin m_ButtonB; // [ property ]
  xiiAnimGraphTriggerOutputPin m_ButtonX; // [ property ]
  xiiAnimGraphTriggerOutputPin m_ButtonY; // [ property ]

  xiiAnimGraphNumberOutputPin m_LeftStickX;  // [ property ]
  xiiAnimGraphNumberOutputPin m_LeftStickY;  // [ property ]
  xiiAnimGraphNumberOutputPin m_RightStickX; // [ property ]
  xiiAnimGraphNumberOutputPin m_RightStickY; // [ property ]

  xiiAnimGraphNumberOutputPin m_LeftTrigger;  // [ property ]
  xiiAnimGraphNumberOutputPin m_RightTrigger; // [ property ]

  xiiAnimGraphTriggerOutputPin m_LeftShoulder;  // [ property ]
  xiiAnimGraphTriggerOutputPin m_RightShoulder; // [ property ]

  xiiAnimGraphTriggerOutputPin m_PadLeft;  // [ property ]
  xiiAnimGraphTriggerOutputPin m_PadRight; // [ property ]
  xiiAnimGraphTriggerOutputPin m_PadUp;    // [ property ]
  xiiAnimGraphTriggerOutputPin m_PadDown;  // [ property ]
};
