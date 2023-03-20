
// clang-format off
static Diligent::CULL_MODE GALCullModeToDiligent[xiiGALCullMode::ENUM_COUNT] =
{
  Diligent::CULL_MODE_NONE,
  Diligent::CULL_MODE_FRONT,
  Diligent::CULL_MODE_BACK
};

static const Diligent::TEXTURE_ADDRESS_MODE GALTextureAddressModeToDiligent[xiiImageAddressMode::ENUM_COUNT] =
{
  Diligent::TEXTURE_ADDRESS_WRAP,
  Diligent::TEXTURE_ADDRESS_CLAMP,
  Diligent::TEXTURE_ADDRESS_BORDER,
  Diligent::TEXTURE_ADDRESS_MIRROR
};

static const Diligent::COMPARISON_FUNCTION GALCompareFuncToDiligent[xiiGALCompareFunc::ENUM_COUNT] =
{
  Diligent::COMPARISON_FUNC_NEVER,
  Diligent::COMPARISON_FUNC_LESS,
  Diligent::COMPARISON_FUNC_EQUAL,
  Diligent::COMPARISON_FUNC_LESS_EQUAL,
  Diligent::COMPARISON_FUNC_GREATER,
  Diligent::COMPARISON_FUNC_NOT_EQUAL,
  Diligent::COMPARISON_FUNC_GREATER_EQUAL,
  Diligent::COMPARISON_FUNC_ALWAYS
};

static const Diligent::STENCIL_OP GALStencilOpTableIndexToDiligent[8] =
{
  Diligent::STENCIL_OP_KEEP,
  Diligent::STENCIL_OP_ZERO,
  Diligent::STENCIL_OP_REPLACE,
  Diligent::STENCIL_OP_INCR_SAT,
  Diligent::STENCIL_OP_DECR_SAT,
  Diligent::STENCIL_OP_INVERT,
  Diligent::STENCIL_OP_INCR_WRAP,
  Diligent::STENCIL_OP_DECR_WRAP
};
// clang-format on
