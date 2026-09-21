/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/IO/JSONParser.h>
#include <Foundation/Types/Variant.h>

/// This JSON reader will read an entire JSON document into a hierarchical structure of xiiVariants.
///
/// The reader will parse the entire document and create a data structure of xiiVariants, which can then be traversed easily.
/// Note that this class is much less efficient at reading large JSON documents, as it will dynamically allocate and copy objects around
/// quite a bit. For small to medium sized documents that might be good enough, for large files one should prefer to write a dedicated
/// class derived from xiiJSONParser.
class XII_FOUNDATION_DLL xiiJSONReader : public xiiJSONParser
{
public:
  enum class ElementType : xiiInt8
  {
    None,       ///< The JSON document is entirely empty (not even containing an empty object or array).
    Dictionary, ///< The top level element in the JSON document is an dictionary.
    Array,      ///< The top level element in the JSON document is an array.
  };

  xiiJSONReader();

  /// Reads the entire stream and creates the internal data structure that represents the JSON document. Returns XII_FAILURE if any parsing
  /// error occurred.
  xiiResult Parse(xiiStreamReader& ref_input, xiiUInt32 uiFirstLineOffset = 0);

  /// Returns the top-level object of the JSON document.
  const xiiVariantDictionary& GetTopLevelObject() const { return m_Stack.PeekBack().m_Dictionary; }

  /// Returns the top-level array of the JSON document.
  const xiiVariantArray& GetTopLevelArray() const { return m_Stack.PeekBack().m_Array; }

  /// Returns whether the top level element is an array or an object.
  ElementType GetTopLevelElementType() const { return m_Stack.PeekBack().m_Mode; }

private:
  /// This function can be overridden to skip certain variables, however the overriding function must still call this.
  virtual bool OnVariable(xiiStringView sVarName) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(xiiStringView sValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(double fValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValue(bool bValue) override;

  /// [internal] Do not override further.
  virtual void OnReadValueNULL() override;

  /// [internal] Do not override further.
  virtual void OnBeginObject() override;

  /// [internal] Do not override further.
  virtual void OnEndObject() override;

  /// [internal] Do not override further.
  virtual void OnBeginArray() override;

  /// [internal] Do not override further.
  virtual void OnEndArray() override;

  virtual void OnParsingError(xiiStringView sMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn) override;

protected:
  struct Element
  {
    xiiString            m_sName;
    ElementType          m_Mode = ElementType::None;
    xiiVariantArray      m_Array;
    xiiVariantDictionary m_Dictionary;
  };

  xiiHybridArray<Element, 32> m_Stack;

  bool      m_bParsingError = false;
  xiiString m_sLastName;
};
