#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>

namespace
{
  struct xiiMsgTest : public xiiMessage
  {
    XII_DECLARE_MESSAGE_TYPE(xiiMsgTest, xiiMessage);
  };

  // clang-format off
  XII_IMPLEMENT_MESSAGE_TYPE(xiiMsgTest);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMsgTest, 1, xiiRTTIDefaultAllocator<xiiMsgTest>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  struct TestMessage1 : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(TestMessage1, xiiMsgTest);

    int m_iValue;
  };

  struct TestMessage2 : public xiiMsgTest
  {
    XII_DECLARE_MESSAGE_TYPE(TestMessage2, xiiMsgTest);

    virtual xiiInt32 GetSortingKey() const override { return 2; }

    int m_iValue;
  };

  // clang-format off
  XII_IMPLEMENT_MESSAGE_TYPE(TestMessage1);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage1, 1, xiiRTTIDefaultAllocator<TestMessage1>)
  XII_END_DYNAMIC_REFLECTED_TYPE;

  XII_IMPLEMENT_MESSAGE_TYPE(TestMessage2);
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage2, 1, xiiRTTIDefaultAllocator<TestMessage2>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestComponentMsg;
  using TestComponentMsgManager = xiiComponentManager<TestComponentMsg, xiiBlockStorageType::FreeList>;

  class TestComponentMsg : public xiiComponent
  {
    XII_DECLARE_COMPONENT_TYPE(TestComponentMsg, xiiComponent, TestComponentMsgManager);

  public:
    TestComponentMsg()
    {
    }
    ~TestComponentMsg() = default;

    virtual void SerializeComponent(xiiWorldWriter& ref_stream) const override {}
    virtual void DeserializeComponent(xiiWorldReader& ref_stream) override {}

    void OnTestMessage(TestMessage1& ref_msg) { m_iSomeData += ref_msg.m_iValue; }

    void OnTestMessage2(TestMessage2& ref_msg) { m_iSomeData2 += 2 * ref_msg.m_iValue; }

    xiiInt32 m_iSomeData  = 1;
    xiiInt32 m_iSomeData2 = 2;
  };

  // clang-format off
  XII_BEGIN_COMPONENT_TYPE(TestComponentMsg, 1, xiiComponentMode::Static)
  {
    XII_BEGIN_MESSAGEHANDLERS
    {
      XII_MESSAGE_HANDLER(TestMessage1, OnTestMessage),
      XII_MESSAGE_HANDLER(TestMessage2, OnTestMessage2),
    }
    XII_END_MESSAGEHANDLERS;
  }
  XII_END_COMPONENT_TYPE;
  // clang-format on

  void ResetComponents(xiiGameObject& ref_object)
  {
    TestComponentMsg* pComponent = nullptr;
    if (ref_object.TryGetComponentOfBaseType(pComponent))
    {
      pComponent->m_iSomeData  = 1;
      pComponent->m_iSomeData2 = 2;
    }

    for (auto it = ref_object.GetChildren(); it.IsValid(); ++it)
    {
      ResetComponents(*it);
    }
  }
} // namespace

XII_CREATE_SIMPLE_TEST(World, Messaging)
{
  xiiWorldDesc worldDesc("Test");
  xiiWorld     world(worldDesc);
  XII_LOCK(world.GetWriteMarker());

  TestComponentMsgManager* pManager = world.GetOrCreateComponentManager<TestComponentMsgManager>();

  xiiGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  xiiGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  TestComponentMsg* pComponent = nullptr;
  pManager->CreateComponent(pRoot, pComponent);

  xiiGameObject* pParents[2];
  desc.m_hParent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pManager->CreateComponent(pParents[0], pComponent);

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pManager->CreateComponent(pParents[1], pComponent);

  for (xiiUInt32 i = 0; i < 2; ++i)
  {
    desc.m_hParent = pParents[i]->GetHandle();
    for (xiiUInt32 j = 0; j < 4; ++j)
    {
      xiiStringBuilder sb;
      sb.AppendFormat("Parent{0}_Child{1}", i + 1, j + 1);
      desc.m_sName.Assign(sb.GetData());

      xiiGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pManager->CreateComponent(pObject, pComponent);
    }
  }

  // one update step so components are initialized
  world.Update();

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage1 msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2);

    TestComponentMsg* pComponent2 = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent2);
    XII_TEST_INT(pComponent2->m_iSomeData, 5);
    XII_TEST_INT(pComponent2->m_iSomeData2, 10);

    // siblings, parent and children should not be affected
    pParents[1]->TryGetComponentOfBaseType(pComponent2);
    XII_TEST_INT(pComponent2->m_iSomeData, 1);
    XII_TEST_INT(pComponent2->m_iSomeData2, 2);

    pRoot->TryGetComponentOfBaseType(pComponent2);
    XII_TEST_INT(pComponent2->m_iSomeData, 1);
    XII_TEST_INT(pComponent2->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent2);
      XII_TEST_INT(pComponent2->m_iSomeData, 1);
      XII_TEST_INT(pComponent2->m_iSomeData2, 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, xiiTime::Zero(), xiiObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, xiiTime::Zero(), xiiObjectMsgQueueType::NextFrame);
    }

    world.Update();

    TestComponentMsg* pComponent2 = nullptr;
    pRoot->TryGetComponentOfBaseType(pComponent2);
    XII_TEST_INT(pComponent2->m_iSomeData, 46);
    XII_TEST_INT(pComponent2->m_iSomeData2, 92);

    xiiFrameAllocator::Reset();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, xiiTime::Seconds(i + 1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, xiiTime::Seconds(i + 1));
    }

    world.GetClock().SetFixedTimeStep(xiiTime::Seconds(1.001f));

    int iDesiredValue  = 1;
    int iDesiredValue2 = 2;

    for (xiiUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      world.Update();

      TestComponentMsg* pComponent2 = nullptr;
      pRoot->TryGetComponentOfBaseType(pComponent2);
      XII_TEST_INT(pComponent2->m_iSomeData, iDesiredValue);
      XII_TEST_INT(pComponent2->m_iSomeData2, iDesiredValue2);
    }

    xiiFrameAllocator::Reset();
  }
}
