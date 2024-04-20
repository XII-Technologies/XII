#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/EnvironmentVariableUtils.h>

#if XII_DISABLED(XII_PLATFORM_WINDOWS_UWP)

static xiiUInt32 uiVersionForVariableSetting = 0;

XII_CREATE_SIMPLE_TEST(Utility, EnvironmentVariableUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetValueString / GetValueInt")
  {
#  if XII_ENABLED(XII_PLATFORM_WINDOWS)

    // Windows will have "NUMBER_OF_PROCESSORS" and "USERNAME" set, let's see if we can get them
    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet("NUMBER_OF_PROCESSORS"));

    xiiInt32 iNumProcessors = xiiEnvironmentVariableUtils::GetValueInt("NUMBER_OF_PROCESSORS", -23);
    XII_TEST_BOOL(iNumProcessors > 0);

    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet("USERNAME"));
    xiiString szUserName = xiiEnvironmentVariableUtils::GetValueString("USERNAME");
    XII_TEST_BOOL(szUserName.GetElementCount() > 0);

#  elif XII_ENABLED(XII_PLATFORM_OSX) || XII_ENABLED(XII_PLATFORM_LINUX)

    // Mac OS & Linux will have "USER" set
    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet("USER"));
    xiiString szUserName = xiiEnvironmentVariableUtils::GetValueString("USER");
    XII_TEST_BOOL(szUserName.GetElementCount() > 0);

#  endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsVariableSet/SetValue")
  {
    xiiStringBuilder szVarName;
    szVarName.SetFormat("XII_THIS_SHOULDNT_EXIST_NOW_OR_THIS_TEST_WILL_FAIL_{0}", uiVersionForVariableSetting++);

    XII_TEST_BOOL(!xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    xiiEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE").IgnoreResult();
    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    XII_TEST_STRING(xiiEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE");

    // Test overwriting the same value again
    xiiEnvironmentVariableUtils::SetValueString(szVarName, "NOW_IT_SHOULD_BE_SOMETHING_ELSE").IgnoreResult();
    XII_TEST_STRING(xiiEnvironmentVariableUtils::GetValueString(szVarName), "NOW_IT_SHOULD_BE_SOMETHING_ELSE");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Variable with very long value")
  {
    // The Windows implementation has a 64 wchar_t buffer for example. Let's try setting a really
    // long variable and getting it back
    const char* szLongVariable =
      "SOME REALLY LONG VALUE, LETS TEST SOME LIMITS WE MIGHT HIT - 012456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghijklmnopqrstuvwxyz";

    xiiStringBuilder szVarName;
    szVarName.SetFormat("XII_LONG_VARIABLE_TEST_{0}", uiVersionForVariableSetting++);

    XII_TEST_BOOL(!xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    xiiEnvironmentVariableUtils::SetValueString(szVarName, szLongVariable).IgnoreResult();
    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    XII_TEST_STRING(xiiEnvironmentVariableUtils::GetValueString(szVarName), szLongVariable);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Unsetting variables")
  {
    const char* szVarName = "XII_TEST_HELLO_WORLD";
    XII_TEST_BOOL(!xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    xiiEnvironmentVariableUtils::SetValueString(szVarName, "TEST").IgnoreResult();

    XII_TEST_BOOL(xiiEnvironmentVariableUtils::IsVariableSet(szVarName));

    xiiEnvironmentVariableUtils::UnsetVariable(szVarName).IgnoreResult();
    XII_TEST_BOOL(!xiiEnvironmentVariableUtils::IsVariableSet(szVarName));
  }
}

#endif
