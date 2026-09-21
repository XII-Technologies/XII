/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/HashedString.h>

/// Describes which kind of token a xiiToken is.
struct XII_FOUNDATION_DLL xiiTokenType
{
  enum Enum
  {
    Unknown,           ///< for internal use
    Whitespace,        ///< The token is a space or tab
    Identifier,        ///< a series of alphanumerics or underscores
    NonIdentifier,     ///< Everything else
    Newline,           ///< Either '\n' or '\r\n'
    LineComment,       ///< A comment that starts with two slashes and ends at the next newline (or end of file)
    BlockComment,      ///< A comment that starts with a slash and a star, and ends at the next star/slash combination (or end of file)
    String1,           ///< A string enclosed in "
    String2,           ///< A string enclosed in '
    Integer,           ///< An integer number
    Float,             ///< A floating point number
    RawString1,        ///< A raw c++11 string enclosed in ". Contents do not contain the enclosing " or the start / end marker.
    RawString1Prefix,  ///< The prefix part of a C++11 string. E.g: R"foo(
    RawString1Postfix, ///< The postfix part of a C++11 string. E.g: )foo"
    EndOfFile,         ///< End-of-file marker
    ENUM_COUNT,
  };

  static const char* EnumNames[ENUM_COUNT];
};

/// Represents one piece of tokenized text in a document.
struct XII_FOUNDATION_DLL xiiToken
{
  xiiToken()
  {
    m_iType         = xiiTokenType::Unknown;
    m_uiLine        = 0;
    m_uiColumn      = 0;
    m_uiCustomFlags = 0;
  }

  /// Typically of type xiiTokenType, but users can put anything in there, that they like
  xiiInt32 m_iType;

  /// The line in which the token appeared
  xiiUInt32 m_uiLine;

  /// The column in the line, at which the token string started.
  xiiUInt32 m_uiColumn;

  /// The actual string data that represents the token. Note that this is a view to a substring of some larger text data.
  /// To get only the relevant piece as one zero-terminated string, assign m_DataView to a xiiStringBuilder and read that instead.
  xiiStringView m_DataView;

  /// For users to be able to store additional info for a token.
  xiiUInt32 m_uiCustomFlags;

  /// The file in which the token appeared.
  xiiHashedString m_File;
};

/// Takes text and splits it up into xiiToken objects. The result can be used for easier parsing.
///
/// The tokenizer is built to work on code that is similar to C. That means it will tokenize comments and
/// strings as they are defined in the C language. Also line breaks that end with a backslash are not
/// really considered as line breaks.\n
/// White space is defined as spaces and tabs.\n
/// Identifiers are names that consist of alphanumerics and underscores.\n
/// Non-Identifiers are everything else. However, they will currently never consist of more than a single character.
/// Ie. '++' will be tokenized as two consecutive non-Identifiers.\n
/// Parenthesis etc. will not be tokenized in any special way, they are all considered as non-Identifiers.
///
/// The token stream will always end with an end-of-file token.
class XII_FOUNDATION_DLL xiiTokenizer
{
public:
  /// Constructor.
  ///
  /// Takes an additional optional allocator. If no allocator is given the default allocator will be used.
  xiiTokenizer(xiiAllocator* pAllocator = nullptr);

  ~xiiTokenizer();

  /// \param data The string data to be tokenized.
  /// \param pLog A log interface that will receive any tokenization errors.
  /// \param bCopyData If set, 'data' will be copied into a member variable and tokenization is run on the copy, allowing for the original data storage to be deallocated after this call. If false, tokenization will reference 'data' directly and thus, 'data' must outlive this instance.
  void Tokenize(xiiArrayPtr<const xiiUInt8> data, xiiLogInterface* pLog, bool bCopyData = true);

  /// Gives read access to the token stream.
  const xiiDeque<xiiToken>& GetTokens() const { return m_Tokens; }

  /// Gives read and write access to the token stream.
  xiiDeque<xiiToken>& GetTokens() { return m_Tokens; }

  /// Returns an array with a copy of all tokens. Use this when using xiiTokenParseUtils.
  void GetAllTokens(xiiDynamicArray<const xiiToken*>& ref_tokens) const;

  /// Returns an array of all tokens. New line tokens are ignored.
  void GetAllLines(xiiDynamicArray<const xiiToken*>& ref_tokens) const;

  /// Returns an array of tokens that represent the next line in the file.
  ///
  /// Returns XII_SUCCESS when there was more data to return, XII_FAILURE if the end of the file was reached already.
  /// uiFirstToken is the index from where to start. It will be updated automatically. Consecutive calls to GetNextLine()
  /// with the same uiFirstToken variable will give one line after the other.
  ///
  /// \note This function takes care of handling the 'backslash/newline' combination, as defined in the C language.
  /// That means all such sequences will be ignored. Therefore the tokens that are returned as one line might not
  /// contain all tokens that are actually in the stream. Also the tokens might have different line numbers, when
  /// two or more lines from the file are merged into one logical line.
  xiiResult GetNextLine(xiiUInt32& ref_uiFirstToken, xiiHybridArray<const xiiToken*, 32>& ref_tokens) const;

  xiiResult GetNextLine(xiiUInt32& ref_uiFirstToken, xiiHybridArray<xiiToken*, 32>& ref_tokens);

  /// Returns the internal copy of the tokenized data. This will be empty if Tokenize was called with 'bCopyData' equals 'false'.
  const xiiArrayPtr<const xiiUInt8> GetTokenizedData() const { return m_Data; }

  /// Enables treating lines that start with # character as line comments
  ///
  /// Needs to be set before tokenization to take effect.
  void SetTreatHashSignAsLineComment(bool bHashSignIsLineComment) { m_bHashSignIsLineComment = bHashSignIsLineComment; }

private:
  void NextChar();
  void AddToken();

  void HandleUnknown();
  void HandleString(char terminator);
  void HandleRawString();
  void HandleNumber();
  void HandleLineComment();
  void HandleBlockComment();
  void HandleWhitespace();
  void HandleIdentifier();
  void HandleNonIdentifier();

  xiiLogInterface*   m_pLog    = nullptr;
  xiiTokenType::Enum m_CurMode = xiiTokenType::Unknown;
  xiiStringView      m_sIterator;
  xiiStringView      m_sRawStringMarker;
  xiiUInt32          m_uiCurLine   = 1;
  xiiUInt32          m_uiCurColumn = xiiInvalidIndex;
  xiiUInt32          m_uiCurChar   = '\0';
  xiiUInt32          m_uiNextChar  = '\0';

  xiiUInt32 m_uiLastLine   = 1;
  xiiUInt32 m_uiLastColumn = 1;

  const char* m_szCurCharStart  = nullptr;
  const char* m_szNextCharStart = nullptr;
  const char* m_szTokenStart    = nullptr;

  xiiDeque<xiiToken>        m_Tokens;
  xiiDynamicArray<xiiUInt8> m_Data;

  bool m_bHashSignIsLineComment = false;
};
