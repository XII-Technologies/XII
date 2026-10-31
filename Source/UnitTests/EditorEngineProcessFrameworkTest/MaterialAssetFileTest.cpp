/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <Core/ResourceManager/ResourceManager.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GraphicsCore/Material/MaterialAssetFile.h>
#include <GraphicsCore/Material/MaterialResource.h>
#include <GraphicsCore/Textures/Texture2DResource.h>
#include <GraphicsFoundationTest/TestingEnvironment/TestingEnvironment.h>
#include <TestFramework/Utilities/TestLogInterface.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialAssetFile)
{
  xiiMaterialResourceDescriptor d;
  d.m_Domain       = xiiMaterialDomain::Decal;
  d.m_ShadingModel = xiiMaterialShadingModel::ClearCoat;
  d.m_BlendMode    = xiiMaterialBlendMode::Masked;
  d.m_AlphaMode    = xiiMaterialAlphaMode::Mask;
  d.m_FeatureFlags.Add(xiiMaterialFeatureFlags::TwoSided | xiiMaterialFeatureFlags::VertexColor);
  d.m_BaseColor           = xiiColor(0.2f, 0.4f, 0.6f, 0.8f);
  d.m_EmissiveColor       = xiiColor(2, 4, 6, 1);
  d.m_fMetallic           = 0.35f;
  d.m_fRoughness          = 0.2f;
  d.m_fOcclusionStrength  = 0.7f;
  d.m_fAlphaCutoff        = 0.3f;
  d.m_fNormalScale        = 0.4f;
  d.m_fDisplacementScale  = 0.5f;
  d.m_fClearCoat          = 0.6f;
  d.m_fClearCoatRoughness = 0.15f;
  d.m_fTransmission       = 0.25f;
  d.m_fThickness          = 0.45f;
  d.m_fIndexOfRefraction  = 1.6f;
  d.m_fAnisotropy         = -0.3f;
  d.m_fSheenRoughness     = 0.65f;
  d.m_iSortPriority       = -125;
  xiiDefaultMemoryStreamStorage storage;
  xiiMemoryStreamWriter         writer(&storage);
  if (!XII_TEST_RESULT(xiiMaterialAssetFile::Write(writer, d)))
    return;
  // Current version byte, empty base/surface/shader and zero custom counts precede the typed block.
  constexpr xiiUInt32           offset    = 1 + 3 * 4 + 4 * 2;
  const auto                    bytes     = storage.GetContiguousMemoryRange(offset);
  const auto                    blockSize = bytes.GetCount();
  xiiRawMemoryStreamReader      reader(bytes.GetPtr(), blockSize);
  xiiMaterialResourceDescriptor restored;
  XII_TEST_RESULT(xiiMaterialAssetFile::ReadPbrBlock(reader, restored));
  XII_TEST_BOOL(restored == d);

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Truncation and invalid typed values leave the destination unchanged")
  {
    const auto original = restored;
    for (xiiUInt32 length = 0; length < blockSize; ++length)
    {
      xiiRawMemoryStreamReader shortReader(bytes.GetPtr(), length);
      XII_TEST_BOOL(xiiMaterialAssetFile::ReadPbrBlock(shortReader, restored).Failed());
      XII_TEST_BOOL(restored == original);
    }
    xiiDynamicArray<xiiUInt8> corrupt;
    corrupt.PushBackRange(bytes);
    corrupt[0] = 255;
    xiiRawMemoryStreamReader invalidEnum(corrupt.GetData(), blockSize);
    XII_TEST_BOOL(xiiMaterialAssetFile::ReadPbrBlock(invalidEnum, restored).Failed());
    XII_TEST_BOOL(restored == original);
    d.m_fMetallic = xiiMath::NaN<float>();
    XII_TEST_BOOL(xiiMaterialAssetFile::Write(writer, d).Failed());
    d.m_fMetallic = 0.35f;
    class FailingWriter : public xiiStreamWriter
    {
      xiiResult WriteBytes(const void*, xiiUInt64) override { return XII_FAILURE; }
    } failed;
    XII_TEST_BOOL(xiiMaterialAssetFile::Write(failed, d).Failed());
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Native resource loading, custom parameters and reload")
  {
    xiiGPUTestingEnvironment environment;
    if (!XII_TEST_RESULT(environment.Initialize()))
      return;
    xiiGALDevice::SetDefaultDevice(environment.GetDeviceShared());
    xiiStartup::StartupHighLevelSystems();
    XII_SCOPE_EXIT(environment.GetDevice()->WaitIdle(); xiiStartup::ShutdownHighLevelSystems(); xiiGALDevice::SetDefaultDevice(nullptr); environment.Shutdown());
    for (auto type : {xiiMaterialResource::DefaultMaterialType::Missing, xiiMaterialResource::DefaultMaterialType::Loading, xiiMaterialResource::DefaultMaterialType::Highlight, xiiMaterialResource::DefaultMaterialType::Preview})
    {
      const auto handle = xiiMaterialResource::GetDefaultMaterial(type);
      XII_TEST_BOOL(handle == xiiMaterialResource::GetDefaultMaterial(type));
      xiiResourceLock<xiiMaterialResource> builtin(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(builtin && !builtin->GetCurrentDescription().m_hShader.IsValid());
      XII_TEST_BOOL(builtin->GetDefaultInstance() && builtin->GetSchema()->IsValid());
    }
    for (xiiUInt8 obsolete : {4, 5, 6, 7, 8})
    {
      xiiStringBuilder path;
      path.SetFormat(":imgout/RemovedMaterialVersion{}.xiiBinMaterial", obsolete);
      {
        xiiFileWriter file;
        if (!XII_TEST_RESULT(file.Open(path)))
          return;
        xiiAssetFileHeader header;
        XII_TEST_RESULT(header.Write(file));
        file << obsolete;
      }
      xiiTestLogInterface   log;
      xiiTestLogSystemScope logScope(&log, true);
      log.ExpectMessage("requires current native material content", xiiLogMsgType::ErrorMsg);
      log.ExpectMessage("Missing Resource of Type 'xiiMaterialResource'", xiiLogMsgType::SeriousWarningMsg);
      const auto                           handle = xiiResourceManager::LoadResource<xiiMaterialResource>(path);
      xiiResourceLock<xiiMaterialResource> rejected(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      XII_TEST_BOOL(rejected.GetAcquireResult() != xiiResourceAcquireResult::Final);
    }
    // Named resources retain their IDs without requiring a texture file to load yet.
    d.m_hBaseColorTexture = xiiResourceManager::LoadResource<xiiTexture2DResource>("Textures/Native-骨.xiiBinTexture2D");
    auto& parameter       = d.m_Parameters.ExpandAndGetRef();
    parameter.m_Name.Assign("CustomGain");
    parameter.m_Value     = 3.25f;
    const char* path      = ":imgout/NativeMaterial/Paint.xiiBinMaterial";
    auto        writeFile = [&]() {
      xiiFileWriter file;
      XII_SUCCEED_OR_RETURN(file.Open(path));
      xiiAssetFileHeader header;
      header.SetFileHashAndVersion(12345, 9);
      XII_SUCCEED_OR_RETURN(header.Write(file));
      return xiiMaterialAssetFile::Write(file, d);
    };
    XII_TEST_RESULT(writeFile());
    auto handle = xiiResourceManager::LoadResource<xiiMaterialResource>(path);
    {
      xiiResourceLock<xiiMaterialResource> material(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
      if (!XII_TEST_BOOL(material && material.GetAcquireResult() == xiiResourceAcquireResult::Final))
        return;
      XII_TEST_BOOL(material->GetDomain() == d.m_Domain && material->GetShadingModel() == d.m_ShadingModel);
      XII_TEST_BOOL(material->GetAlphaMode() == d.m_AlphaMode && material->GetBlendMode() == d.m_BlendMode);
      XII_TEST_BOOL(material->GetFeatureFlags().IsSet(xiiMaterialFeatureFlags::TwoSided));
      XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("Metallic")).ConvertTo<float>(), 0.35f, 0.0001f);
      XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("NormalScale")).ConvertTo<float>(), 0.4f, 0.0001f);
      XII_TEST_FLOAT(material->GetParameter(xiiTempHashedString("CustomGain")).ConvertTo<float>(), 3.25f, 0.0001f);
      XII_TEST_BOOL(material->GetTexture2DBinding(xiiTempHashedString("BaseColorTexture")) == d.m_hBaseColorTexture);
    }
    d.m_fMetallic = 0.75f;
    d.m_AlphaMode = xiiMaterialAlphaMode::Blend;
    d.m_BlendMode = xiiMaterialBlendMode::Translucent;
    XII_TEST_RESULT(writeFile());
    XII_TEST_BOOL(xiiResourceManager::ReloadResource(handle, true));
    xiiResourceLock<xiiMaterialResource> reloaded(handle, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (XII_TEST_BOOL(reloaded && reloaded.GetAcquireResult() == xiiResourceAcquireResult::Final))
    {
      XII_TEST_FLOAT(reloaded->GetParameter(xiiTempHashedString("Metallic")).ConvertTo<float>(), 0.75f, 0.0001f);
      XII_TEST_BOOL(reloaded->IsTranslucent());
    }
  }
}
