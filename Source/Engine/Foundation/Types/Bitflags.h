/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Containers/Implementation/BitIterator.h>
#include <Foundation/Types/Enum.h>

/// The xiiBitflags class allows you to work with type-safe bitflags.
///
/// xiiBitflags takes a struct as its template parameter, which contains an enum for the available flag values.
/// xiiBitflags wraps this type in a way which enables the compiler to do type-checks. This makes it very easy
/// to document and enforce what flags are to be used in an interface.
/// For example, in traditional C++ code, you usually need to have an integer as a function parameter type,
/// when that parameter is supposed to take flags. However, WHICH flags (e.g. from which enum) cannot be enforced
/// through compile time checks. Thus it is difficult for the user to see whether he used the correct type, and
/// it is impossible for the compiler to help find such bugs.
/// xiiBitflags solves this problem. However the flag type used to instantiate xiiBitflags must fulfill some requirements.
///
/// There are two ways to define your bitflags type, that can be used with xiiBitflags.
///
/// The easier, less powerful way: Use the XII_DECLARE_FLAGS() macro.\n
/// Example:\n
/// \code{.cpp}
///   XII_DECLARE_FLAGS(xiiUInt8, SimpleRenderFlags, EnableEffects, EnableLighting, EnableShadows);
/// \endcode
/// This will declare a type 'SimpleRenderFlags' which contains three different flags.
/// You can then create a function which takes flags like this:
/// \code{.cpp}
///   void RenderScene(xiiBitflags<SimpleRenderFlags> Flags);
/// \endcode
/// And this function can be called like this:\n
/// \code{.cpp}
///   RenderScene(EnableEffects | EnableLighting | EnableShadows);
/// \endcode
/// However it will refuse to compile with anything else, for example this will not work:\n
/// \code{.cpp}
///   RenderScene(1);
/// \endcode
///
/// The second way to declare your bitflags type allows even more flexibility. Here you need to declare your bitflag type manually:
/// \code{.cpp}
///   struct SimpleRenderFlags
///   {
///     using StorageType = xiiUInt32;
///
///     enum Enum
///     {
///       EnableEffects   = XII_BIT(0),
///       EnableLighting  = XII_BIT(1),
///       EnableShadows   = XII_BIT(2),
///       FullLighting    = EnableLighting | EnableShadows,
///       AllFeatures     = FullLighting | EnableEffects,
///       Default = AllFeatures
///     };
///
///     struct Bits
///     {
///       StorageType EnableEffects   : 1;
///       StorageType EnableLighting  : 1;
///       StorageType EnableShadows   : 1;
///     };
///   };
///
///   XII_DECLARE_FLAGS_OPERATORS(SimpleRenderFlags);
/// \endcode
///
/// Here we declare a struct which contains our enum that contains all the flags that we want to have. This enum can contain
/// flags that are combinations of other flags. Note also the 'Default' flag, which is mandatory.\n
/// The 'Bits' struct enables debuggers to show exactly which flags are enabled (with nice names) when you inspect a xiiBitflags
/// instance. You could leave this struct empty, but then your debugger can not show helpful information about the flags anymore.
/// The Bits struct should contain one named entry for each individual bit. E.g. here only the flags 'EnableEffects', 'EnableLighting'
/// and 'EnableShadows' actually map to single bits, the other flags are combinations of those. Therefore the Bits struct only
/// specifies names for those first three Bits.\n
/// The alias 'StorageType' is also mandatory, such that xiiBitflags can access it.\n
/// Finally the macro XII_DECLARE_FLAGS_OPERATORS will define the required operator to be able to combine bitflags of your type.
/// I.e. it enables to write xiiBitflags<SimpleRenderFlags> f = EnableEffects | EnableLighting;\n
///
/// For a real world usage example, see xiiCVarFlags.
template <typename T>
struct xiiBitflags
{
private:
  using Enum        = typename T::Enum;
  using Bits        = typename T::Bits;
  using StorageType = typename T::StorageType;

public:
  using ConstIterator = xiiBitIterator<Enum, false>;

  /// Constructor. Initializes the flags to the default value.
  XII_ALWAYS_INLINE xiiBitflags() :
    m_Value(static_cast<StorageType>(T::Default)) // [tested]
  {
  }

  /// Converts the incoming type to xiiBitflags<T>
  XII_ALWAYS_INLINE xiiBitflags(Enum flag1) // [tested]
  {
    m_Value = (StorageType)flag1;
  }

