/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

namespace
{
#define CALL_FUNCTOR(functor, type) functor.template operator()<type>(std::forward<Args>(args)...)

  template <typename Functor, class... Args>
  void DispatchTo(Functor& ref_functor, const xiiAbstractProperty* pProp, Args&&... args)
  {
    const bool bIsPtr = pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer);
    if (bIsPtr)
    {
      CALL_FUNCTOR(ref_functor, xiiTypedPointer);
      return;
    }
    else if (pProp->GetSpecificType() == xiiGetStaticRTTI<const char*>())
    {
      CALL_FUNCTOR(ref_functor, const char*);
      return;
    }
    else if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiUntrackedString>())
    {
      CALL_FUNCTOR(ref_functor, xiiUntrackedString);
      return;
    }
    else if (pProp->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
    {
      CALL_FUNCTOR(ref_functor, xiiVariant);
      return;
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType))
    {
      xiiVariant::DispatchTo(ref_functor, pProp->GetSpecificType()->GetVariantType(), std::forward<Args>(args)...);
      return;
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::IsEnum))
    {
      CALL_FUNCTOR(ref_functor, xiiEnumBase);
      return;
    }
    else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Bitflags))
    {
      CALL_FUNCTOR(ref_functor, xiiBitflagsBase);
      return;
    }
    else if (pProp->GetSpecificType()->GetVariantType() == xiiVariantType::TypedObject)
    {
      CALL_FUNCTOR(ref_functor, xiiTypedObject);
      return;
    }

    XII_REPORT_FAILURE("Unknown dispatch type");
  }

