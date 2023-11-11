#include <GraphicsCore/GraphicsCorePCH.h>

#include <Core/World/GameObject.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphInstance.h>
#include <GraphicsCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <GraphicsCore/AnimationSystem/SkeletonResource.h>
#include <ozz/animation/runtime/skeleton.h>

xiiAnimGraphInstance::xiiAnimGraphInstance() = default;

xiiAnimGraphInstance::~xiiAnimGraphInstance()
{
  if (m_pAnimGraph)
  {
    m_pAnimGraph->GetInstanceDataAlloator().DestructAndDeallocate(m_InstanceData);
  }
}

void xiiAnimGraphInstance::Configure(const xiiAnimGraph& animGraph)
{
  m_pAnimGraph = &animGraph;

  m_InstanceData = m_pAnimGraph->GetInstanceDataAlloator().AllocateAndConstruct();

  // EXTEND THIS if a new type is introduced
  m_pTriggerInputPinStates    = (xiiInt8*)xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Trigger]);
  m_pNumberInputPinStates     = (double*)xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Number]);
  m_pBoolInputPinStates       = (bool*)xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::Bool]);
  m_pBoneWeightInputPinStates = (xiiUInt16*)xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::BoneWeights]);
  m_pModelPoseInputPinStates  = (xiiUInt16*)xiiInstanceDataAllocator::GetInstanceData(m_InstanceData.GetByteBlobPtr(), m_pAnimGraph->m_uiPinInstanceDataOffset[xiiAnimGraphPin::Type::ModelPose]);

  m_LocalPoseInputPinStates.SetCount(animGraph.m_uiInputPinCounts[xiiAnimGraphPin::Type::LocalPose]);
}

void xiiAnimGraphInstance::Update(xiiAnimController& ref_controller, xiiTime diff, xiiGameObject* pTarget, const xiiSkeletonResource* pSekeltonResource)
{
  // reset all pin states
  {
    // EXTEND THIS if a new type is introduced

    xiiMemoryUtils::ZeroFill(m_pTriggerInputPinStates, m_pAnimGraph->m_uiInputPinCounts[xiiAnimGraphPin::Type::Trigger]);
    xiiMemoryUtils::ZeroFill(m_pNumberInputPinStates, m_pAnimGraph->m_uiInputPinCounts[xiiAnimGraphPin::Type::Number]);
    xiiMemoryUtils::ZeroFill(m_pBoolInputPinStates, m_pAnimGraph->m_uiInputPinCounts[xiiAnimGraphPin::Type::Bool]);
    xiiMemoryUtils::ZeroFill(m_pBoneWeightInputPinStates, m_pAnimGraph->m_uiInputPinCounts[xiiAnimGraphPin::Type::BoneWeights]);
    xiiMemoryUtils::PatternFill(m_pModelPoseInputPinStates, 0xFF, m_pAnimGraph->m_uiInputPinCounts[xiiAnimGraphPin::Type::ModelPose]);

    for (auto& pins : m_LocalPoseInputPinStates)
    {
      pins.Clear();
    }
  }

  for (const auto& pNode : m_pAnimGraph->GetNodes())
  {
    pNode->Step(ref_controller, *this, diff, pSekeltonResource, pTarget);
  }
}
