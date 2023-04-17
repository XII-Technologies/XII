#include <TypeScriptPlugin/TypeScriptPluginPCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Threading/DelegateTask.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <TypeScriptPlugin/Transpiler/Transpiler.h>

xiiTypeScriptTranspiler::xiiTypeScriptTranspiler() :
  m_Transpiler("TypeScript Transpiler")
{
}

xiiTypeScriptTranspiler::~xiiTypeScriptTranspiler() = default;

void xiiTypeScriptTranspiler::SetOutputFolder(const char* szFolder)
{
  m_sOutputFolder = szFolder;
}

void xiiTypeScriptTranspiler::StartLoadTranspiler()
{
  if (m_LoadTaskGroup.IsValid())
    return;

  xiiSharedPtr<xiiTask> pTask = XII_DEFAULT_NEW(xiiDelegateTask<void>, "",
                                                [this]() //
                                                {
                                                  XII_PROFILE_SCOPE("Load TypeScript Transpiler");

                                                  if (m_Transpiler.ExecuteFile("typescriptServices.js").Failed())
                                                  {
                                                    xiiLog::Error("typescriptServices.js could not be loaded");
                                                  }

                                                  xiiLog::Success("Loaded TypeScript transpiler.");
                                                });

  pTask->ConfigureTask("Load TypeScript Transpiler", xiiTaskNesting::Never);
  m_LoadTaskGroup = xiiTaskSystem::StartSingleTask(pTask, xiiTaskPriority::LongRunning);
}

void xiiTypeScriptTranspiler::FinishLoadTranspiler()
{
  StartLoadTranspiler();

  xiiTaskSystem::WaitForGroup(m_LoadTaskGroup);
}

xiiResult xiiTypeScriptTranspiler::TranspileString(const char* szString, xiiStringBuilder& out_sResult)
{
  XII_LOG_BLOCK("TranspileString");

  FinishLoadTranspiler();

  XII_PROFILE_SCOPE("Transpile TypeScript");

  xiiDuktapeHelper duk(m_Transpiler);

  m_Transpiler.PushGlobalObject();                 // [ global ]
  if (m_Transpiler.PushLocalObject("ts").Failed()) // [ global ts ]
  {
    xiiLog::Error("'ts' object does not exist");
    duk.PopStack(2); // [ ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_FAILURE, 0);
  }

  if (m_Transpiler.PrepareObjectFunctionCall("transpile").Failed()) // [ global ts transpile ]
  {
    xiiLog::Error("'ts.transpile' function does not exist");
    duk.PopStack(3); // [ ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_FAILURE, 0);
  }

  m_Transpiler.PushString(szString);                // [ global ts transpile source ]
  if (m_Transpiler.CallPreparedFunction().Failed()) // [ global ts result ]
  {
    xiiLog::Error("String could not be transpiled");
    duk.PopStack(3); // [ ]
    XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_FAILURE, 0);
  }

  out_sResult = m_Transpiler.GetStringValue(-1); // [ global ts result ]
  m_Transpiler.PopStack(3);                      // [ ]

  XII_DUK_RETURN_AND_VERIFY_STACK(duk, XII_SUCCESS, 0);
}

xiiResult xiiTypeScriptTranspiler::TranspileFile(const char* szFile, xiiUInt64 uiSkipIfFileHash, xiiStringBuilder& out_sResult, xiiUInt64& out_uiFileHash)
{
  XII_LOG_BLOCK("TranspileFile", szFile);

  FinishLoadTranspiler();

  xiiFileReader file;
  if (file.Open(szFile).Failed())
  {
    xiiLog::Error("File does not exist: '{}'", szFile);
    return XII_FAILURE;
  }

  xiiStringBuilder source;
  source.ReadAll(file);

  if (m_ModifyTsBeforeTranspilationCB.IsValid())
  {
    m_ModifyTsBeforeTranspilationCB(source);
  }

  out_uiFileHash = xiiHashingUtils::xxHash64(source.GetData(), source.GetElementCount());

  if (uiSkipIfFileHash == out_uiFileHash)
    return XII_SUCCESS;

  return TranspileString(source, out_sResult);
}

xiiResult xiiTypeScriptTranspiler::TranspileFileAndStoreJS(const char* szFile, xiiStringBuilder& out_sResult)
{
  XII_LOG_BLOCK("TranspileFileAndStoreJS", szFile);

  XII_ASSERT_DEV(!m_sOutputFolder.IsEmpty(), "Output folder has not been set");

  xiiUInt64 uiExpectedFileHash = 0;
  xiiUInt64 uiActualFileHash   = 0;

  xiiStringBuilder sOutFile = szFile;
  sOutFile.ChangeFileExtension("js");
  sOutFile.Prepend(m_sOutputFolder, "/");
  sOutFile.MakeCleanPath();

  {
    xiiFileReader fileIn;
    if (fileIn.Open(sOutFile).Succeeded())
    {
      out_sResult.ReadAll(fileIn);

      if (out_sResult.StartsWith_NoCase("/*SOURCE-HASH:"))
      {
        xiiStringView sHashView = out_sResult.GetView();
        sHashView.Shrink(14, 0);

        // try to extract the hash
        if (xiiConversionUtils::ConvertHexStringToUInt64(sHashView, uiExpectedFileHash).Failed())
        {
          uiExpectedFileHash = 0;
        }
      }
    }
  }

  XII_SUCCEED_OR_RETURN(TranspileFile(szFile, uiExpectedFileHash, out_sResult, uiActualFileHash));

  if (uiExpectedFileHash != uiActualFileHash)
  {
    xiiFileWriter fileOut;
    if (fileOut.Open(sOutFile).Failed())
    {
      xiiLog::Error("Could not write transpiled JS to file '{}'", sOutFile);
      return XII_FAILURE;
    }

    xiiStringBuilder sHashHeader;
    sHashHeader.Format("/*SOURCE-HASH:{}*/\n", xiiArgU(uiActualFileHash, 16, true, 16, true));
    out_sResult.Prepend(sHashHeader);

    XII_SUCCEED_OR_RETURN(fileOut.WriteBytes(out_sResult.GetData(), out_sResult.GetElementCount()));
    xiiLog::Success("Transpiled '{}'", szFile);
  }

  return XII_SUCCESS;
}

void xiiTypeScriptTranspiler::SetModifyTsBeforeTranspilationCallback(xiiDelegate<void(xiiStringBuilder&)> callback)
{
  m_ModifyTsBeforeTranspilationCB = callback;
}
