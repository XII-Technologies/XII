/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>
#include <TestFramework/Framework/Declarations.h>
#include <TestFramework/Framework/TestBaseClass.h>

class XII_TEST_DLL xiiSimpleTestGroup : public xiiTestBaseClass
{
public:
  using SimpleTestFunc = void (*)();

  xiiSimpleTestGroup(const char* szName) :
    m_szTestName(szName)
  {
  }

  void AddSimpleTest(const char* szName, SimpleTestFunc testFunc);

  virtual const char* GetTestName() const override { return m_szTestName; }

private:
  virtual void          SetupSubTests() override;
  virtual xiiTestAppRun RunSubTest(xiiInt32 iIdentifier, xiiUInt32 uiInvocationCount) override;
  virtual xiiResult     InitializeSubTest(xiiInt32 iIdentifier) override;
  virtual xiiResult     DeInitializeSubTest(xiiInt32 iIdentifier) override;

private:
  struct SimpleTestEntry
  {
    const char*    m_szName;
    SimpleTestFunc m_Func;
  };

  const char*                 m_szTestName;
  std::deque<SimpleTestEntry> m_SimpleTests;
};

class XII_TEST_DLL xiiRegisterSimpleTestHelper : public xiiEnumerable<xiiRegisterSimpleTestHelper>
{
  XII_DECLARE_ENUMERABLE_CLASS(xiiRegisterSimpleTestHelper);

public:
  xiiRegisterSimpleTestHelper(xiiSimpleTestGroup* pTestGroup, const char* szTestName, xiiSimpleTestGroup::SimpleTestFunc func)
  {
    m_pTestGroup = pTestGroup;
    m_szTestName = szTestName;
    m_Func       = func;
  }

  void RegisterTest() { m_pTestGroup->AddSimpleTest(m_szTestName, m_Func); }

private:
  xiiSimpleTestGroup*                m_pTestGroup;
  const char*                        m_szTestName;
  xiiSimpleTestGroup::SimpleTestFunc m_Func;
};

#define XII_CREATE_SIMPLE_TEST_GROUP(GroupName) xiiSimpleTestGroup XII_PP_CONCAT(g_SimpleTestGroup__, GroupName)(XII_PP_STRINGIFY(GroupName))

#define XII_CREATE_SIMPLE_TEST(GroupName, TestName)                                                                               \
  extern xiiSimpleTestGroup   XII_PP_CONCAT(g_SimpleTestGroup__, GroupName);                                                      \
  static void                 xiiSimpleTestFunction__##GroupName##_##TestName();                                                  \
  xiiRegisterSimpleTestHelper xiiRegisterSimpleTest__##GroupName##TestName(                                                       \
    &XII_PP_CONCAT(g_SimpleTestGroup__, GroupName), XII_PP_STRINGIFY(TestName), xiiSimpleTestFunction__##GroupName##_##TestName); \
  static void xiiSimpleTestFunction__##GroupName##_##TestName()
