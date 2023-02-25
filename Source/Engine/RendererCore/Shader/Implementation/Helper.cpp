#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Shader/Implementation/Helper.h>

namespace xiiShaderHelper
{

  void xiiTextSectionizer::Clear()
  {
    m_Sections.Clear();
    m_sText.Clear();
  }

  void xiiTextSectionizer::AddSection(const char* szName) { m_Sections.PushBack(xiiTextSection(szName)); }

  void xiiTextSectionizer::Process(const char* szText)
  {
    for (xiiUInt32 i = 0; i < m_Sections.GetCount(); ++i)
      m_Sections[i].Reset();

    m_sText = szText;


    for (xiiUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName.GetData());

      if (m_Sections[s].m_szSectionStart != nullptr)
        m_Sections[s].m_Content = xiiStringView(m_Sections[s].m_szSectionStart + m_Sections[s].m_sName.GetElementCount());
    }

    for (xiiUInt32 s = 0; s < m_Sections.GetCount(); ++s)
    {
      if (m_Sections[s].m_szSectionStart == nullptr)
        continue;

      xiiUInt32 uiLine = 1;

      const char* sz = m_sText.GetData();
      while (sz < m_Sections[s].m_szSectionStart)
      {
        if (*sz == '\n')
          ++uiLine;

        ++sz;
      }

      m_Sections[s].m_uiFirstLine = uiLine;

      for (xiiUInt32 s2 = 0; s2 < m_Sections.GetCount(); ++s2)
      {
        if (s == s2)
          continue;

        if (m_Sections[s2].m_szSectionStart > m_Sections[s].m_szSectionStart)
        {
          const char* szContentStart = m_Sections[s].m_Content.GetStartPointer();
          const char* szSectionEnd   = xiiMath::Min(m_Sections[s].m_Content.GetEndPointer(), m_Sections[s2].m_szSectionStart);

          m_Sections[s].m_Content = xiiStringView(szContentStart, szSectionEnd);
        }
      }
    }
  }

  xiiStringView xiiTextSectionizer::GetSectionContent(xiiUInt32 uiSection, xiiUInt32& out_uiFirstLine) const
  {
    out_uiFirstLine = m_Sections[uiSection].m_uiFirstLine;
    return m_Sections[uiSection].m_Content;
  }

  void GetShaderSections(const char* szContent, xiiTextSectionizer& out_Sections)
  {
    out_Sections.Clear();

    out_Sections.AddSection("[PLATFORMS]");
    out_Sections.AddSection("[PERMUTATIONS]");
    out_Sections.AddSection("[MATERIALPARAMETER]");
    out_Sections.AddSection("[RENDERSTATE]");
    out_Sections.AddSection("[SHADER]");
    out_Sections.AddSection("[VERTEXSHADER]");
    out_Sections.AddSection("[HULLSHADER]");
    out_Sections.AddSection("[DOMAINSHADER]");
    out_Sections.AddSection("[GEOMETRYSHADER]");
    out_Sections.AddSection("[PIXELSHADER]");
    out_Sections.AddSection("[COMPUTESHADER]");
    out_Sections.AddSection("[TEMPLATE_VARS]");

    out_Sections.Process(szContent);
  }

  xiiUInt32 CalculateHash(const xiiArrayPtr<xiiPermutationVar>& vars)
  {
    xiiHybridArray<xiiUInt64, 128> buffer;
    buffer.SetCountUninitialized(vars.GetCount() * 2);

    for (xiiUInt32 i = 0; i < vars.GetCount(); ++i)
    {
      auto& var         = vars[i];
      buffer[i * 2 + 0] = var.m_sName.GetHash();
      buffer[i * 2 + 1] = var.m_sValue.GetHash();
    }

    auto bytes = buffer.GetByteArrayPtr();
    return xiiHashingUtils::xxHash32(bytes.GetPtr(), bytes.GetCount());
  }
} // namespace xiiShaderHelper


XII_STATICLINK_FILE(RendererCore, RendererCore_Shader_Implementation_Helper);
