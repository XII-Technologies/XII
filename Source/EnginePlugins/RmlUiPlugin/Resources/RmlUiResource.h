#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Foundation/IO/DependencyFile.h>
#include <RmlUiPlugin/RmlUiPluginDLL.h>

struct XII_RMLUIPLUGIN_DLL xiiRmlUiScaleMode
{
  using StorageType = xiiUInt8;

  enum Enum
  {
    Fixed,
    WithScreenSize,

    Default = Fixed
  };
};

XII_DECLARE_REFLECTABLE_TYPE(XII_RMLUIPLUGIN_DLL, xiiRmlUiScaleMode);

struct XII_RMLUIPLUGIN_DLL xiiRmlUiResourceDescriptor
{
  xiiResult Save(xiiStreamWriter& stream);
  xiiResult Load(xiiStreamReader& stream);

  xiiDependencyFile m_DependencyFile;

  xiiString                  m_sRmlFile;
  xiiEnum<xiiRmlUiScaleMode> m_ScaleMode;
  xiiVec2U32                 m_ReferenceResolution;
};

using xiiRmlUiResourceHandle = xiiTypedResourceHandle<class xiiRmlUiResource>;

class XII_RMLUIPLUGIN_DLL xiiRmlUiResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiRmlUiResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiRmlUiResource);
  XII_RESOURCE_DECLARE_CREATEABLE(xiiRmlUiResource, xiiRmlUiResourceDescriptor);

public:
  xiiRmlUiResource();

  const xiiString&                  GetRmlFile() const { return m_sRmlFile; }
  const xiiEnum<xiiRmlUiScaleMode>& GetScaleMode() const { return m_ScaleMode; }
  const xiiVec2U32&                 GetReferenceResolution() const { return m_vReferenceResolution; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* Stream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiString                  m_sRmlFile;
  xiiEnum<xiiRmlUiScaleMode> m_ScaleMode;
  xiiVec2U32                 m_vReferenceResolution = xiiVec2U32::ZeroVector();
};

class xiiRmlUiResourceLoader : public xiiResourceLoaderFromFile
{
public:
  virtual bool IsResourceOutdated(const xiiResource* pResource) const override;
};
