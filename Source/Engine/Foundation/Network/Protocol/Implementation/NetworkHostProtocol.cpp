/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Implementation/NetworkHostInternal.h>
#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Threading/Lock.h>
#include <cstring>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <utility>

using namespace xiiNetworkHostInternal;

void WireWriter::U8(xiiUInt8 uiValue) { m_Bytes.PushBack(uiValue); }
void WireWriter::U32(xiiUInt32 uiValue)
{
  for (int iShift = 24; iShift >= 0; iShift -= 8)
  {
    U8(static_cast<xiiUInt8>(uiValue >> iShift));
  }
}
void WireWriter::U64(xiiUInt64 uiValue)
{
  U32(static_cast<xiiUInt32>(uiValue >> 32));
  U32(static_cast<xiiUInt32>(uiValue));
}
void WireWriter::Uuid(xiiUuid value)
{
  xiiUInt64 uiLow = 0, uiHigh = 0;
  value.GetValues(uiLow, uiHigh);
  U64(uiHigh);
  U64(uiLow);
}
void WireWriter::Bytes(xiiArrayPtr<const xiiUInt8> bytes) { m_Bytes.PushBackRange(bytes); }
void WireWriter::Text(xiiStringView sText)
{
  U32(sText.GetElementCount());
  Bytes({reinterpret_cast<const xiiUInt8*>(sText.GetStartPointer()), sText.GetElementCount()});
}
xiiUInt8 WireReader::U8()
{
  const auto bytes = Bytes(1);
  return bytes.IsEmpty() ? 0 : bytes[0];
}
xiiUInt32 WireReader::U32()
{
  xiiUInt32 uiValue = 0;
  for (xiiUInt32 uiIndex = 0; uiIndex < 4; ++uiIndex)
  {
    uiValue = (uiValue << 8) | U8();
  }
  return uiValue;
}
xiiUInt64 WireReader::U64()
{
  const auto uiHigh = U32();
  return (static_cast<xiiUInt64>(uiHigh) << 32) | U32();
}
xiiUuid WireReader::Uuid()
{
  const auto uiHigh = U64();
  const auto uiLow  = U64();
  return xiiUuid(uiLow, uiHigh);
}
xiiArrayPtr<const xiiUInt8> WireReader::Bytes(xiiUInt32 uiCount)
{
  if (!m_bValid || uiCount > m_Bytes.GetCount() - m_uiPosition)
  {
    m_bValid = false;
    return {};
  }
  const auto bytes = m_Bytes.GetSubArray(m_uiPosition, uiCount);
  m_uiPosition += uiCount;
  return bytes;
}
xiiString WireReader::Text(xiiUInt32 uiLimit)
{
  const auto uiCount = U32();
  if (uiCount > uiLimit)
  {
    m_bValid = false;
    return {};
  }
  const auto bytes = Bytes(uiCount);
  if (!m_bValid || (!bytes.IsEmpty() && std::memchr(bytes.GetPtr(), 0, bytes.GetCount()) != nullptr))
  {
    m_bValid = false;
    return {};
  }
  return xiiStringView(reinterpret_cast<const char*>(bytes.GetPtr()), bytes.GetCount());
}

namespace
{
  xiiString CertificateIdentity(const xiiNetworkCertificate& certificate)
  {
    xiiStringBuilder sIdentity;
    for (auto uiByte : certificate.m_uiSha256)
    {
      sIdentity.AppendFormat("{0}", xiiArgU(uiByte, 2, true, 16));
    }
    return sIdentity;
  }
  bool DigestToken(xiiArrayPtr<const xiiUInt8> token, xiiUInt8 (&out_uiHash)[32])
  {
    unsigned int uiBytes = 0;
    return token.GetCount() == 32 && EVP_Digest(token.GetPtr(), token.GetCount(), out_uiHash, &uiBytes, EVP_sha256(), nullptr) == 1 && uiBytes == 32;
  }
  xiiNetworkError RandomSession(xiiUuid& out_session)
  {
    xiiUInt8 uiBytes[16] = {};
    auto     error       = xiiNetworkSecurityInternal::RandomBytes(uiBytes);
    if (error.Failed())
    {
      return error;
    }
    WireReader reader(uiBytes);
    out_session = reader.Uuid();
    return out_session.IsValid() ? xiiNetworkError() : xiiNetworkError(xiiNetworkErrorCode::ResourceExhausted);
  }
} // namespace

