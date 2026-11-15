/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/FoundationInternal.h>
#include <Foundation/Network/Implementation/NetworkNativeThread.h>
#include <Foundation/Network/NetworkHost.h>
#include <Foundation/Network/NetworkIOContext.h>
#include <Foundation/Network/Protocol/Implementation/NetworkReceiveState.h>
#include <Foundation/Network/Protocol/NetworkFrame.h>
#include <Foundation/Network/Security/NetworkTlsStream.h>
#include <Foundation/Network/Transport/Implementation/NetworkQuic.h>
#include <Foundation/Threading/ConditionVariable.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>
#include <atomic>

XII_FOUNDATION_INTERNAL_HEADER

namespace xiiNetworkHostInternal
{
  constexpr xiiUInt32 InvalidSlot      = 0xFFFFFFFFU;
  constexpr xiiUInt32 DefaultChannelId = 0x00010001U;

  struct WireWriter
  {
    xiiHybridArray<xiiUInt8, 256> m_Bytes;
    void                          U8(xiiUInt8 uiValue);
    void                          U32(xiiUInt32 uiValue);
    void                          U64(xiiUInt64 uiValue);
    void                          Uuid(xiiUuid value);
    void                          Bytes(xiiArrayPtr<const xiiUInt8> bytes);
    void                          Text(xiiStringView sText);
  };

  struct WireReader
  {
    explicit WireReader(xiiArrayPtr<const xiiUInt8> bytes) : m_Bytes(bytes) {}
    xiiArrayPtr<const xiiUInt8> m_Bytes;
    xiiUInt32                   m_uiPosition = 0;
    bool                        m_bValid     = true;
    xiiUInt8                    U8();
    xiiUInt32                   U32();
    xiiUInt64                   U64();
    xiiUuid                     Uuid();
    xiiArrayPtr<const xiiUInt8> Bytes(xiiUInt32 uiCount);
    xiiString                   Text(xiiUInt32 uiLimit);
    bool                        Done() const { return m_bValid && m_uiPosition == m_Bytes.GetCount(); }
  };

  struct TokenBucket
  {
    double  m_fTokens = 0;
    xiiTime m_Updated;
    void    Reset(xiiUInt64 uiBurst);
    bool    Take(xiiUInt32 uiBytes, xiiUInt64 uiRate, xiiUInt64 uiBurst, xiiTime now, xiiTime& ref_nextWake);
  };

  enum class ChannelState : xiiUInt8
  {
    Free,
    Opening,
    Authorizing,
    Open,
    Closing,
    Closed
  };
  struct Channel
  {
    xiiUInt32                      m_uiId         = 0;
    xiiUInt16                      m_uiGeneration = 0;
    ChannelState                   m_State        = ChannelState::Free;
    xiiNetworkChannelConfiguration m_Configuration;
    xiiNetworkChannelStatistics    m_Statistics;
    xiiDeque<xiiUInt32>            m_SendQueue;
    xiiUInt64                      m_uiNextSendSequence    = 1;
    xiiUInt64                      m_uiLastReceiveSequence = 0;
    xiiUInt64                      m_uiPeerFinalSequence   = 0;
    xiiTime                        m_Deadline;
    xiiNetworkReceiveState*        m_pOrderedReceive       = nullptr;
    bool                           m_bHasPeerFinalSequence = false;
    xiiUInt32                      m_uiReservedEvents      = 0;
    bool                           m_bLocal                = false;
    bool                           m_bCloseReceived        = false;
    bool                           m_bCloseSent            = false;
    bool                           m_bCloseAcknowledged    = false;
    bool                           m_bNeedsControl         = false;
    bool                           m_bAccept               = false;
  };

  struct Outbound
  {
    xiiNetworkFrameHeader m_Header;
    xiiUInt8              m_uiHeader[xiiNetworkFrame::HeaderCapacity] = {};
    xiiUInt32             m_uiHeaderBytes                             = 0;
    xiiUInt32             m_uiOffset                                  = 0;
    xiiNetworkIOBuffer    m_Payload;
    xiiUInt32             m_uiMessageSlot      = InvalidSlot;
    xiiUInt64             m_uiCipherEnd        = 0;
    bool                  m_bPlaintextComplete = false;
    bool                  m_bSubmitted         = false;
  };

