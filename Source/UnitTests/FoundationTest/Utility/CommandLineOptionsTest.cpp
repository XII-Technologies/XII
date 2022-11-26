#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineOptions.h>

namespace
{
  class LogTestLogInterface : public xiiLogInterface
  {
  public:
    virtual void HandleLogMessage(const xiiLoggingEventData& le) override
    {
      switch (le.m_EventType)
      {
        case xiiLogMsgType::Flush:
          m_Result.Append("[Flush]\n");
          return;
        case xiiLogMsgType::BeginGroup:
          m_Result.Append(">", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::EndGroup:
          m_Result.Append("<", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_szTag, " ", le.m_szText, "\n");
          break;
        case xiiLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_szTag, " ", le.m_szText, "\n");
          break;

        default:
          XII_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    xiiStringBuilder m_Result;
  };

} // namespace

XII_CREATE_SIMPLE_TEST(Utility, CommandLineOptions)
{
  xiiCommandLineOptionDoc optDoc("__test", "-argDoc", "<doc>", "Doc argument", "no value");

  xiiCommandLineOptionBool optBool1("__test", "-bool1", "bool argument 1", false);
  xiiCommandLineOptionBool optBool2("__test", "-bool2", "bool argument 2", true);

  xiiCommandLineOptionInt optInt1("__test", "-int1", "int argument 1", 1);
  xiiCommandLineOptionInt optInt2("__test", "-int2", "int argument 2", 0, 4, 8);
  xiiCommandLineOptionInt optInt3("__test", "-int3", "int argument 3", 6, -8, 8);

  xiiCommandLineOptionFloat optFloat1("__test", "-float1", "float argument 1", 1);
  xiiCommandLineOptionFloat optFloat2("__test", "-float2", "float argument 2", 0, 4, 8);
  xiiCommandLineOptionFloat optFloat3("__test", "-float3", "float argument 3", 6, -8, 8);

  xiiCommandLineOptionString optString1("__test", "-string1", "string argument 1", "default string");

  xiiCommandLineOptionPath optPath1("__test", "-path1", "path argument 1", "default path");

  xiiCommandLineOptionEnum optEnum1("__test", "-enum1", "enum argument 1", "A | B = 2 | C | D | E = 7", 3);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionBool")
  {
    xiiCommandLineUtils cmd;
    cmd.InjectCustomArgument("-bool1");
    cmd.InjectCustomArgument("on");

    XII_TEST_BOOL(optBool1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd) == true);
    XII_TEST_BOOL(optBool2.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionInt")
  {
    xiiCommandLineUtils cmd;
    cmd.InjectCustomArgument("-int1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-int2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-int3");
    cmd.InjectCustomArgument("-2");

    XII_TEST_INT(optInt1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 3);
    XII_TEST_INT(optInt2.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 0);
    XII_TEST_INT(optInt3.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), -2);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionFloat")
  {
    xiiCommandLineUtils cmd;
    cmd.InjectCustomArgument("-float1");
    cmd.InjectCustomArgument("3");

    cmd.InjectCustomArgument("-float2");
    cmd.InjectCustomArgument("10");

    cmd.InjectCustomArgument("-float3");
    cmd.InjectCustomArgument("-2");

    XII_TEST_FLOAT(optFloat1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 3, 0.001f);
    XII_TEST_FLOAT(optFloat2.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 0, 0.001f);
    XII_TEST_FLOAT(optFloat3.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), -2, 0.001f);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionString")
  {
    xiiCommandLineUtils cmd;
    cmd.InjectCustomArgument("-string1");
    cmd.InjectCustomArgument("hello");

    XII_TEST_STRING(optString1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), "hello");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionPath")
  {
    xiiCommandLineUtils cmd;
    cmd.InjectCustomArgument("-path1");
    cmd.InjectCustomArgument("C:/test");

    const xiiString path = optPath1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd);

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
    XII_TEST_STRING(path, "C:/test");
#else
    XII_TEST_BOOL(path.EndsWith("C:/test"));
#endif
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "xiiCommandLineOptionEnum")
  {
    {
      xiiCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("A");

      XII_TEST_INT(optEnum1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 0);
    }

    {
      xiiCommandLineUtils cmd;
      cmd.InjectCustomArgument("-enum1");
      cmd.InjectCustomArgument("B");

      XII_TEST_INT(optEnum1.GetOptionValue(xiiCommandLineOption::LogMode::Never, &cmd), 2);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "LogAvailableOptions")
  {
    xiiCommandLineUtils cmd;

    xiiStringBuilder result;

    XII_TEST_BOOL(xiiCommandLineOption::LogAvailableOptionsToBuffer(result, xiiCommandLineOption::LogAvailableModes::Always, "__test", &cmd));

    XII_TEST_STRING(result, "\
\n\
-argDoc <doc> = no value\n\
    Doc argument\n\
\n\
-bool1 <bool> = false\n\
    bool argument 1\n\
\n\
-bool2 <bool> = true\n\
    bool argument 2\n\
\n\
-int1 <int> = 1\n\
    int argument 1\n\
\n\
-int2 <int> [4 .. 8] = 0\n\
    int argument 2\n\
\n\
-int3 <int> [-8 .. 8] = 6\n\
    int argument 3\n\
\n\
-float1 <float> = 1\n\
    float argument 1\n\
\n\
-float2 <float> [4 .. 8] = 0\n\
    float argument 2\n\
\n\
-float3 <float> [-8 .. 8] = 6\n\
    float argument 3\n\
\n\
-string1 <string> = default string\n\
    string argument 1\n\
\n\
-path1 <path> = default path\n\
    path argument 1\n\
\n\
-enum1 <A | B | C | D | E> = C\n\
    enum argument 1\n\
\n\
\n\
");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsHelpRequested")
  {
    xiiCommandLineUtils cmd;

    XII_TEST_BOOL(!xiiCommandLineOption::IsHelpRequested(&cmd));

    cmd.InjectCustomArgument("-help");

    XII_TEST_BOOL(xiiCommandLineOption::IsHelpRequested(&cmd));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RequireOptions")
  {
    xiiCommandLineUtils cmd;
    xiiString           missing;

    XII_TEST_BOOL(xiiCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    XII_TEST_STRING(missing, "-opt1");

    cmd.InjectCustomArgument("-opt1");

    XII_TEST_BOOL(xiiCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Failed());
    XII_TEST_STRING(missing, "-opt2");

    cmd.InjectCustomArgument("-opt2");

    XII_TEST_BOOL(xiiCommandLineOption::RequireOptions("-opt1 ; -opt2", &missing, &cmd).Succeeded());
    XII_TEST_STRING(missing, "");
  }
}
