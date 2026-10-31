/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorEngineProcessFrameworkTest/EditorEngineProcessFrameworkTestPCH.h>

#include <GraphicsCore/Material/MaterialInstance.h>

XII_CREATE_SIMPLE_TEST(EditorViews, MaterialInstanceReload)
{
  xiiMaterialSchemaDescription initial;
  initial.m_sName = "Reloadable material";
  initial.AddParameter("Tint", xiiMaterialParameterType::Color, xiiColor::White);
  initial.AddParameter("Roughness", xiiMaterialParameterType::Float, 0.5f);
  initial.AddParameter("Removed", xiiMaterialParameterType::Float, 1.0f);
  initial.AddParameter("Changed", xiiMaterialParameterType::Float, 1.0f);
  initial.AddParameter("Untouched", xiiMaterialParameterType::Float, 1.0f);
  initial.AddTexture("Normal");
  xiiSharedPtr<xiiMaterialSchema> original = XII_DEFAULT_NEW(xiiMaterialSchema);
  XII_TEST_RESULT(original->Build(initial));
  xiiMaterialInstance instance;
  XII_TEST_RESULT(instance.Initialize(original));
  XII_TEST_RESULT(instance.SetParameter("Roughness", 0.8f));
  XII_TEST_RESULT(instance.SetParameter("Removed", 2.0f));
  XII_TEST_RESULT(instance.SetParameter("Changed", 2.0f));
  // An explicit override equal to the original default must remain an override.
  XII_TEST_RESULT(instance.SetParameter("Tint", xiiColor::White));
  const auto normal = xiiMaterialParameterId::Make("Normal");
  XII_TEST_RESULT(instance.SetTexture2D(normal, {}));
  XII_TEST_RESULT(instance.SetBindlessIndex(normal, 42, 8));
  xiiMaterialInstanceSnapshot before;
  instance.ConsumeSnapshot(before);
  XII_TEST_BOOL(before.m_DirtyFlags == xiiMaterialDirtyFlags::All);

  xiiMaterialSchemaDescription updated;
  updated.m_sName = initial.m_sName;
  updated.AddParameter("Tint", xiiMaterialParameterType::Color, xiiColor::Red);
  updated.AddParameter("Roughness", xiiMaterialParameterType::Float, 0.2f);
  updated.AddParameter("Changed", xiiMaterialParameterType::Float3, xiiVec3(1));
  updated.AddParameter("Untouched", xiiMaterialParameterType::Float, 3.0f);
  updated.AddTexture("Normal");
  xiiSharedPtr<xiiMaterialSchema> replacement = XII_DEFAULT_NEW(xiiMaterialSchema);
  XII_TEST_RESULT(replacement->Build(updated));
  xiiDynamicArray<xiiString> warnings;
  XII_TEST_RESULT(instance.RebindSchema(replacement, {}, &warnings));
  XII_TEST_INT(warnings.GetCount(), 2);
  XII_TEST_FLOAT(instance.GetParameter(xiiMaterialParameterId::Make("Roughness")).Get<float>(), 0.8f, 0);
  XII_TEST_BOOL(instance.GetParameter(xiiMaterialParameterId::Make("Tint")).Get<xiiColor>() == xiiColor::White);
  XII_TEST_FLOAT(instance.GetParameter(xiiMaterialParameterId::Make("Untouched")).Get<float>(), 3, 0);
  XII_TEST_BOOL(instance.GetParameter(xiiMaterialParameterId::Make("Changed")).Get<xiiVec3>() == xiiVec3(1));
  XII_TEST_BOOL(!instance.GetParameter(xiiMaterialParameterId::Make("Removed")).IsValid());
  xiiMaterialInstanceSnapshot after;
  instance.CreateSnapshot(after);
  XII_TEST_BOOL(after.m_pSchema.Borrow() == replacement.Borrow());
  XII_TEST_BOOL(before.m_pSchema.Borrow() == original.Borrow());
  XII_TEST_INT(after.m_ResourceBindings[0].m_uiBindlessIndex, xiiInvalidIndex);
  XII_TEST_INT(after.m_ResourceBindings[0].m_uiSamplerIndex, xiiInvalidIndex);
  XII_TEST_BOOL(after.m_uiRevision > before.m_uiRevision);
  const auto revision = instance.GetRevision();
  XII_TEST_BOOL(instance.RebindSchema(nullptr, {}).Failed());
  XII_TEST_INT(instance.GetRevision(), revision);
  XII_TEST_BOOL(instance.GetSchema().Borrow() == replacement.Borrow());
  // An unchanged ordinary value produces no upload or pipeline invalidation.
  instance.ConsumeSnapshot(after);
  XII_TEST_RESULT(instance.SetParameter("Roughness", 0.8f));
  instance.CreateSnapshot(after);
  XII_TEST_BOOL(after.m_DirtyFlags == xiiMaterialDirtyFlags::None);
  XII_TEST_INT(after.m_uiRevision, revision);
  XII_TEST_RESULT(instance.SetParameter("Roughness", 0.6f));
  instance.CreateSnapshot(after);
  XII_TEST_BOOL(after.m_DirtyFlags == xiiMaterialDirtyFlags::Parameters);
}
