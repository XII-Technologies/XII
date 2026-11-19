/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <MCP/MCPPCH.h>

#ifdef XII_USE_QT

#  include <MCP/Panels/ArgumentForm.h>
#  include <MCPPlugin/Json.h>

#  include <QComboBox>
#  include <QFormLayout>
#  include <QLabel>
#  include <QLineEdit>

namespace
{
  QString FormText(xiiStringView sText)
  {
    return QString::fromUtf8(sText.GetStartPointer(), static_cast<qsizetype>(sText.GetElementCount()));
  }
} // namespace

xiiMCPArgumentForm::xiiMCPArgumentForm()
{
  m_pForm = new QFormLayout(this);
  m_pForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
}

void xiiMCPArgumentForm::SetSchema(const xiiVariantDictionary& schema)
{
  m_Fields.Clear();
  while (m_pForm->rowCount() != 0)
  {
    m_pForm->removeRow(0);
  }
  const auto* pProperties = xiiMCPJson::GetObject(schema, "properties");
  if (pProperties == nullptr || pProperties->IsEmpty())
  {
    m_pForm->addRow(new QLabel("This capability declares no argument fields."));
    return;
  }

  const auto*                pRequired = schema.GetValue("required");
  xiiDynamicArray<xiiString> names;
  for (auto it = pProperties->GetIterator(); it.IsValid(); ++it)
  {
    names.PushBack(it.Key());
  }
  names.Sort();
  for (const auto& sName : names)
  {
    const auto* pProperty = xiiMCPJson::GetObject(*pProperties, sName);
    if (pProperty == nullptr)
    {
      continue;
    }
    Field field;
    field.m_sName = sName;
    field.m_sType = xiiMCPJson::GetString(*pProperty, "type");
    if (pRequired != nullptr && pRequired->IsA<xiiVariantArray>())
    {
      for (const auto& required : pRequired->Get<xiiVariantArray>())
      {
        if (required.IsA<xiiString>() && required.Get<xiiString>() == sName)
        {
          field.m_bRequired = true;
          break;
        }
      }
    }

    const auto* pEnum = pProperty->GetValue("enum");
    if (field.m_sType == "boolean" || (pEnum != nullptr && pEnum->IsA<xiiVariantArray>()))
    {
      auto* pChoices = new QComboBox();
      pChoices->addItem(field.m_bRequired ? "Select a value" : "Omit");
      if (field.m_sType == "boolean")
      {
        pChoices->addItem("True", QString("true"));
        pChoices->addItem("False", QString("false"));
      }
      else
      {
        for (const auto& value : pEnum->Get<xiiVariantArray>())
        {
          xiiStringBuilder sValue;
          if (xiiMCPJson::Write(value, sValue, 4096).Succeeded())
          {
            pChoices->addItem(value.IsA<xiiString>() ? FormText(value.Get<xiiString>()) : FormText(sValue), FormText(sValue));
          }
        }
      }
      field.m_pControl = pChoices;
    }
    else
    {
      auto* pText = new QLineEdit();
      if (field.m_sType == "array" || field.m_sType == "object" || field.m_sType.IsEmpty())
      {
        pText->setPlaceholderText("JSON value");
      }
      else if (field.m_sType != "string")
      {
        pText->setPlaceholderText(FormText(field.m_sType));
      }
      field.m_pControl = pText;
    }
    field.m_pControl->setToolTip(FormText(xiiMCPJson::GetString(*pProperty, "description")));
    m_pForm->addRow(FormText(sName) + (field.m_bRequired ? " *" : ""), field.m_pControl);
    m_Fields.PushBack(field);
  }
}

xiiStatus xiiMCPArgumentForm::ReadArguments(xiiVariantDictionary& out_arguments) const
{
  xiiVariantDictionary arguments;
  for (const auto& field : m_Fields)
  {
    const auto* pChoice = qobject_cast<QComboBox*>(field.m_pControl);
    const auto  input   = pChoice != nullptr ? pChoice->currentData().toString().toUtf8() : qobject_cast<QLineEdit*>(field.m_pControl)->text().toUtf8();
    if (input.isEmpty())
    {
      if (field.m_bRequired)
      {
        return xiiStatus(xiiFmt("Enter a value for '{}'.", field.m_sName));
      }
      continue;
    }
    const xiiStringView sInput(input.constData(), static_cast<xiiUInt32>(input.size()));
    xiiVariant          value;
    if (pChoice == nullptr && field.m_sType == "string")
    {
      value = xiiString(sInput);
    }
    else if (xiiMCPJson::Parse(sInput, value).Failed() ||
             (field.m_sType == "object" && !value.IsA<xiiVariantDictionary>()) ||
             (field.m_sType == "array" && !value.IsA<xiiVariantArray>()) ||
             (field.m_sType == "number" && (!value.IsNumber() || value.IsA<bool>())) ||
             (field.m_sType == "integer" && !value.IsA<xiiUInt64>() && !value.IsA<xiiInt64>()))
    {
      return xiiStatus(xiiFmt("The value of '{}' does not match its schema.", field.m_sName));
    }
    arguments[field.m_sName] = value;
  }
  out_arguments = std::move(arguments);
  return XII_SUCCESS;
}

#endif
