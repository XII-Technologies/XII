#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/Reflection/Reflection.h>

struct xiiXRRemotingConnectionState
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    Disconnected,
    Connecting,
    Connected,
    Default = Disconnected
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRRemotingConnectionState);

struct xiiXRRemotingDisconnectReason
{
  using StorageType = xiiUInt8;
  enum Enum : xiiUInt8
  {
    None                           = 0,
    Unknown                        = 1,
    NoServerCertificate            = 2,
    HandshakePortBusy              = 3,
    HandshakeUnreachable           = 4,
    HandshakeConnectionFailed      = 5,
    AuthenticationFailed           = 6,
    RemotingVersionMismatch        = 7,
    IncompatibleTransportProtocols = 8,
    HandshakeFailed                = 9,
    TransportPortBusy              = 10,
    TransportUnreachable           = 11,
    TransportConnectionFailed      = 12,
    ProtocolVersionMismatch        = 13,
    ProtocolError                  = 14,
    VideoCodecNotAvailable         = 15,
    Canceled                       = 16,
    ConnectionLost                 = 17,
    DeviceLost                     = 18,
    DisconnectRequest              = 19,
    HandshakeNetworkUnreachable    = 20,
    HandshakeConnectionRefused     = 21,
    VideoFormatNotAvailable        = 22,
    PeerDisconnectRequest          = 23,
    PeerDisconnectTimeout          = 24,
    SessionOpenTimeout             = 25,
    RemotingHandshakeTimeout       = 26,
    InternalError                  = 27,
    Default                        = None
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_GAMEENGINE_DLL, xiiXRRemotingDisconnectReason);

struct xiiXRRemotingConnectionEventData
{
  xiiEnum<xiiXRRemotingConnectionState>  m_connectionState;
  xiiEnum<xiiXRRemotingDisconnectReason> m_disconnectReason;
};

typedef xiiEvent<const xiiXRRemotingConnectionEventData&> xiiXRRemotingConnectionEvent;

/// \brief XR Remoting singleton interface. Allows for streaming the XR application to a remote device.
///
/// Needs to be initialized before xiiXRInterface to be able to use remoting.
class xiiXRRemotingInterface
{
public:
  /// \brief Enable XR Remoting if available.
  static xiiCVarBool cvar_XrRemoting;
  /// \brief Hostname to connect to for XR Remoting.
  static xiiCVarString cvar_XrRemotingHostName;

  /// \brief Initializes the XR Remoting system. Needs to be done before xiiXRInterface is initialized.
  virtual xiiResult Initialize() = 0;
  /// \brief Shuts down XR Remoting. This will fail if XR actors still exists or if xiiXRInterface is still initialized.
  virtual xiiResult Deinitialize() = 0;
  /// \brief Returns whether XR Remoting is initialized.
  virtual bool IsInitialized() const = 0;

  /// \name Connection Functions
  ///@{

  /// \brief Tries to connect to the remote device.
  virtual xiiResult Connect(const char* remoteHostName, uint16_t remotePort = 8265, bool enableAudio = true, int maxBitrateKbps = 20000) = 0;
  /// \brief Disconnects from the remote device.
  virtual xiiResult Disconnect() = 0;
  /// \brief Get the current connection state to the remote device.
  virtual xiiEnum<xiiXRRemotingConnectionState> GetConnectionState() const = 0;
  /// \brief Returns the connection event to subscribe to connection changes.
  virtual xiiXRRemotingConnectionEvent& GetConnectionEvent() = 0;
  ///@}
};