#undef CALL_FUNCTOR

  struct GetTypeFromVariantTypeFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE void operator()()
    {
      m_pType = xiiGetStaticRTTI<T>();
    }
    const xiiRTTI* m_pType;
  };

  template <>
  XII_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<xiiTypedPointer>()
  {
    m_pType = nullptr;
  }
  template <>
  XII_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<xiiTypedObject>()
  {
    m_pType = nullptr;
  }

  //////////////////////////////////////////////////////////////////////////



  template <typename T>
  struct xiiPropertyValue
  {
    using Type        = T;
    using StorageType = typename xiiVariantTypeDeduction<T>::StorageType;
  };
  template <>
  struct xiiPropertyValue<xiiEnumBase>
  {
    using Type        = xiiInt64;
    using StorageType = xiiInt64;
  };
  template <>
  struct xiiPropertyValue<xiiBitflagsBase>
  {
    using Type        = xiiInt64;
    using StorageType = xiiInt64;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct xiiVariantFromProperty
  {
    xiiVariantFromProperty(xiiVariant& value, const xiiAbstractProperty* pProp) :
      m_value(value)
    {
      XII_IGNORE_UNUSED(pProp);
    }
    ~xiiVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = m_tempValue;
    }

    operator void*()
    {
      return &m_tempValue;
    }

    xiiVariant&                        m_value;
    typename xiiPropertyValue<T>::Type m_tempValue = {};
    bool                               m_bSuccess  = true;
  };

  template <>
  struct xiiVariantFromProperty<xiiVariant>
  {
    xiiVariantFromProperty(xiiVariant& value, const xiiAbstractProperty* pProp) :
      m_value(value)
    {
      XII_IGNORE_UNUSED(pProp);
    }

    operator void*()
    {
      return &m_value;
    }

    xiiVariant& m_value;
    bool        m_bSuccess = true;
  };

  template <>
  struct xiiVariantFromProperty<xiiTypedPointer>
  {
    xiiVariantFromProperty(xiiVariant& value, const xiiAbstractProperty* pProp) :
      m_value(value), m_pProp(pProp)
    {
      XII_IGNORE_UNUSED(pProp);
    }
    ~xiiVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = xiiVariant(m_pPtr, m_pProp->GetSpecificType());
    }

    operator void*()
    {
      return &m_pPtr;
    }

    xiiVariant&                m_value;
    const xiiAbstractProperty* m_pProp    = nullptr;
    void*                      m_pPtr     = nullptr;
    bool                       m_bSuccess = true;
  };

  template <>
  struct xiiVariantFromProperty<xiiTypedObject>
  {
    xiiVariantFromProperty(xiiVariant& value, const xiiAbstractProperty* pProp) :
      m_value(value), m_pProp(pProp)
    {
      m_pPtr = m_pProp->GetSpecificType()->GetAllocator()->Allocate<void>();
    }
    ~xiiVariantFromProperty()
    {
      if (m_bSuccess)
        m_value.MoveTypedObject(m_pPtr, m_pProp->GetSpecificType());
      else
        m_pProp->GetSpecificType()->GetAllocator()->Deallocate(m_pPtr);
    }

    operator void*()
    {
      return m_pPtr;
    }

    xiiVariant&                m_value;
    const xiiAbstractProperty* m_pProp    = nullptr;
    void*                      m_pPtr     = nullptr;
    bool                       m_bSuccess = true;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct xiiVariantToProperty
  {
    xiiVariantToProperty(const xiiVariant& value, const xiiAbstractProperty* pProp)
    {
      XII_IGNORE_UNUSED(pProp);

      m_tempValue = value.ConvertTo<typename xiiPropertyValue<T>::StorageType>();
    }

    operator const void*()
    {
      return &m_tempValue;
    }

    typename xiiPropertyValue<T>::Type m_tempValue = {};
  };

  template <>
  struct xiiVariantToProperty<const char*>
  {
    xiiVariantToProperty(const xiiVariant& value, const xiiAbstractProperty* pProp)
    {
      XII_IGNORE_UNUSED(pProp);

      m_sData  = value.ConvertTo<xiiString>();
      m_pValue = m_sData;
    }

    operator const void*()
    {
      return &m_pValue;
    }
    xiiString   m_sData;
    const char* m_pValue;
  };

  template <>
  struct xiiVariantToProperty<xiiVariant>
  {
    xiiVariantToProperty(const xiiVariant& value, const xiiAbstractProperty* pProp) :
      m_value(value)
    {
      XII_IGNORE_UNUSED(pProp);
    }

    operator const void*()
    {
      return const_cast<xiiVariant*>(&m_value);
    }

    const xiiVariant& m_value;
  };

  template <>
  struct xiiVariantToProperty<xiiTypedPointer>
  {
    xiiVariantToProperty(const xiiVariant& value, const xiiAbstractProperty* pProp)
    {
      XII_IGNORE_UNUSED(pProp);

      m_pPtr = value.Get<xiiTypedPointer>();

      XII_ASSERT_DEBUG(!m_pPtr.m_pType || m_pPtr.m_pType->IsDerivedFrom(pProp->GetSpecificType()), "Pointer of type '{0}' does not derive from '{}'", m_pPtr.m_pType->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }

    operator const void*()
    {
      return &m_pPtr.m_pObject;
    }

    xiiTypedPointer m_pPtr;
  };


  template <>
  struct xiiVariantToProperty<xiiTypedObject>
  {
    xiiVariantToProperty(const xiiVariant& value, const xiiAbstractProperty* pProp)
    {
      XII_IGNORE_UNUSED(pProp);

      m_pPtr = value.GetData();
    }

    operator const void*()
    {
      return m_pPtr;
    }
    const void* m_pPtr = nullptr;
  };

  //////////////////////////////////////////////////////////////////////////

  struct GetValueFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE void operator()(const xiiAbstractMemberProperty* pProp, const void* pObject, xiiVariant& value)
    {
      xiiVariantFromProperty<T> getter(value, pProp);
      pProp->GetValuePtr(pObject, getter);
    }
  };

  struct SetValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractMemberProperty* pProp, void* pObject, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->SetValuePtr(pObject, setter);
    }
  };

  struct GetArrayValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractArrayProperty* pProp, const void* pObject, xiiUInt32 uiIndex, xiiVariant& value)
    {
      xiiVariantFromProperty<T> getter(value, pProp);
      pProp->GetValue(pObject, uiIndex, getter);
    }
  };

  struct SetArrayValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->SetValue(pObject, uiIndex, setter);
    }
  };

  struct InsertArrayValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, uiIndex, setter);
    }
  };

  struct InsertSetValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, setter);
    }
  };

  struct RemoveSetValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->Remove(pObject, setter);
    }
  };

  struct GetMapValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractMapProperty* pProp, const void* pObject, xiiStringView sKey, xiiVariant& value)
    {
      xiiVariantFromProperty<T> getter(value, pProp);
      getter.m_bSuccess = pProp->GetValue(pObject, sKey, getter);
    }
  };

  struct SetMapValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()(const xiiAbstractMapProperty* pProp, void* pObject, xiiStringView sKey, const xiiVariant& value)
    {
      xiiVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, sKey, setter);
    }
  };

  static bool CompareProperties(const void* pObject, const void* pObject2, const xiiRTTI* pType)
  {
    if (pType->GetParentType())
    {
      if (!CompareProperties(pObject, pObject2, pType->GetParentType()))
        return false;
    }

    for (auto* pProp : pType->GetProperties())
    {
      if (!xiiReflectionUtils::IsEqual(pObject, pObject2, pProp))
        return false;
    }

    return true;
  }

  template <typename T>
  struct SetComponentValueImpl
  {
    XII_FORCE_INLINE static void impl(xiiVariant* pVector, xiiUInt32 uiComponent, double fValue)
    {
      XII_IGNORE_UNUSED(pVector);
      XII_IGNORE_UNUSED(uiComponent);
      XII_IGNORE_UNUSED(fValue);
      XII_ASSERT_DEBUG(false, "xiiReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct SetComponentValueImpl<xiiVec2Template<T>>
  {
    XII_FORCE_INLINE static void impl(xiiVariant* pVector, xiiUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<xiiVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<xiiVec3Template<T>>
  {
    XII_FORCE_INLINE static void impl(xiiVariant* pVector, xiiUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<xiiVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<xiiVec4Template<T>>
  {
    XII_FORCE_INLINE static void impl(xiiVariant* pVector, xiiUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<xiiVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
        case 3:
          vec.w = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  struct SetComponentValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()()
    {
      SetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    xiiVariant* m_pVector;
    xiiUInt32   m_iComponent;
    double      m_fValue;
  };

  template <typename T>
  struct GetComponentValueImpl
  {
    XII_FORCE_INLINE static void impl(const xiiVariant* pVector, xiiUInt32 uiComponent, double& out_fValue)
    {
      XII_IGNORE_UNUSED(pVector);
      XII_IGNORE_UNUSED(uiComponent);
      XII_IGNORE_UNUSED(out_fValue);
      XII_ASSERT_DEBUG(false, "xiiReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType());
    }
  };

  template <typename T>
  struct GetComponentValueImpl<xiiVec2Template<T>>
  {
    XII_FORCE_INLINE static void impl(const xiiVariant* pVector, xiiUInt32 uiComponent, double& out_fValue)
    {
      const auto& vec = pVector->Get<xiiVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<xiiVec3Template<T>>
  {
    XII_FORCE_INLINE static void impl(const xiiVariant* pVector, xiiUInt32 uiComponent, double& out_fValue)
    {
      const auto& vec = pVector->Get<xiiVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<xiiVec4Template<T>>
  {
    XII_FORCE_INLINE static void impl(const xiiVariant* pVector, xiiUInt32 uiComponent, double& out_fValue)
    {
      const auto& vec = pVector->Get<xiiVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          out_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          out_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          out_fValue = static_cast<double>(vec.z);
          break;
        case 3:
          out_fValue = static_cast<double>(vec.w);
          break;
      }
    }
  };

  struct GetComponentValueFunc
  {
    template <typename T>
    XII_FORCE_INLINE void operator()()
    {
      GetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    const xiiVariant* m_pVector;
    xiiUInt32         m_iComponent;
    double            m_fValue;
  };
} // namespace

const xiiRTTI* xiiReflectionUtils::GetCommonBaseType(const xiiRTTI* pRtti1, const xiiRTTI* pRtti2)
{
  if (pRtti2 == nullptr)
    return nullptr;

  while (pRtti1 != nullptr)
  {
    const xiiRTTI* pRtti2Parent = pRtti2;

    while (pRtti2Parent != nullptr)
    {
      if (pRtti1 == pRtti2Parent)
        return pRtti2Parent;

      pRtti2Parent = pRtti2Parent->GetParentType();
    }

    pRtti1 = pRtti1->GetParentType();
  }

  return nullptr;
}

bool xiiReflectionUtils::IsBasicType(const xiiRTTI* pRtti)
{
  XII_ASSERT_DEBUG(pRtti != nullptr, "IsBasicType: missing data!");
  xiiVariant::Type::Enum type = pRtti->GetVariantType();
  return (type >= xiiVariant::Type::FirstStandardType && type <= xiiVariant::Type::LastStandardType) || pRtti == xiiGetStaticRTTI<xiiVariant>();
}

bool xiiReflectionUtils::IsValueType(const xiiAbstractProperty* pProp)
{
  return !pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && (pProp->GetFlags().IsSet(xiiPropertyFlags::StandardType) || xiiVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProp->GetSpecificType()));
}

const xiiRTTI* xiiReflectionUtils::GetTypeFromVariant(const xiiVariant& value)
{
  return value.GetReflectedType();
}

const xiiRTTI* xiiReflectionUtils::GetTypeFromVariant(xiiVariantType::Enum type)
{
  GetTypeFromVariantTypeFunc func;
  func.m_pType = nullptr;
  xiiVariant::DispatchTo(func, type);

  return func.m_pType;
}

xiiUInt32 xiiReflectionUtils::GetComponentCount(xiiVariantType::Enum type)
{
  switch (type)
  {
    case xiiVariant::Type::Vector2:
    case xiiVariant::Type::Vector2d:
    case xiiVariant::Type::Vector2I:
    case xiiVariant::Type::Vector2I64:
    case xiiVariant::Type::Vector2U:
    case xiiVariant::Type::Vector2U64:
      return 2;
    case xiiVariant::Type::Vector3:
    case xiiVariant::Type::Vector3d:
    case xiiVariant::Type::Vector3I:
    case xiiVariant::Type::Vector3I64:
    case xiiVariant::Type::Vector3U:
    case xiiVariant::Type::Vector3U64:
      return 3;
    case xiiVariant::Type::Vector4:
    case xiiVariant::Type::Vector4d:
    case xiiVariant::Type::Vector4I:
    case xiiVariant::Type::Vector4I64:
    case xiiVariant::Type::Vector4U:
    case xiiVariant::Type::Vector4U64:
      return 4;
    default:
      XII_REPORT_FAILURE("Not a vector type: '{0}'", type);
      return 0;
  }
}

void xiiReflectionUtils::SetComponent(xiiVariant& ref_vector, xiiUInt32 uiComponent, double fValue)
{
  SetComponentValueFunc func;
  func.m_pVector    = &ref_vector;
  func.m_iComponent = uiComponent;
  func.m_fValue     = fValue;
  xiiVariant::DispatchTo(func, ref_vector.GetType());
}

double xiiReflectionUtils::GetComponent(const xiiVariant& vector, xiiUInt32 uiComponent)
{
  GetComponentValueFunc func;
  func.m_pVector    = &vector;
  func.m_iComponent = uiComponent;
  xiiVariant::DispatchTo(func, vector.GetType());
  return func.m_fValue;
}

xiiVariant xiiReflectionUtils::GetMemberPropertyValue(const xiiAbstractMemberProperty* pProp, const void* pObject)
{
  xiiVariant res;
  XII_ASSERT_DEBUG(pProp != nullptr, "GetMemberPropertyValue: missing data!");

  GetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, res);

  return res;
}

void xiiReflectionUtils::SetMemberPropertyValue(const xiiAbstractMemberProperty* pProp, void* pObject, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMemberPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::Bitflags | xiiPropertyFlags::IsEnum))
  {
    auto pEnumerationProp = static_cast<const xiiAbstractEnumerationProperty*>(pProp);

    // Value can either be an integer or a string (human readable value)
    if (value.IsA<xiiString>())
    {
      xiiInt64 iValue;
      xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<xiiString>(), iValue);
      pEnumerationProp->SetValue(pObject, iValue);
    }
    else if (value.IsA<xiiStringView>())
    {
      xiiInt64 iValue;
      xiiReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<xiiStringView>(), iValue);
      pEnumerationProp->SetValue(pObject, iValue);
    }
    else
    {
      pEnumerationProp->SetValue(pObject, value.ConvertTo<xiiInt64>());
    }
  }
  else
  {
    SetValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, value);
  }
}

xiiVariant xiiReflectionUtils::GetArrayPropertyValue(const xiiAbstractArrayProperty* pProp, const void* pObject, xiiUInt32 uiIndex)
{
  xiiVariant res;
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    xiiLog::Error("GetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    GetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, res);
  }
  return res;
}

void xiiReflectionUtils::SetArrayPropertyValue(const xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    xiiLog::Error("SetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    SetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
  }
}

void xiiReflectionUtils::InsertSetPropertyValue(const xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  InsertSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

void xiiReflectionUtils::RemoveSetPropertyValue(const xiiAbstractSetProperty* pProp, void* pObject, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  RemoveSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

xiiVariant xiiReflectionUtils::GetMapPropertyValue(const xiiAbstractMapProperty* pProp, const void* pObject, xiiStringView sKey)
{
  xiiVariant value;
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetMapPropertyValue: missing data!");

  GetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, sKey, value);
  return value;
}

void xiiReflectionUtils::SetMapPropertyValue(const xiiAbstractMapProperty* pProp, void* pObject, xiiStringView sKey, const xiiVariant& value)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMapPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  SetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, sKey, value);
}

void xiiReflectionUtils::InsertArrayPropertyValue(const xiiAbstractArrayProperty* pProp, void* pObject, const xiiVariant& value, xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex > uiCount)
  {
    xiiLog::Error("InsertArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  InsertArrayValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
}

void xiiReflectionUtils::RemoveArrayPropertyValue(const xiiAbstractArrayProperty* pProp, void* pObject, xiiUInt32 uiIndex)
{
  XII_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(xiiPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    xiiLog::Error("RemoveArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  pProp->Remove(pObject, uiIndex);
}

const xiiAbstractMemberProperty* xiiReflectionUtils::GetMemberProperty(const xiiRTTI* pRtti, xiiUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  xiiTemporaryHybridArray<const xiiAbstractProperty*, 32> props;
  pRtti->GetAllProperties(props);
  if (uiPropertyIndex < props.GetCount())
  {
    const xiiAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == xiiPropertyCategory::Member)
      return static_cast<const xiiAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

const xiiAbstractMemberProperty* xiiReflectionUtils::GetMemberProperty(const xiiRTTI* pRtti, xiiStringView sPropertyName)
{
  if (pRtti == nullptr)
    return nullptr;

  if (const xiiAbstractProperty* pProp = pRtti->FindPropertyByName(sPropertyName))
  {
    if (pProp->GetCategory() == xiiPropertyCategory::Member)
      return static_cast<const xiiAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

void xiiReflectionUtils::GatherTypesDerivedFromClass(const xiiRTTI* pBaseRtti, xiiSet<const xiiRTTI*>& out_types)
{
  xiiRTTI::ForEachDerivedType(pBaseRtti,
                              [&](const xiiRTTI* pRtti) {
                                out_types.Insert(pRtti);
                              });
}

void xiiReflectionUtils::GatherDependentTypes(const xiiRTTI* pRtti, xiiSet<const xiiRTTI*>& inout_typesAsSet, xiiDynamicArray<const xiiRTTI*>* out_pTypesAsStack /*= nullptr*/)
{
  auto AddType = [&](const xiiRTTI* pNewRtti) {
    if (pNewRtti != pRtti && pNewRtti->GetTypeFlags().IsSet(xiiTypeFlags::StandardType) == false && inout_typesAsSet.Contains(pNewRtti) == false)
    {
      inout_typesAsSet.Insert(pNewRtti);
      if (out_pTypesAsStack != nullptr)
      {
        out_pTypesAsStack->PushBack(pNewRtti);
      }

      GatherDependentTypes(pNewRtti, inout_typesAsSet, out_pTypesAsStack);
    }
  };

  if (const xiiRTTI* pParentRtti = pRtti->GetParentType())
  {
    AddType(pParentRtti);
  }

  for (const xiiAbstractProperty* prop : pRtti->GetProperties())
  {
    if (prop->GetCategory() == xiiPropertyCategory::Constant)
      continue;

    if (prop->GetAttributeByType<xiiTemporaryAttribute>() != nullptr)
      continue;

    AddType(prop->GetSpecificType());
  }

  for (const xiiAbstractFunctionProperty* func : pRtti->GetFunctions())
  {
    xiiUInt32 uiNumArgs = func->GetArgumentCount();
    for (xiiUInt32 i = 0; i < uiNumArgs; ++i)
    {
      AddType(func->GetArgumentType(i));
    }
  }

  for (const xiiPropertyAttribute* attr : pRtti->GetAttributes())
  {
    AddType(attr->GetDynamicRTTI());
  }
}

xiiResult xiiReflectionUtils::CreateDependencySortedTypeArray(const xiiSet<const xiiRTTI*>& types, xiiDynamicArray<const xiiRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  xiiSet<const xiiRTTI*>          accu;
  xiiDynamicArray<const xiiRTTI*> tmpStack;

  for (const xiiRTTI* pType : types)
  {
    if (accu.Contains(pType))
      continue;

    GatherDependentTypes(pType, accu, &tmpStack);

    while (tmpStack.IsEmpty() == false)
    {
      const xiiRTTI* pDependentType = tmpStack.PeekBack();
      XII_ASSERT_DEBUG(pDependentType != pType, "A type must not be reported as dependency of itself.");
      tmpStack.PopBack();

      if (types.Contains(pDependentType) == false)
        return XII_FAILURE;

      out_sortedTypes.PushBack(pDependentType);
    }

    accu.Insert(pType);
    out_sortedTypes.PushBack(pType);
  }

  XII_ASSERT_DEV(types.GetCount() == out_sortedTypes.GetCount(), "Not all types have been sorted or the sorted list contains duplicates.");
  return XII_SUCCESS;
}

bool xiiReflectionUtils::EnumerationToString(const xiiRTTI* pEnumerationRtti, xiiInt64 iValue, xiiStringBuilder& out_sOutput, xiiEnum<EnumConversionMode> conversionMode)
{
  out_sOutput.Clear();
  if (pEnumerationRtti->IsDerivedFrom<xiiEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        xiiVariant value = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant();
        if (value.ConvertTo<xiiInt64>() == iValue)
        {
          out_sOutput = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : pProp->GetPropertyName().FindLastSubString("::") + 2;
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<xiiBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        xiiVariant value = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant();
        if ((value.ConvertTo<xiiInt64>() & iValue) != 0)
        {
          out_sOutput.Append(conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : pProp->GetPropertyName().FindLastSubString("::") + 2, "|");
        }
      }
    }
    out_sOutput.Shrink(0, 1);
    return true;
  }
  else
  {
    XII_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class.", pEnumerationRtti->GetTypeName());
    return false;
  }
}

void xiiReflectionUtils::GetEnumKeysAndValues(const xiiRTTI* pEnumerationRtti, xiiDynamicArray<EnumKeyValuePair>& ref_entries, xiiEnum<EnumConversionMode> conversionMode)
{
  /// \test This is new.

  ref_entries.Clear();

  if (pEnumerationRtti->IsDerivedFrom<xiiEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        xiiVariant value = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant();

        auto& e    = ref_entries.ExpandAndGetRef();
        e.m_sKey   = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : pProp->GetPropertyName().FindLastSubString("::") + 2;
        e.m_iValue = value.ConvertTo<xiiInt32>();
      }
    }
  }
}

bool xiiReflectionUtils::StringToEnumeration(const xiiRTTI* pEnumerationRtti, xiiStringView sValue, xiiInt64& out_iValue)
{
  out_iValue = 0;
  if (pEnumerationRtti->IsDerivedFrom<xiiEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        // Testing fully qualified and short value name
        const char* valueNameOnly = pProp->GetPropertyName().FindLastSubString("::", nullptr);
        if ((pProp->GetPropertyName() == sValue) || (valueNameOnly != nullptr && sValue.IsEqual(valueNameOnly + 2)))
        {
          xiiVariant value = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant();
          out_iValue       = value.ConvertTo<xiiInt64>();
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<xiiBitflagsBase>())
  {
    xiiStringBuilder                  temp = sValue;
    xiiTemporaryHybridArray<xiiStringView, 32> values;
    temp.Split(false, values, "|");
    for (auto sValueSplit : values)
    {
      for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
      {
        if (pProp->GetCategory() == xiiPropertyCategory::Constant)
        {
          // Testing fully qualified and short value name
          const char* valueNameOnly = pProp->GetPropertyName().FindLastSubString("::", nullptr);
          if (sValueSplit.IsEqual(pProp->GetPropertyName()) || (valueNameOnly != nullptr && sValueSplit.IsEqual(valueNameOnly + 2)))
          {
            xiiVariant value = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant();
            out_iValue |= value.ConvertTo<xiiInt64>();
          }
        }
      }
    }
    return true;
  }
  else
  {
    XII_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class.", pEnumerationRtti->GetTypeName());
    return false;
  }
}

xiiInt64 xiiReflectionUtils::DefaultEnumerationValue(const xiiRTTI* pEnumerationRtti)
{
  if (pEnumerationRtti->IsDerivedFrom<xiiEnumBase>() || pEnumerationRtti->IsDerivedFrom<xiiBitflagsBase>())
  {
    auto pProp = pEnumerationRtti->GetProperties()[0];
    XII_ASSERT_DEBUG(pProp->GetCategory() == xiiPropertyCategory::Constant && pProp->GetPropertyName().EndsWith("::Default"), "First enumeration property must be the default value constant.");
    return static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<xiiInt64>();
  }
  else
  {
    XII_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class.", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

xiiInt64 xiiReflectionUtils::MakeEnumerationValid(const xiiRTTI* pEnumerationRtti, xiiInt64 iValue)
{
  if (pEnumerationRtti->IsDerivedFrom<xiiEnumBase>())
  {
    // Find current value
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        xiiInt64 iCurrentValue = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<xiiInt64>();
        if (iCurrentValue == iValue)
          return iValue;
      }
    }

    // Current value not found, return default value
    return xiiReflectionUtils::DefaultEnumerationValue(pEnumerationRtti);
  }
  else if (pEnumerationRtti->IsDerivedFrom<xiiBitflagsBase>())
  {
    xiiInt64 iNewValue = 0;
    // Filter valid bits
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == xiiPropertyCategory::Constant)
      {
        xiiInt64 iCurrentValue = static_cast<const xiiAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<xiiInt64>();
        if ((iCurrentValue & iValue) != 0)
        {
          iNewValue |= iCurrentValue;
        }
      }
    }
    return iNewValue;
  }
  else
  {
    XII_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class.", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

bool xiiReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const xiiAbstractProperty* pProp)
{
  // #VAR TEST
  const xiiRTTI* pPropType = pProp->GetSpecificType();

  xiiVariant vTemp;
  xiiVariant vTemp2;

  const bool bIsValueType = xiiReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const xiiAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        vTemp  = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        vTemp2 = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);

        void* pRefrencedObject  = vTemp.ConvertTo<void*>();
        void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();

        if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
          return false;
        if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
          return true;

        if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
        {
          return IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
        }
        else
        {
          return pRefrencedObject == pRefrencedObject2;
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(xiiPropertyFlags::IsEnum | xiiPropertyFlags::Bitflags) || bIsValueType)
        {
          vTemp  = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          vTemp2 = xiiReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
          return vTemp == vTemp2;
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
        {
          void* pSubObject  = pSpecific->GetPropertyPointer(pObject);
          void* pSubObject2 = pSpecific->GetPropertyPointer(pObject2);

          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return IsEqual(pSubObject, pSubObject2, pPropType);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject  = pPropType->GetAllocator()->Allocate<void>();
            pSubObject2 = pPropType->GetAllocator()->Allocate<void>();

            pSpecific->GetValuePtr(pObject, pSubObject);
            pSpecific->GetValuePtr(pObject2, pSubObject2);

            bool bEqual = IsEqual(pSubObject, pSubObject2, pPropType);

            pPropType->GetAllocator()->Deallocate(pSubObject);
            pPropType->GetAllocator()->Deallocate(pSubObject2);

            return bEqual;
          }
          else
          {
            // TODO: return false if prop can't be compared?
            return true;
          }
        }
      }
    }
    break;
    case xiiPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const xiiAbstractArrayProperty*>(pProp);

      const xiiUInt32 uiCount  = pSpecific->GetCount(pObject);
      const xiiUInt32 uiCount2 = pSpecific->GetCount(pObject2);

      if (uiCount != uiCount2)
        return false;

      if (pSpecific->GetFlags().IsSet(xiiPropertyFlags::Pointer))
      {
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp  = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          vTemp2 = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);

          void* pRefrencedObject  = vTemp.ConvertTo<void*>();
          void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();

          if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
            return false;
          if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
            continue;

          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            if (!IsEqual(pRefrencedObject, pRefrencedObject2, pPropType))
              return false;
          }
          else
          {
            if (pRefrencedObject != pRefrencedObject2)
              return false;
          }
        }
        return true;
      }
      else
      {
        if (bIsValueType)
        {
          for (xiiUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp  = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            vTemp2 = xiiReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);

            if (vTemp != vTemp2)
              return false;
          }
          return true;
        }
        else if (pProp->GetFlags().IsSet(xiiPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject  = pPropType->GetAllocator()->Allocate<void>();
          void* pSubObject2 = pPropType->GetAllocator()->Allocate<void>();

          bool bEqual = true;
          for (xiiUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            pSpecific->GetValue(pObject2, i, pSubObject2);

            bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            if (!bEqual)
              break;
          }

          pPropType->GetAllocator()->Deallocate(pSubObject);
          pPropType->GetAllocator()->Deallocate(pSubObject2);
          return bEqual;
        }
      }
    }
    break;
    case xiiPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const xiiAbstractSetProperty*>(pProp);

      xiiTemporaryHybridArray<xiiVariant, 16> values;
      pSpecific->GetValues(pObject, values);
      xiiTemporaryHybridArray<xiiVariant, 16> values2;
      pSpecific->GetValues(pObject2, values2);

      const xiiUInt32 uiCount  = values.GetCount();
      const xiiUInt32 uiCount2 = values2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = values2.Contains(values[i]);
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if (pProp->GetFlags().AreAllSet(xiiPropertyFlags::Pointer | xiiPropertyFlags::PointerOwner))
      {
        // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
        bool bEqual = true;
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          if (pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner))
          {
            void* pRefrencedObject  = values[i].ConvertTo<void*>();
            void* pRefrencedObject2 = values2[i].ConvertTo<void*>();

            if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
              return false;
            if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
              continue;

            bEqual = IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
          }
          if (!bEqual)
            break;
        }

        return bEqual;
      }
    }
    break;
    case xiiPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const xiiAbstractMapProperty*>(pProp);

      xiiTemporaryHybridArray<xiiString, 16> keys;
      pSpecific->GetKeys(pObject, keys);
      xiiTemporaryHybridArray<xiiString, 16> keys2;
      pSpecific->GetKeys(pObject2, keys2);

      const xiiUInt32 uiCount  = keys.GetCount();
      const xiiUInt32 uiCount2 = keys2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;

          xiiVariant value1 = GetMapPropertyValue(pSpecific, pObject, keys[i]);
          xiiVariant value2 = GetMapPropertyValue(pSpecific, pObject2, keys[i]);
          bEqual            = value1 == value2;
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if ((!pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer) || pProp->GetFlags().IsSet(xiiPropertyFlags::PointerOwner)) && pProp->GetFlags().IsSet(xiiPropertyFlags::Class))
      {
        bool bEqual = true;
        for (xiiUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;

          if (pProp->GetFlags().IsSet(xiiPropertyFlags::Pointer))
          {
            const void* value1 = nullptr;
            const void* value2 = nullptr;

            pSpecific->GetValue(pObject, keys[i], &value1);
            pSpecific->GetValue(pObject2, keys[i], &value2);

            if ((value1 == nullptr) != (value2 == nullptr))
              return false;
            if ((value1 == nullptr) && (value2 == nullptr))
              continue;

            bEqual = IsEqual(value1, value2, pPropType);
          }
          else
          {
            if (pPropType->GetAllocator()->CanAllocate())
            {
              void* value1 = pPropType->GetAllocator()->Allocate<void>();
              XII_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value1););

              void* value2 = pPropType->GetAllocator()->Allocate<void>();
              XII_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value2););

              bool bRes1 = pSpecific->GetValue(pObject, keys[i], value1);
              bool bRes2 = pSpecific->GetValue(pObject2, keys[i], value2);

              if (bRes1 != bRes2)
                return false;
              if (!bRes1 && !bRes2)
                continue;

              bEqual = IsEqual(value1, value2, pPropType);
            }
            else
            {
              xiiLog::Error("The property '{0}' can not be compared as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
            }
          }
          if (!bEqual)
            break;
        }
        return bEqual;
      }
    }
    break;

      XII_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
  return true;
}

