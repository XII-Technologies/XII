/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Network/Security/Implementation/NetworkSecurityInternal.h>
#include <Foundation/Network/Session/NetworkSession.h>

xiiNetworkSessionTicket::~xiiNetworkSessionTicket()
{
  xiiNetworkSecurityInternal::ClearSecret(m_uiSecret);
}

bool xiiNetworkSessionTicket::IsValid() const
{
  return m_Session.IsValid() && m_Expires > xiiTime::Now();
}
