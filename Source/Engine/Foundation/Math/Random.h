/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Math/Declarations.h>

/// \brief A random number generator. Currently uses the WELL512 algorithm.
class XII_FOUNDATION_DLL xiiRandom
{
public:
  xiiRandom();

  /// \brief Initializes the RNG with the given seed value. The value should not be zero.
  void Initialize(xiiUInt64 uiSeed); // [tested]

  /// \brief Initializes the RNG using current time stamp.
  /// Not very sophisticated, but good enough for things that do not need to be secure.
  void InitializeFromCurrentTime();

  /// \brief Serializes the current state
  void Save(xiiStreamWriter& ref_stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(xiiStreamReader& ref_stream); // [tested]

  /// \brief Returns a uint32 value, ie. ranging from 0 to (2 ^ 32) - 1
  xiiUInt32 UInt(); // [tested]

  /// \brief Returns a uint32 value in range [0 ; uiRange - 1]
  ///
  /// \note A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
  /// So always use a range of at least 1.
  xiiUInt32 UIntInRange(xiiUInt32 uiRange); // [tested]

  /// \brief Returns an int32 value in range [iMinValue ; iMinValue + uiRange - 1]
  ///
  /// \note A range of 0 is invalid and will assert! It also has no mathematical meaning. A range of 1 already means "between 0 and 1 EXCLUDING 1".
  /// So always use a range of at least 1.
  xiiInt32 IntInRange(xiiInt32 iMinValue, xiiUInt32 uiRange); // [tested]

  /// \brief Returns an int32 value in range [iMinValue ; iMaxValue]
  xiiInt32 IntMinMax(xiiInt32 iMinValue, xiiInt32 iMaxValue); // [tested]

  /// \brief Returns a boolean either being true or false
  XII_ALWAYS_INLINE bool Bool() { return static_cast<bool>(UInt() & 1); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0), ie. including zero, but excluding one
  XII_ALWAYS_INLINE double DoubleZeroToOneExclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0], ie. including zero and one
  XII_ALWAYS_INLINE double DoubleZeroToOneInclusive() { return (double)UInt() / (double)(0xFFFFFFFFUL + 1.0); } // [tested]

  /// \brief Returns a double value in range [fMinValue ; fMinValue + fRange)
  double DoubleInRange(double fMinValue, double fRange); // [tested]

  /// \brief Returns a double value in range [fMinValue ; fMaxValue]
  double DoubleMinMax(double fMinValue, double fMaxValue); // [tested]

  /// \brief Returns a double value around fValue with a given variance (0 - 1 range)
  double DoubleVariance(double fValue, double fVariance);

  /// \brief Returns a double value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
  double DoubleVarianceAroundZero(double fAbsMaxValue);

  /// \brief Returns a value in range [0.0 ; 1.0), ie. including zero, but excluding one
  XII_ALWAYS_INLINE float FloatZeroToOneExclusive() { return static_cast<float>(DoubleZeroToOneExclusive()); } // [tested]

  /// \brief Returns a value in range [0.0 ; 1.0], ie. including zero and one
  XII_ALWAYS_INLINE float FloatZeroToOneInclusive() { return static_cast<float>(DoubleZeroToOneInclusive()); } // [tested]

  /// \brief Returns a float value in range [fMinValue ; fMinValue + fRange)
  XII_ALWAYS_INLINE float FloatInRange(float fMinValue, float fRange) { return static_cast<float>(DoubleInRange(fMinValue, fRange)); } // [tested]

  /// \brief Returns a float value in range [fMinValue ; fMaxValue]
  XII_ALWAYS_INLINE float FloatMinMax(float fMinValue, float fMaxValue) { return static_cast<float>(DoubleMinMax(fMinValue, fMaxValue)); } // [tested]

  /// \brief Returns a float value around fValue with a given variance (0 - 1 range)
  XII_ALWAYS_INLINE float FloatVariance(float fValue, float fVariance) { return static_cast<float>(DoubleVariance(fValue, fVariance)); }

  /// \brief Returns a float value between [-fAbsMaxValue; +fAbsMaxValue] with a Gaussian distribution.
  XII_ALWAYS_INLINE float FloatVarianceAroundZero(float fAbsMaxValue) { return static_cast<float>(DoubleVarianceAroundZero(fAbsMaxValue)); }

private:
  xiiUInt32 m_uiIndex;
  xiiUInt32 m_uiState[16];
};


/// \brief A random number generator that produces values with a normal / Gaussian distribution
class XII_FOUNDATION_DLL xiiRandomGauss
{
public:
  /// \brief Initializes the RNG and sets the maximum value that the functions UnsignedValue() and SignedValue() may return
  ///
  /// The Variance configures the distribution of the samples. 1.0 gives a standard bell-curve. Values below 1 lead to a distribution
  /// with more emphasis around zero, whereas values above 1 result in a flatter curve with more equally distributed results.
  ///
  /// For more details, look here: https://en.wikipedia.org/wiki/Normal_distribution
  void Initialize(xiiUInt64 uiRandomSeed, xiiUInt32 uiMaxValue, float fVariance = 1.0f); // [tested]

  /// \brief Returns a value in range [0; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than uiMaxValue.
  xiiUInt32 UnsignedValue(); // [tested]

  /// \brief Returns a value in range [-uiMaxValue + 1; uiMaxValue - 1] with a Gaussian distribution. Ie. 0 is much more probable than +/-uiMaxValue.
  xiiInt32 SignedValue(); // [tested]

  /// \brief Serializes the current state
  void Save(xiiStreamWriter& ref_stream) const; // [tested]

  /// \brief Deserializes the current state
  void Load(xiiStreamReader& ref_stream); // [tested]

private:
  void SetupTable(xiiUInt32 uiMaxValue, float fSigma);

  float                  m_fSigma;
  double                 m_fAreaSum;
  xiiDynamicArray<float> m_GaussAreaSum;
  xiiRandom              m_Generator;
};

#include <Foundation/Math/Implementation/AllClassesRandom_inl.h>
