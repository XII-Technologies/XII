#pragma once

#include <Foundation/Math/Math.h>
#include <Foundation/Math/Vec2.h>

/// \brief A 3-component vector class.
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
  /// \brief default-constructed vector is uninitialized (for speed)
  xiiVec3Template(); // [tested]

  /// \brief Initializes the vector with x,y,z
  xiiVec3Template(Type inX, Type inY, Type inZ); // [tested]

  /// \brief Initializes all 3 components with xyz
  explicit xiiVec3Template(Type inV); // [tested]
  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Returns a vector with all components set to zero.
  static xiiVec3Template<Type> ZeroVector() { return xiiVec3Template(0); } // [tested]
  /// \brief Returns a vector with all components set to one.
  static xiiVec3Template<Type> OneVector() { return xiiVec3Template(1); }

  /// \brief Returns a vector initialized to the x unit vector (1, 0, 0).
  static const xiiVec3Template<Type> UnitXAxis() { return xiiVec3Template(1, 0, 0); }
  /// \brief Returns a vector initialized to the y unit vector (0, 1, 0).
  static const xiiVec3Template<Type> UnitYAxis() { return xiiVec3Template(0, 1, 0); }
  /// \brief Returns a vector initialized to the z unit vector (0, 0, 1).
  static const xiiVec3Template<Type> UnitZAxis() { return xiiVec3Template(0, 0, 1); }

#if XII_ENABLED(XII_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    XII_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                                "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// \brief Returns a xiiVec2Template with x and y from this vector.
  const xiiVec2Template<Type> GetAsVec2() const; // [tested]

  /// \brief Returns a xiiVec4Template with x,y,z from this vector and w set to the parameter.
  const xiiVec4Template<Type> GetAsVec4(Type inW) const; // [tested]

  /// \brief Returns a xiiVec4Template with x,y,z from this vector and w set 1.
  const xiiVec4Template<Type> GetAsPositionVec4() const; // [tested]

  /// \brief Returns a xiiVec4Template with x,y,z from this vector and w set 0.
  const xiiVec4Template<Type> GetAsDirectionVec4() const; // [tested]

  /// \brief Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// \brief Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// \brief Sets all 3 components to this value.
  void Set(Type xyz); // [tested]

  /// \brief Sets the vector to these values.
  void Set(Type inX, Type inY, Type inZ); // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// \brief Returns the length of the vector.
  Type GetLength() const; // [tested]

  /// \brief Tries to rescale the vector to the given length. If the vector is too close to zero, XII_FAILURE is returned and the vector is
  /// set to zero.
  xiiResult SetLength(Type fNewLength, Type fEpsilon = xiiMath::DefaultEpsilon<Type>()); // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// \brief Returns the squared length in the XY plane. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared2D() const; // [untest]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  Type GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  const xiiVec3Template<Type> GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, XII_FAILURE is returned and the vector is set to the given
  /// fallback value.
  xiiResult NormalizeIfNotZero(const xiiVec3Template<Type>& vFallback = xiiVec3Template(1, 0, 0), Type fEpsilon = xiiMath::SmallEpsilon<Type>()); // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0, 0) within a given epsilon.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  bool IsNormalized(Type fEpsilon = xiiMath::HugeEpsilon<Type>()) const; // [tested]

  /// \brief Returns true, if any of x, y or z is NaN
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]

  /// \brief Returns the distance between two 3D Vectors.
  Type Distance(const xiiVec3Template<Type>& vPoint) const; // [tested]

  /// \brief Returns the squared distance between two 3D Vectors. Faster, since no square-root is taken. Useful, if one only wants to compare the distance of two
  /// vectors regardless of the magnitude.
  Type DistanceSquared(const xiiVec3Template<Type>& vPoint) const; // [tested]


  // *** Operators ***
