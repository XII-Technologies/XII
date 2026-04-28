/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <FoundationTest/Reflection/ReflectionTestClasses.h>

// clang-format off
XII_BEGIN_STATIC_REFLECTED_ENUM(xiiExampleEnum, 1)
  XII_ENUM_CONSTANTS(xiiExampleEnum::Value1, xiiExampleEnum::Value2)
  XII_ENUM_CONSTANT(xiiExampleEnum::Value3),
XII_END_STATIC_REFLECTED_ENUM;

XII_BEGIN_STATIC_REFLECTED_BITFLAGS(xiiExampleBitflags, 1)
  XII_BITFLAGS_CONSTANTS(xiiExampleBitflags::Value1, xiiExampleBitflags::Value2)
  XII_BITFLAGS_CONSTANT(xiiExampleBitflags::Value3),
XII_END_STATIC_REFLECTED_BITFLAGS;


XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiAbstractTestClass, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_TYPE(xiiAbstractTestStruct, xiiNoBase, 1, xiiRTTINoAllocator);
XII_END_STATIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTestStruct, xiiNoBase, 7, xiiRTTIDefaultAllocator<xiiTestStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new xiiDefaultValueAttribute(1.1f)),
    XII_MEMBER_PROPERTY_READ_ONLY("Vector", m_vProperty3)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(3.0f,4.0f,5.0f))),
    XII_ACCESSOR_PROPERTY("Int", GetInt, SetInt)->AddAttributes(new xiiDefaultValueAttribute(2)),
    XII_MEMBER_PROPERTY("UInt8", m_UInt8)->AddAttributes(new xiiDefaultValueAttribute(6)),
    XII_MEMBER_PROPERTY("Variant", m_variant)->AddAttributes(new xiiDefaultValueAttribute("Test")),
    XII_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new xiiDefaultValueAttribute(xiiAngle::MakeFromDegree(0.5f))),
    XII_MEMBER_PROPERTY("Angled", m_Angled)->AddAttributes(new xiiDefaultValueAttribute(xiiAngled::MakeFromDegree(0.5))),
    XII_MEMBER_PROPERTY("DataBuffer", m_DataBuffer)->AddAttributes(new xiiDefaultValueAttribute(xiiTestStruct::GetDefaultDataBuffer())),
    XII_MEMBER_PROPERTY("vVec3I", m_vVec3I)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3I32(1,2,3))),
    XII_MEMBER_PROPERTY("VarianceAngle", m_VarianceAngle)->AddAttributes(new xiiDefaultValueAttribute(xiiVarianceTypeAngle(xiiAngle::MakeFromDegree(90.0f), 0.5f))),
    XII_MEMBER_PROPERTY("VarianceAngled", m_VarianceAngled)->AddAttributes(new xiiDefaultValueAttribute(xiiVarianceTypeAngled(xiiAngled::MakeFromDegree(90.0), 0.5))),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTestStruct3, xiiNoBase, 71, xiiRTTIDefaultAllocator<xiiTestStruct3>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new xiiDefaultValueAttribute(33.3f)),
    XII_ACCESSOR_PROPERTY("Int", GetInt, SetInt),
    XII_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  XII_END_PROPERTIES;
  XII_BEGIN_FUNCTIONS
  {
    XII_CONSTRUCTOR_PROPERTY(),
    XII_CONSTRUCTOR_PROPERTY(double, xiiInt16),
  }
  XII_END_FUNCTIONS;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTypedObjectStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTypedObjectStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Float", m_fFloat1)->AddAttributes(new xiiDefaultValueAttribute(33.3f)),
    XII_MEMBER_PROPERTY("Int", m_iInt32),
    XII_MEMBER_PROPERTY("UInt8", m_UInt8),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestClass1, 11, xiiRTTIDefaultAllocator<xiiTestClass1>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("SubStruct", m_Struct),
    // XII_MEMBER_PROPERTY("MyVector", m_MyVector), Intentionally not reflected
    XII_MEMBER_PROPERTY("Color", m_Color),
    XII_ACCESSOR_PROPERTY_READ_ONLY("SubVector", GetVector)->AddAttributes(new xiiDefaultValueAttribute(xiiVec3(3, 4, 5)))
  }
    XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

