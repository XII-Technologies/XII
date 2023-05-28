#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Utilities/PipelineBarrierDiligent.h>

bool xiiPipelineBarrierDiligent::IsBarrierModified() const
{
  return m_bTransitionStatesRequested;
}

void xiiPipelineBarrierDiligent::FlushBarriers()
{
  for (auto& pBarrier : m_RequestedBarriers)
  {
    pBarrier.Value().m_pContext->TransitionResourceStates(1u, &pBarrier.Value().m_DefaultStateTransition);
  }

  m_RequestedBarriers.Clear();
  m_RequestedBarriers.Compact();

  m_bTransitionStatesRequested = false;
}

void xiiPipelineBarrierDiligent::EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::IBuffer* pBuffer, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, Diligent::IDeviceObject* pResourceBefore)
{
  XII_ASSERT_DEV(pContext != nullptr, "pContext cannot be nullptr");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer cannot be nullptr");
  XII_ASSERT_DEV(transitionState != Diligent::RESOURCE_STATE_UNDEFINED, "The transition state cannot be undefined");

  // Early exit
  if (pBuffer->GetState() == transitionState)
  {
    return;
  }

  Diligent::StateTransitionDesc transitionDesc;
  transitionDesc.pResource      = pBuffer;
  transitionDesc.OldState       = pBuffer->GetState();
  transitionDesc.NewState       = transitionState;
  transitionDesc.TransitionType = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionDesc.Flags          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  StateTransitionInfo transitionInfo;
  if (!m_RequestedBarriers.TryGetValue(transitionDesc, transitionInfo))
  {
    transitionInfo.m_pContext                        = pContext;
    transitionInfo.m_DefaultStateTransition          = transitionDesc;
    transitionInfo.m_DefaultStateTransition.NewState = defaultState;

    pContext->TransitionResourceStates(1u, &transitionDesc);

    m_RequestedBarriers.Insert(transitionDesc, transitionInfo);
  }
  else
  {
    XII_ASSERT_DEV(transitionInfo.m_DefaultStateTransition.NewState == transitionState, "Expected the NewState to match the Transition State");

    transitionInfo.m_pContext->TransitionResourceStates(1u, &transitionDesc);
  }

  m_bTransitionStatesRequested = true;
}

void xiiPipelineBarrierDiligent::EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::ITexture* pTexture, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, Diligent::IDeviceObject* pResourceBefore)
{
  XII_ASSERT_DEV(pContext != nullptr, "pContext cannot be nullptr");
  XII_ASSERT_DEV(pTexture != nullptr, "pTexture cannot be nullptr");
  XII_ASSERT_DEV(transitionState != Diligent::RESOURCE_STATE_UNDEFINED, "The transition state cannot be undefined");

  // Early exit
  if (pTexture->GetState() == transitionState)
  {
    return;
  }

  Diligent::StateTransitionDesc transitionDesc;
  transitionDesc.pResource      = pTexture;
  transitionDesc.OldState       = pTexture->GetState();
  transitionDesc.NewState       = transitionState;
  transitionDesc.TransitionType = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionDesc.Flags          = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  StateTransitionInfo transitionInfo;
  if (!m_RequestedBarriers.TryGetValue(transitionDesc, transitionInfo))
  {
    transitionInfo.m_pContext                        = pContext;
    transitionInfo.m_DefaultStateTransition          = transitionDesc;
    transitionInfo.m_DefaultStateTransition.NewState = defaultState;

    pContext->TransitionResourceStates(1u, &transitionDesc);

    m_RequestedBarriers.Insert(transitionDesc, transitionInfo);
  }
  else
  {
    XII_ASSERT_DEV(transitionInfo.m_DefaultStateTransition.NewState == transitionState, "Expected the NewState to match the Transition State");

    transitionInfo.m_pContext->TransitionResourceStates(1u, &transitionDesc);
  }

  m_bTransitionStatesRequested = true;
}

xiiUInt32 xiiPipelineBarrierDiligent::PipelineBarrierHash::Hash(const Diligent::StateTransitionDesc& stateTransitionDesc)
{
  xiiHashStreamWriter32 writer;
  writer << stateTransitionDesc.pResource;

  return writer.GetHashValue();
}

bool xiiPipelineBarrierDiligent::PipelineBarrierHash::Equal(const Diligent::StateTransitionDesc& a, const Diligent::StateTransitionDesc& b)
{
  return a.pResource == b.pResource;
}
