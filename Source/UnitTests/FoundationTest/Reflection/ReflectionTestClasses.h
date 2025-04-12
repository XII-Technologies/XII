#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/RangeView.h>
#include <Foundation/Types/VarianceTypes.h>

struct xiiExampleEnum
{
  using StorageType = xiiInt8;

  enum Enum : StorageType
  {
    Value1  = 1,     // normal value
    Value2  = -2,    // normal value
    Value3  = 4,     // normal value
    Default = Value1 // Default initialization value (required)
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiExampleEnum);


struct xiiExampleBitflags
{
  using StorageType = xiiUInt64;

  enum Enum : StorageType
  {
    Value1  = XII_BIT(0),  // normal value
    Value2  = XII_BIT(31), // normal value
    Value3  = XII_BIT(63), // normal value
    Default = Value1       // Default initialization value (required)
  };

  struct Bits
  {
    StorageType Value1 : 1;
    StorageType Padding : 30;
    StorageType Value2 : 1;
    StorageType Padding2 : 31;
    StorageType Value3 : 1;
  };
};

XII_DECLARE_FLAGS_OPERATORS(xiiExampleBitflags);

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiExampleBitflags);


class xiiAbstractTestClass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiAbstractTestClass, xiiReflectedClass);

  virtual void AbstractFunction() = 0;
};


struct xiiAbstractTestStruct
{
  virtual void AbstractFunction() = 0;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiAbstractTestStruct);


struct xiiTestStruct
{
  XII_ALLOW_PRIVATE_PROPERTIES(xiiTestStruct);

public:
  static xiiDataBuffer GetDefaultDataBuffer()
  {
    xiiDataBuffer data;
    data.PushBack(255);
    data.PushBack(0);
    data.PushBack(127);
    return data;
  }

  xiiTestStruct()
  {
    m_fFloat1 = 1.1f;
    m_iInt2   = 2;
    m_vProperty3.Set(3, 4, 5);
    m_UInt8                      = 6;
    m_variant                    = "Test";
    m_Angle                      = xiiAngle::MakeFromDegree(0.5);
    m_Angled                     = xiiAngled::MakeFromDegree(0.5);
    m_DataBuffer                 = GetDefaultDataBuffer();
    m_vVec3I                     = xiiVec3I32(1, 2, 3);
    m_VarianceAngle.m_fVariance  = 0.5f;
    m_VarianceAngle.m_Value      = xiiAngle::MakeFromDegree(90.0f);
    m_VarianceAngled.m_fVariance = 0.5f;
    m_VarianceAngled.m_Value     = xiiAngled::MakeFromDegree(90.0);
  }

  bool operator==(const xiiTestStruct& rhs) const
  {
    return m_fFloat1 == rhs.m_fFloat1 && m_UInt8 == rhs.m_UInt8 && m_variant == rhs.m_variant && m_iInt2 == rhs.m_iInt2 && m_vProperty3 == rhs.m_vProperty3 && m_Angle == rhs.m_Angle && m_Angled == rhs.m_Angled && m_DataBuffer == rhs.m_DataBuffer && m_vVec3I == rhs.m_vVec3I && m_VarianceAngled == rhs.m_VarianceAngled;
  }

  float                 m_fFloat1;
  xiiUInt8              m_UInt8;
  xiiVariant            m_variant;
  xiiAngle              m_Angle;
  xiiAngled             m_Angled;
  xiiDataBuffer         m_DataBuffer;
  xiiVec3I32            m_vVec3I;
  xiiVarianceTypeAngle  m_VarianceAngle;
  xiiVarianceTypeAngled m_VarianceAngled;

private:
  void     SetInt(xiiInt32 i) { m_iInt2 = i; }
  xiiInt32 GetInt() const { return m_iInt2; }

  xiiInt32 m_iInt2;
  xiiVec3  m_vProperty3;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTestStruct);


struct xiiTestStruct3
{
  XII_ALLOW_PRIVATE_PROPERTIES(xiiTestStruct3);

public:
  xiiTestStruct3()
  {
    m_fFloat1 = 1.1f;
    m_UInt8   = 6;
    m_iInt32  = 2;
  }
  xiiTestStruct3(double a, xiiInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8   = b;
    m_iInt32  = 32;
  }

