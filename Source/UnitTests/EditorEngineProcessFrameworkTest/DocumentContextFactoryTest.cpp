/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessDocumentContext.h>
#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

namespace
{
  class FactoryTestContext : public xiiEngineProcessDocumentContext
  {
    XII_ADD_DYNAMIC_REFLECTION(FactoryTestContext, xiiEngineProcessDocumentContext);

  public:
    FactoryTestContext() : xiiEngineProcessDocumentContext(xiiEngineProcessDocumentContextFlags::None) { ++s_uiAllocations; }
    ~FactoryTestContext() { --s_uiAllocations; }
    static xiiUInt32 s_uiAllocations;

  protected:
    xiiEngineProcessViewContext* CreateViewContext() override { return nullptr; }
    void                         DestroyViewContext(xiiEngineProcessViewContext*) override {}
  };
  xiiUInt32 FactoryTestContext::s_uiAllocations = 0;

  class DefaultFactoryTestContext : public FactoryTestContext
  {
    XII_ADD_DYNAMIC_REFLECTION(DefaultFactoryTestContext, FactoryTestContext);
  };

  class CustomFactoryTestContext : public FactoryTestContext
  {
    XII_ADD_DYNAMIC_REFLECTION(CustomFactoryTestContext, FactoryTestContext);

  public:
    static xiiEngineProcessDocumentContext* AllocateContext(const xiiDocumentOpenMsgToEngine* request)
    {
      // A custom allocator can select another derived type based on request metadata (as scene layers do).
      if (request->m_DocumentMetaData.IsA<xiiInt32>() && request->m_DocumentMetaData.Get<xiiInt32>() == 42)
        return xiiGetStaticRTTI<DefaultFactoryTestContext>()->GetAllocator()->Allocate<xiiEngineProcessDocumentContext>();
      return nullptr;
    }
  };

  class InvalidFactoryTestContext : public FactoryTestContext
  {
    XII_ADD_DYNAMIC_REFLECTION(InvalidFactoryTestContext, FactoryTestContext);

  public:
    static xiiInt32 AllocateContext()
    {
      XII_TEST_BOOL(false);
      return 0;
    }
  };

  class DuplicateFactoryTestContext : public FactoryTestContext
  {
    XII_ADD_DYNAMIC_REFLECTION(DuplicateFactoryTestContext, FactoryTestContext);
  };

  class UnallocatableFactoryTestContext : public FactoryTestContext
  {
    XII_ADD_DYNAMIC_REFLECTION(UnallocatableFactoryTestContext, FactoryTestContext);
  };

  // clang-format off
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(FactoryTestContext, 1, xiiRTTINoAllocator)
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(DefaultFactoryTestContext, 1, xiiRTTIDefaultAllocator<DefaultFactoryTestContext>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_CONSTANT_PROPERTY("DocumentType", (const char*) "__FactoryTest;__FactoryAlias;__FactoryDuplicate"),
    }
    XII_END_PROPERTIES;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(CustomFactoryTestContext, 1, xiiRTTIDefaultAllocator<CustomFactoryTestContext>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_CONSTANT_PROPERTY("DocumentType", (const char*) "__FactoryCustom"),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_FUNCTIONS
    {
      XII_FUNCTION_PROPERTY(AllocateContext),
    }
    XII_END_FUNCTIONS;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(InvalidFactoryTestContext, 1, xiiRTTIDefaultAllocator<InvalidFactoryTestContext>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_CONSTANT_PROPERTY("DocumentType", (const char*) "__FactoryInvalid"),
    }
    XII_END_PROPERTIES;
    XII_BEGIN_FUNCTIONS
    {
      XII_FUNCTION_PROPERTY(AllocateContext),
    }
    XII_END_FUNCTIONS;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(DuplicateFactoryTestContext, 1, xiiRTTIDefaultAllocator<DuplicateFactoryTestContext>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_CONSTANT_PROPERTY("DocumentType", (const char*) "__FactoryDuplicate"),
    }
    XII_END_PROPERTIES;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(UnallocatableFactoryTestContext, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_CONSTANT_PROPERTY("DocumentType", (const char*) "__FactoryAbstract"),
    }
    XII_END_PROPERTIES;
  }
  XII_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on
} // namespace