  XII_ALWAYS_INLINE void operator=(Enum flag1) { m_Value = (StorageType)flag1; }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr bool operator==(const StorageType rhs) const // [tested]
  {
    return m_Value == rhs;
  }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr bool operator==(const xiiBitflags<T>& rhs) const
  {
    return m_Value == rhs.m_Value;
  }

  /// Clears all flags
  XII_ALWAYS_INLINE void Clear() // [tested]
  {
    m_Value = 0;
  }

  /// Checks if certain flags are set within the bitfield.
  XII_ALWAYS_INLINE bool IsSet(Enum flag) const // [tested]
  {
    return (m_Value & flag) != 0;
  }

  /// Returns whether all the given flags are set.
  XII_ALWAYS_INLINE bool AreAllSet(const xiiBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) == rhs.m_Value;
  }

  /// Returns whether none of the given flags is set.
  XII_ALWAYS_INLINE bool AreNoneSet(const xiiBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) == 0;
  }

  ///  Returns whether any of the given flags is set.
  XII_ALWAYS_INLINE bool IsAnySet(const xiiBitflags<T>& rhs) const // [tested]
  {
    return (m_Value & rhs.m_Value) != 0;
  }

  /// Returns whether there are strictly any of the given flags set.
  XII_ALWAYS_INLINE bool IsStrictlyAnySet(const xiiBitflags<T>& rhs) const
  {
    return ((m_Value & rhs.m_Value) != 0) && ((m_Value & ~rhs.m_Value) == 0);
  }

  /// Sets the given flag.
  XII_ALWAYS_INLINE void Add(const xiiBitflags<T>& rhs) // [tested]
  {
    m_Value |= rhs.m_Value;
  }

  /// Removes the given flag.
  XII_ALWAYS_INLINE void Remove(const xiiBitflags<T>& rhs) // [tested]
  {
    m_Value &= (~rhs.m_Value);
  }

  /// Toggles the state of the given flag.
  XII_ALWAYS_INLINE void Toggle(const xiiBitflags<T>& rhs) // [tested]
  {
    m_Value ^= rhs.m_Value;
  }

  /// Sets or clears the given flag.
  XII_ALWAYS_INLINE void AddOrRemove(const xiiBitflags<T>& rhs, bool bState) // [tested]
  {
    m_Value = (bState) ? m_Value | rhs.m_Value : m_Value & (~rhs.m_Value);
  }

  /// Returns an object that has the flags of \a this and \a rhs combined.
  XII_ALWAYS_INLINE xiiBitflags<T> operator|(const xiiBitflags<T>& rhs) const // [tested]
  {
    return xiiBitflags<T>(m_Value | rhs.m_Value);
  }

  /// Returns an object that has the flags that were set both in \a this and \a rhs.
  XII_ALWAYS_INLINE xiiBitflags<T> operator&(const xiiBitflags<T>& rhs) const // [tested]
  {
    return xiiBitflags<T>(m_Value & rhs.m_Value);
  }

  /// Modifies \a this to also contain the bits from \a rhs.
  XII_ALWAYS_INLINE void operator|=(const xiiBitflags<T>& rhs) // [tested]
  {
    m_Value |= rhs.m_Value;
  }

  /// Modifies \a this to only contain the bits that were set in \a this and \a rhs.
  XII_ALWAYS_INLINE void operator&=(const xiiBitflags<T>& rhs) // [tested]
  {
    m_Value &= rhs.m_Value;
  }

  /// Returns the stored value as the underlying integer type.
  XII_ALWAYS_INLINE StorageType GetValue() const // [tested]
  {
    return m_Value;
  }

  /// Overwrites the flags with a new value.
  XII_ALWAYS_INLINE void SetValue(StorageType value) // [tested]
  {
    m_Value = value;
  }

  /// Returns true if not a single bit is set.
  XII_ALWAYS_INLINE bool IsNoFlagSet() const // [tested]
  {
    return m_Value == 0;
  }

  /// Returns true if any bitflag is set.
  XII_ALWAYS_INLINE bool IsAnyFlagSet() const // [tested]
  {
    return m_Value != 0;
  }

  /// Returns a constant iterator to the very first set bit.
  /// Note that due to the way iterating through bits is accelerated, changes to the bitflags will not affect the iterator after creation.
  XII_ALWAYS_INLINE ConstIterator GetIterator() const // [tested]
  {
    return ConstIterator((Enum)m_Value);
  }

  /// Returns an invalid iterator. Needed to support range based for loops.
  XII_ALWAYS_INLINE ConstIterator GetEndIterator() const // [tested]
  {
    return ConstIterator();
  }

