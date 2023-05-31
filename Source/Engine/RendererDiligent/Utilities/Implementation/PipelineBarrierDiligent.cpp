#include <RendererDiligent/RendererDiligentPCH.h>

#include <RendererDiligent/Utilities/PipelineBarrierDiligent.h>

bool xiiPipelineBarrierDiligent::IsBarrierModified() const
{
  return m_bTransitionStatesModified;
}

void xiiPipelineBarrierDiligent::FlushBarriers()
{
  if (!m_bTransitionStatesModified)
    return;

  for (auto& pBarrier : m_RequestedBarriers)
  {
    pBarrier.Value().m_pContext->TransitionResourceStates(1u, &pBarrier.Value().m_DefaultStateTransition);
  }

  m_RequestedBarriers.Clear();
  m_RequestedBarriers.Compact();

  m_bTransitionStatesModified = false;
}

void xiiPipelineBarrierDiligent::EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::IBuffer* pBuffer, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, bool bIsDeferred /* = false */, Diligent::IDeviceObject* pResourceBefore)
{
  XII_ASSERT_DEV(pContext != nullptr, "pContext cannot be nullptr");
  XII_ASSERT_DEV(pBuffer != nullptr, "pBuffer cannot be nullptr");
  XII_ASSERT_DEV(transitionState != Diligent::RESOURCE_STATE_UNDEFINED, "The transition state cannot be undefined");

  // Early exit
  if (pBuffer->GetState() & transitionState)
  {
    return;
  }

  Diligent::StateTransitionDesc transitionDesc;
  transitionDesc.pResource       = pBuffer;
  transitionDesc.pResourceBefore = pResourceBefore;
  transitionDesc.OldState        = pBuffer->GetState();
  transitionDesc.NewState        = transitionState;
  transitionDesc.TransitionType  = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionDesc.Flags           = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  StateTransitionInfo transitionInfo;
  transitionInfo.m_pContext                               = pContext;
  transitionInfo.m_DefaultStateTransition.pResource       = pBuffer;
  transitionInfo.m_DefaultStateTransition.pResourceBefore = pResourceBefore;
  transitionInfo.m_DefaultStateTransition.NewState        = defaultState;
  transitionInfo.m_DefaultStateTransition.OldState        = Diligent::RESOURCE_STATE_UNKNOWN;
  transitionInfo.m_DefaultStateTransition.TransitionType  = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionInfo.m_DefaultStateTransition.Flags           = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  if (!bIsDeferred)
  {
    transitionInfo.m_pContext->TransitionResourceStates(1u, &transitionDesc);
  }

  if (m_RequestedBarriers.Contains(transitionDesc))
  {
    m_RequestedBarriers.Remove(transitionDesc);
  }

  m_RequestedBarriers.Insert(transitionDesc, transitionInfo);

  if (bIsDeferred)
  {
    if ((pBuffer->GetState() & transitionState) == 0u)
    {
      m_bTransitionStatesModified = true;
    }
  }
  else
  {
    if ((pBuffer->GetState() & defaultState) == 0u)
    {
      m_bTransitionStatesModified = true;
    }
  }
}

void xiiPipelineBarrierDiligent::EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::ITexture* pTexture, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, bool bIsDeferred /* = false */, Diligent::IDeviceObject* pResourceBefore)
{
  XII_ASSERT_DEV(pContext != nullptr, "pContext cannot be nullptr");
  XII_ASSERT_DEV(pTexture != nullptr, "pTexture cannot be nullptr");
  XII_ASSERT_DEV(transitionState != Diligent::RESOURCE_STATE_UNDEFINED, "The transition state cannot be undefined");

  // Early exit
  if (pTexture->GetState() & transitionState)
  {
    return;
  }

  Diligent::StateTransitionDesc transitionDesc;
  transitionDesc.pResource       = pTexture;
  transitionDesc.pResourceBefore = pResourceBefore;
  transitionDesc.OldState        = pTexture->GetState();
  transitionDesc.NewState        = transitionState;
  transitionDesc.TransitionType  = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionDesc.Flags           = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  StateTransitionInfo transitionInfo;
  transitionInfo.m_pContext                               = pContext;
  transitionInfo.m_DefaultStateTransition.pResource       = pTexture;
  transitionInfo.m_DefaultStateTransition.pResourceBefore = pResourceBefore;
  transitionInfo.m_DefaultStateTransition.NewState        = defaultState;
  transitionInfo.m_DefaultStateTransition.OldState        = Diligent::RESOURCE_STATE_UNKNOWN;
  transitionInfo.m_DefaultStateTransition.TransitionType  = Diligent::STATE_TRANSITION_TYPE_IMMEDIATE;
  transitionInfo.m_DefaultStateTransition.Flags           = Diligent::STATE_TRANSITION_FLAG_UPDATE_STATE;

  if (!bIsDeferred)
  {
    transitionInfo.m_pContext->TransitionResourceStates(1u, &transitionDesc);
  }

  if (m_RequestedBarriers.Contains(transitionDesc))
  {
    m_RequestedBarriers.Remove(transitionDesc);
  }

  m_RequestedBarriers.Insert(transitionDesc, transitionInfo);

  if (bIsDeferred && (pTexture->GetState() & transitionState) == 0u)
  {
    m_bTransitionStatesModified = true;
  }
  else
  {
    if ((pTexture->GetState() & defaultState) == 0u)
    {
      m_bTransitionStatesModified = true;
    }
  }
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
