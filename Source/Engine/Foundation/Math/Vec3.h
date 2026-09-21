/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// A 3-component vector class.
template <typename Type>
class xiiVec3Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  XII_DECLARE_POD_TYPE();

  using ComponentType = Type;

  // *** Data ***
public:
  Type x, y, z;

  // *** Constructors ***
public:
  /// default-constructed vector is uninitialized (for speed)
  xiiVec3Template(); // [tested]

  /// Initializes the vector with x,y,z
  xiiVec3Template(Type x, Type y, Type z); // [tested]

  /// Initializes all 3 components with xyz
  explicit xiiVec3Template(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// Returns a vector with all components set to Not-a-Number (NaN).
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static constexpr xiiVec3Template<Type> MakeNaN() { return xiiVec3Template<Type>(xiiMath::NaN<Type>()); }

  /// Returns a vector with all components set to zero.
  [[nodiscard]] static constexpr xiiVec3Template<Type> MakeZero() { return xiiVec3Template<Type>(0); } // [tested]

  /// Returns a vector initialized to the X unit vector (1, 0, 0).
  [[nodiscard]] static constexpr xiiVec3Template<Type> MakeAxisX() { return xiiVec3Template<Type>(1, 0, 0); } // [tested]

  /// Returns a vector initialized to the Y unit vector (0, 1, 0).
  [[nodiscard]] static constexpr xiiVec3Template<Type> MakeAxisY() { return xiiVec3Template<Type>(0, 1, 0); } // [tested]

  /// Returns a vector initialized to the Z unit vector (0, 0, 1).
  [[nodiscard]] static constexpr xiiVec3Template<Type> MakeAxisZ() { return xiiVec3Template<Type>(0, 0, 1); } // [tested]

  /// Returns a vector initialized to x,y,z
  [[nodiscard]] static constexpr xiiVec3Template<Type> Make(Type x, Type y, Type z) { return xiiVec3Template<Type>(x, y, z); } // [tested]

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// Returns a xiiVec2Template with x and y from this vector.
  const xiiVec2Template<Type> GetAsVec2() const; // [tested]

  /// Returns a xiiVec4Template with x,y,z from this vector and w set to the parameter.
  const xiiVec4Template<Type> GetAsVec4(Type w) const; // [tested]

  /// Returns a xiiVec4Template with x,y,z from this vector and w set 1.
  const xiiVec4Template<Type> GetAsPositionVec4() const; // [tested]

  /// Returns a xiiVec4Template with x,y,z from this vector and w set 0.
  const xiiVec4Template<Type> GetAsDirectionVec4() const; // [tested]

  /// Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// Sets all 3 components to this value.
  void Set(Type xyz); // [tested]

  /// Sets the vector to these values.
  void Set(Type x, Type y, Type z); // [tested]

  /// Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// Returns the length of the vector.
  XII_DECLARE_IF_FLOAT_TYPE Type GetLength() const; // [tested]

  /// Tries to rescale the vector to the given length. If the vector is too close to zero, XII_FAILURE is returned and the vector is
  /// set to zero.
  XII_DECLARE_IF_FLOAT_TYPE xiiResult SetLength(Type fNewLength, Type fEpsilon = xiiMath::DefaultEpsilon<Type>()); // [tested]

  /// Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  XII_DECLARE_IF_FLOAT_TYPE Type GetLengthAndNormalize(); // [tested]

  /// Returns a normalized version of this vector, leaves the vector itself unchanged.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] const xiiVec3Template<Type> GetNormalized() const; // [tested]

  /// Normalizes this vector.
  XII_DECLARE_IF_FLOAT_TYPE void Normalize(); // [tested]

  /// Tries to normalize this vector. If the vector is too close to zero, XII_FAILURE is returned and the vector is set to the given fallback value.
  XII_DECLARE_IF_FLOAT_TYPE xiiResult NormalizeIfNotZero(const xiiVec3Template<Type>& vFallback = xiiVec3Template<Type>(1, 0, 0), Type fEpsilon = xiiMath::SmallEpsilon<Type>()); // [tested]

  /// Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  XII_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = xiiMath::HugeEpsilon<Type>()) const; // [tested]

  /// Returns true, if any of x, y or z is NaN
  bool IsNaN() const; // [tested]

  /// Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  /// Returns the distance between two this position and rhs.
  XII_DECLARE_IF_FLOAT_TYPE Type GetDistanceTo(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the squared distance between this position and rhs.
  XII_DECLARE_IF_FLOAT_TYPE Type GetSquaredDistanceTo(const xiiVec3Template<Type>& rhs) const; // [tested]

  // *** Operators ***
public:
  /// Returns the negation of this vector.
  const xiiVec3Template<Type> operator-() const; // [tested]

  /// Adds rhs component-wise to this vector
  void operator+=(const xiiVec3Template<Type>& rhs); // [tested]

  /// Subtracts rhs component-wise from this vector
  void operator-=(const xiiVec3Template<Type>& rhs); // [tested]

  /// Multiplies rhs component-wise to this vector
  void operator*=(const xiiVec3Template<Type>& rhs);

  /// Divides this vector component-wise by rhs
  void operator/=(const xiiVec3Template<Type>& rhs);

  /// Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// Equality Check (bitwise)
  bool IsIdentical(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Equality Check with epsilon
  bool IsEqual(const xiiVec3Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// Returns the positive angle between *this and rhs.
  /// Both this and rhs must be normalized
  xiiAngle GetAngleBetween(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the Dot-product of the two vectors (commutative, order does not matter)
  [[nodiscard]] Type Dot(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  [[nodiscard]] const xiiVec3Template<Type> CrossRH(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise minimum of *this and rhs
  [[nodiscard]] const xiiVec3Template<Type> CompMin(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise maximum of *this and rhs
  [[nodiscard]] const xiiVec3Template<Type> CompMax(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise clamped value of *this between low and high.
  [[nodiscard]] const xiiVec3Template<Type> CompClamp(const xiiVec3Template<Type>& vLow, const xiiVec3Template<Type>& vHigh) const; // [tested]

  /// Returns the component-wise multiplication of *this and rhs
  [[nodiscard]] const xiiVec3Template<Type> CompMul(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// Returns the component-wise division of *this and rhs
  [[nodiscard]] const xiiVec3Template<Type> CompDiv(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  [[nodiscard]] const xiiVec3Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  XII_DECLARE_IF_FLOAT_TYPE xiiResult CalculateNormal(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2, const xiiVec3Template<Type>& v3); // [tested]

  /// Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  XII_DECLARE_IF_FLOAT_TYPE void MakeOrthogonalTo(const xiiVec3Template<Type>& vNormal); // [tested]

  /// Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  XII_DECLARE_IF_FLOAT_TYPE const xiiVec3Template<Type> GetOrthogonalVector() const; // [tested]

  /// Returns this vector reflected at vNormal.
  XII_DECLARE_IF_FLOAT_TYPE const xiiVec3Template<Type> GetReflectedVector(const xiiVec3Template<Type>& vNormal) const; // [tested]

  /// Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  XII_DECLARE_IF_FLOAT_TYPE const xiiVec3Template<Type> GetRefractedVector(const xiiVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const;

  /// Returns a random point inside a unit sphere (radius 1).
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomPointInSphere(xiiRandom& inout_rng); // [tested]

  /// Creates a random direction vector. The vector is normalized.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDirection(xiiRandom& inout_rng); // [tested]

  /// Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationX(xiiRandom& inout_rng, const xiiAngle& maxDeviation); // [tested]

  /// Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationX(xiiRandom& inout_rng, const xiiAngled& maxDeviation); // [tested]

  /// Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationY(xiiRandom& inout_rng, const xiiAngle& maxDeviation); // [tested]

  /// Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationY(xiiRandom& inout_rng, const xiiAngled& maxDeviation); // [tested]

  /// Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationZ(xiiRandom& inout_rng, const xiiAngle& maxDeviation); // [tested]

  /// Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviationZ(xiiRandom& inout_rng, const xiiAngled& maxDeviation); // [tested]

  /// Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviation(xiiRandom& inout_rng, const xiiAngle& maxDeviation, const xiiVec3Template<Type>& vNormal); // [tested]

  /// Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  XII_DECLARE_IF_FLOAT_TYPE [[nodiscard]] static xiiVec3Template<Type> MakeRandomDeviation(xiiRandom& inout_rng, const xiiAngled& maxDeviation, const xiiVec3Template<Type>& vNormal); // [tested]
};

// *** Operators ***

template <typename Type>
const xiiVec3Template<Type> operator+(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]

template <typename Type>
const xiiVec3Template<Type> operator-(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]


template <typename Type>
const xiiVec3Template<Type> operator*(Type f, const xiiVec3Template<Type>& v); // [tested]

template <typename Type>
const xiiVec3Template<Type> operator*(const xiiVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
const xiiVec3Template<Type> operator/(const xiiVec3Template<Type>& v, Type f); // [tested]


template <typename Type>
bool operator==(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]

/// Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec3_inl.h>
