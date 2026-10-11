/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/World/World.h>
#include <Core/World/WorldDesc.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>

namespace
{
  class InitializationTestContext : public xiiEngineProcessDocumentContext
  {
    XII_ADD_DYNAMIC_REFLECTION(InitializationTestContext, xiiEngineProcessDocumentContext);

  public:
    InitializationTestContext() : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::CreateWorld) {}
    explicit InitializationTestContext(xiiWorld& borrowedWorld) : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) { m_pWorld = &borrowedWorld; }

    bool      m_bRejectInitialization = true;
    bool      m_bAcquiredState        = false;
    xiiUInt32 m_uiInitializeCalls     = 0;
    xiiUInt32 m_uiDeinitializeCalls   = 0;

  protected:
    xiiStatus OnInitialize() override
    {
      ++m_uiInitializeCalls;
      XII_TEST_BOOL(GetWorld() != nullptr);
      XII_TEST_BOOL(GetDocumentContext(GetDocumentGuid()) == this);
      m_bAcquiredState = true;
      return m_bRejectInitialization ? xiiStatus("The test context rejected initialization after acquiring state.") : xiiStatus(XII_SUCCESS);
    }
    void OnDeinitialize() override
    {
      ++m_uiDeinitializeCalls;
      XII_TEST_BOOL(GetWorld() != nullptr);
      m_bAcquiredState = false;
    }
    xiiEngineProcessViewContext* CreateViewContext() override { return nullptr; }
    void                         DestroyViewContext(xiiEngineProcessViewContext*) override {}
  };
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(InitializationTestContext, 1, xiiRTTIDefaultAllocator<InitializationTestContext>)
  XII_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, DocumentContextInitialization)
{
  // GetWorldCount includes vacant slots; count live worlds to detect leaked rollback allocations.
  auto liveWorlds = []() {
    xiiUInt32 count = 0;
    for (xiiUInt32 i = 0; i < xiiWorld::GetWorldCount(); ++i)
      count += xiiWorld::GetWorld(i) != nullptr ? 1 : 0;
    return count;
  };
  const auto                 worlds  = liveWorlds();
  const auto                 guid    = xiiUuid::MakeUuid();
  InitializationTestContext* context = xiiGetStaticRTTI<InitializationTestContext>()->GetAllocator()->Allocate<InitializationTestContext>();
  XII_SCOPE_EXIT(if (context) context->GetDynamicRTTI()->GetAllocator()->Deallocate(context));

  auto status = xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, context, nullptr, "Initialization test");
  XII_TEST_BOOL(status.Failed());
  XII_TEST_BOOL(status.GetMessageString().FindSubString("rejected initialization") != nullptr);
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == nullptr);
  XII_TEST_BOOL(context->GetWorld() == nullptr);
  XII_TEST_BOOL(!context->m_bAcquiredState);
  XII_TEST_INT(context->m_uiInitializeCalls, 1);
  XII_TEST_INT(context->m_uiDeinitializeCalls, 1);
  XII_TEST_INT(liveWorlds(), worlds);
  context->Deinitialize();
  XII_TEST_INT(context->m_uiDeinitializeCalls, 1);
  XII_TEST_BOOL(context->Reset().Failed());

  context->m_bRejectInitialization = false;
  XII_TEST_STATUS(xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, context, nullptr, "Initialization test"));
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == context);
  XII_TEST_BOOL(context->m_bAcquiredState);
  XII_TEST_INT(liveWorlds(), worlds + 1);
  auto* world = context->GetWorld();
  XII_TEST_BOOL(context->Initialize(guid, {}, nullptr, "Initialization test").Failed());
  XII_TEST_BOOL(context->GetWorld() == world);

  InitializationTestContext duplicate;
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, &duplicate, nullptr, "Duplicate").Failed());
  XII_TEST_INT(duplicate.m_uiInitializeCalls, 0);
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == context);
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::AddDocumentContext(xiiUuid::MakeUuid(), {}, context, nullptr, "Already initialized").Failed());
  XII_TEST_INT(context->m_uiInitializeCalls, 2);

  // Failed reset releases both the previous world and the newly created partial world.
  context->m_bRejectInitialization = true;
  XII_TEST_BOOL(context->Reset().Failed());
  XII_TEST_BOOL(context->GetWorld() == nullptr);
  XII_TEST_BOOL(!context->m_bAcquiredState);
  XII_TEST_INT(context->m_uiInitializeCalls, 3);
  XII_TEST_INT(context->m_uiDeinitializeCalls, 3);
  XII_TEST_INT(liveWorlds(), worlds);
  xiiEngineProcessDocumentContext::DestroyDocumentContext(guid);
  context = nullptr;
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == nullptr);

  // Rollback must detach from, rather than destroy, a borrowed world as used by scene layers.
  xiiWorldDescription       description("Borrowed document rollback");
  xiiWorld                  borrowedWorld(description);
  InitializationTestContext borrower(borrowedWorld);
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::AddDocumentContext(guid, {}, &borrower, nullptr, "Borrower").Failed());
  XII_TEST_BOOL(borrower.GetWorld() == nullptr);
  XII_TEST_INT(borrower.m_uiDeinitializeCalls, 1);
  XII_TEST_INT(liveWorlds(), worlds + 1);
  XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(guid) == nullptr);
}
