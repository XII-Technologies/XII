/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/Console/ConsoleFunction.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Threading/Mutex.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>

struct XII_CORE_DLL xiiConsoleString
{
  enum class Type : xiiUInt8
  {
    Default,
    Error,
    SeriousWarning,
    Warning,
    Note,
    Success,
    Executed,
    VarName,
    FuncName,
    Dev,
    Debug,
  };

  Type      m_Type = Type::Default;
  xiiString m_sText;
  xiiColor  GetColor() const;

  bool operator<(const xiiConsoleString& rhs) const { return m_sText < rhs.m_sText; }
};

struct XII_CORE_DLL xiiCommandInterpreterState
{
  xiiStringBuilder                     m_sInput;
  xiiHybridArray<xiiConsoleString, 16> m_sOutput;

  void AddOutputLine(const xiiFormatString& text, xiiConsoleString::Type type = xiiConsoleString::Type::Default);
};

class XII_CORE_DLL xiiCommandInterpreter : public xiiRefCounted
{
public:
  virtual void Interpret(xiiCommandInterpreterState& inout_state) = 0;

  virtual void AutoComplete(xiiCommandInterpreterState& inout_state);

  /// Iterates over all cvars and finds all that start with the string \a szVariable.
  static void FindPossibleCVars(xiiStringView sVariable, xiiDeque<xiiString>& ref_commonStrings, xiiDeque<xiiConsoleString>& ref_consoleStrings);

  /// Iterates over all console functions and finds all that start with the string \a szVariable.
  static void FindPossibleFunctions(xiiStringView sVariable, xiiDeque<xiiString>& ref_commonStrings, xiiDeque<xiiConsoleString>& ref_consoleStrings);

  /// Returns the prefix string that is common to all strings in the \a vStrings array.
  static const xiiString FindCommonString(const xiiDeque<xiiString>& strings);
};

/// The event data that is broadcast by the console
struct xiiConsoleEvent
{
  enum class Type : xiiInt32
  {
    OutputLineAdded, ///< A string was added to the console
  };

  Type m_Type;

  /// The console string that was just added.
  const xiiConsoleString* m_AddedpConsoleString;
};

class XII_CORE_DLL xiiConsole
{
public:
  xiiConsole();
  virtual ~xiiConsole();

  /// \name Events
  /// @{

public:
  /// Grants access to subscribe and unsubscribe from console events.
  const xiiEvent<const xiiConsoleEvent&>& Events() const { return m_Events; }

protected:
  /// The console event variable, to attach to.
  xiiEvent<const xiiConsoleEvent&> m_Events;

  /// @}

  /// \name Helpers
  /// @{

public:
  /// Returns the mutex that's used to prevent multi-threaded access
  xiiMutex& GetMutex() const { return m_Mutex; }

  static void        SetMainConsole(xiiConsole* pConsole);
  static xiiConsole* GetMainConsole();

protected:
  mutable xiiMutex m_Mutex;

private:
  static xiiConsole* s_pMainConsole;

  /// @}

  /// \name Command Interpreter
  /// @{

public:
  /// Replaces the current command interpreter.
  ///
  /// This base class doesn't set any default interpreter, but derived classes may do so.
  void SetCommandInterpreter(const xiiSharedPtr<xiiCommandInterpreter>& pInterpreter) { m_pCommandInterpreter = pInterpreter; }

  /// Returns the currently used command interpreter.
  const xiiSharedPtr<xiiCommandInterpreter>& GetCommandInterpreter() const { return m_pCommandInterpreter; }

  /// Auto-completes the given text.
  ///
  /// Returns true, if the string was modified in any way.
  /// Adds additional strings to the console output, if there are further auto-completion suggestions.
  virtual bool AutoComplete(xiiStringBuilder& ref_sText);

  /// Executes the given input string.
  ///
  /// The command is forwarded to the set command interpreter.
  virtual void ExecuteCommand(xiiStringView sInput);

protected:
  xiiSharedPtr<xiiCommandInterpreter> m_pCommandInterpreter;

  /// @}

  /// \name Console Display
  /// @{

public:
  /// Adds a string to the console.
  ///
  /// The base class only broadcasts an event, but does not store the string anywhere.
  virtual void AddConsoleString(xiiStringView sText, xiiConsoleString::Type type = xiiConsoleString::Type::Default);

  /// @}

  /// \name Input History
  /// @{

public:
  /// Adds an item to the input history.
  void AddToInputHistory(xiiStringView sText);

  /// Returns the current input history.
  ///
  /// Make sure to lock the console's mutex while working with the history.
  const xiiStaticArray<xiiString, 16>& GetInputHistory() const { return m_InputHistory; }

  /// Replaces the input line by the next (or previous) history item.
  void RetrieveInputHistory(xiiInt32 iHistoryUp, xiiStringBuilder& ref_sResult);

  /// Writes the current input history to a text file.
  xiiResult SaveInputHistory(xiiStringView sFile);

  /// Reads the text file and appends all lines to the input history.
  void LoadInputHistory(xiiStringView sFile);

protected:
  xiiInt32                      m_iCurrentInputHistoryElement = -1;
  xiiStaticArray<xiiString, 16> m_InputHistory;

  /// @}
};
