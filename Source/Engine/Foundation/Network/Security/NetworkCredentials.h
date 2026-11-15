/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Network/NetworkError.h>
#include <Foundation/Network/Security/NetworkSecurityConfig.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Timestamp.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

enum class xiiNetworkCertificateUsage : xiiUInt8
{
  Server,
  Client
};

class xiiNetworkCredentialsState;
class xiiNetworkSecurityAccess;

struct XII_FOUNDATION_DLL xiiNetworkCertificate
{
  xiiUInt8                  m_uiSha256[32] = {};
  xiiString                 m_sSubject;
  xiiString                 m_sIssuer;
  xiiTimestamp              m_ValidFrom;
  xiiTimestamp              m_ValidUntil;
  xiiDynamicArray<xiiUInt8> m_Der;
};

/// Immutable after Load(). Load and destruction must not race publication; all other calls are safe
/// concurrently. Shared snapshots keep certificate/key/trust ownership through native handshakes.
/// Replacing the snapshot for a host changes only subsequently established connections.
class XII_FOUNDATION_DLL xiiNetworkCredentials final : public xiiRefCountingImpl
{
public:
  xiiNetworkCredentials();
  ~xiiNetworkCredentials();
  xiiNetworkCredentials(const xiiNetworkCredentials&)              = delete;
  xiiNetworkCredentials& operator=(const xiiNetworkCredentials&)   = delete;
  xiiNetworkCredentials(xiiNetworkCredentials&&)                   = delete;
  xiiNetworkCredentials&        operator=(xiiNetworkCredentials&&) = delete;
  [[nodiscard]] xiiNetworkError Load(const xiiNetworkSecurityConfig& configuration, xiiArrayPtr<const xiiUInt8> privateKeyPassword = {});
  [[nodiscard]] bool            IsLoaded() const;
  [[nodiscard]] bool            HasIdentity() const;
  [[nodiscard]] xiiNetworkError GetCertificate(xiiNetworkCertificate& out_certificate) const;
  /// Offline chain/EKU/name validation against this snapshot. No network trust fetching occurs.
  [[nodiscard]] xiiNetworkError VerifyCertificate(xiiArrayPtr<const xiiUInt8> certificateDer, xiiArrayPtr<const xiiArrayPtr<const xiiUInt8>> chainDer,
                                                  xiiStringView sServerName, xiiNetworkCertificateUsage usage, xiiNetworkCertificate& out_certificate);
  /// Requires IsLoaded(); the reference remains valid for this snapshot's lifetime.
  [[nodiscard]] const xiiNetworkSecurityConfig& GetConfiguration() const;
  [[nodiscard]] static const char*              GetProviderVersion();

private:
  friend class xiiNetworkSecurityAccess;
  xiiNetworkCredentialsState* m_pState = nullptr;
};

/// Bounded, immutable application credentials. No implicit string formatting or serialization.
/// The backing bytes are cleansed when the final explicit shared owner releases them.
class XII_FOUNDATION_DLL xiiNetworkAuthenticationData final : public xiiRefCountingImpl
{
public:
  xiiNetworkAuthenticationData() = default;
  ~xiiNetworkAuthenticationData();
  xiiNetworkAuthenticationData(const xiiNetworkAuthenticationData&)                   = delete;
  xiiNetworkAuthenticationData& operator=(const xiiNetworkAuthenticationData&)        = delete;
  xiiNetworkAuthenticationData(xiiNetworkAuthenticationData&&)                        = delete;
  xiiNetworkAuthenticationData&             operator=(xiiNetworkAuthenticationData&&) = delete;
  [[nodiscard]] xiiNetworkError             Initialize(xiiArrayPtr<const xiiUInt8> bytes);
  [[nodiscard]] xiiArrayPtr<const xiiUInt8> GetBytes() const { return m_Bytes; }

private:
  xiiDynamicArray<xiiUInt8> m_Bytes;
  bool                      m_bInitialized = false;
};
