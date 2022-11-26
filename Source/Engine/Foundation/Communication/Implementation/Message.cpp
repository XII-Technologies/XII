#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Message.h>

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMessage, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

// clang-format on

xiiMessageId xiiMessage::s_NextMsgId = 0;


void xiiMessage::PackageForTransfer(const xiiMessage& msg, xiiStreamWriter& stream)
{
  const xiiRTTI* pRtti = msg.GetDynamicRTTI();

  stream << pRtti->GetTypeNameHash();
  stream << (xiiUInt8)pRtti->GetTypeVersion();

  msg.Serialize(stream);
}

xiiUniquePtr<xiiMessage> xiiMessage::ReplicatePackedMessage(xiiStreamReader& stream)
{
  xiiUInt64 uiTypeHash = 0;
  stream >> uiTypeHash;

  xiiUInt8 uiTypeVersion = 0;
  stream >> uiTypeVersion;

  static xiiHashTable<xiiUInt64, const xiiRTTI*, xiiHashHelper<xiiUInt64>, xiiStaticAllocatorWrapper> MessageTypes;

  const xiiRTTI* pRtti = nullptr;
  if (!MessageTypes.TryGetValue(uiTypeHash, pRtti))
  {
    for (pRtti = xiiRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
    {
      if (pRtti->GetTypeNameHash() == uiTypeHash)
      {
        MessageTypes[uiTypeHash] = pRtti;
        break;
      }
    }
  }

  if (pRtti == nullptr || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pMsg = pRtti->GetAllocator()->Allocate<xiiMessage>();

  pMsg->Deserialize(stream, uiTypeVersion);

  return pMsg;
}

XII_STATICLINK_FILE(Foundation, Foundation_Communication_Implementation_Message);
