/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/String.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

#  include <Lua/lua.hpp>

/// This class encapsulates ONE Lua-Script.
///
/// It makes it easier to interact with the script, to get data
/// out of it (e.g. for configuration files), to register C-functions to it and to call script-functions. It is possible
/// to load more than one Lua-File into one Lua-Script, one can dynamically generate code and pass it as
/// a string to the script.
/// It ALSO allows to construct the xiiLuaWrapper with a working lua_State-Pointer and thus
/// only simplify interaction with an already existing script (for example, when a C-Function is called in a Script,
/// it passes its lua_State to that Function).
///
/// \note Lua starts counting at 1, not at 0. However xiiLuaWrapper does NOT do this, but uses the C++ convention instead!
/// That means, when you query the first parameter or return-value passed to your function, you need to query for value 0, not for value 1.
class XII_CORE_DLL xiiLuaWrapper
{
  XII_DISALLOW_COPY_AND_ASSIGN(xiiLuaWrapper);

public:
  /// \name Setting up the Script
  /// @{

  /// Generates a NEW Lua-Script, which is empty.
  xiiLuaWrapper(); // [tested]

  /// Takes an EXISTING Lua-Script and allows to get easier access to it.
  xiiLuaWrapper(lua_State* s);

  /// Destroys the Lua-Script, if it was created, but leaves it intact, if this instance did not generate the Lua-Script.
  ~xiiLuaWrapper(); // [tested]

  /// Clears the script to be empty.
  void Clear(); // [tested]

  /// Returns the Lua state for custom access.
  ///
  /// It is not recommended to modify the Lua state directly, however for certain functionality that the wrapper does not implement
  /// this might be necessary. Make sure to either do all modifications at the start, before using the LuaWrapper on it (as it has some
  /// internal state), or to only do actions that will end up in the same stack state as before.
  lua_State* GetLuaState();

  /// Executes a string containing Lua-Code.
  ///
  /// \param szString
  ///   The Lua code to execute.
  /// \param szDebugChunkName
  ///   An optional name for the Lua code, to ease debugging when errors occur.
  /// \param pLogInterface
  ///   An optional log interface where error messages are written to. If nullptr is passed in, error messages are written to the global log.
  xiiResult ExecuteString(const char* szString, const char* szDebugChunkName = "chunk", xiiLogInterface* pLogInterface = nullptr) const; // [tested]

  /// @}


  /// \name Managing Tables
  /// @{

  /// Opens the Lua-Table with the given name for reading and writing.
  ///
  /// All following calls to functions that read/write variables are working in the scope of the last opened table.
  /// The table to open needs to be in scope itself. Returns XII_FAILURE, if it's not possible (the table does not exist in this scope).
  xiiResult OpenTable(const char* szTable); // [tested]

  /// Opens the Table n, that was passed to a C-Function on its Parameter-Stack.
  xiiResult OpenTableFromParameter(xiiUInt32 uiFunctionParameter); // [tested]

  /// Closes the table that was opened last.
  void CloseTable(); // [tested]

  /// Closes all open Tables.
  void CloseAllTables(); // [tested]

  /// Pushes an existing table onto Lua's stack. Either one that is in local scope, or a global table.
  void PushTable(const char* szTableName, bool bGlobalTable); // [tested]

  /// @}

  /// \name Variable and Function Checks
  /// @{

  /// Checks, whether the Variable with the given Name exists.
  bool IsVariableAvailable(const char* szVariable) const; // [tested]

  /// Checks, whether the Function with the given Name exists.
  bool IsFunctionAvailable(const char* szFunction) const; // [tested]

  /// @}

  /// \name Reading Variables
  /// @{

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  int GetIntVariable(const char* szName, xiiInt32 iDefault = 0) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  bool GetBoolVariable(const char* szName, bool bDefault = false) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  float GetFloatVariable(const char* szName, float fDefault = 0.0f) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  double GetDoubleVariable(const char* szName, double fDefault = 0.0) const; // [tested]

  /// Returns the Value of the Variable with the given name, or the default-value, if it does not exist.
  const char* GetStringVariable(const char* szName, const char* szDefault = "") const; // [tested]

  /// @}

  /// \name Modifying Variables
  /// @{

