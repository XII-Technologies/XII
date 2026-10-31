/// Copyright (c) Theophilus Eriata. All Rights Reserved.
// clang-format off
#pragma once
#if defined(BLEND_MODE) && BLEND_MODE >= BLEND_MODE_TRANSPARENT && RENDER_PASS != RENDER_PASS_DEPTH_ONLY && RENDER_PASS != RENDER_PASS_PICKING && RENDER_PASS != RENDER_PASS_PICKING_WIREFRAME
DepthEnable = true
DepthWriteEnable = false
ComparisonDepthFunction = ComparisonFunction_GreaterEqual
CullMode = CullMode_None
IndependentBlend = true
BlendEnable0 = true
SourceBlend0 = BlendFactor_One
DestinationBlend0 = BlendFactor_One
SourceBlendAlpha0 = BlendFactor_One
DestinationBlendAlpha0 = BlendFactor_One
BlendEnable1 = true
SourceBlend1 = BlendFactor_Zero
DestinationBlend1 = BlendFactor_InverseSourceAlpha
SourceBlendAlpha1 = BlendFactor_Zero
DestinationBlendAlpha1 = BlendFactor_InverseSourceAlpha
BlendEnable2 = true
SourceBlend2 = BlendFactor_One
DestinationBlend2 = BlendFactor_One
SourceBlendAlpha2 = BlendFactor_One
DestinationBlendAlpha2 = BlendFactor_One
BlendEnable3 = true
SourceBlend3 = BlendFactor_Zero
DestinationBlend3 = BlendFactor_SourceColor
SourceBlendAlpha3 = BlendFactor_Zero
DestinationBlendAlpha3 = BlendFactor_SourceAlpha
BlendEnable4 = true
SourceBlend4 = BlendFactor_Zero
DestinationBlend4 = BlendFactor_InverseSourceAlpha
SourceBlendAlpha4 = BlendFactor_Zero
DestinationBlendAlpha4 = BlendFactor_InverseSourceAlpha
#else
DepthEnable = true
DepthWriteEnable = false
ComparisonDepthFunction = ComparisonFunction_GreaterEqual
CullMode = CullMode_None
#endif
// clang-format on