  struct SendSlot
  {
    xiiMutex           m_Mutex;
    xiiNetworkMessage  m_Handle;
    xiiNetworkListener m_Listener;
    xiiNetworkEndpoint m_Destination;
    xiiNetworkChannel  m_Channel;
    Outbound           m_Outbound;
    xiiUInt64          m_uiUserData     = 0;
    xiiUInt32          m_uiPayloadBytes = 0;
    bool               m_bInUse         = false;
    bool               m_bCompleted     = false;
  };

  struct Peer
  {
    mutable xiiMutex                      m_Mutex;
    xiiNetworkConnection                  m_Handle;
    xiiNetworkConnectionInfo              m_Info;
    xiiNetworkConnectOptions              m_Options;
    xiiSharedPtr<xiiNetworkCredentials>   m_pCredentials;
    xiiSharedPtr<xiiNetworkSessionTicket> m_pTicket;
    xiiNetworkCertificate                 m_Certificate;
    xiiUuid                               m_RequestedSession;
    xiiUInt8                              m_uiRequestedTokenHash[32] = {};
    bool                                  m_bAllowFreshSession       = false;
    bool                                  m_bWantsRecovery           = false;
    bool                                  m_bTransportLost           = false;
    xiiNetworkAuthorization               m_Authorization;
    xiiString                             m_sHost;
    xiiString                             m_sService;
    xiiDynamicArray<xiiNetworkEndpoint>   m_Endpoints;
    xiiUInt32                             m_uiEndpointIndex = 0;
    xiiNetworkIORegistration              m_Registration;
    xiiNetworkIOOperation                 m_ReceiveOperation;
    xiiUInt64                             m_uiAttempt         = 0;
    xiiUInt32                             m_uiPendingIO       = 0;
    xiiUInt32                             m_uiReservedEvents  = 0;
    xiiUInt32                             m_uiSessionSlot     = InvalidSlot;
    xiiUInt32                             m_uiMaximumChannels = 0;
    xiiUInt32                             m_uiDeliveryModes   = 0;
    xiiUInt32                             m_uiScheduleChannel = 1;
    xiiUInt32                             m_uiScheduleWeight  = 0;
    xiiUInt64                             m_uiCipherProduced  = 0;
    xiiUInt64                             m_uiCipherCompleted = 0;
    std::atomic<xiiUInt64>                m_uiReceiveBytes{0};
    xiiUInt64                             m_uiPingSequence = 0;
    xiiTime                               m_PingSent;
    xiiTime                               m_LastActivity;
    xiiTime                               m_AttemptStartedAt;
    xiiTime                               m_Deadline;
    xiiTime                               m_ReconnectAt;
    TokenBucket                           m_SendTokens;
    xiiUniquePtr<xiiNetworkTlsStream>     m_pTls;
    xiiNetworkReceiveState                m_Receive;
    xiiNetworkQuicConnection*             m_pQuic = nullptr;
    xiiDeque<xiiNetworkQuicEvent>         m_QuicDeferred;
    xiiTime                               m_QuicStatisticsAt;
    xiiNetworkIOBuffer                    m_CipherInput;
    xiiUInt32                             m_uiCipherOffset = 0;
    xiiDeque<Channel>                     m_Channels;
    xiiDeque<Outbound>                    m_ControlQueue;
    xiiUInt32                             m_uiActiveSend         = InvalidSlot;
    bool                                  m_bActiveControl       = false;
    bool                                  m_bInUse               = false;
    bool                                  m_bStarted             = false;
    bool                                  m_bIncoming            = false;
    bool                                  m_bCipherSendPending   = false;
    bool                                  m_bClosing             = false;
    bool                                  m_bCloseRequested      = false;
    bool                                  m_bGraceful            = true;
    bool                                  m_bDisconnectSent      = false;
    bool                                  m_bShutdownSent        = false;
    bool                                  m_bDeregistered        = false;
    bool                                  m_bPeerEof             = false;
    bool                                  m_bTlsClosed           = false;
    bool                                  m_bTerminalPublished   = false;
    bool                                  m_bTerminalConsumed    = false;
    bool                                  m_bAuthorizationReady  = false;
    bool                                  m_bAcceptAuthorization = false;
    bool                                  m_bHelloReceived       = false;
    bool                                  m_bWelcomeReceived     = false;
    bool                                  m_bReadyReceived       = false;
    bool                                  m_bActivated           = false;
    xiiNetworkError                       m_CloseError;
  };

