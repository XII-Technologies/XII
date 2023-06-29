#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Types/Variant.h>

/// \brief The base class for JSON writers.
///
/// Declares a common interface for writing JSON files. Also implements some utility functions built on top of the interface (AddVariable()).
class XII_FOUNDATION_DLL xiiJSONWriter
{
public:
  /// \brief Modes to configure how much whitespace the JSON writer will output
  enum class WhitespaceMode
  {
    All,             ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
    LessIndentation, ///< Saves some space by using less space for indentation
    NoIndentation,   ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
    NewlinesOnly,    ///< All unnecessary whitespace, except for newlines, is not output.
    None,            ///< No whitespace, not even newlines, is output. This should be used when JSON is used for data exchange, but probably not read by humans.
  };

  /// \brief Modes to configure how arrays are written.
  enum class ArrayMode
  {
    InOneLine,      ///< All array items are written in a single line in the file.
    OneLinePerItem, ///< Each array item is put on a separate line.
  };

  /// \brief Constructor
  xiiJSONWriter();

  /// \brief Destructor
  virtual ~xiiJSONWriter();

  /// \brief Configures how much whitespace is output.
  void SetWhitespaceMode(WhitespaceMode whitespaceMode) { m_WhitespaceMode = whitespaceMode; }

  /// \brief Configures how arrays are written.
  void SetArrayMode(ArrayMode arrayMode) { m_ArrayMode = arrayMode; }

