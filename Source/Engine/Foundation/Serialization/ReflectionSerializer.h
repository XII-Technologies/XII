/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Reflection/Reflection.h>

class xiiOpenDdlReaderElement;

class XII_FOUNDATION_DLL xiiReflectionSerializer
{
public:
  /// Writes all property values of the reflected \a pObject of type \a pRtti to \a stream in DDL format.
  ///
  /// Using ReadObjectPropertiesFromDDL() you can read those properties back into an existing object.
  /// Using ReadObjectFromDDL() an object of the same type is allocated and its properties are restored from the DDL data.
  ///
  /// Non-existing objects (pObject == nullptr) are stored as objects of type "null".
  /// The compact mode and typeMode should be set according to whether the DDL data is used for interchange with other code only,
  /// or might also be read by humans.
  ///
  /// Read-only properties are not written out, as they cannot be restored anyway.
  static void WriteObjectToDDL(xiiStreamWriter& ref_stream, const xiiRTTI* pRtti, const void* pObject, bool bCompactMmode = true,
                               xiiOpenDdlWriter::TypeStringMode typeMode = xiiOpenDdlWriter::TypeStringMode::Shortest); // [tested]

  /// Overload of WriteObjectToDDL that takes an existing DDL writer to output to.
  static void WriteObjectToDDL(xiiOpenDdlWriter& ref_ddl, const xiiRTTI* pRtti, const void* pObject, xiiUuid guid = xiiUuid()); // [tested]

  /// Same as WriteObjectToDDL but binary.
  static void WriteObjectToBinary(xiiStreamWriter& ref_stream, const xiiRTTI* pRtti, const void* pObject); // [tested]

  /// Reads the entire DDL data in the stream and restores a reflected object.
  ///
  /// The object type is read from the DDL information in the stream and the object is either allocated through the given allocator,
  /// or, if none is provided, the default allocator for the type is used.
  ///
  /// All properties are set to the values as described in the DDL data, as long as the properties can be matched to the runtime type.
  static void* ReadObjectFromDDL(xiiStreamReader& ref_stream, const xiiRTTI*& ref_pRtti); // [tested]

  static void* ReadObjectFromDDL(const xiiOpenDdlReaderElement* pRootElement, const xiiRTTI*& ref_pRtti); // [tested]

  /// Same as ReadObjectFromDDL but binary.
  static void* ReadObjectFromBinary(xiiStreamReader& ref_stream, const xiiRTTI*& ref_pRtti); // [tested]

  /// Reads the entire DDL data in the stream and sets all properties of the given object.
  ///
  /// All properties are set to the values as described in the DDL data, as long as the properties can be matched to the runtime type.
  /// The given object should ideally be of the same type as the object had that was written to the stream. However, if the types do
  /// not match or the properties have changed, the data will still be restored as good as possible.
  ///
  /// The object itself will not be reset to the default state before the properties are set, so properties that do not appear
  /// in the DDL data, or cannot be matched, will not be affected.
  static void ReadObjectPropertiesFromDDL(xiiStreamReader& ref_stream, const xiiRTTI& rtti, void* pObject); // [tested]

  /// Same as ReadObjectPropertiesFromDDL but binary.
  static void ReadObjectPropertiesFromBinary(xiiStreamReader& ref_stream, const xiiRTTI& rtti, void* pObject); // [tested]

  /// Clones pObject of type pType and returns it.
  ///
  /// In case a class derived from xiiReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do.
  static void* Clone(const void* pObject, const xiiRTTI* pType); // [tested]

  /// Clones pObject of type pType into the already existing pClone.
  ///
  /// In case a class derived from xiiReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do. However, the function will assert if pObject and pClone
  /// actually have a different type.
  static void Clone(const void* pObject, void* pClone, const xiiRTTI* pType); // [tested]

  /// Templated convenience function that calls Clone and automatically deduces the type.
  template <typename T>
  static T* Clone(const T* pObject)
  {
    return static_cast<T*>(Clone(pObject, xiiGetStaticRTTI<T>()));
  }
};
