/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

#include <Foundation/Math/Declarations.h>

using xiiMathTestType = float;

using xiiVec2T           = xiiVec2Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiVec3T           = xiiVec3Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiVec4T           = xiiVec4Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiMat3T           = xiiMat3Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiMat4T           = xiiMat4Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiQuatT           = xiiQuatTemplate<xiiMathTestType>;           ///< This is only for testing purposes
using xiiPlaneT          = xiiPlaneTemplate<xiiMathTestType>;          ///< This is only for testing purposes
using xiiBoundingBoxT    = xiiBoundingBoxTemplate<xiiMathTestType>;    ///< This is only for testing purposes
using xiiBoundingSphereT = xiiBoundingSphereTemplate<xiiMathTestType>; ///< This is only for testing purposes