bool xiiReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const xiiRTTI* pType)
{
  XII_ASSERT_DEV(pObject && pObject2 && pType, "invalid type.");

  if (pType->IsDerivedFrom<xiiReflectedClass>())
  {
    const xiiReflectedClass* pRefObject  = static_cast<const xiiReflectedClass*>(pObject);
    const xiiReflectedClass* pRefObject2 = static_cast<const xiiReflectedClass*>(pObject2);
    pType                                = pRefObject->GetDynamicRTTI();
    if (pType != pRefObject2->GetDynamicRTTI())
      return false;
  }

  return CompareProperties(pObject, pObject2, pType);
}


void xiiReflectionUtils::DeleteObject(void* pObject, const xiiAbstractProperty* pOwnerProperty)
{
  if (!pObject)
    return;

  const xiiRTTI* pType = pOwnerProperty->GetSpecificType();
  if (pType->IsDerivedFrom<xiiReflectedClass>())
  {
    xiiReflectedClass* pRefObject = static_cast<xiiReflectedClass*>(pObject);
    pType                         = pRefObject->GetDynamicRTTI();
  }

  if (!pType->GetAllocator()->CanAllocate())
  {
    xiiLog::Error("Tried to deallocate object of type '{0}', but it has no allocator.", pType->GetTypeName());
    return;
  }
  pType->GetAllocator()->Deallocate(pObject);
}

