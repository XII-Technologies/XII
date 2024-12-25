#pragma once

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Strings/String.h>
#include <Texture/TexConv/TexConvEnums.h>

struct XII_TEXTURE_DLL xiiTextureAtlasCreationDesc
{
  struct Layer
  {
    xiiEnum<xiiTexConvUsage> m_Usage;
    xiiUInt8                 m_uiNumChannels = 4;
  };

  struct Item
  {
    xiiUInt32 m_uiUniqueID;
    xiiUInt32 m_uiFlags;
    xiiString m_sAlphaInput;
    xiiString m_sLayerInput[4];
  };

  xiiHybridArray<Layer, 4> m_Layers;
  xiiDynamicArray<Item>    m_Items;

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);

  xiiResult Save(xiiStringView sFile) const;
  xiiResult Load(xiiStringView sFile);
};

struct XII_TEXTURE_DLL xiiTextureAtlasRuntimeDesc
{
  struct Item
  {
    xiiUInt32  m_uiFlags;
    xiiRectU32 m_LayerRects[4];
  };

  xiiUInt32                    m_uiNumLayers = 0;
  xiiArrayMap<xiiUInt32, Item> m_Items;

  void Clear();

  xiiResult Serialize(xiiStreamWriter& inout_stream) const;
  xiiResult Deserialize(xiiStreamReader& inout_stream);
};
