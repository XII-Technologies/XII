/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#ifdef XII_USE_QT

#  include <Foundation/Containers/DynamicArray.h>
#  include <Foundation/Types/Status.h>
#  include <Foundation/Types/Variant.h>

#  include <QWidget>

class QFormLayout;

class xiiMCPArgumentForm final : public QWidget
{
public:
  xiiMCPArgumentForm();

  void                    SetSchema(const xiiVariantDictionary& schema);
  [[nodiscard]] xiiStatus ReadArguments(xiiVariantDictionary& out_arguments) const;

private:
  struct Field
  {
    xiiString m_sName;
    xiiString m_sType;
    QWidget*  m_pControl  = nullptr;
    bool      m_bRequired = false;
  };

  QFormLayout*           m_pForm = nullptr;
  xiiDynamicArray<Field> m_Fields;
};

#endif
