/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MujocoPlugin/MujocoPluginPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Threading/Lock.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <MujocoPlugin/Resources/Implementation/MuJoCoCompiledModel.h>
#include <MujocoPlugin/Resources/MuJoCoModelResource.h>

XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiMuJoCoModelResource, 1, xiiRTTIDefaultAllocator<xiiMuJoCoModelResource>)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_RESOURCE_IMPLEMENT_COMMON_CODE(xiiMuJoCoModelResource);

namespace
{
  constexpr xiiUInt32 ModelMagic       = 0x4D4A4958; // XIJM
  constexpr xiiUInt32 MaxModelBytes    = 512 * 1024 * 1024;
  constexpr int       NativeModelMagic = 54321; // MuJoCo's MJB header identifier.

  // Dispatch by stream contents because callers may use a custom resource loader.
  class ModelStreamReader final : public xiiStreamReader
  {
  public:
    explicit ModelStreamReader(xiiStreamReader& source) : m_Source(source)
    {
      m_uiRemaining = static_cast<xiiUInt32>(source.ReadBytes(m_Prefix, XII_ARRAY_SIZE(m_Prefix)));
    }

    bool IsAsset() const
    {
      return m_uiRemaining == sizeof(m_Prefix) && xiiMemoryUtils::RawByteCompare(m_Prefix, "xiiAsset", sizeof(m_Prefix)) == 0;
    }

    bool IsNativeModel() const
    {
      return m_uiRemaining >= sizeof(NativeModelMagic) && xiiMemoryUtils::RawByteCompare(m_Prefix, &NativeModelMagic, sizeof(NativeModelMagic)) == 0;
    }

    xiiUInt64 ReadBytes(void* pBuffer, xiiUInt64 uiBytes) override
    {
      const auto uiPrefixBytes = static_cast<xiiUInt32>(xiiMath::Min<xiiUInt64>(uiBytes, m_uiRemaining));
      if (uiPrefixBytes == 0)
        return m_Source.ReadBytes(pBuffer, uiBytes);
      auto* pBytes = static_cast<xiiUInt8*>(pBuffer);
      if (pBytes)
        xiiMemoryUtils::Copy(pBytes, m_Prefix + sizeof(m_Prefix) - m_uiRemaining, uiPrefixBytes);
      m_uiRemaining -= uiPrefixBytes;
      return uiPrefixBytes + (uiPrefixBytes == uiBytes ? 0 : m_Source.ReadBytes(pBytes ? pBytes + uiPrefixBytes : nullptr, uiBytes - uiPrefixBytes));
    }

  private:
    xiiStreamReader& m_Source;
    xiiUInt8         m_Prefix[8];
    xiiUInt32        m_uiRemaining = 0;
  };
} // namespace

xiiUInt64 xiiMuJoCoModelResourceDescriptor::GetCompilerVersion()
{
  // Native payloads depend on the exact MuJoCo version, scalar sizes, ABI, and byte order.
  const xiiUInt64 version[] = {mjVERSION_HEADER, sizeof(mjtNum), sizeof(mjtSize), sizeof(void*), sizeof(mjModel), sizeof(mjOption), sizeof(mjVisual), 1};
  return xiiHashingUtils::xxHash64(version, sizeof(version));
}

xiiResult xiiMuJoCoModelResourceDescriptor::Save(xiiStreamWriter& stream) const
{
  if (m_ModelData.IsEmpty() || m_ModelData.GetCount() > MaxModelBytes || mj_version() != mjVERSION_HEADER)
    return XII_FAILURE;
  stream << ModelMagic << xiiUInt8{1} << GetCompilerVersion() << m_ModelData.GetCount();
  stream << xiiHashingUtils::xxHash64(m_ModelData.GetData(), m_ModelData.GetCount());
  return stream.WriteBytes(m_ModelData.GetData(), m_ModelData.GetCount());
}

xiiResult xiiMuJoCoModelResourceDescriptor::Load(xiiStreamReader& stream)
{
  xiiUInt32 magic = 0, size = 0;
  xiiUInt8  version  = 0;
  xiiUInt64 compiler = 0, checksum = 0;
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&magic));
  if (stream.ReadBytes(&version, sizeof(version)) != sizeof(version))
    return XII_FAILURE;
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&compiler));
  XII_SUCCEED_OR_RETURN(stream.ReadDWordValue(&size));
  XII_SUCCEED_OR_RETURN(stream.ReadQWordValue(&checksum));
  if (magic != ModelMagic || version != 1 || compiler != GetCompilerVersion() || mj_version() != mjVERSION_HEADER || size == 0 || size > MaxModelBytes)
    return XII_FAILURE;
  xiiDynamicArray<xiiUInt8> bytes;
  bytes.SetCountUninitialized(size);
  if (stream.ReadBytes(bytes.GetData(), size) != size || xiiHashingUtils::xxHash64(bytes.GetData(), size) != checksum)
    return XII_FAILURE;
  m_ModelData = std::move(bytes);
  return XII_SUCCESS;
}

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
      ModelStreamReader reader(*pStream);
      if (reader.IsAsset())
      {
        xiiAssetFileHeader               header;
        xiiMuJoCoModelResourceDescriptor descriptor;
        if (header.Read(reader).Succeeded() && descriptor.Load(reader).Succeeded())
          pModel = xiiMuJoCoCompiledModel::LoadBinary(descriptor.m_ModelData, GetResourceID());
        else
          xiiLog::Error("MuJoCo model resource '{}' is invalid or incompatible. Transform it again with the current MuJoCo build.", GetResourceID());
      }
      else if (reader.IsNativeModel())
      {
        xiiContiguousMemoryStreamStorage bytes;
        bytes.ReadAll(reader, MaxModelBytes + 1);
        if (bytes.GetStorageSize64() <= MaxModelBytes)
          pModel = xiiMuJoCoCompiledModel::LoadBinary(bytes.GetContiguousMemoryRange(0), GetResourceID());
        else
          xiiLog::Error("MuJoCo model resource '{}' exceeds the supported MJB stream size.", GetResourceID());
      }
      else
        xiiLog::Error("MuJoCo model resource '{}' has an unrecognized file format.", GetResourceID());
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
