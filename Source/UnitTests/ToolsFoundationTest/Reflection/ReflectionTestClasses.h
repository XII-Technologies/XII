#pragma once

#include <Foundation/Math/Color8UNorm.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/VarianceTypes.h>

struct xiiIntegerStruct
{
public:
  xiiIntegerStruct()
  {
    m_iInt8    = 1;
    m_uiUInt8  = 1;
    m_iInt16   = 1;
    m_iUInt16  = 1;
    m_iInt32   = 1;
    m_uiUInt32 = 1;
    m_iInt64   = 1;
    m_iUInt64  = 1;
  }

  void      SetInt8(xiiInt8 i) { m_iInt8 = i; }
  xiiInt8   GetInt8() const { return m_iInt8; }
  void      SetUInt8(xiiUInt8 i) { m_uiUInt8 = i; }
  xiiUInt8  GetUInt8() const { return m_uiUInt8; }
  void      SetInt32(xiiInt32 i) { m_iInt32 = i; }
  xiiInt32  GetInt32() const { return m_iInt32; }
  void      SetUInt32(xiiUInt32 i) { m_uiUInt32 = i; }
  xiiUInt32 GetUInt32() const { return m_uiUInt32; }

  xiiInt16  m_iInt16;
  xiiUInt16 m_iUInt16;
  xiiInt64  m_iInt64;
  xiiUInt64 m_iUInt64;

private:
  xiiInt8   m_iInt8;
  xiiUInt8  m_uiUInt8;
  xiiInt32  m_iInt32;
  xiiUInt32 m_uiUInt32;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiIntegerStruct);


struct xiiFloatStruct
{
public:
  xiiFloatStruct()
  {
    m_fFloat  = 1.0f;
    m_fDouble = 1.0;
    m_Time    = xiiTime::Seconds(1.0);
    m_Angle   = xiiAngle::Degree(45.0f);
  }

  void     SetFloat(float f) { m_fFloat = f; }
  float    GetFloat() const { return m_fFloat; }
  void     SetDouble(double d) { m_fDouble = d; }
  double   GetDouble() const { return m_fDouble; }
  void     SetTime(xiiTime t) { m_Time = t; }
  xiiTime  GetTime() const { return m_Time; }
  xiiAngle GetAngle() const { return m_Angle; }
  void     SetAngle(xiiAngle t) { m_Angle = t; }

private:
  float    m_fFloat;
  double   m_fDouble;
  xiiTime  m_Time;
  xiiAngle m_Angle;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiFloatStruct);


class xiiPODClass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPODClass, xiiReflectedClass);

public:
  xiiPODClass()
  {
    m_bBool   = true;
    m_Color   = xiiColor(1.0f, 0.0f, 0.0f, 0.0f);
    m_Color2  = xiiColorGammaUB(255, 10, 1);
    m_sString = "Test";
    m_Buffer.PushBack(0xFF);
    m_Buffer.PushBack(0x0);
    m_Buffer.PushBack(0xCD);
    m_VarianceAngle = {0.1f, xiiAngle::Degree(90.0f)};
  }

  xiiIntegerStruct m_IntegerStruct;
  xiiFloatStruct   m_FloatStruct;

  void        SetBool(bool b) { m_bBool = b; }
  bool        GetBool() const { return m_bBool; }
  void        SetColor(xiiColor c) { m_Color = c; }
  xiiColor    GetColor() const { return m_Color; }
  const char* GetString() const { return m_sString.GetData(); }
  void        SetString(const char* szSz) { m_sString = szSz; }

  const xiiDataBuffer& GetBuffer() const { return m_Buffer; }
  void                 SetBuffer(const xiiDataBuffer& data) { m_Buffer = data; }

  xiiVarianceTypeAngle GetCustom() const { return m_VarianceAngle; }
  void                 SetCustom(xiiVarianceTypeAngle value) { m_VarianceAngle = value; }

private:
  bool                 m_bBool;
  xiiColor             m_Color;
  xiiColorGammaUB      m_Color2;
  xiiString            m_sString;
  xiiDataBuffer        m_Buffer;
  xiiVarianceTypeAngle m_VarianceAngle;
};


class xiiMathClass : public xiiPODClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMathClass, xiiPODClass);

