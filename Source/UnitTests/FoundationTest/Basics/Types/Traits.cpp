/// Copyright (c) Theophilus Eriata. All Rights Reserved.

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

  static_assert(xiiGetTypeClass<AggregatePod>::value == xiiTypeIsPod::value);
  static_assert(xiiGetTypeClass<AggregatePod2>::value == xiiTypeIsPod::value);
  static_assert(xiiGetTypeClass<MemRelocateable>::value == xiiTypeIsMemRelocatable::value);
  static_assert(xiiGetTypeClass<AggregateMemRelocateable>::value == xiiTypeIsMemRelocatable::value);
  static_assert(xiiGetTypeClass<ClassType>::value == xiiTypeIsClass::value);
  static_assert(xiiGetTypeClass<AggregateClass>::value == xiiTypeIsClass::value);
} // namespace
