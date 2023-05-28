#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Logging/Log.h>

#include <Graphics/GraphicsEngine/interface/APIInfo.h>
#include <Graphics/GraphicsEngine/interface/BlendState.h>
#include <Graphics/GraphicsEngine/interface/BottomLevelAS.h>
#include <Graphics/GraphicsEngine/interface/Buffer.h>
#include <Graphics/GraphicsEngine/interface/BufferView.h>
#include <Graphics/GraphicsEngine/interface/CommandList.h>
#include <Graphics/GraphicsEngine/interface/CommandQueue.h>
#include <Graphics/GraphicsEngine/interface/Constants.h>
#include <Graphics/GraphicsEngine/interface/Dearchiver.h>
#include <Graphics/GraphicsEngine/interface/DepthStencilState.h>
#include <Graphics/GraphicsEngine/interface/DeviceContext.h>
#include <Graphics/GraphicsEngine/interface/DeviceMemory.h>
#include <Graphics/GraphicsEngine/interface/DeviceObject.h>
#include <Graphics/GraphicsEngine/interface/EngineFactory.h>
#include <Graphics/GraphicsEngine/interface/Fence.h>
#include <Graphics/GraphicsEngine/interface/Framebuffer.h>
#include <Graphics/GraphicsEngine/interface/GraphicsTypesX.hpp>
#include <Graphics/GraphicsEngine/interface/InputLayout.h>
#include <Graphics/GraphicsEngine/interface/PipelineResourceSignature.h>
#include <Graphics/GraphicsEngine/interface/PipelineState.h>
#include <Graphics/GraphicsEngine/interface/PipelineStateCache.h>
#include <Graphics/GraphicsEngine/interface/Query.h>
#include <Graphics/GraphicsEngine/interface/RasterizerState.h>
#include <Graphics/GraphicsEngine/interface/RenderDevice.h>
#include <Graphics/GraphicsEngine/interface/RenderPass.h>
#include <Graphics/GraphicsEngine/interface/ResourceMapping.h>
#include <Graphics/GraphicsEngine/interface/Sampler.h>
#include <Graphics/GraphicsEngine/interface/Shader.h>
#include <Graphics/GraphicsEngine/interface/ShaderBindingTable.h>
#include <Graphics/GraphicsEngine/interface/ShaderResourceBinding.h>
#include <Graphics/GraphicsEngine/interface/ShaderResourceVariable.h>
#include <Graphics/GraphicsEngine/interface/SwapChain.h>
#include <Graphics/GraphicsEngine/interface/Texture.h>
#include <Graphics/GraphicsEngine/interface/TextureView.h>
#include <Graphics/GraphicsEngine/interface/TopLevelAS.h>

#include <Graphics/GraphicsAccessories/interface/GraphicsAccessories.hpp>

#if XII_ENABLED(XII_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

#include <RendererDiligent/Utilities/ConversionUtilsDiligent.h>
#include <RendererDiligent/Utilities/PipelineBarrierDiligent.h>
