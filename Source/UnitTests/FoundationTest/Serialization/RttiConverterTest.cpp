#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <FoundationTest/Reflection/ReflectionTestClasses.h>

XII_CREATE_SIMPLE_TEST_GROUP(Serialization);

class TestContext : public xiiRttiConverterContext
{
public:
  virtual xiiInternal::NewInstance<void> CreateObject(const xiiUuid& guid, const xiiRTTI* pRtti) override
  {
    auto pObj = pRtti->GetAllocator()->Allocate<void>();
    RegisterObject(guid, pRtti, pObj);
    return pObj;
  }

  virtual void DeleteObject(const xiiUuid& guid) override
  {
    auto object = GetObjectByGUID(guid);
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);

    UnregisterObject(guid);
  }
};

template <typename T>
void TestSerialize(T* pObject)
{
  xiiAbstractObjectGraph graph;
  TestContext            context;
  xiiRttiConverterWriter conv(&graph, &context, true, true);

  const xiiRTTI* pRtti = xiiGetStaticRTTI<T>();
  xiiUuid        guid;
  guid.CreateNewUuid();

  context.RegisterObject(guid, pRtti, pObject);
  xiiAbstractObjectNode* pNode = conv.AddObjectToGraph(pRtti, pObject, "root");

  XII_TEST_BOOL(pNode->GetGuid() == guid);
  XII_TEST_STRING(pNode->GetType(), pRtti->GetTypeName());
  XII_TEST_INT(pNode->GetProperties().GetCount(), pNode->GetProperties().GetCount());

  {
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiMemoryStreamReader            reader(&storage);

    xiiAbstractGraphDdlSerializer::Write(writer, &graph);

    xiiStringBuilder sData, sData2;
    sData.SetSubString_ElementCount((const char*)storage.GetData(), storage.GetStorageSize32());


    xiiRttiConverterReader convRead(&graph, &context);
    auto*                  pRootNode = graph.GetNodeByName("root");
    XII_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    XII_TEST_BOOL(target == *pObject);

    // Overwrite again to test for leaks as existing values have to be removed first by xiiRttiConverterReader.
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    XII_TEST_BOOL(target == *pObject);

    {
      T clone;
      xiiReflectionSerializer::Clone(pObject, &clone, pRtti);
      XII_TEST_BOOL(clone == *pObject);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&clone, pObject, pRtti));
    }

    {
      T* pClone = xiiReflectionSerializer::Clone(pObject);
      XII_TEST_BOOL(*pClone == *pObject);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(pClone, pObject));
      // Overwrite again to test for leaks as existing values have to be removed first by clone.
      xiiReflectionSerializer::Clone(pObject, pClone, pRtti);
      XII_TEST_BOOL(*pClone == *pObject);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(pClone, pObject, pRtti));
      pRtti->GetAllocator()->Deallocate(pClone);
    }

    xiiAbstractObjectGraph graph2;
    xiiAbstractGraphDdlSerializer::Read(reader, &graph2).IgnoreResult();

    xiiContiguousMemoryStreamStorage storage2;
    xiiMemoryStreamWriter            writer2(&storage2);

    xiiAbstractGraphDdlSerializer::Write(writer2, &graph2);
    sData2.SetSubString_ElementCount((const char*)storage2.GetData(), storage2.GetStorageSize32());

    XII_TEST_BOOL(sData == sData2);
  }

  {
    xiiContiguousMemoryStreamStorage storage;
    xiiMemoryStreamWriter            writer(&storage);
    xiiMemoryStreamReader            reader(&storage);

    xiiAbstractGraphBinarySerializer::Write(writer, &graph);

    xiiRttiConverterReader convRead(&graph, &context);
    auto*                  pRootNode = graph.GetNodeByName("root");
    XII_TEST_BOOL(pRootNode != nullptr);

    T target;
    convRead.ApplyPropertiesToObject(pRootNode, pRtti, &target);
    XII_TEST_BOOL(target == *pObject);

    xiiAbstractObjectGraph graph2;
    xiiAbstractGraphBinarySerializer::Read(reader, &graph2);

    xiiContiguousMemoryStreamStorage storage2;
    xiiMemoryStreamWriter            writer2(&storage2);

    xiiAbstractGraphBinarySerializer::Write(writer2, &graph2);

    XII_TEST_INT(storage.GetStorageSize32(), storage2.GetStorageSize32());

    if (storage.GetStorageSize32() == storage2.GetStorageSize32())
    {
      XII_TEST_BOOL(xiiMemoryUtils::RawByteCompare(storage.GetData(), storage2.GetData(), storage.GetStorageSize32()) == 0);
    }
  }
}

