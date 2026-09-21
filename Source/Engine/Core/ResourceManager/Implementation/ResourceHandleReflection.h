/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Core/ResourceManager/ResourceManager.h>

/// Adds two member functions to a class, GetXyzFile() and SetXyzFile() with Xyz being equal to 'name', which allow to access the handle through strings.
///
/// This macro is just for convenience, so that one doesn't need to write this boilerplate code by hand for every resource handle that
/// should be exposed through the reflection system.
/// The accessors still need to be exposed to the reflection system like this:
///
/// XII_ACCESSOR_PROPERTY("XyzResource", GetXyzFile, SetXyzFile)->AddAttributes(new xiiAssetBrowserAttribute("CompatibleAsset_Xyz")),
///
#define XII_ADD_RESOURCEHANDLE_ACCESSORS(name, member)                                  \
  void Set##name##File(xiiStringView sFile)                                             \
  {                                                                                     \
    if (!sFile.IsEmpty())                                                               \
    {                                                                                   \
      member = xiiResourceManager::LoadResource<decltype(member)::ResourceType>(sFile); \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
      member = {};                                                                      \
    }                                                                                   \
  }                                                                                     \
                                                                                        \
  xiiStringView Get##name##File() const                                                 \
  {                                                                                     \
    return member.GetResourceID();                                                      \
  }

/// Same as XII_ADD_RESOURCEHANDLE_ACCESSORS, but calls 'setterFunc' instead of assigning to 'member' directly.
///
/// This can be used, if the setter should do additional validation or bookkeeping.
#define XII_ADD_RESOURCEHANDLE_ACCESSORS_WITH_SETTER(name, member, setterFunc)             \
  void Set##name##File(xiiStringView sFile)                                                \
  {                                                                                        \
    if (!sFile.IsEmpty())                                                                  \
    {                                                                                      \
      setterFunc(xiiResourceManager::LoadResource<decltype(member)::ResourceType>(sFile)); \
    }                                                                                      \
    else                                                                                   \
    {                                                                                      \
      setterFunc({});                                                                      \
    }                                                                                      \
  }                                                                                        \
                                                                                           \
  xiiStringView Get##name##File() const                                                    \
  {                                                                                        \
    return member.GetResourceID();                                                         \
  }


/// [internal] Helper class to generate accessor functions for (private) resource handle members
template <typename Class, typename Type, Type Class::* Member>
struct xiiResourceHandlePropertyAccessor
{
  static xiiStringView GetValue(const Class* pInstance) { return ((*pInstance).*Member).GetResourceID(); }

  static void SetValue(Class* pInstance, xiiStringView value)
  {
    if (!value.IsEmpty())
    {
      (*pInstance).*Member = xiiResourceManager::LoadResource<typename Type::ResourceType>(value);
    }
    else
    {
      (*pInstance).*Member = {};
    }
  }

  static void* GetPropertyPointer(const Class* pInstance)
  {
    XII_IGNORE_UNUSED(pInstance);

    // No access to sub-properties
    return nullptr;
  }
};

/// Similar to XII_MEMBER_PROPERTY, but makes it convenient to expose resource handle properties
#define XII_RESOURCE_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                                      \
  (new xiiMemberProperty<OwnType, xiiStringView>(PropertyName,                                                                                                      \
                                                 &xiiResourceHandlePropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
                                                 &xiiResourceHandlePropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
                                                 &xiiResourceHandlePropertyAccessor<OwnType, XII_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// [internal] An implementation of xiiTypedMemberProperty that uses custom getter / setter functions to access a property.
template <typename Class, typename Type>
class xiiResourceAccessorProperty : public xiiTypedMemberProperty<xiiStringView>
{
public:
  using RealType     = xiiStringView;
  using HandleType   = typename xiiTypeTraits<Type>::NonConstReferenceType;
  using ResourceType = typename HandleType::ResourceType;
  using GetterFunc   = Type (Class::*)() const;
  using SetterFunc   = void (Class::*)(Type value);

  xiiResourceAccessorProperty(xiiStringView sPropertyName, GetterFunc getter, SetterFunc setter) :
    xiiTypedMemberProperty<RealType>(sPropertyName)
  {
    XII_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      xiiAbstractMemberProperty::m_Flags.Add(xiiPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    XII_IGNORE_UNUSED(pInstance);

    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual RealType GetValue(const void* pInstance) const override // [tested]
  {
    return (static_cast<const Class*>(pInstance)->*m_Getter)().GetResourceID();
  }

  virtual void SetValue(void* pInstance, RealType value) const override // [tested]
  {
    XII_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", xiiAbstractProperty::GetPropertyName());

    if (m_Setter)
    {
      if (!value.IsEmpty())
      {
        (static_cast<Class*>(pInstance)->*m_Setter)(xiiResourceManager::LoadResource<ResourceType>(value));
      }
      else
      {
        (static_cast<Class*>(pInstance)->*m_Setter)({});
      }
    }
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};

/// Similar to XII_RESOURCE_MEMBER_PROPERTY, but takes a getter and setter function that access the resource handle.
///
/// This can be used to control what other things should happen, if a handle gets modified.
#define XII_RESOURCE_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new xiiResourceAccessorProperty<OwnType, XII_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))