xiiVariant xiiReflectionUtils::GetDefaultVariantFromType(xiiVariant::Type::Enum type)
{
  switch (type)
  {
    case xiiVariant::Type::Invalid:
      return xiiVariant();
    case xiiVariant::Type::Bool:
      return xiiVariant(false);
    case xiiVariant::Type::Int8:
      return xiiVariant((xiiInt8)0);
    case xiiVariant::Type::UInt8:
      return xiiVariant((xiiUInt8)0);
    case xiiVariant::Type::Int16:
      return xiiVariant((xiiInt16)0);
    case xiiVariant::Type::UInt16:
      return xiiVariant((xiiUInt16)0);
    case xiiVariant::Type::Int32:
      return xiiVariant((xiiInt32)0);
    case xiiVariant::Type::UInt32:
      return xiiVariant((xiiUInt32)0);
    case xiiVariant::Type::Int64:
      return xiiVariant((xiiInt64)0);
    case xiiVariant::Type::UInt64:
      return xiiVariant((xiiUInt64)0);
    case xiiVariant::Type::Float:
      return xiiVariant(0.0f);
    case xiiVariant::Type::Double:
      return xiiVariant(0.0);
    case xiiVariant::Type::Color:
      return xiiVariant(xiiColor(1.0f, 1.0f, 1.0f));
    case xiiVariant::Type::ColorGamma:
      return xiiVariant(xiiColorGammaUB(255, 255, 255));
    case xiiVariant::Type::Vector2:
      return xiiVariant(xiiVec2(0.0f, 0.0f));
    case xiiVariant::Type::Vector2d:
      return xiiVariant(xiiVec2d(0.0, 0.0));
    case xiiVariant::Type::Vector3:
      return xiiVariant(xiiVec3(0.0f, 0.0f, 0.0f));
    case xiiVariant::Type::Vector3d:
      return xiiVariant(xiiVec3d(0.0, 0.0, 0.0));
    case xiiVariant::Type::Vector4:
      return xiiVariant(xiiVec4(0.0f, 0.0f, 0.0f, 0.0f));
    case xiiVariant::Type::Vector4d:
      return xiiVariant(xiiVec4d(0.0, 0.0, 0.0, 0.0));
    case xiiVariant::Type::Vector2I:
      return xiiVariant(xiiVec2I32(0, 0));
    case xiiVariant::Type::Vector2I64:
      return xiiVariant(xiiVec2I64(0, 0));
    case xiiVariant::Type::Vector3I:
      return xiiVariant(xiiVec3I32(0, 0, 0));
    case xiiVariant::Type::Vector3I64:
      return xiiVariant(xiiVec3I64(0, 0, 0));
    case xiiVariant::Type::Vector4I:
      return xiiVariant(xiiVec4I32(0, 0, 0, 0));
    case xiiVariant::Type::Vector4I64:
      return xiiVariant(xiiVec4I64(0, 0, 0, 0));
    case xiiVariant::Type::Vector2U:
      return xiiVariant(xiiVec2U32(0, 0));
    case xiiVariant::Type::Vector2U64:
      return xiiVariant(xiiVec2U64(0, 0));
    case xiiVariant::Type::Vector3U:
      return xiiVariant(xiiVec3U32(0, 0, 0));
    case xiiVariant::Type::Vector3U64:
      return xiiVariant(xiiVec3U64(0, 0, 0));
    case xiiVariant::Type::Vector4U:
      return xiiVariant(xiiVec4U32(0, 0, 0, 0));
    case xiiVariant::Type::Vector4U64:
      return xiiVariant(xiiVec4U64(0, 0, 0, 0));
    case xiiVariant::Type::Quaternion:
      return xiiVariant(xiiQuat(0.0f, 0.0f, 0.0f, 1.0f));
    case xiiVariant::Type::Quaterniond:
      return xiiVariant(xiiQuatd(0.0, 0.0, 0.0, 1.0));
    case xiiVariant::Type::Matrix3:
      return xiiVariant(xiiMat3::MakeIdentity());
    case xiiVariant::Type::Matrix3d:
      return xiiVariant(xiiMat3d::MakeIdentity());
    case xiiVariant::Type::Matrix4:
      return xiiVariant(xiiMat4::MakeIdentity());
    case xiiVariant::Type::Matrix4d:
      return xiiVariant(xiiMat4d::MakeIdentity());
    case xiiVariant::Type::Transform:
      return xiiVariant(xiiTransform::MakeIdentity());
    case xiiVariant::Type::Transformd:
      return xiiVariant(xiiTransformd::MakeIdentity());
    case xiiVariant::Type::String:
      return xiiVariant(xiiString());
    case xiiVariant::Type::StringView:
      return xiiVariant(xiiStringView(), false);
    case xiiVariant::Type::HashedString:
      return xiiVariant(xiiHashedString());
    case xiiVariant::Type::TempHashedString:
      return xiiVariant(xiiTempHashedString());
    case xiiVariant::Type::DataBuffer:
      return xiiVariant(xiiDataBuffer());
    case xiiVariant::Type::Time:
      return xiiVariant(xiiTime());
    case xiiVariant::Type::Uuid:
      return xiiVariant(xiiUuid());
    case xiiVariant::Type::Angle:
      return xiiVariant(xiiAngle());
    case xiiVariant::Type::Angled:
      return xiiVariant(xiiAngled());
    case xiiVariant::Type::VariantArray:
      return xiiVariantArray();
    case xiiVariant::Type::VariantDictionary:
      return xiiVariantDictionary();
    case xiiVariant::Type::TypedPointer:
      return xiiVariant(static_cast<void*>(nullptr), nullptr);

    default:
      XII_REPORT_FAILURE("Invalid case statement");
      return xiiVariant();
  }
}

