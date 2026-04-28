/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <GraphicsFoundation/GraphicsFoundationPCH.h>

#include <GraphicsFoundation/ShaderCompiler/ShaderTextSectionizer.h>

void xiiGALShaderTextSectionizer::Clear()
{
  m_Sections.Clear();
  m_sText.Clear();
}

void xiiGALShaderTextSectionizer::AddSection(xiiStringView sName)
{
  m_Sections.PushBack(xiiTextSection(sName));
}

void xiiGALShaderTextSectionizer::Process(xiiStringView sText)
{
  for (xiiUInt32 i = 0; i < m_Sections.GetCount(); ++i)
    m_Sections[i].Reset();

  m_sText = sText;

  for (xiiUInt32 s = 0; s < m_Sections.GetCount(); ++s)
  {
    m_Sections[s].m_szSectionStart = m_sText.FindSubString_NoCase(m_Sections[s].m_sName);

    if (m_Sections[s].m_szSectionStart != nullptr)
    {
      m_Sections[s].m_sContent = xiiStringView(m_Sections[s].m_szSectionStart + m_Sections[s].m_sName.GetElementCount());
    }
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
        const char* szContentStart = m_Sections[s].m_sContent.GetStartPointer();
        const char* szSectionEnd   = xiiMath::Min(m_Sections[s].m_sContent.GetEndPointer(), m_Sections[s2].m_szSectionStart);

        m_Sections[s].m_sContent = xiiStringView(szContentStart, szSectionEnd);
      }
    }
  }
}

xiiStringView xiiGALShaderTextSectionizer::GetSectionContent(xiiUInt32 uiSection, xiiUInt32& out_uiFirstLine) const
{
  out_uiFirstLine = m_Sections[uiSection].m_uiFirstLine;
  return m_Sections[uiSection].m_sContent;
}

void xiiGALShaderSections::GetShaderSections(xiiStringView sContent, xiiGALShaderTextSectionizer& out_sections)
{
  out_sections.Clear();

  out_sections.AddSection("[PLATFORMS]");
  out_sections.AddSection("[PERMUTATIONS]");
  out_sections.AddSection("[MATERIAL_CONSTANTS]");
  out_sections.AddSection("[MATERIAL_PARAMETER]");
  out_sections.AddSection("[MATERIAL_CONFIGURATION]");
  out_sections.AddSection("[RENDERSTATE]");
  out_sections.AddSection("[SHADER]");
  out_sections.AddSection("[VERTEX_SHADER]");
  out_sections.AddSection("[PIXEL_SHADER]");
  out_sections.AddSection("[GEOMETRY_SHADER]");
  out_sections.AddSection("[HULL_SHADER]");
  out_sections.AddSection("[DOMAIN_SHADER]");
  out_sections.AddSection("[COMPUTE_SHADER]");
  out_sections.AddSection("[AMPLIFICATION_SHADER]");
  out_sections.AddSection("[MESH_SHADER]");
  out_sections.AddSection("[RAY_GENERATION_SHADER]");
  out_sections.AddSection("[RAY_MISS_SHADER]");
  out_sections.AddSection("[RAY_CLOSEST_HIT_SHADER]");
  out_sections.AddSection("[RAY_ANY_HIT_SHADER]");
  out_sections.AddSection("[RAY_INTERSECTION_SHADER]");
  out_sections.AddSection("[CALLABLE_SHADER]");
  out_sections.AddSection("[TILE_SHADER]");
  out_sections.AddSection("[TEMPLATE_VARIABLES]");

  out_sections.Process(sContent);
}