void xiiNetworkHostRuntime::Queue(Peer& peer, xiiNetworkFrameType type, const WireWriter& writer, xiiUInt32 uiChannel, xiiUInt64 uiSequence)
{
  if (peer.m_bClosing)
  {
    return;
  }
  if (writer.m_Bytes.GetCount() > xiiNetworkFrame::MaximumControlPayload || peer.m_ControlQueue.GetCount() >= 16)
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
    return;
  }
  Outbound outbound;
  outbound.m_Header.m_Type           = type;
  outbound.m_Header.m_uiChannel      = uiChannel;
  outbound.m_Header.m_uiSequence     = uiSequence;
  outbound.m_Header.m_uiPayloadBytes = writer.m_Bytes.GetCount();
  auto error                         = xiiNetworkFrame::EncodeHeader(outbound.m_Header, outbound.m_uiHeader, outbound.m_uiHeaderBytes);
  if (error.Succeeded())
  {
    error = m_IO.AcquireBuffer(writer.m_Bytes.GetCount(), outbound.m_Payload);
  }
  if (error.Failed())
  {
    Close(peer, error);
    return;
  }
  if (!writer.m_Bytes.IsEmpty())
  {
    std::memcpy(outbound.m_Payload.GetBytes().GetPtr(), writer.m_Bytes.GetData(), writer.m_Bytes.GetCount());
  }
  peer.m_ControlQueue.PushBack(std::move(outbound));
}

void xiiNetworkHostRuntime::SendHello(Peer& peer)
{
  WireWriter writer;
  writer.U32(0x58494933U);
  writer.Uuid(m_Configuration.m_Protocol);
  writer.U32(m_Configuration.m_uiMinimumProtocolVersion);
  writer.U32(m_Configuration.m_uiMaximumProtocolVersion);
  writer.U64(m_Configuration.m_uiSupportedFeatures);
  writer.U64(m_Configuration.m_uiRequiredFeatures);
  writer.U32(m_Capabilities.m_uiDeliveryModes);
  writer.U32(m_Configuration.m_uiMaximumMessageBytes);
  writer.U32(m_Configuration.m_uiMaximumChannelsPerConnection);
  writer.U8(static_cast<xiiUInt8>(m_Configuration.m_Authentication));
  writer.U8(static_cast<xiiUInt8>((peer.m_Options.m_bAllowFreshSession ? 1 : 0) | (m_Configuration.m_bEnableSessionRecovery ? 2 : 0)));
  auto pTicket = peer.m_Options.m_pSessionTicket;
  if (pTicket != nullptr)
  {
    if (!pTicket->IsValid())
    {
      if (!peer.m_Options.m_bAllowFreshSession)
      {
        Close(peer, xiiNetworkError(xiiNetworkErrorCode::SessionExpired));
        return;
      }
      pTicket.Clear();
    }
    else if (!m_Capabilities.m_bEncrypted || pTicket->m_sOrigin != peer.m_Options.m_sServerName || pTicket->m_uiPort != peer.m_Info.m_RemoteEndpoint.GetPort() ||
             pTicket->m_Protocol != m_Configuration.m_Protocol || CRYPTO_memcmp(pTicket->m_uiServerCertificate, peer.m_Certificate.m_uiSha256, 32) != 0)
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::AuthenticationFailed));
      return;
    }
  }
  writer.Uuid(pTicket != nullptr ? pTicket->m_Session : xiiUuid());
  xiiUInt8 uiEmptyToken[32] = {};
  writer.Bytes(pTicket != nullptr ? xiiArrayPtr<const xiiUInt8>(pTicket->m_uiSecret) : xiiArrayPtr<const xiiUInt8>(uiEmptyToken));
  const auto pAuthentication = peer.m_Options.m_pAuthentication;
  const auto authentication  = pAuthentication != nullptr ? pAuthentication->GetBytes() : xiiArrayPtr<const xiiUInt8>();
  if (!authentication.IsEmpty() && (m_Configuration.m_Authentication != xiiNetworkAuthenticationPolicy::Application || !m_Capabilities.m_bEncrypted))
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::InvalidArgument));
    return;
  }
  writer.U32(authentication.GetCount());
  writer.Bytes(authentication);
  Queue(peer, xiiNetworkFrameType::Hello, writer);
  xiiNetworkSecurityInternal::ClearSecret(writer.m_Bytes);
}

