#pragma once

#include <Foundation/Math/Declarations.h>

/// \brief Wrapper class for a safe usage and conversions of angles.
///
/// Uses radian internally. Will <b>not</b> automatically keep its range between 0 degree - 360 degree (0 - 2PI) but you can call NormalizeRange to do
/// so.
template <typename Type>
class xiiAngleTemplate
{
public:
  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static XII_ALWAYS_INLINE Type DegToRadMultiplier(); // [tested]

  /// \brief Returns the constant to multiply with an angle in degree to convert it to radians.
  constexpr static XII_ALWAYS_INLINE Type RadToDegMultiplier(); // [tested]

  /// \brief Converts an angle in degree to radians.
  constexpr static Type DegToRad(Type f); // [tested]

  /// \brief Converts an angle in radians to degree.
  constexpr static Type RadToDeg(Type f); // [tested]

  /// \brief Creates an instance of xiiAngleTemplate that was initialized from degree. (Performs a conversion)
  constexpr static xiiAngleTemplate<Type> Degree(Type fDegree); // [tested]

  /// \brief Creates an instance of xiiAngleTemplate that was initialized from radian. (No need for any conversion)
  constexpr static xiiAngleTemplate<Type> Radian(Type fRadian); // [tested]

public:
  XII_DECLARE_POD_TYPE();

  /// \brief Standard constructor, initializing with 0.
  constexpr xiiAngleTemplate() :
    m_fRadian(static_cast<Type>(0))
  {
  } // [tested]

  /// \brief Returns the degree value. (Performs a conversion)
  constexpr Type GetDegree() const; // [tested]

  /// \brief Returns the radian value. (No need for any conversion)
  constexpr Type GetRadian() const; // [tested]

  /// \brief Sets the radian value. (No need for any conversion)
  XII_ALWAYS_INLINE void SetRadian(Type rad) { m_fRadian = rad; };

  /// \brief Brings the angle into the range of 0 degree - 360 degree
  /// \see GetNormalizedRange()
  void NormalizeRange(); // [tested]

  /// \brief Returns an equivalent angle with range between 0 degree - 360 degree
  /// \see NormalizeRange()
  xiiAngleTemplate<Type> GetNormalizedRange() const; // [tested]

  /// \brief Computes the smallest angle between the two given angles. The angle will always be a positive value.
  /// \note The two angles must be in the same range. E.g. they should be either normalized or at least the absolute angle between them should not be
  /// more than 180 degree.
  constexpr static xiiAngleTemplate<Type> AngleBetween(xiiAngleTemplate<Type> a, xiiAngleTemplate<Type> b); // [tested]

  /// \brief Equality check with epsilon. Simple check without normalization. 360 degree will equal 0 degree, but 720 will not.
  bool IsEqualSimple(xiiAngleTemplate<Type> rhs, xiiAngleTemplate<Type> epsilon) const; // [tested]

  /// \brief Equality check with epsilon that uses normalized angles. Will recognize 720 degree == 0 degree.
  bool IsEqualNormalized(xiiAngleTemplate<Type> rhs, xiiAngleTemplate<Type> epsilon) const; // [tested]

  // Unary Operators
  constexpr xiiAngleTemplate<Type> operator-() const; // [tested]

  // Arithmetic Operators
  constexpr xiiAngleTemplate<Type> operator+(xiiAngleTemplate<Type> r) const; // [tested]
  constexpr xiiAngleTemplate<Type> operator-(xiiAngleTemplate<Type> r) const; // [tested]

  // Compound Assignment Operators
  void operator+=(xiiAngleTemplate<Type> r); // [tested]
  void operator-=(xiiAngleTemplate<Type> r); // [tested]

  // Comparison
  constexpr bool operator==(const xiiAngleTemplate<Type>& r) const; // [tested]

  // At least the < operator is implement to make clamping etc. work
  constexpr bool operator<(const xiiAngleTemplate<Type>& r) const;
  constexpr bool operator>(const xiiAngleTemplate<Type>& r) const;
  constexpr bool operator<=(const xiiAngleTemplate<Type>& r) const;
  constexpr bool operator>=(const xiiAngleTemplate<Type>& r) const;

  // Note: relational operators on angles are not really possible - is 0 degree smaller or bigger than 359 degree?

private:
  /// \brief For internal use only.
  constexpr explicit xiiAngleTemplate(Type fRadian) :
    m_fRadian(fRadian)
  {
  }

  /// The xiiRadian value
  Type m_fRadian;

  /// Preventing an include circle by defining pi again (annoying, but unlikely to change ;)). Normally you should use xiiMath::Pi<Type>()
  constexpr static Type Pi();
};

// Mathematical operators

/// \brief Returns f times angle a.
template <typename Type>
constexpr xiiAngleTemplate<Type> operator*(xiiAngleTemplate<Type> a, Type f); // [tested]
/// \brief Returns f times angle a.
template <typename Type>
constexpr xiiAngleTemplate<Type> operator*(Type f, xiiAngleTemplate<Type> a); // [tested]

/// \brief Returns the angle a divided by f.
template <typename Type>
constexpr xiiAngleTemplate<Type> operator/(xiiAngleTemplate<Type> a, Type f); // [tested]
/// \brief Returns the fraction of angle a divided by angle b.
template <typename Type>
constexpr Type operator/(xiiAngleTemplate<Type> a, xiiAngleTemplate<Type> b); // [tested]


#include <Foundation/Math/Implementation/Angle_inl.h>
