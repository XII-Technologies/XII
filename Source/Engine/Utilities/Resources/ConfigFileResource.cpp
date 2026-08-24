/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Utilities/UtilitiesPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Utilities/Resources/ConfigFileResource.h>

static xiiConfigFileResourceLoader s_ConfigFileResourceLoader;

// clang-format off
XII_BEGIN_SUBSYSTEM_DECLARATION(Utilties, ConfigFileResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    xiiResourceManager::SetResourceTypeLoader<xiiConfigFileResource>(&s_ConfigFileResourceLoader);

    auto hFallback = xiiResourceManager::LoadResource<xiiConfigFileResource>("Empty.xiiConfig");
    xiiResourceManager::SetResourceTypeMissingFallback<xiiConfigFileResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    xiiResourceManager::SetResourceTypeMissingFallback<xiiConfigFileResource>(xiiConfigFileResourceHandle());
    xiiResourceManager::SetResourceTypeLoader<xiiConfigFileResource>(nullptr);
    xiiConfigFileResource::CleanupDynamicPluginReferences();
  }

  XII_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiConfigFileResource, 1, xiiRTTIDefaultAllocator<xiiConfigFileResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiConfigFileResource);

xiiConfigFileResource::xiiConfigFileResource() :
  xiiResource(xiiResource::DoUpdate::OnAnyThread, 0)
{
}

xiiConfigFileResource::~xiiConfigFileResource() = default;

xiiInt32 xiiConfigFileResource::GetInt(xiiTempHashedString sName, xiiInt32 iFallback) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return iFallback;
}

xiiInt32 xiiConfigFileResource::GetInt(xiiTempHashedString sName) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  xiiLog::Error("{}: 'int' config variable (name hash = {}) doesn't exist.", this->GetResourceDescription(), sName.GetHash());
  return 0;
}

float xiiConfigFileResource::GetFloat(xiiTempHashedString sName, float fFallback) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

double xiiConfigFileResource::GetDouble(xiiTempHashedString sName, double fFallback) const
{
  auto it = m_DoubleData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

float xiiConfigFileResource::GetFloat(xiiTempHashedString sName) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  xiiLog::Error("{}: 'float' config variable (name hash = {}) doesn't exist.", this->GetResourceDescription(), sName.GetHash());
  return 0;
}

double xiiConfigFileResource::GetDouble(xiiTempHashedString sName) const
{
  auto it = m_DoubleData.Find(sName);
  if (it.IsValid())
    return it.Value();

  xiiLog::Error("{}: 'double' config variable (name hash = {}) doesn't exist.", this->GetResourceDescription(), sName.GetHash());
  return 0;
}

bool xiiConfigFileResource::GetBool(xiiTempHashedString sName, bool bFallback) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return bFallback;
}

bool xiiConfigFileResource::GetBool(xiiTempHashedString sName) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  xiiLog::Error("{}: 'float' config variable (name hash = {}) doesn't exist.", this->GetResourceDescription(), sName.GetHash());
  return false;
}

const char* xiiConfigFileResource::GetString(xiiTempHashedString sName, const char* szFallback) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return szFallback;
}

const char* xiiConfigFileResource::GetString(xiiTempHashedString sName) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  xiiLog::Error("{}: 'string' config variable '(name hash = {}) doesn't exist.", this->GetResourceDescription(), sName.GetHash());
  return "";
}

xiiResourceLoadDescription xiiConfigFileResource::UnloadData(Unload WhatToUnload)
{
  m_IntData.Clear();
  m_FloatData.Clear();
  m_DoubleData.Clear();
  m_StringData.Clear();
  m_BoolData.Clear();

  xiiResourceLoadDescription d;
  d.m_State                      = xiiResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable    = 0;
  return d;
}

