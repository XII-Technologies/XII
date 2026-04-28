/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Utilities/CommandLineUtils.h>

XII_CREATE_SIMPLE_TEST(Utility, CommandLineUtils)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetParameterCount / GetParameter")
  {
    const int   argc       = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    XII_TEST_INT(CmdLn.GetParameterCount(), 9);
    XII_TEST_STRING(CmdLn.GetParameter(0), "bla/blub/myprogram.exe");
    XII_TEST_STRING(CmdLn.GetParameter(1), "-Test1");
    XII_TEST_STRING(CmdLn.GetParameter(2), "true");
    XII_TEST_STRING(CmdLn.GetParameter(3), "-Test2");
    XII_TEST_STRING(CmdLn.GetParameter(4), "off");
    XII_TEST_STRING(CmdLn.GetParameter(5), "-Test3");
    XII_TEST_STRING(CmdLn.GetParameter(6), "-Test4");
    XII_TEST_STRING(CmdLn.GetParameter(7), "on");
    XII_TEST_STRING(CmdLn.GetParameter(8), "-Test5");
    CmdLn.InjectCustomArgument("-duh");
    XII_TEST_INT(CmdLn.GetParameterCount(), 10);
    XII_TEST_STRING(CmdLn.GetParameter(9), "-duh");
    CmdLn.InjectCustomArgument("I need my Space");
    XII_TEST_INT(CmdLn.GetParameterCount(), 11);
    XII_TEST_STRING(CmdLn.GetParameter(10), "I need my Space");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetOptionIndex / GetStringOptionArguments  / GetStringOption")
  {
    const int   argc       = 15;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-opt1", "true", "false", "-opt2", "\"test2\"", "-opt3", "-opt4", "one", "two = three",
                              "four", "   five  ", " six ", "-opt5", "-opt6"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    XII_TEST_INT(CmdLn.GetOptionIndex("-opt1"), 1);
    XII_TEST_INT(CmdLn.GetOptionIndex("-opt2"), 4);
    XII_TEST_INT(CmdLn.GetOptionIndex("-opt3"), 6);
    XII_TEST_INT(CmdLn.GetOptionIndex("-opt4"), 7);
    XII_TEST_INT(CmdLn.GetOptionIndex("-opt5"), 13);
    XII_TEST_INT(CmdLn.GetOptionIndex("-opt6"), 14);

    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt1"), 2);
    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt2"), 1);
    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt3"), 0);
    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt4"), 5);
    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt5"), 0);
    XII_TEST_INT(CmdLn.GetStringOptionArguments("-opt6"), 0);

    XII_TEST_STRING(CmdLn.GetStringOption("-opt1", 0), "true");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt1", 1), "false");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt1", 2, "end"), "end");

    XII_TEST_STRING(CmdLn.GetStringOption("-opt2", 0), "\"test2\"");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt2", 1, "end"), "end");

    XII_TEST_STRING(CmdLn.GetStringOption("-opt3", 0, "end"), "end");

    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 0), "one");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 1), "two = three");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 2), "four");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 3), "   five  ");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 4), " six ");
    XII_TEST_STRING(CmdLn.GetStringOption("-opt4", 5, "end"), "end");

    XII_TEST_STRING(CmdLn.GetStringOption("-opt5", 0), "");

    XII_TEST_STRING(CmdLn.GetStringOption("-opt6", 0), "");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetBoolOption")
  {
    const int   argc       = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "true", "-Test2", "off", "-Test3", "-Test4", "on", "-Test5"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test1", true, true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test1", false, true) == false);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-test2", true, true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test2", false, true) == false);

    // case insensitive and wrong
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test1", true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test1", false) == true);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-test2", true) == false);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-test2", false) == false);

    // case sensitive and correct
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test1", true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test1", false) == true);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test2", true) == false);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test2", false) == false);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test3", true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test3", false) == true);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test4", true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test4", false) == true);

    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test5", true) == true);
    XII_TEST_BOOL(CmdLn.GetBoolOption("-Test5", false) == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIntOption")
  {
    const int   argc       = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    XII_TEST_INT(CmdLn.GetIntOption("-test1", 2, true), 2);
    XII_TEST_INT(CmdLn.GetIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    XII_TEST_INT(CmdLn.GetIntOption("-test1", 2), 23);
    XII_TEST_INT(CmdLn.GetIntOption("-test2", 17), 42);

    // case sensitive and correct
    XII_TEST_INT(CmdLn.GetIntOption("-Test1", 2), 23);
    XII_TEST_INT(CmdLn.GetIntOption("-Test2", 3), 42);
    XII_TEST_INT(CmdLn.GetIntOption("-Test3", 4), 4);
    XII_TEST_INT(CmdLn.GetIntOption("-Test4", 5), -11);
    XII_TEST_INT(CmdLn.GetIntOption("-Test5"), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetUIntOption")
  {
    const int   argc       = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23", "-Test2", "42", "-Test3", "-Test4", "-11", "-Test5"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    XII_TEST_INT(CmdLn.GetUIntOption("-test1", 2, true), 2);
    XII_TEST_INT(CmdLn.GetUIntOption("-test2", 17, true), 17);

    // case insensitive and wrong
    XII_TEST_INT(CmdLn.GetUIntOption("-test1", 2), 23);
    XII_TEST_INT(CmdLn.GetUIntOption("-test2", 17), 42);

    // case sensitive and correct
    XII_TEST_INT(CmdLn.GetUIntOption("-Test1", 2), 23);
    XII_TEST_INT(CmdLn.GetUIntOption("-Test2", 3), 42);
    XII_TEST_INT(CmdLn.GetUIntOption("-Test3", 4), 4);
    XII_TEST_INT(CmdLn.GetUIntOption("-Test4", 5), 5);
    XII_TEST_INT(CmdLn.GetUIntOption("-Test5"), 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFloatOption")
  {
    const int   argc       = 9;
    const char* argv[argc] = {"bla/blub/myprogram.exe", "-Test1", "23.45", "-Test2", "42.3", "-Test3", "-Test4", "-11", "-Test5"};

    xiiCommandLineUtils CmdLn;
    CmdLn.SetCommandLine(argc, argv);

    // case sensitive and wrong
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3, true), 2.3, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8, true), 17.8, 0.0);

    // case insensitive and wrong
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-test1", 2.3), 23.45, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-test2", 17.8), 42.3, 0.0);

    // case sensitive and correct
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-Test1", 2.3), 23.45, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-Test2", 3.4), 42.3, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-Test3", 4.5), 4.5, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-Test4", 5.6), -11, 0.0);
    XII_TEST_DOUBLE(CmdLn.GetFloatOption("-Test5"), 0, 0.0);
  }
}
