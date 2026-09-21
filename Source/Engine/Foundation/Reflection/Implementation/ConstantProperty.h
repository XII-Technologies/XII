/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// The base class for all typed member properties. Ie. once the type of a property is determined, it can be cast to the proper
/// version of this.
///
/// For example, when you have a pointer to a xiiAbstractMemberProperty and it returns that the property is of type 'int', you can cast the
/// pointer to an pointer to xiiTypedMemberProperty<int> which then allows you to access its values.
template <typename Type>
class xiiTypedConstantProperty : public xiiAbstractConstantProperty
{
public:
  /// Passes the property name through to xiiAbstractMemberProperty.
  xiiTypedConstantProperty(xiiStringView sPropertyName) :
    xiiAbstractConstantProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<Type>();
  }

  /// Returns the actual type of the property. You can then compare that with known types, eg. compare it to xiiGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const xiiRTTI* GetSpecificType() const override // [tested]
  {
    return xiiGetStaticRTTI<typename xiiTypeTraits<Type>::NonConstReferenceType>();
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const = 0;
};

/// [internal] An implementation of xiiTypedConstantProperty that accesses the property data directly.
template <typename Type>
class xiiConstantProperty : public xiiTypedConstantProperty<Type>
{
public:
  /// Constructor.
  xiiConstantProperty(xiiStringView sPropertyName, Type value) :
    xiiTypedConstantProperty<Type>(sPropertyName), m_Value(value)
  {
    XII_ASSERT_DEBUG(this->m_Flags.IsSet(xiiPropertyFlags::StandardType), "Only constants that can be put in a xiiVariant are currently supported!");
  }

  /// Returns a pointer to the member property.
  virtual void* GetPropertyPointer() const override { return (void*)&m_Value; }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const override // [tested]
  {
    return m_Value;
  }

  virtual xiiVariant GetConstant() const override { return xiiVariant(m_Value); }

private:
  Type m_Value;
};