xiiInt32 xiiTestClass2Allocator::m_iAllocs = 0;
xiiInt32 xiiTestClass2Allocator::m_iDeallocs = 0;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestClass2, 22, xiiTestClass2Allocator)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("CharPtr", GetCharPtr, SetCharPtr)->AddAttributes(new xiiDefaultValueAttribute("AAA")),
    XII_ACCESSOR_PROPERTY("String", GetString, SetString)->AddAttributes(new xiiDefaultValueAttribute("BBB")),
    XII_ACCESSOR_PROPERTY("StringView", GetStringView, SetStringView)->AddAttributes(new xiiDefaultValueAttribute("CCC")),
    XII_MEMBER_PROPERTY("Time", m_Time),
    XII_ENUM_MEMBER_PROPERTY("Enum", xiiExampleEnum, m_enumClass),
    XII_BITFLAGS_MEMBER_PROPERTY("Bitflags", xiiExampleBitflags, m_bitflagsClass),
    XII_ARRAY_MEMBER_PROPERTY("Array", m_array),
    XII_MEMBER_PROPERTY("Variant", m_Variant),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestClass2b, 24, xiiRTTIDefaultAllocator<xiiTestClass2b>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("Text2b", GetText, SetText),
    XII_MEMBER_PROPERTY("SubStruct", m_Struct),
    XII_MEMBER_PROPERTY("Color", m_Color),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestArrays, 1, xiiRTTIDefaultAllocator<xiiTestArrays>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ARRAY_MEMBER_PROPERTY("Hybrid", m_Hybrid),
    XII_ARRAY_MEMBER_PROPERTY("HybridChar", m_HybridChar),
    XII_ARRAY_MEMBER_PROPERTY("Dynamic", m_Dynamic),
    XII_ARRAY_MEMBER_PROPERTY("Deque", m_Deque),
    XII_ARRAY_MEMBER_PROPERTY("Custom", m_CustomVariant),
    XII_ARRAY_MEMBER_PROPERTY("Custom2", m_CustomVariant2),

    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridRO", m_Hybrid),
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("HybridCharRO", m_HybridChar),
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("DynamicRO", m_Dynamic),
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("DequeRO", m_Deque),
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("CustomRO", m_CustomVariant),
    XII_ARRAY_MEMBER_PROPERTY_READ_ONLY("CustomRO2", m_CustomVariant2),

    XII_ARRAY_ACCESSOR_PROPERTY("AcHybrid", GetCount, GetValue, SetValue, Insert, Remove),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridRO", GetCount, GetValue),
    XII_ARRAY_ACCESSOR_PROPERTY("AcHybridChar", GetCountChar, GetValueChar, SetValueChar, InsertChar, RemoveChar),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcHybridCharRO", GetCountChar, GetValueChar),
    XII_ARRAY_ACCESSOR_PROPERTY("AcDynamic", GetCountDyn, GetValueDyn, SetValueDyn, InsertDyn, RemoveDyn),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDynamicRO", GetCountDyn, GetValueDyn),
    XII_ARRAY_ACCESSOR_PROPERTY("AcDeque", GetCountDeq, GetValueDeq, SetValueDeq, InsertDeq, RemoveDeq),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcDequeRO", GetCountDeq, GetValueDeq),
    XII_ARRAY_ACCESSOR_PROPERTY("AcCustom", GetCountCustom, GetValueCustom, SetValueCustom, InsertCustom, RemoveCustom),
    XII_ARRAY_ACCESSOR_PROPERTY("AcCustom2", GetCountCustom2, GetValueCustom2, SetValueCustom2, InsertCustom2, RemoveCustom2),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcCustomRO", GetCountCustom, GetValueCustom),
    XII_ARRAY_ACCESSOR_PROPERTY_READ_ONLY("AcCustomRO2", GetCountCustom2, GetValueCustom2),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

