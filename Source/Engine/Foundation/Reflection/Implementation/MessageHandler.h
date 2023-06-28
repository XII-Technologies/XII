#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

class xiiMessage;

/// \brief The base class for all message handlers that a type provides.
class XII_FOUNDATION_DLL xiiAbstractMessageHandler
{
public:
  virtual ~xiiAbstractMessageHandler() = default;

  XII_ALWAYS_INLINE void operator()(void* pInstance, xiiMessage& ref_msg) { (*m_DispatchFunc)(pInstance, ref_msg); }

  XII_FORCE_INLINE void operator()(const void* pInstance, xiiMessage& ref_msg)
  {
    XII_ASSERT_DEV(m_bIsConst, "Calling a non const message handler with a const instance.");
    (*m_ConstDispatchFunc)(pInstance, ref_msg);
  }

  XII_ALWAYS_INLINE xiiMessageId GetMessageId() const { return m_Id; }

  XII_ALWAYS_INLINE bool IsConst() const { return m_bIsConst; }

protected:
  using DispatchFunc      = void (*)(void*, xiiMessage&);
  using ConstDispatchFunc = void (*)(const void*, xiiMessage&);

  union
  {
    DispatchFunc      m_DispatchFunc;
    ConstDispatchFunc m_ConstDispatchFunc;
  };
  xiiMessageId m_Id;
  bool         m_bIsConst;
};

struct xiiMessageSenderInfo
{
  const char*    m_szName;
  const xiiRTTI* m_pMessageType;
};

namespace xiiInternal
{
  template <typename Class, typename MessageType>
  struct MessageHandlerTraits
  {
    static xiiCompileTimeTrueType  IsConst(void (Class::*)(MessageType&) const);
    static xiiCompileTimeFalseType IsConst(...);
  };

  template <bool bIsConst>
  struct MessageHandler
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&)>
    class Impl : public xiiAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_DispatchFunc = &Dispatch;
        m_Id           = MessageType::GetTypeMsgId();
        m_bIsConst     = false;
      }

      static void Dispatch(void* pInstance, xiiMessage& ref_msg)
      {
        Class* pTargetInstance = static_cast<Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };

  template <>
  struct MessageHandler<true>
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&) const>
    class Impl : public xiiAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_ConstDispatchFunc = &Dispatch;
        m_Id                = MessageType::GetTypeMsgId();
        m_bIsConst          = true;
      }

      /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
      static void Dispatch(const void* pInstance, xiiMessage& ref_msg)
      {
        const Class* pTargetInstance = static_cast<const Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };
} // namespace xiiInternal

#define XII_IS_CONST_MESSAGE_HANDLER(Class, MessageType, Method) \
  (sizeof(xiiInternal::MessageHandlerTraits<Class, MessageType>::IsConst(Method)) == sizeof(xiiCompileTimeTrueType))
