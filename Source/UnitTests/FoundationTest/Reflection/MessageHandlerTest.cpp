#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Reflection/Reflection.h>

#ifdef GetMessage
#  undef GetMessage
#endif

namespace
{
  struct xiiMsgTest : public xiiMessage
  {
    XII_DECLARE_MESSAGE_TYPE(xiiMsgTest, xiiMessage);
  };

  XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTest);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTest, 1, xiiRTTIDefaultAllocator<xiiMsgTest>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  struct AddMessage : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(AddMessage, xiiMsgTest);

    xiiInt32 m_iValue;
  };
  XII_IMPLEMENT_MESSAGE_TYPE(AddMessage);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(AddMessage, 1, xiiRTTIDefaultAllocator<AddMessage>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  struct SubMessage : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(SubMessage, xiiMsgTest);

    xiiInt32 m_iValue;
  };
  XII_IMPLEMENT_MESSAGE_TYPE(SubMessage);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(SubMessage, 1, xiiRTTIDefaultAllocator<SubMessage>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  struct MulMessage : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(MulMessage, xiiMsgTest);

    xiiInt32 m_iValue;
  };
  XII_IMPLEMENT_MESSAGE_TYPE(MulMessage);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(MulMessage, 1, xiiRTTIDefaultAllocator<MulMessage>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  struct GetMessage : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(GetMessage, xiiMsgTest);

    xiiInt32 m_iValue;
  };
  XII_IMPLEMENT_MESSAGE_TYPE(GetMessage);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(GetMessage, 1, xiiRTTIDefaultAllocator<GetMessage>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

class BaseHandler : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(BaseHandler, xiiReflectedClass);

public:
  BaseHandler() = default;

  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue; }

  void OnMulMessage(MulMessage& ref_msg) { m_iValue *= ref_msg.m_iValue; }

  void OnGetMessage(GetMessage& ref_msg) const { ref_msg.m_iValue = m_iValue; }

  xiiInt32 m_iValue = 0;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(BaseHandler, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_MESSAGEHANDLERS{
      XII_MESSAGE_HANDLER(AddMessage, OnAddMessage),
      XII_MESSAGE_HANDLER(MulMessage, OnMulMessage),
      XII_MESSAGE_HANDLER(GetMessage, OnGetMessage),
  } XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class DerivedHandler : public BaseHandler
{
  XII_ADD_DYNAMIC_REFLECTION(DerivedHandler, BaseHandler);

public:
  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue * 2; }

  void OnSubMessage(SubMessage& ref_msg) { m_iValue -= ref_msg.m_iValue; }
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedHandler, 1, xiiRTTINoAllocator)
{
  XII_BEGIN_MESSAGEHANDLERS
  {
    XII_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    XII_MESSAGE_HANDLER(SubMessage, OnSubMessage),
  }
  XII_END_MESSAGEHANDLERS;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_CREATE_SIMPLE_TEST(Reflection, MessageHandler)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Dispatch")
  {
    BaseHandler    test;
    const xiiRTTI* pRTTI = test.GetStaticRTTI();

    XII_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    XII_TEST_BOOL(!pRTTI->CanHandleMessage<SubMessage>());
    XII_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());
    XII_TEST_BOOL(pRTTI->CanHandleMessage<GetMessage>());

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled    = pRTTI->DispatchMessage(&test, addMsg);
    XII_TEST_BOOL(handled);

    XII_TEST_INT(test.m_iValue, 4);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled         = pRTTI->DispatchMessage(&test, subMsg); // should do nothing
    XII_TEST_BOOL(!handled);

    XII_TEST_INT(test.m_iValue, 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Simple Dispatch const")
  {
    const BaseHandler test;
    const xiiRTTI*    pRTTI = test.GetStaticRTTI();

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled    = pRTTI->DispatchMessage(&test, addMsg);
    XII_TEST_BOOL(!handled); // should do nothing since object is const and the add message handler is non-const

    XII_TEST_INT(test.m_iValue, 0);

    GetMessage getMsg;
    getMsg.m_iValue = 12;
    handled         = pRTTI->DispatchMessage(&test, getMsg);
    XII_TEST_BOOL(handled);
    XII_TEST_INT(getMsg.m_iValue, 0);

    XII_TEST_INT(test.m_iValue, 0); // object must not be modified
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dispatch with inheritance")
  {
    DerivedHandler test;
    const xiiRTTI* pRTTI = test.GetStaticRTTI();

    XII_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    XII_TEST_BOOL(pRTTI->CanHandleMessage<SubMessage>());
    XII_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());

    // message handler overridden by derived class
    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled    = pRTTI->DispatchMessage(&test, addMsg);
    XII_TEST_BOOL(handled);

    XII_TEST_INT(test.m_iValue, 8);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled         = pRTTI->DispatchMessage(&test, subMsg);
    XII_TEST_BOOL(handled);

    XII_TEST_INT(test.m_iValue, 4);

    // message handled by base class
    MulMessage mulMsg;
    mulMsg.m_iValue = 4;
    handled         = pRTTI->DispatchMessage(&test, mulMsg);
    XII_TEST_BOOL(handled);

    XII_TEST_INT(test.m_iValue, 16);
  }
}