  struct Listener
  {
    bool                     m_bDeregistered      = false;
    bool                     m_bTerminalPublished = false;
    bool                     m_bTerminalConsumed  = false;
    xiiNetworkError          m_Error;
    xiiUInt64                m_uiQueuedBytes = 0;
    mutable xiiMutex         m_Mutex;
    xiiNetworkListener       m_Handle;
    xiiNetworkIORegistration m_Registration;
    xiiNetworkEndpoint       m_Endpoint;
    xiiDeque<xiiUInt32>      m_SendQueue;
    xiiNetworkQuicListener*  m_pQuic = nullptr;
    TokenBucket              m_SendTokens;
    xiiUInt32                m_uiPendingIO     = 0;
    bool                     m_bInUse          = false;
    bool                     m_bClosing        = false;
    bool                     m_bReceivePending = false;
    xiiUInt32                m_uiMessageSlot   = InvalidSlot;
  };

  struct SessionRecord
  {
    xiiUuid              m_Id;
    xiiNetworkConnection m_Active;
    xiiString            m_sIdentity;
    xiiUInt8             m_uiTokenHash[32]   = {};
    xiiUInt8             m_uiCertificate[32] = {};
    xiiTime              m_Created;
    xiiTime              m_Expires;
    bool                 m_bInUse     = false;
    bool                 m_bMayResume = false;
  };
} // namespace xiiNetworkHostInternal

/// Protocol driver exclusively owns wire state. Public admissions take a peer lock briefly; crypto,
/// native submissions and parsing run on the protocol driver, never under the host admission lock.
/// Lock order: admission -> peer -> send pool -> event queue. Dequeue releases event locks before
/// touching peer/send state. Native callback quiescence must happen outside every peer/queue lock.
class xiiNetworkHostRuntime
{
public:
  using Peer       = xiiNetworkHostInternal::Peer;
  using Channel    = xiiNetworkHostInternal::Channel;
  using Listener   = xiiNetworkHostInternal::Listener;
  using Outbound   = xiiNetworkHostInternal::Outbound;
  using WireWriter = xiiNetworkHostInternal::WireWriter;
  using WireReader = xiiNetworkHostInternal::WireReader;
  xiiNetworkError          ConfigureTcp(xiiTcpSocket& socket) const;
  xiiNetworkHostStatistics GetStatistics() const;
  void                     TrackMessage(bool bSent, xiiUInt32 uiBytes);
  void                     TrackTerminal(xiiNetworkError error);
  xiiNetworkHostRuntime();
  ~xiiNetworkHostRuntime();
  xiiNetworkError Start(const xiiNetworkHostConfiguration& configuration);
  void            Shutdown();
  void            Run();
  bool            ProcessQuic();
  void            CompleteQuic(xiiNetworkQuicEvent&& event);
  bool            ReceiveQuic(Peer& peer, xiiNetworkQuicEvent& event);
  void            ReceiveQuicDatagram(Peer& peer, xiiNetworkQuicEvent& event);
  void            DriveQuicOutput(Peer& peer, xiiTime now, xiiTime& ref_nextWake);
  void            CloseQuicPeer(Peer& peer);
  void            FinishQuicChannel(Peer& peer, Channel& channel);
  void            DrivePeer(Peer& peer, xiiTime now, xiiTime& ref_nextWake);
  void            DriveListener(Listener& listener, xiiTime now, xiiTime& ref_nextWake);
  void            CompleteIO(xiiNetworkIOResult&& result);
  void            StartAttempt(Peer& peer);
  void            StartTransport(Peer& peer);
  void            BeginProtocol(Peer& peer);
  void            Receive(Peer& peer);
  void            DriveInput(Peer& peer);
  void            Parse(Peer& peer, xiiArrayPtr<const xiiUInt8> bytes);
  xiiUInt32       Parse(Peer& peer, xiiNetworkReceiveState& receive, xiiArrayPtr<const xiiUInt8> bytes);
  void            Frame(Peer& peer, xiiNetworkFrame&& frame, xiiNetworkReceiveState& receive);
  void            DriveOutput(Peer& peer, xiiTime now, xiiTime& ref_nextWake);
  void            FinishSend(Peer& peer, xiiUInt32 uiSlot, xiiNetworkError error);
  void            Close(Peer& peer, xiiNetworkError error, bool bGraceful = false);
  void            FinishClose(Peer& peer);
  void            Queue(Peer& peer, xiiNetworkFrameType type, const WireWriter& writer = {}, xiiUInt32 uiChannel = 0, xiiUInt64 uiSequence = 0);
  void            SendHello(Peer& peer);
  void            SendWelcome(Peer& peer);
  void            Activate(Peer& peer);
  void            ProcessChannels(Peer& peer);
  void            ProcessChannelFrame(Peer& peer, xiiNetworkFrame&& frame);
  void            DetachSession(Peer& peer);
  xiiNetworkError EstablishSession(Peer& peer, xiiUuid requested, xiiArrayPtr<const xiiUInt8> secret, bool bAllowFresh);
  void            FinishDatagram(xiiUInt32 uiSlot, xiiNetworkError error);
  xiiNetworkError AllocatePeer(bool bIncoming, Peer*& out_pPeer);
  xiiNetworkError AllocateSend(xiiNetworkChannel channel, xiiNetworkIOBuffer&& payload, xiiUInt64 uiUserData, xiiNetworkMessage& out_message);
  Peer*           Find(xiiNetworkConnection handle);
  Channel*        Find(Peer& peer, xiiNetworkChannel handle);
  Listener*       Find(xiiNetworkListener handle);
  bool            ReserveEvent();
  void            ReleaseEvents(xiiUInt32 uiCount);
  void            Publish(xiiNetworkEvent&& event);
  bool            PeerEvent(Peer& peer, xiiNetworkEventType type, xiiNetworkError error = {});
  bool            ChannelEvent(Peer& peer, Channel& channel, xiiNetworkEventType type, xiiNetworkError error = {});
  bool            Poll(xiiNetworkEvent& out_event);
  bool            OwnsBuffer(const xiiNetworkIOBuffer& buffer) const;

