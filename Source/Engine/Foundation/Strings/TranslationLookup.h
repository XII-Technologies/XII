/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>

/// What a translated string is used for.
enum class xiiTranslationUsage
{
  Default,
  Tooltip,
  HelpURL,

  ENUM_COUNT
};

/// Base class to translate one string into another
class XII_FOUNDATION_DLL xiiTranslator
{
public:
  xiiTranslator();
  virtual ~xiiTranslator();

  /// The given string (with the given hash) shall be translated
  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) = 0;

  /// Called to reset internal state
  virtual void Reset();

  /// May reload the known translations
  virtual void Reload();

  /// Will call Reload() on all currently active translators
  static void ReloadAllTranslators();

  static void HighlightUntranslated(bool bHighlight);

  static bool GetHighlightUntranslated() { return s_bHighlightUntranslated; }

private:
  static bool                              s_bHighlightUntranslated;
  static xiiHybridArray<xiiTranslator*, 4> s_AllTranslators;
};

/// Just returns the same string that is passed into it. Can be used to display the actually untranslated strings
class XII_FOUNDATION_DLL xiiTranslatorPassThrough : public xiiTranslator
{
public:
  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override
  {
    XII_IGNORE_UNUSED(uiStringHash);
    XII_IGNORE_UNUSED(usage);
    return sString;
  }
};

/// Can store translated strings and all translation requests will come from that storage. Returns nullptr if the requested string is
/// not known
class XII_FOUNDATION_DLL xiiTranslatorStorage : public xiiTranslator
{
public:
  /// Stores sString as the translation for the string with the given hash
  virtual void StoreTranslation(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage);

  /// Returns the translated string for uiStringHash, or nullptr, if not available
  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;

  /// Clears all stored translation strings
  virtual void Reset() override;

  /// Simply executes Reset() on this translator
  virtual void Reload() override;

protected:
  xiiMap<xiiUInt64, xiiString> m_Translations[(xiiInt32)xiiTranslationUsage::ENUM_COUNT];
};

/// Outputs a 'Missing Translation' warning the first time a string translation is requested.
/// Otherwise always returns nullptr, allowing the next translator to take over.
class XII_FOUNDATION_DLL xiiTranslatorLogMissing : public xiiTranslatorStorage
{
public:
  /// Can be used from external code to (temporarily) deactivate error logging (a bit hacky)
  static bool s_bActive;

  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;
};

/// Loads translations from files. Each translator can have different search paths, but the files to be loaded are the same for all of them.
class XII_FOUNDATION_DLL xiiTranslatorFromFiles : public xiiTranslatorStorage
{
public:
  /// Loads all files recursively from the specified folder as translation files.
  ///
  /// The given path must be absolute or resolvable to an absolute path.
  /// On failure, the function does nothing.
  /// This function depends on xiiFileSystemIterator to be available.
  void AddTranslationFilesFromFolder(xiiStringView sFolder);

  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;

  virtual void Reload() override;

private:
  void LoadTranslationFile(xiiStringView sFullPath);

  xiiHybridArray<xiiString, 4> m_Folders;
};

/// Returns the same string that is passed into it, but strips off class names and separates the text at CamelCase boundaries.
class XII_FOUNDATION_DLL xiiTranslatorMakeMoreReadable : public xiiTranslatorStorage
{
public:
  virtual xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage) override;
};

/// Handles looking up translations for strings.
///
/// Multiple translators can be registered to get translations from different sources.
class XII_FOUNDATION_DLL xiiTranslationLookup
{
public:
  /// Translators will be queried in the reverse order that they were added.
  static void AddTranslator(xiiUniquePtr<xiiTranslator> pTranslator);

  /// Prefer to use the xiiTranslate macro instead of calling this function directly. Will query all translators for a translation,
  /// until one is found.
  static xiiStringView Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage);

  /// Deletes all translators.
  static void Clear();

private:
  static xiiHybridArray<xiiUniquePtr<xiiTranslator>, 16> s_Translators;
};

/// Use this macro to query a translation for a string from the xiiTranslationLookup system.
#define xiiTranslate(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::Default)

/// Use this macro to query a translation for a tooltip string from the xiiTranslationLookup system.
#define xiiTranslateTooltip(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::Tooltip)

/// Use this macro to query a translation for a help URL from the xiiTranslationLookup system.
#define xiiTranslateHelpURL(string) xiiTranslationLookup::Translate(string, xiiHashingUtils::StringHash(string), xiiTranslationUsage::HelpURL)