xiiUInt32 xiiTestArrays::GetCount() const
{
  return m_Hybrid.GetCount();
}
double xiiTestArrays::GetValue(xiiUInt32 uiIndex) const
{
  return m_Hybrid[uiIndex];
}
void xiiTestArrays::SetValue(xiiUInt32 uiIndex, double value)
{
  m_Hybrid[uiIndex] = value;
}
void xiiTestArrays::Insert(xiiUInt32 uiIndex, double value)
{
  m_Hybrid.InsertAt(uiIndex, value);
}
void xiiTestArrays::Remove(xiiUInt32 uiIndex)
{
  m_Hybrid.RemoveAtAndCopy(uiIndex);
}

xiiUInt32 xiiTestArrays::GetCountChar() const
{
  return m_HybridChar.GetCount();
}
xiiStringView xiiTestArrays::GetValueChar(xiiUInt32 uiIndex) const
{
  return m_HybridChar[uiIndex];
}
void xiiTestArrays::SetValueChar(xiiUInt32 uiIndex, xiiStringView sValue)
{
  m_HybridChar[uiIndex] = sValue;
}
void xiiTestArrays::InsertChar(xiiUInt32 uiIndex, xiiStringView sValue)
{
  m_HybridChar.InsertAt(uiIndex, sValue);
}
void xiiTestArrays::RemoveChar(xiiUInt32 uiIndex)
{
  m_HybridChar.RemoveAtAndCopy(uiIndex);
}

xiiUInt32 xiiTestArrays::GetCountDyn() const
{
  return m_Dynamic.GetCount();
}
const xiiTestStruct3& xiiTestArrays::GetValueDyn(xiiUInt32 uiIndex) const
{
  return m_Dynamic[uiIndex];
}
void xiiTestArrays::SetValueDyn(xiiUInt32 uiIndex, const xiiTestStruct3& value)
{
  m_Dynamic[uiIndex] = value;
}
void xiiTestArrays::InsertDyn(xiiUInt32 uiIndex, const xiiTestStruct3& value)
{
  m_Dynamic.InsertAt(uiIndex, value);
}
void xiiTestArrays::RemoveDyn(xiiUInt32 uiIndex)
{
  m_Dynamic.RemoveAtAndCopy(uiIndex);
}

xiiUInt32 xiiTestArrays::GetCountDeq() const
{
  return m_Deque.GetCount();
}
const xiiTestArrays& xiiTestArrays::GetValueDeq(xiiUInt32 uiIndex) const
{
  return m_Deque[uiIndex];
}
void xiiTestArrays::SetValueDeq(xiiUInt32 uiIndex, const xiiTestArrays& value)
{
  m_Deque[uiIndex] = value;
}
void xiiTestArrays::InsertDeq(xiiUInt32 uiIndex, const xiiTestArrays& value)
{
  m_Deque.InsertAt(uiIndex, value);
}
void xiiTestArrays::RemoveDeq(xiiUInt32 uiIndex)
{
  m_Deque.RemoveAtAndCopy(uiIndex);
}

xiiUInt32 xiiTestArrays::GetCountCustom() const
{
  return m_CustomVariant.GetCount();
}
xiiVarianceTypeAngle xiiTestArrays::GetValueCustom(xiiUInt32 uiIndex) const
{
  return m_CustomVariant[uiIndex];
}
void xiiTestArrays::SetValueCustom(xiiUInt32 uiIndex, xiiVarianceTypeAngle value)
{
  m_CustomVariant[uiIndex] = value;
}
void xiiTestArrays::InsertCustom(xiiUInt32 uiIndex, xiiVarianceTypeAngle value)
{
  m_CustomVariant.InsertAt(uiIndex, value);
}
void xiiTestArrays::RemoveCustom(xiiUInt32 uiIndex)
{
  m_CustomVariant.RemoveAtAndCopy(uiIndex);
}

