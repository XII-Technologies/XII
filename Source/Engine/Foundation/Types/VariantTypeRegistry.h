/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Utilities/EnumerableClass.h>

class xiiStreamWriter;
class xiiStreamReader;
class xiiVariantTypeInfo;

/// Variant type registry allows for custom variant type infos to be accessed.
///
/// Custom variant types are defined via the XII_DECLARE_CUSTOM_VARIANT_TYPE and XII_DEFINE_CUSTOM_VARIANT_TYPE macros.
/// \sa XII_DECLARE_CUSTOM_VARIANT_TYPE, XII_DEFINE_CUSTOM_VARIANT_TYPE
class XII_FOUNDATION_DLL xiiVariantTypeRegistry
{
  XII_DECLARE_SINGLETON(xiiVariantTypeRegistry);

public:
  /// Find the variant type info for the given xiiRTTI type.
  /// \return xiiVariantTypeInfo if one exits for the given type, otherwise nullptr.
  const xiiVariantTypeInfo* FindVariantTypeInfo(const xiiRTTI* pType) const;
  ~xiiVariantTypeRegistry();

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, VariantTypeRegistry);
  xiiVariantTypeRegistry();

  void PluginEventHandler(const xiiPluginEvent& EventData);
  void UpdateTypes();

  xiiHashTable<const xiiRTTI*, const xiiVariantTypeInfo*> m_TypeInfos;
};

/// Defines functions to allow the full feature set of xiiVariant to be used.
/// \sa XII_DEFINE_CUSTOM_VARIANT_TYPE, xiiVariantTypeRegistry
class XII_FOUNDATION_DLL xiiVariantTypeInfo : public xiiEnumerable<xiiVariantTypeInfo>
{
public:
  xiiVariantTypeInfo();
  virtual const xiiRTTI* GetType() const                                                   = 0;
  virtual xiiUInt32      Hash(const void* pObject) const                                   = 0;
  virtual bool           Equal(const void* pObjectA, const void* pObjectB) const           = 0;
  virtual void           Serialize(xiiStreamWriter& ref_writer, const void* pObject) const = 0;
  virtual void           Deserialize(xiiStreamReader& ref_reader, void* pObject) const     = 0;

  XII_DECLARE_ENUMERABLE_CLASS(xiiVariantTypeInfo);
};

/// Helper template used by XII_DEFINE_CUSTOM_VARIANT_TYPE.
/// \sa XII_DEFINE_CUSTOM_VARIANT_TYPE
template <typename T>
class xiiVariantTypeInfoT : public xiiVariantTypeInfo
{
  const xiiRTTI* GetType() const override
  {
    return xiiGetStaticRTTI<T>();
  }
  xiiUInt32 Hash(const void* pObject) const override
  {
    return xiiHashHelper<T>::Hash(*static_cast<const T*>(pObject));
  }
  bool Equal(const void* pObjectA, const void* pObjectB) const override
  {
    return xiiHashHelper<T>::Equal(*static_cast<const T*>(pObjectA), *static_cast<const T*>(pObjectB));
  }
  void Serialize(xiiStreamWriter& writer, const void* pObject) const override
  {
    writer << *static_cast<const T*>(pObject);
  }
  void Deserialize(xiiStreamReader& reader, void* pObject) const override
  {
    reader >> *static_cast<T*>(pObject);
  }
};

/// Defines a custom variant type, allowing it to be serialized and compared. The type needs to be declared first before using this macro.
///
/// The given type must implement xiiHashHelper and xiiStreamWriter / xiiStreamReader operators.
/// Macros should be placed in any cpp. Note that once a custom type is defined, it is considered a value type and will be passed by value. It must be linked into every editor and engine dll to allow serialization. Thus it should only be used for common types in base libraries.
/// Limitations: Currently only member variables are supported on custom types, no arrays, set, maps etc. For best performance, any custom type smaller than 16 bytes should be POD so it can be inlined into the xiiVariant.
/// \sa XII_DECLARE_CUSTOM_VARIANT_TYPE, xiiVariantTypeRegistry, xiiVariant
#define XII_DEFINE_CUSTOM_VARIANT_TYPE(TYPE)                                                                                                                           \
  static_assert(xiiVariantTypeDeduction<TYPE>::value == xiiVariantType::TypedObject, "XII_DECLARE_CUSTOM_VARIANT_TYPE needs to be added to the header defining TYPE"); \
  xiiVariantTypeInfoT<TYPE> g_xiiVariantTypeInfoT_##TYPE;
