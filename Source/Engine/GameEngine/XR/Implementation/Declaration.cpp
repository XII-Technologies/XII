#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/Declarations.h>
#include <GameEngine/XR/XRRemotingInterface.h>

xiiCVarBool   xiiXRRemotingInterface::cvar_XrRemoting("XR.Remoting", false, xiiCVarFlags::Default, "Enable XR Remoting if available.");
xiiCVarString xiiXRRemotingInterface::cvar_XrRemotingHostName("XR.Remoting.HostName", "", xiiCVarFlags::Save, "Hostname to connect to for XR Remoting.");

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRTransformSpace, 1)
  XII_BITFLAGS_CONSTANTS(xiiXRTransformSpace::Local, xiiXRTransformSpace::Global)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRDeviceType, 1)
  XII_BITFLAGS_CONSTANTS(xiiXRDeviceType::HMD, xiiXRDeviceType::LeftController, xiiXRDeviceType::RightController)
  XII_BITFLAGS_CONSTANTS(xiiXRDeviceType::DeviceID0, xiiXRDeviceType::DeviceID1, xiiXRDeviceType::DeviceID2, xiiXRDeviceType::DeviceID3)
  XII_BITFLAGS_CONSTANTS(xiiXRDeviceType::DeviceID4, xiiXRDeviceType::DeviceID5, xiiXRDeviceType::DeviceID6, xiiXRDeviceType::DeviceID7)
  XII_BITFLAGS_CONSTANTS(xiiXRDeviceType::DeviceID8, xiiXRDeviceType::DeviceID9, xiiXRDeviceType::DeviceID10, xiiXRDeviceType::DeviceID11)
  XII_BITFLAGS_CONSTANTS(xiiXRDeviceType::DeviceID12, xiiXRDeviceType::DeviceID13, xiiXRDeviceType::DeviceID14, xiiXRDeviceType::DeviceID15)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRRemotingConnectionState, 1)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingConnectionState::Disconnected, xiiXRRemotingConnectionState::Connecting, xiiXRRemotingConnectionState::Connected)
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_ENUM(xiiXRRemotingDisconnectReason, 1)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::None, xiiXRRemotingDisconnectReason::Unknown, xiiXRRemotingDisconnectReason::NoServerCertificate)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::HandshakePortBusy, xiiXRRemotingDisconnectReason::HandshakeUnreachable, xiiXRRemotingDisconnectReason::HandshakeConnectionFailed)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::AuthenticationFailed, xiiXRRemotingDisconnectReason::RemotingVersionMismatch, xiiXRRemotingDisconnectReason::IncompatibleTransportProtocols)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::HandshakeFailed, xiiXRRemotingDisconnectReason::TransportPortBusy, xiiXRRemotingDisconnectReason::TransportUnreachable)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::TransportConnectionFailed, xiiXRRemotingDisconnectReason::ProtocolVersionMismatch, xiiXRRemotingDisconnectReason::ProtocolError)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::VideoCodecNotAvailable, xiiXRRemotingDisconnectReason::Canceled, xiiXRRemotingDisconnectReason::ConnectionLost)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::DeviceLost, xiiXRRemotingDisconnectReason::DisconnectRequest, xiiXRRemotingDisconnectReason::HandshakeNetworkUnreachable)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::HandshakeConnectionRefused, xiiXRRemotingDisconnectReason::VideoFormatNotAvailable, xiiXRRemotingDisconnectReason::PeerDisconnectRequest)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::PeerDisconnectTimeout, xiiXRRemotingDisconnectReason::SessionOpenTimeout, xiiXRRemotingDisconnectReason::RemotingHandshakeTimeout)
  XII_BITFLAGS_CONSTANTS(xiiXRRemotingDisconnectReason::InternalError)
XII_END_STATIC_REFLECTED_ENUM;
// clang-format on

xiiXRDeviceState::xiiXRDeviceState()
{
  m_vGripPosition.SetZero();
  m_qGripRotation.SetIdentity();

  m_vAimPosition.SetZero();
  m_qAimRotation.SetIdentity();
}


XII_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_Declaration);
