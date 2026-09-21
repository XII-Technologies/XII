/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/CodeUtils/TokenParseUtils.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Timestamp.h>

/// This object caches files in a tokenized state. It can be shared among xiiPreprocessor instances to improve performance when
/// they access the same files.
class XII_FOUNDATION_DLL xiiTokenizedFileCache
{
public:
  struct FileData
  {
    xiiTokenizer m_Tokens;
    xiiTimestamp m_Timestamp;
  };

  /// Checks whether \a sFileName is already in the cache, returns an iterator to it. If the iterator is invalid, the file is not cached yet.
  xiiMap<xiiString, FileData>::ConstIterator Lookup(const xiiString& sFileName) const;

  /// Removes the cached content for \a sFileName from the cache. Should be used when the file content has changed and needs to be re-read.
  void Remove(const xiiString& sFileName);

  /// Removes all files from the cache to ensure that they will be re-read.
  void Clear();

  /// Stores \a FileContent for the file \a sFileName as the new cached data.
  ///
  //// The file content is tokenized first and all #line directives are evaluated, to update the line number and file origin for each token.
  /// Any errors are written to the given log.
  const xiiTokenizer* Tokenize(const xiiString& sFileName, xiiArrayPtr<const xiiUInt8> fileContent, const xiiTimestamp& fileTimeStamp, xiiLogInterface* pLog);

private:
  void SkipWhitespace(xiiDeque<xiiToken>& Tokens, xiiUInt32& uiCurToken);

  mutable xiiMutex            m_Mutex;
  xiiMap<xiiString, FileData> m_Cache;
};

/// xiiPreprocessor implements a standard C preprocessor. It can be used to pre-process files to get the output after macro expansion and #ifdef
/// handling.
///
/// For a detailed documentation about the C preprocessor, see https://gcc.gnu.org/onlinedocs/cpp/
///
/// This class implements all standard features:
///   * object and function macros
///   * Full evaluation of #if, #ifdef etc. including mathematical operations such as #if A > 42
///   * Parameter stringification
///   * Parameter concatenation
///   * __LINE__ and __FILE__ macros
///   * Fully correct #line evaluation for error output
///   * Correct handling of __VA_ARGS__
///   * #include handling
///   * #pragma once
///   * #warning and #error for custom failure messages
class XII_FOUNDATION_DLL xiiPreprocessor
{
public:
  /// Describes the type of #include that was encountered during preprocessing
  enum IncludeType
  {
    MainFile,        ///< This is used for the very first access to the main source file
    RelativeInclude, ///< An #include "file" has been encountered
    GlobalInclude    ///< An #include <file> has been encountered
  };

  /// This type of callback is used to read an #include file. \a szAbsoluteFile is the path that the FileLocatorCB reported, the result needs
  /// to be stored in \a FileContent.
  using FileOpenCB = xiiDelegate<xiiResult(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& FileContent, xiiTimestamp& out_FileModification)>;

  /// This type of callback is used to retrieve the absolute path of the \a szIncludeFile when #included inside \a szCurAbsoluteFile.
  ///
  /// Note that you should ensure that \a out_sAbsoluteFilePath is always identical (including casing and path slashes) when it is supposed to point
  /// to the same file, as this exact name is used for file lookup (and therefore also file caching).
  /// If it is not identical, file caching will not work, and on different OSes the file may be found or not.
  using FileLocatorCB = xiiDelegate<xiiResult(xiiStringView sCurAbsoluteFile, xiiStringView sIncludeFile, IncludeType IncType, xiiStringBuilder& out_sAbsoluteFilePath)>;

  /// Every time an unknown command (e.g. '#version') is encountered, this callback is used to determine whether the command shall be passed
  /// through.
  ///
  /// If the callback returns false, an error is generated and parsing fails. The callback thus acts as a whitelist for all commands that shall be
  /// passed through.
  using PassThroughUnknownCmdCB = xiiDelegate<bool(xiiStringView sUnknownCommand)>;

