/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/EnumProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// [internal] An implementation of xiiTypedEnumProperty that uses custom getter / setter functions to access a bitflags property.
template <typename Class, typename EnumType, typename Type>
class xiiBitflagsAccessorProperty : public xiiTypedEnumProperty<EnumType>
{
public:
  using RealType   = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// Constructor.
  xiiBitflagsAccessorProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter) :
    xiiTypedEnumProperty<EnumType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::Bitflags);

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
    typename EnumType::StorageType enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)().GetValue();
    return (xiiInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, xiiInt64 value) const override // [tested]
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// [internal] An implementation of xiiTypedEnumProperty that accesses the bitflags property data directly.
template <typename Class, typename EnumType, typename Type>
class xiiBitflagsMemberProperty : public xiiTypedEnumProperty<EnumType>
{
public:
  using GetterFunc  = Type (*)(const Class* pInstance);
  using SetterFunc  = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// Constructor.
  xiiBitflagsMemberProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer) :
    xiiTypedEnumProperty<EnumType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::Bitflags);

    m_Getter  = getter;
    m_Setter  = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual xiiInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::StorageType enumTemp = m_Getter(static_cast<const Class*>(pInstance)).GetValue();
    return (xiiInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, xiiInt64 value) const override // [tested]
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