  /// \brief Shorthand for "BeginVariable(szName); WriteBool(value); EndVariable(); "
  void AddVariableBool(xiiStringView sName, bool value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteInt32(value); EndVariable(); "
  void AddVariableInt32(xiiStringView sName, xiiInt32 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUInt32(value); EndVariable(); "
  void AddVariableUInt32(xiiStringView sName, xiiUInt32 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteInt64(value); EndVariable(); "
  void AddVariableInt64(xiiStringView sName, xiiInt64 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUInt64(value); EndVariable(); "
  void AddVariableUInt64(xiiStringView sName, xiiUInt64 value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteFloat(value); EndVariable(); "
  void AddVariableFloat(xiiStringView sName, float value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteDouble(value); EndVariable(); "
  void AddVariableDouble(xiiStringView sName, double value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteString(value); EndVariable(); "
  void AddVariableString(xiiStringView sName, xiiStringView value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteNULL(value); EndVariable(); "
  void AddVariableNULL(xiiStringView sName); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteTime(value); EndVariable(); "
  void AddVariableTime(xiiStringView sName, xiiTime value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteUuid(value); EndVariable(); "
  void AddVariableUuid(xiiStringView sName, xiiUuid value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteAngle(value); EndVariable(); "
  void AddVariableAngle(xiiStringView sName, xiiAngle value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteAngle(value); EndVariable(); "
  void AddVariableAngle(xiiStringView sName, xiiAngled value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteColor(value); EndVariable(); "
  void AddVariableColor(xiiStringView sName, const xiiColor& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteColorGamma(value); EndVariable(); "
  void AddVariableColorGamma(xiiStringView sName, const xiiColorGammaUB& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2(value); EndVariable(); "
  void AddVariableVec2(xiiStringView sName, const xiiVec2& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2d(value); EndVariable(); "
  void AddVariableVec2d(xiiStringView sName, const xiiVec2d& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3(value); EndVariable(); "
  void AddVariableVec3(xiiStringView sName, const xiiVec3& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3d(value); EndVariable(); "
  void AddVariableVec3d(xiiStringView sName, const xiiVec3d& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4(value); EndVariable(); "
  void AddVariableVec4(xiiStringView sName, const xiiVec4& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4d(value); EndVariable(); "
  void AddVariableVec4d(xiiStringView sName, const xiiVec4d& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2I32(value); EndVariable(); "
  void AddVariableVec2I32(xiiStringView sName, const xiiVec2I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2I64(value); EndVariable(); "
  void AddVariableVec2I64(xiiStringView sName, const xiiVec2I64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3I32(value); EndVariable(); "
  void AddVariableVec3I32(xiiStringView sName, const xiiVec3I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3I64(value); EndVariable(); "
  void AddVariableVec3I64(xiiStringView sName, const xiiVec3I64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4I32(value); EndVariable(); "
  void AddVariableVec4I32(xiiStringView sName, const xiiVec4I32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4I64(value); EndVariable(); "
  void AddVariableVec4I64(xiiStringView sName, const xiiVec4I64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2U32(value); EndVariable(); "
  void AddVariableVec2U32(xiiStringView sName, const xiiVec2U32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec2U64(value); EndVariable(); "
  void AddVariableVec2U64(xiiStringView sName, const xiiVec2U64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3U32(value); EndVariable(); "
  void AddVariableVec3U32(xiiStringView sName, const xiiVec3U32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec3U64(value); EndVariable(); "
  void AddVariableVec3U64(xiiStringView sName, const xiiVec3U64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4U32(value); EndVariable(); "
  void AddVariableVec4U32(xiiStringView sName, const xiiVec4U32& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVec4U64(value); EndVariable(); "
  void AddVariableVec4U64(xiiStringView sName, const xiiVec4U64& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteQuat(value); EndVariable(); "
  void AddVariableQuat(xiiStringView sName, const xiiQuat& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteQuatd(value); EndVariable(); "
  void AddVariableQuatd(xiiStringView sName, const xiiQuatd& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat3(value); EndVariable(); "
  void AddVariableMat3(xiiStringView sName, const xiiMat3& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat3d(value); EndVariable(); "
  void AddVariableMat3d(xiiStringView sName, const xiiMat3d& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat4(value); EndVariable(); "
  void AddVariableMat4(xiiStringView sName, const xiiMat4& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteMat4d(value); EndVariable(); "
  void AddVariableMat4d(xiiStringView sName, const xiiMat4d& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteTransform(value); EndVariable(); "
  void AddVariableTransform(xiiStringView sName, const xiiTransform& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteTransformd(value); EndVariable(); "
  void AddVariableTransformd(xiiStringView sName, const xiiTransformd& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteDataBuffer(value); EndVariable(); "
  void AddVariableDataBuffer(xiiStringView sName, const xiiDataBuffer& value); // [tested]

  /// \brief Shorthand for "BeginVariable(szName); WriteVariant(value); EndVariable(); "
  void AddVariableVariant(xiiStringView sName, const xiiVariant& value); // [tested]


  /// \brief Writes a bool to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteBool(bool value) = 0;

  /// \brief Writes an int32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt32(xiiInt32 value) = 0;

  /// \brief Writes a uint32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt32(xiiUInt32 value) = 0;

  /// \brief Writes an int64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteInt64(xiiInt64 value) = 0;

  /// \brief Writes a uint64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteUInt64(xiiUInt64 value) = 0;

  /// \brief Writes a float to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteFloat(float value) = 0;

  /// \brief Writes a double to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteDouble(double value) = 0;

  /// \brief Writes a string to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteString(xiiStringView value) = 0;

  /// \brief Writes the value 'null' to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteNULL() = 0;

  /// \brief Writes a time value to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  virtual void WriteTime(xiiTime value) = 0;

  /// \brief Writes an xiiColor to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColor(const xiiColor& value) = 0;

  /// \brief Writes an xiiColorGammaUB to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteColorGamma(const xiiColorGammaUB& value) = 0;

  /// \brief Writes an xiiVec2 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2(const xiiVec2& value) = 0;

  /// \brief Writes an xiiVec2d to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2d(const xiiVec2d& value) = 0;

  /// \brief Writes an xiiVec3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3(const xiiVec3& value) = 0;

  /// \brief Writes an xiiVec3d to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3d(const xiiVec3d& value) = 0;

  /// \brief Writes an xiiVec4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4(const xiiVec4& value) = 0;

  /// \brief Writes an xiiVec4d to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4d(const xiiVec4d& value) = 0;

  /// \brief Writes an xiiVec2I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2I32(const xiiVec2I32& value) = 0;

  /// \brief Writes an xiiVec2I64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2I64(const xiiVec2I64& value) = 0;

  /// \brief Writes an xiiVec3I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3I32(const xiiVec3I32& value) = 0;

  /// \brief Writes an xiiVec3I64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3I64(const xiiVec3I64& value) = 0;

  /// \brief Writes an xiiVec4I32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4I32(const xiiVec4I32& value) = 0;

  /// \brief Writes an xiiVec4I64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4I64(const xiiVec4I64& value) = 0;

  /// \brief Writes an xiiVec2U32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2U32(const xiiVec2U32& value) = 0;

  /// \brief Writes an xiiVec2U64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec2U64(const xiiVec2U64& value) = 0;

  /// \brief Writes an xiiVec3U32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3U32(const xiiVec3U32& value) = 0;

  /// \brief Writes an xiiVec3U64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec3U64(const xiiVec3U64& value) = 0;

  /// \brief Writes an xiiVec4U32 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4U32(const xiiVec4U32& value) = 0;

  /// \brief Writes an xiiVec4U64 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteVec4U64(const xiiVec4U64& value) = 0;

  /// \brief Writes an xiiQuat to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteQuat(const xiiQuat& value) = 0;

  /// \brief Writes an xiiQuatd to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteQuatd(const xiiQuatd& value) = 0;

  /// \brief Writes an xiiMat3 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat3(const xiiMat3& value) = 0;

  /// \brief Writes an xiiMat3d to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat3d(const xiiMat3d& value) = 0;

  /// \brief Writes an xiiMat4 to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat4(const xiiMat4& value) = 0;

  /// \brief Writes an xiiMat4d to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteMat4d(const xiiMat4d& value) = 0;

  /// \brief Writes an xiiTransform to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteTransform(const xiiTransform& value) = 0;

  /// \brief Writes an xiiTransformd to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteTransformd(const xiiTransformd& value) = 0;

  /// \brief Writes an xiiUuid to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteUuid(const xiiUuid& value) = 0;

  /// \brief Writes an xiiAngle to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteAngle(xiiAngle value) = 0; // [tested]

  /// \brief Writes an xiiAngled to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteAngle(xiiAngled value) = 0; // [tested]

  /// \brief Writes an xiiDataBuffer to the JSON file. Can only be called between BeginVariable() / EndVariable() or BeginArray() / EndArray().
  ///
  /// \note Standard JSON does not have a suitable type for this. A derived class might turn this into an object or output it via WriteBinaryData().
  virtual void WriteDataBuffer(const xiiDataBuffer& value) = 0; // [tested]

  /// \brief The default implementation dispatches all supported types to WriteBool, WriteInt32, etc. and asserts on the more complex types.
  ///
  /// A derived class may override this function to implement support for the remaining variant types, if required.
  virtual void WriteVariant(const xiiVariant& value); // [tested]

  /// \brief Outputs a chunk of memory in some JSON form that can be interpreted as binary data when reading it again.
  ///
  /// How exactly the raw data is represented in JSON is up to the derived class. \a szDataType allows to additionally output a string
  /// that identifies the type of data.
  virtual void WriteBinaryData(xiiStringView sDataType, const void* pData, xiiUInt32 uiBytes, xiiStringView sValueString = {}) = 0;

  /// \brief Begins outputting a variable. \a szName is the variable name.
  ///
  /// Between BeginVariable() and EndVariable() you can call the WriteXYZ functions once to write out the variable's data.
  /// You can also call BeginArray() and BeginObject() without a variable name to output an array or object variable.
  virtual void BeginVariable(xiiStringView sName) = 0;

  /// \brief Ends outputting a variable.
  virtual void EndVariable() = 0;

  /// \brief Begins outputting an array variable.
  ///
  /// If szName is nullptr this will create an anonymous array, which is necessary when you want to put an array as a value into another array.
  /// BeginArray() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndArray() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginArray(xiiStringView sName = {}) = 0;

  /// \brief Ends outputting an array variable.
  virtual void EndArray() = 0;

  /// \brief Begins outputting an object variable.
  ///
  /// If szName is nullptr this will create an anonymous object, which is necessary when you want to put an object as a value into an array.
  /// BeginObject() with a non-nullptr value for \a szName is identical to calling BeginVariable() first. In this case EndObject() will also
  /// end the variable definition, so no additional call to EndVariable() is required.
  virtual void BeginObject(xiiStringView sName = {}) = 0;

  /// \brief Ends outputting an object variable.
  virtual void EndObject() = 0;

  /// \brief Indicates if an error was encountered while writing
  ///
  /// If any error was encountered at any time during writing, this will return true
  bool HadWriteError() const;

protected:
  WhitespaceMode m_WhitespaceMode = WhitespaceMode::All;
  ArrayMode      m_ArrayMode      = ArrayMode::InOneLine;

  /// \brief called internally when there was an error during writing
  void SetWriteErrorState();

private:
  bool m_bHadWriteError = false;
};


/// \brief Implements a standard compliant JSON writer, all numbers are output as double values.
///
/// xiiStandardJSONWriter also implements WriteBinaryData() and the functions WriteVec2() etc., for which there is no standard way to implement them in
/// JSON. WriteVec2() etc. will simply redirect to WriteBinaryData(), which in turn implements the MongoDB convention of outputting binary data.
/// I.e. it will turn the data into a JSON object which contains one variable called "$type" that identifies the data type, and one variable called
/// "$binary" which contains the raw binary data Hex encoded in little endian format.
/// If you want to write a fully standard compliant JSON file, just don't output any of these types.
class XII_FOUNDATION_DLL xiiStandardJSONWriter : public xiiJSONWriter
{
public:
  /// \brief Constructor.
  xiiStandardJSONWriter(); // [tested]

  /// \brief Destructor.
  ~xiiStandardJSONWriter(); // [tested]

  /// \brief All output is written to this binary stream.
  void SetOutputStream(xiiStreamWriter* pOutput); // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteBool()
  virtual void WriteBool(bool value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteInt32()
  virtual void WriteInt32(xiiInt32 value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteUInt32()
  virtual void WriteUInt32(xiiUInt32 value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteInt64()
  virtual void WriteInt64(xiiInt64 value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteUInt64()
  virtual void WriteUInt64(xiiUInt64 value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteFloat()
  virtual void WriteFloat(float value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteDouble()
  virtual void WriteDouble(double value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteString()
  virtual void WriteString(xiiStringView value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteNULL()
  virtual void WriteNULL() override; // [tested]

  /// \brief Writes the time value as a double (i.e. redirects to WriteDouble()).
  virtual void WriteTime(xiiTime value) override; // [tested]

  /// \brief Outputs the value via WriteVec4().
  virtual void WriteColor(const xiiColor& value) override; // [tested]

  /// \brief Outputs the value via WriteVec4().
  virtual void WriteColorGamma(const xiiColorGammaUB& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2(const xiiVec2& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2d(const xiiVec2d& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3(const xiiVec3& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3d(const xiiVec3d& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4(const xiiVec4& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4d(const xiiVec4d& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2I32(const xiiVec2I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2I64(const xiiVec2I64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3I32(const xiiVec3I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3I64(const xiiVec3I64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4I32(const xiiVec4I32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4I64(const xiiVec4I64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2U32(const xiiVec2U32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec2U64(const xiiVec2U64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3U32(const xiiVec3U32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec3U64(const xiiVec3U64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4U32(const xiiVec4U32& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteVec4U64(const xiiVec4U64& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteQuat(const xiiQuat& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteQuatd(const xiiQuatd& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat3(const xiiMat3& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat3d(const xiiMat3d& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat4(const xiiMat4& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteMat4d(const xiiMat4d& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteTransform(const xiiTransform& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteTransformd(const xiiTransformd& value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteUuid(const xiiUuid& value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteFloat()
  virtual void WriteAngle(xiiAngle value) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::WriteDouble()
  virtual void WriteAngle(xiiAngled value) override; // [tested]

  /// \brief Outputs the value via WriteBinaryData().
  virtual void WriteDataBuffer(const xiiDataBuffer& value) override; // [tested]

  /// \brief Implements the MongoDB way of writing binary data. First writes a "$type" variable, then a "$binary" variable that represents the raw
  /// data (Hex encoded, little endian).
  virtual void WriteBinaryData(xiiStringView sDataType, const void* pData, xiiUInt32 uiBytes, xiiStringView sValueString = {}) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::BeginVariable()
  virtual void BeginVariable(xiiStringView sName) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::EndVariable()
  virtual void EndVariable() override; // [tested]

  /// \brief \copydoc xiiJSONWriter::BeginArray()
  virtual void BeginArray(xiiStringView sName = {}) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::EndArray()
  virtual void EndArray() override; // [tested]

  /// \brief \copydoc xiiJSONWriter::BeginObject()
  virtual void BeginObject(xiiStringView sName = {}) override; // [tested]

  /// \brief \copydoc xiiJSONWriter::EndObject()
  virtual void EndObject() override; // [tested]

protected:
  void End();

  enum State
  {
    Invalid,
    Empty,
    Variable,
    Object,
    NamedObject,
    Array,
    NamedArray,
  };

  struct JSONState
  {
    JSONState();

    State m_State;
    bool  m_bRequireComma;
    bool  m_bValueWasWritten;
  };

  struct CommaWriter
  {
    CommaWriter(xiiStandardJSONWriter* pWriter);
    ~CommaWriter();

    xiiStandardJSONWriter* m_pWriter;
  };

  void OutputString(xiiStringView s);
  void OutputEscapedString(xiiStringView s);
  void OutputIndentation();

  xiiInt32         m_iIndentation;
  xiiStreamWriter* m_pOutput;

  xiiHybridArray<JSONState, 16> m_StateStack;
};