xiiVariant xiiReflectionUtils::GetDefaultValue(const xiiAbstractProperty* pProperty, xiiVariant index)
{
  const bool                      isValueType = xiiReflectionUtils::IsValueType(pProperty);
  const xiiVariantType::Enum      type        = pProperty->GetFlags().IsSet(xiiPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(xiiPropertyFlags::Class) && !isValueType) ? xiiVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();
  const xiiDefaultValueAttribute* pAttrib     = pProperty->GetAttributeByType<xiiDefaultValueAttribute>();

  switch (pProperty->GetCategory())
  {
    case xiiPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pProperty->GetSpecificType() == xiiGetStaticRTTI<xiiVariant>())
            return pAttrib->GetValue();
          if (pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(xiiTypeFlags::IsEnum | xiiTypeFlags::Bitflags))
      {
        xiiInt64 iValue = xiiReflectionUtils::DefaultEnumerationValue(pProperty->GetSpecificType());
        if (pAttrib)
        {
          if (pAttrib->GetValue().CanConvertTo(xiiVariantType::Int64))
            iValue = pAttrib->GetValue().ConvertTo<xiiInt64>();
        }
        return xiiReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue);
      }
      else // Class
      {
        return xiiUuid();
      }
    }
    break;
    case xiiPropertyCategory::Array:
    case xiiPropertyCategory::Set:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<xiiVariantArray>())
          {
            if (!index.IsValid())
              return pAttrib->GetValue();

            xiiUInt32   iIndex       = index.ConvertTo<xiiUInt32>();
            const auto& defaultArray = pAttrib->GetValue().Get<xiiVariantArray>();
            if (iIndex < defaultArray.GetCount())
            {
              return defaultArray[iIndex];
            }
            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return xiiVariantArray();

        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return xiiVariantArray();

        return xiiUuid();
      }
      break;
    case xiiPropertyCategory::Map:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<xiiVariantDictionary>())
          {
            if (!index.IsValid())
            {
              return pAttrib->GetValue();
            }
            xiiString   sKey        = index.ConvertTo<xiiString>();
            const auto& defaultDict = pAttrib->GetValue().Get<xiiVariantDictionary>();
            if (auto it = defaultDict.Find(sKey); it.IsValid())
              return it.Value();

            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return xiiVariantDictionary();
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return xiiVariantDictionary();

        return xiiUuid();
      }
      break;
    default:
      break;
  }

  XII_REPORT_FAILURE("Don't reach here");
  return xiiVariant();
}

