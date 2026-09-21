/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

/// Do not cast into this class or any of its derived classes, use xiiAbstractSetProperty instead.
template <typename Type>
class xiiTypedSetProperty : public xiiAbstractSetProperty
{
public:
  xiiTypedSetProperty(xiiStringView sPropertyName) :
    xiiAbstractSetProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<Type>();
  }

  virtual const xiiRTTI* GetSpecificType() const override { return xiiGetStaticRTTI<typename xiiTypeTraits<Type>::NonConstReferencePointerType>(); }
};

/// Specialization of xiiTypedArrayProperty to retain the pointer in const char*.
template <>
class xiiTypedSetProperty<const char*> : public xiiAbstractSetProperty
{
public:
  xiiTypedSetProperty(xiiStringView sPropertyName) :
    xiiAbstractSetProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const xiiRTTI* GetSpecificType() const override { return xiiGetStaticRTTI<const char*>(); }
};


template <typename Class, typename Type, typename Container>
class xiiAccessorSetProperty : public xiiTypedSetProperty<Type>
{
public:
  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using RealType      = typename xiiTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc    = void (Class::*)(Type value);
  using RemoveFunc    = void (Class::*)(Type value);
  using GetValuesFunc = Container (Class::*)() const;

  xiiAccessorSetProperty(xiiStringView sPropertyName, GetValuesFunc getValues, InsertFunc insert, RemoveFunc remove) :
    xiiTypedSetProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getValues != nullptr, "The get values function of an set property cannot be nullptr.");

    m_GetValues = getValues;
    m_Insert    = insert;
    m_Remove    = remove;

    if (m_Insert == nullptr || m_Remove == nullptr)
      xiiAbstractSetProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }


  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is read-only",
                     xiiAbstractProperty::GetPropertyName());

    // We must not cache the container c here as the Remove can make it invalid
    // e.g. xiiArrayPtr by value.
    while (!IsEmpty(pInstance))
    {
      // This should be decltype(auto) c = ...; but MSVC 16 does not deduce that (MSVC 15 works fine)
      decltype((static_cast<const Class*>(pInstance)->*m_GetValues)()) c     = (static_cast<const Class*>(pInstance)->*m_GetValues)();
      auto                                                             it    = cbegin(c);
      RealType                                                         value = *it;
      Remove(pInstance, &value);
    }
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Insert)(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Remove)(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    for (const auto& value : (static_cast<const Class*>(pInstance)->*m_GetValues)())
    {
      if (value == *static_cast<const RealType*>(pObject))
        return true;
    }
    return false;
  }

  virtual void GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : (static_cast<const Class*>(pInstance)->*m_GetValues)())
    {
      out_keys.PushBack(xiiVariant(value));
    }
  }

private:
  GetValuesFunc m_GetValues;
  InsertFunc    m_Insert;
  RemoveFunc    m_Remove;
};



template <typename Class, typename Container, Container Class::* Member>
struct xiiSetPropertyAccessor
{
  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using Type          = typename xiiTypeTraits<typename xiiContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class xiiMemberSetProperty : public xiiTypedSetProperty<typename xiiTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType              = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc      = Container& (*)(Class * pInstance);

  xiiMemberSetProperty(xiiStringView sPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter) :
    xiiTypedSetProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an set property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter      = getter;

    if (m_Getter == nullptr)
      xiiAbstractSetProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).Insert(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).Remove(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).Contains(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, xiiDynamicArray<xiiVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : m_ConstGetter(static_cast<const Class*>(pInstance)))
    {
      out_keys.PushBack(xiiVariant(value));
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc      m_Getter;
};
