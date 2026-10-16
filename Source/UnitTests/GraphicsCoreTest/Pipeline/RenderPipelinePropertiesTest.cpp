/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsCoreTest/GraphicsCoreTestPCH.h>

#include <Foundation/IO/MemoryStream.h>
#include <GraphicsCore/Pipeline/RenderPipelineInstance.h>

struct xiiPipelinePropertyInline
{
  float   m_fThreshold = 0.25f;
  xiiVec2 m_Scale      = xiiVec2(1.0f);
};
XII_DECLARE_REFLECTABLE_TYPE(XII_NO_LINKAGE, xiiPipelinePropertyInline);
XII_BEGIN_STATIC_REFLECTED_TYPE(xiiPipelinePropertyInline, xiiNoBase, 1, xiiRTTIDefaultAllocator<xiiPipelinePropertyInline>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Threshold", m_fThreshold),
      XII_MEMBER_PROPERTY("Scale", m_Scale),
    } XII_END_PROPERTIES;
  }
XII_END_STATIC_REFLECTED_TYPE;

class xiiPipelinePropertySettings : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelinePropertySettings, xiiReflectedClass);

public:
  static xiiUInt32 s_uiLive;
  xiiPipelinePropertySettings() { ++s_uiLive; }
  ~xiiPipelinePropertySettings()
  {
    XII_DEFAULT_DELETE(m_pChild);
    --s_uiLive;
  }
  float                        m_fScale = 1.0f;
  xiiString                    m_sLabel;
  xiiPipelinePropertySettings* m_pChild = nullptr;
};
xiiUInt32 xiiPipelinePropertySettings::s_uiLive = 0;
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelinePropertySettings, 1, xiiRTTIDefaultAllocator<xiiPipelinePropertySettings>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Scale", m_fScale),
      XII_MEMBER_PROPERTY("Label", m_sLabel),
      XII_MEMBER_PROPERTY("Child", m_pChild)->AddFlags(xiiPropertyFlags::PointerOwner),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelinePropertyDerivedSettings : public xiiPipelinePropertySettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelinePropertyDerivedSettings, xiiPipelinePropertySettings);

public:
  bool m_bEnabled = true;
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelinePropertyDerivedSettings, 1, xiiRTTIDefaultAllocator<xiiPipelinePropertyDerivedSettings>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_MEMBER_PROPERTY("Enabled", m_bEnabled),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

class xiiPipelinePropertyPass : public xiiRenderPipelinePass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiPipelinePropertyPass, xiiRenderPipelinePass);