public:
  /// \brief Returns the negation of this vector.
  const xiiVec3Template<Type> operator-() const; // [tested]

  /// \brief Adds rhs component-wise to this vector
  void operator+=(const xiiVec3Template<Type>& rhs); // [tested]

  /// \brief Subtracts rhs component-wise from this vector
  void operator-=(const xiiVec3Template<Type>& rhs); // [tested]

  /// \brief Multiplies rhs component-wise to this vector
  void operator*=(const xiiVec3Template<Type>& rhs);

  /// \brief Divides this vector component-wise by rhs
  void operator/=(const xiiVec3Template<Type>& rhs);

  /// \brief Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// \brief Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const xiiVec3Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// \brief Returns the positive angle between *this and rhs.
  /// Both this and rhs must be normalized
  xiiAngle GetAngleBetween(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  Type Dot(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the Cross-product of the two vectors (NOT commutative, order DOES matter)
  const xiiVec3Template<Type> CrossRH(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  const xiiVec3Template<Type> CompMin(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  const xiiVec3Template<Type> CompMax(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise clamped value of *this between low and high.
  const xiiVec3Template<Type> CompClamp(const xiiVec3Template<Type>& vLow, const xiiVec3Template<Type>& vHigh) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  const xiiVec3Template<Type> CompMul(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  const xiiVec3Template<Type> CompDiv(const xiiVec3Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const xiiVec3Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// \brief Calculates the normal of the triangle defined by the three vertices. Vertices are assumed to be ordered counter-clockwise.
  xiiResult CalculateNormal(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2, const xiiVec3Template<Type>& v3); // [tested]

  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  void MakeOrthogonalTo(const xiiVec3Template<Type>& vNormal); // [tested]

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  const xiiVec3Template<Type> GetOrthogonalVector() const; // [tested]

  /// \brief Returns this vector reflected at vNormal.
  const xiiVec3Template<Type> GetReflectedVector(const xiiVec3Template<Type>& vNormal) const; // [tested]

  /// \brief Returns this vector, refracted at vNormal, using the refraction index of the current medium and the medium it enters.
  const xiiVec3Template<Type> GetRefractedVector(const xiiVec3Template<Type>& vNormal, Type fRefIndex1, Type fRefIndex2) const;

  /// \brief Sets the vector to a random point inside a unit sphere (radius 1).
  static xiiVec3Template<Type> CreateRandomPointInSphere(xiiRandom& ref_rng); // [tested]

  /// \brief Creates a random direction vector. The vector is normalized.
  static xiiVec3Template<Type> CreateRandomDirection(xiiRandom& ref_rng); // [tested]

  /// \brief Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationX(xiiRandom& ref_rng, const xiiAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the x axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationX(xiiRandom& ref_rng, const xiiAngled& maxDeviation); // [tested]

  /// \brief Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationY(xiiRandom& ref_rng, const xiiAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the y axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationY(xiiRandom& ref_rng, const xiiAngled& maxDeviation); // [tested]

  /// \brief Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationZ(xiiRandom& ref_rng, const xiiAngle& maxDeviation); // [tested]

  /// \brief Creates a random vector around the z axis with a maximum deviation angle of \a maxDeviation. The vector is normalized.
  /// The deviation angle must be larger than zero.
  static xiiVec3Template<Type> CreateRandomDeviationZ(xiiRandom& ref_rng, const xiiAngled& maxDeviation); // [tested]

  /// \brief Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  static xiiVec3Template<Type> CreateRandomDeviation(xiiRandom& ref_rng, const xiiAngle& maxDeviation, const xiiVec3Template<Type>& vNormal); // [tested]

  /// \brief Creates a random vector around the given normal with a maximum deviation.
  /// \note If you are going to do this many times with the same axis, rather than calling this function, instead manually
  /// do what this function does (see inline code) and only compute the quaternion once.
  static xiiVec3Template<Type> CreateRandomDeviation(xiiRandom& ref_rng, const xiiAngled& maxDeviation, const xiiVec3Template<Type>& vNormal); // [tested]
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

template <typename Type>
bool operator!=(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const xiiVec3Template<Type>& v1, const xiiVec3Template<Type>& v2); // [tested]

#include <Foundation/Math/Implementation/Vec3_inl.h>