public:
  xiiMathClass()
  {
    m_vVec2 = xiiVec2(1.0f, 1.0f);
    m_vVec3 = xiiVec3(1.0f, 1.0f, 1.0f);
    m_vVec4 = xiiVec4(1.0f, 1.0f, 1.0f, 1.0f);
    m_Vec2I = xiiVec2I32(1, 1);
    m_Vec3I = xiiVec3I32(1, 1, 1);
    m_Vec4I = xiiVec4I32(1, 1, 1, 1);
    m_qQuat = xiiQuat(1.0f, 1.0f, 1.0f, 1.0f);
    m_mMat3.SetZero();
    m_mMat4.SetZero();
  }

  void    SetVec2(xiiVec2 v) { m_vVec2 = v; }
  xiiVec2 GetVec2() const { return m_vVec2; }
  void    SetVec3(xiiVec3 v) { m_vVec3 = v; }
  xiiVec3 GetVec3() const { return m_vVec3; }
  void    SetVec4(xiiVec4 v) { m_vVec4 = v; }
  xiiVec4 GetVec4() const { return m_vVec4; }
  void    SetQuat(xiiQuat q) { m_qQuat = q; }
  xiiQuat GetQuat() const { return m_qQuat; }
  void    SetMat3(xiiMat3 m) { m_mMat3 = m; }
  xiiMat3 GetMat3() const { return m_mMat3; }
  void    SetMat4(xiiMat4 m) { m_mMat4 = m; }
  xiiMat4 GetMat4() const { return m_mMat4; }

  xiiVec2I32 m_Vec2I;
  xiiVec3I32 m_Vec3I;
  xiiVec4I32 m_Vec4I;

private:
  xiiVec2 m_vVec2;
  xiiVec3 m_vVec3;
  xiiVec4 m_vVec4;
  xiiQuat m_qQuat;
  xiiMat3 m_mMat3;
  xiiMat4 m_mMat4;
};


struct xiiExampleEnum
{
  using StorageType = xiiInt8;
  enum Enum
  {
    Value1  = 0,     // normal value
    Value2  = -2,    // normal value
    Value3  = 4,     // normal value
    Default = Value1 // Default initialization value (required)
  };
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiExampleEnum);


struct xiiExampleBitflags
{
  using StorageType = xiiUInt64;
  enum Enum : xiiUInt64
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
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiExampleBitflags);


class xiiEnumerationsClass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiEnumerationsClass, xiiReflectedClass);

public:
  xiiEnumerationsClass()
  {
    m_EnumClass     = xiiExampleEnum::Value2;
    m_BitflagsClass = xiiExampleBitflags::Value2;
  }

  void                            SetEnum(xiiExampleEnum::Enum e) { m_EnumClass = e; }
  xiiExampleEnum::Enum            GetEnum() const { return m_EnumClass; }
  void                            SetBitflags(xiiBitflags<xiiExampleBitflags> e) { m_BitflagsClass = e; }
  xiiBitflags<xiiExampleBitflags> GetBitflags() const { return m_BitflagsClass; }

private:
  xiiEnum<xiiExampleEnum>         m_EnumClass;
  xiiBitflags<xiiExampleBitflags> m_BitflagsClass;
};


struct InnerStruct
{
  XII_DECLARE_POD_TYPE();

public:
  float m_fP1;
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, InnerStruct);


class OuterClass : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(OuterClass, xiiReflectedClass);

public:
  InnerStruct m_Inner1;
  float       m_fP1;
};

class ExtendedOuterClass : public OuterClass
{
  XII_ADD_DYNAMIC_REFLECTION(ExtendedOuterClass, OuterClass);

public:
  xiiString m_more;
};

class xiiObjectTest : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiObjectTest, xiiReflectedClass);

public:
  xiiObjectTest() = default;
  ~xiiObjectTest()
  {
    for (OuterClass* pTest : m_ClassPtrArray)
    {
      xiiGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(pTest);
    }
    for (xiiObjectTest* pTest : m_SubObjectSet)
    {
      xiiGetStaticRTTI<xiiObjectTest>()->GetAllocator()->Deallocate(pTest);
    }
    for (auto it = m_ClassPtrMap.GetIterator(); it.IsValid(); ++it)
    {
      xiiGetStaticRTTI<OuterClass>()->GetAllocator()->Deallocate(it.Value());
    }
  }

  xiiArrayPtr<const xiiString> GetStandardTypeSet() const;
  void                         StandardTypeSetInsert(const xiiString& value);
  void                         StandardTypeSetRemove(const xiiString& value);

  OuterClass m_MemberClass;

  xiiDynamicArray<double>     m_StandardTypeArray;
  xiiDynamicArray<OuterClass> m_ClassArray;
  xiiDeque<OuterClass*>       m_ClassPtrArray;

  xiiDynamicArray<xiiString> m_StandardTypeSet;
  xiiSet<xiiObjectTest*>     m_SubObjectSet;

  xiiMap<xiiString, double>           m_StandardTypeMap;
  xiiHashTable<xiiString, OuterClass> m_ClassMap;
  xiiMap<xiiString, OuterClass*>      m_ClassPtrMap;
};


class xiiMirrorTest : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiMirrorTest, xiiReflectedClass);

public:
  xiiMirrorTest() = default;

  xiiMathClass  m_math;
  xiiObjectTest m_object;
};
