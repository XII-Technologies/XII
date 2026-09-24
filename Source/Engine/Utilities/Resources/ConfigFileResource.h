/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/IO/DependencyFile.h>
#include <Foundation/Strings/HashedString.h>
#include <Utilities/UtilitiesDLL.h>

using xiiConfigFileResourceHandle = xiiTypedResourceHandle<class xiiConfigFileResource>;

/// This resource loads config files containing key/value pairs
///
/// The config files usually use the file extension '.xiiConfig'.
///
/// The file format looks like this:
///
/// To declare a key/value pair for the first time, write its type, name and value:
///   int i = 1
///   float f = 2.3
///   bool b = false
///   string s = "hello"
///
/// To set a variable to a different value than before, it has to be marked with 'override':
///
///   override i = 4
///
/// The format supports C preprocessor features like #include, #define, #ifdef, etc.
/// This can be used to build hierarchical config files:
///
///   #include "BaseConfig.xiiConfig"
///   override int SomeValue = 7
///
/// It can also be used to define 'enum types':
///
///   #define SmallValue 3
///   #define BigValue 5
///   int MyValue = BigValue
///
/// Since resources can be reloaded at runtime, config resources are a convenient way to define game parameters
/// that you may want to tweak at any time.
/// Using C preprocessor logic (#define, #if, #else, etc) you can quickly select between different configuration sets.
///
/// Once loaded, accessing the data is very efficient.
class XII_UTILITIES_DLL xiiConfigFileResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiConfigFileResource, xiiResource);

  XII_RESOURCE_DECLARE_COMMON_CODE(xiiConfigFileResource);

public:
  xiiConfigFileResource();
  ~xiiConfigFileResource();

  /// Returns the 'int' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  xiiInt32 GetInt(xiiTempHashedString sName) const;

  /// Returns the 'float' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  float GetFloat(xiiTempHashedString sName) const;

  /// Returns the 'double' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  double GetDouble(xiiTempHashedString sName) const;

  /// Returns the 'bool' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  bool GetBool(xiiTempHashedString sName) const;

  /// Returns the 'string' variable with the given name. Logs an error, if the variable doesn't exist in the config file.
  const char* GetString(xiiTempHashedString sName) const;

  /// Returns the 'int' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  xiiInt32 GetInt(xiiTempHashedString sName, xiiInt32 iFallback) const;

  /// Returns the 'float' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  float GetFloat(xiiTempHashedString sName, float fFallback) const;

  /// Returns the 'double' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  double GetDouble(xiiTempHashedString sName, double fFallback) const;

  /// Returns the 'bool' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  bool GetBool(xiiTempHashedString sName, bool bFallback) const;

  /// Returns the 'string' variable with the given name. Returns the 'fallback' value, if the variable doesn't exist in the config file.
  const char* GetString(xiiTempHashedString sName, const char* szFallback) const;

protected:
  virtual xiiResourceLoadDescription UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDescription UpdateContent(xiiStreamReader* Stream) override;
  virtual void                       UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

private:
  friend class xiiConfigFileResourceLoader;

  xiiHashTable<xiiHashedString, xiiInt32>  m_IntData;
  xiiHashTable<xiiHashedString, float>     m_FloatData;
  xiiHashTable<xiiHashedString, double>    m_DoubleData;
  xiiHashTable<xiiHashedString, xiiString> m_StringData;
  xiiHashTable<xiiHashedString, bool>      m_BoolData;

  xiiDependencyFile m_RequiredFiles;
};


class XII_UTILITIES_DLL xiiConfigFileResourceLoader : public xiiResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData() :
      m_Reader(&m_Storage)
    {
    }

    xiiDefaultMemoryStreamStorage m_Storage;
    xiiMemoryStreamReader         m_Reader;
    xiiDependencyFile             m_RequiredFiles;

    xiiResult PrePropFileLocator(xiiStringView sCurAbsoluteFile, xiiStringView sIncludeFile, xiiPreprocessor::IncludeType incType, xiiStringBuilder& out_sAbsoluteFilePath);
  };

  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;
  virtual bool                IsResourceOutdated(const xiiResource* pResource) const override;
};
