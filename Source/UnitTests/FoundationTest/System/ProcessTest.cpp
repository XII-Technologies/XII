#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

XII_CREATE_SIMPLE_TEST_GROUP(System);

#if XII_ENABLED(XII_SUPPORTS_PROCESSES)

XII_CREATE_SIMPLE_TEST(System, Process)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Command Line")
  {
    xiiProcessOptions proc;
    proc.m_Arguments.PushBack("-bla");
    proc.m_Arguments.PushBack("blub blub");
    proc.m_Arguments.PushBack("\"di dub\"");
    proc.AddArgument(" -test ");
    proc.AddArgument("-hmpf {}", 27);
    proc.AddCommandLine("-a b   -c  d  -e \"f g h\" ");

    xiiStringBuilder cmdLine;
    proc.BuildCommandLineString(cmdLine);

    XII_TEST_STRING(cmdLine, "-bla \"blub blub\" \"di dub\" -test \"-hmpf 27\" -a b -c d -e \"f g h\"");
  }

  static const char* g_szTestMsg      = "Tell me more!\nAnother line\n520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\nThat's all";
  static const char* g_szTestMsgLine0 = "Tell me more!\n";
  static const char* g_szTestMsgLine1 = "Another line\n";
  static const char* g_szTestMsgLine2 = "520CharactersInOneLineAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA_END\n";
  static const char* g_szTestMsgLine3 = "That's all";


  // we can launch FoundationTest with the -cmd parameter to execute a couple of useful things to test launching process
  const xiiStringBuilder pathToSelf = xiiCommandLineUtils::GetGlobalInstance()->GetParameter(0);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Execute")
  {
    xiiProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    xiiInt32 exitCode = -1;

    if (!XII_TEST_BOOL_MSG(xiiProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    XII_TEST_INT(exitCode, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Launch / WaitToFinish")
  {
    xiiProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("500");

    xiiProcess proc;
    XII_TEST_BOOL(proc.GetState() == xiiProcessState::NotStarted);

    if (!XII_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    XII_TEST_BOOL(proc.GetState() == xiiProcessState::Running);
    XII_TEST_BOOL(proc.WaitToFinish(xiiTime::Seconds(5)).Succeeded());
    XII_TEST_BOOL(proc.GetState() == xiiProcessState::Finished);
    XII_TEST_INT(proc.GetExitCode(), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Launch / Terminate")
  {
    xiiProcessOptions opt;
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-sleep");
    opt.m_Arguments.PushBack("10000");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("0");

    xiiProcess proc;
    XII_TEST_BOOL(proc.GetState() == xiiProcessState::NotStarted);

    if (!XII_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
      return;

    XII_TEST_BOOL(proc.GetState() == xiiProcessState::Running);
    XII_TEST_BOOL(proc.Terminate().Succeeded());
    XII_TEST_BOOL(proc.GetState() == xiiProcessState::Finished);
    XII_TEST_INT(proc.GetExitCode(), -1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Launch / Detach")
  {
    xiiTime tTerminate;

    {
      xiiProcessOptions opt;
      opt.m_sProcess = pathToSelf;
      opt.m_Arguments.PushBack("-cmd");
      opt.m_Arguments.PushBack("-sleep");
      opt.m_Arguments.PushBack("10000");

      xiiProcess proc;
      if (!XII_TEST_BOOL_MSG(proc.Launch(opt).Succeeded(), "Failed to start process."))
        return;

      proc.Detach();

      tTerminate = xiiTime::Now();
    }

    const xiiTime tDiff = xiiTime::Now() - tTerminate;
    XII_TEST_BOOL_MSG(tDiff < xiiTime::Seconds(1.0), "Destruction of xiiProcess should be instant after Detach() was used.");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STDOUT")
  {
    xiiDynamicArray<xiiStringBuilder> lines;
    xiiStringBuilder                  out;
    xiiProcessOptions                 opt;
    opt.m_onStdOut = [&](xiiStringView view) {
      out.Append(view);
      lines.PushBack(view);
    };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!XII_TEST_BOOL_MSG(xiiProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (XII_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    XII_TEST_STRING(out, g_szTestMsg);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STDERROR")
  {
    xiiStringBuilder  err;
    xiiProcessOptions opt;
    opt.m_onStdError = [&err](xiiStringView view) {
      err.Append(view);
    };

    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stderr");
    opt.m_Arguments.PushBack("NOT A VALID COMMAND");
    opt.m_Arguments.PushBack("-exitcode");
    opt.m_Arguments.PushBack("1");

    xiiInt32 exitCode = 0;

    if (!XII_TEST_BOOL_MSG(xiiProcess::Execute(opt, &exitCode).Succeeded(), "Failed to start process."))
      return;

    XII_TEST_BOOL_MSG(!err.IsEmpty(), "Error stream should contain something.");
    XII_TEST_INT(exitCode, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "STDOUT_STDERROR")
  {
    xiiDynamicArray<xiiStringBuilder> lines;
    xiiStringBuilder                  out;
    xiiStringBuilder                  err;
    xiiProcessOptions                 opt;
    opt.m_onStdOut = [&](xiiStringView view) {
      out.Append(view);
      lines.PushBack(view);
    };
    opt.m_onStdError = [&err](xiiStringView view) {
      err.Append(view);
    };
    opt.m_sProcess = pathToSelf;
    opt.m_Arguments.PushBack("-cmd");
    opt.m_Arguments.PushBack("-stdout");
    opt.m_Arguments.PushBack(g_szTestMsg);

    if (!XII_TEST_BOOL_MSG(xiiProcess::Execute(opt).Succeeded(), "Failed to start process."))
      return;

    if (XII_TEST_BOOL(lines.GetCount() == 4))
    {
      lines[0].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[0], g_szTestMsgLine0);
      lines[1].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[1], g_szTestMsgLine1);
      lines[2].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[2], g_szTestMsgLine2);
      lines[3].ReplaceAll("\r\n", "\n");
      XII_TEST_STRING(lines[3], g_szTestMsgLine3);
    }

    out.ReplaceAll("\r\n", "\n");
    XII_TEST_STRING(out, g_szTestMsg);
    XII_TEST_BOOL_MSG(err.IsEmpty(), "Error stream should be empty.");
  }
}
#endif
