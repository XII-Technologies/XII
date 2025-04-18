#pragma once

#include <TestFramework/TestFrameworkDLL.h>

#include <Foundation/Algorithm/HashingUtils.h>

struct xiiConstructionCounter
{
  /// Dummy m_iData, such that one can test the constructor with initialization
  xiiInt32 m_iData;
  bool     m_valid;

  /// Default Constructor
  xiiConstructionCounter() :
    m_iData(0), m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Constructor with initialization
  xiiConstructionCounter(xiiInt32 d) :
    m_iData(d), m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Copy Constructor
  xiiConstructionCounter(const xiiConstructionCounter& cc) :
    m_iData(cc.m_iData), m_valid(true)
  {
    ++s_iConstructions;
  }

  /// Move construction counts as a construction as well.
  xiiConstructionCounter(xiiConstructionCounter&& cc) noexcept
    :
    m_iData(cc.m_iData), m_valid(true)
  {
    cc.m_iData = 0; // data has been moved, so "destroy" it.
    ++s_iConstructions;
  }

  /// Destructor
  ~xiiConstructionCounter()
  {
    XII_ASSERT_ALWAYS(m_valid, "Destroying object twice");
    m_valid = false;
    ++s_iDestructions;
  }

  /// Assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const xiiConstructionCounter& cc) { m_iData = cc.m_iData; }
  /// Move assignment does not change the construction counter, because it is only executed on already constructed objects.
  void operator=(const xiiConstructionCounter&& cc) noexcept { m_iData = cc.m_iData; }

  bool operator==(const xiiConstructionCounter& cc) const { return m_iData == cc.m_iData; }

  bool operator<(const xiiConstructionCounter& rhs) const { return m_iData < rhs.m_iData; }

  /// Checks whether n constructions have been done since the last check.
  static bool HasConstructed(xiiInt32 iCons)
  {
    const bool b         = s_iConstructions == s_iConstructionsLast + iCons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n destructions have been done since the last check.
  static bool HasDestructed(xiiInt32 iCons)
  {
    const bool b         = s_iDestructions == s_iDestructionsLast + iCons;
    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    if (!b)
      PrintStats();

    return (b);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(xiiInt32 iCons, xiiInt32 iDes)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + iCons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + iDes));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    return (bc && bd);
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast, s_iDestructions,
           s_iDestructions - s_iDestructionsLast);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions     = 0;
    s_iConstructionsLast = 0;
    s_iDestructions      = 0;
    s_iDestructionsLast  = 0;
  }

  static xiiInt32 s_iConstructions;
  static xiiInt32 s_iConstructionsLast;
  static xiiInt32 s_iDestructions;
  static xiiInt32 s_iDestructionsLast;
};

struct xiiConstructionCounterRelocatable
{
  XII_DECLARE_MEM_RELOCATABLE_TYPE();

  /// Dummy m_iData, such that one can test the constructor with initialization
  xiiInt32 m_iData;

  /// Bool to track if the element was default constructed or received valid data.
  bool m_valid = false;

  xiiConstructionCounterRelocatable() = default;

  xiiConstructionCounterRelocatable(xiiInt32 d) :
    m_iData(d), m_valid(true)
  {
    s_iConstructions++;
  }

  xiiConstructionCounterRelocatable(const xiiConstructionCounterRelocatable& other) = delete;

  xiiConstructionCounterRelocatable(xiiConstructionCounterRelocatable&& other) noexcept
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
  }

  ~xiiConstructionCounterRelocatable()
  {
    if (m_valid)
      s_iDestructions++;
  }

  void operator=(xiiConstructionCounterRelocatable&& other) noexcept
  {
    m_iData = other.m_iData;
    m_valid = other.m_valid;

    other.m_valid = false;
    ;
  }

  /// For debugging and getting tests right: Prints out the current number of constructions and destructions
  static void PrintStats()
  {
    printf("Constructions: %d (New: %i), Destructions: %d (New: %i) \n", s_iConstructions, s_iConstructions - s_iConstructionsLast, s_iDestructions,
           s_iDestructions - s_iDestructionsLast);
  }

  /// Checks whether n constructions and destructions have been done since the last check.
  static bool HasDone(xiiInt32 iCons, xiiInt32 iDes)
  {
    const bool bc = (s_iConstructions == (s_iConstructionsLast + iCons));
    const bool bd = (s_iDestructions == (s_iDestructionsLast + iDes));

    if (!(bc && bd))
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    return (bc && bd);
  }

  /// Checks that all instances have been destructed.
  static bool HasAllDestructed()
  {
    if (s_iConstructions != s_iDestructions)
      PrintStats();

    s_iConstructionsLast = s_iConstructions;
    s_iDestructionsLast  = s_iDestructions;

    return (s_iConstructions == s_iDestructions);
  }

  static void Reset()
  {
    s_iConstructions     = 0;
    s_iConstructionsLast = 0;
    s_iDestructions      = 0;
    s_iDestructionsLast  = 0;
  }

  static xiiInt32 s_iConstructions;
  static xiiInt32 s_iConstructionsLast;
  static xiiInt32 s_iDestructions;
  static xiiInt32 s_iDestructionsLast;
};

template <>
struct xiiHashHelper<xiiConstructionCounter>
{
  static xiiUInt32 Hash(const xiiConstructionCounter& value) { return xiiHashHelper<xiiInt32>::Hash(value.m_iData); }

  XII_ALWAYS_INLINE static bool Equal(const xiiConstructionCounter& a, const xiiConstructionCounter& b) { return a == b; }
};
