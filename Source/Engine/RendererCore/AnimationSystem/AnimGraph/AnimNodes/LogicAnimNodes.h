#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiLogicAndAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogicAndAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicAndAnimNode

public:
  xiiLogicAndAnimNode();
  ~xiiLogicAndAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  xiiAnimGraphTriggerInputPin  m_ActivePin; // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiLogicOrAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogicOrAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicOrAnimNode

public:
  xiiLogicOrAnimNode();
  ~xiiLogicOrAnimNode();

  bool m_bNegateResult = false; // [ property ]

private:
  xiiAnimGraphTriggerInputPin  m_ActivePin; // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiLogicNotAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogicNotAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicNotAnimNode

public:
  xiiLogicNotAnimNode();
  ~xiiLogicNotAnimNode();

private:
  xiiAnimGraphTriggerInputPin  m_ActivePin; // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutputPin; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiCompareNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCompareNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCompareNumberAnimNode

public:
  float                          m_fReferenceValue = 1.0f; // [ property ]
  xiiEnum<xiiComparisonOperator> m_Comparison;             // [ property ]

private:
  xiiAnimGraphTriggerOutputPin m_ActivePin; // [ property ]
  xiiAnimGraphNumberInputPin   m_NumberPin; // [ property ]
};