  bool operator==(const xiiTestStruct3& rhs) const { return m_fFloat1 == rhs.m_fFloat1 && m_iInt32 == rhs.m_iInt32 && m_UInt8 == rhs.m_UInt8; }

  double   m_fFloat1;
  xiiInt16 m_UInt8;

  xiiUInt32 GetIntPublic() const { return m_iInt32; }

private:
  void      SetInt(xiiUInt32 i) { m_iInt32 = i; }
  xiiUInt32 GetInt() const { return m_iInt32; }

  xiiInt32 m_iInt32;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTestStruct3);

struct xiiTypedObjectStruct
{
  XII_ALLOW_PRIVATE_PROPERTIES(xiiTypedObjectStruct);

public:
  xiiTypedObjectStruct()
  {
    m_fFloat1 = 1.1f;
    m_UInt8   = 6;
    m_iInt32  = 2;
  }
  xiiTypedObjectStruct(double a, xiiInt16 b)
  {
    m_fFloat1 = a;
    m_UInt8   = b;
    m_iInt32  = 32;
  }

  double   m_fFloat1;
  xiiInt16 m_UInt8;
  xiiInt32 m_iInt32;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTypedObjectStruct);
XII_DECLARE_CUSTOM_VARIANT_TYPE(xiiTypedObjectStruct);

class xiiTestClass1 : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestClass1, xiiReflectedClass);

public:
  xiiTestClass1()
  {
    m_MyVector.Set(3, 4, 5);

    m_Struct.m_fFloat1 = 33.3f;

    m_Color = xiiColor::CornflowerBlue; // The Original!
  }

  xiiTestClass1(const xiiColor& c, const xiiTestStruct& s)
  {
    m_Color  = c;
    m_Struct = s;
    m_MyVector.Set(1, 2, 3);
  }

  bool operator==(const xiiTestClass1& rhs) const { return m_Struct == rhs.m_Struct && m_MyVector == rhs.m_MyVector && m_Color == rhs.m_Color; }

  xiiVec3 GetVector() const { return m_MyVector; }

  xiiTestStruct m_Struct;
  xiiVec3       m_MyVector;
  xiiColor      m_Color;
};


class xiiTestClass2 : public xiiTestClass1
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestClass2, xiiTestClass1);

public:
  xiiTestClass2()
  {
    m_sCharPtr    = "AAA";
    m_sString     = "BBB";
    m_sStringView = "CCC";
  }

  bool operator==(const xiiTestClass2& rhs) const { return m_Time == rhs.m_Time && m_enumClass == rhs.m_enumClass && m_bitflagsClass == rhs.m_bitflagsClass && m_array == rhs.m_array && m_Variant == rhs.m_Variant && m_sCharPtr == rhs.m_sCharPtr && m_sString == rhs.m_sString && m_sStringView == rhs.m_sStringView; }

  const char* GetCharPtr() const { return m_sCharPtr.GetData(); }
  void        SetCharPtr(const char* szSz) { m_sCharPtr = szSz; }

  const xiiString& GetString() const { return m_sString; }
  void             SetString(const xiiString& sStr) { m_sString = sStr; }

  xiiStringView GetStringView() const { return m_sStringView.GetView(); }
  void          SetStringView(xiiStringView sStrView) { m_sStringView = sStrView; }

  xiiTime                         m_Time;
  xiiEnum<xiiExampleEnum>         m_enumClass;
  xiiBitflags<xiiExampleBitflags> m_bitflagsClass;
  xiiHybridArray<float, 4>        m_array;
  xiiVariant                      m_Variant;

private:
  xiiString m_sCharPtr;
  xiiString m_sString;
  xiiString m_sStringView;
};


struct xiiTestClass2Allocator : public xiiRTTIAllocator
{
  virtual xiiInternal::NewInstance<void> AllocateInternal(xiiAllocatorBase* pAllocator) override
  {
    ++m_iAllocs;

    return XII_DEFAULT_NEW(xiiTestClass2);
  }

