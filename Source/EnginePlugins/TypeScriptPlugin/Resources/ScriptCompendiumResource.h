#pragma once

#include <TypeScriptPlugin/TypeScriptPluginDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Foundation/Containers/DynamicArray.h>

class xiiStreamWriter;
class xiiStreamReader;

struct XII_TYPESCRIPTPLUGIN_DLL xiiScriptCompendiumResourceDesc
{
  xiiMap<xiiString, xiiString> m_PathToSource;

  struct ComponentTypeInfo
  {
    xiiString m_sComponentTypeName;
    xiiString m_sComponentFilePath;

    xiiResult Serialize(xiiStreamWriter& stream) const;
    xiiResult Deserialize(xiiStreamReader& stream);
  };

  xiiMap<xiiUuid, ComponentTypeInfo> m_AssetGuidToInfo;

  xiiResult Serialize(xiiStreamWriter& stream) const;
  xiiResult Deserialize(xiiStreamReader& stream);
};

class XII_TYPESCRIPTPLUGIN_DLL xiiScriptCompendiumResource : public xiiResource
{
  XII_ADD_DYNAMIC_REFLECTION(xiiScriptCompendiumResource, xiiResource);
  XII_RESOURCE_DECLARE_COMMON_CODE(xiiScriptCompendiumResource);

public:
  xiiScriptCompendiumResource();
  ~xiiScriptCompendiumResource();

  const xiiScriptCompendiumResourceDesc& GetDescriptor() const { return m_Desc; }

private:
  virtual xiiResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual xiiResourceLoadDesc UpdateContent(xiiStreamReader* pStream) override;
  virtual void                UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  xiiScriptCompendiumResourceDesc m_Desc;
};

using xiiScriptCompendiumResourceHandle = xiiTypedResourceHandle<class xiiScriptCompendiumResource>;
