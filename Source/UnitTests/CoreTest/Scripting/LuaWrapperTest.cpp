/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Scripting/LuaWrapper.h>

XII_CREATE_SIMPLE_TEST_GROUP(Scripting);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static const char* g_Script = "\
globaltable = true;\n\
function f_globaltable()\n\
end\n\
function f_NotWorking()\n\
  DoNothing();\n\
end\n\
intvar1 = 4;\n\
intvar2 = 7;\n\
floatvar1 = 4.3;\n\
floatvar2 = 7.3;\n\
doublevar1 = 12.12;\n\
doublevar2 = 22.22;\n\
boolvar1 = true;\n\
boolvar2 = false;\n\
stringvar1 = \"zweiundvierzig\";\n\
stringvar2 = \"OhWhatsInHere\";\n\
\n\
\n\
function f1()\n\
end\n\
\n\
function f2()\n\
end\n\
\n\
MyTable =\n\
{\n\
  table1 = true;\n\
  \n\
  f_table1 = function()\n\
  end;\n\
  intvar1 = 14;\n\
  intvar2 = 17;\n\
  floatvar1 = 14.3;\n\
  floatvar2 = 17.3;\n\
  doublevar1 = 121.12;\n\
  doublevar2 = 222.22;\n\
  boolvar1 = false;\n\
  boolvar2 = true;\n\
  stringvar1 = \"+zweiundvierzig\";\n\
  stringvar2 = \"+OhWhatsInHere\";\n\
  \n\
  SubTable =\n\
  {\n\
    table2 = true;\n\
    f_table2 = function()\n\
    end;\n\
    intvar1 = 24;\n\
  };\n\
};\n\
\n\
";

class ScriptLog : public xiiLogInterface
{
public:
  virtual void HandleLogMessage(const xiiLoggingEventData& le) override
  {
    XII_TEST_FAILURE("Script Error", le.m_sText);
    XII_TEST_DEBUG_BREAK;
  }
};

class ScriptLogIgnore : public xiiLogInterface
{
public:
  static xiiInt32 g_iErrors;

  virtual void HandleLogMessage(const xiiLoggingEventData& le) override
  {
    switch (le.m_EventType)
    {
      case xiiLogMsgType::ErrorMsg:
      case xiiLogMsgType::SeriousWarningMsg:
      case xiiLogMsgType::WarningMsg:
        ++g_iErrors;
      default:
        break;
    }
  }
};

xiiInt32 ScriptLogIgnore::g_iErrors = 0;

int MyFunc1(lua_State* pState)
{
  xiiLuaWrapper s(pState);

  XII_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  return s.ReturnToScript();
}

int MyFunc2(lua_State* pState)
{
  xiiLuaWrapper s(pState);

  XII_TEST_INT(s.GetNumberOfFunctionParameters(), 7);
  XII_TEST_BOOL(s.IsParameterBool(0));
  XII_TEST_BOOL(s.IsParameterFloat(1));
  XII_TEST_BOOL(s.IsParameterInt(2));
  XII_TEST_BOOL(s.IsParameterNil(3));
  XII_TEST_BOOL(s.IsParameterString(4));
  XII_TEST_BOOL(s.IsParameterString(5));
  XII_TEST_BOOL(s.IsParameterDouble(6));

  XII_TEST_BOOL(s.GetBoolParameter(0) == true);
  XII_TEST_FLOAT(s.GetFloatParameter(1), 2.3f, 0.0001f);
  XII_TEST_INT(s.GetIntParameter(2), 42);
  XII_TEST_STRING(s.GetStringParameter(4), "test");
  XII_TEST_STRING(s.GetStringParameter(5), "tut");
  XII_TEST_DOUBLE(s.GetDoubleParameter(6), 22.3, 0.0001);

  return s.ReturnToScript();
}

int MyFunc3(lua_State* pState)
{
  xiiLuaWrapper s(pState);

  XII_TEST_INT(s.GetNumberOfFunctionParameters(), 0);

  s.PushReturnValue(false);
  s.PushReturnValue(2.3f);
  s.PushReturnValue(42);
  s.PushReturnValueNil();
  s.PushReturnValue("test");
  s.PushReturnValue("tuttut", 3);
  s.PushReturnValue(22.3);

  return s.ReturnToScript();
}