  virtual void Deallocate(void* pObject, xiiAllocatorBase* pAllocator) override
  {
    ++m_iDeallocs;

    xiiTestClass2* pPointer = (xiiTestClass2*)pObject;
    XII_DEFAULT_DELETE(pPointer);
  }

  static xiiInt32 m_iAllocs;
  static xiiInt32 m_iDeallocs;
};


class xiiTestClass2b : xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestClass2b, xiiReflectedClass);

public:
  xiiTestClass2b() { m_sText = "Tut"; }

  xiiStringView GetText() const { return m_sText; }
  void          SetText(xiiStringView sSz) { m_sText = sSz; }

  xiiTestStruct3 m_Struct;
  xiiColor       m_Color;

private:
  xiiString m_sText;
};


class xiiTestArrays : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestArrays, xiiReflectedClass);

public:
  xiiTestArrays() = default;

  bool operator==(const xiiTestArrays& rhs) const
  {
    return m_Hybrid == rhs.m_Hybrid && m_Dynamic == rhs.m_Dynamic && m_Deque == rhs.m_Deque && m_HybridChar == rhs.m_HybridChar && m_CustomVariant == rhs.m_CustomVariant && m_CustomVariant2 == rhs.m_CustomVariant2;
  }

  xiiUInt32 GetCount() const;
  double    GetValue(xiiUInt32 uiIndex) const;
  void      SetValue(xiiUInt32 uiIndex, double value);
  void      Insert(xiiUInt32 uiIndex, double value);
  void      Remove(xiiUInt32 uiIndex);

  xiiUInt32     GetCountChar() const;
  xiiStringView GetValueChar(xiiUInt32 uiIndex) const;
  void          SetValueChar(xiiUInt32 uiIndex, xiiStringView sValue);
  void          InsertChar(xiiUInt32 uiIndex, xiiStringView sValue);
  void          RemoveChar(xiiUInt32 uiIndex);

  xiiUInt32             GetCountDyn() const;
  const xiiTestStruct3& GetValueDyn(xiiUInt32 uiIndex) const;
  void                  SetValueDyn(xiiUInt32 uiIndex, const xiiTestStruct3& value);
  void                  InsertDyn(xiiUInt32 uiIndex, const xiiTestStruct3& value);
  void                  RemoveDyn(xiiUInt32 uiIndex);

  xiiUInt32            GetCountDeq() const;
  const xiiTestArrays& GetValueDeq(xiiUInt32 uiIndex) const;
  void                 SetValueDeq(xiiUInt32 uiIndex, const xiiTestArrays& value);
  void                 InsertDeq(xiiUInt32 uiIndex, const xiiTestArrays& value);
  void                 RemoveDeq(xiiUInt32 uiIndex);

  xiiUInt32            GetCountCustom() const;
  xiiVarianceTypeAngle GetValueCustom(xiiUInt32 uiIndex) const;
  void                 SetValueCustom(xiiUInt32 uiIndex, xiiVarianceTypeAngle value);
  void                 InsertCustom(xiiUInt32 uiIndex, xiiVarianceTypeAngle value);
  void                 RemoveCustom(xiiUInt32 uiIndex);

  xiiUInt32             GetCountCustom2() const;
  xiiVarianceTypeAngled GetValueCustom2(xiiUInt32 uiIndex) const;
  void                  SetValueCustom2(xiiUInt32 uiIndex, xiiVarianceTypeAngled value);
  void                  InsertCustom2(xiiUInt32 uiIndex, xiiVarianceTypeAngled value);
  void                  RemoveCustom2(xiiUInt32 uiIndex);

  xiiHybridArray<double, 5>                m_Hybrid;
  xiiHybridArray<xiiString, 2>             m_HybridChar;
  xiiDynamicArray<xiiTestStruct3>          m_Dynamic;
  xiiDeque<xiiTestArrays>                  m_Deque;
  xiiHybridArray<xiiVarianceTypeAngle, 1>  m_CustomVariant;
  xiiHybridArray<xiiVarianceTypeAngled, 1> m_CustomVariant2;
};


class xiiTestSets : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestSets, xiiReflectedClass);

