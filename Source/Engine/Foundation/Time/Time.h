#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Configuration/StaticSubSystem.h>

/// \brief The time class encapsulates a double value storing the time in seconds.
///
/// It offers convenient functions to get the time in other units.
/// xiiTime is a high-precision time using the OS specific high-precision timing functions
/// and may thus be used for profiling as well as simulation code.
struct XII_FOUNDATION_DLL xiiTime
{
public:
  /// \brief Gets the current time
  static xiiTime Now(); // [tested]

  /// \brief Creates an instance of xiiTime that was initialized from nanoseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromNanoseconds(double fNanoseconds) { return xiiTime(fNanoseconds * 0.000000001); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Nanoseconds(double fNanoseconds) { return xiiTime(fNanoseconds * 0.000000001); }

  /// \brief Creates an instance of xiiTime that was initialized from microseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMicroseconds(double fMicroseconds) { return xiiTime(fMicroseconds * 0.000001); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Microseconds(double fMicroseconds) { return xiiTime(fMicroseconds * 0.000001); }

  /// \brief Creates an instance of xiiTime that was initialized from milliseconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMilliseconds(double fMilliseconds) { return xiiTime(fMilliseconds * 0.001); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Milliseconds(double fMilliseconds) { return xiiTime(fMilliseconds * 0.001); }

  /// \brief Creates an instance of xiiTime that was initialized from seconds.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromSeconds(double fSeconds) { return xiiTime(fSeconds); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Seconds(double fSeconds) { return xiiTime(fSeconds); }

  /// \brief Creates an instance of xiiTime that was initialized from minutes.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromMinutes(double fMinutes) { return xiiTime(fMinutes * 60); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Minutes(double fMinutes) { return xiiTime(fMinutes * 60); }

  /// \brief Creates an instance of xiiTime that was initialized from hours.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeFromHours(double fHours) { return xiiTime(fHours * 60 * 60); }
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime Hours(double fHours) { return xiiTime(fHours * 60 * 60); }

  /// \brief Creates an instance of xiiTime that was initialized with zero.
  [[nodiscard]] XII_ALWAYS_INLINE constexpr static xiiTime MakeZero() { return xiiTime(0.0); }

  XII_DECLARE_POD_TYPE();

  /// \brief The default constructor sets the time to zero.
  XII_ALWAYS_INLINE constexpr xiiTime() = default;

  /// \brief Returns true if the stored time is exactly zero. That typically means the value was not changed from the default.
  XII_ALWAYS_INLINE constexpr bool IsZero() const { return m_fTime == 0.0; }

  /// \brief Checks for a negative time value.
  XII_ALWAYS_INLINE constexpr bool IsNegative() const { return m_fTime < 0.0; }

  /// \brief Checks for a positive time value. This does not include zero.
  XII_ALWAYS_INLINE constexpr bool IsPositive() const { return m_fTime > 0.0; }

  /// \brief Returns true if the stored time is zero or negative.
  XII_ALWAYS_INLINE constexpr bool IsZeroOrNegative() const { return m_fTime <= 0.0; }

  /// \brief Returns true if the stored time is zero or positive.
  XII_ALWAYS_INLINE constexpr bool IsZeroOrPositive() const { return m_fTime >= 0.0; }

  /// \brief Returns the time as a float value (in seconds).
  ///
  /// Useful for simulation time steps etc.
  /// Please note that it is not recommended to use the float value for long running
  /// time calculations since the precision can deteriorate quickly. (Only use for delta times is recommended)
  constexpr float AsFloatInSeconds() const;

  /// \brief Returns the nanoseconds value
  constexpr double GetNanoseconds() const;

  /// \brief Returns the microseconds value
  constexpr double GetMicroseconds() const;

  /// \brief Returns the milliseconds value
  constexpr double GetMilliseconds() const;

  /// \brief Returns the seconds value.
  constexpr double GetSeconds() const;

  /// \brief Returns the minutes value.
  constexpr double GetMinutes() const;

  /// \brief Returns the hours value.
  constexpr double GetHours() const;

  /// \brief Subtracts the time value of "other" from this instances value.
  constexpr void operator-=(const xiiTime& other);

  /// \brief Adds the time value of "other" to this instances value.
  constexpr void operator+=(const xiiTime& other);

  /// \brief Multiplies the time by the given factor
  constexpr void operator*=(double fFactor);

  /// \brief Divides the time by the given factor
  constexpr void operator/=(double fFactor);

  /// \brief Returns the difference: "this instance - other"
  constexpr xiiTime operator-(const xiiTime& other) const;

  /// \brief Returns the sum: "this instance + other"
  constexpr xiiTime operator+(const xiiTime& other) const;

  constexpr xiiTime operator-() const;

  constexpr bool operator==(const xiiTime& rhs) const { return m_fTime == rhs.m_fTime; }

  constexpr std::partial_ordering operator<=>(const xiiTime& rhs) const { return m_fTime <=> rhs.m_fTime; }

private:
  /// \brief For internal use only.
  constexpr explicit xiiTime(double fTime);

  /// \brief The time is stored in seconds
  double m_fTime = 0.0;

private:
  XII_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, Time);

  static void Initialize();
};

constexpr xiiTime operator*(xiiTime t, double f);
constexpr xiiTime operator*(double f, xiiTime t);
constexpr xiiTime operator*(xiiTime f, xiiTime t); // not physically correct, but useful (should result in seconds squared)

constexpr xiiTime operator/(xiiTime t, double f);
constexpr xiiTime operator/(double f, xiiTime t);
constexpr xiiTime operator/(xiiTime f, xiiTime t); // not physically correct, but useful (should result in a value without a unit)


#include <Foundation/Time/Implementation/Time_inl.h>
