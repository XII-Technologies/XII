#pragma once

#include <Core/ResourceManager/ResourceHandle.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphNode.h>
#include <GraphicsCore/AnimationSystem/AnimationClipResource.h>

class XII_RENDERERCORE_DLL xiiRestPoseAnimNode : public xiiAnimGraphNode
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRestPoseAnimNode, xiiAnimGraphNode);

  //////////////////////////////////////////////////////////////////////////
  // xiiAnimGraphNode

protected:
  virtual xiiResult SerializeNode(xiiStreamWriter& stream) const override;
  virtual xiiResult DeserializeNode(xiiStreamReader& stream) override;

  virtual void Step(xiiAnimController& ref_controller, xiiAnimGraphInstance& ref_graph, xiiTime tDiff, const xiiSkeletonResource* pSkeleton, xiiGameObject* pTarget) const override;

  //////////////////////////////////////////////////////////////////////////
  // xiiRestPoseAnimNode

private:
  xiiAnimGraphLocalPoseOutputPin m_OutPose; // [ property ]
};
