#pragma once

#include <Core/Console/Console.h>

struct xiiLoggingEventData;

/// \brief A Quake-style console for in-game configuration of xiiCVar and xiiConsoleFunction.
///
/// The console displays the recent log activity and allows to modify cvars and call console functions.
/// It supports auto-completion of known keywords.
/// Additionally, 'keys' can be bound to arbitrary commands, such that useful commands can be executed
/// easily.
/// The default implementation uses xiiConsoleInterpreter::Lua as the interpreter for commands typed into it.
/// The interpreter can be replaced with custom implementations.
class XII_CORE_DLL xiiQuakeConsole : public xiiConsole
{
public:
  xiiQuakeConsole();
  virtual ~xiiQuakeConsole();



  /// \name Configuration
  /// @{

  /// \brief Adjusts how many strings the console will keep in memory at maximum.
  void SetMaxConsoleStrings(xiiUInt32 uiMax) { m_uiMaxConsoleStrings = xiiMath::Clamp<xiiUInt32>(uiMax, 0, 100000); }

  /// \brief Returns how many strings the console will keep in memory at maximum.
  xiiUInt32 GetMaxConsoleStrings() const { return m_uiMaxConsoleStrings; }

  /// \brief Enables or disables that the output from xiiGlobalLog is displayed in the console. Enabled by default.
  virtual void EnableLogOutput(bool bEnable);

  /// \brief Writes the state of the console (history, bound keys) to the stream.
  virtual void SaveState(xiiStreamWriter& Stream) const;

  /// \brief Reads the state of the console (history, bound keys) from the stream.
  virtual void LoadState(xiiStreamReader& Stream);

  /// @}

  /// \name Command Processing
  /// @{



  /// \brief Executes the given command using the current command interpreter.
  virtual void ExecuteCommand(xiiStringView input) override;

  /// \brief Binds \a szCommand to \a szKey. Calling ExecuteBoundKey() with this key will then run that command.
  ///
  /// A key can be any arbitrary string. However, it might make sense to either use the standard ASCII characters A-Z and a-z, which allows
  /// to trigger actions by the press of any of those buttons.
  /// You can, however, also use names for input buttons, such as 'Key_Left', but then you also need to call ExecuteBoundKey() with those
  /// names.
  /// If you use such virtual key names, it makes also sense to listen to the auto-complete event and suggest those key names there.
  void BindKey(const char* szKey, const char* szCommand);

  /// \brief Removes the key binding.
  void UnbindKey(const char* szKey);

  /// \brief Executes the command that was bound to this key.
  void ExecuteBoundKey(const char* szKey);

  /// @}

  /// \name Input Handling
  /// @{

  /// \brief Inserts one character at the caret position into the console input line.
  ///
  /// This function also calls ProcessInputCharacter and FilterInputCharacter. By default this already reacts on Tab, Enter and ESC
  /// and filters out all non ASCII characters.
  void AddInputCharacter(xiiUInt32 uiChar);

  /// \brief Clears the input line of the console.
  void ClearInputLine();

  /// \brief Returns the current content of the input line.
  const char* GetInputLine() const { return m_sInputLine.GetData(); }

  /// \brief Returns the position (in characters) of the caret.
  xiiInt32 GetCaretPosition() const { return m_iCaretPosition; }

  /// \brief Moves the caret in the text. Its position will be clamped to the length of the current input line text.
  void MoveCaret(xiiInt32 iMoveOffset);

  /// \brief Deletes the character following the caret position.
  void DeleteNextCharacter();

  /// \brief Scrolls the contents of the console up or down. Will be clamped to the available range.
  void Scroll(xiiInt32 iLines);

  /// \brief Returns the current scroll position. This must be used during rendering to start with the proper line.
  xiiUInt32 GetScrollPosition() const { return m_iScrollPosition; }

  /// \brief This function implements input handling (via xiiInputManager) for the console.
  ///
  /// If the console is 'open' (ie. has full focus), it will handle more input for caret movement etc.
  /// However, in the 'closed' state, it will still execute bound keys and commands from the history.
  /// It is not required to call this function, you can implement input handling entirely outside the console.
  ///
  /// If this function is used, it should be called once per frame and if the console is considered 'open',
  /// no further keyboard input should be processed, as that might lead to confusing behavior when the user types
  /// text into the console.
  ///
  /// The state whether the console is considered open has to be managed by the application.
  virtual void DoDefaultInputHandling(bool bConsoleOpen);

  /// @}

  /// \name Console Content
  /// @{

  /// \brief Adds a string to the console.
  virtual void AddConsoleString(xiiStringView text, xiiConsoleString::Type type = xiiConsoleString::Type::Default) override;

  /// \brief Returns all current console strings. Use GetScrollPosition() to know which one should be displayed as the first one.
  const xiiDeque<xiiConsoleString>& GetConsoleStrings() const;

  /// \brief Deletes all console strings, making the console empty.
  void ClearConsoleStrings();


  /// @}

  /// \name Helpers
  /// @{


  /// \brief Returns a nice string containing all the important information about the cvar.
  static xiiString GetFullInfoAsString(xiiCVar* pCVar);

  /// \brief Returns the value of the cvar as a string.
  static const xiiString GetValueAsString(xiiCVar* pCVar);

  /// @}

protected:
  /// \brief Deletes the character at the given position in the input line.
  void RemoveCharacter(xiiUInt32 uiInputLinePosition);

  /// \brief Makes sure the caret position is clamped to the input line length.
  void ClampCaretPosition();


  /// \brief The function that is used to read xiiGlobalLog messages.
  void LogHandler(const xiiLoggingEventData& data);

  xiiInt32         m_iCaretPosition;
  xiiStringBuilder m_sInputLine;

  virtual bool ProcessInputCharacter(xiiUInt32 uiChar);
  virtual bool FilterInputCharacter(xiiUInt32 uiChar);
  virtual void InputStringChanged();

  xiiDeque<xiiConsoleString> m_ConsoleStrings;
  bool                       m_bUseFilteredStrings = false;
  xiiDeque<xiiConsoleString> m_FilteredConsoleStrings;
  xiiUInt32                  m_uiMaxConsoleStrings;
  xiiInt32                   m_iScrollPosition;
  bool                       m_bLogOutputEnabled;
  bool                       m_bDefaultInputHandlingInitialized;


  xiiMap<xiiString, xiiString> m_BoundKeys;
};
