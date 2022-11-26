#include <SampleGamePlugin/SampleGamePluginPCH.h>

#include <SampleGamePlugin/Messages/Messages.h>

// clang-format off
// BEGIN-DOCS-CODE-SNIPPET: message-impl
XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgSetText);
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgSetText, 1, xiiRTTIDefaultAllocator<xiiMsgSetText>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// END-DOCS-CODE-SNIPPET
// clang-format on
