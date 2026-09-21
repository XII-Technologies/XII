/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

XII_WARNING_PUSH()
XII_WARNING_DISABLE_CLANG("-Wunused-local-typedef")
XII_WARNING_DISABLE_GCC("-Wunused-local-typedefs")

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType* d = xiiStaticCast<DerivedType*>(pObj);
template <typename T>
XII_ALWAYS_INLINE T xiiStaticCast(xiiReflectedClass* pObject)
{
  using NonPointerT = typename xiiTypeTraits<T>::NonPointerType;
  XII_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
                 pObject->GetDynamicRTTI()->GetTypeName(), xiiGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType* d = xiiStaticCast<const DerivedType*>(pConstObj);
template <typename T>
XII_ALWAYS_INLINE T xiiStaticCast(const xiiReflectedClass* pObject)
{
  using NonPointerT = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  XII_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
                 pObject->GetDynamicRTTI()->GetTypeName(), xiiGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType& d = xiiStaticCast<DerivedType&>(obj);
template <typename T>
XII_ALWAYS_INLINE T xiiStaticCast(xiiReflectedClass& ref_object)
{
  using NonReferenceT = typename xiiTypeTraits<T>::NonReferenceType;
  XII_ASSERT_DEV(ref_object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
                 ref_object.GetDynamicRTTI()->GetTypeName(), xiiGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(ref_object);
}

/// Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType& d = xiiStaticCast<const DerivedType&>(constObj);
template <typename T>
XII_ALWAYS_INLINE T xiiStaticCast(const xiiReflectedClass& object)
{
  using NonReferenceT = typename xiiTypeTraits<T>::NonConstReferenceType;
  XII_ASSERT_DEV(object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
                 object.GetDynamicRTTI()->GetTypeName(), xiiGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(object);
}

/// Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. DerivedType* d = xiiDynamicCast<DerivedType*>(pObj);
template <typename T>
XII_ALWAYS_INLINE T xiiDynamicCast(xiiReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename xiiTypeTraits<T>::NonPointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

/// Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. const DerivedType* d = xiiDynamicCast<const DerivedType*>(pConstObj);
template <typename T>
XII_ALWAYS_INLINE T xiiDynamicCast(const xiiReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename xiiTypeTraits<T>::NonConstReferencePointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

XII_WARNING_POP()
