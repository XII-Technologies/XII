#include <FoundationTest/FoundationTestPCH.h>

// This test does not actually run, it tests compile time stuff

namespace
{
  struct AggregatePod
  {
    int   m_1;
    float m_2;

    XII_DETECT_TYPE_CLASS(int, float);
  };

  struct AggregatePod2
  {
    int          m_1;
    float        m_2;
    AggregatePod m_3;

    XII_DETECT_TYPE_CLASS(int, float, AggregatePod);
  };

  struct MemRelocateable
  {
    XII_DECLARE_MEM_RELOCATABLE_TYPE();
  };

  struct AggregateMemRelocateable
  {
    int             m_1;
    float           m_2;
    AggregatePod    m_3;
    MemRelocateable m_4;

    XII_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable);
  };

  class ClassType
  {
  };

  struct AggregateClass
  {
    int             m_1;
    float           m_2;
    AggregatePod    m_3;
    MemRelocateable m_4;
    ClassType       m_5;

    XII_DETECT_TYPE_CLASS(int, float, AggregatePod, MemRelocateable, ClassType);
  };

  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<AggregatePod>::value == xiiTypeIsPod::value);
  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<AggregatePod2>::value == xiiTypeIsPod::value);
  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<MemRelocateable>::value == xiiTypeIsMemRelocatable::value);
  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<AggregateMemRelocateable>::value == xiiTypeIsMemRelocatable::value);
  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<ClassType>::value == xiiTypeIsClass::value);
  XII_CHECK_AT_COMPILETIME(xiiGetTypeClass<AggregateClass>::value == xiiTypeIsClass::value);
} // namespace