private:
  XII_ALWAYS_INLINE explicit xiiBitflags(StorageType flags) :
    m_Value(flags)
  {
  }

  union
  {
    StorageType m_Value;
    Bits        m_bits;
  };
};

//////////////////////////////////////////////////////////////////////////
// begin() and end() for range-based for-loop support
template <typename T>
typename xiiBitflags<T>::ConstIterator begin(const xiiBitflags<T>& container)
{
  return container.GetIterator();
}

template <typename T>
typename xiiBitflags<T>::ConstIterator cbegin(const xiiBitflags<T>& container)
{
  return container.GetIterator();
}

template <typename T>
typename xiiBitflags<T>::ConstIterator end(const xiiBitflags<T>& container)
{
  return container.GetEndIterator();
}

template <typename T>
typename xiiBitflags<T>::ConstIterator cend(const xiiBitflags<T>& container)
{
  return container.GetEndIterator();
}

/// This macro will define the operator| and operator& function that is required for class \a FlagsType to work with xiiBitflags.
/// See class xiiBitflags for more information.
#define XII_DECLARE_FLAGS_OPERATORS(FlagsType)                                      \
  inline xiiBitflags<FlagsType> operator|(FlagsType::Enum lhs, FlagsType::Enum rhs) \
  {                                                                                 \
    return (xiiBitflags<FlagsType>(lhs) | xiiBitflags<FlagsType>(rhs));             \
  }                                                                                 \
                                                                                    \
  inline xiiBitflags<FlagsType> operator&(FlagsType::Enum lhs, FlagsType::Enum rhs) \
  {                                                                                 \
    return (xiiBitflags<FlagsType>(lhs) & xiiBitflags<FlagsType>(rhs));             \
  }



/// This macro allows to conveniently declare a bitflag type that can be used with the xiiBitflags class.
///
/// Usage: XII_DECLARE_FLAGS(xiiUInt32, FlagsTypeName, Flag1Name, Flag2Name, Flag3Name, Flag4Name, ...)
///
/// This macro will define a simple type of with the name that is given as the second parameter,
/// which can be used as type-safe bitflags. Everything that is necessary to work with the xiiBitflags
/// class, will be set up automatically.
/// The bitflag type will use the integer type that is given as the first parameter for its internal
/// storage. So if you pass xiiUInt32 as the first parameter, your bitflag type will take up 4 bytes
/// and will support up to 32 flags. You can also pass any other integer type to adjust the required
/// storage space, if you don't need that many flags.
///
/// The third parameter and onwards declare the names of the flags that the type should contain.
/// Each flag will use a different bit. If you need to define flags that are combinations of several
/// other flags, you need to declare the bitflag struct manually. See the xiiBitflags class for more
/// information on how to do that.
#define XII_DECLARE_FLAGS_WITH_DEFAULT(InternalStorageType, BitflagsTypeName, DefaultValue, ...) \
  struct BitflagsTypeName                                                                        \
  {                                                                                              \
    static constexpr xiiUInt32 Count = XII_VA_NUM_ARGS(__VA_ARGS__);                             \
    using StorageType                = InternalStorageType;                                      \
    enum Enum : StorageType                                                                      \
    {                                                                                            \
      XII_EXPAND_ARGS_WITH_INDEX(XII_DECLARE_FLAGS_ENUM, ##__VA_ARGS__) Default = DefaultValue   \
    };                                                                                           \
    struct Bits                                                                                  \
    {                                                                                            \
      XII_EXPAND_ARGS(XII_DECLARE_FLAGS_BITS, ##__VA_ARGS__)                                     \
    };                                                                                           \
    XII_ENUM_TO_STRING(__VA_ARGS__)                                                              \
  };                                                                                             \
  XII_DECLARE_FLAGS_OPERATORS(BitflagsTypeName)

#define XII_DECLARE_FLAGS(InternalStorageType, BitflagsTypeName, ...) \
  XII_DECLARE_FLAGS_WITH_DEFAULT(InternalStorageType, BitflagsTypeName, 0, ##__VA_ARGS__)
/// \cond

/// Internal Do not use.
#define XII_DECLARE_FLAGS_ENUM(name, n) name = XII_BIT(n),

/// Internal Do not use.
#define XII_DECLARE_FLAGS_BITS(name) StorageType name : 1;

/// \endcond
