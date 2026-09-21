/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Types/Variant.h>

template <typename T>
struct xiiCleanType2
{
  using Type     = T;
  using RttiType = T;
};

template <typename T>
struct xiiCleanType2<xiiEnum<T>>
{
  using Type     = xiiEnum<T>;
  using RttiType = T;
};

template <typename T>
struct xiiCleanType2<xiiBitflags<T>>
{
  using Type     = xiiBitflags<T>;
  using RttiType = T;
};

template <typename T>
struct xiiCleanType
{
  using Type     = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  using RttiType = typename xiiCleanType2<typename xiiTypeTraits<T>::NonConstReferencePointerType>::RttiType;
};

template <>
struct xiiCleanType<const char*>
{
  using Type     = const char*;
  using RttiType = const char*;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct xiiIsOutParam
{
  static constexpr bool value = false;
};

template <typename T>
struct xiiIsOutParam<T&>
{
  static constexpr bool value = !std::is_const<typename xiiTypeTraits<T>::NonReferencePointerType>::value;
};

template <typename T>
struct xiiIsOutParam<T*>
{
  static constexpr bool value = !std::is_const<typename xiiTypeTraits<T>::NonReferencePointerType>::value;
};

//////////////////////////////////////////////////////////////////////////

/// Used to determine if the given type is a build-in standard variant type.
template <class T, class C = typename xiiCleanType<T>::Type>
struct xiiIsStandardType
{
  static constexpr bool value = xiiVariant::TypeDeduction<C>::value >= xiiVariantType::FirstStandardType && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::LastStandardType;
};

template <class T>
struct xiiIsStandardType<T, xiiVariant>
{
  static constexpr bool value = true;
};

//////////////////////////////////////////////////////////////////////////

/// Used to determine if the given type can be stored by value inside a xiiVariant (either standard type or custom type).
template <class T, class C = typename xiiCleanType<T>::Type>
struct xiiIsValueType
{
  static constexpr bool value = (xiiVariant::TypeDeduction<C>::value >= xiiVariantType::FirstStandardType && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::LastStandardType) || xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast;
};

template <class T>
struct xiiIsValueType<T, xiiVariant>
{
  static constexpr bool value = true;
};

//////////////////////////////////////////////////////////////////////////
/// Used to automatically assign any value to a xiiVariant using the assignment rules
/// outlined in xiiAbstractFunctionProperty::Execute.
template <class T,                                              ///< Only this parameter needs to be provided, the actual type of the value.
          class C             = typename xiiCleanType<T>::Type, ///< Same as T but without the const&* fluff.
          xiiInt32 VALUE_TYPE = xiiIsValueType<T>::value>       ///< Is 1 if T is a xiiTypeFlags::StandardType or a custom type
struct xiiVariantAssignmentAdapter
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(RealType* rhs) { m_Value = rhs; }
  void operator=(RealType&& rhs)
  {
    if (m_Value.IsValid())
      *m_Value.Get<RealType*>() = rhs;
  }
  xiiVariant& m_Value;
};

template <class T, class S>
struct xiiVariantAssignmentAdapter<T, xiiEnum<S>, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(xiiEnum<S>&& rhs) { m_Value = static_cast<xiiInt64>(rhs.GetValue()); }

  xiiVariant& m_Value;
};

template <class T, class S>
struct xiiVariantAssignmentAdapter<T, xiiBitflags<S>, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(xiiBitflags<S>&& rhs) { m_Value = static_cast<xiiInt64>(rhs.GetValue()); }

  xiiVariant& m_Value;
};

template <class T, class C>
struct xiiVariantAssignmentAdapter<T, C, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(T&& rhs) { m_Value = rhs; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAssignmentAdapter<T, xiiVariantArray, 0>
{
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(T&& rhs) { m_Value = rhs; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAssignmentAdapter<T, xiiVariantDictionary, 0>
{
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  void operator=(T&& rhs) { m_Value = rhs; }

  xiiVariant& m_Value;
};

//////////////////////////////////////////////////////////////////////////

/// Used to implicitly retrieve any value from a xiiVariant to be used as a function argument
/// using the assignment rules outlined in xiiAbstractFunctionProperty::Execute.
template <class T,                                              ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
          class C             = typename xiiCleanType<T>::Type, ///< Same as T but without the const&* fluff.
          xiiInt32 VALUE_TYPE = xiiIsValueType<T>::value,       ///< Is 1 if T is a xiiTypeFlags::StandardType or a custom type
          xiiInt32 OUT_PARAM  = xiiIsOutParam<T>::value>         ///< Is 1 if T a non-const reference or pointer.
struct xiiVariantAdapter
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator RealType&() { return *m_Value.Get<RealType*>(); }

  operator RealType*() { return m_Value.IsValid() ? m_Value.Get<RealType*>() : nullptr; }

  xiiVariant& m_Value;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiEnum<S>, 0, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
    if (m_Value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_Value.ConvertTo<xiiInt64>());
  }

