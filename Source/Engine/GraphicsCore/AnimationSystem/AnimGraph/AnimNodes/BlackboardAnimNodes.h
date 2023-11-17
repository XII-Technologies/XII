#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

class XII_GRAPHICSCORE_DLL xiiSetBlackboardNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSetBlackboardNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSetBlackboardNumberAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

  double m_fNumber = 0.0f; // [ property ]

private:
  xiiAnimGraphTriggerInputPin m_InActivate;       // [ property ]
  xiiAnimGraphNumberInputPin  m_InNumber;         // [ property ]
  xiiHashedString             m_sBlackboardEntry; // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiGetBlackboardNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGetBlackboardNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiGetBlackboardNumberAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

private:
  xiiHashedString             m_sBlackboardEntry; // [ property ]
  xiiAnimGraphNumberOutputPin m_OutNumber;        // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiCompareBlackboardNumberAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCompareBlackboardNumberAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCompareBlackboardNumberAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

  double                         m_fReferenceValue = 0.0; // [ property ]
  xiiEnum<xiiComparisonOperator> m_Comparison;            // [ property ]

private:
  xiiHashedString              m_sBlackboardEntry; // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnTrue;        // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnFalse;       // [ property ]
  xiiAnimGraphBoolOutputPin    m_OutIsTrue;        // [ property ]

  struct InstanceData
  {
    xiiInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiCheckBlackboardBoolAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiCheckBlackboardBoolAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiCheckBlackboardBoolAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

private:
  xiiHashedString              m_sBlackboardEntry; // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnTrue;        // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnFalse;       // [ property ]
  xiiAnimGraphBoolOutputPin    m_OutBool;          // [ property ]

  struct InstanceData
  {
    xiiInt8 m_iIsTrue = -1; // -1 == undefined, 0 == false, 1 == true
  };
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiSetBlackboardBoolAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiSetBlackboardBoolAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiSetBlackboardBoolAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

  bool m_bBool = false; // [ property ]

private:
  xiiHashedString             m_sBlackboardEntry; // [ property ]
  xiiAnimGraphTriggerInputPin m_InActivate;       // [ property ]
  xiiAnimGraphBoolInputPin    m_InBool;           // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiGetBlackboardBoolAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiGetBlackboardBoolAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiGetBlackboardBoolAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

private:
  xiiHashedString           m_sBlackboardEntry; // [ property ]
  xiiAnimGraphBoolOutputPin m_OutBool;          // [ property ]
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class XII_GRAPHICSCORE_DLL xiiOnBlackboardValueChangedAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiOnBlackboardValueChangedAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
  virtual bool GetInstanceDataDesc(xiiInstanceDataDesc& out_desc) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiOnBlackboardValuechangedAnimNode

public:
  void        SetBlackboardEntry(const char* szEntry); // [ property ]
  const char* GetBlackboardEntry() const;              // [ property ]

private:
  xiiHashedString              m_sBlackboardEntry;  // [ property ]
  xiiAnimGraphTriggerOutputPin m_OutOnValueChanged; // [ property ]

  struct InstanceData
  {
    xiiUInt32 m_uiChangeCounter = xiiInvalidIndex;
  };
};