  /// Sets the Variable with the given name (in scope) to nil.
  void SetVariableNil(const char* szName) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, xiiInt32 iValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, bool bValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, float fValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, double fValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, const char* szValue) const; // [tested]

  /// Sets the Variable with the given name (in scope) with the given value.
  void SetVariable(const char* szName, const char* szValue, xiiUInt32 uiLen) const; // [tested]

  /// @}

  /// \name Calling Functions
  /// @{

  /// Registers a C-Function to the Script under a certain Name.
  void RegisterCFunction(const char* szFunctionName, lua_CFunction function, void* pLightUserData = nullptr) const; // [tested]

  /// Prepares a function to be called. After that the parameters can be pushed. Returns false if no function with the given name exists in
  /// the scope.
  bool PrepareFunctionCall(const char* szFunctionName); // [tested]

  /// Calls the prepared Function with the previously pushed Parameters.
  ///
  /// You must pass in how many return values you expect from this function and the function must stick to that, otherwise an assert will
  /// trigger. After you are finished inspecting the return values, you need to call DiscardReturnValues() to clean them up.
  ///
  /// Returns XII_FAILURE if anything went wrong during function execution. Reports errors via \a pLogInterface.
  xiiResult CallPreparedFunction(xiiUInt32 uiExpectedReturnValues = 0, xiiLogInterface* pLogInterface = nullptr); // [tested]

  /// Call this after you called a prepared Lua-function, that returned some values. If zero values were returned, this function is
  /// optional.
  void DiscardReturnValues(); // [tested]

  /// Return the value of this function in a called C-Function.
  ///
  /// A typical C function should look like this:
  /// \code
  /// int CFunction(lua_State* state)
  /// {
  ///   xiiLuaWrapper s(state);
  ///   .. do something ..
  ///   return s.ReturnToScript();
  /// }
  /// \endcode
  xiiInt32 ReturnToScript() const; // [tested]

  /// @}

  /// \name Calling Function with Parameters
  /// @{

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(xiiInt32 iParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(bool bParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(float fParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(double fParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(const char* szParam); // [tested]

  /// Pushes a parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameter(const char* szParam, xiiUInt32 uiLength); // [tested]

  /// Pushes a nil parameter on the stack to be passed to the next function called.
  /// Do this after PrepareFunctionCall() and before CallPreparedFunction().
  void PushParameterNil(); // [tested]

  /// @}

  /// \name Inspecting Function Parameters
  /// @{

  /// Returns the currently executed function light user data that was passed to RegisterCFunction.
  void* GetFunctionLightUserData() const;

  /// Returns how many Parameters were passed to the called C-Function.
  xiiUInt32 GetNumberOfFunctionParameters() const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterInt(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterBool(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterFloat(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterDouble(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterTable(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterString(xiiUInt32 uiParameter) const; // [tested]

  /// Checks the nth Parameter passed to a C-Function for its type.
  bool IsParameterNil(xiiUInt32 uiParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  int GetIntParameter(xiiUInt32 uiParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  bool GetBoolParameter(xiiUInt32 uiParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  float GetFloatParameter(xiiUInt32 uiParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  double GetDoubleParameter(xiiUInt32 uiParameter) const; // [tested]

  /// Returns the Value of the nth Parameter.
  const char* GetStringParameter(xiiUInt32 uiParameter) const; // [tested]

  /// @}

  /// \name Function Return Values
  /// @{

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(xiiInt32 iParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(bool bParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(float fParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(double fParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(const char* szParam); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValue(const char* szParam, xiiUInt32 uiLength); // [tested]

  /// Pushes a value as a return value for a called C-Function
  void PushReturnValueNil(); // [tested]


  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueInt(xiiUInt32 uiReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueBool(xiiUInt32 uiReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueFloat(xiiUInt32 uiReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueDouble(xiiUInt32 uiReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueString(xiiUInt32 uiReturnValue) const; // [tested]

  /// Checks the nth return-value passed to a C-Function for its type.
  bool IsReturnValueNil(xiiUInt32 uiReturnValue) const; // [tested]


  /// Returns the value of the nth return-value.
  int GetIntReturnValue(xiiUInt32 uiReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  bool GetBoolReturnValue(xiiUInt32 uiReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  float GetFloatReturnValue(xiiUInt32 uiReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  double GetDoubleReturnValue(xiiUInt32 uiReturnValue) const; // [tested]

  /// Returns the value of the nth return-value.
  const char* GetStringReturnValue(xiiUInt32 uiReturnValue) const; // [tested]

  /// @}


private:
  /// An Allocator for Lua. Optimizes (in theory) the allocations.
  static void* lua_allocator(void* ud, void* ptr, size_t osize, size_t nsize);

  /// The Lua-State for the Script.
  lua_State* m_pState;

  /// If this script created the Lua-State, it also releases it on exit.
  bool m_bReleaseOnExit;

  struct xiiScriptStates
  {
    xiiScriptStates() = default;

    /// How many Parameters were pushed for the next function-call.
    xiiInt32 m_iParametersPushed = 0;

    /// How many Tables have been opened inside the Lua-Script.
    xiiInt32 m_iOpenTables = 0;

    /// How many values the called Lua-function should return
    xiiInt32 m_iLuaReturnValues = 0;
  };

  xiiScriptStates m_States;

  static constexpr xiiInt32 s_iParamOffset = 1; // should be one, to start counting at 0, instead of 1
};

#  include <Core/Scripting/LuaWrapper/LuaWrapper.inl>

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