int MyFunc4(lua_State* pState)
{
  xiiLuaWrapper s(pState);

  XII_TEST_INT(s.GetNumberOfFunctionParameters(), 1);

  XII_TEST_BOOL(s.IsParameterTable(0));

  XII_TEST_BOOL(s.OpenTableFromParameter(0) == XII_SUCCESS);

  XII_TEST_BOOL(s.IsVariableAvailable("table1") == true);

  s.CloseAllTables();

  return s.ReturnToScript();
}

XII_CREATE_SIMPLE_TEST(Scripting, LuaWrapper)
{
  ScriptLog       Log;
  ScriptLogIgnore LogIgnore;

  xiiLuaWrapper sMain;
  XII_TEST_BOOL(sMain.ExecuteString(g_Script, "MainScript", &Log) == XII_SUCCESS);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExecuteString")
  {
    xiiLuaWrapper s;
    ScriptLogIgnore::g_iErrors = 0;

    XII_TEST_BOOL(s.ExecuteString(" pups ", "FailToCompile", &LogIgnore) == XII_FAILURE);
    XII_TEST_INT(ScriptLogIgnore::g_iErrors, 1);

    XII_TEST_BOOL(s.ExecuteString(" pups(); ", "FailToExecute", &LogIgnore) == XII_FAILURE);
    XII_TEST_INT(ScriptLogIgnore::g_iErrors, 2);

    XII_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == XII_SUCCESS);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Clear")
  {
    xiiLuaWrapper s;
    XII_TEST_BOOL(s.ExecuteString(g_Script, "MainScript", &Log) == XII_SUCCESS);

    XII_TEST_BOOL(s.IsVariableAvailable("globaltable") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("boolvar1") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("boolvar2") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("intvar1") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("intvar2") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("floatvar1") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("floatvar2") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("stringvar1") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("stringvar2") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("doublevar1") == true);
    XII_TEST_BOOL(s.IsVariableAvailable("doublevar2") == true);

    s.Clear();

    // after clearing the script, these variables should not be available anymore
    XII_TEST_BOOL(s.IsVariableAvailable("globaltable") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("boolvar1") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("boolvar2") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("intvar1") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("intvar2") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("floatvar1") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("floatvar2") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("stringvar1") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("stringvar2") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("doublevar1") == false);
    XII_TEST_BOOL(s.IsVariableAvailable("doublevar2") == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsVariableAvailable (Global)")
  {
    XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("nonexisting1") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("boolvar1") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("boolvar2") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("nonexisting2") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("intvar1") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("intvar2") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("nonexisting3") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("floatvar1") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("floatvar2") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("nonexisting4") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("stringvar2") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("nonexisting5") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("doublevar1") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("doublevar2") == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "IsFunctionAvailable (Global)")
  {
    XII_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting1") == false);
    XII_TEST_BOOL(sMain.IsFunctionAvailable("f1") == true);
    XII_TEST_BOOL(sMain.IsFunctionAvailable("f2") == true);
    XII_TEST_BOOL(sMain.IsFunctionAvailable("nonexisting2") == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIntVariable (Global)")
  {
    XII_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    XII_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
    XII_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    XII_TEST_INT(sMain.GetIntVariable("intvar2", 13), 7);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetIntVariable (Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_INT(sMain.GetIntVariable("nonexisting1", 13), 13);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    XII_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);
    XII_TEST_INT(sMain.GetIntVariable("nonexisting2", 14), 14);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    XII_TEST_INT(sMain.GetIntVariable("intvar2", 13), 17);

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFloatVariable (Global)")
  {
    XII_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 7.3f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDoubleVariable (Global)")
  {
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("nonexisting1", 13), 13, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 12.12, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar2", 13), 22.22, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("nonexisting2", 14), 14, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 12.12, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar2", 13), 22.22, xiiMath::DefaultEpsilon<double>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetFloatVariable (Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_FLOAT(sMain.GetFloatVariable("nonexisting1", 13), 13, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("nonexisting2", 14), 14, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, xiiMath::DefaultEpsilon<float>());
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar2", 13), 17.3f, xiiMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetDoubleVariable (Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_DOUBLE(sMain.GetDoubleVariable("nonexisting1", 13), 13, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 121.12, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar2", 13), 222.22, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("nonexisting2", 14), 14, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 121.12, xiiMath::DefaultEpsilon<double>());
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar2", 13), 222.22, xiiMath::DefaultEpsilon<double>());

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetBoolVariable (Global)")
  {
    XII_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
    XII_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar1", false) == true);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar2", true) == false);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetBoolVariable (Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_BOOL(sMain.GetBoolVariable("nonexisting1", true) == true);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);
    XII_TEST_BOOL(sMain.GetBoolVariable("nonexisting2", false) == false);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar1", true) == false);
    XII_TEST_BOOL(sMain.GetBoolVariable("boolvar2", false) == true);

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetStringVariable (Global)")
  {
    XII_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
    XII_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "OhWhatsInHere");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "GetStringVariable (Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_STRING(sMain.GetStringVariable("nonexisting1", "a"), "a");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");
    XII_TEST_STRING(sMain.GetStringVariable("nonexisting2", "b"), "b");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "a"), "+zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar2", "a"), "+OhWhatsInHere");

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (int, Global)")
  {
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
    sMain.SetVariable("intvar1", 27);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 27);
    sMain.SetVariable("intvar1", 4);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 4);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (int, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);
    sMain.SetVariable("intvar1", 127);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 127);
    sMain.SetVariable("intvar1", 14);
    XII_TEST_INT(sMain.GetIntVariable("intvar1", 13), 14);

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (float, Global)")
  {
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, xiiMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 27.3f);
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 27.3f, xiiMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 4.3f);
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 4.3f, xiiMath::DefaultEpsilon<float>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (double, Global)")
  {
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 12.12, xiiMath::DefaultEpsilon<double>());
    sMain.SetVariable("doublevar1", 27.3);
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 27.3, xiiMath::DefaultEpsilon<double>());
    sMain.SetVariable("doublevar1", 12.12);
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 12.12, xiiMath::DefaultEpsilon<double>());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (float, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, xiiMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 127.3f);
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 127.3f, xiiMath::DefaultEpsilon<float>());
    sMain.SetVariable("floatvar1", 14.3f);
    XII_TEST_FLOAT(sMain.GetFloatVariable("floatvar1", 13), 14.3f, xiiMath::DefaultEpsilon<float>());

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (double, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 121.12, xiiMath::DefaultEpsilon<double>());
    sMain.SetVariable("doublevar1", 127.3);
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 127.3, xiiMath::DefaultEpsilon<double>());
    sMain.SetVariable("doublevar1", 121.12);
    XII_TEST_DOUBLE(sMain.GetDoubleVariable("doublevar1", 13), 121.12, xiiMath::DefaultEpsilon<double>());

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (bool, Global)")
  {
    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (bool, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);
    sMain.SetVariable("boolvar1", true);
    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", false), true);
    sMain.SetVariable("boolvar1", false);
    XII_TEST_INT(sMain.GetBoolVariable("boolvar1", true), false);

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (string, Global)")
  {
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "test1");
    sMain.SetVariable("stringvar1", "zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariable("stringvar1", "test1", 3);
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "tes");
    sMain.SetVariable("stringvar1", "zweiundvierzigabc", 14);
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (string, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+test1");
    sMain.SetVariable("stringvar1", "+zweiundvierzig");
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariable("stringvar1", "+test1", 4);
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+tes");
    sMain.SetVariable("stringvar1", "+zweiundvierzigabc", 15);
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (nil, Global)")
  {
    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    XII_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "zweiundvierzig");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "SetVariable (nil, Table)")
  {
    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);

    XII_TEST_STRING(sMain.GetStringVariable("stringvar1", "bla"), "+zweiundvierzig");

    sMain.SetVariableNil("stringvar1");

    XII_TEST_BOOL(sMain.IsVariableAvailable("stringvar1") == false); // It is Nil -> 'not available'

    sMain.SetVariable("stringvar1", "+zweiundvierzig");

    sMain.CloseTable();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "OpenTable")
  {
    XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == true);
    XII_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
    XII_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

    XII_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == true);
    XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
    XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

    XII_TEST_BOOL(sMain.OpenTable("NotMyTable") == XII_FAILURE);

    XII_TEST_BOOL(sMain.OpenTable("MyTable") == XII_SUCCESS);
    {
      XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      XII_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      XII_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      XII_TEST_BOOL(sMain.OpenTable("NotMyTable") == XII_FAILURE);

      XII_TEST_BOOL(sMain.OpenTable("SubTable") == XII_SUCCESS);
      {
        XII_TEST_BOOL(sMain.OpenTable("NotMyTable") == XII_FAILURE);

        XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        XII_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        XII_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
      XII_TEST_BOOL(sMain.IsVariableAvailable("table1") == true);
      XII_TEST_BOOL(sMain.IsVariableAvailable("table2") == false);

      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == true);
      XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == false);

      XII_TEST_BOOL(sMain.OpenTable("NotMyTable") == XII_FAILURE);

      XII_TEST_BOOL(sMain.OpenTable("SubTable") == XII_SUCCESS);
      {
        XII_TEST_BOOL(sMain.OpenTable("NotMyTable") == XII_FAILURE);

        XII_TEST_BOOL(sMain.IsVariableAvailable("globaltable") == false);
        XII_TEST_BOOL(sMain.IsVariableAvailable("table1") == false);
        XII_TEST_BOOL(sMain.IsVariableAvailable("table2") == true);

        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_globaltable") == false);
        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table1") == false);
        XII_TEST_BOOL(sMain.IsFunctionAvailable("f_table2") == true);

        sMain.CloseTable();
      }

      sMain.CloseTable();
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "RegisterCFunction")
  {
    XII_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == false);

    sMain.RegisterCFunction("Func1", MyFunc1);

    XII_TEST_BOOL(sMain.IsFunctionAvailable("Func1") == true);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call Lua Function")
  {
    XII_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    XII_TEST_BOOL(sMain.PrepareFunctionCall("f_globaltable") == true);
    XII_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == XII_SUCCESS);

    ScriptLogIgnore::g_iErrors = 0;
    XII_TEST_BOOL(sMain.PrepareFunctionCall("f_NotWorking") == true);
    XII_TEST_BOOL(sMain.CallPreparedFunction(0, &LogIgnore) == XII_FAILURE);
    XII_TEST_INT(ScriptLogIgnore::g_iErrors, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call C Function")
  {
    XII_TEST_BOOL(sMain.PrepareFunctionCall("NotExisting") == false);

    if (sMain.IsFunctionAvailable("Func1") == false)
      sMain.RegisterCFunction("Func1", MyFunc1);

    XII_TEST_BOOL(sMain.PrepareFunctionCall("Func1") == true);

    XII_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == XII_SUCCESS);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call C Function with Parameters")
  {
    if (sMain.IsFunctionAvailable("Func2") == false)
      sMain.RegisterCFunction("Func2", MyFunc2);

    XII_TEST_BOOL(sMain.PrepareFunctionCall("Func2") == true);

    sMain.PushParameter(true);
    sMain.PushParameter(2.3f);
    sMain.PushParameter(42);
    sMain.PushParameterNil();
    sMain.PushParameter("test");
    sMain.PushParameter("tuttut", 3);
    sMain.PushParameter(22.3);

    XII_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == XII_SUCCESS);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call C Function with Return Values")
  {
    if (sMain.IsFunctionAvailable("Func3") == false)
      sMain.RegisterCFunction("Func3", MyFunc3);

    XII_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    XII_TEST_BOOL(sMain.CallPreparedFunction(7, &Log) == XII_SUCCESS);

    XII_TEST_BOOL(sMain.IsReturnValueBool(0));
    XII_TEST_BOOL(sMain.IsReturnValueFloat(1));
    XII_TEST_BOOL(sMain.IsReturnValueInt(2));
    XII_TEST_BOOL(sMain.IsReturnValueNil(3));
    XII_TEST_BOOL(sMain.IsReturnValueString(4));
    XII_TEST_BOOL(sMain.IsReturnValueString(5));
    XII_TEST_BOOL(sMain.IsReturnValueDouble(6));

    XII_TEST_BOOL(sMain.GetBoolReturnValue(0) == false);
    XII_TEST_FLOAT(sMain.GetFloatReturnValue(1), 2.3f, 0.0001f);
    XII_TEST_INT(sMain.GetIntReturnValue(2), 42);
    XII_TEST_STRING(sMain.GetStringReturnValue(4), "test");
    XII_TEST_STRING(sMain.GetStringReturnValue(5), "tut");
    XII_TEST_DOUBLE(sMain.GetDoubleReturnValue(6), 22.3, 0.0001);

    sMain.DiscardReturnValues();

    XII_TEST_BOOL(sMain.PrepareFunctionCall("Func3") == true);
    XII_TEST_BOOL(sMain.CallPreparedFunction(7, &Log) == XII_SUCCESS);

    sMain.DiscardReturnValues();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call C Function with Table Parameter")
  {
    if (sMain.IsFunctionAvailable("Func4") == false)
      sMain.RegisterCFunction("Func4", MyFunc4);

    XII_TEST_BOOL(sMain.PrepareFunctionCall("Func4") == true);

    sMain.PushTable("MyTable", true);

    XII_TEST_BOOL(sMain.CallPreparedFunction(0, &Log) == XII_SUCCESS);
  }
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT
