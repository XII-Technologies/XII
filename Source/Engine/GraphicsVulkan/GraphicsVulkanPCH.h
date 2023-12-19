#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#include <Diligent/Graphics/GraphicsEngine/interface/APIInfo.h>
#include <Diligent/Graphics/GraphicsEngine/interface/BlendState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/BottomLevelAS.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Buffer.h>
#include <Diligent/Graphics/GraphicsEngine/interface/BufferView.h>
#include <Diligent/Graphics/GraphicsEngine/interface/CommandList.h>
#include <Diligent/Graphics/GraphicsEngine/interface/CommandQueue.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Constants.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Dearchiver.h>
#include <Diligent/Graphics/GraphicsEngine/interface/DepthStencilState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceMemory.h>
#include <Diligent/Graphics/GraphicsEngine/interface/DeviceObject.h>
#include <Diligent/Graphics/GraphicsEngine/interface/EngineFactory.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Fence.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Framebuffer.h>
#include <Diligent/Graphics/GraphicsEngine/interface/GraphicsTypesX.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/InputLayout.h>
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineResourceSignature.h>
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/PipelineStateCache.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Query.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RasterizerState.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Diligent/Graphics/GraphicsEngine/interface/RenderPass.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ResourceMapping.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Sampler.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Shader.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ShaderBindingTable.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
#include <Diligent/Graphics/GraphicsEngine/interface/ShaderResourceVariable.h>
#include <Diligent/Graphics/GraphicsEngine/interface/SwapChain.h>
#include <Diligent/Graphics/GraphicsEngine/interface/Texture.h>
#include <Diligent/Graphics/GraphicsEngine/interface/TextureView.h>
#include <Diligent/Graphics/GraphicsEngine/interface/TopLevelAS.h>

#include <Diligent/Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <vulkan/vulkan.hpp>

// Some of the functionality we need has moved from vulkan.hpp to vulkan_format_traits.hpp in later versions of the Vulkan SDK.
#if __has_include(<vulkan/vulkan_format_traits.hpp>)
#  include <vulkan/vulkan_format_traits.hpp>
#endif

#include <GraphicsVulkan/Utilities/DiligentTypeConversions.h>
#include <GraphicsVulkan/Utilities/VulkanTypeConversions.h>
