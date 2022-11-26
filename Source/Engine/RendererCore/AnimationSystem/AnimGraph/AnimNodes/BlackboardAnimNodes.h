#pragma once

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_RENDERERCORE_DLL xiiSetBlackboardValueAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSetBlackboardValueAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSetBlackboardValueAnimNode

public:
  void        SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;             // [ property ]

  float m_fOnActivatedValue   = 1.0f;  // [ property ]
  float m_fOnHoldValue        = 1.0f;  // [ property ]
  float m_fOnDeactivatedValue = 0.0f;  // [ property ]
  bool  m_bSetOnActivation    = true;  // [ property ]
  bool  m_bSetOnHold          = false; // [ property ]
  bool  m_bSetOnDeactivation  = false; // [ property ]

private:
  xiiAnimGraphTriggerInputPin m_ActivePin; // [ property ]
  xiiHashedString             m_sBlackboardEntry;
  bool                        m_bLastActiveState = false;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiCheckBlackboardValueAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCheckBlackboardValueAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCheckBlackboardValueAnimNode

public:
  void        SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;             // [ property ]

  float                          m_fReferenceValue = 1.0f; // [ property ]
  xiiEnum<xiiComparisonOperator> m_Comparison;             // [ property ]

private:
  xiiAnimGraphTriggerOutputPin m_ActivePin; // [ property ]

  xiiHashedString m_sBlackboardEntry;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_RENDERERCORE_DLL xiiGetBlackboardNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGetBlackboardNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimGraph& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCheckBlackboardValueAnimNode

public:
  void        SetBlackboardEntry(const char* szFile); // [ property ]
  const char* GetBlackboardEntry() const;             // [ property ]

private:
  xiiAnimGraphNumberOutputPin m_NumberPin; // [ property ]

  xiiHashedString m_sBlackboardEntry;
};
