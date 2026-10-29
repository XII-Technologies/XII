/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Settings.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Types/ScopeExit.h>

class xiiTestSettingsChild : public xiiReflectedClass
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestSettingsChild, xiiReflectedClass);

public:
  xiiInt32 m_iValue = 2;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestSettingsChild, 1, xiiRTTIDefaultAllocator<xiiTestSettingsChild>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(new xiiClampValueAttribute(0, 10)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiTestSettings : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestSettings, xiiSettings);

public:
  xiiInt32             m_iCount = 2;
  float                m_fScale = 1.0f;
  xiiString            m_sLabel = "constructor default";
  xiiTestSettingsChild m_Child;
};

// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestSettings, 1, xiiRTTIDefaultAllocator<xiiTestSettings>)
{
  XII_BEGIN_ATTRIBUTES
  {
    new xiiSettingsScopeAttribute(xiiSettingsScope::Application),
  }
  XII_END_ATTRIBUTES;
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Count", m_iCount)->AddAttributes(new xiiDefaultValueAttribute(7), new xiiClampValueAttribute(0, 10)),
    XII_MEMBER_PROPERTY("Scale", m_fScale),
    XII_MEMBER_PROPERTY("Label", m_sLabel),
    XII_MEMBER_PROPERTY("Child", m_Child),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiTestSettingsNoAllocator : public xiiSettings
{
  XII_ADD_DYNAMIC_REFLECTION(xiiTestSettingsNoAllocator, xiiSettings);
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiTestSettingsNoAllocator, 1, xiiRTTINoAllocator)
  {
    XII_BEGIN_ATTRIBUTES
    {
      new xiiSettingsScopeAttribute(xiiSettingsScope::Application),
    } XII_END_ATTRIBUTES;
  }
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Configuration, Settings)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Concrete RTTI discovery and explicit defaults")
  {
    xiiDynamicArray<const xiiRTTI*> types;
    xiiSettings::GatherTypes(xiiSettingsScope::Application, types);
    XII_TEST_BOOL(types.Contains(xiiGetStaticRTTI<xiiTestSettings>()));
    XII_TEST_BOOL(!types.Contains(xiiGetStaticRTTI<xiiTestSettingsNoAllocator>()));
    XII_TEST_BOOL(xiiSettings::Create(xiiGetStaticRTTI<xiiTestSettingsNoAllocator>()) == nullptr);
    XII_TEST_BOOL(xiiSettings::Create(xiiGetStaticRTTI<xiiSettings>()) == nullptr);
    XII_TEST_BOOL(xiiSettings::Create(nullptr) == nullptr);
    xiiSettings::GatherTypes(xiiSettingsScope::Project, types);
    XII_TEST_BOOL(!types.Contains(xiiGetStaticRTTI<xiiTestSettings>()));
    auto* pSettings = static_cast<xiiTestSettings*>(xiiSettings::Create(xiiGetStaticRTTI<xiiTestSettings>()));
    XII_SCOPE_EXIT(pSettings->GetDynamicRTTI()->GetAllocator()->Deallocate(pSettings));
    XII_TEST_INT(pSettings->m_iCount, 7);
    XII_TEST_STRING(pSettings->m_sLabel, "constructor default");
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Validation, commit notifications and isolation")
  {
    xiiTestSettings                      live;
    xiiTestSettings                      staged;
    xiiUInt32                            notifications = 0;
    xiiEvent<xiiSettings*>::Unsubscriber subscription;
    live.m_SettingsChangedEvent.AddEventHandler([&](xiiSettings* pSettings) {
      XII_TEST_BOOL(pSettings == &live);
      ++notifications;
    },
                                                subscription);
    staged.m_iCount = 8;
    XII_TEST_INT(live.m_iCount, 2);
    XII_TEST_RESULT(live.Apply(staged));
    XII_TEST_INT(live.m_iCount, 8);
    XII_TEST_INT(notifications, 1);
    XII_TEST_RESULT(live.Apply(staged));
    XII_TEST_INT(notifications, 1);
    staged.m_iCount = -1;
    XII_TEST_BOOL(live.Apply(staged).Failed());
    XII_TEST_INT(live.m_iCount, 8);
    staged.m_iCount = 8;
    staged.m_fScale = xiiMath::NaN<float>();
    XII_TEST_BOOL(staged.Validate().Failed());
    staged.m_fScale = xiiMath::Infinity<float>();
    XII_TEST_BOOL(staged.Validate().Failed());
    XII_TEST_INT(notifications, 1);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Old DDL properties, defaults, removed properties and invalid values")
  {
    xiiTestSettings        settings;
    xiiAbstractObjectGraph graph;
    auto*                  pNode = graph.AddNode(xiiUuid::MakeUuid(), "xiiTestSettings", 1, "root");
    pNode->AddProperty("Count", 6);
    pNode->AddProperty("RemovedProperty", "ignored safely");
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    xiiMemoryStreamReader reader(&storage);
    XII_TEST_RESULT(settings.ReadFromDDL(reader));
    XII_TEST_INT(settings.m_iCount, 6);
    XII_TEST_STRING(settings.m_sLabel, "constructor default");
    pNode->ChangeProperty("Count", 99);
    storage.Clear();
    writer.SetWritePosition(0);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    reader.SetReadPosition(0);
    XII_TEST_BOOL(settings.ReadFromDDL(reader).Failed());
    XII_TEST_INT(settings.m_iCount, 6);
    graph.Clear();
    storage.Clear();
    writer.SetWritePosition(0);
    xiiAbstractGraphDdlSerializer::Write(writer, &graph);
    reader.SetReadPosition(0);
    XII_TEST_BOOL(settings.ReadFromDDL(reader).Failed());
    XII_TEST_INT(settings.m_iCount, 6);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Reflected round trip does not notify during restoration")
  {
    xiiTestSettings original;
    original.m_iCount = 9;
    original.m_sLabel = "persisted";
    xiiDefaultMemoryStreamStorage storage;
    xiiMemoryStreamWriter         writer(&storage);
    xiiReflectionSerializer::WriteObjectToDDL(writer, original.GetDynamicRTTI(), &original);
    xiiTestSettings restored;
    xiiUInt32       notifications = 0;
    restored.m_SettingsChangedEvent.AddEventHandler([&](xiiSettings*) { ++notifications; });
    xiiMemoryStreamReader reader(&storage);
    XII_TEST_RESULT(restored.ReadFromDDL(reader));
    XII_TEST_BOOL(restored.HasSameValues(original));
    XII_TEST_INT(notifications, 0);
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Malformed values and missing or unloaded nested types")
  {
    xiiTestSettings        settings;
    xiiAbstractObjectGraph graph;
    auto*                  pRoot = graph.AddNode(xiiUuid::MakeUuid(), "xiiTestSettings", 1, "root");
    pRoot->AddProperty("Count", "not an integer");
    XII_TEST_BOOL(settings.ReadFromGraph(graph).Failed());
    XII_TEST_INT(settings.m_iCount, 2);
    pRoot->ChangeProperty("Count", 6);
    const auto childGuid = xiiUuid::MakeUuid();
    pRoot->AddProperty("Child", childGuid);
    XII_TEST_BOOL(settings.ReadFromGraph(graph).Failed());
    auto* pChild = graph.AddNode(childGuid, "UnloadedSettingsPluginType", 1);
    XII_TEST_BOOL(settings.ReadFromGraph(graph).Failed());
    graph.RemoveNode(childGuid);
    pChild = graph.AddNode(childGuid, "xiiTestSettingsChild", 1);
    pChild->AddProperty("Value", 99);
    XII_TEST_BOOL(settings.ReadFromGraph(graph).Failed());
    XII_TEST_INT(settings.m_iCount, 2);
    pChild->ChangeProperty("Value", 4);
    XII_TEST_RESULT(settings.ReadFromGraph(graph));
    XII_TEST_INT(settings.m_iCount, 6);
    XII_TEST_INT(settings.m_Child.m_iValue, 4);
  }

#if XII_ENABLED(XII_SUPPORTS_DYNAMIC_PLUGINS) && XII_ENABLED(XII_COMPILE_ENGINE_AS_DLL)
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Plugin discovery before and after unload")
  {
    XII_TEST_RESULT(xiiPlugin::LoadPlugin(xiiFoundationTest_Plugin1));
    const auto* pType = xiiRTTI::FindTypeByName("xiiPluginTestSettings");
    if (XII_TEST_BOOL(pType != nullptr))
    {
      xiiDynamicArray<const xiiRTTI*> types;
      xiiSettings::GatherTypes(xiiSettingsScope::Application, types);
      XII_TEST_BOOL(types.Contains(pType));
      auto* pSettings = xiiSettings::Create(pType);
      XII_TEST_BOOL(pSettings != nullptr);
      pType->GetAllocator()->Deallocate(pSettings);
    }
    xiiPlugin::UnloadAllPlugins();
    XII_TEST_BOOL(xiiRTTI::FindTypeByName("xiiPluginTestSettings") == nullptr);
    xiiDynamicArray<const xiiRTTI*> types;
    xiiSettings::GatherTypes(xiiSettingsScope::Application, types);
    for (const auto* pRegistered : types)
      XII_TEST_BOOL(pRegistered->GetTypeName() != "xiiPluginTestSettings");
  }
#endif
}
