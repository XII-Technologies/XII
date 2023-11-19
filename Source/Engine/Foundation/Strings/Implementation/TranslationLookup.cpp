#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

bool                              xiiTranslator::s_bHighlightUntranslated = false;
xiiHybridArray<xiiTranslator*, 4> xiiTranslator::s_AllTranslators;

xiiTranslator::xiiTranslator()
{
  s_AllTranslators.PushBack(this);
}

xiiTranslator::~xiiTranslator()
{
  s_AllTranslators.RemoveAndSwap(this);
}

void xiiTranslator::Reset() {}

void xiiTranslator::Reload() {}

void xiiTranslator::ReloadAllTranslators()
{
  XII_LOG_BLOCK("ReloadAllTranslators");

  for (xiiTranslator* pTranslator : s_AllTranslators)
  {
    pTranslator->Reload();
  }
}

void xiiTranslator::HighlightUntranslated(bool bHighlight)
{
  if (s_bHighlightUntranslated == bHighlight)
    return;

  s_bHighlightUntranslated = bHighlight;

  ReloadAllTranslators();
}

//////////////////////////////////////////////////////////////////////////

xiiHybridArray<xiiUniquePtr<xiiTranslator>, 16> xiiTranslationLookup::s_Translators;

void xiiTranslationLookup::AddTranslator(xiiUniquePtr<xiiTranslator> pTranslator)
{
  s_Translators.PushBack(std::move(pTranslator));
}


xiiStringView xiiTranslationLookup::Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  for (xiiUInt32 i = s_Translators.GetCount(); i > 0; --i)
  {
    xiiStringView sResult = s_Translators[i - 1]->Translate(sString, uiStringHash, usage);

    if (!sResult.IsEmpty())
      return sResult;
  }

  return sString;
}


void xiiTranslationLookup::Clear()
{
  s_Translators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void xiiTranslatorFromFiles::AddTranslationFilesFromFolder(xiiStringView sFolder)
{
  XII_LOG_BLOCK("AddTranslationFilesFromFolder", sFolder);

  if (!m_Folders.Contains(sFolder))
  {
    m_Folders.PushBack(sFolder);
  }

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiStringBuilder startPath;
  if (xiiFileSystem::ResolvePath(sFolder, &startPath, nullptr).Failed())
    return;

  xiiStringBuilder fullpath;

  xiiFileSystemIterator it;
  it.StartSearch(startPath, xiiFileSystemIteratorFlags::ReportFilesRecursive);


  while (it.IsValid())
  {
    fullpath = it.GetCurrentPath();
    fullpath.AppendPath(it.GetStats().m_sName);

    LoadTranslationFile(fullpath);

    it.Next();
  }

#endif
}

xiiStringView xiiTranslatorFromFiles::Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  return xiiTranslatorStorage::Translate(sString, uiStringHash, usage);
}

void xiiTranslatorFromFiles::Reload()
{
  xiiTranslatorStorage::Reload();

  for (const auto& sFolder : m_Folders)
  {
    AddTranslationFilesFromFolder(sFolder);
  }
}

