/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Texture/TexturePCH.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Texture/Utilities/TextureAtlasDescription.h>

xiiResult xiiTextureAtlasCreationDesc::Serialize(xiiStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(3);

  if (m_Layers.GetCount() > 255u)
    return XII_FAILURE;

  const xiiUInt8 uiNumLayers = static_cast<xiiUInt8>(m_Layers.GetCount());
  inout_stream << uiNumLayers;

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream << m_Layers[l].m_Usage;
    inout_stream << m_Layers[l].m_uiNumChannels;
  }

  inout_stream << m_Items.GetCount();
  for (auto& item : m_Items)
  {
    inout_stream << item.m_uiUniqueID;
    inout_stream << item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream << item.m_sLayerInput[l];
    }

    inout_stream << item.m_sAlphaInput;
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasCreationDesc::Deserialize(xiiStreamReader& inout_stream)
{
  const xiiTypeVersion uiVersion = inout_stream.ReadVersion(3);

  xiiUInt8 uiNumLayers = 0;
  inout_stream >> uiNumLayers;

  m_Layers.SetCount(uiNumLayers);

  for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
  {
    inout_stream >> m_Layers[l].m_Usage;
    inout_stream >> m_Layers[l].m_uiNumChannels;
  }

  xiiUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.SetCount(uiNumItems);

  for (auto& item : m_Items)
  {
    inout_stream >> item.m_uiUniqueID;
    inout_stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < uiNumLayers; ++l)
    {
      inout_stream >> item.m_sLayerInput[l];
    }

    if (uiVersion >= 3)
    {
      inout_stream >> item.m_sAlphaInput;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasCreationDesc::Save(xiiStringView sFile) const
{
  xiiFileWriter file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  return Serialize(file);
}

xiiResult xiiTextureAtlasCreationDesc::Load(xiiStringView sFile)
{
  xiiFileReader file;
  XII_SUCCEED_OR_RETURN(file.Open(sFile));

  return Deserialize(file);
}

void xiiTextureAtlasRuntimeDesc::Clear()
{
  m_uiNumLayers = 0;
  m_Items.Clear();
}

xiiResult xiiTextureAtlasRuntimeDesc::Serialize(xiiStreamWriter& inout_stream) const
{
  m_Items.Sort();

  inout_stream << m_uiNumLayers;
  inout_stream << m_Items.GetCount();

  for (xiiUInt32 i = 0; i < m_Items.GetCount(); ++i)
  {
    inout_stream << m_Items.GetKey(i);
    inout_stream << m_Items.GetValue(i).m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      const auto& r = m_Items.GetValue(i).m_LayerRects[l];
      inout_stream << r.x;
      inout_stream << r.y;
      inout_stream << r.width;
      inout_stream << r.height;
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureAtlasRuntimeDesc::Deserialize(xiiStreamReader& inout_stream)
{
  Clear();

  inout_stream >> m_uiNumLayers;

  xiiUInt32 uiNumItems = 0;
  inout_stream >> uiNumItems;
  m_Items.Reserve(uiNumItems);

  for (xiiUInt32 i = 0; i < uiNumItems; ++i)
  {
    xiiUInt32 key = 0;
    inout_stream >> key;

    auto& item = m_Items[key];
    inout_stream >> item.m_uiFlags;

    for (xiiUInt32 l = 0; l < m_uiNumLayers; ++l)
    {
      auto& r = item.m_LayerRects[l];
      inout_stream >> r.x;
      inout_stream >> r.y;
      inout_stream >> r.width;
      inout_stream >> r.height;
    }
  }

  m_Items.Sort();
  return XII_SUCCESS;
}
