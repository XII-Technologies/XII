/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <TextureConverter/TextureConverterPCH.h>

#include <TextureConverter/TextureConverter.h>

static xiiStringView ToString(xiiTextureConverterChannelValue::Enum e)
{
  switch (e)
  {
    case xiiTextureConverterChannelValue::Red:
      return "Red";
    case xiiTextureConverterChannelValue::Green:
      return "Green";
    case xiiTextureConverterChannelValue::Blue:
      return "Blue";
    case xiiTextureConverterChannelValue::Alpha:
      return "Alpha";
    case xiiTextureConverterChannelValue::Black:
      return "Black";
    case xiiTextureConverterChannelValue::White:
      return "White";

    default:
      XII_ASSERT_NOT_IMPLEMENTED;
  }

  return "";
}

xiiResult xiiTextureConverter::ParseChannelMappings()
{
  if (m_Processor.m_Descriptor.m_OutputType == xiiTextureConverterOutputType::Atlas)
    return XII_SUCCESS;

  auto& mappings = m_Processor.m_Descriptor.m_ChannelMappings;

  XII_SUCCEED_OR_RETURN(ParseChannelSliceMapping(-1));

  for (xiiUInt32 slice = 0; slice < 64; ++slice)
  {
    const xiiUInt32 uiPrevMappings = mappings.GetCount();

    XII_SUCCEED_OR_RETURN(ParseChannelSliceMapping(slice));

    if (uiPrevMappings == mappings.GetCount())
    {
      // if no new mapping was found, don't try to find more
      break;
    }
  }

  if (!mappings.IsEmpty())
  {
    xiiLog::Info("Custom output channel mapping:");
    for (xiiUInt32 m = 0; m < mappings.GetCount(); ++m)
    {
      xiiLog::Info("Slice {}, R -> Input file {}, {}", m, mappings[m].m_Channel[0].m_iInputImageIndex, ToString(mappings[m].m_Channel[0].m_ChannelValue));
      xiiLog::Info("Slice {}, G -> Input file {}, {}", m, mappings[m].m_Channel[1].m_iInputImageIndex, ToString(mappings[m].m_Channel[1].m_ChannelValue));
      xiiLog::Info("Slice {}, B -> Input file {}, {}", m, mappings[m].m_Channel[2].m_iInputImageIndex, ToString(mappings[m].m_Channel[2].m_ChannelValue));
      xiiLog::Info("Slice {}, A -> Input file {}, {}", m, mappings[m].m_Channel[3].m_iInputImageIndex, ToString(mappings[m].m_Channel[3].m_ChannelValue));
    }
  }

  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseChannelSliceMapping(xiiInt32 iSlice)
{
  const auto       pCmd     = xiiCommandLineUtils::GetGlobalInstance();
  auto&            mappings = m_Processor.m_Descriptor.m_ChannelMappings;
  xiiStringBuilder tmp, param;

  const xiiUInt32 uiMappingIdx = iSlice < 0 ? 0 : iSlice;

  // input to output mappings
  {
    param = "-rgba";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, false));
    }

    param = "-rgb";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, false));
    }

    param = "-rg";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, false));
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, false));
    }

    param = "-r";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[0], tmp, 0, true));
    }

    param = "-g";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[1], tmp, 1, true));
    }

    param = "-b";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[2], tmp, 2, true));
    }

    param = "-a";
    if (iSlice != -1)
      param.AppendFormat("{}", iSlice);

    tmp = pCmd->GetStringOption(param);
    if (!tmp.IsEmpty())
    {
      mappings.EnsureCount(uiMappingIdx + 1);
      XII_SUCCEED_OR_RETURN(ParseChannelMappingConfig(mappings[uiMappingIdx].m_Channel[3], tmp, 3, true));
    }
  }
  return XII_SUCCESS;
}

xiiResult xiiTextureConverter::ParseChannelMappingConfig(xiiTextureConverterChannelMapping& out_mapping, xiiStringView sCfg, xiiInt32 iChannelIndex, bool bSingleChannel)
{
  out_mapping.m_iInputImageIndex = -1;
  out_mapping.m_ChannelValue     = xiiTextureConverterChannelValue::White;

  xiiStringBuilder tmp = sCfg;

  // '-r black' for setting it to zero
  if (tmp.IsEqual_NoCase("black"))
  {
    out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::Black;
    return XII_SUCCESS;
  }

  // '-r white' for setting it to 255
  if (tmp.IsEqual_NoCase("white"))
  {
    out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::White;
    return XII_SUCCESS;
  }

  // skip the 'in', if found
  // 'in' is optional, one can also write '-r 1.r' for '-r in1.r'
  if (tmp.StartsWith_NoCase("in"))
    tmp.Shrink(2, 0);

  if (tmp.StartsWith("."))
  {
    // no index given, e.g. '-r in.r'
    // in is equal to in0

    out_mapping.m_iInputImageIndex = 0;
  }
  else
  {
    xiiInt32    num       = -1;
    const char* szLastPos = nullptr;
    if (xiiConversionUtils::StringToInt(tmp, num, &szLastPos).Failed())
    {
      xiiLog::Error("Could not parse channel mapping '{0}'", sCfg);
      return XII_FAILURE;
    }

    // valid index after the 'in'
    if (num >= 0 && num < (xiiInt32)m_Processor.m_Descriptor.m_InputFiles.GetCount())
    {
      out_mapping.m_iInputImageIndex = (xiiInt8)num;
    }
    else
    {
      xiiLog::Error("Invalid channel mapping input file index '{0}'", num);
      return XII_FAILURE;
    }

    xiiStringBuilder dummy = szLastPos;

    // continue after the index
    tmp = dummy;
  }

  // no additional info, e.g. '-g in2' is identical to '-g in2.g' (same channel)
  if (tmp.IsEmpty())
  {
    out_mapping.m_ChannelValue = (xiiTextureConverterChannelValue::Enum)((xiiInt32)xiiTextureConverterChannelValue::Red + iChannelIndex);
    return XII_SUCCESS;
  }

  if (!tmp.StartsWith("."))
  {
    xiiLog::Error("Invalid channel mapping: Expected '.' after input file index in '{0}'", sCfg);
    return XII_FAILURE;
  }

  tmp.Shrink(1, 0);

  if (!bSingleChannel)
  {
    // in case of '-rgb in1.bgr' map r to b, g to g, b to r, etc.
    // in case of '-rgb in1.r' map everything to the same input
    if (tmp.GetCharacterCount() > 1)
      tmp.Shrink(iChannelIndex, 0);
  }

  // no additional info, e.g. '-rgb in2.rg'
  if (tmp.IsEmpty())
  {
    xiiLog::Error("Invalid channel mapping: Too few channel identifiers '{0}'", sCfg);
    return XII_FAILURE;
  }

  {
    const xiiUInt32 uiChar = tmp.GetIteratorFront().GetCharacter();

    if (uiChar == 'r')
    {
      out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::Red;
    }
    else if (uiChar == 'g')
    {
      out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::Green;
    }
    else if (uiChar == 'b')
    {
      out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::Blue;
    }
    else if (uiChar == 'a')
    {
      out_mapping.m_ChannelValue = xiiTextureConverterChannelValue::Alpha;
    }
    else
    {
      xiiLog::Error("Invalid channel mapping: Unexpected channel identifier in '{}'", sCfg);
      return XII_FAILURE;
    }

    tmp.Shrink(1, 0);
  }

  return XII_SUCCESS;
}
