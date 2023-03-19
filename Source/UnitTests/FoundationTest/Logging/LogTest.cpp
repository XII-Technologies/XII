#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Threading/Thread.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST_GROUP(Logging);

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
          m_Result.Append(">", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::EndGroup:
          m_Result.Append("<", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::ErrorMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::SeriousWarningMsg:
          m_Result.Append("SW:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::WarningMsg:
          m_Result.Append("W:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::SuccessMsg:
          m_Result.Append("S:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::InfoMsg:
          m_Result.Append("I:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::DevMsg:
          m_Result.Append("E:", le.m_sTag, " ", le.m_sText, "\n");
          break;
        case xiiLogMsgType::DebugMsg:
          m_Result.Append("D:", le.m_sTag, " ", le.m_sText, "\n");
          break;

        default:
          XII_REPORT_FAILURE("Invalid msg type");
          break;
      }
    }

    xiiStringBuilder m_Result;
  };

} // namespace

XII_CREATE_SIMPLE_TEST(Logging, Log)
{
  LogTestLogInterface log;
  LogTestLogInterface log2;
  xiiLogSystemScope   logScope(&log);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Output")
  {
    XII_LOG_BLOCK("Verse 1", "Portal: Still Alive");

    xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::All);

    xiiLog::Success("{0}", "This was a triumph.");
    xiiLog::Info("{0}", "I'm making a note here:");
    xiiLog::Error("{0}", "Huge Success");
    xiiLog::Info("{0}", "It's hard to overstate my satisfaction.");
    xiiLog::Dev("{0}", "Aperture Science. We do what we must, because we can,");
    xiiLog::Dev("{0}", "For the good of all of us, except the ones who are dead.");
    xiiLog::Flush();
    xiiLog::Flush(); // second flush should be ignored

    {
      XII_LOG_BLOCK("Verse 2");

      xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::DevMsg);

      xiiLog::Dev("But there's no sense crying over every mistake.");
      xiiLog::Debug("You just keep on trying 'till you run out of cake.");
      xiiLog::Info("And the science gets done, and you make a neat gun");
      xiiLog::Error("for the people who are still alive.");
    }

    {
      XII_LOG_BLOCK("Verse 3");

      xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::InfoMsg);

      xiiLog::Info("I'm not even angry.");
      xiiLog::Debug("I'm being so sincere right now.");
      xiiLog::Dev("Even though you broke my heart and killed me.");
      xiiLog::Info("And tore me to pieces,");
      xiiLog::Dev("and threw every piece into a fire.");
      xiiLog::Info("As they burned it hurt because I was so happy for you.");
      xiiLog::Error("Now these points of data make a beautiful line");
      xiiLog::Dev("and we're off the beta, we're releasing on time.");
      xiiLog::Flush();
      xiiLog::Flush();

      {
        XII_LOG_BLOCK("Verse 4");

        xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::SuccessMsg);

        xiiLog::Info("So I'm glad I got burned,");
        xiiLog::Debug("think of all the things we learned");
        xiiLog::Debug("for the people who are still alive.");

        {
          xiiLogSystemScope logScope2(&log2);
          XII_LOG_BLOCK("Interlude");
          xiiLog::Info("Well here we are again. It's always such a pleasure.");
          xiiLog::Error("Remember when you tried to kill me twice?");
        }

        {
          XII_LOG_BLOCK("Verse 5");

          xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::WarningMsg);

          xiiLog::Debug("Go ahead and leave me.");
          xiiLog::Info("I think I prefer to stay inside.");
          xiiLog::Dev("Maybe you'll find someone else, to help you.");
          xiiLog::Dev("Maybe Black Mesa.");
          xiiLog::Info("That was a joke. Haha. Fat chance.");
          xiiLog::Warning("Anyway, this cake is great.");
          xiiLog::Success("It's so delicious and moist.");
          xiiLog::Dev("Look at me still talking when there's science to do.");
          xiiLog::Error("When I look up there it makes me glad I'm not you.");
          xiiLog::Info("I've experiments to run,");
          xiiLog::SeriousWarning("there is research to be done on the people who are still alive.");
        }
      }
    }
  }

  {
    XII_LOG_BLOCK("Verse 6", "Last One");

    xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::ErrorMsg);

    xiiLog::Dev("And believe me I am still alive.");
    xiiLog::Info("I'm doing science and I'm still alive.");
    xiiLog::Success("I feel fantastic and I'm still alive.");
    xiiLog::Warning("While you're dying I'll be still alive.");
    xiiLog::Error("And when you're dead I will be, still alive.");
    xiiLog::Debug("Still alive, still alive.");
  }

  /// \todo This test will fail if XII_COMPILE_FOR_DEVELOPMENT is disabled.
  /// We also currently don't test xiiLog::Debug, because our build machines compile in release and then the text below would need to be
  /// different.

  const char* szResult   = log.m_Result;
  const char* szExpected = "\
>Portal: Still Alive Verse 1\n\
S: This was a triumph.\n\
I: I'm making a note here:\n\
E: Huge Success\n\
I: It's hard to overstate my satisfaction.\n\
E: Aperture Science. We do what we must, because we can,\n\
E: For the good of all of us, except the ones who are dead.\n\
[Flush]\n\
> Verse 2\n\
E: But there's no sense crying over every mistake.\n\
I: And the science gets done, and you make a neat gun\n\
E: for the people who are still alive.\n\
< Verse 2\n\
> Verse 3\n\
I: I'm not even angry.\n\
I: And tore me to pieces,\n\
I: As they burned it hurt because I was so happy for you.\n\
E: Now these points of data make a beautiful line\n\
[Flush]\n\
> Verse 4\n\
> Verse 5\n\
W: Anyway, this cake is great.\n\
E: When I look up there it makes me glad I'm not you.\n\
SW: there is research to be done on the people who are still alive.\n\
< Verse 5\n\
< Verse 4\n\
< Verse 3\n\
<Portal: Still Alive Verse 1\n\
>Last One Verse 6\n\
E: And when you're dead I will be, still alive.\n\
<Last One Verse 6\n\
";

  XII_TEST_STRING(szResult, szExpected);

  const char* szResult2   = log2.m_Result;
  const char* szExpected2 = "\
> Interlude\n\
I: Well here we are again. It's always such a pleasure.\n\
E: Remember when you tried to kill me twice?\n\
< Interlude\n\
";

  XII_TEST_STRING(szResult2, szExpected2);
}

XII_CREATE_SIMPLE_TEST(Logging, GlobalTestLog)
{
  xiiLog::GetThreadLocalLogSystem()->SetLogLevel(xiiLogMsgType::All);

  {
    xiiTestLogInterface   log;
    xiiTestLogSystemScope scope(&log, true);

    log.ExpectMessage("managed to break", xiiLogMsgType::ErrorMsg);
    log.ExpectMessage("my heart", xiiLogMsgType::WarningMsg);
    log.ExpectMessage("see you", xiiLogMsgType::WarningMsg, 10);

    {
      class LogThread : public xiiThread
      {
      public:
        virtual xiiUInt32 Run() override
        {
          xiiLog::Warning("I see you!");
          xiiLog::Debug("Test debug");
          return 0;
        }
      };

      LogThread thread[10];

      for (xiiUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Start();
      }

      xiiLog::Error("The only thing you managed to break so far");
      xiiLog::Warning("is my heart");

      for (xiiUInt32 i = 0; i < 10; ++i)
      {
        thread[i].Join();
      }
    }
  }
}
