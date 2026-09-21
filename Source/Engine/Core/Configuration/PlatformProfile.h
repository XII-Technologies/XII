/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiChunkStreamWriter;
class xiiChunkStreamReader;

//////////////////////////////////////////////////////////////////////////

/// Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class XII_CORE_DLL xiiProfileConfigData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProfileConfigData, xiiReflectedClass);

public:
  xiiProfileConfigData();
  ~xiiProfileConfigData();

  virtual void SaveRuntimeData(xiiChunkStreamWriter& ref_stream) const;
  virtual void LoadRuntimeData(xiiChunkStreamReader& ref_stream);
};

//////////////////////////////////////////////////////////////////////////

class XII_CORE_DLL xiiPlatformProfile final : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlatformProfile, xiiReflectedClass);

public:
  xiiPlatformProfile();
  ~xiiPlatformProfile();

  void          SetConfigName(xiiStringView sName) { m_sName = sName; }
  xiiStringView GetConfigName() const { return m_sName; }

  void          SetTargetPlatform(xiiStringView sPlatform) { m_sTargetPlatform = sPlatform; }
  xiiStringView GetTargetPlatform() const { return m_sTargetPlatform; }

  void Clear();
  void AddMissingConfigs();

  template <typename TYPE>
  const TYPE* GetTypeConfig() const
  {
    return static_cast<const TYPE*>(GetTypeConfig(xiiGetStaticRTTI<TYPE>()));
  }

  template <typename TYPE>
  TYPE* GetTypeConfig()
  {
    return static_cast<TYPE*>(GetTypeConfig(xiiGetStaticRTTI<TYPE>()));
  }

  const xiiProfileConfigData* GetTypeConfig(const xiiRTTI* pRtti) const;
  xiiProfileConfigData*       GetTypeConfig(const xiiRTTI* pRtti);

  xiiResult SaveForRuntime(xiiStringView sFile) const;
  xiiResult LoadForRuntime(xiiStringView sFile);

  /// Returns a number indicating when the profile counter changed last. By storing and comparing this value, other code can update their state if necessary.
  xiiUInt32 GetLastModificationCounter() const { return m_uiLastModificationCounter; }

private:
  xiiUInt32                              m_uiLastModificationCounter = 0;
  xiiString                              m_sName;
  xiiString                              m_sTargetPlatform = "Windows";
  xiiDynamicArray<xiiProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
