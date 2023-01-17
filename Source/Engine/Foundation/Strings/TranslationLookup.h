#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief What a translated string is used for.
enum class xiiTranslationUsage
{
  Default,
  Tooltip,
  HelpURL,

  ENUM_COUNT
};

/// \brief Base class to translate one string into another
class XII_FOUNDATION_DLL xiiTranslator
{
public:
  xiiTranslator();
  virtual ~xiiTranslator();

  /// \brief The given string (with the given hash) shall be translated
  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) = 0;

  /// \brief Called to reset internal state
  virtual void Reset();

  /// \brief May reload the known translations
  virtual void Reload();

  /// \brief Will call Reload() on all currently active translators
  static void ReloadAllTranslators();

  static void HighlightUntranslated(bool bHighlight);

  static bool GetHighlightUntranslated() { return s_bHighlightUntranslated; }

private:
  static bool                              s_bHighlightUntranslated;
  static xiiHybridArray<xiiTranslator*, 4> s_AllTranslators;
};

/// \brief Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class XII_FOUNDATION_DLL xiiTranslatorPassThrough : public xiiTranslator
{
public:
  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override { return szString; }
};

/// \brief Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is
/// not known
class XII_FOUNDATION_DLL xiiTranslatorStorage : public xiiTranslator
{
public:
  /// \brief Stores szString as the translation for the string with the given hash
  virtual void StoreTranslation(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage);

  /// \brief Returns the translated string for uiStringHash, or nullptr, if not available
  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;

  /// \brief Clears all stored translation strings
  virtual void Reset() override;

  /// \brief Simply executes Reset() on this translator
  virtual void Reload() override;

protected:
  xiiMap<xiiUInt64, xiiString> m_Translations[(int)xiiTranslationUsage::ENUM_COUNT];
};

/// \brief Outputs a 'Missing Translation' warning the first time a string translation is requested.
/// Otherwise always returns nullptr, allowing the next translator to take over.
class XII_FOUNDATION_DLL xiiTranslatorLogMissing : public xiiTranslatorStorage
{
public:
  /// Can be used from external code to (temporarily) deactivate error logging (a bit hacky)
  static bool s_bActive;

  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;
};

/// \brief Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them.
class XII_FOUNDATION_DLL xiiTranslatorFromFiles : public xiiTranslatorStorage
{
public:
  /// \brief Loads all files recursively from the specified folder as translation files.
  ///
  /// The given path must be absolute or resolvable to an absolute path.
  /// On failure, the function does nothing.
  /// This function depends on xiiFileSystemIterator to be available.
  void AddTranslationFilesFromFolder(const char* szFolder);

  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;

  virtual void Reload() override;

private:
  void LoadTranslationFile(const char* szFullPath);

  xiiHybridArray<xiiString, 4> m_Folders;
};

/// \brief Returns the same string that is passed into it, but strips off class names and separates the text at CamelCase boundaries.
class XII_FOUNDATION_DLL xiiTranslatorMakeMoreReadable : public xiiTranslatorStorage
{
public:
  virtual const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;
};

/// \brief Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class XII_FOUNDATION_DLL xiiTranslationLookup
{
public:
  /// \brief Translators will be queried in the reverse order that they were added.
  static void AddTranslator(xiiUniquePtr<xiiTranslator> pTranslator);

  /// \brief Prefer to use the xiiTranslate macro instead of calling this function directly. Will query all translators for a translation,
  /// until one is found.
  static const char* Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage);

  /// \brief Deletes all translators.
  static void Clear();

private:
  static xiiHybridArray<xiiUniquePtr<xiiTranslator>, 16> s_Translators;
};

/// \brief Use this macro to query a translation for a string from the xiiTranslationLookup system
#define xiiTranslate(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::Default)

/// \brief Use this macro to query a translation for a tooltip string from the xiiTranslationLookup system
#define xiiTranslateTooltip(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::Tooltip)

/// \brief Use this macro to query a translation for a help URL from the xiiTranslationLookup system
#define xiiTranslateHelpURL(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::HelpURL)
