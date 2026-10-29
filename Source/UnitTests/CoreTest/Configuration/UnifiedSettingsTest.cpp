/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <CoreTest/CoreTestPCH.h>

#include <Core/Configuration/PlatformProfile.h>

class xiiUnifiedProfileTestConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUnifiedProfileTestConfig, xiiProfileConfigData);

public:
  xiiInt32 m_iValue = 2;
};
// clang-format off
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUnifiedProfileTestConfig, 1, xiiRTTIDefaultAllocator<xiiUnifiedProfileTestConfig>)
{
  XII_BEGIN_PROPERTIES
  {
    XII_MEMBER_PROPERTY("Value", m_iValue)->AddAttributes(new xiiDefaultValueAttribute(7)),
  }
  XII_END_PROPERTIES;
}
XII_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class xiiUnavailableProfileTestConfig : public xiiProfileConfigData
{
  XII_ADD_DYNAMIC_REFLECTION(xiiUnavailableProfileTestConfig, xiiProfileConfigData);
};
XII_BEGIN_DYNAMIC_REFLECTED_TYPE(xiiUnavailableProfileTestConfig, 1, xiiRTTINoAllocator)
XII_END_DYNAMIC_REFLECTED_TYPE;

XII_CREATE_SIMPLE_TEST(Configuration, UnifiedProfileSettings)
{
  xiiPlatformProfile first;
  xiiPlatformProfile second;
  first.AddMissingConfigs();
  first.AddMissingConfigs();
  second.AddMissingConfigs();
  auto* pFirst  = first.GetTypeConfig<xiiUnifiedProfileTestConfig>();
  auto* pSecond = second.GetTypeConfig<xiiUnifiedProfileTestConfig>();
  XII_TEST_BOOL(pFirst != nullptr && pSecond != nullptr && pFirst != pSecond);
  XII_TEST_BOOL(pFirst->GetDynamicRTTI()->IsDerivedFrom<xiiSettings>());
  XII_TEST_INT(pFirst->m_iValue, 7);
  pFirst->m_iValue = 9;
  XII_TEST_INT(pSecond->m_iValue, 7);
  XII_TEST_BOOL(first.GetTypeConfig<xiiUnavailableProfileTestConfig>() == nullptr);
  xiiDynamicArray<const xiiRTTI*> types;
  xiiSettings::GatherTypes(xiiSettingsScope::Platform, types);
  XII_TEST_BOOL(types.Contains(xiiGetStaticRTTI<xiiUnifiedProfileTestConfig>()));
  XII_TEST_BOOL(!types.Contains(xiiGetStaticRTTI<xiiUnavailableProfileTestConfig>()));
  first.Clear();
  XII_TEST_BOOL(first.GetTypeConfig<xiiUnifiedProfileTestConfig>() == nullptr);
}
