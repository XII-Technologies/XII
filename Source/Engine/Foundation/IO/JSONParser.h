/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>

class xiiLogInterface;

/// A low level JSON parser that can incrementally parse the structure of a JSON document.
///
/// The document structure is returned through virtual functions that need to be overridden.
class XII_FOUNDATION_DLL xiiJSONParser
{
public:
  /// Constructor.
  xiiJSONParser();

  virtual ~xiiJSONParser() = default;

  /// Allows to specify a xiiLogInterface through which errors and warnings are reported.
  void SetLogInterface(xiiLogInterface* pLog) { m_pLogInterface = pLog; }

protected:
  /// Resets the parser to the start state and configures it to read from the given stream.
  void SetInputStream(xiiStreamReader& stream, xiiUInt32 uiFirstLineOffset = 0);

  /// Does one parsing step.
  ///
  /// While this function returns true, the document has not been parsed completely.
  /// This function may call any of the OnSomething functions through which the structure of the document is obtained.
  /// This function calls at most one such callback, but there is no guarantee that it calls any at all, it might just
  /// advance its internal state.
  bool ContinueParsing();

  /// Calls ContinueParsing() in a loop until that returns false.
  void ParseAll();

  /// Skips the rest of the currently open object. No OnEndArray() and OnEndObject() calls will be done for this object,
  /// cleanup must be done manually.
  void SkipObject();

  /// Skips the rest of the currently open array. No OnEndArray() and OnEndObject() calls will be done for this object,
  /// cleanup must be done manually.
  void SkipArray();

  /// Outputs that a parsing error was detected (via OnParsingError) and stops further parsing, if bFatal is set to true.
  void ParsingError(xiiStringView sMessage, bool bFatal);

  xiiLogInterface* m_pLogInterface = nullptr;

private:
  /// Called whenever a new variable is encountered. The variable name is passed along.
  /// At this point the type of the variable (simple, array, object) is not yet determined.
  ///
  /// The entire variable (independent of whether it is a simple value, an array or an object) can
  /// be skipped by returning false.
  virtual bool OnVariable(xiiStringView sVarName) = 0;

  /// Called whenever a new value is read.
  ///
  /// Directly following a call to OnVariable(), this means that the variable is a simple variable.
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnReadValue(xiiStringView sValue) = 0;

  /// \copydoc xiiJSONParser::OnReadValue()
  virtual void OnReadValue(double fValue) = 0;

  /// \copydoc xiiJSONParser::OnReadValue()
  virtual void OnReadValue(bool bValue) = 0;

  /// \copydoc xiiJSONParser::OnReadValue()
  virtual void OnReadValueNULL() = 0;

  /// Called when a new object is encountered.
  ///
  /// Directly following a call to OnVariable(), this means the variable is of type 'object' (and has a name).
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnBeginObject() = 0;

  /// Called when the end of an object is encountered.
  virtual void OnEndObject() = 0;

  /// Called when a new array is encountered.
  ///
  /// Directly following a call to OnVariable(), this means the variable is of type 'array' (and has a name).
  /// In between calls to OnBeginArray() and OnEndArray() it is another value in the array.
  virtual void OnBeginArray() = 0;

  /// Called when the end of an array is encountered.
  virtual void OnEndArray() = 0;

  /// Called when something unexpected is encountered in the JSON document.
  ///
  /// The error message describes what was expected and what was encountered.
  /// If bFatal is true, the error has left the parser in an unrecoverable state and thus it not continue parsing.
  /// In that case client code will need to clean up it's open state, as no further OnEndObject() / OnEndArray() will be called.
  /// If bFatal is false, the document does not contain valid JSON, but the parser is able to continue still.
  virtual void OnParsingError(xiiStringView sMessage, bool bFatal, xiiUInt32 uiLine, xiiUInt32 uiColumn)
  {
    XII_IGNORE_UNUSED(sMessage);
    XII_IGNORE_UNUSED(bFatal);
    XII_IGNORE_UNUSED(uiLine);
    XII_IGNORE_UNUSED(uiColumn);
  }

private:
  enum State
  {
    NotStarted,
    Finished,
    ReadingObject,
    ReadingArray,
    ReadingValue,
    ReadingVariable,
    ExpectSeparator
  };

  struct JSONState
  {
    JSONState() { m_State = NotStarted; }

    State m_State;
  };

  void   StartParsing();
  void   SkipWhitespace();
  void   SkipString();
  void   ReadString();
  double ReadNumber();
  void   ReadWord();

  void ContinueObject();
  void ContinueArray();
  void ContinueVariable();
  void ContinueValue();
  void ContinueSeparator();

  bool ReadCharacter(bool bSkipComments);
  void ReadNextByte();

  void SkipStack(State s);

  xiiUInt8  m_uiCurByte;
  xiiUInt8  m_uiNextByte;
  xiiUInt32 m_uiCurLine;
  xiiUInt32 m_uiCurColumn;

  xiiStreamReader*               m_pInput;
  xiiHybridArray<JSONState, 32>  m_StateStack;
  xiiHybridArray<xiiUInt8, 4096> m_TempString;

  bool m_bSkippingMode;
};
