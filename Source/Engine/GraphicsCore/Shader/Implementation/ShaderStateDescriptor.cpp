#include <GraphicsCore/GraphicsCorePCH.h>

#include <GraphicsCore/Shader/ShaderPermutationBinary.h>

struct xiiShaderStateVersion
{
  enum Enum : xiiUInt32
  {
    Version0 = 0,
    Version1,
    Version2,
    Version3,

    ENUM_COUNT,
    Current = ENUM_COUNT - 1
  };
};

void xiiShaderStateResourceDescriptor::Save(xiiStreamWriter& inout_stream) const
{
  inout_stream << (xiiUInt32)xiiShaderStateVersion::Current;

  // Blend State
  {
    inout_stream << m_BlendDesc.m_bAlphaToCoverage;
    inout_stream << m_BlendDesc.m_bIndependentBlend;

    const xiiUInt8 iBlends = m_BlendDesc.m_bIndependentBlend ? XII_GAL_MAX_RENDERTARGET_COUNT : 1;
    inout_stream << iBlends; // in case XII_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (xiiUInt32 b = 0; b < iBlends; ++b)
    {
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend;
      inout_stream << (xiiUInt8)m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha;
      inout_stream << m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_DepthTestFunc;
    inout_stream << m_DepthStencilDesc.m_bDepthTest;
    inout_stream << m_DepthStencilDesc.m_bDepthWrite;
    inout_stream << m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream << m_DepthStencilDesc.m_bStencilTest;
    inout_stream << m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream << m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp;
    inout_stream << (xiiUInt8)m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc;
  }

  // Rasterizer State
  {
    inout_stream << m_RasterizerDesc.m_bFrontCounterClockwise;
    inout_stream << m_RasterizerDesc.m_bScissorTest;
    inout_stream << m_RasterizerDesc.m_bWireFrame;
    inout_stream << (xiiUInt8)m_RasterizerDesc.m_CullMode;
    inout_stream << m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream << m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream << m_RasterizerDesc.m_iDepthBias;
    inout_stream << m_RasterizerDesc.m_bConservativeRasterization;
  }
}

void xiiShaderStateResourceDescriptor::Load(xiiStreamReader& inout_stream)
{
  xiiUInt32 uiVersion = 0;
  inout_stream >> uiVersion;

  XII_ASSERT_DEV(uiVersion >= xiiShaderStateVersion::Version1 && uiVersion <= xiiShaderStateVersion::Current, "Invalid version {0}", uiVersion);

  // Blend State
  {
    inout_stream >> m_BlendDesc.m_bAlphaToCoverage;
    inout_stream >> m_BlendDesc.m_bIndependentBlend;

    xiiUInt8 iBlends = 0;
    inout_stream >> iBlends; // in case XII_GAL_MAX_RENDERTARGET_COUNT ever changes

    for (xiiUInt32 b = 0; b < iBlends; ++b)
    {
      xiiUInt8 uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_bBlendingEnabled;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOp = (xiiGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_BlendOpAlpha = (xiiGALBlendOp::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlend = (xiiGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_DestBlendAlpha = (xiiGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlend = (xiiGALBlend::Enum)uiTemp;
      inout_stream >> uiTemp;
      m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_SourceBlendAlpha = (xiiGALBlend::Enum)uiTemp;
      inout_stream >> m_BlendDesc.m_RenderTargetBlendDescriptions[b].m_uiWriteMask;
    }
  }

  // Depth Stencil State
  {
    xiiUInt8 uiTemp = 0;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_DepthTestFunc = (xiiGALCompareFunc::Enum)uiTemp;
    inout_stream >> m_DepthStencilDesc.m_bDepthTest;
    inout_stream >> m_DepthStencilDesc.m_bDepthWrite;
    inout_stream >> m_DepthStencilDesc.m_bSeparateFrontAndBack;
    inout_stream >> m_DepthStencilDesc.m_bStencilTest;
    inout_stream >> m_DepthStencilDesc.m_uiStencilReadMask;
    inout_stream >> m_DepthStencilDesc.m_uiStencilWriteMask;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (xiiGALCompareFunc::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (xiiGALStencilOp::Enum)uiTemp;
    inout_stream >> uiTemp;
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (xiiGALCompareFunc::Enum)uiTemp;
  }

  // Rasterizer State
  {
    xiiUInt8 uiTemp = 0;

    if (uiVersion < xiiShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bFrontCounterClockwise;

    if (uiVersion < xiiShaderStateVersion::Version2)
    {
      bool dummy;
      inout_stream >> dummy;
      inout_stream >> dummy;
    }

    inout_stream >> m_RasterizerDesc.m_bScissorTest;
    inout_stream >> m_RasterizerDesc.m_bWireFrame;
    inout_stream >> uiTemp;
    m_RasterizerDesc.m_CullMode = (xiiGALCullMode::Enum)uiTemp;
    inout_stream >> m_RasterizerDesc.m_fDepthBiasClamp;
    inout_stream >> m_RasterizerDesc.m_fSlopeScaledDepthBias;
    inout_stream >> m_RasterizerDesc.m_iDepthBias;

    if (uiVersion >= xiiShaderStateVersion::Version3)
    {
      inout_stream >> m_RasterizerDesc.m_bConservativeRasterization;
    }
  }
}

xiiUInt32 xiiShaderStateResourceDescriptor::CalculateHash() const
{
  return m_BlendDesc.CalculateHash() + m_RasterizerDesc.CalculateHash() + m_DepthStencilDesc.CalculateHash();
}

static const char* InsertNumber(const char* szString, xiiUInt32 uiNumber, xiiStringBuilder& ref_sTemp)
{
  ref_sTemp.Format(szString, uiNumber);
  return ref_sTemp.GetData();
}

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
static xiiSet<xiiString> s_AllAllowedVariables;
#endif

static bool GetBoolStateVariable(const xiiMap<xiiString, xiiString>& variables, const char* szVariable, bool bDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return bDefValue;

  if (it.Value() == "true")
    return true;
  if (it.Value() == "false")
    return false;

  xiiLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Should be 'true' or 'false'", szVariable, it.Value());
  return bDefValue;
}

static xiiInt32 GetEnumStateVariable(
  const xiiMap<xiiString, xiiString>& variables,
  const xiiMap<xiiString, xiiInt32>&  values,
  const char*                         szVariable,
  xiiInt32                            iDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  auto itVal = values.Find(it.Value());
  if (!itVal.IsValid())
  {
    xiiStringBuilder valid;
    for (auto vv = values.GetIterator(); vv.IsValid(); ++vv)
    {
      valid.Append(" ", vv.Key());
    }

    xiiLog::Error("Shader state variable '{0}' is set to invalid value '{1}'. Valid values are:{2}", szVariable, it.Value(), valid);
    return iDefValue;
  }

  return itVal.Value();
}

static float GetFloatStateVariable(const xiiMap<xiiString, xiiString>& variables, const char* szVariable, float fDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return fDefValue;

  double result = 0;
  if (xiiConversionUtils::StringToFloat(it.Value(), result).Failed())
  {
    xiiLog::Error("Shader state variable '{0}' is not a valid float value: '{1}'.", szVariable, it.Value());
    return fDefValue;
  }

  return (float)result;
}

static xiiInt32 GetIntStateVariable(const xiiMap<xiiString, xiiString>& variables, const char* szVariable, xiiInt32 iDefValue)
{
#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  s_AllAllowedVariables.Insert(szVariable);
#endif

  auto it = variables.Find(szVariable);

  if (!it.IsValid())
    return iDefValue;

  xiiInt32 result = 0;
  if (xiiConversionUtils::StringToInt(it.Value(), result).Failed())
  {
    xiiLog::Error("Shader state variable '{0}' is not a valid int value: '{1}'.", szVariable, it.Value());
    return iDefValue;
  }

  return result;
}

// Global variables don't use memory tracking, so these won't reported as memory leaks.
static xiiMap<xiiString, xiiInt32> StateValuesBlend;
static xiiMap<xiiString, xiiInt32> StateValuesBlendOp;
static xiiMap<xiiString, xiiInt32> StateValuesCullMode;
static xiiMap<xiiString, xiiInt32> StateValuesCompareFunc;
static xiiMap<xiiString, xiiInt32> StateValuesStencilOp;

xiiResult xiiShaderStateResourceDescriptor::Parse(const char* szSource)
{
  xiiMap<xiiString, xiiString> VariableValues;

  // extract all state assignments
  {
    xiiStringBuilder sSource = szSource;

    xiiHybridArray<xiiStringView, 32> allAssignments;
    xiiHybridArray<xiiStringView, 4>  components;
    sSource.Split(false, allAssignments, "\n", ";", "\r");

    xiiStringBuilder temp;
    for (const xiiStringView& assignment : allAssignments)
    {
      temp = assignment;
      temp.Trim(" \t\r\n;");
      if (temp.IsEmpty())
        continue;

      temp.Split(false, components, " ", "\t", "=", "\r");

      if (components.GetCount() != 2)
      {
        xiiLog::Error("Malformed shader state assignment: '{0}'", temp);
        continue;
      }

      VariableValues[components[0]] = components[1];
    }
  }

  if (StateValuesBlend.IsEmpty())
  {
    // xiiGALBlend
    {
      StateValuesBlend["Blend_Zero"]              = xiiGALBlend::Zero;
      StateValuesBlend["Blend_One"]               = xiiGALBlend::One;
      StateValuesBlend["Blend_SrcColor"]          = xiiGALBlend::SrcColor;
      StateValuesBlend["Blend_InvSrcColor"]       = xiiGALBlend::InvSrcColor;
      StateValuesBlend["Blend_SrcAlpha"]          = xiiGALBlend::SrcAlpha;
      StateValuesBlend["Blend_InvSrcAlpha"]       = xiiGALBlend::InvSrcAlpha;
      StateValuesBlend["Blend_DestAlpha"]         = xiiGALBlend::DestAlpha;
      StateValuesBlend["Blend_InvDestAlpha"]      = xiiGALBlend::InvDestAlpha;
      StateValuesBlend["Blend_DestColor"]         = xiiGALBlend::DestColor;
      StateValuesBlend["Blend_InvDestColor"]      = xiiGALBlend::InvDestColor;
      StateValuesBlend["Blend_SrcAlphaSaturated"] = xiiGALBlend::SrcAlphaSaturated;
      StateValuesBlend["Blend_BlendFactor"]       = xiiGALBlend::BlendFactor;
      StateValuesBlend["Blend_InvBlendFactor"]    = xiiGALBlend::InvBlendFactor;
    }

    // xiiGALBlendOp
    {
      StateValuesBlendOp["BlendOp_Add"]         = xiiGALBlendOp::Add;
      StateValuesBlendOp["BlendOp_Subtract"]    = xiiGALBlendOp::Subtract;
      StateValuesBlendOp["BlendOp_RevSubtract"] = xiiGALBlendOp::RevSubtract;
      StateValuesBlendOp["BlendOp_Min"]         = xiiGALBlendOp::Min;
      StateValuesBlendOp["BlendOp_Max"]         = xiiGALBlendOp::Max;
    }

    // xiiGALCullMode
    {
      StateValuesCullMode["CullMode_None"]  = xiiGALCullMode::None;
      StateValuesCullMode["CullMode_Front"] = xiiGALCullMode::Front;
      StateValuesCullMode["CullMode_Back"]  = xiiGALCullMode::Back;
    }

    // xiiGALCompareFunc
    {
      StateValuesCompareFunc["CompareFunc_Never"]        = xiiGALCompareFunc::Never;
      StateValuesCompareFunc["CompareFunc_Less"]         = xiiGALCompareFunc::Less;
      StateValuesCompareFunc["CompareFunc_Equal"]        = xiiGALCompareFunc::Equal;
      StateValuesCompareFunc["CompareFunc_LessEqual"]    = xiiGALCompareFunc::LessEqual;
      StateValuesCompareFunc["CompareFunc_Greater"]      = xiiGALCompareFunc::Greater;
      StateValuesCompareFunc["CompareFunc_NotEqual"]     = xiiGALCompareFunc::NotEqual;
      StateValuesCompareFunc["CompareFunc_GreaterEqual"] = xiiGALCompareFunc::GreaterEqual;
      StateValuesCompareFunc["CompareFunc_Always"]       = xiiGALCompareFunc::Always;
    }

    // xiiGALStencilOp
    {
      StateValuesStencilOp["StencilOp_Keep"]               = xiiGALStencilOp::Keep;
      StateValuesStencilOp["StencilOp_Zero"]               = xiiGALStencilOp::Zero;
      StateValuesStencilOp["StencilOp_Replace"]            = xiiGALStencilOp::Replace;
      StateValuesStencilOp["StencilOp_IncrementSaturated"] = xiiGALStencilOp::IncrementSaturated;
      StateValuesStencilOp["StencilOp_DecrementSaturated"] = xiiGALStencilOp::DecrementSaturated;
      StateValuesStencilOp["StencilOp_Invert"]             = xiiGALStencilOp::Invert;
      StateValuesStencilOp["StencilOp_Increment"]          = xiiGALStencilOp::Increment;
      StateValuesStencilOp["StencilOp_Decrement"]          = xiiGALStencilOp::Decrement;
    }
  }

  // Retrieve Blend State
  {
    m_BlendDesc.m_bAlphaToCoverage  = GetBoolStateVariable(VariableValues, "AlphaToCoverage", m_BlendDesc.m_bAlphaToCoverage);
    m_BlendDesc.m_bIndependentBlend = GetBoolStateVariable(VariableValues, "IndependentBlend", m_BlendDesc.m_bIndependentBlend);

    xiiStringBuilder s;

    for (xiiUInt32 i = 0; i < 8; ++i)
    {
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled = GetBoolStateVariable(
        VariableValues, InsertNumber("BlendingEnabled{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOp = (xiiGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOp{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOp);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha = (xiiGALBlendOp::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlendOp, InsertNumber("BlendOpAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_BlendOpAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlend = (xiiGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha = (xiiGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("DestBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_DestBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlend = (xiiGALBlend::Enum)GetEnumStateVariable(
        VariableValues, StateValuesBlend, InsertNumber("SourceBlend{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha = (xiiGALBlend::Enum)GetEnumStateVariable(VariableValues, StateValuesBlend,
                                                                                                                  InsertNumber("SourceBlendAlpha{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlendAlpha);
      m_BlendDesc.m_RenderTargetBlendDescriptions[i].m_uiWriteMask      = static_cast<xiiUInt8>(GetIntStateVariable(VariableValues, InsertNumber("WriteMask{0}", i, s), m_BlendDesc.m_RenderTargetBlendDescriptions[0].m_uiWriteMask));
    }
  }

  // Retrieve Rasterizer State
  {
    m_RasterizerDesc.m_bFrontCounterClockwise =
      GetBoolStateVariable(VariableValues, "FrontCounterClockwise", m_RasterizerDesc.m_bFrontCounterClockwise);
    m_RasterizerDesc.m_bScissorTest = GetBoolStateVariable(VariableValues, "ScissorTest", m_RasterizerDesc.m_bScissorTest);
    m_RasterizerDesc.m_bConservativeRasterization =
      GetBoolStateVariable(VariableValues, "ConservativeRasterization", m_RasterizerDesc.m_bConservativeRasterization);
    m_RasterizerDesc.m_bWireFrame = GetBoolStateVariable(VariableValues, "WireFrame", m_RasterizerDesc.m_bWireFrame);
    m_RasterizerDesc.m_CullMode =
      (xiiGALCullMode::Enum)GetEnumStateVariable(VariableValues, StateValuesCullMode, "CullMode", m_RasterizerDesc.m_CullMode);
    m_RasterizerDesc.m_fDepthBiasClamp = GetFloatStateVariable(VariableValues, "DepthBiasClamp", m_RasterizerDesc.m_fDepthBiasClamp);
    m_RasterizerDesc.m_fSlopeScaledDepthBias =
      GetFloatStateVariable(VariableValues, "SlopeScaledDepthBias", m_RasterizerDesc.m_fSlopeScaledDepthBias);
    m_RasterizerDesc.m_iDepthBias = GetIntStateVariable(VariableValues, "DepthBias", m_RasterizerDesc.m_iDepthBias);
  }

  // Retrieve Depth-Stencil State
  {
    m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceDepthFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFaceFailOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "BackFacePassOp", m_DepthStencilDesc.m_BackFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc = (xiiGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "BackFaceStencilFunc", m_DepthStencilDesc.m_BackFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceDepthFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_DepthFailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFaceFailOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_FailOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp = (xiiGALStencilOp::Enum)GetEnumStateVariable(
      VariableValues, StateValuesStencilOp, "FrontFacePassOp", m_DepthStencilDesc.m_FrontFaceStencilOp.m_PassOp);
    m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc = (xiiGALCompareFunc::Enum)GetEnumStateVariable(
      VariableValues, StateValuesCompareFunc, "FrontFaceStencilFunc", m_DepthStencilDesc.m_FrontFaceStencilOp.m_StencilFunc);

    m_DepthStencilDesc.m_bDepthTest  = GetBoolStateVariable(VariableValues, "DepthTest", m_DepthStencilDesc.m_bDepthTest);
    m_DepthStencilDesc.m_bDepthWrite = GetBoolStateVariable(VariableValues, "DepthWrite", m_DepthStencilDesc.m_bDepthWrite);
    m_DepthStencilDesc.m_bSeparateFrontAndBack =
      GetBoolStateVariable(VariableValues, "SeparateFrontAndBack", m_DepthStencilDesc.m_bSeparateFrontAndBack);
    m_DepthStencilDesc.m_bStencilTest = GetBoolStateVariable(VariableValues, "StencilTest", m_DepthStencilDesc.m_bStencilTest);
    m_DepthStencilDesc.m_DepthTestFunc =
      (xiiGALCompareFunc::Enum)GetEnumStateVariable(VariableValues, StateValuesCompareFunc, "DepthTestFunc", m_DepthStencilDesc.m_DepthTestFunc);
    m_DepthStencilDesc.m_uiStencilReadMask  = static_cast<xiiUInt8>(GetIntStateVariable(VariableValues, "StencilReadMask", m_DepthStencilDesc.m_uiStencilReadMask));
    m_DepthStencilDesc.m_uiStencilWriteMask = static_cast<xiiUInt8>(GetIntStateVariable(VariableValues, "StencilWriteMask", m_DepthStencilDesc.m_uiStencilWriteMask));
  }

#if XII_ENABLED(XII_COMPILE_FOR_DEBUG)
  // check for invalid variable names
  {
    for (auto it = VariableValues.GetIterator(); it.IsValid(); ++it)
    {
      if (!s_AllAllowedVariables.Contains(it.Key()))
      {
        xiiLog::Error("The shader state variable '{0}' does not exist.", it.Key());
      }
    }
  }
#endif


  return XII_SUCCESS;
}

XII_STATICLINK_FILE(GraphicsCore, GraphicsCore_Shader_Implementation_ShaderStateDescriptor);
