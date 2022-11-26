#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiMathExpressionAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMathExpressionAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Initialize(xiiAnimGraph& graph, const xiiSkeletonResource* pSkeleton) override;
  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicAndAnimNode

public:
  xiiMathExpressionAnimNode();
  ~xiiMathExpressionAnimNode();

  void        SetExpression(const char* sz);
  const char* GetExpression() const;

private:
  xiiAnimGraphNumberInputPin  m_ValueAPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueBPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueCPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueDPin; // [ property ]
  xiiAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  xiiMathExpression m_mExpression;
};
