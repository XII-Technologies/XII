#pragma once

#include <Foundation/Strings/String.h>
#include <RendererCore/Declarations.h>

namespace xiiShaderHelper
{
  class XII_RENDERERCORE_DLL xiiTextSectionizer
  {
  public:
    void Clear();

    void AddSection(const char* szName);

    void Process(const char* szText);

    xiiStringView GetSectionContent(xiiUInt32 uiSection, xiiUInt32& out_uiFirstLine) const;

  private:
    struct xiiTextSection
    {
      xiiTextSection(const char* szName)
      {
        m_sName          = szName;
        m_szSectionStart = nullptr;
        m_uiFirstLine    = 0;
      }

      void Reset()
      {
        m_szSectionStart = nullptr;
        m_Content        = xiiStringView();
        m_uiFirstLine    = 0;
      }

      xiiString     m_sName;
      const char*   m_szSectionStart;
      xiiStringView m_Content;
      xiiUInt32     m_uiFirstLine;
    };

    xiiStringBuilder                   m_sText;
    xiiHybridArray<xiiTextSection, 16> m_Sections;
  };

  struct xiiShaderSections
  {
    enum Enum
    {
      PLATFORMS,
      PERMUTATIONS,
      MATERIALPARAMETER,
      RENDERSTATE,
      SHADER,
      VERTEXSHADER,
      PIXELSHADER,
      GEOMETRYSHADER,
      HULLSHADER,
      DOMAINSHADER,
      COMPUTESHADER,
      AMPLIFICATIONSHADER,
      MESHSHADER,
      RAYGENSHADER,
      RAYMISSSHADER,
      RAYANYHITSHADER,
      RAYCLOSESTHITSHADER,
      RAYINTERSECTIONSHADER,
      CALLABLESHADER,
      TEMPLATE_VARS
    };
  };

  XII_RENDERERCORE_DLL void GetShaderSections(const char* szContent, xiiTextSectionizer& out_sections);

  xiiUInt32 CalculateHash(const xiiArrayPtr<xiiPermutationVar>& vars);
} // namespace xiiShaderHelper
