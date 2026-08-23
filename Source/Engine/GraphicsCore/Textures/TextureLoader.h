/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GraphicsCore/GraphicsCoreDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <Texture/Image/Image.h>

class XII_GRAPHICSCORE_DLL xiiTextureResourceLoader : public xiiResourceTypeLoader
{
public:
  struct LoadedData
  {
    LoadedData() :
      m_Reader(&m_Storage)
    {
    }

    xiiContiguousMemoryStreamStorage m_Storage;
    xiiMemoryStreamReader            m_Reader;
    xiiImage                         m_Image;

    xiiGALTextureCreationDescription m_TextureDescription;
    xiiGALSamplerCreationDescription m_SamplerDescription;
    bool                             m_bIsFallback = false;
  };

  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;
  virtual bool                IsResourceOutdated(const xiiResource* pResource) const override;

  static xiiResult LoadTexFile(xiiStreamReader& inout_stream, LoadedData& ref_data);
  static void      WriteTextureLoadStream(xiiStreamWriter& inout_stream, const LoadedData& data);
};
