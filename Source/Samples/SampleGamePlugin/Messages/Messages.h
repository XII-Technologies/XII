#pragma once

#include <Foundation/Communication/Message.h>

// BEGIN-DOCS-CODE-SNIPPET: message-decl
struct xiiMsgSetText : public xiiMessage
{
  XII_DECLARE_MESSAGE_TYPE(xiiMsgSetText, xiiMessage);

  xiiString m_sText;
};
// END-DOCS-CODE-SNIPPET
