#pragma once

#include <Core/ResourceManager/Resource.h>
#include <Core/ResourceManager/ResourceTypeLoader.h>
#include <GraphicsCore/GraphicsCoreDLL.h>
#include <GraphicsCore/RenderContext/Implementation/RenderContextStructs.h>
#include <GraphicsFoundation/GraphicsFoundationDLL.h>
#include <Texture/Image/Image.h>
#include <Texture/xiiTexFormat/xiiTexFormat.h>

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

    bool         m_bIsFallback = false;
    xiiTexFormat m_TexFormat;
  };

  virtual xiiResourceLoadData OpenDataStream(const xiiResource* pResource) override;
  virtual void                CloseDataStream(const xiiResource* pResource, const xiiResourceLoadData& loaderData) override;
  virtual bool                IsResourceOutdated(const xiiResource* pResource) const override;

  static xiiResult LoadTexFile(xiiStreamReader& inout_stream, LoadedData& ref_data);
  static void      WriteTextureLoadStream(xiiStreamWriter& inout_stream, const LoadedData& data);
};
