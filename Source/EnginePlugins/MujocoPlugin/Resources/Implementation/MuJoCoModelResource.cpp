/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <MujocoPlugin/Resources/Implementation/MuJoCoCompiledModel.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelResource, 1, xiiRTTIDefaultAllocator<xiiMuJoCoModelResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMuJoCoModelResource);

xiiSharedPtr<xiiMuJoCoCompiledModel> xiiMuJoCoCompiledModel::LoadBinary(xiiArrayPtr<const xiiUInt8> bytes, xiiStringView sSource)
{
  ModelPtr pModel(bytes.IsEmpty() ? nullptr : mj_loadModelBuffer(bytes.GetPtr(), bytes.GetCount()), mj_deleteModel);
  return Adopt(std::move(pModel), sSource);
}

xiiSharedPtr<xiiMuJoCoCompiledModel> xiiMuJoCoCompiledModel::Adopt(ModelPtr pModel, xiiStringView sSource)
{
  if (!pModel)
  {
    xiiLog::Error("MuJoCo could not load model '{}'.", sSource);
    return {};
  }

  if (!xiiMath::IsFinite(pModel->opt.timestep) || pModel->opt.timestep <= 0.0)
  {
    xiiLog::Error("MuJoCo model '{}' has an invalid timestep.", sSource);
    return {};
  }

  return XII_DEFAULT_NEW(xiiMuJoCoCompiledModel, std::move(pModel));
}

xiiMuJoCoModelResource::xiiMuJoCoModelResource() :
  xiiResource(DoUpdate::OnAnyThread, 1)
{
}

xiiMuJoCoModelResource::~xiiMuJoCoModelResource() = default;

xiiResourceLoadDescription xiiMuJoCoModelResource::UnloadData(Unload)
{
  XII_LOCK(m_ModelMutex);
  m_pModel.Clear();

  xiiResourceLoadDescription desc;
  desc.m_State                      = xiiResourceState::Unloaded;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable    = 0;
  return desc;
}

xiiResourceLoadDescription xiiMuJoCoModelResource::UpdateContent(xiiStreamReader* pStream)
{
  xiiSharedPtr<xiiMuJoCoCompiledModel> pModel;
  if (pStream != nullptr)
  {
    // The resource file loader prefixes the payload with its source path.
    xiiUInt32 uiPathBytes = 0;
    if (pStream->ReadDWordValue(&uiPathBytes).Succeeded() && pStream->SkipBytes(uiPathBytes) == uiPathBytes)
    {
      xiiContiguousMemoryStreamStorage bytes;
      bytes.ReadAll(*pStream, xiiMath::MaxValue<xiiInt32>());
      if (bytes.GetStorageSize64() < xiiMath::MaxValue<xiiInt32>())
        pModel = xiiMuJoCoCompiledModel::LoadBinary(bytes.GetContiguousMemoryRange(0), GetResourceID());
      else
        xiiLog::Error("MuJoCo model resource '{}' exceeds the supported MJB stream size.", GetResourceID());
    }
    else
    {
      xiiLog::Error("MuJoCo model resource '{}' has an invalid stream header.", GetResourceID());
    }
  }

  xiiResourceLoadDescription desc;
  desc.m_State                      = pModel ? xiiResourceState::Loaded : xiiResourceState::LoadedResourceMissing;
  desc.m_uiQualityLevelsDiscardable = 0;
  desc.m_uiQualityLevelsLoadable    = 0;

  XII_LOCK(m_ModelMutex);
  m_pModel = std::move(pModel);
  m_uiRevision.Increment();
  return desc;
}

void xiiMuJoCoModelResource::UpdateMemoryUsage(MemoryUsage& out_usage)
{
  XII_LOCK(m_ModelMutex);
  out_usage.m_uiMemoryCPU = sizeof(*this);
  if (m_pModel)
    out_usage.m_uiMemoryCPU += sizeof(xiiMuJoCoCompiledModel) + sizeof(mjModel) + static_cast<xiiUInt64>(m_pModel->GetModel()->nbuffer);
  out_usage.m_uiMemoryGPU = 0;
}

xiiSharedPtr<xiiMuJoCoCompiledModel> xiiMuJoCoModelResource::GetModel(xiiUInt32& out_uiRevision) const
{
  XII_LOCK(m_ModelMutex);
  out_uiRevision = m_uiRevision;
  return m_pModel;
}

XII_STATICLINK_FILE(MujocoPlugin, MujocoPlugin_Resources_Implementation_MuJoCoModelResource);
