#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

/// \brief This is a helper class to interact with environment variables.
class XII_FOUNDATION_DLL xiiEnvironmentVariableUtils
{
public:
  /// \brief Returns the current value of the request environment variable. If it isn't set szDefault will be returned.
  static xiiString GetValueString(xiiStringView sName, xiiStringView sDefault = nullptr);

  /// \brief Sets the environment variable for the current execution environment (i.e. this process and child processes created after this call).
  static xiiResult SetValueString(xiiStringView sName, xiiStringView sValue);

  /// \brief Returns the current value of the request environment variable. If it isn't set iDefault will be returned.
  static xiiInt32 GetValueInt(xiiStringView sName, xiiInt32 iDefault = -1);

  /// \brief Sets the environment variable for the current execution environment.
  static xiiResult SetValueInt(xiiStringView sName, xiiInt32 iValue);

  /// \brief Returns true if the environment variable with the given name is set, false otherwise.
  static bool IsVariableSet(xiiStringView sName);

  /// \brief Removes an environment variable from the current execution context (i.e. this process and child processes created after this call).
  static xiiResult UnsetVariable(xiiStringView sName);

private:
  /// \brief [internal]
  static xiiString GetValueStringImpl(xiiStringView sName, xiiStringView sDefault);

  /// \brief [internal]
  static xiiResult SetValueStringImpl(xiiStringView sName, xiiStringView sValue);

  /// \brief [internal]
  static bool IsVariableSetImpl(xiiStringView sName);

  /// \brief [internal]
  static xiiResult UnsetVariableImpl(xiiStringView sName);
};
