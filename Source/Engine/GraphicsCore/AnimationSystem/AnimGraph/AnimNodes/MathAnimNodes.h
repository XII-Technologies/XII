#pragma once

#include <Foundation/CodeUtils/MathExpression.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiMathExpressionAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMathExpressionAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicAndAnimNode

public:
  xiiMathExpressionAnimNode();
  ~xiiMathExpressionAnimNode();

  void      SetExpression(xiiString sExpr);
  xiiString GetExpression() const;

private:
  xiiAnimGraphNumberInputPin  m_ValueAPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueBPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueCPin; // [ property ]
  xiiAnimGraphNumberInputPin  m_ValueDPin; // [ property ]
  xiiAnimGraphNumberOutputPin m_ResultPin; // [ property ]

  xiiString m_sExpression;

  struct InstanceData
  {
    xiiMathExpression m_mExpression;
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiCompareNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCompareNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCompareNumberAnimNode

public:
  double                         m_fReferenceValue = 0.0f; // [ property ]
  xiiEnum<xiiComparisonOperator> m_Comparison;             // [ property ]

private:
  xiiAnimGraphNumberInputPin m_InNumber;    // [ property ]
  xiiAnimGraphNumberInputPin m_InReference; // [ property ]
  xiiAnimGraphBoolOutputPin  m_OutIsTrue;   // [ property ]
  xiiAnimGraphBoolOutputPin  m_OutIsFalse;  // [ property ]
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiBoolToNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoolToNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBoolToNumberAnimNode

public:
  xiiBoolToNumberAnimNode();
  ~xiiBoolToNumberAnimNode();

  double m_fFalseValue = 0.0f;
  double m_fTrueValue  = 1.0f;

private:
  xiiAnimGraphBoolInputPin    m_InValue;   // [ property ]
  xiiAnimGraphNumberOutputPin m_OutNumber; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiBoolToTriggerAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiBoolToTriggerAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiBoolToNumberAnimNode

public:
  xiiBoolToTriggerAnimNode();
  ~xiiBoolToTriggerAnimNode();

private:
  xiiAnimGraphBoolInputPin     m_InValue;    // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnTrue;  // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnFalse; // [ property ]

  struct InstanceData
  {
    xiiInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};