xiiNetworkError xiiNetworkHostRuntime::EstablishSession(Peer& peer, xiiUuid requested, xiiArrayPtr<const xiiUInt8> tokenHash, bool bAllowFresh)
{
  XII_LOCK(m_SessionMutex);
  const auto     now     = xiiTime::Now();
  SessionRecord* pRecord = nullptr;
  xiiUInt32      uiSlot  = 0;
  if (requested.IsValid())
  {
    for (uiSlot = 0; uiSlot < m_Sessions.GetCount(); ++uiSlot)
    {
      auto& record = m_Sessions[uiSlot];
      if (record.m_bInUse && record.m_Id == requested && record.m_Expires > now && !record.m_Active.IsValid() && record.m_bMayResume &&
          record.m_sIdentity == peer.m_Authorization.m_sIdentity && tokenHash.GetCount() == 32 &&
          CRYPTO_memcmp(record.m_uiTokenHash, tokenHash.GetPtr(), 32) == 0 && CRYPTO_memcmp(record.m_uiCertificate, peer.m_Certificate.m_uiSha256, 32) == 0)
      {
        pRecord                       = &record;
        peer.m_Info.m_bSessionResumed = true;
        break;
      }
    }
    if (pRecord == nullptr && !bAllowFresh)
    {
      return xiiNetworkError(xiiNetworkErrorCode::SessionExpired);
    }
  }
  if (pRecord == nullptr)
  {
    auto error = RandomSession(peer.m_Info.m_Session);
    if (error.Failed())
    {
      return error;
    }
    if (!m_Configuration.m_bEnableSessionRecovery || !peer.m_Authorization.m_bMayResumeSession)
    {
      return {};
    }
    for (uiSlot = 0; uiSlot < m_Sessions.GetCount(); ++uiSlot)
    {
      if (!m_Sessions[uiSlot].m_bInUse || (!m_Sessions[uiSlot].m_Active.IsValid() && m_Sessions[uiSlot].m_Expires <= now))
      {
        pRecord            = &m_Sessions[uiSlot];
        *pRecord           = {};
        pRecord->m_Id      = peer.m_Info.m_Session;
        pRecord->m_Created = now;
        pRecord->m_Expires = now + m_Configuration.m_MaximumSessionLifetime;
        break;
      }
    }
    if (pRecord == nullptr)
    {
      return xiiNetworkError(xiiNetworkErrorCode::Backpressure);
    }
  }
  peer.m_Info.m_Session                         = pRecord->m_Id;
  peer.m_uiSessionSlot                          = uiSlot;
  xiiSharedPtr<xiiNetworkSessionTicket> pTicket = XII_DEFAULT_NEW(xiiNetworkSessionTicket);
  auto                                  error   = xiiNetworkSecurityInternal::RandomBytes(pTicket->m_uiSecret);
  if (error.Failed() || !DigestToken(pTicket->m_uiSecret, pRecord->m_uiTokenHash))
  {
    pRecord->m_bInUse = false;
    return error.Failed() ? error : xiiNetworkSecurityInternal::LastError();
  }
  pRecord->m_Active    = peer.m_Handle;
  pRecord->m_sIdentity = peer.m_Authorization.m_sIdentity;
  std::memcpy(pRecord->m_uiCertificate, peer.m_Certificate.m_uiSha256, 32);
  pRecord->m_bMayResume = true;
  pRecord->m_bInUse     = true;
  pTicket->m_Session    = pRecord->m_Id;
  pTicket->m_Expires    = pRecord->m_Created + m_Configuration.m_MaximumSessionLifetime;
  peer.m_pTicket        = pTicket;
  return {};
}

