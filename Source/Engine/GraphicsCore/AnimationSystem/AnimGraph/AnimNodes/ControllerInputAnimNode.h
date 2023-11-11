#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiControllerInputAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiControllerInputAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiControllerInputAnimNode

private:
  xiiAnimGraphNumberOutputPin m_OutLeftStickX;  // [ property ]
  xiiAnimGraphNumberOutputPin m_OutLeftStickY;  // [ property ]
  xiiAnimGraphNumberOutputPin m_OutRightStickX; // [ property ]
  xiiAnimGraphNumberOutputPin m_OutRightStickY; // [ property ]

  xiiAnimGraphNumberOutputPin m_OutLeftTrigger;  // [ property ]
  xiiAnimGraphNumberOutputPin m_OutRightTrigger; // [ property ]

  xiiAnimGraphBoolOutputPin m_OutButtonA; // [ property ]
  xiiAnimGraphBoolOutputPin m_OutButtonB; // [ property ]
  xiiAnimGraphBoolOutputPin m_OutButtonX; // [ property ]
  xiiAnimGraphBoolOutputPin m_OutButtonY; // [ property ]

  xiiAnimGraphBoolOutputPin m_OutLeftShoulder;  // [ property ]
  xiiAnimGraphBoolOutputPin m_OutRightShoulder; // [ property ]

  xiiAnimGraphBoolOutputPin m_OutPadLeft;  // [ property ]
  xiiAnimGraphBoolOutputPin m_OutPadRight; // [ property ]
  xiiAnimGraphBoolOutputPin m_OutPadUp;    // [ property ]
  xiiAnimGraphBoolOutputPin m_OutPadDown;  // [ property ]
};
