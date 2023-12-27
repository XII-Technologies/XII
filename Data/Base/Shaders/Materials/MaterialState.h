#pragma once

// clang-format off

// Rasterizer State
#if defined(RENDER_PASS) && (RENDER_PASS == RENDER_PASS_WIREFRAME || RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME)
  FillMode = FillMode_Wireframe
#endif

#if TWO_SIDED == TRUE
  CullMode = CullMode_None
#else
  #if FLIP_WINDING == TRUE
    CullMode = CullMode_Front
  #endif
#endif


// Depth-Stencil State
DepthEnable = true
DepthWriteEnable = true
ComparisonDepthFunction = ComparisonFunction_LessEqual

#if defined(BLEND_MODE) && (BLEND_MODE == BLEND_MODE_OPAQUE || BLEND_MODE == BLEND_MODE_MASKED)

  #if defined(RENDER_PASS) && (RENDER_PASS == RENDER_PASS_FORWARD || RENDER_PASS == RENDER_PASS_EDITOR)

    #if defined(FORWARD_PASS_WRITE_DEPTH)
      #if FORWARD_PASS_WRITE_DEPTH == FALSE
        DepthWriteEnable = false
        ComparisonDepthFunction = ComparisonFunction_Equal
      #endif
    #endif

  #endif

#else
  DepthWriteEnable = false
#endif


// Blend State
#if defined(BLEND_MODE)

  #if BLEND_MODE == BLEND_MODE_TRANSPARENT
    BlendEnable0 = true
    BlendOperation0 = BlendOperation_Add
    SourceBlend0 = BlendFactor_SourceAlpha
    DestinationBlend0 = BlendFactor_InverseSourceAlpha
    DestinationBlendAlpha0 = BlendFactor_InverseSourceAlpha

  #elif BLEND_MODE == BLEND_MODE_ADDITIVE
    BlendEnable0 = true
    BlendOperation0 = BlendOperation_Add
    SourceBlend0 = BlendFactor_SourceAlpha
    DestinationBlend0 = BlendFactor_One
    SourceBlendAlpha0 = BlendFactor_Zero
    DestinationBlendAlpha0 = BlendFactor_One

  #elif BLEND_MODE == BLEND_MODE_MODULATE
    BlendEnable0 = true
    BlendOperation0 = BlendOperation_Add
    SourceBlend0 = BlendFactor_Zero
    DestinationBlend0 = BlendFactor_SourceColor
    SourceBlendAlpha0 = BlendFactor_Zero
    DestinationBlendAlpha0 = BlendFactor_One
  #endif

#endif

#if (RENDER_PASS == RENDER_PASS_EDITOR 				    /* disable blending for all editor debug render modes*/\
	|| RENDER_PASS == RENDER_PASS_PICKING 			    /* for transparent objects to be pickable*/\
	|| RENDER_PASS == RENDER_PASS_PICKING_WIREFRAME /* for transparent objects to be pickable*/\
	|| RENDER_PASS == RENDER_PASS_DEPTH_ONLY 		    /* for transparent objects to have a selection outline */\
	)

  BlendEnable0 = false
  DepthWriteEnable = true
#endif

  // clang-format on
