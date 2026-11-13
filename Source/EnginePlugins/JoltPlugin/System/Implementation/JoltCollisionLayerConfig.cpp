/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <JoltPlugin/JoltPluginPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/ConversionUtils.h>
#include <JoltPlugin/System/JoltCollisionLayerConfig.h>

xiiJoltCollisionLayerConfig::xiiJoltCollisionLayerConfig()
{
  m_Names[0] = "Default";
  for (auto& row : m_Masks)
    for (auto& word : row)
      word = ~xiiUInt64{0};
}

xiiStatus xiiJoltCollisionLayerConfig::SetName(xiiUInt8 uiLayer, xiiStringView sName)
{
  xiiStringBuilder name(sName);
  name.Trim(" \t\r\n");
  if (name.GetCharacterCount() > 64)
    return xiiStatus("Layer names must contain at most 64 characters.");
  if (!name.IsEmpty())
  {
    xiiUInt32   numeric;
    const char* end = nullptr;
    if (xiiConversionUtils::StringToUInt(name, numeric, &end).Succeeded() && *end == '\0')
      return xiiStatus("Layer names cannot be numeric IDs.");
    for (xiiUInt32 i = 0; i < LayerCount; ++i)
      if (i != uiLayer && m_Names[i].IsEqual_NoCase(name))
        return xiiStatus("Collision layer names must be unique (ignoring case).");
  }
  m_Names[uiLayer] = name;
  return XII_SUCCESS;
}

xiiUInt32 xiiJoltCollisionLayerConfig::FindLayer(xiiStringView sName) const
{
  if (!sName.IsEmpty())
    for (xiiUInt32 i = 0; i < LayerCount; ++i)
      if (m_Names[i].IsEqual_NoCase(sName))
        return i;
  return xiiInvalidIndex;
}

void xiiJoltCollisionLayerConfig::SetCollisionEnabled(xiiUInt8 uiA, xiiUInt8 uiB, bool bEnabled)
{
  const auto set = [bEnabled](xiiUInt64& word, xiiUInt8 bit) {
    const auto mask = xiiUInt64{1} << (bit % 64);
    if (bEnabled)
      word |= mask;
    else
      word &= ~mask;
  };
  set(m_Masks[uiA][uiB / 64], uiB);
  set(m_Masks[uiB][uiA / 64], uiA);
}

namespace
{
  xiiResult ReadID(const xiiOpenDdlReaderElement* element, xiiUInt32& value)
  {
    if (element == nullptr || !element->HasPrimitives(xiiOpenDdlPrimitiveType::UInt32, 1) || element->GetNumPrimitives() != 1)
      return XII_FAILURE;
    value = element->GetPrimitivesUInt32()[0];
    return XII_SUCCESS;
  }
  xiiResult ReadName(const xiiOpenDdlReaderElement* element, xiiString& value)
  {
    if (element == nullptr || !element->HasPrimitives(xiiOpenDdlPrimitiveType::String, 1) || element->GetNumPrimitives() != 1)
      return XII_FAILURE;
    value = element->GetPrimitivesString()[0];
    return XII_SUCCESS;
  }
} // namespace

xiiStatus xiiJoltCollisionLayerConfig::Load(xiiStringView sPath)
{
  if (!xiiFileSystem::ExistsFile(sPath))
  {
    *this = xiiJoltCollisionLayerConfig();
    return XII_SUCCESS;
  }
  xiiFileReader file;
  if (file.Open(sPath).Failed())
    return xiiStatus("Could not open the Jolt collision-layer configuration.");
  xiiOpenDdlReader reader;
  if (reader.ParseDocument(file).Failed())
    return xiiStatus("Invalid Jolt collision-layer configuration DDL.");
  const auto* root    = reader.GetRootElement()->FindChildOfType("JoltCollisionLayers");
  xiiUInt32   version = 0;
  if (root == nullptr || ReadID(root->FindChild("Version"), version).Failed() || version != 1)
    return xiiStatus("Unsupported Jolt collision-layer configuration version.");
  xiiJoltCollisionLayerConfig replacement;
  // Clear defaults before validating names, so layer zero can be renamed or moved.
  for (auto& name : replacement.m_Names)
    name.Clear();
  bool seen[LayerCount]              = {};
  bool pairs[LayerCount][LayerCount] = {};
  for (const auto* entry = root->GetFirstChild(); entry; entry = entry->GetSibling())
  {
    if (entry->IsCustomType("Layer"))
    {
      xiiUInt32 id;
      xiiString name;
      if (ReadID(entry->FindChild("ID"), id).Failed() || id >= LayerCount || seen[id] ||
          ReadName(entry->FindChild("Name"), name).Failed())
        return xiiStatus("Invalid or duplicate Jolt collision layer ID.");
      seen[id] = true;
      XII_SUCCEED_OR_RETURN(replacement.SetName(static_cast<xiiUInt8>(id), name));
    }
    else if (entry->IsCustomType("DisabledPair"))
    {
      xiiUInt32 a, b;
      if (ReadID(entry->FindChild("A"), a).Failed() ||
          ReadID(entry->FindChild("B"), b).Failed() || a >= LayerCount || b >= LayerCount || pairs[a][b])
        return xiiStatus("Invalid or duplicate Jolt collision pair.");
      pairs[a][b] = pairs[b][a] = true;
      replacement.SetCollisionEnabled(static_cast<xiiUInt8>(a), static_cast<xiiUInt8>(b), false);
    }
  }
  *this = std::move(replacement);
  return XII_SUCCESS;
}

xiiStatus xiiJoltCollisionLayerConfig::Save(xiiStringView sPath) const
{
  xiiDeferredFileWriter file;
  file.SetOutput(sPath);
  xiiOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.BeginObject("JoltCollisionLayers");
  xiiOpenDdlUtils::StoreUInt32(writer, 1, "Version");
  for (xiiUInt32 i = 0; i < LayerCount; ++i)
  {
    if (!m_Names[i].IsEmpty())
    {
      writer.BeginObject("Layer");
      xiiOpenDdlUtils::StoreUInt32(writer, i, "ID");
      xiiOpenDdlUtils::StoreString(writer, m_Names[i], "Name");
      writer.EndObject();
    }
    for (xiiUInt32 j = i; j < LayerCount; ++j)
      if (!IsCollisionEnabled(static_cast<xiiUInt8>(i), static_cast<xiiUInt8>(j)))
      {
        writer.BeginObject("DisabledPair");
        xiiOpenDdlUtils::StoreUInt32(writer, i, "A");
        xiiOpenDdlUtils::StoreUInt32(writer, j, "B");
        writer.EndObject();
      }
  }
  writer.EndObject();
  return file.Close().Succeeded() ? xiiStatus(XII_SUCCESS) : xiiStatus("Could not save the Jolt collision-layer configuration.");
}

XII_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_Implementation_JoltCollisionLayerConfig);