void xiiTranslatorFromFiles::LoadTranslationFile(xiiStringView sFullPath)
{
  XII_LOG_BLOCK("LoadTranslationFile", sFullPath);

  xiiLog::Dev("Loading Localization File '{0}'", sFullPath);

  xiiFileReader file;
  if (file.Open(sFullPath).Failed())
  {
    xiiLog::Warning("Failed to open localization file '{0}'", sFullPath);
    return;
  }

  xiiStringBuilder sContent;
  sContent.ReadAll(file);

  xiiDeque<xiiStringView> Lines;
  sContent.Split(false, Lines, "\n");

  xiiHybridArray<xiiStringView, 4> entries;

  xiiStringBuilder sLine, sKey, sValue, sTooltip, sHelpUrl;
  for (const auto& line : Lines)
  {
    sLine = line;
    sLine.Trim(" \t\r\n");

    if (sLine.IsEmpty() || sLine.StartsWith("#"))
      continue;

    entries.Clear();
    sLine.Split(true, entries, ";");

    if (entries.GetCount() <= 1)
    {
      xiiLog::Error("Invalid line in translation file: '{0}'", sLine);
      continue;
    }

    sKey   = entries[0];
    sValue = entries[1];

    sTooltip.Clear();
    sHelpUrl.Clear();

    if (entries.GetCount() >= 3)
      sTooltip = entries[2];
    if (entries.GetCount() >= 4)
      sHelpUrl = entries[3];

    sKey.Trim(" \t\r\n");
    sValue.Trim(" \t\r\n");
    sTooltip.Trim(" \t\r\n");
    sHelpUrl.Trim(" \t\r\n");

    if (GetHighlightUntranslated())
    {
      sValue.Prepend("# ");
      sValue.Append(" (@", sKey, ")");
    }

    StoreTranslation(sValue, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::Default);
    StoreTranslation(sTooltip, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::Tooltip);
    StoreTranslation(sHelpUrl, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::HelpURL);
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiTranslatorStorage::StoreTranslation(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  m_Translations[(xiiUInt32)usage][uiStringHash] = sString;
}

xiiStringView xiiTranslatorStorage::Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  auto it = m_Translations[(xiiUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return {};
}

void xiiTranslatorStorage::Reset()
{
  for (xiiUInt32 i = 0; i < (xiiUInt32)xiiTranslationUsage::ENUM_COUNT; ++i)
  {
    m_Translations[i].Clear();
  }
}

void xiiTranslatorStorage::Reload()
{
  Reset();
}

//////////////////////////////////////////////////////////////////////////

bool xiiTranslatorLogMissing::s_bActive = true;

xiiStringView xiiTranslatorLogMissing::Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  if (!xiiTranslatorLogMissing::s_bActive && !GetHighlightUntranslated())
    return {};

  if (usage != xiiTranslationUsage::Default)
    return {};

  xiiStringView sResult = xiiTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (sResult.IsEmpty())
  {
    xiiLog::Warning("Missing translation: {0};", sString);

    StoreTranslation(sString, uiStringHash, usage);
  }

  return {};
}

xiiStringView xiiTranslatorMakeMoreReadable::Translate(xiiStringView sString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  xiiStringView sResult = xiiTranslatorStorage::Translate(sString, uiStringHash, usage);

  if (!sResult.IsEmpty())
    return sResult;

  xiiStringBuilder result;
  xiiStringBuilder tmp = sString;
  tmp.Trim(" _-");

  tmp.TrimWordStart("xii");

  xiiStringView sComponent = "Component";
  if (tmp.EndsWith(sComponent) && tmp.GetElementCount() > sComponent.GetElementCount())
  {
    tmp.Shrink(0, sComponent.GetElementCount());
  }

  auto IsUpper = [](xiiUInt32 c) {
    return c == xiiStringUtils::ToUpperChar(c);
  };
  auto IsNumber = [](xiiUInt32 c) {
    return c >= '0' && c <= '9';
  };

  xiiUInt32 uiPrev = ' ';
  xiiUInt32 uiCur  = ' ';
  xiiUInt32 uiNext = ' ';

  bool bContinue = true;

  for (auto it = tmp.GetIteratorFront(); bContinue; ++it)
  {
    uiPrev = uiCur;
    uiCur  = uiNext;

    if (it.IsValid())
    {
      uiNext = it.GetCharacter();
    }
    else
    {
      uiNext    = ' ';
      bContinue = false;
    }

    if (uiCur == '_')
      uiCur = ' ';

    if (uiCur == ':')
    {
      result.Clear();
      continue;
    }

    if (uiPrev != '[' && uiCur != ']' && IsNumber(uiPrev) != IsNumber(uiCur))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (IsNumber(uiPrev) && IsNumber(uiCur))
    {
      result.Append(uiCur);
      continue;
    }

    if (IsUpper(uiPrev) && IsUpper(uiCur) && !IsUpper(uiNext))
    {
      result.Append(" ");
      result.Append(uiCur);
      continue;
    }

    if (!IsUpper(uiCur) && IsUpper(uiNext))
    {
      result.Append(uiCur);
      result.Append(" ");
      continue;
    }

    result.Append(uiCur);
  }

  result.Trim(" ");
  while (result.ReplaceAll("  ", " ") > 0)
  {
    // Remove double whitespaces.
  }

  if (GetHighlightUntranslated())
  {
    result.Append(" (@", sString, ")");
  }

  StoreTranslation(result, uiStringHash, usage);

  return xiiTranslatorStorage::Translate(sString, uiStringHash, usage);
}

XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_TranslationLookup);