xiiUInt32 xiiTestArrays::GetCountCustom2() const
{
  return m_CustomVariant2.GetCount();
}
xiiVarianceTypeAngled xiiTestArrays::GetValueCustom2(xiiUInt32 uiIndex) const
{
  return m_CustomVariant2[uiIndex];
}
void xiiTestArrays::SetValueCustom2(xiiUInt32 uiIndex, xiiVarianceTypeAngled value)
{
  m_CustomVariant2[uiIndex] = value;
}
void xiiTestArrays::InsertCustom2(xiiUInt32 uiIndex, xiiVarianceTypeAngled value)
{
  m_CustomVariant2.InsertAt(uiIndex, value);
}
void xiiTestArrays::RemoveCustom2(xiiUInt32 uiIndex)
{
  m_CustomVariant2.RemoveAtAndCopy(uiIndex);
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestSets, 1, xiiRTTIDefaultAllocator<xiiTestSets>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_SET_MEMBER_PROPERTY("Set", m_SetMember),
    XII_SET_MEMBER_PROPERTY_READ_ONLY("SetRO", m_SetMember),
    XII_SET_ACCESSOR_PROPERTY("AcSet", GetSet, Insert, Remove),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("AcSetRO", GetSet),
    XII_SET_MEMBER_PROPERTY("HashSet", m_HashSetMember),
    XII_SET_MEMBER_PROPERTY_READ_ONLY("HashSetRO", m_HashSetMember),
    XII_SET_ACCESSOR_PROPERTY("HashAcSet", GetHashSet, HashInsert, HashRemove),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("HashAcSetRO", GetHashSet),
    XII_SET_ACCESSOR_PROPERTY("AcPseudoSet", GetPseudoSet, PseudoInsert, PseudoRemove),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSetRO", GetPseudoSet),
    XII_SET_ACCESSOR_PROPERTY("AcPseudoSet2", GetPseudoSet2, PseudoInsert2, PseudoRemove2),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("AcPseudoSet2RO", GetPseudoSet2),
    XII_SET_ACCESSOR_PROPERTY("AcPseudoSet2b", GetPseudoSet2, PseudoInsert2b, PseudoRemove2b),
    XII_SET_MEMBER_PROPERTY("CustomHashSet", m_CustomVariant),
    XII_SET_MEMBER_PROPERTY_READ_ONLY("CustomHashSetRO", m_CustomVariant),
    XII_SET_ACCESSOR_PROPERTY("CustomHashAcSet", GetCustomHashSet, CustomHashInsert, CustomHashRemove),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("CustomHashAcSetRO", GetCustomHashSet),
    XII_SET_MEMBER_PROPERTY("CustomHashSet2", m_CustomVariant2),
    XII_SET_MEMBER_PROPERTY_READ_ONLY("CustomHashSetRO2", m_CustomVariant2),
    XII_SET_ACCESSOR_PROPERTY("CustomHashAcSet2", GetCustomHashSet2, CustomHashInsert2, CustomHashRemove2),
    XII_SET_ACCESSOR_PROPERTY_READ_ONLY("CustomHashAcSetRO2", GetCustomHashSet2),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const xiiSet<double>& xiiTestSets::GetSet() const
{
  return m_SetAccessor;
}

void xiiTestSets::Insert(double value)
{
  m_SetAccessor.Insert(value);
}

void xiiTestSets::Remove(double value)
{
  m_SetAccessor.Remove(value);
}


const xiiHashSet<xiiInt64>& xiiTestSets::GetHashSet() const
{
  return m_HashSetAccessor;
}

void xiiTestSets::HashInsert(xiiInt64 value)
{
  m_HashSetAccessor.Insert(value);
}

void xiiTestSets::HashRemove(xiiInt64 value)
{
  m_HashSetAccessor.Remove(value);
}

const xiiDeque<int>& xiiTestSets::GetPseudoSet() const
{
  return m_Deque;
}

void xiiTestSets::PseudoInsert(int value)
{
  if (!m_Deque.Contains(value))
    m_Deque.PushBack(value);
}

void xiiTestSets::PseudoRemove(int value)
{
  m_Deque.RemoveAndCopy(value);
}


xiiArrayPtr<const xiiString> xiiTestSets::GetPseudoSet2() const
{
  return m_Array;
}

void xiiTestSets::PseudoInsert2(const xiiString& value)
{
  if (!m_Array.Contains(value))
    m_Array.PushBack(value);
}

void xiiTestSets::PseudoRemove2(const xiiString& value)
{
  m_Array.RemoveAndCopy(value);
}

void xiiTestSets::PseudoInsert2b(xiiStringView sValue)
{
  if (!m_Array.Contains(sValue))
    m_Array.PushBack(sValue);
}

void xiiTestSets::PseudoRemove2b(xiiStringView sValue)
{
  m_Array.RemoveAndCopy(sValue);
}

const xiiHashSet<xiiVarianceTypeAngle>& xiiTestSets::GetCustomHashSet() const
{
  return m_CustomVariant;
}

void xiiTestSets::CustomHashInsert(xiiVarianceTypeAngle value)
{
  m_CustomVariant.Insert(value);
}

void xiiTestSets::CustomHashRemove(xiiVarianceTypeAngle value)
{
  m_CustomVariant.Remove(value);
}

const xiiHashSet<xiiVarianceTypeAngled>& xiiTestSets::GetCustomHashSet2() const
{
  return m_CustomVariant2;
}

void xiiTestSets::CustomHashInsert2(xiiVarianceTypeAngled value)
{
  m_CustomVariant2.Insert(value);
}

void xiiTestSets::CustomHashRemove2(xiiVarianceTypeAngled value)
{
  m_CustomVariant2.Remove(value);
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestMaps, 1, xiiRTTIDefaultAllocator<xiiTestMaps>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MAP_MEMBER_PROPERTY("Map", m_MapMember),
    XII_MAP_MEMBER_PROPERTY_READ_ONLY("MapRO", m_MapMember),
    XII_MAP_WRITE_ACCESSOR_PROPERTY("AcMap", GetContainer, Insert, Remove),
    XII_MAP_MEMBER_PROPERTY("HashTable", m_HashTableMember),
    XII_MAP_MEMBER_PROPERTY_READ_ONLY("HashTableRO", m_HashTableMember),
    XII_MAP_WRITE_ACCESSOR_PROPERTY("AcHashTable", GetContainer2, Insert2, Remove2),
    XII_MAP_ACCESSOR_PROPERTY("Accessor", GetKeys3, GetValue3, Insert3, Remove3),
    XII_MAP_ACCESSOR_PROPERTY_READ_ONLY("AccessorRO", GetKeys3, GetValue3),
    XII_MAP_MEMBER_PROPERTY("CustomVariant", m_CustomVariant),
    XII_MAP_MEMBER_PROPERTY_READ_ONLY("CustomVariantRO", m_CustomVariant),
    XII_MAP_MEMBER_PROPERTY("CustomVariant2", m_CustomVariant2),
    XII_MAP_MEMBER_PROPERTY_READ_ONLY("CustomVariantRO2", m_CustomVariant2),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

bool xiiTestMaps::operator==(const xiiTestMaps& rhs) const
{
  for (xiiUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    bool bRes = false;
    for (xiiUInt32 j = 0; j < rhs.m_Accessor3.GetCount(); j++)
    {
      if (m_Accessor3[i].m_Key == rhs.m_Accessor3[j].m_Key)
      {
        if (m_Accessor3[i].m_Value == rhs.m_Accessor3[j].m_Value)
          bRes = true;
      }
    }
    if (!bRes)
      return false;
  }
  return m_MapMember == rhs.m_MapMember && m_MapAccessor == rhs.m_MapAccessor && m_HashTableMember == rhs.m_HashTableMember && m_HashTableAccessor == rhs.m_HashTableAccessor && m_CustomVariant == rhs.m_CustomVariant && m_CustomVariant2 == rhs.m_CustomVariant2;
}

const xiiMap<xiiString, xiiInt64>& xiiTestMaps::GetContainer() const
{
  return m_MapAccessor;
}

void xiiTestMaps::Insert(xiiStringView sKey, xiiInt64 value)
{
  m_MapAccessor.Insert(sKey, value);
}

void xiiTestMaps::Remove(xiiStringView sKey)
{
  m_MapAccessor.Remove(sKey);
}

const xiiHashTable<xiiString, xiiString>& xiiTestMaps::GetContainer2() const
{
  return m_HashTableAccessor;
}

void xiiTestMaps::Insert2(xiiStringView sKey, const xiiString& value)
{
  m_HashTableAccessor.Insert(sKey, value);
}


void xiiTestMaps::Remove2(xiiStringView sKey)
{
  m_HashTableAccessor.Remove(sKey);
}

const xiiRangeView<xiiStringView, xiiUInt32> xiiTestMaps::GetKeys3() const
{
  return xiiRangeView<xiiStringView, xiiUInt32>([this]() -> xiiUInt32 { return 0; }, [this]() -> xiiUInt32 { return m_Accessor3.GetCount(); }, [this](xiiUInt32& ref_uiIt) { ++ref_uiIt; }, [this](const xiiUInt32& uiIt) -> const char* { return m_Accessor3[uiIt].m_Key; });
}

void xiiTestMaps::Insert3(xiiStringView sKey, const xiiVariant& value)
{
  for (auto&& t : m_Accessor3)
  {
    if (t.m_Key == sKey)
    {
      t.m_Value = value;
      return;
    }
  }
  auto&& t  = m_Accessor3.ExpandAndGetRef();
  t.m_Key   = sKey;
  t.m_Value = value;
}

void xiiTestMaps::Remove3(xiiStringView sKey)
{
  for (xiiUInt32 i = 0; i < m_Accessor3.GetCount(); i++)
  {
    const Tuple& t = m_Accessor3[i];
    if (t.m_Key == sKey)
    {
      m_Accessor3.RemoveAtAndSwap(i);
      break;
    }
  }
}

bool xiiTestMaps::GetValue3(xiiStringView sKey, xiiVariant& out_value) const
{
  for (const auto& t : m_Accessor3)
  {
    if (t.m_Key == sKey)
    {
      out_value = t.m_Value;
      return true;
    }
  }
  return false;
}

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestPtr, 1, xiiRTTIDefaultAllocator<xiiTestPtr>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ACCESSOR_PROPERTY("ConstCharPtr", GetString, SetString),
    XII_ACCESSOR_PROPERTY("ArraysPtr", GetArrays, SetArrays)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_MEMBER_PROPERTY("ArraysPtrDirect", m_pArraysDirect)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_ARRAY_MEMBER_PROPERTY("PtrArray", m_ArrayPtr)->AddFlags(xiiPropertyFlags::PointerOwner),
    XII_SET_MEMBER_PROPERTY("PtrSet", m_SetPtr)->AddFlags(xiiPropertyFlags::PointerOwner),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;


XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTestEnumStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTestEnumStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_ENUM_MEMBER_PROPERTY("m_enum", xiiExampleEnum, m_enum),
    XII_ENUM_MEMBER_PROPERTY("m_enumClass", xiiExampleEnum, m_enumClass),
    XII_ENUM_ACCESSOR_PROPERTY("m_enum2", xiiExampleEnum, GetEnum, SetEnum),
    XII_ENUM_ACCESSOR_PROPERTY("m_enumClass2", xiiExampleEnum,  GetEnumClass, SetEnumClass),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;

XII_BEGIN_STATIC_REFLECTED_TYPE(xiiTestBitflagsStruct, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiTestBitflagsStruct>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_BITFLAGS_MEMBER_PROPERTY("m_bitflagsClass", xiiExampleBitflags, m_bitflagsClass),
    XII_BITFLAGS_ACCESSOR_PROPERTY("m_bitflagsClass2", xiiExampleBitflags, GetBitflagsClass, SetBitflagsClass),
  }
  XII_END_PROPERTIES;
}
XII_END_STATIC_REFLECTED_TYPE;
// clang-format on