  using MacroParameters = xiiDeque<xiiTokenParseUtils::TokenStream>;

  /// The event data that the processor broadcasts
  ///
  /// Please note that m_pToken contains a lot of interesting information, such as
  /// the current file and line number and of course the current piece of text.
  struct ProcessingEvent
  {
    /// The event types that the processor broadcasts
    enum EventType
    {
      BeginExpansion,  ///< A macro is now going to be expanded
      EndExpansion,    ///< A macro is finished being expanded
      Error,           ///< An error was encountered
      Warning,         ///< A warning has been output.
      CheckDefined,    ///< A 'defined(X)' is being evaluated
      CheckIfdef,      ///< A '#ifdef X' is being evaluated
      CheckIfndef,     ///< A '#ifndef X' is being evaluated
      EvaluateUnknown, ///< Inside an #if an unknown identifier has been encountered, it will be evaluated as zero
      Define,          ///< A #define X has been stored
      Redefine,        ///< A #define for an already existing macro name (also logged as a warning)
    };

    EventType       m_Type   = Error;
    const xiiToken* m_pToken = nullptr;
    xiiStringView   m_sInfo;
  };

  /// Broadcasts events during the processing. This can be used to create detailed callstacks when an error is encountered.
  /// It also broadcasts errors and warnings with more detailed information than the log interface allows.
  xiiEvent<const ProcessingEvent&> m_ProcessingEvents;

  xiiPreprocessor();

  /// All error output is sent to the given xiiLogInterface.
  ///
  /// Note that when the preprocessor encounters any error, it will stop immediately and usually no output is generated.
  /// However, there are also a few cases where only a warning is generated, in this case preprocessing will continue without problems.
  ///
  /// Additionally errors and warnings are also broadcast through m_ProcessingEvents. So if you want to output more detailed information,
  /// that method should be preferred, because the events carry more information about the current file and line number etc.
  void SetLogInterface(xiiLogInterface* pLog);

  /// Allows to specify a custom cache object that should be used for storing the tokenized result of files.
  ///
  /// This allows to share one cache across multiple instances of xiiPreprocessor and across time. E.g. it makes it possible
  /// to prevent having to read and tokenize include files that are referenced often.
  void SetCustomFileCache(xiiTokenizedFileCache* pFileCache = nullptr);

  /// If set to true, all #pragma commands are passed through to the output, otherwise they are removed.
  void SetPassThroughPragma(bool bPassThrough) { m_bPassThroughPragma = bPassThrough; }

  /// If set to true, all #line commands are passed through to the output, otherwise they are removed.
  void SetPassThroughLine(bool bPassThrough) { m_bPassThroughLine = bPassThrough; }

  /// Sets the callback that is used to determine whether an unknown command is passed through or triggers an error.
  void SetPassThroughUnknownCmdsCB(PassThroughUnknownCmdCB callback) { m_PassThroughUnknownCmdCB = callback; }

  /// Sets the callback that is needed to read input data.
  ///
  /// The default file open function will just try to open files via xiiFileReader.
  void SetFileOpenFunction(FileOpenCB openAbsFileCB);

  /// Sets the callback that is needed to locate an input file
  ///
  /// The default file locator will assume that the main source file and all files #included in angle brackets can be opened without modification.
  /// Files #included in "" will be appended as relative paths to the path of the file they appeared in.
  void SetFileLocatorFunction(FileLocatorCB locateAbsFileCB);

  /// Adds a #define to the preprocessor, even before any file is processed.
  ///
  /// This allows to have global macros that are always defined for all processed files, such as the current platform etc.
  /// \a szDefinition must be in the form of the text that follows a #define statement. So to define the macro "WIN32", just
  /// pass that string. You can define any macro that could also be defined in the source files.
  ///
  /// If the definition is invalid, XII_FAILURE is returned. Also the preprocessor might end up in an invalid state, so using it any
  /// further might fail (including crashing).
  xiiResult AddCustomDefine(xiiStringView sDefinition);

