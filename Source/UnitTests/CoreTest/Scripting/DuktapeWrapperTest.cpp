#include <CoreTest/CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeContext.h>

#  include <Duktape/duk_module_duktape.h>
#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static duk_ret_t ModuleSearchFunction(duk_context* ctx);

static int CFuncPrint(duk_context* pContext)
{
  xiiDuktapeFunction wrapper(pContext);
  const char*        szText = wrapper.GetStringValue(0, nullptr);

  xiiLog::Info("Print: '{}'", szText);
  return wrapper.ReturnVoid();
}

static int CFuncPrintVA(duk_context* pContext)
{
  xiiDuktapeFunction wrapper(pContext);

  const xiiUInt32 uiNumArgs = wrapper.GetNumVarArgFunctionParameters();

  xiiStringBuilder s;
  s.AppendFormat("#Args: {}", uiNumArgs);

  for (xiiUInt32 arg = 0; arg < uiNumArgs; ++arg)
  {
    if (wrapper.IsNumber(arg))
    {
      double val = wrapper.GetNumberValue(arg);
      s.AppendFormat(", #{}: Number = {}", arg, val);
    }
    else if (wrapper.IsBool(arg))
    {
      bool val = wrapper.GetBoolValue(arg);
      s.AppendFormat(", #{}: Bool = {}", arg, val);
    }
    else if (wrapper.IsString(arg))
    {
      const char* val = wrapper.GetStringValue(arg);
      s.AppendFormat(", #{}: String = {}", arg, val);
    }
    else if (wrapper.IsNull(arg))
    {
      s.AppendFormat(", #{}: null", arg);
    }
    else if (wrapper.IsUndefined(arg))
    {
      s.AppendFormat(", #{}: undefined", arg);
    }
    else if (wrapper.IsObject(arg))
    {
      s.AppendFormat(", #{}: object", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_BUFFER))
    {
      s.AppendFormat(", #{}: buffer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_POINTER))
    {
      s.AppendFormat(", #{}: pointer", arg);
    }
    else if (duk_check_type_mask(pContext, arg, DUK_TYPE_MASK_LIGHTFUNC))
    {
      s.AppendFormat(", #{}: lightfunc", arg);
    }
    else
    {
      s.AppendFormat(", #{}: UNKNOWN TYPE", arg);
    }
  }

  xiiLog::Info(s);
  return wrapper.ReturnString(s);
}

static int CFuncMagic(duk_context* pContext)
{
  xiiDuktapeFunction wrapper(pContext);
  xiiInt16           iMagic = wrapper.GetFunctionMagicValue();

  xiiLog::Info("Magic: '{}'", iMagic);
  return wrapper.ReturnInt(iMagic);
}


XII_CREATE_SIMPLE_TEST(Scripting, DuktapeWrapper)
{
  // setup file system
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    xiiStringBuilder sTestDataDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/Duktape");
    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(sTestDataDir, "DuktapeTest")))
      return;
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Basics")
  {
    xiiDuktapeContext duk("DukTest");

    duk_eval_string(duk.GetContext(), "'testString'.toUpperCase()");
    xiiStringBuilder sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    XII_TEST_STRING(sTestString, "TESTSTRING");

    XII_TEST_RESULT(duk.ExecuteString("function MakeUpper(bla) { return bla.toUpperCase() }"));


    duk_eval_string(duk.GetContext(), "MakeUpper(\"myTest\")");
    sTestString = duk_get_string(duk.GetContext(), -1);
    duk_pop(duk.GetContext());
    XII_TEST_STRING(sTestString, "MYTEST");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExecuteString (error)")
  {
    xiiDuktapeContext duk("DukTest");

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("SyntaxError: parse error (line 1)", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(duk.ExecuteString(" == invalid code == ").Failed());

    log.ExpectMessage("ReferenceError: identifier 'Print' undefined", xiiLogMsgType::ErrorMsg);
    XII_TEST_BOOL(duk.ExecuteString("Print(\"do stuff\")").Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "ExecuteFile")
  {
    xiiDuktapeContext duk("DukTest");
    duk.EnableModuleSupport(nullptr);

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Print: 'called f1'", xiiLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    duk.ExecuteFile("ExecuteFile.js").IgnoreResult();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "C Function")
  {
    xiiDuktapeContext duk("DukTest");

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Hello Test", xiiLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    duk.ExecuteString("Print('Hello Test')").IgnoreResult();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "VarArgs C Function")
  {
    xiiDuktapeContext duk("DukTest");

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("#Args: 5, #0: String = text, #1: Number = 7, #2: Bool = true, #3: null, #4: object", xiiLogMsgType::InfoMsg);

    duk.RegisterGlobalFunctionWithVarArgs("PrintVA", CFuncPrintVA);

    duk.ExecuteString("PrintVA('text', 7, true, null, {})").IgnoreResult();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Call Function")
  {
    xiiDuktapeContext duk("DukTest");

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("You did it", xiiLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    if (XII_TEST_RESULT(duk.PrepareGlobalFunctionCall("Print"))) // [ Print ] / [ ]
    {
      duk.PushString("You did it, Fry!");          // [ Print String ]
      XII_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                              // [ ]
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Function Magic Value")
  {
    xiiDuktapeContext duk("DukTest");

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);

    log.ExpectMessage("Magic: '1'", xiiLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '2'", xiiLogMsgType::InfoMsg);
    log.ExpectMessage("Magic: '3'", xiiLogMsgType::InfoMsg);

    duk.RegisterGlobalFunction("Magic1", CFuncMagic, 0, 1);
    duk.RegisterGlobalFunction("Magic2", CFuncMagic, 0, 2);
    duk.RegisterGlobalFunction("Magic3", CFuncMagic, 0, 3);

    if (XII_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic1"))) // [ Magic1 ]
    {
      XII_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                              // [ ]
    }

    if (XII_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic2"))) // [ Magic2 ]
    {
      XII_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                              // [ ]
    }

    if (XII_TEST_RESULT(duk.PrepareGlobalFunctionCall("Magic3"))) // [ Magic2 ]
    {
      XII_TEST_RESULT(duk.CallPreparedFunction()); // [ result ]
      duk.PopStack();                              // [ ]
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Inspect Object")
  {
    xiiDuktapeContext duk("DukTest");
    xiiDuktapeHelper  val(duk);
    XII_TEST_RESULT(duk.ExecuteFile("Object.js"));

    duk.PushGlobalObject();                      // [ global ]
    XII_TEST_RESULT(duk.PushLocalObject("obj")); // [ global obj ]

    XII_TEST_BOOL(duk.HasProperty("i"));
    XII_TEST_INT(duk.GetIntProperty("i", 0), 23);

    XII_TEST_BOOL(duk.HasProperty("f"));
    XII_TEST_FLOAT(duk.GetFloatProperty("f", 0), 4.2f, 0.01f);
    XII_TEST_DOUBLE(duk.GetNumberProperty("f", 0), 4.2, 0.01);

    XII_TEST_BOOL(duk.HasProperty("b"));
    XII_TEST_BOOL(duk.GetBoolProperty("b", false));

    XII_TEST_BOOL(duk.HasProperty("s"));
    XII_TEST_STRING(duk.GetStringProperty("s", ""), "text");

    XII_TEST_BOOL(duk.HasProperty("n"));

    XII_TEST_BOOL(duk.HasProperty("o"));

    {
      XII_TEST_RESULT(duk.PushLocalObject("o")); // [ global obj o ]
      XII_TEST_BOOL(duk.HasProperty("sub"));
      XII_TEST_STRING(duk.GetStringProperty("sub", ""), "wub");
    }

    duk.PopStack(3); // [ ]
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "require")
  {
    xiiDuktapeContext duk("DukTest");
    duk.EnableModuleSupport(ModuleSearchFunction);

    duk.RegisterGlobalFunction("Print", CFuncPrint, 1);

    xiiTestLogInterface   log;
    xiiTestLogSystemScope logSystemScope(&log);
    log.ExpectMessage("Print: 'called f1'", xiiLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'Called require.js'", xiiLogMsgType::InfoMsg);
    log.ExpectMessage("Print: 'require.js: called f1'", xiiLogMsgType::InfoMsg);

    XII_TEST_RESULT(duk.ExecuteFile("require.js"));
  }

  xiiFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

static duk_ret_t ModuleSearchFunction(duk_context* ctx)
{
  xiiDuktapeFunction script(ctx);

  /* Nargs was given as 4 and we get the following stack arguments:
   *   index 0: id
   *   index 1: require
   *   index 2: exports
   *   index 3: module
   */

  xiiStringBuilder id = script.GetStringValue(0);
  id.ChangeFileExtension("js");

  xiiStringBuilder source;
  xiiFileReader    file;
  file.Open(id).IgnoreResult();
  source.ReadAll(file);

  return script.ReturnString(source);

  /* Return 'undefined' to indicate no source code. */
  // return 0;
}

#endif
