#include <ToolsFoundationTest/ToolsFoundationTestPCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>
#include <ToolsFoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST_GROUP(Versioning);

struct xiiPatchTestBase
{
public:
  xiiPatchTestBase()
  {
    m_string  = "Base";
    m_string2 = "";
  }

  xiiString m_string;
  xiiString m_string2;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiPatchTestBase);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPatchTestBase, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPatchTestBase>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("String", m_string),
    XII_MEMBER_PROPERTY("String2", m_string2),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

struct xiiPatchTest : public xiiPatchTestBase
{
public:
  xiiPatchTest() { m_iInt32 = 1; }

  xiiInt32 m_iInt32;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiPatchTest);

// clang-format off
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPatchTest, xiiPatchTestBase, 1, xiiRTTIDefaultAllocator<xiiPatchTest>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Int", m_iInt32),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on

namespace
{
  /// Patch class
  class xiiPatchTestP : public xiiGraphPatch
  {
  public:
    xiiPatchTestP() :
      xiiGraphPatch("xiiPatchTestP", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      pNode->RenameProperty("Int", "IntRenamed");
      pNode->ChangeProperty("IntRenamed", 2);
    }
  };
  xiiPatchTestP g_xiiPatchTestP;

  /// Patch base class
  class xiiPatchTestBaseBP : public xiiGraphPatch
  {
  public:
    xiiPatchTestBaseBP() :
      xiiGraphPatch("xiiPatchTestBaseBP", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String", "BaseClassPatched");
    }
  };
  xiiPatchTestBaseBP g_xiiPatchTestBaseBP;

  /// Rename class
  class xiiPatchTestRN : public xiiGraphPatch
  {
  public:
    xiiPatchTestRN() :
      xiiGraphPatch("xiiPatchTestRN", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      context.RenameClass("xiiPatchTestRN2");
      pNode->ChangeProperty("String", "RenameExecuted");
    }
  };
  xiiPatchTestRN g_xiiPatchTestRN;

  /// Patch renamed class to v3
  class xiiPatchTestRN2 : public xiiGraphPatch
  {
  public:
    xiiPatchTestRN2() :
      xiiGraphPatch("xiiPatchTestRN2", 3)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      pNode->ChangeProperty("String2", "Patched");
    }
  };
  xiiPatchTestRN2 g_xiiPatchTestRN2;

  /// Change base class
  class xiiPatchTestCB : public xiiGraphPatch
  {
  public:
    xiiPatchTestCB() :
      xiiGraphPatch("xiiPatchTestCB", 2)
    {
    }
    virtual void Patch(xiiGraphPatchContext& context, xiiAbstractObjectGraph* pGraph, xiiAbstractObjectNode* pNode) const override
    {
      xiiVersionKey bases[] = {{"xiiPatchTestBaseBP", 1}};
      context.ChangeBaseClass(bases);
      pNode->ChangeProperty("String2", "ChangedBase");
    }
  };
  xiiPatchTestCB g_xiiPatchTestCB;

  void ReplaceTypeName(xiiAbstractObjectGraph& graph, xiiAbstractObjectGraph& typesGraph, const char* szOldName, const char* szNewName)
  {
    for (auto it : graph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if (xiiStringUtils::IsEqual(szOldName, pNode->GetType()))
        pNode->SetType(szNewName);
    }

    for (auto it : typesGraph.GetAllNodes())
    {
      auto* pNode = it.Value();

      if (xiiStringUtils::IsEqual("xiiReflectedTypeDescriptor", pNode->GetType()))
      {
        if (auto* pProp = pNode->FindProperty("TypeName"))
        {
          if (xiiStringUtils::IsEqual(szOldName, pProp->m_Value.Get<xiiString>()))
            pProp->m_Value = szNewName;
        }
        if (auto* pProp = pNode->FindProperty("ParentTypeName"))
        {
          if (xiiStringUtils::IsEqual(szOldName, pProp->m_Value.Get<xiiString>()))
            pProp->m_Value = szNewName;
        }
      }
    }
  }

  xiiAbstractObjectNode* SerializeObject(xiiAbstractObjectGraph& graph, xiiAbstractObjectGraph& typesGraph, const xiiRTTI* pRtti, void* pObject)
  {
    xiiAbstractObjectNode* pNode = nullptr;
    {
      // Object
      xiiRttiConverterContext context;
      xiiRttiConverterWriter  rttiConverter(&graph, &context, true, true);
      context.RegisterObject(xiiUuid::StableUuidForString(pRtti->GetTypeName()), pRtti, pObject);
      pNode = rttiConverter.AddObjectToGraph(pRtti, pObject, "ROOT");
    }
    {
      // Types
      xiiSet<const xiiRTTI*> types;
      types.Insert(pRtti);
      xiiReflectionUtils::GatherDependentTypes(pRtti, types);
      xiiToolsSerializationUtils::SerializeTypes(types, typesGraph);
    }
    return pNode;
  }

  void PatchGraph(xiiAbstractObjectGraph& graph, xiiAbstractObjectGraph& typesGraph)
  {
    xiiGraphVersioning::GetSingleton()->PatchGraph(&typesGraph);
    xiiGraphVersioning::GetSingleton()->PatchGraph(&graph, &typesGraph);
  }
} // namespace

