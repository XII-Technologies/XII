#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>
#include <Foundation/Types/Variant.h>

#include <Foundation/Math/Declarations.h>

typedef float xiiMathTestType;

typedef xiiVec2Template<xiiMathTestType>              xiiVec2T;              ///< This is only for testing purposes
typedef xiiVec3Template<xiiMathTestType>              xiiVec3T;              ///< This is only for testing purposes
typedef xiiVec4Template<xiiMathTestType>              xiiVec4T;              ///< This is only for testing purposes
typedef xiiMat3Template<xiiMathTestType>              xiiMat3T;              ///< This is only for testing purposes
typedef xiiMat4Template<xiiMathTestType>              xiiMat4T;              ///< This is only for testing purposes
typedef xiiQuatTemplate<xiiMathTestType>              xiiQuatT;              ///< This is only for testing purposes
typedef xiiPlaneTemplate<xiiMathTestType>             xiiPlaneT;             ///< This is only for testing purposes
typedef xiiBoundingBoxTemplate<xiiMathTestType>       xiiBoundingBoxT;       ///< This is only for testing purposes
typedef xiiBoundingBoxSphereTemplate<xiiMathTestType> xiiBoundingBoxSphereT; ///< This is only for testing purposes
typedef xiiBoundingSphereTemplate<xiiMathTestType>    xiiBoundingSphereT;    ///< This is only for testing purposes
typedef xiiTransformTemplate<xiiMathTestType>         xiiTransformT;

#define xiiFoundationTest_Plugin1 "xiiFoundationTest_Plugin1"
#define xiiFoundationTest_Plugin2 "xiiFoundationTest_Plugin2"
