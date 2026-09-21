/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class xiiRTTI;

/// Do not cast into this class or any of its derived classes, use xiiTypedArrayProperty instead.
template <typename Type>
class xiiTypedArrayProperty : public xiiAbstractArrayProperty
{
public:
  xiiTypedArrayProperty(xiiStringView sPropertyName) :
    xiiAbstractArrayProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<Type>();
    static_assert(!std::is_pointer<Type>::value || xiiVariantTypeDeduction<typename xiiTypeTraits<Type>::NonConstReferencePointerType>::value == xiiVariantType::Invalid,
                  "Pointer to standard types are not supported.");
  }

  virtual const xiiRTTI* GetSpecificType() const override { return xiiGetStaticRTTI<typename xiiTypeTraits<Type>::NonConstReferencePointerType>(); }
};

/// Specialization of xiiTypedArrayProperty to retain the pointer in const char*.
template <>
class xiiTypedArrayProperty<const char*> : public xiiAbstractArrayProperty
{
public:
  xiiTypedArrayProperty(xiiStringView sPropertyName) :
    xiiAbstractArrayProperty(sPropertyName)
  {
    m_Flags = xiiPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const xiiRTTI* GetSpecificType() const override { return xiiGetStaticRTTI<const char*>(); }
};


template <typename Class, typename Type>
class xiiAccessorArrayProperty : public xiiTypedArrayProperty<Type>
{
public:
  using RealType     = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetCountFunc = xiiUInt32 (Class::*)() const;
  using GetValueFunc = Type (Class::*)(xiiUInt32 uiIndex) const;
  using SetValueFunc = void (Class::*)(xiiUInt32 uiIndex, Type value);
  using InsertFunc   = void (Class::*)(xiiUInt32 uiIndex, Type value);
  using RemoveFunc   = void (Class::*)(xiiUInt32 uiIndex);

  xiiAccessorArrayProperty(xiiStringView sPropertyName, GetCountFunc getCount, GetValueFunc getter, SetValueFunc setter, InsertFunc insert, RemoveFunc remove) :
    xiiTypedArrayProperty<Type>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getCount != nullptr, "The get count function of an array property cannot be nullptr.");
    XII_ASSERT_DEBUG(getter != nullptr, "The get value function of an array property cannot be nullptr.");

    m_GetCount = getCount;
    m_Getter   = getter;
    m_Setter   = setter;
    m_Insert   = insert;
    m_Remove   = remove;

    if (m_Setter == nullptr)
      xiiAbstractArrayProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual xiiUInt32 GetCount(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetCount)(); }

  virtual void GetValue(const void* pInstance, xiiUInt32 uiIndex, void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));

    *static_cast<RealType*>(pObject) = (static_cast<const Class*>(pInstance)->*m_Getter)(uiIndex);
  }

  virtual void SetValue(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Setter)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Insert(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Insert)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, xiiUInt32 uiIndex) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    (static_cast<Class*>(pInstance)->*m_Remove)(uiIndex);
  }

  virtual void Clear(void* pInstance) const override { SetCount(pInstance, 0); }

  virtual void SetCount(void* pInstance, xiiUInt32 uiCount) const override
  {
    XII_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is fixed-size.", xiiAbstractProperty::GetPropertyName());

    while (uiCount < GetCount(pInstance))
    {
      Remove(pInstance, GetCount(pInstance) - 1);
    }
    while (uiCount > GetCount(pInstance))
    {
      RealType elem = RealType();
      Insert(pInstance, GetCount(pInstance), &elem);
    }
  }

private:
  GetCountFunc m_GetCount;
  GetValueFunc m_Getter;
  SetValueFunc m_Setter;
  InsertFunc   m_Insert;
  RemoveFunc   m_Remove;
};



template <typename Class, typename Container, Container Class::* Member>
struct xiiArrayPropertyAccessor
{
  using ContainerType = typename xiiTypeTraits<Container>::NonConstReferenceType;
  using Type          = typename xiiTypeTraits<typename xiiContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class xiiMemberArrayProperty : public xiiTypedArrayProperty<typename xiiTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType              = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc      = Container& (*)(Class * pInstance);

  xiiMemberArrayProperty(xiiStringView sPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter) :
    xiiTypedArrayProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter      = getter;

    if (m_Getter == nullptr)
      xiiAbstractArrayProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual xiiUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, xiiUInt32 uiIndex, void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));

    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance))[uiIndex] = *static_cast<const RealType*>(pObject);
  }

  virtual void Insert(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).InsertAt(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, xiiUInt32 uiIndex) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).RemoveAtAndCopy(uiIndex);
  }

  virtual void Clear(void* pInstance) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void SetCount(void* pInstance, xiiUInt32 uiCount) const override
  {
    XII_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    m_Getter(static_cast<Class*>(pInstance)).SetCount(uiCount);
  }

  virtual void* GetValuePointer(void* pInstance, xiiUInt32 uiIndex) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));

    return &(m_Getter(static_cast<Class*>(pInstance))[uiIndex]);
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc      m_Getter;
};

/// Read only version of xiiMemberArrayProperty that does not call any functions that modify the array. This is needed to reflect xiiArrayPtr members.
template <typename Class, typename Container, typename Type>
class xiiMemberArrayReadOnlyProperty : public xiiTypedArrayProperty<typename xiiTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType              = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);

  xiiMemberArrayReadOnlyProperty(xiiStringView sPropertyName, GetConstContainerFunc constGetter) :
    xiiTypedArrayProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    xiiAbstractArrayProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual xiiUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, xiiUInt32 uiIndex, void* pObject) const override
  {
    XII_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));

    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_REPORT_FAILURE("The property '{0}' is read-only.", xiiAbstractProperty::GetPropertyName());
  }

  virtual void Insert(void* pInstance, xiiUInt32 uiIndex, const void* pObject) const override
  {
    XII_REPORT_FAILURE("The property '{0}' is read-only.", xiiAbstractProperty::GetPropertyName());
  }

  virtual void Remove(void* pInstance, xiiUInt32 uiIndex) const override
  {
    XII_REPORT_FAILURE("The property '{0}' is read-only.", xiiAbstractProperty::GetPropertyName());
  }

  virtual void Clear(void* pInstance) const override
  {
    XII_REPORT_FAILURE("The property '{0}' is read-only.", xiiAbstractProperty::GetPropertyName());
  }

  virtual void SetCount(void* pInstance, xiiUInt32 uiCount) const override
  {
    XII_REPORT_FAILURE("The property '{0}' is read-only.", xiiAbstractProperty::GetPropertyName());
  }

private:
  GetConstContainerFunc m_ConstGetter;
};