XII_CREATE_SIMPLE_TEST(EditorViews, DocumentContextFactory)
{
  xiiDocumentOpenMsgToEngine request;
  request.m_DocumentGuid                   = xiiUuid::MakeUuid();
  request.m_bDocumentOpen                  = true;
  xiiEngineProcessDocumentContext* context = nullptr;
  auto                             reject  = [&](xiiStringView type) {
    request.m_sDocumentType = type;
    const xiiStatus status  = xiiEngineProcessDocumentContext::AllocateDocumentContext(request, context);
    XII_TEST_BOOL(status.Failed());
    XII_TEST_BOOL(!status.GetMessageString().IsEmpty());
    XII_TEST_BOOL(context == nullptr);
    XII_TEST_INT(FactoryTestContext::s_uiAllocations, 0);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(request.m_DocumentGuid) == nullptr);
  };
  reject("");
  reject("__FactoryMissing");
  reject("FactoryTest"); // exact token matching, not substring matching
  reject("__FactoryTest;__FactoryAlias");
  reject("__FactoryDuplicate");
  reject("__FactoryInvalid");
  reject("__FactoryAbstract");
  reject("__FactoryCustom"); // custom rejection must not fall back to the default allocator

  request.m_bDocumentOpen = false;
  reject("__FactoryTest");
  request.m_bDocumentOpen = true;
  request.m_DocumentGuid  = xiiUuid();
  reject("__FactoryTest");
  request.m_DocumentGuid = xiiUuid::MakeUuid();

  for (xiiStringView type : {xiiStringView("__FactoryTest"), xiiStringView("__FactoryAlias"), xiiStringView("__FactoryCustom")})
  {
    request.m_sDocumentType    = type;
    request.m_DocumentMetaData = 42;
    XII_TEST_STATUS(xiiEngineProcessDocumentContext::AllocateDocumentContext(request, context));
    if (!XII_TEST_BOOL(context != nullptr))
      continue;
    XII_TEST_BOOL(context->GetDynamicRTTI() == xiiGetStaticRTTI<DefaultFactoryTestContext>());
    XII_TEST_INT(FactoryTestContext::s_uiAllocations, 1);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(request.m_DocumentGuid) == nullptr);
    xiiEngineProcessDocumentContext::AddDocumentContext(request.m_DocumentGuid, request.m_DocumentMetaData, context, nullptr, type);
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(request.m_DocumentGuid) == context);
    XII_TEST_STRING(context->GetDocumentType(), type);
    context->Reset();
    XII_TEST_BOOL(xiiEngineProcessDocumentContext::GetDocumentContext(request.m_DocumentGuid) == context);
    xiiEngineProcessDocumentContext::DestroyDocumentContext(request.m_DocumentGuid);
    context = nullptr;
    XII_TEST_INT(FactoryTestContext::s_uiAllocations, 0);
  }

  for (bool succeeded : {false, true})
  {
    xiiDocumentOpenResponseMsgToEditor response;
    XII_TEST_BOOL(!response.m_bSucceeded);
    response.m_DocumentGuid = request.m_DocumentGuid;
    response.m_bSucceeded   = succeeded;
    if (!succeeded)
      response.m_sFailure = "No context is registered for the requested document type.";
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToBinary(writer, response.GetDynamicRTTI(), &response);
    xiiMemoryStreamReader              reader(&storage);
    xiiDocumentOpenResponseMsgToEditor restored;
    xiiReflectionSerializer::ReadObjectPropertiesFromBinary(reader, *restored.GetDynamicRTTI(), &restored);
    XII_TEST_BOOL(restored.m_DocumentGuid == response.m_DocumentGuid);
    XII_TEST_BOOL(restored.m_bSucceeded == succeeded);
    XII_TEST_STRING(restored.m_sFailure, response.m_sFailure);
  }
}
