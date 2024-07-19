#pragma once

#include <TestFramework/Framework/TestFramework.h>
#include <TestFramework/Utilities/ConstructionCounter.h>

#include <Foundation/Basics.h>
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

using xiiMathTestType = float;

using xiiVec2T              = xiiVec2Template<xiiMathTestType>;              ///< This is only for testing purposes
using xiiVec3T              = xiiVec3Template<xiiMathTestType>;              ///< This is only for testing purposes
using xiiVec4T              = xiiVec4Template<xiiMathTestType>;              ///< This is only for testing purposes
using xiiMat3T              = xiiMat3Template<xiiMathTestType>;              ///< This is only for testing purposes
using xiiMat4T              = xiiMat4Template<xiiMathTestType>;              ///< This is only for testing purposes
using xiiQuatT              = xiiQuatTemplate<xiiMathTestType>;              ///< This is only for testing purposes
using xiiPlaneT             = xiiPlaneTemplate<xiiMathTestType>;             ///< This is only for testing purposes
using xiiBoundingBoxT       = xiiBoundingBoxTemplate<xiiMathTestType>;       ///< This is only for testing purposes
using xiiBoundingBoxSphereT = xiiBoundingBoxSphereTemplate<xiiMathTestType>; ///< This is only for testing purposes
using xiiBoundingSphereT    = xiiBoundingSphereTemplate<xiiMathTestType>;    ///< This is only for testing purposes
using xiiTransformT         = xiiTransformTemplate<xiiMathTestType>;

#define xiiFoundationTest_Plugin1 "xiiFoundationTest_Plugin1"
#define xiiFoundationTest_Plugin2 "xiiFoundationTest_Plugin2"
