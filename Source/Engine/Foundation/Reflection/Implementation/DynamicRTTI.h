/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// This needs to be put into the class declaration of EVERY dynamically reflectable class.
///
/// This macro extends a class, such that it is now able to return its own type information via GetDynamicRTTI(),
/// which is a virtual function, that is reimplemented on each type. A class needs to be derived from xiiReflectedClass
/// (at least indirectly) for this.
#define XII_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE) \
  XII_ALLOW_PRIVATE_PROPERTIES(SELF);                         \
                                                              \
public:                                                       \
  using SUPER = BASE_TYPE;                                    \
  XII_ALWAYS_INLINE static const xiiRTTI* GetStaticRTTI()     \
  {                                                           \
    return &SELF::s_RTTI;                                     \
  }                                                           \
                                                              \
private:                                                      \
  static xiiRTTI s_RTTI;                                      \
  XII_REFLECTION_DEBUG_CODE


#define XII_ADD_DYNAMIC_REFLECTION(SELF, BASE_TYPE)      \
  XII_ADD_DYNAMIC_REFLECTION_NO_GETTER(SELF, BASE_TYPE)  \
public:                                                  \
  virtual const xiiRTTI* GetDynamicRTTI() const override \
  {                                                      \
    return &SELF::s_RTTI;                                \
  }


#if XII_ENABLED(XII_COMPILE_FOR_DEVELOPMENT) && XII_ENABLED(XII_COMPILER_MSVC)

#  define XII_REFLECTION_DEBUG_CODE                       \
    static const xiiRTTI* ReflectionDebug_GetParentType() \
    {                                                     \
      return __super::GetStaticRTTI();                    \
    }

#  define XII_REFLECTION_DEBUG_GETPARENTFUNC &OwnType::ReflectionDebug_GetParentType

#else
#  define XII_REFLECTION_DEBUG_CODE          /*empty*/
#  define XII_REFLECTION_DEBUG_GETPARENTFUNC nullptr
#endif


/// Implements the necessary functionality for a type to be dynamically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass xiiNoBase
/// \param AllocatorType
///   The type of a xiiRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass xiiRTTINoAllocator for types that should not be created dynamically.
///   Pass xiiRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom xiiRTTIAllocator type to handle allocation differently.
#define XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, AllocatorType) \
  XII_RTTIINFO_DECL(Type, Type::SUPER, Version)                        \
  xiiRTTI Type::s_RTTI = GetRTTI((Type*)0);                            \
  XII_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, Type::SUPER, AllocatorType)

/// Ends the reflection code block that was opened with XII_BEGIN_DYNAMIC_REFLECTED_TYPE.
#define XII_END_DYNAMIC_REFLECTED_TYPE                                                                                                             \
  return xiiRTTI(GetTypeName((OwnType*)0), xiiGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),                          \
                 xiiVariant::TypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, \
                 XII_REFLECTION_DEBUG_GETPARENTFUNC);                                                                                              \
  }

/// Same as XII_BEGIN_DYNAMIC_REFLECTED_TYPE but forces the type to be treated as abstract by reflection even though it might not be abstract from a C++ perspective.
#define XII_BEGIN_ABSTRACT_DYNAMIC_REFLECTED_TYPE(Type, Version)      \
  XII_BEGIN_DYNAMIC_REFLECTED_TYPE(Type, Version, xiiRTTINoAllocator) \
    flags.Add(xiiTypeFlags::Abstract);

#define XII_END_ABSTRACT_DYNAMIC_REFLECTED_TYPE XII_END_DYNAMIC_REFLECTED_TYPE

/// All classes that should be dynamically reflectable, need to be derived from this base class.
class XII_FOUNDATION_DLL xiiReflectedClass : public xiiNoBase
{
  XII_ADD_DYNAMIC_REFLECTION_NO_GETTER(xiiReflectedClass, xiiNoBase);

public:
  virtual const xiiRTTI* GetDynamicRTTI() const { return &xiiReflectedClass::s_RTTI; }

public:
  XII_ALWAYS_INLINE xiiReflectedClass()          = default;
  XII_ALWAYS_INLINE virtual ~xiiReflectedClass() = default;

  /// Returns whether the type of this instance is of the given type or derived from it.
  bool IsInstanceOf(const xiiRTTI* pType) const;

  /// Returns whether the type of this instance is of the given type or derived from it.
  template <typename T>
  XII_ALWAYS_INLINE bool IsInstanceOf() const
  {
    const xiiRTTI* pType = xiiGetStaticRTTI<T>();
    return IsInstanceOf(pType);
  }
};
