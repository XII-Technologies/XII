#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

xiiResult xiiTextureAtlasCreationDesc::Serialize(xiiStreamWriter& ref_stream) const
{
  ref_stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return XII_FAILURE;

  const xiiUInt8 uiNumLayers = static_cast<xiiUInt8>(m_Layers.GetCount());
  ref_stream << uiNumLayers;

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    ref_stream << m_Layers[l].m_Usage;
    ref_stream << m_Layers[l].m_uiNumChannels;
  }

  ref_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    ref_stream << item.m_uiUniqueID;
    ref_stream << item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      ref_stream << item.m_sLayerInput[l];
    }

    ref_stream << item.m_sAlphaInput;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasCreationDesc::Deserialize(xiiStreamReader& ref_stream)
{
  const xiiTypeVersion uiVersion = ref_stream.ReadVersion(3);

  xiiUInt8 uiNumLayers = 0;
  ref_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    ref_stream >> m_Layers[l].m_Usage;
    ref_stream >> m_Layers[l].m_uiNumChannels;
  }

  xiiUInt32 uiNumItems = 0;
  ref_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    ref_stream >> item.m_uiUniqueID;
    ref_stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      ref_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      ref_stream >> item.m_sAlphaInput;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasCreationDesc::Save(const char* szFile) const
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  return Serialize(file);
}

xiiResult xiiTextureAtlasCreationDesc::Load(const char* szFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(szFile));

  return Deserialize(file);
}

void xiiTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

xiiResult xiiTextureAtlasRuntimeDesc::Serialize(xiiStreamWriter& ref_stream) const
{
  m_Items.Sort();

  ref_stream << m_uiNumLayers;
  ref_stream << m_Items.GetCount();

  for (xiiUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    ref_stream << m_Items.GetKey(i);
    ref_stream << m_Items.GetValue(i).m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      ref_stream << r.x;
      ref_stream << r.y;
      ref_stream << r.width;
      ref_stream << r.height;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasRuntimeDesc::Deserialize(xiiStreamReader& ref_stream)
{
  Clear();

  ref_stream >> m_uiNumLayers;

  xiiUInt32 uiNumItems = 0;
  ref_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (xiiUInt32 i = 0; i < uiNumItems; ++i)
  {
    xiiUInt32 key = 0;
    ref_stream >> key;

    auto& item = m_Items[key];
    ref_stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      ref_stream >> r.x;
      ref_stream >> r.y;
      ref_stream >> r.width;
      ref_stream >> r.height;
    }
  }

  m_Items.Sort();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureAtlasDesc);
