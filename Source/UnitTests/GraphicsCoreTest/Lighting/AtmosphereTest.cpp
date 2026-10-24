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
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereMultiScatterPass.h>
#include <GraphicsCore/Pipeline/Passes/Atmosphere/AtmosphereTransmittancePass.h>
#include <GraphicsCoreTest/Pipeline/PipelineLibraryTestUtils.h>

XII_CREATE_SIMPLE_TEST(Lighting, Atmosphere)
{
  XII_TEST_BOOL(xiiIsValidAtmosphereSettings(xiiAtmosphereSettings()));

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

  xiiAtmosphereSettings settings;
  settings.m_fMiePhaseG = 0.72f;
  XII_TEST_BOOL(xiiIsValidAtmosphereSettings(settings));
  auto invalid                  = settings;
  invalid.m_fAtmosphereRadiusKm = invalid.m_fPlanetRadiusKm;
  XII_TEST_BOOL(!xiiIsValidAtmosphereSettings(invalid));
  invalid = settings;
  invalid.m_vPlanetUpDirection.SetZero();
  XII_TEST_BOOL(!xiiIsValidAtmosphereSettings(invalid));
  invalid                                   = settings;
  invalid.m_uiTransmittanceIntegrationSteps = 1025;
  XII_TEST_BOOL(!xiiIsValidAtmosphereSettings(invalid));
  invalid                             = settings;
  invalid.m_uiMultiScatterSqrtSamples = 65;
  XII_TEST_BOOL(!xiiIsValidAtmosphereSettings(invalid));
  invalid                    = settings;
  invalid.m_vMieAbsorption.x = -0.1f;
  XII_TEST_BOOL(!xiiIsValidAtmosphereSettings(invalid));
}

XII_CREATE_SIMPLE_TEST(Pipeline, AtmosphereLookupResourceContracts)
{
  xiiAtmosphereTransmittancePass transmission;
  xiiAtmosphereMultiScatterPass  multiple;
  xiiVariantDictionary           properties;
  XII_TEST_RESULT(transmission.GetProperties(properties));
  properties.Insert("Width", 0U);
  XII_TEST_BOOL(transmission.Configure(properties).Failed());
  properties.Insert("Width", 33U);
  properties.Insert("Height", 17U);
  XII_TEST_RESULT(transmission.Configure(properties));
  properties.Clear();
  properties.Insert("Width", 4097U);
  XII_TEST_BOOL(multiple.Configure(properties).Failed());
  xiiDynamicArray<xiiRenderPipelinePin> pins;
  transmission.GetPins(pins);
  XII_TEST_RESULT(transmission.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[transmission.m_Output.GetBindingIndex()].m_Format == xiiGALResourceFormat::RGBA32Float);
  pins.Clear();
  multiple.GetPins(pins);
  XII_TEST_RESULT(multiple.InitializePinBindings(pins));
  XII_TEST_BOOL(pins[multiple.m_Transmittance.GetBindingIndex()].m_Constraints.m_RequiredBindFlags.IsSet(xiiGALBindFlags::ShaderResource));
  const auto                                 lookup = xiiCreateAtmosphereLookupTestResource(), world = xiiCreateWorldAtmosphereViewTestResource();
  xiiResourceLock<xiiRenderPipelineResource> plain(lookup, xiiResourceAcquireMode::BlockTillLoaded_NeverFail), scene(world, xiiResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (!XII_TEST_BOOL(plain.IsValid() && scene.IsValid() && plain->GetPipeline() && scene->GetPipeline()))
    return;
  XII_TEST_INT(plain->GetPipeline()->GetExecutionOrder().GetCount(), 3);
  XII_TEST_INT(scene->GetPipeline()->GetExecutionOrder().GetCount(), 4);
  XII_TEST_BOOL(plain->GetPipeline()->GetExtractionRequirements().IsNoFlagSet() && plain->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
  XII_TEST_BOOL(scene->GetPipeline()->GetExtractionRequirements() == xiiRenderExtractionCategory::Atmosphere && scene->GetPipeline()->GetRuntimeFeatures().IsNoFlagSet());
}
