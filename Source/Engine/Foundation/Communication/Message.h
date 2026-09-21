/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

using xiiMessageId = xiiUInt16;
class xiiStreamWriter;
class xiiStreamReader;

/// Base class for all message types. Each message type has it's own id which is used to dispatch messages efficiently.
///
/// To implement a custom message type derive from xiiMessage and add XII_DECLARE_MESSAGE_TYPE to the type declaration.
/// XII_IMPLEMENT_MESSAGE_TYPE needs to be added to a cpp.
/// \see xiiRTTI
///
/// For the automatic cloning to work and for efficiency the messages must only contain simple data members.
/// For instance, everything that allocates internally (strings, arrays) should be avoided.
/// Instead, such objects should be located somewhere else and the message should only contain pointers to the data.
///
class XII_FOUNDATION_DLL xiiMessage : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMessage, xiiReflectedClass);

protected:
  explicit xiiMessage(size_t messageSize)
  {
    const auto sizeOffset = (reinterpret_cast<uintptr_t>(&m_Id) - reinterpret_cast<uintptr_t>(this)) + sizeof(m_Id);
    memset((void*)xiiMemoryUtils::AddByteOffset(this, sizeOffset), 0, messageSize - sizeOffset);
    m_uiSize = static_cast<xiiUInt16>(messageSize);
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    m_uiDebugMessageRouting = 0;
#endif
  }

public:
  XII_ALWAYS_INLINE xiiMessage() :
    xiiMessage(sizeof(xiiMessage))
  {
  }

  virtual ~xiiMessage() = default;

  /// Derived message types can override this method to influence sorting order. Smaller keys are processed first.
  virtual xiiInt32 GetSortingKey() const { return 0; }

  /// Returns the id for this message type.
  XII_ALWAYS_INLINE xiiMessageId GetId() const { return m_Id; }

  /// Returns the size in byte of this message.
  XII_ALWAYS_INLINE xiiUInt16 GetSize() const { return m_uiSize; }

  /// Calculates a hash of the message.
  XII_ALWAYS_INLINE xiiUInt64 GetHash() const { return xiiHashingUtils::xxHash64(this, m_uiSize); }

  /// Implement this for efficient transmission across process boundaries (e.g. network transfer etc.)
  ///
  /// If the message is only ever sent within the same process between nodes of the same xiiWorld,
  /// this does not need to be implemented.
  ///
  /// Note that PackageForTransfer() will automatically include the xiiRTTI type version into the stream
  /// and ReplicatePackedMessage() will pass this into Deserialize(). Use this if the serialization changes.
  virtual void Serialize(xiiStreamWriter& ref_stream) const
  {
    XII_IGNORE_UNUSED(ref_stream);

    XII_ASSERT_NOT_IMPLEMENTED;
  }

  /// \see Serialize()
  virtual void Deserialize(xiiStreamReader& ref_stream, xiiUInt8 uiTypeVersion)
  {
    XII_IGNORE_UNUSED(ref_stream);
    XII_IGNORE_UNUSED(uiTypeVersion);

    XII_ASSERT_NOT_IMPLEMENTED;
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  /// set to true while debugging a message routing problem
  /// if the message is not delivered to any recipient at all, information about why that is will be written to xiiLog
  XII_ALWAYS_INLINE void SetDebugMessageRouting(bool bDebug) { m_uiDebugMessageRouting = bDebug; }

  XII_ALWAYS_INLINE bool GetDebugMessageRouting() const { return m_uiDebugMessageRouting; }
#endif

protected:
  XII_ALWAYS_INLINE static xiiMessageId GetNextMsgId() { return s_NextMsgId++; }

  xiiMessageId m_Id;

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  xiiUInt16 m_uiSize : 15;
  xiiUInt16 m_uiDebugMessageRouting : 1;
#else
  xiiUInt16 m_uiSize;
#endif

  static xiiMessageId s_NextMsgId;


  //////////////////////////////////////////////////////////////////////////
  // Transferring and replicating messages
  //

public:
  /// Writes msg to stream in such a way that ReplicatePackedMessage() can restore it even in another process
  ///
  /// For this to work the message type has to have the Serialize and Deserialize functions implemented.
  ///
  /// \note This is NOT used by xiiWorld. Within the same process messages can be dispatched more efficiently.
  static void PackageForTransfer(const xiiMessage& msg, xiiStreamWriter& ref_stream);

  /// Restores a message that was written by PackageForTransfer()
  ///
  /// If the message type is unknown, nullptr is returned.
  /// \see PackageForTransfer()
  static xiiUniquePtr<xiiMessage> ReplicatePackedMessage(xiiStreamReader& ref_stream);

private:
};

/// Add this macro to the declaration of your custom message type.
#define XII_DECLARE_MESSAGE_TYPE(messageType, baseType)                              \
private:                                                                             \
  XII_ADD_DYNAMIC_REFLECTION(messageType, baseType);                                 \
  static xiiMessageId MSG_ID;                                                        \
                                                                                     \
protected:                                                                           \
  XII_ALWAYS_INLINE explicit messageType(size_t messageSize) : baseType(messageSize) \
  {                                                                                  \
    m_Id = messageType::MSG_ID;                                                      \
  }                                                                                  \
                                                                                     \
public:                                                                              \
  static xiiMessageId GetTypeMsgId()                                                 \
  {                                                                                  \
    static xiiMessageId id = xiiMessage::GetNextMsgId();                             \
    return id;                                                                       \
  }                                                                                  \
                                                                                     \
  XII_ALWAYS_INLINE messageType() : messageType(sizeof(messageType))                 \
  {                                                                                  \
  }

/// Implements the given message type. Add this macro to a cpp outside of the type declaration.
#define XII_IMPLEMENT_MESSAGE_TYPE(messageType) xiiMessageId messageType::MSG_ID = messageType::GetTypeMsgId();

/// Base class for all message senders.
template <typename T>
struct xiiMessageSenderBase
{
  using MessageType = T;
};
