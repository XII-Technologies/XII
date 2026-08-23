/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <Core/CorePCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/Utilities/AssetFileHeader.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiCurve1DResource, 1, xiiRTTIDefaultAllocator<xiiCurve1DResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiCurve1DResource);

xiiCurve1DResource::xiiCurve1DResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

XII_RESOURCE_IMPLEMENT_CREATEABLE(xiiCurve1DResource, xiiCurve1DResourceDescriptor)
{
  m_Descriptor = descriptor;

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Loaded;

  return res;
}

xiiResourceLoadDescription xiiCurve1DResource::UnloadData(Unload WhatToUnload)
{
  XII_IGNORE_UNUSED(WhatToUnload);

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;
  res.m_State                      = xiiResourceState::Unloaded;

  m_Descriptor.m_Curves.Clear();

  return res;
}

xiiResourceLoadDescription xiiCurve1DResource::UpdateContent(xiiStreamReader* Stream)
{
  XII_LOG_BLOCK("xiiCurve1DResource::UpdateContent", GetResourceIdOrDescription());

  xiiResourceLoadDescription res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable    = 0;

  if (Stream == nullptr)
  {
    res.m_State = xiiResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    xiiStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  // skip the asset file header at the start of the file
  xiiAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  m_Descriptor.Load(*Stream);

  res.m_State = xiiResourceState::Loaded;
  return res;
}

void xiiCurve1DResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
  out_NewMemoryUsage.m_uiMemoryCPU = static_cast<xiiUInt32>(m_Descriptor.m_Curves.GetHeapMemoryUsage()) + static_cast<xiiUInt32>(sizeof(m_Descriptor));

  for (const auto& curve : m_Descriptor.m_Curves)
  {
    out_NewMemoryUsage.m_uiMemoryCPU += curve.GetHeapMemoryUsage();
  }
}

void xiiCurve1DResourceDescriptor::Save(xiiStreamWriter& ref_stream) const
{
  const xiiUInt8 uiVersion = 1;

  ref_stream << uiVersion;

  const xiiUInt8 uiCurves = static_cast<xiiUInt8>(m_Curves.GetCount());
  ref_stream << uiCurves;

  for (xiiUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Save(ref_stream);
  }
}

void xiiCurve1DResourceDescriptor::Load(xiiStreamReader& ref_stream)
{
  xiiUInt8 uiVersion = 0;

  ref_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion == 1, "Invalid file version {0}", uiVersion);

  xiiUInt8 uiCurves = 0;
  ref_stream >> uiCurves;

  m_Curves.SetCount(uiCurves);

  for (xiiUInt32 i = 0; i < uiCurves; ++i)
  {
    m_Curves[i].Load(ref_stream);

    /// \todo We can do this on load, or somehow ensure this is always already correctly saved
    m_Curves[i].SortControlPoints();
    m_Curves[i].CreateLinearApproximation();
  }
}



XII_STATICLINK_FILE(Core, Core_Curves_Implementation_Curve1DResource);
