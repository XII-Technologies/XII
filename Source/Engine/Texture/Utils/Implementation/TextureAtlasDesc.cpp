#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utils/TextureAtlasDesc.h>

xiiResult xiiTextureAtlasCreationDesc::Serialize(xiiStreamWriter& stream) const
{
  stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return XII_FAILURE;

  const xiiUInt8 uiNumLayers = static_cast<xiiUInt8>(m_Layers.GetCount());
  stream << uiNumLayers;

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream << m_Layers[l].m_Usage;
    stream << m_Layers[l].m_uiNumChannels;
  }

  stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    stream << item.m_uiUniqueID;
    stream << item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream << item.m_sLayerInput[l];
    }

    stream << item.m_sAlphaInput;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasCreationDesc::Deserialize(xiiStreamReader& stream)
{
  const xiiTypeVersion uiVersion = stream.ReadVersion(3);

  xiiUInt8 uiNumLayers = 0;
  stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    stream >> m_Layers[l].m_Usage;
    stream >> m_Layers[l].m_uiNumChannels;
  }

  xiiUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    stream >> item.m_uiUniqueID;
    stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      stream >> item.m_sAlphaInput;
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

xiiResult xiiTextureAtlasRuntimeDesc::Serialize(xiiStreamWriter& stream) const
{
  m_Items.Sort();

  stream << m_uiNumLayers;
  stream << m_Items.GetCount();

  for (xiiUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    stream << m_Items.GetKey(i);
    stream << m_Items.GetValue(i).m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      stream << r.x;
      stream << r.y;
      stream << r.width;
      stream << r.height;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasRuntimeDesc::Deserialize(xiiStreamReader& stream)
{
  Clear();

  stream >> m_uiNumLayers;

  xiiUInt32 uiNumItems = 0;
  stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (xiiUInt32 i = 0; i < uiNumItems; ++i)
  {
    xiiUInt32 key = 0;
    stream >> key;

    auto& item = m_Items[key];
    stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      stream >> r.x;
      stream >> r.y;
      stream >> r.width;
      stream >> r.height;
    }
  }

  m_Items.Sort();
  return XII_SUCCESS;
}

XII_STATICLINK_FILE(Texture, Texture_Utils_Implementation_TextureAtlasDesc);