  mutable xiiMutex                                m_LifecycleMutex;
  mutable xiiMutex                                m_AdmissionMutex;
  mutable xiiMutex                                m_SendPoolMutex;
  mutable xiiMutex                                m_CredentialsMutex;
  mutable xiiMutex                                m_SessionMutex;
  mutable xiiConditionVariable                    m_EventsMutex;
  xiiThreadSignal                                 m_WorkSignal;
  xiiNetworkIOContext                             m_IO;
  xiiUniquePtr<xiiNetworkQuic>                    m_pQuic;
  xiiNetworkHostConfiguration                     m_Configuration;
  xiiNetworkTransportCapabilities                 m_Capabilities;
  xiiSharedPtr<xiiNetworkCredentials>             m_pCredentials;
  xiiNetworkIOInternal::NativeThread              m_Thread;
  xiiDeque<Peer>                                  m_Peers;
  xiiDeque<Listener>                              m_Listeners;
  xiiDeque<xiiNetworkHostInternal::SendSlot>      m_Sends;
  xiiDeque<xiiNetworkHostInternal::SessionRecord> m_Sessions;
  xiiDeque<xiiNetworkEvent>                       m_Events;
  xiiNetworkHostInternal::TokenBucket             m_SendTokens;
  std::atomic<bool>                               m_bRunning{false};
  std::atomic<bool>                               m_bStopping{false};
  std::atomic<xiiUInt32>                          m_uiEventReservations{0};
  std::atomic<xiiUInt64>                          m_uiNextAttempt{1};
  std::atomic<xiiUInt64>                          m_uiReceiveBytes{0};
  std::atomic<xiiUInt64>                          m_uiConnectionsEstablished{0};
  std::atomic<xiiUInt64>                          m_uiAuthenticationFailures{0};
  std::atomic<xiiUInt64>                          m_uiProtocolFailures{0};
  std::atomic<xiiUInt64>                          m_uiTimeouts{0};
  std::atomic<xiiUInt64>                          m_uiBackpressureEvents{0};
  std::atomic<xiiUInt64>                          m_uiMessagesSent{0};
  std::atomic<xiiUInt64>                          m_uiMessagesReceived{0};
  std::atomic<xiiUInt64>                          m_uiPayloadBytesSent{0};
  std::atomic<xiiUInt64>                          m_uiPayloadBytesReceived{0};
  std::atomic<xiiUInt64>                          m_uiDroppedDatagrams{0};
  const xiiUInt64                                 m_uiHostId;
  bool                                            m_bStarted = false;
};