xiiVariant xiiReflectionUtils::GetDefaultVariantFromType(const xiiRTTI* pRtti)
{
  xiiVariantType::Enum type = pRtti->GetVariantType();
  switch (type)
  {
    case xiiVariant::Type::TypedObject:
    {
      xiiVariant val;
      val.MoveTypedObject(pRtti->GetAllocator()->Allocate<void>(), pRtti);
      return val;
    }
    break;

    default:
      return GetDefaultVariantFromType(type);
  }
}

void xiiReflectionUtils::SetAllMemberPropertiesToDefault(const xiiRTTI* pRtti, void* pObject)
{
  xiiTemporaryHybridArray<const xiiAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == xiiPropertyCategory::Member)
    {
      const xiiVariant defValue = xiiReflectionUtils::GetDefaultValue(pProp);

      xiiReflectionUtils::SetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(pProp), pObject, defValue);
    }
  }
}

namespace
{
  template <class C>
  struct xiiClampCategoryType
  {
    static constexpr xiiInt32 value = (((xiiVariant::TypeDeduction<C>::value >= xiiVariantType::Int8 && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::Double) || (xiiVariant::TypeDeduction<C>::value == xiiVariantType::Time) || (xiiVariant::TypeDeduction<C>::value == xiiVariantType::Angle) || (xiiVariant::TypeDeduction<C>::value == xiiVariantType::Angled))) + ((xiiVariant::TypeDeduction<C>::value >= xiiVariantType::Vector2I && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::Vector4d) * 2);
  };

