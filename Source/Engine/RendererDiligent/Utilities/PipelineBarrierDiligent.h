#pragma once

#include <RendererDiligent/RendererDiligentDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Types/Bitflags.h>

/// \brief Handles resource state transitions, to ensure that resources are always at their expected states.
class XII_RENDERERDILIGENT_DLL xiiPipelineBarrierDiligent
{
public:
  /// \brief Returns true if the pipeline barrier has been modified.
  bool IsBarrierModified() const;

  /// \brief Restores resources to their default state.
  void FlushBarriers();

  void EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::IBuffer* pBuffer, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, bool bIsDeferred = false, bool bIsExclusive = false, Diligent::IDeviceObject* pResourceBefore = nullptr);
  void EnsureResourceState(Diligent::IDeviceContext* pContext, Diligent::ITexture* pTexture, Diligent::RESOURCE_STATE transitionState, Diligent::RESOURCE_STATE defaultState, bool bIsDeferred = false, bool bIsExclusive = false, Diligent::IDeviceObject* pResourceBefore = nullptr);

private:
  struct PipelineBarrierHash
  {
    static xiiUInt32 Hash(const Diligent::StateTransitionDesc& stateTransitionDesc);
    static bool      Equal(const Diligent::StateTransitionDesc& a, const Diligent::StateTransitionDesc& b);
  };

  struct StateTransitionInfo
  {
    Diligent::IDeviceContext*     m_pContext               = nullptr;
    Diligent::StateTransitionDesc m_DefaultStateTransition = {};
  };

  bool m_bTransitionStatesModified = false;

  // Stores the requested state transition, as well as a default state to be transitioned into when flushing barriers.
  xiiHashTable<Diligent::StateTransitionDesc, StateTransitionInfo, PipelineBarrierHash> m_RequestedBarriers;
};