XII_CREATE_SIMPLE_TEST(Versioning, GraphPatch)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PatchClass")
  {
    xiiAbstractObjectGraph graph;
    xiiAbstractObjectGraph typesGraph;

    xiiPatchTest data;
    data.m_iInt32                = 5;
    xiiAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, xiiGetStaticRTTI<xiiPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "xiiPatchTest", "xiiPatchTestP");
    PatchGraph(graph, typesGraph);

    xiiAbstractObjectNode::Property* pInt = pNode->FindProperty("IntRenamed");
    XII_TEST_INT(2, pInt->m_Value.Get<xiiInt32>());
    XII_TEST_BOOL(pNode->FindProperty("Int") == nullptr);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PatchBaseClass")
  {
    xiiAbstractObjectGraph graph;
    xiiAbstractObjectGraph typesGraph;

    xiiPatchTest data;
    data.m_string                = "Unpatched";
    xiiAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, xiiGetStaticRTTI<xiiPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "xiiPatchTestBase", "xiiPatchTestBaseBP");
    PatchGraph(graph, typesGraph);

    xiiAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    XII_TEST_STRING(pString->m_Value.Get<xiiString>(), "BaseClassPatched");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RenameClass")
  {
    xiiAbstractObjectGraph graph;
    xiiAbstractObjectGraph typesGraph;

    xiiPatchTest data;
    data.m_string                = "NotRenamed";
    xiiAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, xiiGetStaticRTTI<xiiPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "xiiPatchTest", "xiiPatchTestRN");
    PatchGraph(graph, typesGraph);

    xiiAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    XII_TEST_BOOL(pString->m_Value.Get<xiiString>() == "RenameExecuted");
    XII_TEST_STRING(pNode->GetType(), "xiiPatchTestRN2");
    XII_TEST_INT(pNode->GetTypeVersion(), 3);
    xiiAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    XII_TEST_BOOL(pString2->m_Value.Get<xiiString>() == "Patched");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ChangeBaseClass")
  {
    xiiAbstractObjectGraph graph;
    xiiAbstractObjectGraph typesGraph;

    xiiPatchTest data;
    data.m_string                = "NotPatched";
    xiiAbstractObjectNode* pNode = SerializeObject(graph, typesGraph, xiiGetStaticRTTI<xiiPatchTest>(), &data);
    ReplaceTypeName(graph, typesGraph, "xiiPatchTest", "xiiPatchTestCB");
    PatchGraph(graph, typesGraph);

    xiiAbstractObjectNode::Property* pString = pNode->FindProperty("String");
    XII_TEST_STRING(pString->m_Value.Get<xiiString>(), "BaseClassPatched");
    XII_TEST_INT(pNode->GetTypeVersion(), 2);
    xiiAbstractObjectNode::Property* pString2 = pNode->FindProperty("String2");
    XII_TEST_STRING(pString2->m_Value.Get<xiiString>(), "ChangedBase");
  }
}
