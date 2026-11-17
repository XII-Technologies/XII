/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Messaging/Message.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMessage, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiMessageId xiiMessage::s_NextMsgId = 0;

XII_STATICLINK_FILE(Foundation, Foundation_Messaging_Implementation_Message);