  template <typename T, xiiInt32 V = xiiClampCategoryType<T>::value>
  struct ClampVariantFuncImpl
  {
    static XII_ALWAYS_INLINE xiiResult Func(xiiVariant& value, const xiiClampValueAttribute* pAttrib)
    {
      XII_IGNORE_UNUSED(value);
      XII_IGNORE_UNUSED(pAttrib);
      return XII_FAILURE;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 1> // scalar types
  {
    static XII_ALWAYS_INLINE xiiResult Func(xiiVariant& value, const xiiClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = xiiMath::Max(value.ConvertTo<T>(), pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = xiiMath::Min(value.ConvertTo<T>(), pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return XII_SUCCESS;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 2> // vector types
  {
    static XII_ALWAYS_INLINE xiiResult Func(xiiVariant& value, const xiiClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMax(pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMin(pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return XII_SUCCESS;
    }
  };

  struct ClampVariantFunc
  {
    template <typename T>
    XII_ALWAYS_INLINE xiiResult operator()(xiiVariant& value, const xiiClampValueAttribute* pAttrib)
    {
      return ClampVariantFuncImpl<T>::Func(value, pAttrib);
    }
  };
} // namespace

xiiResult xiiReflectionUtils::ClampValue(xiiVariant& value, const xiiClampValueAttribute* pAttrib)
{
  xiiVariantType::Enum type = value.GetType();
  if (type == xiiVariantType::Invalid || pAttrib == nullptr)
    return XII_SUCCESS; // If there is nothing to clamp or no clamp attribute we call it a success.

  ClampVariantFunc func;
  return xiiVariant::DispatchTo(func, type, value, pAttrib);
}

XII_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_ReflectionUtils);
