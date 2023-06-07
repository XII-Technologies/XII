#include <CoreTest/CoreTestPCH.h>

#ifdef BUILDSYSTEM_ENABLE_DUKTAPE_SUPPORT

#  include <Core/Scripting/DuktapeContext.h>

#  include <Duktape/duktape.h>
#  include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#  include <Foundation/IO/FileSystem/FileReader.h>
#  include <Foundation/IO/FileSystem/FileSystem.h>
#  include <Foundation/IO/FileSystem/FileWriter.h>
#  include <TestFramework/Utilities/TestLogInterface.h>

static xiiResult TranspileString(const char* szSource, xiiDuktapeContext& ref_script, xiiStringBuilder& ref_sResult)
{
  ref_script.PushGlobalObject();                                            // [ global ]
  ref_script.PushLocalObject("ts").IgnoreResult();                          // [ global ts ]
  XII_SUCCEED_OR_RETURN(ref_script.PrepareObjectFunctionCall("transpile")); // [ global ts transpile ]
  ref_script.PushString(szSource);                                          // [ global ts transpile source ]
  XII_SUCCEED_OR_RETURN(ref_script.CallPreparedFunction());                 // [ global ts result ]
  ref_sResult = ref_script.GetStringValue(-1);                              // [ global ts result ]
  ref_script.PopStack(3);                                                   // [ ]

  return XII_SUCCESS;
}

static xiiResult TranspileFile(const char* szFile, xiiDuktapeContext& ref_script, xiiStringBuilder& ref_sResult)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  xiiStringBuilder source;
  source.ReadAll(file);

  return TranspileString(source, ref_script, ref_sResult);
}

static xiiResult TranspileFileToJS(const char* szFile, xiiDuktapeContext& ref_script, xiiStringBuilder& ref_sResult)
{
  XII_SUCCEED_OR_RETURN(TranspileFile(szFile, ref_script, ref_sResult));

  xiiStringBuilder sFile(":TypeScriptTest/", szFile);
  sFile.ChangeFileExtension("js");

  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  XII_SUCCEED_OR_RETURN(file.WriteBytes(ref_sResult.GetData(), ref_sResult.GetElementCount()));
  return XII_SUCCESS;
}

static int Duk_Print(duk_context* pContext)
{
  xiiDuktapeFunction duk(pContext);

  xiiLog::Info(duk.GetStringValue(0));

  return duk.ReturnVoid();
}

static duk_ret_t ModuleSearchFunction2(duk_context* pCtx)
{
  xiiDuktapeFunction script(pCtx);

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

XII_CREATE_SIMPLE_TEST(Scripting, TypeScript)
{
  // setup file system
  {
    xiiFileSystem::RegisterDataDirectoryFactory(xiiDataDirectory::FolderType::Factory);

    xiiStringBuilder sTestDataDir(">sdk/", xiiTestFramework::GetInstance()->GetRelTestDataPath());
    sTestDataDir.AppendPath("Scripting/TypeScript");
    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(sTestDataDir, "TypeScriptTest", "TypeScriptTest", xiiFileSystem::AllowWrites)))
      return;

    if (!XII_TEST_RESULT(xiiFileSystem::AddDataDirectory(">sdk/Data/Tools/xiiEditor", "DuktapeTest")))
      return;
  }

  xiiDuktapeContext duk("DukTS");
  duk.EnableModuleSupport(ModuleSearchFunction2);

  duk.RegisterGlobalFunction("Print", Duk_Print, 1);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compile TypeScriptServices") { XII_TEST_RESULT(duk.ExecuteFile("Typescript/typescriptServices.js")); }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpile Simple")
  {
    // simple way
    XII_TEST_RESULT(duk.ExecuteString("ts.transpile('class X{}');"));

    // complicated way, needed to retrieve the result
    xiiStringBuilder sTranspiled;
    TranspileString("class X{}", duk, sTranspiled).IgnoreResult();

    // validate that the transpiled code can be executed by Duktape
    xiiDuktapeContext duk2("duk");
    XII_TEST_RESULT(duk2.ExecuteString(sTranspiled));
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Transpile File")
  {
    xiiStringBuilder result;
    XII_TEST_RESULT(TranspileFileToJS("Foo.ts", duk, result));

    duk.ExecuteFile("Foo.js").IgnoreResult();
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Import files")
  {
    xiiStringBuilder result;
    XII_TEST_RESULT(TranspileFileToJS("Bar.ts", duk, result));

    duk.ExecuteFile("Bar.js").IgnoreResult();
  }

  xiiFileSystem::DeleteFile(":TypeScriptTest/Foo.js");
  xiiFileSystem::DeleteFile(":TypeScriptTest/Bar.js");

  xiiFileSystem::RemoveDataDirectoryGroup("DuktapeTest");
}

#endif
