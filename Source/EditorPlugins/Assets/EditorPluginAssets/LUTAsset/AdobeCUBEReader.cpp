
#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/LUTAsset/AdobeCUBEReader.h>
#include <Foundation/CodeUtils/Tokenizer.h>

// This file implements a simple reader for the Adobe CUBE LUT file format.
// The specification can be found here (at the time of this writing):
// https://wwwimages2.adobe.com/content/dam/acom/en/products/speedgrade/cc/pdfs/cube-lut-specification-1.0.pdf

namespace
{
  bool GetVec3FromLine(xiiHybridArray<const xiiToken*, 32> line, xiiUInt32 uiSkip, xiiVec3& ref_vOut)
  {
    if (line.GetCount() < (uiSkip + 3 + 2))
    {
      return false;
    }

    if ((line[uiSkip + 0]->m_iType != xiiTokenType::Float && line[uiSkip + 0]->m_iType != xiiTokenType::Integer) ||
        line[uiSkip + 1]->m_iType != xiiTokenType::Whitespace ||
        (line[uiSkip + 2]->m_iType != xiiTokenType::Float && line[uiSkip + 2]->m_iType != xiiTokenType::Integer) ||
        line[uiSkip + 3]->m_iType != xiiTokenType::Whitespace ||
        (line[uiSkip + 4]->m_iType != xiiTokenType::Float && line[uiSkip + 4]->m_iType != xiiTokenType::Integer))
    {
      return false;
    }

    double    res  = 0;
    xiiString sVal = line[uiSkip + 0]->m_DataView;

    if (xiiConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.x = static_cast<float>(res);

    sVal = line[uiSkip + 2]->m_DataView;
    if (xiiConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.y = static_cast<float>(res);


    sVal = line[uiSkip + 4]->m_DataView;
    if (xiiConversionUtils::StringToFloat(sVal, res).Failed())
      return false;

    ref_vOut.z = static_cast<float>(res);

    return true;
  }
} // namespace

xiiAdobeCUBEReader::xiiAdobeCUBEReader()  = default;
xiiAdobeCUBEReader::~xiiAdobeCUBEReader() = default;

xiiStatus xiiAdobeCUBEReader::ParseFile(xiiStreamReader& inout_stream, xiiLogInterface* pLog /*= nullptr*/)
{
  xiiString sContent;
  sContent.ReadAll(inout_stream);

  xiiTokenizer tokenizer;
  tokenizer.SetTreatHashSignAsLineComment(true);

  tokenizer.Tokenize(
    xiiArrayPtr<const xiiUInt8>((const xiiUInt8*)sContent.GetData(), sContent.GetElementCount()), pLog ? pLog : xiiLog::GetThreadLocalLogSystem());


  auto tokens = tokenizer.GetTokens();

  xiiHybridArray<const xiiToken*, 32> line;
  xiiUInt32                           firstToken = 0;

  while (tokenizer.GetNextLine(firstToken, line).Succeeded())
  {
    if (line[0]->m_iType == xiiTokenType::LineComment || line[0]->m_iType == xiiTokenType::Newline)
      continue;

    if (line[0]->m_DataView == "TITLE")
    {
      if (line.GetCount() < 3)
      {
        return xiiStatus(xiiFmt("LUT file has invalid TITLE line."));
      }

      if (line[1]->m_iType != xiiTokenType::Whitespace && line[2]->m_iType != xiiTokenType::String1)
      {
        return xiiStatus(xiiFmt("LUT file has invalid TITLE line, expected TITLE<whitespace>\"<string>\"."));
      }

      m_sTitle = line[2]->m_DataView;

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MIN")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMin))
      {
        return xiiStatus(xiiFmt("LUT file has invalid DOMAIN_MIN line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "DOMAIN_MAX")
    {
      if (!::GetVec3FromLine(line, 2, m_vDomainMax))
      {
        return xiiStatus(xiiFmt("LUT file has invalid DOMAIN_MAX line."));
      }

      continue;
    }
    else if (line[0]->m_DataView == "LUT_1D_SIZE")
    {
      return xiiStatus(xiiFmt("LUT file specifies a 1D LUT which is currently not implemented."));
    }
    else if (line[0]->m_DataView == "LUT_3D_SIZE")
    {
      if (m_uiLUTSize > 0)
      {
        return xiiStatus(xiiFmt("LUT file has more than one LUT_3D_SIZE entry. Aborting parse."));
      }

      if (line.GetCount() < 3)
      {
        return xiiStatus(xiiFmt("LUT file has invalid LUT_3D_SIZE line."));
      }

      if (line[1]->m_iType != xiiTokenType::Whitespace && line[2]->m_iType != xiiTokenType::Integer)
      {
        return xiiStatus(xiiFmt("LUT file has invalid LUT_3D_SIZE line, expected LUT_3D_SIZE<whitespace><N>."));
      }

      const xiiString sVal = line[2]->m_DataView;
      if (xiiConversionUtils::StringToUInt(sVal, m_uiLUTSize).Failed())
      {
        return xiiStatus(xiiFmt("LUT file has invalid LUT_3D_SIZE line, couldn't parse LUT size as xiiUInt32."));
      }

      if (m_uiLUTSize < 2 || m_uiLUTSize > 256)
      {
        return xiiStatus(xiiFmt("LUT file has invalid LUT_3D_SIZE size, got {0} - but must be in range 2, 256.", m_uiLUTSize));
      }

      m_LUTValues.Reserve(m_uiLUTSize * m_uiLUTSize * m_uiLUTSize);

      continue;
    }

    if (line[0]->m_iType == xiiTokenType::Float || line[0]->m_iType == xiiTokenType::Integer)
    {
      if (m_uiLUTSize == 0)
      {
        return xiiStatus(xiiFmt("LUT data before LUT size was specified."));
      }

      xiiVec3 lineValues;
      if (!::GetVec3FromLine(line, 0, lineValues))
      {
        return xiiStatus(xiiFmt("LUT data couldn't be read."));
      }

      m_LUTValues.PushBack(lineValues);
    }
  }

  if (m_vDomainMin.x > m_vDomainMax.x || m_vDomainMin.y > m_vDomainMax.y || m_vDomainMin.z > m_vDomainMax.z)
  {
    return xiiStatus("LUT file has invalid domain min/max values.");
  }

  if (m_LUTValues.GetCount() != (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize))
  {
    return xiiStatus(xiiFmt("LUT data incomplete, read {0} values but expected {1} values given a LUT size of {2}.", m_LUTValues.GetCount(),
                            (m_uiLUTSize * m_uiLUTSize * m_uiLUTSize), m_uiLUTSize));
  }

  return XII_SUCCESS;
}

xiiVec3 xiiAdobeCUBEReader::GetDomainMin() const
{
  return m_vDomainMin;
}

xiiVec3 xiiAdobeCUBEReader::GetDomainMax() const
{
  return m_vDomainMax;
}

xiiUInt32 xiiAdobeCUBEReader::GetLUTSize() const
{
  return m_uiLUTSize;
}

const xiiString& xiiAdobeCUBEReader::GetTitle() const
{
  return m_sTitle;
}

xiiVec3 xiiAdobeCUBEReader::GetLUTEntry(xiiUInt32 r, xiiUInt32 g, xiiUInt32 b) const
{
  return m_LUTValues[GetLUTIndex(r, g, b)];
}

xiiUInt32 xiiAdobeCUBEReader::GetLUTIndex(xiiUInt32 r, xiiUInt32 g, xiiUInt32 b) const
{
  return b * m_uiLUTSize * m_uiLUTSize + g * m_uiLUTSize + r;
}