  /// Processes the given file and returns the result as a stream of tokens.
  ///
  /// This function is useful when you want to further process the output afterwards and thus need it in a tokenized form anyway.
  xiiResult Process(xiiStringView sMainFile, xiiTokenParseUtils::TokenStream& ref_tokenOutput);

  /// Processes the given file and returns the result as a string.
  ///
  /// This function creates a string from the tokenized result. If \a bKeepComments is true, all block and line comments
  /// are included in the output string, otherwise they are removed.
  xiiResult Process(xiiStringView sMainFile, xiiStringBuilder& ref_sOutput, bool bKeepComments = true, bool bRemoveRedundantWhitespace = false, bool bInsertLine = false);


private:
  struct FileData
  {
    FileData()
    {
      m_iCurrentLine = 1;
      m_iExpandDepth = 0;
    }

    xiiHashedString m_sVirtualFileName;
    xiiHashedString m_sFileName;
    xiiInt32        m_iCurrentLine;
    xiiInt32        m_iExpandDepth;
  };

  enum IfDefActivity
  {
    IsActive,
    IsInactive,
    WasActive,
  };

  struct CustomDefine
  {
    xiiHybridArray<xiiUInt8, 64> m_Content;
    xiiTokenizer                 m_Tokenized;
  };

  // This class-local allocator is used to get rid of some of the memory allocation tracking that would otherwise occur for allocations made by the preprocessor.
  // If changing its position in the class, make sure it always comes before all other members that depend on it to ensure deallocations in those members happen before the allocator get destroyed.
  xiiAllocatorWithPolicy<xiiAllocationPolicyHeap, xiiAllocatorTrackingMode::DoNotTrack> m_ClassAllocator;

  bool                    m_bPassThroughPragma;
  bool                    m_bPassThroughLine;
  PassThroughUnknownCmdCB m_PassThroughUnknownCmdCB;

  // this file cache is used as long as the user does not provide his own
  xiiTokenizedFileCache m_InternalFileCache;

  // pointer to the file cache that is in use
  xiiTokenizedFileCache* m_pUsedFileCache;

  xiiDeque<FileData> m_CurrentFileStack;

  xiiLogInterface* m_pLog;

  xiiDeque<CustomDefine> m_CustomDefines;

  struct IfDefState
  {
    IfDefState(IfDefActivity activeState = IfDefActivity::IsActive) :
      m_ActiveState(activeState)
    {
    }

    IfDefActivity m_ActiveState;
    bool          m_bIsInElseClause = false;
  };

  xiiDeque<IfDefState> m_IfdefActiveStack;

  xiiResult ProcessFile(xiiStringView sFile, xiiTokenParseUtils::TokenStream& TokenOutput);
  xiiResult ProcessCmd(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& TokenOutput);

public:
  static xiiResult DefaultFileLocator(xiiStringView sCurAbsoluteFile, xiiStringView sIncludeFile, xiiPreprocessor::IncludeType incType, xiiStringBuilder& out_sAbsoluteFilePath);
  static xiiResult DefaultFileOpen(xiiStringView sAbsoluteFile, xiiDynamicArray<xiiUInt8>& ref_fileContent, xiiTimestamp& out_fileModification);

private: // *** File Handling ***
  xiiResult OpenFile(xiiStringView sFile, const xiiTokenizer** pTokenizer);

  FileOpenCB                  m_FileOpenCallback;
  FileLocatorCB               m_FileLocatorCallback;
  xiiSet<xiiTempHashedString> m_PragmaOnce;

private: // *** Macro Definition ***
  bool      RemoveDefine(xiiStringView sName);
  xiiResult HandleDefine(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken);

