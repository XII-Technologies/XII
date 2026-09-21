/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>

#include <typeinfo>

/// \file


/// xiiSingletonRegistry knows about all singleton instances of classes that use XII_DECLARE_SINGLETON.
///
/// It allows to query for a specific interface implementation by type name only, which makes it possible to
/// get rid of unwanted library dependencies and use pure virtual interface classes, without singleton code
/// (and thus link dependencies).
///
/// See XII_DECLARE_SINGLETON and XII_DECLARE_SINGLETON_OF_INTERFACE for details.
class XII_FOUNDATION_DLL xiiSingletonRegistry
{
public:
  struct SingletonEntry
  {
    xiiString m_sName;
    void*     m_pInstance = nullptr;
  };

  /// \todo Events for new/deleted singletons -> xiiInspector integration

  /// Retrieves a singleton instance by type name. Returns nullptr if no singleton instance is available.
  template <typename Interface>
  inline static Interface* GetSingletonInstance() // [tested]
  {
    return static_cast<Interface*>(s_Singletons.GetValueOrDefault(GetHash<Interface>(), {"", nullptr}).m_pInstance);
  }

  /// Retrieves a singleton instance by type name. Asserts if no singleton instance is available.
  template <typename Interface>
  inline static Interface* GetRequiredSingletonInstance() // [tested]
  {
    auto value = GetSingletonInstance<Interface>();
    XII_ASSERT_ALWAYS(value, "No instance of singleton type \"{0}\" has been registered!", typeid(Interface).name());
    return value;
  }

  /// Allows to inspect all known singletons
  static const xiiMap<size_t, SingletonEntry>& GetAllRegisteredSingletons();

  /// Registers a singleton instance under a given type name. This is automatically called by xiiSingletonRegistrar.
  template <typename Interface>
  inline static void Register(Interface* pSingletonInstance) // [tested]
  {
    XII_ASSERT_DEV(pSingletonInstance != nullptr, "Invalid singleton instance pointer");
    XII_ASSERT_DEV(s_Singletons[GetHash<Interface>()].m_pInstance == nullptr, "Singleton for type '{0}' has already been registered", typeid(Interface).name());

    s_Singletons[GetHash<Interface>()] = {typeid(Interface).name(), pSingletonInstance};
  }

  /// Unregisters a singleton instance. This is automatically called by xiiSingletonRegistrar.
  template <typename Interface>
  inline static void Unregister() // [tested]
  {
    XII_ASSERT_DEV(s_Singletons[GetHash<Interface>()].m_pInstance != nullptr, "Singleton for type '{0}' is currently not registered", typeid(Interface).name());

    s_Singletons.Remove(GetHash<Interface>());
  }

private:
  template <typename>
  friend class xiiSingletonRegistrar;

  template <typename Interface>
  inline static size_t GetHash()
  {
    static const size_t hash = typeid(Interface).hash_code();
    return hash;
  }

  static xiiMap<size_t, SingletonEntry> s_Singletons;
};


/// Insert this into a class declaration to turn the class into a singleton.
///
///        You can access the singleton instance in two ways.
///        By calling the static GetSingleton() function on the specific type.
///        By querying the instance through xiiSingletonRegistry giving the class type as a string.
///        The latter allows to get the implementation of an interface that is only declared through a simple header
///        but was not linked against.
///
///        Use XII_DECLARE_SINGLETON for a typical singleton class.
///        Use XII_DECLARE_SINGLETON_OF_INTERFACE for a singleton class that implements a specific interface,
///        which is itself not declared as a singleton and thus does not support to get to the interface implementation
///        through GetSingleton(). This is necessary, if you want to decouple library link dependencies and thus not put
///        any singleton code into the interface declaration, to keep it a pure virtual interface.
///        You can then query that class pointer also through the name of the interface using xiiSingletonRegistry.
#define XII_DECLARE_SINGLETON(self)                 \
public:                                             \
  XII_ALWAYS_INLINE static self* GetSingleton()     \
  {                                                 \
    return s_pSingleton;                            \
  }                                                 \
                                                    \
private:                                            \
  XII_DISALLOW_COPY_AND_ASSIGN(self);               \
  void RegisterSingleton()                          \
  {                                                 \
    s_pSingleton = this;                            \
    xiiSingletonRegistry::Register<self>(this);     \
  }                                                 \
  static void UnregisterSingleton()                 \
  {                                                 \
    if (s_pSingleton)                               \
    {                                               \
      xiiSingletonRegistry::Unregister<self>();     \
      s_pSingleton = nullptr;                       \
    }                                               \
  }                                                 \
  friend class xiiSingletonRegistrar<self>;         \
  xiiSingletonRegistrar<self> m_SingletonRegistrar; \
  static self*                s_pSingleton

/// Insert this into a class declaration to turn the class into a singleton.
///
///        You can access the singleton instance in two ways.
///        By calling the static GetSingleton() function on the specific type.
///        By querying the instance through xiiSingletonRegistry giving the class type as a string.
///        The latter allows to get the implementation of an interface that is only declared through a simple header
///        but was not linked against.
///
///        Use XII_DECLARE_SINGLETON for a typical singleton class.
///        Use XII_DECLARE_SINGLETON_OF_INTERFACE for a singleton class that implements a specific interface,
///        which is itself not declared as a singleton and thus does not support to get to the interface implementation
///        through GetSingleton(). This is necessary, if you want to decouple library link dependencies and thus not put
///        any singleton code into the interface declaration, to keep it a pure virtual interface.
///        You can then query that class pointer also through the name of the interface using xiiSingletonRegistry.
#define XII_DECLARE_SINGLETON_OF_INTERFACE(self, interface) \
public:                                                     \
  XII_ALWAYS_INLINE static self* GetSingleton()             \
  {                                                         \
    return s_pSingleton;                                    \
  }                                                         \
                                                            \
private:                                                    \
  XII_DISALLOW_COPY_AND_ASSIGN(self);                       \
  void RegisterSingleton()                                  \
  {                                                         \
    s_pSingleton = this;                                    \
    xiiSingletonRegistry::Register<self>(this);             \
    xiiSingletonRegistry::Register<interface>(this);        \
  }                                                         \
  static void UnregisterSingleton()                         \
  {                                                         \
    if (s_pSingleton)                                       \
    {                                                       \
      xiiSingletonRegistry::Unregister<interface>();        \
      xiiSingletonRegistry::Unregister<self>();             \
      s_pSingleton = nullptr;                               \
    }                                                       \
  }                                                         \
  friend class xiiSingletonRegistrar<self>;                 \
  xiiSingletonRegistrar<self> m_SingletonRegistrar;         \
  static self*                s_pSingleton


/// Put this into the cpp of a singleton class
#define XII_IMPLEMENT_SINGLETON(self) self* self::s_pSingleton = nullptr



/// [internal] Helper class to implement xiiSingletonRegistry and XII_DECLARE_SINGLETON
///
/// Classes that use XII_DECLARE_SINGLETON must pass their this pointer to their m_SingletonRegistrar member
/// during construction.
template <class TYPE>
class xiiSingletonRegistrar
{
public:
  XII_ALWAYS_INLINE xiiSingletonRegistrar(TYPE* pType) // [tested]
  {
    pType->RegisterSingleton();
  }

  XII_ALWAYS_INLINE ~xiiSingletonRegistrar() // [tested]
  {
    TYPE::UnregisterSingleton();
  }
};
