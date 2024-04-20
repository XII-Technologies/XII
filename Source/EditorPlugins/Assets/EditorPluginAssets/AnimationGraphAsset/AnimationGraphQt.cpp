#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphQt.h>

xiiQtAnimationGraphNode::xiiQtAnimationGraphNode() = default;

void xiiQtAnimationGraphNode::UpdateState()
{
  xiiQtNode::UpdateState();

  xiiStringBuilder sTitle;

  const xiiRTTI* pRtti = GetObject()->GetType();

  xiiVariant customTitle = GetObject()->GetTypeAccessor().GetValue("CustomTitle");
  if (customTitle.IsValid() && customTitle.CanConvertTo<xiiString>())
  {
    sTitle = customTitle.ConvertTo<xiiString>();
  }

  if (sTitle.IsEmpty())
  {
    if (const xiiTitleAttribute* pAttr = pRtti->GetAttributeByType<xiiTitleAttribute>())
    {
      sTitle = pAttr->GetTitle();

      xiiStringBuilder tmp, tmp2;
      xiiVariant       val;

      // replace enum properties with translated strings
      {
        xiiHybridArray<const xiiAbstractProperty*, 32> properties;
        pRtti->GetAllProperties(properties);

        for (const auto& prop : properties)
        {
          if (prop->GetSpecificType()->IsDerivedFrom<xiiEnumBase>() || prop->GetSpecificType()->IsDerivedFrom<xiiBitflagsBase>())
          {
            val = GetObject()->GetTypeAccessor().GetValue(prop->GetPropertyName());

            xiiReflectionUtils::EnumerationToString(prop->GetSpecificType(), val.ConvertTo<xiiInt64>(), tmp);

            tmp2.Set("{", prop->GetPropertyName(), "}");
            sTitle.ReplaceAll(tmp2, xiiTranslate(tmp));
          }
        }
      }

      // replace the rest
      while (true)
      {
        const char* szOpen = sTitle.FindSubString("{");

        if (szOpen == nullptr)
          break;

        const char* szClose = sTitle.FindSubString("}", szOpen);

        if (szClose == nullptr)
          break;

        tmp.SetSubString_FromTo(szOpen + 1, szClose);

        // three array indices should be enough for everyone
        if (tmp.TrimWordEnd("[0]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 0);
        else if (tmp.TrimWordEnd("[1]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 1);
        else if (tmp.TrimWordEnd("[2]"))
          val = GetObject()->GetTypeAccessor().GetValue(tmp, 2);
        else
          val = GetObject()->GetTypeAccessor().GetValue(tmp);

        if (val.IsValid())
        {

          tmp.SetFormat("{}", val);

          if (xiiConversionUtils::IsStringUuid(tmp))
          {
            if (auto pAsset = xiiAssetCurator::GetSingleton()->FindSubAsset(tmp))
            {
              tmp = pAsset->GetName();
            }
          }

          sTitle.ReplaceSubString(szOpen, szClose + 1, tmp);
        }
        else
        {
          sTitle.ReplaceSubString(szOpen, szClose + 1, "");
        }
      }
    }
  }

  sTitle.ReplaceAll("''", "");
  sTitle.ReplaceAll("\"\"", "");
  sTitle.ReplaceAll("  ", " ");
  sTitle.Trim(" ");

  if (sTitle.GetCharacterCount() > 30)
  {
    sTitle.Shrink(0, sTitle.GetCharacterCount() - 31);
    sTitle.Append("...");
  }

  if (!sTitle.IsEmpty())
  {
    m_pTitleLabel->setPlainText(sTitle.GetData());
  }
}
