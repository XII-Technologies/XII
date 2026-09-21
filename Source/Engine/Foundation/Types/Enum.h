/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

/// \file

/// A custom enum implementation that allows to define the underlying storage type to control its memory footprint.
///
/// Advantages over a simple C++ enum:
/// 1) Storage type can be defined
/// 2) Enum is default initialized automatically
/// 3) Definition of the enum itself, the storage type and the default init value is in one place
/// 4) It makes function definitions shorter, instead of:
///      void function(xiiExampleEnumBase::Enum value)
///    You can write:
///      void function(xiiExampleEnum value)
/// 5) In all other ways it works exactly like a C++ enum
///
/// Example:
///
/// struct xiiExampleEnumBase
/// {
///   using StorageType = xiiUInt8;
///
///   enum Enum
///   {
///     Value1 = 1,          // normal value
///     Value2 = 2,          // normal value
///     Value3 = 3,          // normal value
///     Default = Value1 // Default initialization value (required)
///   };
/// };
/// using xiiExampleEnum = xiiEnum<xiiExampleEnumBase>;
///
/// This defines an "xiiExampleEnum" which is stored in a xiiUInt8 and is default initialized with Value1
/// For more examples see the enum test.
template <typename Derived>
struct xiiEnum : public Derived
{
public:
  using SelfType    = xiiEnum<Derived>;
  using StorageType = typename Derived::StorageType;

  /// Default constructor
  XII_ALWAYS_INLINE xiiEnum() :
    m_Value((StorageType)Derived::Default)
  {
  } // [tested]

  /// Copy constructor
  XII_ALWAYS_INLINE xiiEnum(const SelfType& rh) :
    m_Value(rh.m_Value)
  {
  }

  /// Construct from a C++ enum, and implicit conversion from enum type
  XII_ALWAYS_INLINE xiiEnum(typename Derived::Enum init) :
    m_Value((StorageType)init)
  {
  } // [tested]

  /// Assignment operator
  XII_ALWAYS_INLINE void operator=(const SelfType& rh) // [tested]
  {
    m_Value = rh.m_Value;
  }

  /// Assignment operator.
  XII_ALWAYS_INLINE void operator=(const typename Derived::Enum value) // [tested]
  {
    m_Value = (StorageType)value;
  }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr bool operator==(const SelfType& rhs) const { return m_Value == rhs.m_Value; }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr std::strong_ordering operator<=>(const SelfType& rhs) const { return m_Value <=> rhs.m_Value; }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr bool operator==(typename Derived::Enum value) const { return m_Value == (StorageType)value; }

  /// Comparison operator.
  XII_ALWAYS_INLINE constexpr std::strong_ordering operator<=>(typename Derived::Enum value) const { return m_Value <=> (StorageType)value; }

  /// brief Bitwise operators
  XII_ALWAYS_INLINE SelfType operator|(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value | (StorageType)rhs.m_Value); } // [tested]
  XII_ALWAYS_INLINE SelfType operator&(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value & (StorageType)rhs.m_Value); } // [tested]

  /// Implicit conversion to enum type.
  XII_ALWAYS_INLINE constexpr operator typename Derived::Enum() const // [tested]
  {
    return static_cast<typename Derived::Enum>(m_Value);
  }

  /// Returns the enum value as an integer
  XII_ALWAYS_INLINE StorageType GetValue() const // [tested]
  {
    return m_Value;
  }

  /// Sets the enum value through an integer
  XII_ALWAYS_INLINE void SetValue(StorageType value) // [tested]
  {
    m_Value = value;
  }

private:
  StorageType m_Value;
};


#define XII_ENUM_VALUE_TO_STRING(name) \
  case name:                           \
    return XII_PP_STRINGIFY(name);

/// Helper macro to generate a 'ToString' function for enum values.
///
/// Usage: XII_ENUM_TO_STRING(Value1, Value2, Value3, Value4)
/// Embed it into a struct (which defines the enums).
/// Example:
/// struct xiiExampleEnum
/// {
///   enum Enum
///   {
///     A,
///     B,
///     C,
///   };
///
///   XII_ENUM_TO_STRING(A, B, C);
/// };
#define XII_ENUM_TO_STRING(...)                                \
  const char* ToString(xiiUInt32 value)                        \
  {                                                            \
    switch (value)                                             \
    {                                                          \
      XII_EXPAND_ARGS(XII_ENUM_VALUE_TO_STRING, ##__VA_ARGS__) \
      default:                                                 \
        return nullptr;                                        \
    }                                                          \
  }
