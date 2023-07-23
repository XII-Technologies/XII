#pragma once

/// \file

#include <Foundation/Reflection/Implementation/MemberProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief The base class for enum and bitflags member properties.
///
/// Cast any property whose type derives from xiiEnumBase or xiiBitflagsBase class to access its value.
class xiiAbstractEnumerationProperty : public xiiAbstractMemberProperty
{
public:
  /// \brief Passes the property name through to xiiAbstractMemberProperty.
  xiiAbstractEnumerationProperty(xiiStringView sPropertyName) :
    xiiAbstractMemberProperty(sPropertyName)
  {
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual xiiInt64 GetValue(const void* pInstance) const = 0;

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, xiiInt64 value) = 0;

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override
  {
    *static_cast<xiiInt64*>(pObject) = GetValue(pInstance);
  }

  virtual void SetValuePtr(void* pInstance, const void* pObject) override
  {
    SetValue(pInstance, *static_cast<const xiiInt64*>(pObject));
  }
};


/// \brief [internal] Base class for enum / bitflags properties that already defines the type.
template <typename EnumType>
class xiiTypedEnumProperty : public xiiAbstractEnumerationProperty
{
public:
  /// \brief Passes the property name through to xiiAbstractEnumerationProperty.
  xiiTypedEnumProperty(xiiStringView sPropertyName) :
    xiiAbstractEnumerationProperty(sPropertyName)
  {
  }

  /// \brief Returns the actual type of the property. You can then test whether it derives from xiiEnumBase or
  ///  xiiBitflagsBase to determine whether we are dealing with an enum or bitflags property.
  virtual const xiiRTTI* GetSpecificType() const override // [tested]
  {
    return xiiGetStaticRTTI<typename xiiTypeTraits<EnumType>::NonConstReferenceType>();
  }
};


/// \brief [internal] An implementation of xiiTypedEnumProperty that uses custom getter / setter functions to access an enum property.
template <typename Class, typename EnumType, typename Type>
class xiiEnumAccessorProperty : public xiiTypedEnumProperty<EnumType>
{
public:
  using RealType   = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// \brief Constructor.
  xiiEnumAccessorProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter) :
    xiiTypedEnumProperty<EnumType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::IsEnum);

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual xiiInt64 GetValue(const void* pInstance) const override // [tested]
  {
    xiiEnum<EnumType> enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)();
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, xiiInt64 value) override // [tested]
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// \brief [internal] An implementation of xiiTypedEnumProperty that accesses the enum property data directly.
template <typename Class, typename EnumType, typename Type>
class xiiEnumMemberProperty : public xiiTypedEnumProperty<EnumType>
{
public:
  using GetterFunc  = Type (*)(const Class* pInstance);
  using SetterFunc  = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// \brief Constructor.
  xiiEnumMemberProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) :
    xiiTypedEnumProperty<EnumType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::IsEnum);

    m_Getter  = getter;
    m_Setter  = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual xiiInt64 GetValue(const void* pInstance) const override // [tested]
  {
    xiiEnum<EnumType> enumTemp = m_Getter(static_cast<const Class*>(pInstance));
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, xiiInt64 value) override // [tested]
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (typename EnumType::Enum)value);
  }

private:
  GetterFunc  m_Getter;
  SetterFunc  m_Setter;
  PointerFunc m_Pointer;
};