public:
  xiiTestSets() = default;

  bool operator==(const xiiTestSets& rhs) const
  {
    return m_SetMember == rhs.m_SetMember && m_SetAccessor == rhs.m_SetAccessor && m_Deque == rhs.m_Deque && m_Array == rhs.m_Array && m_CustomVariant == rhs.m_CustomVariant;
  }

  bool operator!=(const xiiTestSets& rhs) const { return !(*this == rhs); }

  const xiiSet<double>& GetSet() const;
  void                  Insert(double value);
  void                  Remove(double value);

  const xiiHashSet<xiiInt64>& GetHashSet() const;
  void                        HashInsert(xiiInt64 value);
  void                        HashRemove(xiiInt64 value);

  const xiiDeque<int>& GetPseudoSet() const;
  void                 PseudoInsert(int value);
  void                 PseudoRemove(int value);

  xiiArrayPtr<const xiiString> GetPseudoSet2() const;
  void                         PseudoInsert2(const xiiString& value);
  void                         PseudoRemove2(const xiiString& value);

  void PseudoInsert2b(xiiStringView sValue);
  void PseudoRemove2b(xiiStringView sValue);

  const xiiHashSet<xiiVarianceTypeAngle>& GetCustomHashSet() const;
  void                                    CustomHashInsert(xiiVarianceTypeAngle value);
  void                                    CustomHashRemove(xiiVarianceTypeAngle value);

  const xiiHashSet<xiiVarianceTypeAngled>& GetCustomHashSet2() const;
  void                                     CustomHashInsert2(xiiVarianceTypeAngled value);
  void                                     CustomHashRemove2(xiiVarianceTypeAngled value);

  xiiSet<xiiInt8> m_SetMember;
  xiiSet<double>  m_SetAccessor;

  xiiHashSet<xiiInt32> m_HashSetMember;
  xiiHashSet<xiiInt64> m_HashSetAccessor;

  xiiDeque<int>                     m_Deque;
  xiiDynamicArray<xiiString>        m_Array;
  xiiHashSet<xiiVarianceTypeAngle>  m_CustomVariant;
  xiiHashSet<xiiVarianceTypeAngled> m_CustomVariant2;
};


class xiiTestMaps : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestMaps, xiiReflectedClass);

public:
  xiiTestMaps() = default;

  bool operator==(const xiiTestMaps& rhs) const;

  const xiiMap<xiiString, xiiInt64>& GetContainer() const;
  void                               Insert(xiiStringView sKey, xiiInt64 value);
  void                               Remove(xiiStringView sKey);

  const xiiHashTable<xiiString, xiiString>& GetContainer2() const;
  void                                      Insert2(xiiStringView sKey, const xiiString& value);
  void                                      Remove2(xiiStringView sKey);

  const xiiRangeView<xiiStringView, xiiUInt32> GetKeys3() const;
  void                                         Insert3(xiiStringView sKey, const xiiVariant& value);
  void                                         Remove3(xiiStringView sKey);
  bool                                         GetValue3(xiiStringView sKey, xiiVariant& out_value) const;

  xiiMap<xiiString, int>      m_MapMember;
  xiiMap<xiiString, xiiInt64> m_MapAccessor;

  xiiHashTable<xiiString, double>    m_HashTableMember;
  xiiHashTable<xiiString, xiiString> m_HashTableAccessor;

  xiiMap<xiiString, xiiVarianceTypeAngle>  m_CustomVariant;
  xiiMap<xiiString, xiiVarianceTypeAngled> m_CustomVariant2;

  struct Tuple
  {
    xiiString  m_Key;
    xiiVariant m_Value;
  };
  xiiHybridArray<Tuple, 2> m_Accessor3;
};

class xiiTestPtr : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestPtr, xiiReflectedClass);

