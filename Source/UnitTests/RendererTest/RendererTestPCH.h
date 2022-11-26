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

typedef float xiiMathTestType;

typedef xiiVec2Template<xiiMathTestType>           xiiVec2T;           ///< This is only for testing purposes
typedef xiiVec3Template<xiiMathTestType>           xiiVec3T;           ///< This is only for testing purposes
typedef xiiVec4Template<xiiMathTestType>           xiiVec4T;           ///< This is only for testing purposes
typedef xiiMat3Template<xiiMathTestType>           xiiMat3T;           ///< This is only for testing purposes
typedef xiiMat4Template<xiiMathTestType>           xiiMat4T;           ///< This is only for testing purposes
typedef xiiQuatTemplate<xiiMathTestType>           xiiQuatT;           ///< This is only for testing purposes
typedef xiiPlaneTemplate<xiiMathTestType>          xiiPlaneT;          ///< This is only for testing purposes
typedef xiiBoundingBoxTemplate<xiiMathTestType>    xiiBoundingBoxT;    ///< This is only for testing purposes
typedef xiiBoundingSphereTemplate<xiiMathTestType> xiiBoundingSphereT; ///< This is only for testing purposes