XII_CREATE_SIMPLE_TEST(Serialization, RttiConverter)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "PODs")
  {
    xiiTestStruct t1;
    t1.m_fFloat1 = 5.0f;
    t1.m_UInt8   = 222;
    t1.m_variant = "A";
    t1.m_Angle   = xiiAngle::Degree(5);
    t1.m_DataBuffer.PushBack(1);
    t1.m_DataBuffer.PushBack(5);
    t1.m_vVec3I = xiiVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      xiiTestStruct clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestStruct>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestStruct>()));
      clone.m_variant = "Test";
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestStruct>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "EmbededStruct")
  {
    xiiTestClass1 t1;
    t1.m_Color            = xiiColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8   = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle   = xiiAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = xiiVec3I32(0, 1, 333);
    TestSerialize(&t1);

    {
      xiiTestClass1 clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestClass1>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass1>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      clone.m_Struct.m_variant       = xiiVec3(1, 2, 3);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass1>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Enum")
  {
    xiiTestEnumStruct t1;
    t1.m_enum      = xiiExampleEnum::Value2;
    t1.m_enumClass = xiiExampleEnum::Value3;
    t1.SetEnum(xiiExampleEnum::Value2);
    t1.SetEnumClass(xiiExampleEnum::Value3);
    TestSerialize(&t1);

    {
      xiiTestEnumStruct clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestEnumStruct>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestEnumStruct>()));
      clone.m_enum = xiiExampleEnum::Value3;
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestEnumStruct>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Bitflags")
  {
    xiiTestBitflagsStruct t1;
    t1.m_bitflagsClass.SetValue(0);
    t1.SetBitflagsClass(xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2);
    TestSerialize(&t1);

    {
      xiiTestBitflagsStruct clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestBitflagsStruct>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestBitflagsStruct>()));
      clone.m_bitflagsClass = xiiExampleBitflags::Value1;
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestBitflagsStruct>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Derived Class")
  {
    xiiTestClass2 t1;
    t1.m_Color            = xiiColor::Yellow;
    t1.m_Struct.m_fFloat1 = 5.0f;
    t1.m_Struct.m_UInt8   = 222;
    t1.m_Struct.m_variant = "A";
    t1.m_Struct.m_Angle   = xiiAngle::Degree(5);
    t1.m_Struct.m_DataBuffer.PushBack(1);
    t1.m_Struct.m_DataBuffer.PushBack(5);
    t1.m_Struct.m_vVec3I = xiiVec3I32(0, 1, 333);
    t1.m_Time            = xiiTime::Seconds(22.2f);
    t1.m_enumClass       = xiiExampleEnum::Value3;
    t1.m_bitflagsClass   = xiiExampleBitflags::Value1 | xiiExampleBitflags::Value2;
    t1.m_array.PushBack(40.0f);
    t1.m_array.PushBack(-1.5f);
    t1.m_Variant = xiiVec4(1, 2, 3, 4);
    t1.SetText("LALALALA");
    TestSerialize(&t1);

    {
      xiiTestClass2 clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestClass2>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 6;
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass2>()));
      clone.m_Struct.m_DataBuffer[1] = 5;
      t1.m_array.PushBack(-1.33f);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestClass2>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Arrays")
  {
    xiiTestArrays t1;
    t1.m_Hybrid.PushBack(4.5f);
    t1.m_Hybrid.PushBack(2.3f);
    t1.m_HybridChar.PushBack("Test");

    xiiTestStruct3 ts;
    ts.m_fFloat1 = 5.0f;
    ts.m_UInt8   = 22;
    t1.m_Dynamic.PushBack(ts);
    t1.m_Dynamic.PushBack(ts);
    t1.m_Deque.PushBack(xiiTestArrays());
    TestSerialize(&t1);

    {
      xiiTestArrays clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestArrays>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestArrays>()));
      clone.m_Dynamic.PushBack(xiiTestStruct3());
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestArrays>()));
      clone.m_Dynamic.PopBack();
      clone.m_Hybrid.PushBack(444.0f);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestArrays>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Sets")
  {
    xiiTestSets t1;
    t1.m_SetMember.Insert(0);
    t1.m_SetMember.Insert(5);
    t1.m_SetMember.Insert(-33);
    t1.m_SetAccessor.Insert(-0.0f);
    t1.m_SetAccessor.Insert(5.4f);
    t1.m_SetAccessor.Insert(-33.0f);
    t1.m_Deque.PushBack(3);
    t1.m_Deque.PushBack(33);
    t1.m_Array.PushBack("Test");
    t1.m_Array.PushBack("Bla");
    TestSerialize(&t1);

    {
      xiiTestSets clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestSets>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestSets>()));
      clone.m_SetMember.Insert(12);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestSets>()));
      clone.m_SetMember.Remove(12);
      clone.m_Array.PushBack("Bla2");
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestSets>()));
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Pointer")
  {
    xiiTestPtr t1;
    t1.m_sString       = "Ttttest";
    t1.m_pArrays       = XII_DEFAULT_NEW(xiiTestArrays);
    t1.m_pArraysDirect = XII_DEFAULT_NEW(xiiTestArrays);
    t1.m_ArrayPtr.PushBack(XII_DEFAULT_NEW(xiiTestArrays));
    t1.m_SetPtr.Insert(XII_DEFAULT_NEW(xiiTestSets));
    TestSerialize(&t1);

    {
      xiiTestPtr clone;
      xiiReflectionSerializer::Clone(&t1, &clone, xiiGetStaticRTTI<xiiTestPtr>());
      XII_TEST_BOOL(t1 == clone);
      XII_TEST_BOOL(xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PushBack(42);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestPtr>()));
      clone.m_SetPtr.GetIterator().Key()->m_Deque.PopBack();
      clone.m_ArrayPtr[0]->m_Hybrid.PushBack(123.0f);
      XII_TEST_BOOL(!xiiReflectionUtils::IsEqual(&t1, &clone, xiiGetStaticRTTI<xiiTestPtr>()));
    }
  }
}
