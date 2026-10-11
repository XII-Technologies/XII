/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TextureConverter/TextureConverterPCH.h>

#include <Foundation/Utilities/CommandLineOptions.h>
#include <TextureConverter/TextureConverter.h>

namespace
{
  xiiCommandLineOptionDoc s_MinFilter("_TextureConverter", "-minFilter", "1|2|3", "Override the sampler minification filter: Point=1, Linear=2, Anisotropic=3.", "");
  xiiCommandLineOptionDoc s_MagFilter("_TextureConverter", "-magFilter", "1|2|3", "Override the sampler magnification filter: Point=1, Linear=2, Anisotropic=3.", "");
  xiiCommandLineOptionDoc s_MipFilter("_TextureConverter", "-mipFilter", "1|2|3", "Override the sampler mip filter: Point=1, Linear=2, Anisotropic=3.", "");
  xiiCommandLineOptionDoc s_Anisotropy("_TextureConverter", "-maxAnisotropy", "0..16", "Maximum anisotropy. Use zero with point or linear filtering; anisotropic filtering requires all three filters and a value from 1 to 16.", "");
  xiiCommandLineOptionDoc s_Bias("_TextureConverter", "-mipLODBias", "float", "Finite mip LOD bias stored in the native sampler.", "");
  xiiCommandLineOptionDoc s_MinLOD("_TextureConverter", "-minLOD", "float", "Nonnegative minimum mip LOD stored in the native sampler.", "");
  xiiCommandLineOptionDoc s_MaxLOD("_TextureConverter", "-maxLOD", "float", "Maximum mip LOD, at least minLOD. Zero disables mip selection without discarding image mip levels.", "");

  xiiResult ReadUInt(const char* name, xiiUInt32& inout_value)
  {
    const auto*    command = xiiCommandLineUtils::GetGlobalInstance();
    const xiiInt32 index   = command->GetOptionIndex(name);
    if (index < 0)
      return XII_SUCCESS;
    const xiiString text  = xiiUInt32(index + 1) < command->GetParameterCount() ? command->GetParameter(index + 1) : xiiString();
    const char*     end   = nullptr;
    xiiUInt32       value = 0;
    if (text.IsEmpty() || xiiConversionUtils::StringToUInt(text, value, &end).Failed() || end != text.GetView().GetEndPointer())
    {
      xiiLog::Error("Invalid unsigned sampler value for '{}': '{}'.", name, text);
      return XII_FAILURE;
    }
    inout_value = value;
    return XII_SUCCESS;
  }

  xiiResult ReadFloat(const char* name, float& inout_value)
  {
    const auto*    command = xiiCommandLineUtils::GetGlobalInstance();
    const xiiInt32 index   = command->GetOptionIndex(name);
    if (index < 0)
      return XII_SUCCESS;
    const xiiString text  = xiiUInt32(index + 1) < command->GetParameterCount() ? command->GetParameter(index + 1) : xiiString();
    const char*     end   = nullptr;
    double          value = 0;
    if (text.IsEmpty() || xiiConversionUtils::StringToFloat(text, value, &end).Failed() || end != text.GetView().GetEndPointer() ||
        !xiiMath::IsFinite(value) || xiiMath::Abs(value) > double(xiiMath::MaxValue<float>()) * (1.0 + double(xiiMath::FloatEpsilon<float>()) * 0.25))
    {
      xiiLog::Error("Invalid finite sampler value for '{}': '{}'.", name, text);
      return XII_FAILURE;
    }
    // Decimal round-tripping of FLT_MAX may lie slightly above its exact double value.
    const double maximum = double(xiiMath::MaxValue<float>());
    inout_value          = static_cast<float>(xiiMath::Clamp(value, -maximum, maximum));
    return XII_SUCCESS;
  }
} // namespace

xiiResult xiiTextureConverter::ParseSamplerOverrides()
{
  auto                       sampler   = m_Processor.m_Descriptor.m_SamplerDescription;
  const char*                names[]   = {"-minFilter", "-magFilter", "-mipFilter"};
  xiiEnum<xiiGALFilterType>* filters[] = {&sampler.m_MinFilter, &sampler.m_MagFilter, &sampler.m_MipFilter};
  for (xiiUInt32 i = 0; i < XII_ARRAY_SIZE(names); ++i)
  {
    xiiUInt32 value = filters[i]->GetValue();
    XII_SUCCEED_OR_RETURN(ReadUInt(names[i], value));
    if (value < xiiGALFilterType::Point || value > xiiGALFilterType::Anisotropic)
    {
      xiiLog::Error("Sampler filter '{}' must be Point=1, Linear=2 or Anisotropic=3.", names[i]);
      return XII_FAILURE;
    }
    *filters[i] = static_cast<xiiGALFilterType::Enum>(value);
  }
  XII_SUCCEED_OR_RETURN(ReadUInt("-maxAnisotropy", sampler.m_uiMaxAnisotropy));
  XII_SUCCEED_OR_RETURN(ReadFloat("-mipLODBias", sampler.m_fMipLODBias));
  XII_SUCCEED_OR_RETURN(ReadFloat("-minLOD", sampler.m_fMinLOD));
  XII_SUCCEED_OR_RETURN(ReadFloat("-maxLOD", sampler.m_fMaxLOD));
  if (xiiTextureConverterSamplerPreset::ValidateFiltering(sampler).Failed())
  {
    xiiLog::Error("Invalid sampler filter combination, anisotropy or LOD bounds.");
    return XII_FAILURE;
  }
  m_Processor.m_Descriptor.m_SamplerDescription = sampler;
  return XII_SUCCESS;
}
