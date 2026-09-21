/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/StaticSubSystem.h>

/// The time class encapsulates a double value storing the time in seconds.
///
/// It offers convenient functions to get the time in other units.
/// xiiTime is a high-precision time using the OS specific high-precision timing functions
/// and may thus be used for profiling as well as simulation code.
struct XII_FOUNDATION_DLL xiiTime
{
public:
  /// Gets the current time
  static xiiTime Now(); // [tested]

  /// Creates an instance of xiiTime that was initialized from nanoseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromNanoseconds(double fNanoseconds) { return xiiTime(fNanoseconds * 0.000000001); }

  /// Creates an instance of xiiTime that was initialized from microseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMicroseconds(double fMicroseconds) { return xiiTime(fMicroseconds * 0.000001); }

  /// Creates an instance of xiiTime that was initialized from milliseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMilliseconds(double fMilliseconds) { return xiiTime(fMilliseconds * 0.001); }

  /// Creates an instance of xiiTime that was initialized from seconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromSeconds(double fSeconds) { return xiiTime(fSeconds); }

  /// Creates an instance of xiiTime that was initialized from minutes.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMinutes(double fMinutes) { return xiiTime(fMinutes * 60); }

  /// Creates an instance of xiiTime that was initialized from hours.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromHours(double fHours) { return xiiTime(fHours * 60 * 60); }

  /// Creates an instance of xiiTime that was initialized with zero.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeZero() { return xiiTime(0.0); }

  XII_DECLARE_POD_TYPE();

  /// The default constructor sets the time to zero.
  XII_ALWAYS_INLINE constexpr xiiTime() = default;

  /// Returns true if the stored time is exactly zero. That typically means the value was not changed from the default.
  XII_ALWAYS_INLINE constexpr bool IsZero() const { return m_fTime == 0.0; }

  /// Checks for a negative time value.
  XII_ALWAYS_INLINE constexpr bool IsNegative() const { return m_fTime < 0.0; }

  /// Checks for a positive time value. This does not include zero.
  XII_ALWAYS_INLINE constexpr bool IsPositive() const { return m_fTime > 0.0; }

  /// Returns true if the stored time is zero or negative.
  XII_ALWAYS_INLINE constexpr bool IsZeroOrNegative() const { return m_fTime <= 0.0; }

  /// Returns true if the stored time is zero or positive.
  XII_ALWAYS_INLINE constexpr bool IsZeroOrPositive() const { return m_fTime >= 0.0; }

  /// Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  constexpr float AsFloatInSeconds() const;

  /// Returns the nanoseconds value
  constexpr double GetNanoseconds() const;

  /// Returns the microseconds value
  constexpr double GetMicroseconds() const;

  /// Returns the milliseconds value
  constexpr double GetMilliseconds() const;

  /// Returns the seconds value.
  constexpr double GetSeconds() const;

  /// Returns the minutes value.
  constexpr double GetMinutes() const;

  /// Returns the hours value.
  constexpr double GetHours() const;

  /// Subtracts the time value of "other" from this instances value.
  constexpr void operator-=(const xiiTime& other);

  /// Adds the time value of "other" to this instances value.
  constexpr void operator+=(const xiiTime& other);

  /// Multiplies the time by the given factor
  constexpr void operator*=(double fFactor);

  /// Divides the time by the given factor
  constexpr void operator/=(double fFactor);

  /// Returns the difference: "this instance - other"
  constexpr xiiTime operator-(const xiiTime& other) const;

  /// Returns the sum: "this instance + other"
  constexpr xiiTime operator+(const xiiTime& other) const;

  constexpr xiiTime operator-() const;

  constexpr bool operator==(const xiiTime& rhs) const { return m_fTime == rhs.m_fTime; }

  constexpr std::partial_ordering operator<=>(const xiiTime& rhs) const { return m_fTime <=> rhs.m_fTime; }

private:
  /// For internal use only.
  constexpr explicit xiiTime(double fTime);

  /// The time is stored in seconds
  double m_fTime = 0.0;

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

constexpr xiiTime operator*(const xiiTime& t, double f);
constexpr xiiTime operator*(double f, const xiiTime& t);
constexpr xiiTime operator*(const xiiTime& f, const xiiTime& t); // not physically correct, but useful (should result in seconds squared)

constexpr xiiTime operator/(const xiiTime& t, double f);
constexpr xiiTime operator/(double f, const xiiTime& t);
constexpr xiiTime operator/(const xiiTime& f, const xiiTime& t); // not physically correct, but useful (should result in a value without a unit)


#include <Foundation/Time/Implementation/Time_inl.h>