xiiResourceLoadDescription xiiConfigFileResource::UpdateContent(xiiStreamReader* Stream)
{
  xiiResourceLoadDescription d;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable    = 0;
  d.m_State                      = xiiResourceState::Loaded;

  if (Stream == nullptr)
  {
    d.m_State = xiiResourceState::LoadedResourceMissing;
    return d;
  }

  m_RequiredFiles.ReadDependencyFile(*Stream).IgnoreResult();
  Stream->ReadHashTable(m_IntData).IgnoreResult();
  Stream->ReadHashTable(m_FloatData).IgnoreResult();
  Stream->ReadHashTable(m_DoubleData).IgnoreResult();
  Stream->ReadHashTable(m_StringData).IgnoreResult();
  Stream->ReadHashTable(m_BoolData).IgnoreResult();

  return d;
}

void xiiConfigFileResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = m_IntData.GetHeapMemoryUsage() + m_FloatData.GetHeapMemoryUsage() + m_StringData.GetHeapMemoryUsage() + m_BoolData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

xiiResult xiiConfigFileResourceLoader::LoadedData::PrePropFileLocator(xiiStringView sCurAbsoluteFile, xiiStringView sIncludeFile, xiiPreprocessor::IncludeType incType, xiiStringBuilder& out_sAbsoluteFilePath)
{
  xiiResult res = xiiPreprocessor::DefaultFileLocator(sCurAbsoluteFile, sIncludeFile, incType, out_sAbsoluteFilePath);

  m_RequiredFiles.AddFileDependency(out_sAbsoluteFilePath);

  return res;
}