void xiiNetworkHostRuntime::SendWelcome(Peer& peer)
{
  peer.m_Authorization.m_bMayResumeSession &= peer.m_bWantsRecovery;
  const auto error = EstablishSession(peer, peer.m_RequestedSession, peer.m_uiRequestedTokenHash, peer.m_bAllowFreshSession);
  if (error.Failed())
  {
    Close(peer, error);
    return;
  }
  peer.m_Info.m_sAuthenticatedIdentity = peer.m_Authorization.m_sIdentity;
  WireWriter writer;
  writer.Uuid(m_Configuration.m_Protocol);
  writer.U32(peer.m_Info.m_uiProtocolVersion);
  writer.U64(peer.m_Info.m_uiNegotiatedFeatures);
  writer.U32(peer.m_uiDeliveryModes);
  writer.U32(peer.m_Info.m_uiMaximumMessageBytes);
  writer.U32(peer.m_uiMaximumChannels);
  writer.Uuid(peer.m_Info.m_Session);
  writer.U8(peer.m_Info.m_bSessionResumed ? 1 : 0);
  writer.U8(static_cast<xiiUInt8>((peer.m_Authorization.m_bMaySend ? 1 : 0) | (peer.m_Authorization.m_bMayOpenChannels ? 2 : 0) | (peer.m_Authorization.m_bMayResumeSession ? 4 : 0)));
  writer.Text(peer.m_Authorization.m_sIdentity);
  const auto uiLifetime = peer.m_pTicket != nullptr ? static_cast<xiiUInt64>(xiiMath::Max(0.0, (peer.m_pTicket->m_Expires - xiiTime::Now()).GetMilliseconds())) : 0;
  writer.U64(uiLifetime);
  xiiUInt8 uiEmpty[32] = {};
  writer.Bytes(peer.m_pTicket != nullptr ? xiiArrayPtr<const xiiUInt8>(peer.m_pTicket->m_uiSecret) : xiiArrayPtr<const xiiUInt8>(uiEmpty));
  Queue(peer, xiiNetworkFrameType::Welcome, writer);
  xiiNetworkSecurityInternal::ClearSecret(writer.m_Bytes);
  peer.m_Info.m_State = xiiNetworkConnectionState::Negotiating;
}

void xiiNetworkHostRuntime::Activate(Peer& peer)
{
  if (peer.m_bClosing)
  {
    return;
  }
  auto& defaultChannel = peer.m_Channels[1];
  if (peer.m_pQuic != nullptr)
  {
    if (!ReserveEvent())
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
      return;
    }
    defaultChannel.m_uiReservedEvents = 1;
  }
  peer.m_bActivated                               = true;
  peer.m_Info.m_State                             = xiiNetworkConnectionState::Connected;
  auto& channel                                   = defaultChannel;
  channel.m_uiId                                  = DefaultChannelId;
  channel.m_uiGeneration                          = 1;
  channel.m_State                                 = ChannelState::Open;
  channel.m_Configuration.m_uiMaximumQueuedBytes  = m_Configuration.m_uiMaximumPeerSendBytes;
  peer.m_LastActivity                             = xiiTime::Now();
  peer.m_Info.m_EstablishmentTime                 = peer.m_LastActivity - peer.m_AttemptStartedAt;
  xiiSharedPtr<xiiNetworkSessionInfo> sessionInfo = XII_DEFAULT_NEW(xiiNetworkSessionInfo);
  sessionInfo->m_Connection                       = peer.m_Handle;
  sessionInfo->m_DefaultChannel                   = {peer.m_Handle, DefaultChannelId, peer.m_uiAttempt};
  sessionInfo->m_ConnectionInfo                   = peer.m_Info;
  sessionInfo->m_PeerCertificate                  = peer.m_Certificate;
  peer.m_pSessionInfo                             = std::move(sessionInfo);
  ++m_uiConnectionsEstablished;
  if (!PeerEvent(peer, xiiNetworkEventType::Connected))
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
  }
  if (peer.m_bIncoming)
  {
    peer.m_pTicket.Clear();
  }
}

