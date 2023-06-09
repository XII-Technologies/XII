#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/UniquePtr.h>


namespace
{
  XII_ALWAYS_INLINE void SkipWhitespace(xiiToken& ref_token, xiiUInt32& i, const xiiDeque<xiiToken>& tokens)
  {
    while (ref_token.m_iType == xiiTokenType::Whitespace)
    {
      ref_token = tokens[++i];
    }
  }

  XII_ALWAYS_INLINE void SkipLine(xiiToken& ref_token, xiiUInt32& i, const xiiDeque<xiiToken>& tokens)
  {
    while (ref_token.m_iType != xiiTokenType::Newline && ref_token.m_iType != xiiTokenType::EndOfFile)
    {
      ref_token = tokens[++i];
    }
  }
} // namespace

class xiiHeaderCheckApp : public xiiApplication
{
private:
  xiiString                                                     m_sSearchDir;
  xiiString                                                     m_sProjectName;
  bool                                                          m_bHadErrors;
  bool                                                          m_bHadSeriousWarnings;
  bool                                                          m_bHadWarnings;
  xiiUniquePtr<xiiStackAllocator<xiiMemoryTrackingFlags::None>> m_pStackAllocator;
  xiiDynamicArray<xiiString>                                    m_IncludeDirectories;

  struct IgnoreInfo
  {
    xiiHashSet<xiiString> m_byName;
  };

  IgnoreInfo m_IgnoreTarget;
  IgnoreInfo m_IgnoreSource;

public:
  using SUPER = xiiApplication;

  xiiHeaderCheckApp() :
    xiiApplication("HeaderCheck")
  {
    m_bHadErrors          = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings        = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const xiiLoggingEventData& eventData)
  {
    xiiHeaderCheckApp* app = (xiiHeaderCheckApp*)xiiApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case xiiLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case xiiLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case xiiLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  xiiResult ParseArray(const xiiVariant& value, xiiHashSet<xiiString>& ref_dst)
  {
    if (!value.CanConvertTo<xiiVariantArray>())
    {
      xiiLog::Error("Expected array");
      return XII_FAILURE;
    }
    auto       a         = value.Get<xiiVariantArray>();
    const auto arraySize = a.GetCount();
    for (xiiUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<xiiString>())
      {
        xiiLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return XII_FAILURE;
      }
      xiiStringBuilder file = el.Get<xiiString>();
      file.ToLower();
      ref_dst.Insert(file);
    }
    return XII_SUCCESS;
  }