public:
  xiiTestPtr()
  {
    m_pArrays       = nullptr;
    m_pArraysDirect = nullptr;
  }

  ~xiiTestPtr()
  {
    XII_DEFAULT_DELETE(m_pArrays);
    XII_DEFAULT_DELETE(m_pArraysDirect);
    for (auto ptr : m_ArrayPtr)
    {
      XII_DEFAULT_DELETE(ptr);
    }
    m_ArrayPtr.Clear();
    for (auto ptr : m_SetPtr)
    {
      XII_DEFAULT_DELETE(ptr);
    }
    m_SetPtr.Clear();
  }

  bool operator==(const xiiTestPtr& rhs) const
  {
    if (m_sString != rhs.m_sString || (m_pArrays != rhs.m_pArrays && *m_pArrays != *rhs.m_pArrays))
      return false;

    if (m_ArrayPtr.GetCount() != rhs.m_ArrayPtr.GetCount())
      return false;

    for (xiiUInt32 i = 0; i < m_ArrayPtr.GetCount(); i++)
    {
      if (!(*m_ArrayPtr[i] == *rhs.m_ArrayPtr[i]))
        return false;
    }

    // only works for the test data if the test.
    if (m_SetPtr.IsEmpty() && rhs.m_SetPtr.IsEmpty())
      return true;

    if (m_SetPtr.GetCount() != 1 || rhs.m_SetPtr.GetCount() != 1)
      return true;

    return *m_SetPtr.GetIterator().Key() == *rhs.m_SetPtr.GetIterator().Key();
  }

  void        SetString(const char* szValue) { m_sString = szValue; }
  const char* GetString() const { return m_sString; }

  void           SetArrays(xiiTestArrays* pValue) { m_pArrays = pValue; }
  xiiTestArrays* GetArrays() const { return m_pArrays; }


  xiiString                m_sString;
  xiiTestArrays*           m_pArrays;
  xiiTestArrays*           m_pArraysDirect;
  xiiDeque<xiiTestArrays*> m_ArrayPtr;
  xiiSet<xiiTestSets*>     m_SetPtr;
};


struct xiiTestEnumStruct
{
  XII_ALLOW_PRIVATE_PROPERTIES(xiiTestEnumStruct);

public:
  xiiTestEnumStruct()
  {
    m_enum       = xiiExampleEnum::Value1;
    m_enumClass  = xiiExampleEnum::Value1;
    m_Enum2      = xiiExampleEnum::Value1;
    m_EnumClass2 = xiiExampleEnum::Value1;
  }

  bool operator==(const xiiTestEnumStruct& rhs) const { return m_Enum2 == rhs.m_Enum2 && m_enum == rhs.m_enum && m_enumClass == rhs.m_enumClass && m_EnumClass2 == rhs.m_EnumClass2; }

  xiiExampleEnum::Enum    m_enum;
  xiiEnum<xiiExampleEnum> m_enumClass;

  void                    SetEnum(xiiExampleEnum::Enum e) { m_Enum2 = e; }
  xiiExampleEnum::Enum    GetEnum() const { return m_Enum2; }
  void                    SetEnumClass(xiiEnum<xiiExampleEnum> e) { m_EnumClass2 = e; }
  xiiEnum<xiiExampleEnum> GetEnumClass() const { return m_EnumClass2; }

private:
  xiiExampleEnum::Enum    m_Enum2;
  xiiEnum<xiiExampleEnum> m_EnumClass2;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTestEnumStruct);


struct xiiTestBitflagsStruct
{
  XII_ALLOW_PRIVATE_PROPERTIES(xiiTestBitflagsStruct);

public:
  xiiTestBitflagsStruct()
  {
    m_bitflagsClass  = xiiExampleBitflags::Value1;
    m_BitflagsClass2 = xiiExampleBitflags::Value1;
  }

  bool operator==(const xiiTestBitflagsStruct& rhs) const { return m_bitflagsClass == rhs.m_bitflagsClass && m_BitflagsClass2 == rhs.m_BitflagsClass2; }

  xiiBitflags<xiiExampleBitflags> m_bitflagsClass;

  void                            SetBitflagsClass(xiiBitflags<xiiExampleBitflags> e) { m_BitflagsClass2 = e; }
  xiiBitflags<xiiExampleBitflags> GetBitflagsClass() const { return m_BitflagsClass2; }

private:
  xiiBitflags<xiiExampleBitflags> m_BitflagsClass2;
};

XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiTestBitflagsStruct);
