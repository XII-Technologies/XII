/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/Variant.h>

// ***********************************************
// ***** Base class for accessing properties *****


/// The base class for all typed member properties. I.e. once the type of a property is determined, it can be cast to the proper
/// version of this.
///
/// For example, when you have a pointer to a xiiAbstractMemberProperty and it returns that the property is of type 'int', you can cast the
/// pointer to an pointer to xiiTypedMemberProperty<int> which then allows you to access its values.
template <typename Type>
class xiiTypedMemberProperty : public xiiAbstractMemberProperty
{
public:
  /// Passes the property name through to xiiAbstractMemberProperty.
  xiiTypedMemberProperty(xiiStringView sPropertyName) :
    xiiAbstractMemberProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<Type>();
    static_assert(!std::is_pointer<Type>::value || xiiVariant::TypeDeduction<typename xiiTypeTraits<Type>::NonConstReferencePointerType>::value == xiiVariantType::Invalid, "Pointer to standard types are not supported.");
  }

  /// Returns the actual type of the property. You can then compare that with known types, eg. compare it to xiiGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const xiiRTTI* GetSpecificType() const override // [tested]
  {
    return xiiGetStaticRTTI<typename xiiTypeTraits<Type>::NonConstReferencePointerType>();
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const = 0; // [tested]

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const = 0; // [tested]

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<Type*>(pObject) = GetValue(pInstance); };
  virtual void SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const Type*>(pObject)); };
};

/// Specialization of xiiTypedMemberProperty for const char*.
///
/// This works because xiiTypedMemberProperty< typename xiiTypeTraits<Type>::NonConstReferenceType > in xiiAccessorProperty
/// does not actually remove the constness of the type but of the pointer, so const char* is not affected.
template <>
class xiiTypedMemberProperty<const char*> : public xiiAbstractMemberProperty
{
public:
  xiiTypedMemberProperty(xiiStringView sPropertyName) :
    xiiAbstractMemberProperty(sPropertyName)
  {
    // We treat const char* as a basic type and not a pointer.
    m_Flags = xiiPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const xiiRTTI* GetSpecificType() const override // [tested]
  {
    return xiiGetStaticRTTI<const char*>();
  }

  virtual const char* GetValue(const void* pInstance) const              = 0;
  virtual void        SetValue(void* pInstance, const char* value) const = 0;
  virtual void        GetValuePtr(const void* pInstance, void* pObject) const override { *static_cast<const char**>(pObject) = GetValue(pInstance); };
  virtual void        SetValuePtr(void* pInstance, const void* pObject) const override { SetValue(pInstance, *static_cast<const char* const*>(pObject)); };
};


// *******************************************************************
// ***** Class for properties that use custom accessor functions *****

/// [internal] An implementation of xiiTypedMemberProperty that uses custom getter / setter functions to access a property.
template <typename Class, typename Type>
class xiiAccessorProperty : public xiiTypedMemberProperty<typename xiiTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType   = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// Constructor.
  xiiAccessorProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter) :
    xiiTypedMemberProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  /// Always returns nullptr; once a property is modified through accessors, there is no point in giving more direct access to
  /// others.
  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    XII_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)();
  }

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)(value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


// *************************************************************
// ***** Classes for properties that are accessed directly *****

/// [internal] Helper class to generate accessor functions for (private) members of another class
template <typename Class, typename Type, Type Class::* Member>
struct xiiPropertyAccessor
{
  static Type GetValue(const Class* pInstance) { return (*pInstance).*Member; }

  static void SetValue(Class* pInstance, Type value) { (*pInstance).*Member = value; }

  static void* GetPropertyPointer(const Class* pInstance) { return (void*)&((*pInstance).*Member); }
};


/// [internal] An implementation of xiiTypedMemberProperty that accesses the property data directly.
template <typename Class, typename Type>
class xiiMemberProperty : public xiiTypedMemberProperty<Type>
{
public:
  using GetterFunc  = Type (*)(const Class* pInstance);
  using SetterFunc  = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// Constructor.
  xiiMemberProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) :
    xiiTypedMemberProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter  = getter;
    m_Setter  = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  /// Returns a pointer to the member property.
  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  /// Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue(const void* pInstance) const override { return m_Getter(static_cast<const Class*>(pInstance)); }

  /// Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, Type value) const override
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), value);
  }

private:
  GetterFunc  m_Getter;
  SetterFunc  m_Setter;
  PointerFunc m_Pointer;
};
