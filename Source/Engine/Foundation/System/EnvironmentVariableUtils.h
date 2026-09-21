/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/String.h>

/// This is a helper class to interact with environment variables.
class XII_FOUNDATION_DLL xiiEnvironmentVariableUtils
{
public:
  /// Returns the current value of the request environment variable. If it isn't set szDefault will be returned.
  static xiiString GetValueString(xiiStringView sName, xiiStringView sDefault = nullptr);

  /// Sets the environment variable for the current execution environment (i.e. this process and child processes created after this call).
  static xiiResult SetValueString(xiiStringView sName, xiiStringView sValue);

  /// Returns the current value of the request environment variable. If it isn't set iDefault will be returned.
  static xiiInt32 GetValueInt(xiiStringView sName, xiiInt32 iDefault = -1);

  /// Sets the environment variable for the current execution environment.
  static xiiResult SetValueInt(xiiStringView sName, xiiInt32 iValue);

  /// Returns true if the environment variable with the given name is set, false otherwise.
  static bool IsVariableSet(xiiStringView sName);

  /// Removes an environment variable from the current execution context (i.e. this process and child processes created after this call).
  static xiiResult UnsetVariable(xiiStringView sName);

private:
  /// [internal]
  static xiiString GetValueStringImpl(xiiStringView sName, xiiStringView sDefault);

  /// [internal]
  static xiiResult SetValueStringImpl(xiiStringView sName, xiiStringView sValue);

  /// [internal]
  static bool IsVariableSetImpl(xiiStringView sName);

  /// [internal]
  static xiiResult UnsetVariableImpl(xiiStringView sName);
};