  operator const xiiEnum<S>&() { return m_realValue; }
  operator const xiiEnum<S>*() { return m_Value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant& m_Value;
  xiiEnum<S>  m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiEnum<S>, 0, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
    if (m_Value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_Value.ConvertTo<xiiInt64>());
  }
  ~xiiVariantAdapter()
  {
    if (m_Value.IsValid())
      m_Value = static_cast<xiiInt64>(m_realValue.GetValue());
  }

  operator xiiEnum<S>&() { return m_realValue; }
  operator xiiEnum<S>*() { return m_Value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant& m_Value;
  xiiEnum<S>  m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiBitflags<S>, 0, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
    if (m_Value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_Value.ConvertTo<xiiInt64>()));
  }

  operator const xiiBitflags<S>&() { return m_realValue; }
  operator const xiiBitflags<S>*() { return m_Value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant&    m_Value;
  xiiBitflags<S> m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiBitflags<S>, 0, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
    if (m_Value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_Value.ConvertTo<xiiInt64>()));
  }
  ~xiiVariantAdapter()
  {
    if (m_Value.IsValid())
      m_Value = static_cast<xiiInt64>(m_realValue.GetValue());
  }

  operator xiiBitflags<S>&() { return m_realValue; }
  operator xiiBitflags<S>*() { return m_Value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant&    m_Value;
  xiiBitflags<S> m_realValue;
};

template <class T, class C>
struct xiiVariantAdapter<T, C, 1, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const C&()
  {
    if constexpr (xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast)
    {
      if (m_Value.GetType() == xiiVariantType::TypedPointer)
        return *m_Value.Get<RealType*>();
    }
    return m_Value.Get<RealType>();
  }

  operator const C*()
  {
    if constexpr (xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast)
    {
      if (m_Value.GetType() == xiiVariantType::TypedPointer)
        return m_Value.IsValid() ? m_Value.Get<RealType*>() : nullptr;
    }
    return m_Value.IsValid() ? &m_Value.Get<RealType>() : nullptr;
  }

  xiiVariant& m_Value;
};

template <class T, class C>
struct xiiVariantAdapter<T, C, 1, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
    // We ignore the return value here instead const_cast the Get<> result to profit from the Get methods runtime type checks.
    m_Value.GetWriteAccess();
  }

  operator C&()
  {
    if (m_Value.GetType() == xiiVariantType::TypedPointer)
      return *m_Value.Get<RealType*>();
    else
      return const_cast<RealType&>(m_Value.Get<RealType>());
  }
  operator C*()
  {
    if (m_Value.GetType() == xiiVariantType::TypedPointer)
      return m_Value.IsValid() ? m_Value.Get<RealType*>() : nullptr;
    else
      return m_Value.IsValid() ? &const_cast<RealType&>(m_Value.Get<RealType>()) : nullptr;
  }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariant, 1, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const xiiVariant&() { return m_Value; }
  operator const xiiVariant*() { return &m_Value; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariant, 1, 1>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator xiiVariant&() { return m_Value; }
  operator xiiVariant*() { return &m_Value; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariantArray, 0, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const xiiVariantArray&() { return m_Value.Get<xiiVariantArray>(); }
  operator const xiiVariantArray*() { return m_Value.IsValid() ? &m_Value.Get<xiiVariantArray>() : nullptr; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariantArray, 0, 1>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator xiiVariantArray&() { return m_Value.GetWritable<xiiVariantArray>(); }
  operator xiiVariantArray*() { return m_Value.IsValid() ? &m_Value.GetWritable<xiiVariantArray>() : nullptr; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariantDictionary, 0, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const xiiVariantDictionary&() { return m_Value.Get<xiiVariantDictionary>(); }
  operator const xiiVariantDictionary*() { return m_Value.IsValid() ? &m_Value.Get<xiiVariantDictionary>() : nullptr; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariantDictionary, 0, 1>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator xiiVariantDictionary&() { return m_Value.GetWritable<xiiVariantDictionary>(); }
  operator xiiVariantDictionary*() { return m_Value.IsValid() ? &m_Value.GetWritable<xiiVariantDictionary>() : nullptr; }

  xiiVariant& m_Value;
};

template <>
struct xiiVariantAdapter<const char*, const char*, 1, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const char*() { return m_Value.IsValid() ? m_Value.Get<xiiString>().GetData() : nullptr; }

  xiiVariant& m_Value;
};

template <class T>
struct xiiVariantAdapter<T, xiiStringView, 1, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_Value(value)
  {
  }

  operator const xiiStringView() { return m_Value.IsA<xiiStringView>() ? m_Value.Get<xiiStringView>() : m_Value.Get<xiiString>().GetView(); }

  xiiVariant& m_Value;
};
