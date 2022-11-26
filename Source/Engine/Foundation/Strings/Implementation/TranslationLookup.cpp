#include <Foundation/FoundationPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Strings/TranslationLookup.h>

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

//////////////////////////////////////////////////////////////////////////

xiiHybridArray<xiiUniquePtr<xiiTranslator>, 16> xiiTranslationLookup::s_Translators;

void xiiTranslationLookup::AddTranslator(xiiUniquePtr<xiiTranslator> pTranslator)
{
  s_Translators.PushBack(std::move(pTranslator));
}


const char* xiiTranslationLookup::Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  for (xiiUInt32 i = s_Translators.GetCount(); i > 0; --i)
  {
    const char* szResult = s_Translators[i - 1]->Translate(szString, uiStringHash, usage);

    if (szResult != nullptr)
      return szResult;
  }

  return szString;
}


void xiiTranslationLookup::Clear()
{
  s_Translators.Clear();
}

//////////////////////////////////////////////////////////////////////////

void xiiTranslatorFromFiles::AddTranslationFilesFromFolder(const char* szFolder)
{
  XII_LOG_BLOCK("AddTranslationFilesFromFolder", szFolder);

  if (!m_Folders.Contains(szFolder))
  {
    m_Folders.PushBack(szFolder);
  }

#if XII_ENABLED(XII_SUPPORTS_FILE_ITERATORS)
  xiiStringBuilder startPath;
  if (xiiFileSystem::ResolvePath(szFolder, &startPath, nullptr).Failed())
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

void xiiTranslatorFromFiles::Reload()
{
  xiiTranslatorStorage::Reload();

  for (const auto& sFolder : m_Folders)
  {
    AddTranslationFilesFromFolder(sFolder);
  }
}

void xiiTranslatorFromFiles::LoadTranslationFile(const char* szFullPath)
{
  XII_LOG_BLOCK("LoadTranslationFile", szFullPath);

  xiiLog::Dev("Loading Localization File '{0}'", szFullPath);

  xiiFileReader file;
  if (file.Open(szFullPath).Failed())
  {
    xiiLog::Warning("Failed to open localization file '{0}'", szFullPath);
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

    if (sLine.IsEmpty())
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

    StoreTranslation(sValue, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::Default);
    StoreTranslation(sTooltip, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::Tooltip);
    StoreTranslation(sHelpUrl, xiiHashingUtils::StringHash(sKey), xiiTranslationUsage::HelpURL);
  }
}

//////////////////////////////////////////////////////////////////////////

void xiiTranslatorStorage::StoreTranslation(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  m_Translations[(xiiUInt32)usage][uiStringHash] = szString;
}

const char* xiiTranslatorStorage::Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  auto it = m_Translations[(xiiUInt32)usage].Find(uiStringHash);
  if (it.IsValid())
    return it.Value().GetData();

  return nullptr;
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

const char* xiiTranslatorLogMissing::Translate(const char* szString, xiiUInt64 uiStringHash, xiiTranslationUsage usage)
{
  const char* szResult = xiiTranslatorStorage::Translate(szString, uiStringHash, usage);

  if (szResult != nullptr)
    return szResult;

  if (usage != xiiTranslationUsage::Default)
    return "";

  if (xiiTranslatorLogMissing::s_bActive)
  {
    xiiLog::Warning("Missing Translation for '{0}'", szString);
  }

  StoreTranslation(szString, uiStringHash, usage);
  return szString;
}



XII_STATICLINK_FILE(Foundation, Foundation_Strings_Implementation_TranslationLookup);
