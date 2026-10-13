/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/IO/Stream.h>

namespace xiiDecalSerialization
{
  inline constexpr xiiUInt32 MaxStringBytes = 16 * 1024;

  // These descriptor formats do not use string deduplication. Bound metadata
  // before allocation and require complete UTF-8 before interning resource IDs.
  inline xiiResult ReadString(xiiStreamReader& stream, xiiStringBuilder& text)
  {
    xiiUInt32 length = 0;
    if (stream.ReadDWordValue(&length).Failed() || length > MaxStringBytes)
      return XII_FAILURE;
    xiiHybridArray<char, 256> bytes;
    bytes.SetCountUninitialized(length + 1);
    if (stream.ReadBytes(bytes.GetData(), length) != length)
      return XII_FAILURE;
    bytes[length] = '\0';
    if (!xiiUnicodeUtils::IsValidUtf8(bytes.GetData(), bytes.GetData() + length) || xiiStringUtils::GetStringElementCount(bytes.GetData()) != length)
      return XII_FAILURE;
    text = xiiStringView(bytes.GetData(), length);
    return XII_SUCCESS;
  }

  template <typename ResourceType>
  xiiResult ReadResourceHandle(xiiStreamReader& stream, xiiTypedResourceHandle<ResourceType>& handle)
  {
    xiiStringBuilder type, id;
    XII_SUCCEED_OR_RETURN(ReadString(stream, type));
    if (type.IsEmpty())
    {
      handle.Invalidate();
      return XII_SUCCESS;
    }
    XII_SUCCEED_OR_RETURN(ReadString(stream, id));
    const auto* rtti = xiiResourceManager::FindResourceForAssetType(type);
    if (rtti == nullptr)
      rtti = xiiRTTI::FindTypeByName(type);
    if (id.IsEmpty() || rtti != xiiGetStaticRTTI<ResourceType>())
      return XII_FAILURE;
    handle = xiiResourceManager::LoadResource<ResourceType>(id);
    return XII_SUCCESS;
  }
} // namespace xiiDecalSerialization