xiiResourceLoadData xiiConfigFileResourceLoader::OpenDataStream(const xiiResource* pResource)
{
  XII_PROFILE_SCOPE("ReadResourceFile");
  XII_LOG_BLOCK("Load Config Resource", pResource->GetResourceID());

  xiiStringBuilder sConfig;

  xiiMap<xiiString, xiiInt32>  intData;
  xiiMap<xiiString, float>     floatData;
  xiiMap<xiiString, double>    doubleData;
  xiiMap<xiiString, xiiString> stringData;
  xiiMap<xiiString, bool>      boolData;

  LoadedData* pData = XII_DEFAULT_NEW(LoadedData);
  pData->m_Reader.SetStorage(&pData->m_Storage);

  xiiPreprocessor preprop;

  // used to gather all the transitive file dependencies
  preprop.SetFileLocatorFunction(xiiMakeDelegate(&xiiConfigFileResourceLoader::LoadedData::PrePropFileLocator, pData));

  if (pResource->GetResourceID() == "Empty.xiiConfig")
  {
    // do nothing
  }
  else if (preprop.Process(pResource->GetResourceID(), sConfig, false, true, false).Succeeded())
  {
    sConfig.ReplaceAll("\r", "");
    sConfig.ReplaceAll("\n", ";");

    xiiHybridArray<xiiStringView, 32> lines;
    sConfig.Split(false, lines, ";");

    xiiStringBuilder key, value, line;

    for (xiiStringView tmp : lines)
    {
      line = tmp;
      line.Trim(" \t");

      if (line.IsEmpty())
        continue;

      const char* szAssign = line.FindSubString("=");

      if (szAssign == nullptr)
      {
        xiiLog::Error("Invalid line in config file: '{}'", tmp);
      }
      else
      {
        value = szAssign + 1;
        value.Trim(" ");

        line.SetSubString_FromTo(line.GetData(), szAssign);
        line.ReplaceAll("\t", " ");
        line.ReplaceAll("  ", " ");
        line.Trim(" ");

        const bool bOverride = line.TrimWordStart("override ");
        line.Trim(" ");

        if (line.StartsWith("int "))
        {
          key.SetSubString_FromTo(line.GetData() + 4, szAssign);
          key.Trim(" ");

          if (bOverride && !intData.Contains(key))
            xiiLog::Error("Config 'int' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && intData.Contains(key))
            xiiLog::Error("Config 'int' key '{}' is not marked override, but exist already. Use 'override int' instead.", key);

          xiiInt32 val;
          if (xiiConversionUtils::StringToInt(value, val).Succeeded())
          {
            intData[key] = val;
          }
          else
          {
            xiiLog::Error("Failed to parse 'int' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("float "))
        {
          key.SetSubString_FromTo(line.GetData() + 6, szAssign);
          key.Trim(" ");

          if (bOverride && !floatData.Contains(key))
            xiiLog::Error("Config 'float' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && floatData.Contains(key))
            xiiLog::Error("Config 'float' key '{}' is not marked override, but exist already. Use 'override float' instead.", key);

          double val;
          if (xiiConversionUtils::StringToFloat(value, val).Succeeded())
          {
            floatData[key] = (float)val;
          }
          else
          {
            xiiLog::Error("Failed to parse 'float' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("double "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, szAssign);
          key.Trim(" ");

          if (bOverride && !doubleData.Contains(key))
            xiiLog::Error("Config 'double' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && doubleData.Contains(key))
            xiiLog::Error("Config 'double' key '{}' is not marked override, but exist already. Use 'override double' instead.", key);

          double val;
          if (xiiConversionUtils::StringToFloat(value, val).Succeeded())
          {
            doubleData[key] = val;
          }
          else
          {
            xiiLog::Error("Failed to parse 'double' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("bool "))
        {
          key.SetSubString_FromTo(line.GetData() + 5, szAssign);
          key.Trim(" ");

          if (bOverride && !boolData.Contains(key))
            xiiLog::Error("Config 'bool' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && boolData.Contains(key))
            xiiLog::Error("Config 'bool' key '{}' is not marked override, but exist already. Use 'override bool' instead.", key);

          bool val;
          if (xiiConversionUtils::StringToBool(value, val).Succeeded())
          {
            boolData[key] = val;
          }
          else
          {
            xiiLog::Error("Failed to parse 'bool' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("string "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, szAssign);
          key.Trim(" ");

          if (bOverride && !stringData.Contains(key))
            xiiLog::Error("Config 'string' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && stringData.Contains(key))
            xiiLog::Error("Config 'string' key '{}' is not marked override, but exist already. Use 'override string' instead.", key);

          if (!value.StartsWith("\"") || !value.EndsWith("\""))
          {
            xiiLog::Error("Failed to parse 'string' in config file: '{}'", tmp);
          }
          else
          {
            value.Shrink(1, 1);
            stringData[key] = value;
          }
        }
        else
        {
          xiiLog::Error("Invalid line in config file: '{}'", tmp);
        }
      }
    }
  }
  else
  {
    // empty stream
    return {};
  }

  xiiResourceLoadData res;
  res.m_pDataStream       = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

#if XII_ENABLED(XII_SUPPORTS_FILE_STATS)
  xiiFileStats stat;
  if (xiiFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_sResourceDescription       = stat.m_sName;
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }
#endif

  xiiMemoryStreamWriter writer(&pData->m_Storage);

  pData->m_RequiredFiles.StoreCurrentTimeStamp();
  pData->m_RequiredFiles.WriteDependencyFile(writer).IgnoreResult();
  writer.WriteMap(intData).IgnoreResult();
  writer.WriteMap(floatData).IgnoreResult();
  writer.WriteMap(doubleData).IgnoreResult();
  writer.WriteMap(stringData).IgnoreResult();
  writer.WriteMap(boolData).IgnoreResult();

  return res;
}

void xiiConfigFileResourceLoader::CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData)
{
  LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);

  XII_DEFAULT_DELETE(pData);
}

bool xiiConfigFileResourceLoader::IsResourceOutdated(const xiiResource* pResource) const
{
  return static_cast<const xiiConfigFileResource*>(pResource)->m_RequiredFiles.HasAnyFileChanged();
}


XII_STATICLINK_FILE(Utilities, Utilities_Resources_ConfigFileResource);