public:
  ~xiiPipelinePropertyPass()
  {
    for (auto* settings : m_Settings)
      XII_DEFAULT_DELETE(settings);
  }
  xiiDynamicArray<xiiUInt32>                    m_Samples;
  xiiMap<xiiString, float>                      m_Weights;
  xiiDynamicArray<xiiPipelinePropertySettings*> m_Settings;
  xiiPipelinePropertyInline                     m_Inline;
  xiiDynamicArray<xiiPipelinePropertyInline>    m_Ranges;
  xiiEnum<xiiGALResourceFormat>                 m_Format = xiiGALResourceFormat::RGBA16Float;
  xiiBitflags<xiiGALBindFlags>                  m_Access = xiiGALBindFlags::ShaderResource;
  xiiUInt32                                     GetReadOnlyValue() const { return 7; }
  void                                          GetPins(xiiDynamicArray<xiiRenderPipelinePin>&) const override {}
  xiiResult                                     Build(xiiRenderPipelinePassContext&) override { return XII_SUCCESS; }
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiPipelinePropertyPass, 1, xiiRTTIDefaultAllocator<xiiPipelinePropertyPass>)
  {
    XII_BEGIN_PROPERTIES
    {
      XII_ARRAY_MEMBER_PROPERTY("Samples", m_Samples),
      XII_MAP_MEMBER_PROPERTY("Weights", m_Weights),
      XII_ARRAY_MEMBER_PROPERTY("Settings", m_Settings)->AddFlags(xiiPropertyFlags::PointerOwner),
      XII_MEMBER_PROPERTY("Inline", m_Inline),
      XII_ARRAY_MEMBER_PROPERTY("Ranges", m_Ranges),
      XII_ENUM_MEMBER_PROPERTY("Format", xiiGALResourceFormat, m_Format),
      XII_BITFLAGS_MEMBER_PROPERTY("Access", xiiGALBindFlags, m_Access),
      XII_ACCESSOR_PROPERTY_READ_ONLY("ReadOnlyValue", GetReadOnlyValue),
    } XII_END_PROPERTIES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Pipeline, ReflectedPassProperties)
{
  XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, 0);
  {
    xiiPipelinePropertyPass original;
    original.m_Samples.PushBack(2);
    original.m_Samples.PushBack(4);
    original.m_Weights.Insert("Near", 0.25f);
    original.m_Weights.Insert("Far", 0.75f);
    original.m_Inline.m_fThreshold = 0.75f;
    original.m_Ranges.PushBack(original.m_Inline);
    xiiPipelinePropertyDerivedSettings* settings = XII_DEFAULT_NEW(xiiPipelinePropertyDerivedSettings);
    settings->m_sLabel                           = "Persistent settings";
    settings->m_fScale                           = 0.5f;
    settings->m_bEnabled                         = false;
    settings->m_pChild                           = XII_DEFAULT_NEW(xiiPipelinePropertySettings);
    settings->m_pChild->m_sLabel                 = "Nested settings";
    original.m_Settings.PushBack(settings);
    original.m_Settings.PushBack(nullptr);
    original.m_ParameterOverrides.Insert("Occurrence", 12U);

    xiiVariantDictionary properties;
    XII_TEST_RESULT(original.GetProperties(properties));
    XII_TEST_BOOL(!properties.Contains("ReadOnlyValue") && !properties.Contains("ParameterOverrides"));
    XII_TEST_INT(properties["Settings"].Get<xiiVariantArray>().GetCount(), 2);
    xiiPipelinePropertyPass restored;
    XII_TEST_RESULT(restored.Configure(properties));
    XII_TEST_BOOL(restored.m_Samples == original.m_Samples);
    XII_TEST_BOOL(restored.m_Weights == original.m_Weights);
    XII_TEST_BOOL(restored.m_Format == original.m_Format && restored.m_Access == original.m_Access);
    XII_TEST_FLOAT(restored.m_Inline.m_fThreshold, 0.75f, 0);
    if (XII_TEST_INT(restored.m_Ranges.GetCount(), 1))
      XII_TEST_FLOAT(restored.m_Ranges[0].m_fThreshold, 0.75f, 0);
    if (!XII_TEST_INT(restored.m_Settings.GetCount(), 2))
      return;
    XII_TEST_BOOL(restored.m_Settings[0] != settings && restored.m_Settings[1] == nullptr);
    const auto* derived = xiiDynamicCast<xiiPipelinePropertyDerivedSettings*>(restored.m_Settings[0]);
    if (XII_TEST_BOOL(derived != nullptr))
      XII_TEST_BOOL(!derived->m_bEnabled);
    XII_TEST_STRING(restored.m_Settings[0]->m_pChild->m_sLabel, "Nested settings");

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Invalid replacement settings release partial allocations and preserve previous owners")
    {
      auto invalid = properties["Settings"].Get<xiiVariantArray>();
      auto fields  = invalid[0].Get<xiiVariantDictionary>();
      fields.Insert("Scale", "not a number");
      invalid.PushBack(fields);
      xiiVariantDictionary replacement;
      replacement.Insert("Settings", invalid);
      const auto live     = xiiPipelinePropertySettings::s_uiLive;
      auto*      previous = restored.m_Settings[0];
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, live);
      XII_TEST_BOOL(restored.m_Settings[0] == previous);
      fields.Insert("$type", "xiiPipelinePropertyPass");
      xiiVariantArray wrongType;
      wrongType.PushBack(fields);
      replacement.Insert("Settings", wrongType);
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, live);
      replacement.Clear();
      replacement.Insert("ReadOnlyValue", 99U);
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      replacement.Clear();
      replacement.Insert("Format", "NoSuchResourceFormat");
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      replacement.Clear();
      xiiVariantArray badSamples;
      badSamples.PushBack(8U);
      badSamples.PushBack("not a sample count");
      replacement.Insert("Samples", badSamples);
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      XII_TEST_BOOL(restored.m_Samples == original.m_Samples);
      replacement.Clear();
      xiiVariantDictionary badWeights;
      badWeights.Insert("Near", "not a weight");
      replacement.Insert("Weights", badWeights);
      XII_TEST_BOOL(restored.Configure(replacement).Failed());
      XII_TEST_BOOL(restored.m_Weights == original.m_Weights);
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Empty authored containers clear constructor and previous configuration values")
    {
      xiiPipelinePropertyPass empty;
      xiiVariantDictionary    emptyProperties;
      XII_TEST_RESULT(empty.GetProperties(emptyProperties));
      XII_TEST_RESULT(restored.Configure(emptyProperties));
      XII_TEST_BOOL(restored.m_Settings.IsEmpty() && restored.m_Samples.IsEmpty() && restored.m_Weights.IsEmpty());
    }

    XII_TEST_BLOCK(xiiTestBlock::Enabled, "Compiled resource serialization and instances retain structured properties")
    {
      xiiRenderPipelineResourceDescriptor definition;
      definition.AddPass("Structured", "xiiPipelinePropertyPass", properties);
      xiiDefaultMemoryStreamStorage storage;
      xiiMemoryStreamWriter         writer(&storage);
      XII_TEST_RESULT(definition.Serialize(writer));
      xiiMemoryStreamReader               reader(&storage);
      xiiRenderPipelineResourceDescriptor decoded;
      XII_TEST_RESULT(decoded.Deserialize(reader));
      XII_TEST_BOOL(decoded.m_Passes[0].m_Properties == properties);
      xiiSharedPtr<const xiiRenderPipeline> compiled;
      XII_TEST_RESULT(xiiRenderPipeline::Compile(decoded, compiled));
      xiiRenderPipelineInstance first, second;
      XII_TEST_RESULT(first.Initialize(compiled));
      XII_TEST_RESULT(second.Initialize(compiled));
      auto* a = static_cast<const xiiPipelinePropertyPass*>(first.GetPass(0));
      auto* b = static_cast<const xiiPipelinePropertyPass*>(second.GetPass(0));
      if (!XII_TEST_BOOL(a && b && a->m_Settings.GetCount() == 2 && b->m_Settings.GetCount() == 2))
        return;
      XII_TEST_BOOL(a->m_Settings[0] != b->m_Settings[0]);
      a->m_Settings[0]->m_fScale = 0.125f;
      XII_TEST_FLOAT(b->m_Settings[0]->m_fScale, 0.5f, 0);
      xiiVariantDictionary captured;
      XII_TEST_RESULT(b->GetProperties(captured));
      XII_TEST_BOOL(captured == properties);
    }
  }
  XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, 0);
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Excessively nested owned properties fail with bounded allocation and traversal")
  {
    xiiPipelinePropertyPass      nested;
    xiiPipelinePropertySettings* root = XII_DEFAULT_NEW(xiiPipelinePropertySettings);
    nested.m_Settings.PushBack(root);
    auto* tail = root;
    for (xiiUInt32 i = 0; i < 40; ++i)
    {
      tail->m_pChild = XII_DEFAULT_NEW(xiiPipelinePropertySettings);
      tail           = tail->m_pChild;
    }
    xiiVariantDictionary properties;
    properties.Insert("Sentinel", true);
    XII_TEST_BOOL(nested.GetProperties(properties).Failed());
    XII_TEST_BOOL(properties["Sentinel"].Get<bool>());
    xiiVariantDictionary fields;
    for (xiiUInt32 i = 0; i < 40; ++i)
    {
      xiiVariantDictionary parent;
      parent.Insert("Child", fields);
      fields = std::move(parent);
    }
    properties.Clear();
    xiiVariantArray settings;
    settings.PushBack(fields);
    properties.Insert("Settings", settings);
    const auto live = xiiPipelinePropertySettings::s_uiLive;
    XII_TEST_BOOL(nested.Configure(properties).Failed());
    XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, live);
    XII_TEST_BOOL(nested.m_Settings[0] == root);
  }
  XII_TEST_INT(xiiPipelinePropertySettings::s_uiLive, 0);
}
