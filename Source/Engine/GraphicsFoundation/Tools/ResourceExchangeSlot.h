/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsFoundation/GraphicsFoundationDLL.h>

/// Tracks one reusable producer allocation independently of its type and consumer.
/// Fence values belong to the same producer queue throughout a slot's lifetime.
/// The owner holds the allocation through GPU completion and keeps its export handles
/// valid until the consumer acknowledges the publication. Acknowledgment alone never
/// authorizes external reuse. Retire with the producer's acquire submission fence.
class xiiGALResourceExchangeSlot
{
public:
  [[nodiscard]] bool IsAvailable(xiiUInt64 uiCompletedFence) const
  {
    return m_State == State::Available || (m_State == State::Retiring && m_uiFence <= uiCompletedFence);
  }

  void BeginProduction(xiiUInt64 uiCompletedFence)
  {
    XII_ASSERT_DEV(IsAvailable(uiCompletedFence), "The resource slot is still in use.");
    m_State = State::Producing;
  }

  /// Cancels a reservation before issuing any GPU commands for the publication.
  void CancelProduction()
  {
    XII_ASSERT_DEV(m_State == State::Producing, "Only a producer reservation can be cancelled.");
    m_State = State::Available;
  }

  void QueuePublication(xiiUInt64 uiPublicationID, xiiUInt64 uiFence)
  {
    XII_ASSERT_DEV(m_State == State::Producing, "Reserve the resource slot before submitting a publication.");
    m_uiPublicationID = uiPublicationID;
    m_uiFence         = uiFence;
    m_State           = State::PendingPublication;
  }

  [[nodiscard]] bool HasPendingPublication() const { return m_State == State::PendingPublication; }
  [[nodiscard]] bool IsReadyToPublish(xiiUInt64 uiCompletedFence) const { return HasPendingPublication() && m_uiFence <= uiCompletedFence; }

  void Publish(xiiUInt64 uiCompletedFence)
  {
    XII_ASSERT_DEV(IsReadyToPublish(uiCompletedFence), "Wait for the producer's GPU work to complete before publishing.");
    m_State = State::Published;
  }

  [[nodiscard]] bool MatchesPublication(xiiUInt64 uiPublicationID) const
  {
    return m_State == State::Published && m_uiPublicationID == uiPublicationID;
  }

  /// Retires a frame after GPU-visible ownership was returned, or local work was discarded.
  /// To abandon an exported allocation after a failed import, retire with the original
  /// release fence and replace the allocation before the next production.
  void Retire(xiiUInt64 uiFence)
  {
    XII_ASSERT_DEV(m_State != State::Available && m_State != State::Retiring, "The resource slot has no work to retire.");
    m_uiFence = uiFence;
    m_State   = State::Retiring;
  }

  [[nodiscard]] xiiUInt64 GetFence() const { return m_uiFence; }

private:
  enum class State : xiiUInt8
  {
    Available,
    Producing,
    PendingPublication,
    Published,
    Retiring,
  };

  xiiUInt64 m_uiPublicationID = 0;
  xiiUInt64 m_uiFence         = 0;
  State     m_State           = State::Available;
};
