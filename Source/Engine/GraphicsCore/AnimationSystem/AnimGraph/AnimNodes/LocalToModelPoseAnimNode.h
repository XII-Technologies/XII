#pragma once

#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>

//class XII_GRAPHICSCORE_DLL xiiLocalToModelPoseAnimNode : public xiiAnimGraphNode
//{
//  XII_ADD_DYNAMIC_REFLECTION(xiiLocalToModelPoseAnimNode, xiiAnimGraphNode);
//
//  //////////////////////////////////////////////////////////////////////////
//  // xiiAnimGraphNode
//
//protected:
//  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
//  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;
//
//  virtual void Step(xiiAnimGraphExecutor& executor, xiiAnimGraphInstance& graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;
//
//  //////////////////////////////////////////////////////////////////////////
//  // xiiLocalToModelPoseAnimNode
//
//public:
//  xiiLocalToModelPoseAnimNode();
//  ~xiiLocalToModelPoseAnimNode();
//
//private:
//  xiiAnimGraphLocalPoseInputPin m_LocalPosePin;  // [ property ]
//  xiiAnimGraphModelPoseOutputPin m_ModelPosePin; // [ property ]
//};