  struct MacroDefinition
  {
    const xiiToken*                 m_MacroIdentifier     = nullptr;
    bool                            m_bIsFunction         = false;
    bool                            m_bCurrentlyExpanding = false;
    bool                            m_bHasVarArgs         = false;
    xiiUInt32                       m_uiNumParameters     = xiiInvalidIndex;
    xiiTokenParseUtils::TokenStream m_Replacement;
  };

  xiiResult StoreDefine(const xiiToken* pMacroNameToken, const xiiTokenParseUtils::TokenStream* pReplacementTokens, xiiUInt32 uiFirstReplacementToken, xiiInt32 iNumParameters, bool bUsesVarArgs);
  xiiResult ExtractParameterName(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiString& sIdentifierName);

  xiiMap<xiiString256, MacroDefinition> m_Macros;

  static constexpr xiiInt32 s_iMacroParameter0 = xiiTokenType::ENUM_COUNT + 2;
  static xiiString          s_ParamNames[32];
  xiiToken                  m_ParameterTokens[32];

private: // *** #if condition parsing ***
  xiiResult EvaluateCondition(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseCondition(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseFactor(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionMul(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionOr(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionAnd(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionPlus(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionShift(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionBitOr(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionBitAnd(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);
  xiiResult ParseExpressionBitXor(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiInt64& iResult);


private: // *** Parsing ***
  xiiResult CopyTokensAndEvaluateDefined(const xiiTokenParseUtils::TokenStream& Source, xiiUInt32 uiFirstSourceToken, xiiTokenParseUtils::TokenStream& Destination);
  void      CopyTokensReplaceParams(const xiiTokenParseUtils::TokenStream& Source, xiiUInt32 uiFirstSourceToken, xiiTokenParseUtils::TokenStream& Destination, const xiiArrayPtr<xiiString>& parameters);

  xiiResult Expect(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiStringView sToken, xiiUInt32* pAccepted = nullptr);
  xiiResult Expect(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiTokenType::Enum Type, xiiUInt32* pAccepted = nullptr);
  xiiResult Expect(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiStringView sToken1, xiiStringView sToken2, xiiUInt32* pAccepted = nullptr);
  xiiResult ExpectEndOfLine(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken);

private: // *** Macro Expansion ***
  xiiResult Expand(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& Output);
  xiiResult ExpandOnce(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& Output);
  xiiResult ExpandObjectMacro(MacroDefinition& Macro, xiiTokenParseUtils::TokenStream& Output, const xiiToken* pMacroToken);
  xiiResult ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, xiiTokenParseUtils::TokenStream& Output, const xiiToken* pMacroToken);
  xiiResult ExpandMacroParam(const xiiToken& MacroToken, xiiUInt32 uiParam, xiiTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void      PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, xiiTokenParseUtils::TokenStream& Output);
  xiiToken* AddCustomToken(const xiiToken* pPrevious, const xiiStringView& sNewText);
  void      OutputNotExpandableMacro(MacroDefinition& Macro, xiiTokenParseUtils::TokenStream& Output);
  xiiResult ExtractAllMacroParameters(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiDeque<xiiTokenParseUtils::TokenStream>& AllParameters);
  xiiResult ExtractParameterValue(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32& uiCurToken, xiiTokenParseUtils::TokenStream& ParamTokens);

  xiiResult InsertParameters(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  xiiResult InsertStringifiedParameters(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  xiiResult ConcatenateParameters(const xiiTokenParseUtils::TokenStream& Tokens, xiiTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);
  void      MergeTokens(const xiiToken* pFirst, const xiiToken* pSecond, xiiTokenParseUtils::TokenStream& Output, const MacroDefinition& Macro);

  struct CustomToken
  {
    xiiToken  m_Token;
    xiiString m_sIdentifierString;
  };

  enum TokenFlags : xiiUInt32
  {
    NoFurtherExpansion = XII_BIT(0),
  };

  xiiToken        m_TokenFile;
  xiiToken        m_TokenLine;
  const xiiToken* m_pTokenOpenParenthesis;
  const xiiToken* m_pTokenClosedParenthesis;
  const xiiToken* m_pTokenComma;

  xiiDeque<const MacroParameters*> m_MacroParamStack;
  xiiDeque<const MacroParameters*> m_MacroParamStackExpanded;
  xiiDeque<CustomToken>            m_CustomTokens;

private: // *** Other ***
  static void StringifyTokens(const xiiTokenParseUtils::TokenStream& Tokens, xiiStringBuilder& sResult, bool bSurroundWithQuotes);
  xiiToken*   CreateStringifiedParameter(xiiUInt32 uiParam, const xiiToken* pParamToken, const MacroDefinition& Macro);

  xiiResult HandleErrorDirective(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleWarningDirective(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleUndef(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);

  xiiResult HandleEndif(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleElif(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleIf(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleElse(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken);
  xiiResult HandleIfdef(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken, bool bIsIfdef);
  xiiResult HandleInclude(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken, xiiTokenParseUtils::TokenStream& TokenOutput);
  xiiResult HandleLine(const xiiTokenParseUtils::TokenStream& Tokens, xiiUInt32 uiCurToken, xiiUInt32 uiDirectiveToken, xiiTokenParseUtils::TokenStream& TokenOutput);
};

#define PP_LOG0(Type, FormatStr, ErrorToken)                                                                                                         \
  {                                                                                                                                                  \
    ProcessingEvent pe;                                                                                                                              \
    pe.m_Type   = ProcessingEvent::Type;                                                                                                             \
    pe.m_pToken = ErrorToken;                                                                                                                        \
    pe.m_sInfo  = FormatStr;                                                                                                                         \
    if (pe.m_pToken->m_uiLine == 0 && pe.m_pToken->m_uiColumn == 0)                                                                                  \
    {                                                                                                                                                \
      const_cast<xiiToken*>(pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                   \
      const_cast<xiiToken*>(pe.m_pToken)->m_File   = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                               \
    }                                                                                                                                                \
    m_ProcessingEvents.Broadcast(pe);                                                                                                                \
    xiiLog::Type(m_pLog, "File '{0}', Line {1} ({2}): " FormatStr, pe.m_pToken->m_File.GetString(), pe.m_pToken->m_uiLine, pe.m_pToken->m_uiColumn); \
  }

#define PP_LOG(Type, FormatStr, ErrorToken, ...)                                                                                                        \
  {                                                                                                                                                     \
    ProcessingEvent _pe;                                                                                                                                \
    _pe.m_Type   = ProcessingEvent::Type;                                                                                                               \
    _pe.m_pToken = ErrorToken;                                                                                                                          \
    if (_pe.m_pToken->m_uiLine == 0 && _pe.m_pToken->m_uiColumn == 0)                                                                                   \
    {                                                                                                                                                   \
      const_cast<xiiToken*>(_pe.m_pToken)->m_uiLine = m_CurrentFileStack.PeekBack().m_iCurrentLine;                                                     \
      const_cast<xiiToken*>(_pe.m_pToken)->m_File   = m_CurrentFileStack.PeekBack().m_sVirtualFileName;                                                 \
    }                                                                                                                                                   \
    xiiStringBuilder sInfo;                                                                                                                             \
    sInfo.SetFormat(FormatStr, ##__VA_ARGS__);                                                                                                          \
    _pe.m_sInfo = sInfo.GetData();                                                                                                                      \
    m_ProcessingEvents.Broadcast(_pe);                                                                                                                  \
    xiiLog::Type(m_pLog, "File '{0}', Line {1} ({2}): {3}", _pe.m_pToken->m_File.GetString(), _pe.m_pToken->m_uiLine, _pe.m_pToken->m_uiColumn, sInfo); \
  }