  xiiResult ParseIgnoreFile(const xiiStringView sIgnoreFilePath)
  {
    xiiJSONReader jsonReader;
    jsonReader.SetLogInterface(xiiLog::GetThreadLocalLogSystem());

    xiiFileReader reader;
    if (reader.Open(sIgnoreFilePath).Failed())
    {
      xiiLog::Error("Failed to open ignore file {0}", sIgnoreFilePath);
      return XII_FAILURE;
    }

    if (jsonReader.Parse(reader).Failed())
      return XII_FAILURE;

    const xiiStringView includeTarget = "includeTarget";
    const xiiStringView includeSource = "includeSource";
    const xiiStringView byName        = "byName";

    auto topLevel = jsonReader.GetTopLevelObject();
    for (auto it = topLevel.GetIterator(); it.IsValid(); it.Next())
    {
      if (it.Key() == includeTarget || it.Key() == includeSource)
      {
        IgnoreInfo& info  = (it.Key() == includeTarget) ? m_IgnoreTarget : m_IgnoreSource;
        auto        inner = it.Value().Get<xiiVariantDictionary>();
        for (auto it2 = inner.GetIterator(); it2.IsValid(); it2.Next())
        {
          if (it2.Key() == byName)
          {
            if (ParseArray(it2.Value(), info.m_byName).Failed())
            {
              xiiLog::Error("Failed to parse value of '{0}.{1}'.", it.Key(), it2.Key());
              return XII_FAILURE;
            }
          }
          else
          {
            xiiLog::Error("Unknown field of '{0}.{1}'", it.Key(), it2.Key());
            return XII_FAILURE;
          }
        }
      }
      else
      {
        xiiLog::Error("Unknown json member in root object '{0}'", it.Key().GetView());
        return XII_FAILURE;
      }
    }
    return XII_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    xiiGlobalLog::AddLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
    xiiGlobalLog::AddLogWriter(LogInspector);

    m_pStackAllocator = XII_DEFAULT_NEW(xiiStackAllocator<xiiMemoryTrackingFlags::None>, "Temp Allocator", xiiFoundation::GetAlignedAllocator());

    if (GetArgumentCount() < 2)
      xiiLog::Error("This tool requires at leas one command-line argument: An absolute path to the top-level folder of a library.");

    // Add the empty data directory to access files via absolute paths
    xiiFileSystem::AddDataDirectory("", "App", ":", xiiFileSystem::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    xiiStringBuilder sSearchDir;

    auto numArgs         = GetArgumentCount();
    auto shortInclude    = xiiStringView("-i");
    auto longInclude     = xiiStringView("--includeDir");
    auto shortIgnoreFile = xiiStringView("-f");
    auto longIgnoreFile  = xiiStringView("--ignoreFile");
    for (xiiUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = xiiStringView(GetArgument(argi));
      if (arg == shortInclude || arg == longInclude)
      {
        if (numArgs <= argi + 1)
        {
          xiiLog::Error("Missing path for {0}", arg);
          return;
        }
        xiiStringBuilder includeDir = GetArgument(argi + 1);
        if (includeDir == shortInclude || includeDir == longInclude || includeDir == shortIgnoreFile || includeDir == longIgnoreFile)
        {
          xiiLog::Error("Missing path for {0} found {1} instead", arg, includeDir.GetView());
          return;
        }
        argi++;
        includeDir.MakeCleanPath();
        m_IncludeDirectories.PushBack(includeDir);
      }
      else if (arg == shortIgnoreFile || arg == longIgnoreFile)
      {
        if (numArgs <= argi + 1)
        {
          xiiLog::Error("Missing path for {0}", arg);
          return;
        }
        xiiStringBuilder ignoreFile = GetArgument(argi + 1);
        if (ignoreFile == shortInclude || ignoreFile == longInclude || ignoreFile == shortIgnoreFile || ignoreFile == longIgnoreFile)
        {
          xiiLog::Error("Missing path for {0} found {1} instead", arg, ignoreFile.GetView());
          return;
        }
        argi++;
        ignoreFile.MakeCleanPath();
        if (ParseIgnoreFile(ignoreFile.GetView()).Failed())
          return;
      }
      else
      {
        if (sSearchDir.IsEmpty())
        {
          sSearchDir = arg;
          sSearchDir.MakeCleanPath();
        }
        else
        {
          xiiLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir?");
        }
      }
    }

    if (!xiiPathUtils::IsAbsolutePath(sSearchDir.GetData()))
      xiiLog::Error("The given path is not absolute: '{0}'", sSearchDir);

    m_sSearchDir = sSearchDir;

    auto projectStart = m_sSearchDir.GetView().FindLastSubString("/");
    if (projectStart == nullptr)
    {
      xiiLog::Error("Failed to parse project name from search path {0}", sSearchDir);
      return;
    }
    xiiStringBuilder projectName = xiiStringView(projectStart + 1, m_sSearchDir.GetView().GetEndPointer());
    projectName.ToUpper();
    m_sProjectName = projectName;

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      xiiLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bHadErrors || m_bHadSeriousWarnings)
      SetReturnCode(2);
    else if (m_bHadWarnings)
      SetReturnCode(1);
    else
      SetReturnCode(0);

    m_pStackAllocator = nullptr;

    xiiGlobalLog::RemoveLogWriter(LogInspector);
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::Console::LogMessageHandler);
    xiiGlobalLog::RemoveLogWriter(xiiLogWriter::VisualStudio::LogMessageHandler);
  }

  xiiResult ReadEntireFile(const char* szFile, xiiStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    xiiFileReader File;
    if (File.Open(szFile) == XII_FAILURE)
    {
      xiiLog::Error("Could not open for reading: '{0}'", szFile);
      return XII_FAILURE;
    }

    xiiDynamicArray<xiiUInt8> FileContent;

    xiiUInt8  Temp[4024];
    xiiUInt64 uiRead = File.ReadBytes(Temp, XII_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(xiiArrayPtr<xiiUInt8>(Temp, (xiiUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, XII_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!xiiUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      xiiLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                    "an editor that does not save the file in Utf8 encoding.",
                    szFile);
      return XII_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    return XII_SUCCESS;
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    const xiiUInt32 uiSearchDirLength = m_sSearchDir.GetElementCount() + 1;

    // get a directory iterator for the search directory
    xiiFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), xiiFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      xiiStringBuilder currentFile, sExt;

      // while there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // build the absolute path to the current file
        currentFile = it.GetCurrentPath();
        currentFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = currentFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          xiiLog::Info("Checking: {}", currentFile);

          XII_LOG_BLOCK("Header", &currentFile.GetData()[uiSearchDirLength]);
          CheckHeaderFile(currentFile);
          m_pStackAllocator->Reset();
        }
      }
    }
    else
      xiiLog::Error("Could not search the directory '{0}'", m_sSearchDir);
  }

  void CheckInclude(const xiiStringBuilder& sCurrentFile, const xiiStringBuilder& sIncludePath, xiiUInt32 uiLine)
  {
    xiiStringBuilder absIncludePath(m_pStackAllocator.Borrow());
    bool             includeOutside = true;
    if (sIncludePath.IsAbsolutePath())
    {
      for (auto& includeDir : m_IncludeDirectories)
      {
        if (sIncludePath.StartsWith(includeDir))
        {
          includeOutside = false;
          break;
        }
      }
    }
    else
    {
      bool includeFound = false;
      if (sIncludePath.StartsWith("ThirdParty"))
      {
        includeOutside = true;
      }
      else
      {
        for (auto& includeDir : m_IncludeDirectories)
        {
          absIncludePath = includeDir;
          absIncludePath.AppendPath(sIncludePath);
          if (xiiOSFile::ExistsFile(absIncludePath))
          {
            includeOutside = false;
            break;
          }
        }
      }
    }

    if (includeOutside)
    {
      xiiStringBuilder includeFileLower = sIncludePath.GetFileNameAndExtension();
      includeFileLower.ToLower();
      xiiStringBuilder currentFileLower = sCurrentFile.GetFileNameAndExtension();
      currentFileLower.ToLower();

      bool ignore = m_IgnoreTarget.m_byName.Contains(includeFileLower) || m_IgnoreSource.m_byName.Contains(currentFileLower);

      if (!ignore)
      {
        xiiLog::Error("Including '{0}' in {1}:{2} leaks underlying implementation details. Including system or thirdparty headers in public XII header "
                      "files is not allowed. Please use an interface, factory or pimpl pattern to hide the implementation and avoid the include. See "
                      "the Documentation Chapter 'General->Header Files' for details.",
                      sIncludePath.GetView(), sCurrentFile.GetView(), uiLine);
      }
    }
  }

  void CheckHeaderFile(const xiiStringBuilder& sCurrentFile)
  {
    xiiStringBuilder fileContents(m_pStackAllocator.Borrow());
    ReadEntireFile(sCurrentFile.GetData(), fileContents).IgnoreResult();

    auto fileDir = sCurrentFile.GetFileDirectory();

    xiiStringBuilder internalMacroToken(m_pStackAllocator.Borrow());
    internalMacroToken.Append("XII_", m_sProjectName, "_INTERNAL_HEADER");
    auto internalMacroTokenView = internalMacroToken.GetView();

    xiiTokenizer tokenizer(m_pStackAllocator.Borrow());
    auto         dataView = fileContents.GetView();
    tokenizer.Tokenize(xiiArrayPtr<const xiiUInt8>(reinterpret_cast<const xiiUInt8*>(dataView.GetStartPointer()), dataView.GetElementCount()), xiiLog::GetThreadLocalLogSystem());

    xiiStringView hash("#");
    xiiStringView include("include");
    xiiStringView openAngleBracket("<");
    xiiStringView closeAngleBracket(">");

    bool       isInternalHeader = false;
    auto       tokens           = tokenizer.GetTokens();
    const auto numTokens        = tokens.GetCount();
    for (xiiUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == xiiTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == xiiTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == xiiTokenType::Whitespace);

        if (curToken.m_iType == xiiTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == xiiTokenType::Whitespace);

          if (curToken.m_iType == xiiTokenType::String1)
          {
            // #include "bla"
            xiiStringBuilder absIncludePath(m_pStackAllocator.Borrow());
            xiiStringBuilder relativePath(m_pStackAllocator.Borrow());
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!xiiOSFile::ExistsFile(absIncludePath))
            {
              xiiLog::Error("The file '{0}' does not exist. Includes relative to the global include directories should use the #include "
                            "<path/to/file.h> syntax.",
                            absIncludePath);
            }
            else if (!isInternalHeader)
            {
              CheckInclude(sCurrentFile, absIncludePath, includeToken.m_uiLine);
            }
          }
          else if (curToken.m_iType == xiiTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error      = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == xiiTokenType::Newline)
              {
                xiiLog::Error("Non-terminated '<' in #include {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != xiiTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != xiiTokenType::Newline);
            }
            else if (!isInternalHeader)
            {
              xiiStringBuilder includePath(m_pStackAllocator.Borrow());
              includePath = xiiStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              CheckInclude(sCurrentFile, includePath, startToken.m_uiLine);
            }
          }
          else
          {
            // error
            xiiLog::Error("Can not parse #include statement in {0} line {1}", sCurrentFile.GetView(), includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != xiiTokenType::Newline && curToken.m_iType != xiiTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        if (curToken.m_iType == xiiTokenType::Identifier && curToken.m_DataView == internalMacroTokenView)
        {
          isInternalHeader = true;
        }
        else
        {
          while (curToken.m_iType != xiiTokenType::Newline && curToken.m_iType != xiiTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
    }
  }

  virtual xiiApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return xiiApplication::Execution::Quit;

    IterateOverFiles();

    return xiiApplication::Execution::Quit;
  }
};

XII_CONSOLEAPP_ENTRY_POINT(xiiHeaderCheckApp);
