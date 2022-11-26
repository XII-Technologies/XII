#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Time.h>

#include <vector>

namespace
{
  enum constants
  {
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
    NUM_SAMPLES           = 128,
    NUM_APPENDS           = 1024 * 32,
    NUM_RECUSRIVE_APPENDS = 128
#else
    NUM_SAMPLES           = 1024,
    NUM_APPENDS           = 1024 * 64,
    NUM_RECUSRIVE_APPENDS = 256
#endif
  };

  struct SomeBigObject
  {
    XII_DECLARE_MEM_RELOCATABLE_TYPE();

    static xiiUInt32 constructionCount;
    static xiiUInt32 destructionCount;
    xiiUInt64        i1, i2, i3, i4, i5, i6, i7, i8;

    SomeBigObject(xiiUInt64 init) :
      i1(init), i2(init), i3(init), i4(init), i5(init), i6(init), i7(init), i8(init)
    {
      constructionCount++;
    }

    ~SomeBigObject() { destructionCount++; }

    SomeBigObject(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }

    void operator=(const SomeBigObject& rh)
    {
      constructionCount++;
      this->i1 = rh.i1;
      this->i2 = rh.i2;
      this->i3 = rh.i3;
      this->i4 = rh.i4;
      this->i5 = rh.i5;
      this->i6 = rh.i6;
      this->i7 = rh.i7;
      this->i8 = rh.i8;
    }
  };

  xiiUInt32 SomeBigObject::constructionCount = 0;
  xiiUInt32 SomeBigObject::destructionCount  = 0;
} // namespace

// Enable when needed
#define XII_PERFORMANCE_TESTS_STATE xiiTestBlock::DisabledNoWarning

XII_CREATE_SIMPLE_TEST(Performance, Container)
{
  const char*     TestString       = "There are 10 types of people in the world. Those who understand binary and those who don't.";
  const xiiUInt32 TestStringLength = (xiiUInt32)strlen(TestString);

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "POD Dynamic Array Appending")
  {
    xiiTime   t0  = xiiTime::Now();
    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      xiiDynamicArray<int> a;
      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(i);
      }

      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]POD Dynamic Array Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "POD std::vector Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<int> a;
      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(i);
      }

      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        sum += a[i];
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]POD std::vector Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "xiiDynamicArray<xiiDynamicArray<char>> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      xiiDynamicArray<xiiDynamicArray<char>> a;
      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        xiiUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        xiiDynamicArray<char>& cur = a[count];
        for (xiiUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info(
      "[test]xiiDynamicArray<xiiDynamicArray<char>> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "xiiDynamicArray<xiiHybridArray<char, 64>> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      xiiDynamicArray<xiiHybridArray<char, 64>> a;
      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        xiiUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        xiiHybridArray<char, 64>& cur = a[count];
        for (xiiUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.PushBack(TestString[j]);
        }
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetCount();
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]xiiDynamicArray<xiiHybridArray<char, 64>> Appending {0}ms",
                 xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "std::vector<std::vector<char>> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::vector<char>> a;
      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        xiiUInt32 count = (xiiUInt32)a.size();
        a.resize(count + 1);
        std::vector<char>& cur = a[count];
        for (xiiUInt32 j = 0; j < TestStringLength; j++)
        {
          cur.push_back(TestString[j]);
        }
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (xiiUInt32)a[i].size();
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info(
      "[test]std::vector<std::vector<char>> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "xiiDynamicArray<xiiString> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      xiiDynamicArray<xiiString> a;
      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        xiiUInt32 count = a.GetCount();
        a.SetCount(count + 1);
        xiiString&       cur = a[count];
        xiiStringBuilder b;
        for (xiiUInt32 j = 0; j < TestStringLength; j++)
        {
          b.Append(TestString[i]);
        }
        cur = std::move(b);
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += a[i].GetElementCount();
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]xiiDynamicArray<xiiString> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "std::vector<std::string> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<std::string> a;
      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        std::string cur;
        for (xiiUInt32 j = 0; j < TestStringLength; j++)
        {
          cur += TestString[i];
        }
        a.push_back(std::move(cur));
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (xiiUInt32)a[i].length();
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]std::vector<std::string> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "xiiDynamicArray<SomeBigObject> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      xiiDynamicArray<SomeBigObject> a;
      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.PushBack(SomeBigObject(i));
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (xiiUInt32)a[i].i1;
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info(
      "[test]xiiDynamicArray<SomeBigObject> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(XII_PERFORMANCE_TESTS_STATE, "std::vector<SomeBigObject> Appending")
  {
    xiiTime t0 = xiiTime::Now();

    xiiUInt32 sum = 0;
    for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
    {
      std::vector<SomeBigObject> a;
      for (xiiUInt32 i = 0; i < NUM_APPENDS; i++)
      {
        a.push_back(SomeBigObject(i));
      }

      for (xiiUInt32 i = 0; i < NUM_RECUSRIVE_APPENDS; i++)
      {
        sum += (xiiUInt32)a[i].i1;
      }
    }

    xiiTime t1 = xiiTime::Now();
    xiiLog::Info("[test]std::vector<SomeBigObject> Appending {0}ms", xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
  }

  XII_TEST_BLOCK(xiiTestBlock::DisabledNoWarning, "xiiMap<void*, xiiUInt32>")
  {
    xiiUInt32 sum = 0;



    for (xiiUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      xiiMap<void*, xiiUInt32> map;

      for (xiiUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      xiiTime t0 = xiiTime::Now();
      for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
      {
        for (xiiUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (xiiUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        auto last = map.GetLastIterator();
        for (auto it = map.GetIterator(); it != last; ++it)
        {
          sum += it.Value();
        }
      }
      xiiTime t1 = xiiTime::Now();

      auto last = map.GetLastIterator();
      for (auto it = map.GetIterator(); it != last; ++it)
      {
        free(it.Key());
      }
      xiiLog::Info(
        "[test]xiiMap<void*, xiiUInt32> size = {0} => {1}ms", size, xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }

  XII_TEST_BLOCK(xiiTestBlock::DisabledNoWarning, "xiiHashTable<void*, xiiUInt32>")
  {
    xiiUInt32 sum = 0;



    for (xiiUInt32 size = 1024; size < 4096 * 32; size += 1024)
    {
      xiiHashTable<void*, xiiUInt32> map;

      for (xiiUInt32 i = 0; i < size; i++)
      {
        map.Insert(malloc(64), 64);
      }

      void* ptrs[1024];

      xiiTime t0 = xiiTime::Now();
      for (xiiUInt32 n = 0; n < NUM_SAMPLES; n++)
      {

        for (xiiUInt32 i = 0; i < 1024; i++)
        {
          void* mem = malloc(64);
          map.Insert(mem, 64);
          map.Remove(mem);
          ptrs[i] = mem;
        }

        for (xiiUInt32 i = 0; i < 1024; i++)
          free(ptrs[i]);

        for (auto it = map.GetIterator(); it.IsValid(); it.Next())
        {
          sum += it.Value();
        }
      }
      xiiTime t1 = xiiTime::Now();

      for (auto it = map.GetIterator(); it.IsValid(); it.Next())
      {
        free(it.Key());
      }

      xiiLog::Info("[test]xiiHashTable<void*, xiiUInt32> size = {0} => {1}ms", size,
                   xiiArgF((t1 - t0).GetMilliseconds() / static_cast<double>(NUM_SAMPLES), 4), sum);
    }
  }
}
