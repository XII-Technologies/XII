/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>

class xiiStreamWriter;
class xiiStreamReader;

/// This class allows for writing type versions to a stream in a centralized place so that
/// each object doesn't need to write its own version manually.
///
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// xiiStreamWriter for subsequent serialization operations. Call AddType to add a type and its parent types to the version table.
/// Call End() once you want to finish writing the type versions.
class XII_FOUNDATION_DLL xiiTypeVersionWriteContext : public xiiSerializationContext<xiiTypeVersionWriteContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiTypeVersionWriteContext);

public:
  xiiTypeVersionWriteContext();
  ~xiiTypeVersionWriteContext();

  /// Call this method to begin collecting type version info. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  xiiStreamWriter& Begin(xiiStreamWriter& ref_originalStream);

  /// Ends the type version collection and writes the data to the original stream.
  xiiResult End();

  /// Adds the given type and its parent types to the version table.
  void AddType(const xiiRTTI* pRtti);

  /// Manually write the version table to the given stream.
  /// Can be used instead of Begin()/End() if all necessary types are available in one place anyways.
  void WriteTypeVersions(xiiStreamWriter& ref_stream) const;

  /// Returns the original stream that was passed to Begin().
  xiiStreamWriter& GetOriginalStream() { return *m_pOriginalStream; }

protected:
  xiiStreamWriter* m_pOriginalStream = nullptr;

  xiiDefaultMemoryStreamStorage m_TempStreamStorage;
  xiiMemoryStreamWriter         m_TempStreamWriter;

  xiiHashSet<const xiiRTTI*> m_KnownTypes;
};

/// Use this class to restore type versions written to a stream using a xiiTypeVersionWriteContext.
class XII_FOUNDATION_DLL xiiTypeVersionReadContext : public xiiSerializationContext<xiiTypeVersionReadContext>
{
  XII_DECLARE_SERIALIZATION_CONTEXT(xiiTypeVersionReadContext);

public:
  /// Reads the type version table from the stream
  xiiTypeVersionReadContext(xiiStreamReader& ref_stream);
  ~xiiTypeVersionReadContext();

  xiiUInt32 GetTypeVersion(const xiiRTTI* pRtti) const;

protected:
  xiiHashTable<const xiiRTTI*, xiiUInt32> m_TypeVersions;
};
