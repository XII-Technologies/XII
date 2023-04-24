#include <TestFramework/Framework/TestFramework.h>

#include <Foundation/Basics.h>
#include <Foundation/Basics/Assert.h>
#include <Foundation/Types/TypeTraits.h>
#include <Foundation/Types/Types.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>

#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>

#include <Foundation/Math/Declarations.h>

#include <Core/Graphics/Camera.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Shader/ShaderResource.h>
#include <RendererCore/Textures/Texture2DResource.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <Texture/Image/Image.h>
#include <Texture/Image/ImageConversion.h>
#include <Texture/Image/ImageUtils.h>

using xiiMathTestType = float;

using xiiVec2T         = xiiVec2Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiVec3T         = xiiVec3Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiVec4T         = xiiVec4Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiMat3T         = xiiMat3Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiMat4T         = xiiMat4Template<xiiMathTestType>;           ///< This is only for testing purposes
using xiiQuatT         = xiiQuatTemplate<xiiMathTestType>;           ///< This is only for testing purposes
using xiiPlaneT        = xiiPlaneTemplate<xiiMathTestType>;          ///< This is only for testing purposes
using xiiBoundingBoxT  = xiiBoundingBoxTemplate<xiiMathTestType>;    ///< This is only for testing purposes
using xBoundingSphereT = xiiBoundingSphereTemplate<xiiMathTestType>; ///< This is only for testing purposes
