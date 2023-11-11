#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiLogicAndAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogicAndAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicAndAnimNode

public:
  xiiLogicAndAnimNode();
  ~xiiLogicAndAnimNode();

private:
  xiiUInt8                                    m_uiBoolCount = 2; // [ property ]
  xiiHybridArray<xiiAnimGraphBoolInputPin, 2> m_InBool;          // [ property ]
  xiiAnimGraphBoolOutputPin                   m_OutIsTrue;       // [ property ]
  xiiAnimGraphBoolOutputPin                   m_OutIsFalse;      // [ property ]
};

class XII_RENDERERCORE_DLL xiiLogicEventAndAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiLogicEventAndAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicEventAndAnimNode

public:
  xiiLogicEventAndAnimNode();
  ~xiiLogicEventAndAnimNode();

private:
  xiiAnimGraphTriggerInputPin  m_InActivate;     // [ property ]
  xiiAnimGraphBoolInputPin     m_InBool;         // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnActivated; // [ property ]
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

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicOrAnimNode

public:
  xiiLogicOrAnimNode();
  ~xiiLogicOrAnimNode();

private:
  xiiUInt8                                    m_uiBoolCount = 2; // [ property ]
  xiiHybridArray<xiiAnimGraphBoolInputPin, 2> m_InBool;          // [ property ]
  xiiAnimGraphBoolOutputPin                   m_OutIsTrue;       // [ property ]
  xiiAnimGraphBoolOutputPin                   m_OutIsFalse;      // [ property ]
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

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiLogicNotAnimNode

public:
  xiiLogicNotAnimNode();
  ~xiiLogicNotAnimNode();

private:
  xiiAnimGraphBoolInputPin  m_InBool;  // [ property ]
  xiiAnimGraphBoolOutputPin m_OutBool; // [ property ]
};
