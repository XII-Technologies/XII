#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class xiiRTTI;

template <typename Type>
class xiiTypedMapProperty : public xiiAbstractMapProperty
{
public:
  xiiTypedMapProperty(xiiStringView sPropertyName) :
    xiiAbstractMapProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<Type>();
    XII_CHECK_AT_COMPILETIME_MSG(!std::is_pointer<Type>::value || xiiVariant::TypeDeduction<typename xiiTypeTraits<Type>::NonConstReferencePointerType>::value == xiiVariantType::Invalid,
                                 "Pointer to standard types are not supported.");
  }

  virtual const xiiRTTI* GetSpecificType() const override { return xiiGetStaticRTTI<typename xiiTypeTraits<Type>::NonConstReferencePointerType>(); }
};


template <typename Class, typename Type, typename Container>
class xiiAccessorMapProperty : public xiiTypedMapProperty<Type>
{
public:
  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using RealType      = typename xiiTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc      = void (Class::*)(xiiStringView sKey, Type value);
  using RemoveFunc      = void (Class::*)(xiiStringView sKey);
  using GetValueFunc    = bool (Class::*)(xiiStringView sKey, RealType& value) const;
  using GetKeyRangeFunc = Container (Class::*)() const;

  xiiAccessorMapProperty(xiiStringView sPropertyName, GetKeyRangeFunc getKeys, GetValueFunc getValue, InsertFunc insert, RemoveFunc remove) :
    xiiTypedMapProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getKeys != nullptr, "The getKeys function of a map property cannot be nullptr.");
    XII_ASSERT_DEBUG(getValue != nullptr, "The GetValueFunc function of a map property cannot be nullptr.");

    m_GetKeyRange = getKeys;
    m_GetValue    = getValue;
    m_Insert      = insert;
    m_Remove      = remove;

    if (m_Insert == nullptr || remove == nullptr)
      xiiAbstractMapProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override
  {
    // this should be decltype(auto) c = ...; but MSVC 16 is too dumb for that (MSVC 15 works fine)
    decltype((static_cast<const Class*>(pInstance)->*m_GetKeyRange)()) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();

    return begin(c) == end(c);
  }

  virtual void Clear(void* pInstance) override
  {
    while (true)
    {
      // this should be decltype(auto) c = ...; but MSVC 16 is too dumb for that (MSVC 15 works fine)
      decltype((static_cast<const Class*>(pInstance)->*m_GetKeyRange)()) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();

      auto it = begin(c);
      if (it != end(c))
        Remove(pInstance, *it);
      else
        return;
    }
  }

  virtual void Insert(void* pInstance, xiiStringView sKey, const void* pObject) override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(sKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, xiiStringView sKey) override
  {
    XII_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no remove function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(sKey);
  }

  virtual bool Contains(const void* pInstance, xiiStringView sKey) const override
  {
    RealType value;
    return (static_cast<const Class*>(pInstance)->*m_GetValue)(sKey, value);
  }

  virtual bool GetValue(const void* pInstance, xiiStringView sKey, void* pObject) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetValue)(sKey, *static_cast<RealType*>(pObject));
  }

  virtual void GetKeys(const void* pInstance, xiiHybridArray<xiiString, 16>& out_keys) const override
  {
    out_keys.Clear();
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetKeyRange)();
    for (const auto& key : c)
    {
      out_keys.PushBack(key);
    }
  }

private:
  GetKeyRangeFunc m_GetKeyRange;
  GetValueFunc    m_GetValue;
  InsertFunc      m_Insert;
  RemoveFunc      m_Remove;
};


template <typename Class, typename Type, typename Container>
class xiiWriteAccessorMapProperty : public xiiTypedMapProperty<Type>
{
public:
  using ContainerType    = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using ContainerSubType = typename xiiContainerSubTypeResolver<ContainerType>::Type;
  using RealType         = typename xiiTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc       = void (Class::*)(xiiStringView sKey, Type value);
  using RemoveFunc       = void (Class::*)(xiiStringView sKey);
  using GetContainerFunc = Container (Class::*)() const;

  xiiWriteAccessorMapProperty(xiiStringView sPropertyName, GetContainerFunc getContainer, InsertFunc insert, RemoveFunc remove) :
    xiiTypedMapProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getContainer != nullptr, "The get count function of a map property cannot be nullptr.");

    m_GetContainer = getContainer;
    m_Insert       = insert;
    m_Remove       = remove;

    if (m_Insert == nullptr)
      xiiAbstractMapProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetContainer)().IsEmpty(); }

  virtual void Clear(void* pInstance) override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    while (!IsEmpty(pInstance))
    {
      auto it = c.GetIterator();
      Remove(pInstance, it.Key());
    }
  }

  virtual void Insert(void* pInstance, xiiStringView sKey, const void* pObject) override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(sKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, xiiStringView sKey) override
  {
    XII_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no remove function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(sKey);
  }

  virtual bool Contains(const void* pInstance, xiiStringView sKey) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetContainer)().Contains(sKey);
  }

  virtual bool GetValue(const void* pInstance, xiiStringView sKey, void* pObject) const override
  {
    decltype(auto)  c     = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    const RealType* value = c.GetValue(sKey);
    if (value)
    {
      *static_cast<RealType*>(pObject) = *value;
    }
    return value != nullptr;
  }

  virtual void GetKeys(const void* pInstance, xiiHybridArray<xiiString, 16>& out_keys) const override
  {
    decltype(auto) c = (static_cast<const Class*>(pInstance)->*m_GetContainer)();
    out_keys.Clear();
    for (auto it = c.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(it.Key());
    }
  }

private:
  GetContainerFunc m_GetContainer;
  InsertFunc       m_Insert;
  RemoveFunc       m_Remove;
};


template <typename Class, typename Container, Container Class::*Member>
struct xiiMapPropertyAccessor
{
  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using Type          = typename xiiTypeTraits<typename xiiContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class xiiMemberMapProperty : public xiiTypedMapProperty<typename xiiTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType              = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc      = Container& (*)(Class* pInstance);

  xiiMemberMapProperty(xiiStringView sPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter) :
    xiiTypedMapProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter      = getter;

    if (m_Getter == nullptr)
      xiiAbstractMapProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, xiiStringView sKey, const void* pObject) override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Insert(sKey, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, xiiStringView sKey) override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Remove(sKey);
  }

  virtual bool Contains(const void* pInstance, xiiStringView sKey) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).Contains(sKey);
  }

  virtual bool GetValue(const void* pInstance, xiiStringView sKey, void* pObject) const override
  {
    const RealType* value = m_ConstGetter(static_cast<const Class*>(pInstance)).GetValue(sKey);
    if (value)
    {
      *static_cast<RealType*>(pObject) = *value;
    }
    return value != nullptr;
  }

  virtual void GetKeys(const void* pInstance, xiiHybridArray<xiiString, 16>& out_keys) const override
  {
    decltype(auto) c = m_ConstGetter(static_cast<const Class*>(pInstance));
    out_keys.Clear();
    for (auto it = c.GetIterator(); it.IsValid(); ++it)
    {
      out_keys.PushBack(it.Key());
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc      m_Getter;
};
