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
  enum
  {
    value = false,
  };
};

template <typename T>
struct xiiIsOutParam<T&>
{
  enum
  {
    value = !std::is_const<typename xiiTypeTraits<T>::NonReferencePointerType>::value,
  };
};

template <typename T>
struct xiiIsOutParam<T*>
{
  enum
  {
    value = !std::is_const<typename xiiTypeTraits<T>::NonReferencePointerType>::value,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type is a build-in standard variant type.
template <class T, class C = typename xiiCleanType<T>::Type>
struct xiiIsStandardType
{
  enum
  {
    value = xiiVariant::TypeDeduction<C>::value >= xiiVariantType::FirstStandardType && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::LastStandardType,
  };
};

template <class T>
struct xiiIsStandardType<T, xiiVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type can be stored by value inside an xiiVariant (either standard type or custom type).
template <class T, class C = typename xiiCleanType<T>::Type>
struct xiiIsValueType
{
  enum
  {
    value = (xiiVariant::TypeDeduction<C>::value >= xiiVariantType::FirstStandardType && xiiVariant::TypeDeduction<C>::value <= xiiVariantType::LastStandardType) || xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast,
  };
};

template <class T>
struct xiiIsValueType<T, xiiVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////
/// \brief Used to automatically assign any value to an xiiVariant using the assignment rules
/// outlined in xiiAbstractFunctionProperty::Execute.
template <class T,                                         ///< Only this parameter needs to be provided, the actual type of the value.
          class C        = typename xiiCleanType<T>::Type, ///< Same as T but without the const&* fluff.
          int VALUE_TYPE = xiiIsValueType<T>::value>       ///< Is 1 if T is a xiiTypeFlags::StandardType or a custom type
struct xiiVariantAssignmentAdapter
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  void operator=(RealType* rhs) { m_value = rhs; }
  void operator=(RealType&& rhs)
  {
    if (m_value.IsValid())
      *m_value.Get<RealType*>() = rhs;
  }
  xiiVariant& m_value;
};

template <class T, class S>
struct xiiVariantAssignmentAdapter<T, xiiEnum<S>, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  void operator=(xiiEnum<S>&& rhs) { m_value = static_cast<xiiInt64>(rhs.GetValue()); }

  xiiVariant& m_value;
};

template <class T, class S>
struct xiiVariantAssignmentAdapter<T, xiiBitflags<S>, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  void operator=(xiiBitflags<S>&& rhs) { m_value = static_cast<xiiInt64>(rhs.GetValue()); }

  xiiVariant& m_value;
};

template <class T, class C>
struct xiiVariantAssignmentAdapter<T, C, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAssignmentAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  xiiVariant& m_value;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to implicitly retrieve any value from an xiiVariant to be used as a function argument
/// using the assignment rules outlined in xiiAbstractFunctionProperty::Execute.
template <class T,                                         ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
          class C        = typename xiiCleanType<T>::Type, ///< Same as T but without the const&* fluff.
          int VALUE_TYPE = xiiIsValueType<T>::value,       ///< Is 1 if T is a xiiTypeFlags::StandardType or a custom type
          int OUT_PARAM  = xiiIsOutParam<T>::value>         ///< Is 1 if T a non-const reference or pointer.
struct xiiVariantAdapter
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;

  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  operator RealType&() { return *m_value.Get<RealType*>(); }

  operator RealType*() { return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr; }

  xiiVariant& m_value;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiEnum<S>, 0, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<xiiInt64>());
  }

  operator const xiiEnum<S> &() { return m_realValue; }
  operator const xiiEnum<S> *() { return m_value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant& m_value;
  xiiEnum<S>  m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiEnum<S>, 0, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<xiiInt64>());
  }
  ~xiiVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<xiiInt64>(m_realValue.GetValue());
  }

  operator xiiEnum<S> &() { return m_realValue; }
  operator xiiEnum<S> *() { return m_value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant& m_value;
  xiiEnum<S>  m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiBitflags<S>, 0, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<xiiInt64>()));
  }

  operator const xiiBitflags<S> &() { return m_realValue; }
  operator const xiiBitflags<S> *() { return m_value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant&    m_value;
  xiiBitflags<S> m_realValue;
};

template <class T, class S>
struct xiiVariantAdapter<T, xiiBitflags<S>, 0, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<xiiInt64>()));
  }
  ~xiiVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<xiiInt64>(m_realValue.GetValue());
  }

  operator xiiBitflags<S> &() { return m_realValue; }
  operator xiiBitflags<S> *() { return m_value.IsValid() ? &m_realValue : nullptr; }

  xiiVariant&    m_value;
  xiiBitflags<S> m_realValue;
};

template <class T, class C>
struct xiiVariantAdapter<T, C, 1, 0>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  operator const C&()
  {
    if constexpr (xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == xiiVariantType::TypedPointer)
        return *m_value.Get<RealType*>();
    }
    return m_value.Get<RealType>();
  }

  operator const C*()
  {
    if constexpr (xiiVariantTypeDeduction<C>::classification == xiiVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == xiiVariantType::TypedPointer)
        return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    }
    return m_value.IsValid() ? &m_value.Get<RealType>() : nullptr;
  }

  xiiVariant& m_value;
};

template <class T, class C>
struct xiiVariantAdapter<T, C, 1, 1>
{
  using RealType = typename xiiTypeTraits<T>::NonConstReferencePointerType;
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
    // We ignore the return value here instead const_cast the Get<> result to profit from the Get methods runtime type checks.
    m_value.GetWriteAccess();
  }

  operator C&()
  {
    if (m_value.GetType() == xiiVariantType::TypedPointer)
      return *m_value.Get<RealType*>();
    else
      return const_cast<RealType&>(m_value.Get<RealType>());
  }
  operator C*()
  {
    if (m_value.GetType() == xiiVariantType::TypedPointer)
      return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    else
      return m_value.IsValid() ? &const_cast<RealType&>(m_value.Get<RealType>()) : nullptr;
  }

  xiiVariant& m_value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariant, 1, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  operator const xiiVariant&() { return m_value; }
  operator const xiiVariant*() { return &m_value; }

  xiiVariant& m_value;
};

template <class T>
struct xiiVariantAdapter<T, xiiVariant, 1, 1>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  operator xiiVariant&() { return m_value; }
  operator xiiVariant*() { return &m_value; }

  xiiVariant& m_value;
};

template <>
struct xiiVariantAdapter<const char*, const char*, 1, 0>
{
  xiiVariantAdapter(xiiVariant& value) :
    m_value(value)
  {
  }

  operator const char*() { return m_value.IsValid() ? m_value.Get<xiiString>().GetData() : nullptr; }

  xiiVariant& m_value;
};
