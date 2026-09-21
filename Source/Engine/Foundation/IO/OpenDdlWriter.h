/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/OpenDdlParser.h>

// TODO
// Write primitives in HEX (esp. float)

/// The base class for OpenDDL writers.
///
/// Declares a common interface for writing OpenDDL files.
class XII_FOUNDATION_DLL xiiOpenDdlWriter
{
public:
  enum class TypeStringMode
  {
    Compliant,            ///< All primitive types are written as the OpenDDL standard defines them (very verbose)
    ShortenedUnsignedInt, ///< unsigned_intX is shortened to uintX
    Shortest              ///< All primitive type names are shortened to one or two characters: i1, i2, i3, i4, u1, u2, u3, u4, b, s, f, d (int, uint, bool,
                          ///< string, float, double)
  };

  enum class FloatPrecisionMode
  {
    Readable, ///< Float values are printed as readable numbers. Precision might get lost though.
    Exact,    ///< Float values are printed as HEX, representing the exact binary data.
  };

  /// Constructor
  xiiOpenDdlWriter();

  virtual ~xiiOpenDdlWriter() = default;

  /// All output is written to this binary stream.
  void SetOutputStream(xiiStreamWriter* pOutput) { m_pOutput = pOutput; } // [tested]

  /// Configures how much whitespace is output.
  void SetCompactMode(bool bCompact) { m_bCompactMode = bCompact; } // [tested]

  /// Configures how verbose the type strings are going to be written.
  void SetPrimitiveTypeStringMode(TypeStringMode mode) { m_TypeStringMode = mode; }

  /// Configures how float values are output.
  void SetFloatPrecisionMode(FloatPrecisionMode mode) { m_FloatPrecisionMode = mode; }

  /// Returns how float values are output.
  FloatPrecisionMode GetFloatPrecisionMode() const { return m_FloatPrecisionMode; }

  /// Allows to set the indentation. Negative values are possible.
  /// This makes it possible to set the indentation e.g. to -2, thus the output will only have indentation after a level of 3 has been reached.
  void SetIndentation(xiiInt8 iIndentation) { m_iIndentation = iIndentation; }

  /// Begins outputting an object.
  void BeginObject(xiiStringView sType, xiiStringView sName = {}, bool bGlobalName = false, bool bSingleLine = false); // [tested]

  /// Ends outputting an object.
  void EndObject(); // [tested]

  /// Begins outputting a list of primitives of the given type.
  void BeginPrimitiveList(xiiOpenDdlPrimitiveType type, xiiStringView sName = {}, bool bGlobalName = false); // [tested]

  /// Ends outputting the list of primitives.
  void EndPrimitiveList(); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteBool(const bool* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt8(const xiiInt8* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt16(const xiiInt16* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt32(const xiiInt32* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteInt64(const xiiInt64* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt8(const xiiUInt8* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt16(const xiiUInt16* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt32(const xiiUInt32* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteUInt64(const xiiUInt64* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteFloat(const float* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a number of values to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteDouble(const double* pValues, xiiUInt32 uiCount = 1); // [tested]

  /// Writes a single string to the primitive list. Can be called multiple times between BeginPrimitiveList() / EndPrimitiveList().
  void WriteString(const xiiStringView& sString); // [tested]

  /// Writes a single string to the primitive list, but the value is a HEX representation of the given binary data.
  void WriteBinaryAsString(const void* pData, xiiUInt32 uiBytes);


protected:
  enum State
  {
    Invalid          = -5,
    Empty            = -4,
    ObjectSingleLine = -3,
    ObjectMultiLine  = -2,
    ObjectStart      = -1,
    PrimitivesBool   = 0, // same values as in xiiOpenDdlPrimitiveType to enable casting
    PrimitivesInt8,
    PrimitivesInt16,
    PrimitivesInt32,
    PrimitivesInt64,
    PrimitivesUInt8,
    PrimitivesUInt16,
    PrimitivesUInt32,
    PrimitivesUInt64,
    PrimitivesFloat,
    PrimitivesDouble,
    PrimitivesString,
  };

  struct DdlState
  {
    State m_State              = State::Empty;
    bool  m_bPrimitivesWritten = false;
  };

  XII_ALWAYS_INLINE void OutputString(xiiStringView s) { m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).IgnoreResult(); }
  XII_ALWAYS_INLINE void OutputString(xiiStringView s, xiiUInt32 uiElementCount) { m_pOutput->WriteBytes(s.GetStartPointer(), uiElementCount).IgnoreResult(); }
  void                   OutputEscapedString(const xiiStringView& string);
  void                   OutputIndentation();
  void                   OutputPrimitiveTypeNameCompliant(xiiOpenDdlPrimitiveType type);
  void                   OutputPrimitiveTypeNameShort(xiiOpenDdlPrimitiveType type);
  void                   OutputPrimitiveTypeNameShortest(xiiOpenDdlPrimitiveType type);
  void                   WritePrimitiveType(xiiOpenDdlWriter::State exp);
  void                   OutputObjectName(xiiStringView sName, bool bGlobalName);
  void                   WriteBinaryAsHex(const void* pData, xiiUInt32 uiBytes);
  void                   OutputObjectBeginning();

  xiiInt32           m_iIndentation       = 0;
  bool               m_bCompactMode       = false;
  TypeStringMode     m_TypeStringMode     = TypeStringMode::ShortenedUnsignedInt;
  FloatPrecisionMode m_FloatPrecisionMode = FloatPrecisionMode::Exact;
  xiiStreamWriter*   m_pOutput            = nullptr;
  xiiStringBuilder   m_sTemp;

  xiiHybridArray<DdlState, 16> m_StateStack;
};
