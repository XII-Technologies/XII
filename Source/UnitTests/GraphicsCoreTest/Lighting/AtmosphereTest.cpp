/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/GraphVersioning.h>
#include <GraphicsCore/Components/Fog/VolumetricCloudComponent.h>
#include <GraphicsCore/Components/Lights/SkyAtmosphereComponent.h>
#include <GraphicsCore/Lighting/Atmosphere.h>

XII_CREATE_SIMPLE_TEST(Lighting, Atmosphere)
{
  XII_TEST_BOOL(xiiAtmosphereManager::IsInitialized());

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Serialized zero-default settings are migrated without resetting authored values")
  {
    for (bool binary : {false, true})
    {
      xiiAbstractObjectGraph      source;
      const auto*                 type = xiiGetStaticRTTI<xiiAtmosphereSettings>();
      const xiiAtmosphereSettings defaults;
      auto*                       zero     = source.AddNode(xiiUuid::MakeUuid(), type->GetTypeName(), 3, "ZeroDefaults");
      auto*                       authored = source.AddNode(xiiUuid::MakeUuid(), type->GetTypeName(), 3, "Authored");
      auto*                       partial  = source.AddNode(xiiUuid::MakeUuid(), type->GetTypeName(), 3, "Partial");
      auto*                       missing  = source.AddNode(xiiUuid::MakeUuid(), type->GetTypeName(), 3, "Missing");
      for (const auto* property : type->GetProperties())
      {
        const auto empty = xiiReflectionUtils::GetDefaultVariantFromType(property->GetSpecificType());
        zero->AddProperty(property->GetPropertyName(), empty);
        partial->AddProperty(property->GetPropertyName(), empty);
        if (property->GetPropertyName() != "MiePhaseG")
          missing->AddProperty(property->GetPropertyName(), empty);
        authored->AddProperty(property->GetPropertyName(), xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), &defaults));
      }
      zero->AddProperty("AuthorData", "Preserve unknown data");
      authored->ChangeProperty("MiePhaseG", 0.72f);
      partial->ChangeProperty("RayleighScattering", xiiVec3(0.01f, 0.02f, 0.03f));
      const auto             zeroId = zero->GetGuid();
      xiiAbstractObjectGraph types;
      auto*                  version = types.AddNode(xiiUuid::MakeUuid(), "xiiReflectedTypeDescriptor", 1);
      version->AddProperty("TypeName", type->GetTypeName());
      version->AddProperty("ParentTypeName", "");
      version->AddProperty("TypeVersion", xiiUInt32(3));
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      if (binary)
        xiiAbstractGraphBinarySerializer::Write(writer, &source, &types);
      else
        xiiAbstractGraphDdlSerializer::Write(writer, &source, &types);
      xiiAbstractObjectGraph loaded, loadedTypes;
      xiiMemoryStreamReader  reader(&storage);
      if (binary)
        xiiAbstractGraphBinarySerializer::Read(reader, &loaded, &loadedTypes);
      else
      {
        XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(reader, &loaded, &loadedTypes, false));
      }
      // The type table schema is current; only the settings instances need patching.
      xiiGraphVersioning::GetSingleton()->PatchGraph(&loaded, &loadedTypes);
      const auto* migrated = loaded.GetNodeByName("ZeroDefaults");
      if (!XII_TEST_BOOL(migrated != nullptr))
        return;
      XII_TEST_BOOL(migrated->GetGuid() == zeroId);
      XII_TEST_INT(migrated->GetTypeVersion(), 4);
      XII_TEST_STRING(migrated->FindProperty("AuthorData")->m_Value.Get<xiiString>(), "Preserve unknown data");
      for (const auto* property : type->GetProperties())
      {
        const auto expected = xiiReflectionUtils::GetMemberPropertyValue(static_cast<const xiiAbstractMemberProperty*>(property), &defaults);
        XII_TEST_BOOL_MSG(migrated->FindProperty(property->GetPropertyName())->m_Value == expected, "Migrated default differs from native %s", property->GetPropertyName().GetStartPointer());
        const auto* original = source.GetNodeByName("Authored")->FindProperty(property->GetPropertyName());
        XII_TEST_BOOL(loaded.GetNodeByName("Authored")->FindProperty(property->GetPropertyName())->m_Value == original->m_Value);
        original = source.GetNodeByName("Partial")->FindProperty(property->GetPropertyName());
        XII_TEST_BOOL(loaded.GetNodeByName("Partial")->FindProperty(property->GetPropertyName())->m_Value == original->m_Value);
      }
      XII_TEST_BOOL(loaded.GetNodeByName("Missing")->FindProperty("MiePhaseG") == nullptr);
      XII_TEST_FLOAT(loaded.GetNodeByName("Missing")->FindProperty("PlanetRadiusKm")->m_Value.Get<float>(), 0.0f, 0.0f);

      // A newly authored all-zero record at the current schema version must not be
      // reset. The serialized type table, as in real documents, owns that version.
      version->ChangeProperty("TypeVersion", xiiUInt32(4));
      storage.Clear();
      xiiMemoryStreamWriter currentWriter(&storage);
      if (binary)
        xiiAbstractGraphBinarySerializer::Write(currentWriter, &source, &types);
      else
        xiiAbstractGraphDdlSerializer::Write(currentWriter, &source, &types);
      xiiAbstractObjectGraph current, currentTypes;
      xiiMemoryStreamReader  currentReader(&storage);
      if (binary)
        xiiAbstractGraphBinarySerializer::Read(currentReader, &current, &currentTypes);
      else
      {
        XII_TEST_RESULT(xiiAbstractGraphDdlSerializer::Read(currentReader, &current, &currentTypes, false));
      }
      xiiGraphVersioning::GetSingleton()->PatchGraph(&current, &currentTypes);
      for (const auto* property : type->GetProperties())
        XII_TEST_BOOL(current.GetNodeByName("ZeroDefaults")->FindProperty(property->GetPropertyName())->m_Value == source.GetNodeByName("ZeroDefaults")->FindProperty(property->GetPropertyName())->m_Value);
    }
  }

  const xiiRTTI* pComponentType = xiiGetStaticRTTI<xiiSkyAtmosphereComponent>();
  XII_TEST_BOOL(pComponentType->FindPropertyByName("Atmosphere") != nullptr);
  XII_TEST_BOOL(pComponentType->FindPropertyByName("Priority") != nullptr);

  const xiiRTTI* pCloudComponentType = xiiGetStaticRTTI<xiiVolumetricCloudComponent>();
  XII_TEST_BOOL(pCloudComponentType->FindPropertyByName("Cloud") != nullptr);
  XII_TEST_BOOL(pCloudComponentType->FindPropertyByName("Priority") != nullptr);

  const xiiAtmosphereSettings original           = xiiAtmosphereManager::GetConfiguration();
  const xiiUInt64             uiOriginalRevision = xiiAtmosphereManager::GetConfigurationRevision();

  xiiAtmosphereSettings modified = original;
  modified.m_fMiePhaseG          = 0.72f;
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(modified).Succeeded());
  XII_TEST_BOOL(xiiAtmosphereManager::GetConfigurationRevision() > uiOriginalRevision);
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending());

  const xiiUInt64 uiModifiedRevision = xiiAtmosphereManager::GetConfigurationRevision();
  xiiAtmosphereManager::MarkLUTsGenerated(uiModifiedRevision - 1U);
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending());
  xiiAtmosphereManager::MarkLUTsGenerated(uiModifiedRevision);
  XII_TEST_BOOL(!xiiAtmosphereManager::IsGenerationPending());

  xiiAtmosphereSettings invalid = modified;
  invalid.m_fAtmosphereRadiusKm = invalid.m_fPlanetRadiusKm;
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(invalid).Failed());
  XII_TEST_INT(xiiAtmosphereManager::GetConfigurationRevision(), uiModifiedRevision);

  invalid                      = modified;
  invalid.m_vPlanetUpDirection = xiiVec3::MakeZero();
  XII_TEST_BOOL(xiiAtmosphereManager::Configure(invalid).Failed());
  XII_TEST_INT(xiiAtmosphereManager::GetConfigurationRevision(), uiModifiedRevision);

  xiiAtmosphereSettings second = modified;
  second.m_fMiePhaseG          = 0.71f;
  xiiAtmosphereLUTHandle hModified;
  xiiAtmosphereLUTHandle hSecond;
  XII_TEST_BOOL(xiiAtmosphereManager::AcquireLUTs(modified, hModified).Succeeded());
  XII_TEST_BOOL(xiiAtmosphereManager::AcquireLUTs(second, hSecond).Succeeded());
  XII_TEST_BOOL(!hModified.IsInvalidated());
  XII_TEST_BOOL(!hSecond.IsInvalidated());
  XII_TEST_BOOL(!(hModified == hSecond));
  xiiAtmosphereManager::MarkLUTsGenerated(hModified);
  XII_TEST_BOOL(!xiiAtmosphereManager::IsGenerationPending(hModified));
  XII_TEST_BOOL(xiiAtmosphereManager::IsGenerationPending(hSecond));

  XII_TEST_BOOL(xiiAtmosphereManager::Configure(original).Succeeded());
}
