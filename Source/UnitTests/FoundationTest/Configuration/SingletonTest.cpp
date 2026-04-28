/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Singleton.h>

class TestSingleton
{
  XII_DECLARE_SINGLETON(TestSingleton);

public:
  TestSingleton() :
    m_SingletonRegistrar(this)
  {
  }

  xiiInt32 m_iValue = 41;
};

XII_IMPLEMENT_SINGLETON(TestSingleton);

class SingletonInterface
{
public:
  virtual xiiInt32 GetValue() = 0;
};

class TestSingletonOfInterface : public SingletonInterface
{
  XII_DECLARE_SINGLETON_OF_INTERFACE(TestSingletonOfInterface, SingletonInterface);

public:
  TestSingletonOfInterface() :
    m_SingletonRegistrar(this)
  {
  }

  virtual xiiInt32 GetValue() { return 23; }
};

XII_IMPLEMENT_SINGLETON(TestSingletonOfInterface);


XII_CREATE_SIMPLE_TEST(Configuration, Singleton)
{
  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Singleton Registration")
  {
    {
      TestSingleton* pSingleton = xiiSingletonRegistry::GetSingletonInstance<TestSingleton>();
      XII_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingleton g_Singleton;

      {
        TestSingleton* pSingleton = xiiSingletonRegistry::GetSingletonInstance<TestSingleton>();
        XII_TEST_BOOL(pSingleton == &g_Singleton);
        XII_TEST_INT(pSingleton->m_iValue, 41);
      }
    }

    {
      TestSingleton* pSingleton = xiiSingletonRegistry::GetSingletonInstance<TestSingleton>();
      XII_TEST_BOOL(pSingleton == nullptr);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::Enabled, "Singleton of Interface")
  {
    {
      SingletonInterface* pSingleton = xiiSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      XII_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface g_Singleton;

      {
        SingletonInterface* pSingleton = xiiSingletonRegistry::GetSingletonInstance<SingletonInterface>();
        XII_TEST_BOOL(pSingleton == &g_Singleton);
        XII_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = xiiSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
        XII_TEST_BOOL(pSingleton == &g_Singleton);
        XII_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        SingletonInterface* pSingleton = xiiSingletonRegistry::GetRequiredSingletonInstance<SingletonInterface>();
        XII_TEST_BOOL(pSingleton == &g_Singleton);
        XII_TEST_INT(pSingleton->GetValue(), 23);
      }

      {
        TestSingletonOfInterface* pSingleton = xiiSingletonRegistry::GetRequiredSingletonInstance<TestSingletonOfInterface>();
        XII_TEST_BOOL(pSingleton == &g_Singleton);
        XII_TEST_INT(pSingleton->GetValue(), 23);
      }
    }

    {
      SingletonInterface* pSingleton = xiiSingletonRegistry::GetSingletonInstance<SingletonInterface>();
      XII_TEST_BOOL(pSingleton == nullptr);
    }

    {
      TestSingletonOfInterface* pSingleton = xiiSingletonRegistry::GetSingletonInstance<TestSingletonOfInterface>();
      XII_TEST_BOOL(pSingleton == nullptr);
    }
  }
}
