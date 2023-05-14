#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class xiiChunkStreamWriter;
class xiiChunkStreamReader;

struct xiiProfileTargetPlatform
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    PC,
    UWP,
    Android,

    Default = PC
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_CORE_DLL, xiiProfileTargetPlatform);

//////////////////////////////////////////////////////////////////////////

/// \brief Base class for configuration objects that store e.g. asset transform settings or runtime configuration information
class XII_CORE_DLL xiiProfileConfigData : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiProfileConfigData, xiiReflectedClass);

public:
  xiiProfileConfigData();
  ~xiiProfileConfigData();

  virtual void SaveRuntimeData(xiiChunkStreamWriter& stream) const;
  virtual void LoadRuntimeData(xiiChunkStreamReader& stream);
};

//////////////////////////////////////////////////////////////////////////

class XII_CORE_DLL xiiPlatformProfile : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPlatformProfile, xiiReflectedClass);

public:
  xiiPlatformProfile();
  ~xiiPlatformProfile();

  xiiStringView GetConfigName() const { return m_sName; }

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

  xiiString                              m_sName;
  xiiEnum<xiiProfileTargetPlatform>      m_TargetPlatform;
  xiiDynamicArray<xiiProfileConfigData*> m_Configs;
};

//////////////////////////////////////////////////////////////////////////
