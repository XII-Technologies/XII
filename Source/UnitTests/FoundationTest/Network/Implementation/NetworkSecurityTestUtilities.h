/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Network/Security/NetworkSecurityConfig.h>
#include <Foundation/Strings/String.h>

/// Ephemeral test-only CA and identities; no private test key is stored in source control.
struct NetworkSecurityTestCertificates
{
  NetworkSecurityTestCertificates() = default;
  ~NetworkSecurityTestCertificates();
  NetworkSecurityTestCertificates(const NetworkSecurityTestCertificates&)            = delete;
  NetworkSecurityTestCertificates& operator=(const NetworkSecurityTestCertificates&) = delete;
  xiiResult                        Initialize();
  xiiNetworkSecurityConfig         Configuration(xiiStringView sIdentity = "server") const;
  xiiString                        Path(xiiStringView sName) const;
  xiiString                        m_sDirectory;
};