void xiiNetworkHostRuntime::Frame(Peer& peer, xiiNetworkFrame&& frame, xiiNetworkReceiveState& receive)
{
  peer.m_LastActivity = xiiTime::Now();
  const auto type     = frame.m_Header.m_Type;
  WireReader reader(frame.m_Payload.GetBytes());
  if (peer.m_bProtocolRejected)
  {
    if (type == xiiNetworkFrameType::Hello || type == xiiNetworkFrameType::Welcome)
    {
      xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
    }
    return;
  }
  auto reject = [&](xiiNetworkErrorCode code) {
    peer.m_bProtocolRejected = true;
    if (peer.m_bActivated)
    {
      Close(peer, xiiNetworkError(code));
      return;
    }
    WireWriter reason;
    reason.U32(static_cast<xiiUInt32>(code));
    Queue(peer, xiiNetworkFrameType::Disconnect, reason);
    peer.m_bDisconnectSent = true;
    Close(peer, xiiNetworkError(code), true);
  };
  const bool bChannelFrame = type == xiiNetworkFrameType::Data || type == xiiNetworkFrameType::OpenChannel || type == xiiNetworkFrameType::ChannelAccepted || type == xiiNetworkFrameType::CloseChannel;
  if (bChannelFrame != (frame.m_Header.m_uiChannel != 0))
  {
    reject(xiiNetworkErrorCode::ProtocolViolation);
    return;
  }
  if (type == xiiNetworkFrameType::Hello)
  {
    if (!peer.m_bIncoming || peer.m_bHelloReceived || peer.m_bActivated || frame.m_Header.m_uiSequence != 0)
    {
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    const auto uiMagic           = reader.U32();
    const auto protocol          = reader.Uuid();
    const auto uiMinimum         = reader.U32();
    const auto uiMaximum         = reader.U32();
    const auto uiSupported       = reader.U64();
    const auto uiRequired        = reader.U64();
    const auto uiDelivery        = reader.U32();
    const auto uiMaximumMessage  = reader.U32();
    const auto uiMaximumChannels = reader.U32();
    const auto uiAuthentication  = reader.U8();
    const auto uiFlags           = reader.U8();
    peer.m_RequestedSession      = reader.Uuid();
    const auto token             = reader.Bytes(32);
    const auto uiCredentialBytes = reader.U32();
    const auto credentials       = reader.Bytes(uiCredentialBytes);
    if (!reader.Done() || uiCredentialBytes > 4096 || uiFlags > 3 || uiMagic != 0x58494933U || uiMinimum == 0 || uiMaximum < uiMinimum ||
        uiMaximumMessage == 0 || uiMaximumChannels < 2 || uiMaximumChannels > 65534 || (uiRequired & ~uiSupported) != 0)
    {
      xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    if (protocol != m_Configuration.m_Protocol || uiMaximum < m_Configuration.m_uiMinimumProtocolVersion || uiMinimum > m_Configuration.m_uiMaximumProtocolVersion ||
        (uiRequired & ~m_Configuration.m_uiSupportedFeatures) != 0 || (m_Configuration.m_uiRequiredFeatures & ~uiSupported) != 0 ||
        (uiDelivery & m_Capabilities.m_uiDeliveryModes & xiiNetworkDeliveryMask(xiiNetworkDeliveryMode::ReliableOrdered)) == 0 ||
        uiAuthentication != static_cast<xiiUInt8>(m_Configuration.m_Authentication))
    {
      xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
      reject(xiiNetworkErrorCode::ProtocolMismatch);
      return;
    }
    peer.m_Info.m_uiProtocolVersion              = xiiMath::Min(uiMaximum, m_Configuration.m_uiMaximumProtocolVersion);
    peer.m_Info.m_uiNegotiatedFeatures           = uiSupported & m_Configuration.m_uiSupportedFeatures;
    peer.m_uiDeliveryModes                       = uiDelivery & m_Capabilities.m_uiDeliveryModes;
    peer.m_Info.m_Capabilities.m_uiDeliveryModes = peer.m_uiDeliveryModes;
    peer.m_Info.m_uiMaximumMessageBytes          = xiiMath::Min(uiMaximumMessage, m_Configuration.m_uiMaximumMessageBytes);
    peer.m_uiMaximumChannels                     = xiiMath::Min(uiMaximumChannels, m_Configuration.m_uiMaximumChannelsPerConnection);
    peer.m_bAllowFreshSession                    = (uiFlags & 1) != 0;
    peer.m_Authorization.m_bMayResumeSession     = (uiFlags & 2) != 0;
    peer.m_bWantsRecovery                        = (uiFlags & 2) != 0;
    peer.m_bHelloReceived                        = true;
    if (!DigestToken(token, peer.m_uiRequestedTokenHash))
    {
      xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
      reject(xiiNetworkErrorCode::TlsError);
      return;
    }
    if (m_Configuration.m_Authentication == xiiNetworkAuthenticationPolicy::Application)
    {
      peer.m_Info.m_State                                        = xiiNetworkConnectionState::Authenticating;
      peer.m_Deadline                                            = xiiTime::Now() + m_Configuration.m_AuthenticationTimeout;
      xiiSharedPtr<xiiNetworkAuthenticationData> pAuthentication = XII_DEFAULT_NEW(xiiNetworkAuthenticationData);
      const auto                                 error           = pAuthentication->Initialize(credentials);
      if (error.Failed())
      {
        xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
        reject(error.m_Code);
        return;
      }
      if (peer.m_uiReservedEvents <= 1)
      {
        if (!ReserveEvent())
        {
          xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
          reject(xiiNetworkErrorCode::Backpressure);
          return;
        }
      }
      else
      {
        --peer.m_uiReservedEvents;
      }
      xiiNetworkEvent event;
      event.m_Type            = xiiNetworkEventType::AuthenticationRequired;
      event.m_Connection      = peer.m_Handle;
      event.m_pAuthentication = pAuthentication;
      Publish(std::move(event));
    }
    else
    {
      if (!credentials.IsEmpty() || (m_Configuration.m_Authentication == xiiNetworkAuthenticationPolicy::Certificate && peer.m_Certificate.m_Der.IsEmpty()))
      {
        xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
        reject(xiiNetworkErrorCode::AuthenticationFailed);
        return;
      }
      if (m_Configuration.m_Authentication == xiiNetworkAuthenticationPolicy::LocalUser)
      {
        const auto error = m_IO.GetLocalPeerIdentity(peer.m_Registration, peer.m_Authorization.m_sIdentity, &peer.m_Info.m_uiLocalPeerProcessId);
        if (error.Failed())
        {
          reject(xiiNetworkErrorCode::AuthenticationFailed);
          return;
        }
        peer.m_Authorization.m_bMayResumeSession = false;
      }
      else
      {
        peer.m_Authorization.m_sIdentity = m_Capabilities.m_bAuthenticated ? CertificateIdentity(peer.m_Certificate) : xiiString("development");
      }
      SendWelcome(peer);
    }
    xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
    return;
  }
  if (type == xiiNetworkFrameType::Welcome)
  {
    if (peer.m_bIncoming || peer.m_bWelcomeReceived || frame.m_Header.m_uiSequence != 0)
    {
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    const auto protocol          = reader.Uuid();
    const auto uiVersion         = reader.U32();
    const auto uiFeatures        = reader.U64();
    const auto uiModes           = reader.U32();
    const auto uiMaximumMessage  = reader.U32();
    const auto uiMaximumChannels = reader.U32();
    const auto session           = reader.Uuid();
    const auto uiResumed         = reader.U8();
    const auto uiPermissions     = reader.U8();
    const auto sIdentity         = reader.Text(256);
    const auto uiLifetime        = reader.U64();
    const auto token             = reader.Bytes(32);
    if (!reader.Done() || protocol != m_Configuration.m_Protocol || uiVersion < m_Configuration.m_uiMinimumProtocolVersion || uiVersion > m_Configuration.m_uiMaximumProtocolVersion ||
        (uiFeatures & m_Configuration.m_uiRequiredFeatures) != m_Configuration.m_uiRequiredFeatures || (uiFeatures & ~m_Configuration.m_uiSupportedFeatures) != 0 ||
        (uiModes & ~m_Capabilities.m_uiDeliveryModes) != 0 || (uiModes & xiiNetworkDeliveryMask(xiiNetworkDeliveryMode::ReliableOrdered)) == 0 ||
        uiMaximumMessage == 0 || uiMaximumMessage > m_Configuration.m_uiMaximumMessageBytes || uiMaximumChannels < 2 || uiMaximumChannels > m_Configuration.m_uiMaximumChannelsPerConnection ||
        !session.IsValid() || uiResumed > 1 || uiPermissions > 7 || sIdentity.IsEmpty() || uiLifetime > 86400000 ||
        (uiResumed != 0 && (peer.m_Options.m_pSessionTicket == nullptr || peer.m_Options.m_pSessionTicket->m_Session != session)))
    {
      xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
      reject(xiiNetworkErrorCode::ProtocolMismatch);
      return;
    }
    peer.m_Info.m_uiProtocolVersion              = uiVersion;
    peer.m_Info.m_uiNegotiatedFeatures           = uiFeatures;
    peer.m_uiDeliveryModes                       = uiModes;
    peer.m_Info.m_Capabilities.m_uiDeliveryModes = uiModes;
    peer.m_Info.m_uiMaximumMessageBytes          = uiMaximumMessage;
    peer.m_uiMaximumChannels                     = uiMaximumChannels;
    peer.m_Info.m_Session                        = session;
    peer.m_Info.m_bSessionResumed                = uiResumed != 0;
    if (m_Configuration.m_Authentication == xiiNetworkAuthenticationPolicy::LocalUser)
    {
      const auto error = m_IO.GetLocalPeerIdentity(peer.m_Registration, peer.m_Info.m_sAuthenticatedIdentity, &peer.m_Info.m_uiLocalPeerProcessId);
      if (error.Failed())
      {
        reject(xiiNetworkErrorCode::AuthenticationFailed);
        return;
      }
    }
    else
    {
      peer.m_Info.m_sAuthenticatedIdentity = m_Capabilities.m_bAuthenticated ? CertificateIdentity(peer.m_Certificate) : xiiString("development");
    }
    peer.m_Authorization.m_sIdentity         = sIdentity;
    peer.m_Authorization.m_bMaySend          = (uiPermissions & 1) != 0;
    peer.m_Authorization.m_bMayOpenChannels  = (uiPermissions & 2) != 0;
    peer.m_Authorization.m_bMayResumeSession = (uiPermissions & 4) != 0;
    peer.m_bWelcomeReceived                  = true;
    if (uiLifetime != 0)
    {
      if (!m_Capabilities.m_bEncrypted || !m_Configuration.m_bEnableSessionRecovery)
      {
        xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
        reject(xiiNetworkErrorCode::ProtocolMismatch);
        return;
      }
      xiiSharedPtr<xiiNetworkSessionTicket> pTicket = XII_DEFAULT_NEW(xiiNetworkSessionTicket);
      pTicket->m_Session                            = session;
      pTicket->m_Protocol                           = protocol;
      pTicket->m_sOrigin                            = peer.m_Options.m_sServerName;
      pTicket->m_uiPort                             = peer.m_Info.m_RemoteEndpoint.GetPort();
      pTicket->m_Expires                            = xiiTime::Now() + xiiTime::MakeFromMilliseconds(static_cast<double>(uiLifetime));
      std::memcpy(pTicket->m_uiSecret, token.GetPtr(), 32);
      std::memcpy(pTicket->m_uiServerCertificate, peer.m_Certificate.m_uiSha256, 32);
      peer.m_pTicket                  = pTicket;
      peer.m_Options.m_pSessionTicket = pTicket;
    }
    xiiNetworkSecurityInternal::ClearSecret(frame.m_Payload.GetBytes());
    receive.m_Decoder.SetMaximumPayloadBytes(peer.m_Info.m_uiMaximumMessageBytes).IgnoreResult();
    Queue(peer, xiiNetworkFrameType::Ready);
    return;
  }
  if (type == xiiNetworkFrameType::Ready)
  {
    if (!peer.m_bIncoming || !peer.m_bHelloReceived || peer.m_bReadyReceived || !reader.Done() || !peer.m_Info.m_Session.IsValid() || frame.m_Header.m_uiSequence != 0)
    {
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    peer.m_bReadyReceived = true;
    Queue(peer, xiiNetworkFrameType::Activated);
    receive.m_Decoder.SetMaximumPayloadBytes(peer.m_Info.m_uiMaximumMessageBytes).IgnoreResult();
    Activate(peer);
    return;
  }
  if (type == xiiNetworkFrameType::Activated)
  {
    if (peer.m_bIncoming || !peer.m_bWelcomeReceived || peer.m_bActivated || !reader.Done() || frame.m_Header.m_uiSequence != 0)
    {
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    Activate(peer);
    return;
  }
  if (type == xiiNetworkFrameType::Disconnect)
  {
    const auto uiCode = reader.U32();
    if (!reader.Done() || uiCode > static_cast<xiiUInt32>(xiiNetworkErrorCode::UnsupportedCapability))
    {
      Close(peer, xiiNetworkError(xiiNetworkErrorCode::ProtocolViolation));
      return;
    }
    peer.m_bDisconnectSent          = true;
    peer.m_bLocalDisconnectReceived = peer.m_Info.m_LocalServiceEndpoint.IsValid();
    Close(peer, xiiNetworkError(uiCode == 0 ? xiiNetworkErrorCode::ConnectionClosed : static_cast<xiiNetworkErrorCode>(uiCode)), true);
    return;
  }
  if (!peer.m_bActivated)
  {
    reject(xiiNetworkErrorCode::ProtocolViolation);
    return;
  }
  if (type == xiiNetworkFrameType::Ping || type == xiiNetworkFrameType::Pong)
  {
    if (!reader.Done())
    {
      reject(xiiNetworkErrorCode::ProtocolViolation);
      return;
    }
    if (type == xiiNetworkFrameType::Ping)
    {
      Queue(peer, xiiNetworkFrameType::Pong, {}, 0, frame.m_Header.m_uiSequence);
    }
    else if (frame.m_Header.m_uiSequence == peer.m_uiPingSequence && !peer.m_PingSent.IsZero())
    {
      peer.m_Info.m_Statistics.m_HeartbeatRoundTrip = xiiTime::Now() - peer.m_PingSent;
      peer.m_PingSent                               = xiiTime::MakeZero();
    }
    return;
  }
  if (type != xiiNetworkFrameType::Data)
  {
    ProcessChannelFrame(peer, std::move(frame));
    return;
  }
  auto* pChannel = Find(peer, {peer.m_Handle, frame.m_Header.m_uiChannel, peer.m_uiAttempt});
  if (pChannel == nullptr || (pChannel->m_State != ChannelState::Open && pChannel->m_State != ChannelState::Closing))
  {
    reject(xiiNetworkErrorCode::ChannelClosed);
    return;
  }
  if (peer.m_bIncoming && !peer.m_Authorization.m_bMaySend)
  {
    reject(xiiNetworkErrorCode::PermissionDenied);
    return;
  }
  const auto delivery = pChannel->m_Configuration.m_Delivery;
  if (delivery == xiiNetworkDeliveryMode::ReliableOrdered && frame.m_Header.m_uiSequence != pChannel->m_uiLastReceiveSequence + 1)
  {
    reject(xiiNetworkErrorCode::ProtocolViolation);
    return;
  }
  if (delivery == xiiNetworkDeliveryMode::UnreliableSequenced && frame.m_Header.m_uiSequence <= pChannel->m_uiLastReceiveSequence)
  {
    ++pChannel->m_Statistics.m_uiStaleMessages;
    ++peer.m_Info.m_Statistics.m_uiStaleDatagrams;
    return;
  }
  pChannel->m_uiLastReceiveSequence = frame.m_Header.m_uiSequence;
  if (!receive.m_bEventReserved && !ReserveEvent())
  {
    Close(peer, xiiNetworkError(xiiNetworkErrorCode::Backpressure));
    return;
  }
  receive.m_bEventReserved  = false;
  receive.m_uiReservedBytes = 0;
  xiiNetworkEvent event;
  event.m_Type         = xiiNetworkEventType::Message;
  event.m_pSessionInfo = peer.m_pSessionInfo;
  event.m_Connection   = peer.m_Handle;
  event.m_Channel      = {peer.m_Handle, frame.m_Header.m_uiChannel, peer.m_uiAttempt};
  event.m_Payload      = std::move(frame.m_Payload);
  ++pChannel->m_Statistics.m_uiMessagesReceived;
  ++peer.m_Info.m_Statistics.m_uiMessagesReceived;
  peer.m_Info.m_Statistics.m_uiPayloadBytesReceived += event.m_Payload.GetCount();
  TrackMessage(false, event.m_Payload.GetCount());
  Publish(std::move(event));
}

void xiiNetworkHostRuntime::DetachSession(Peer& peer)
{
  if (peer.m_uiSessionSlot == InvalidSlot)
  {
    return;
  }
  XII_LOCK(m_SessionMutex);
  auto& record = m_Sessions[peer.m_uiSessionSlot];
  if (record.m_Active == peer.m_Handle)
  {
    record.m_Active  = {};
    record.m_Expires = xiiMath::Min(record.m_Created + m_Configuration.m_MaximumSessionLifetime, xiiTime::Now() + m_Configuration.m_SessionRecoveryWindow);
    if (!peer.m_bActivated || peer.m_bGraceful || peer.m_bCloseRequested || !record.m_bMayResume)
    {
      record.m_bInUse = false;
    }
  }
  peer.m_uiSessionSlot = InvalidSlot;
}

xiiNetworkError xiiNetworkHost::RevokeSession(xiiUuid session)
{
  XII_LOCK(m_pRuntime->m_SessionMutex);
  for (auto& record : m_pRuntime->m_Sessions)
  {
    if (record.m_bInUse && record.m_Id == session)
    {
      record.m_bInUse = false;
      m_pRuntime->m_WorkSignal.RaiseSignal();
      return {};
    }
  }
  return xiiNetworkError(xiiNetworkErrorCode::SessionExpired);
}
