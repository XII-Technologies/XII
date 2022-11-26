#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/MessageQueue.h>

namespace
{
  struct xiiMsgTest : public xiiMessage
  {
    XII_DECLARE_MESSAGE_TYPE(xiiMsgTest, xiiMessage);
  };

  XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTest);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTest, 1, xiiRTTIDefaultAllocator<xiiMsgTest>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  struct TestMessage : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(TestMessage, xiiMsgTest);

    int x;
    int y;
  };

  struct MetaData
  {
    int receiver;
  };

  typedef xiiMessageQueue<MetaData> TestMessageQueue;

  XII_IMPLEMENT_MESSAGE_TYPE(TestMessage);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage, 1, xiiRTTIDefaultAllocator<TestMessage>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

XII_CREATE_SIMPLE_TEST(Communication, MessageQueue)
{
  {
    TestMessage msg;
    XII_TEST_INT(msg.GetSize(), sizeof(TestMessage));
  }

  TestMessageQueue q;

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enqueue")
  {
    for (xiiUInt32 i = 0; i < 100; ++i)
    {
      TestMessage* pMsg = XII_DEFAULT_NEW(TestMessage);
      pMsg->x           = rand();
      pMsg->y           = rand();

      MetaData md;
      md.receiver = rand() % 10;

      q.Enqueue(pMsg, md);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sorting")
  {
    struct MessageComparer
    {
      bool Less(const TestMessageQueue::Entry& a, const TestMessageQueue::Entry& b) const
      {
        if (a.m_MetaData.receiver != b.m_MetaData.receiver)
          return a.m_MetaData.receiver < b.m_MetaData.receiver;

        return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
      }
    };

    q.Sort(MessageComparer());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "operator[]")
  {
    XII_LOCK(q);

    xiiMessage* pLastMsg = q[0].m_pMessage;
    MetaData    lastMd   = q[0].m_MetaData;

    for (xiiUInt32 i = 1; i < q.GetCount(); ++i)
    {
      xiiMessage* pMsg = q[i].m_pMessage;
      MetaData    md   = q[i].m_MetaData;

      if (md.receiver == lastMd.receiver)
      {
        XII_TEST_BOOL(pMsg->GetHash() >= pLastMsg->GetHash());
      }
      else
      {
        XII_TEST_BOOL(md.receiver >= lastMd.receiver);
      }
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Dequeue")
  {
    xiiMessage* pMsg = nullptr;
    MetaData    md;

    while (q.TryDequeue(pMsg, md))
    {
      XII_DEFAULT_DELETE(pMsg);
    }
  }
}
