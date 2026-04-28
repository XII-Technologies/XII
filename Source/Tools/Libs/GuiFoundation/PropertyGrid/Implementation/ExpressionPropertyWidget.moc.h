/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <GuiFoundation/PropertyGrid/Implementation/PropertyWidget.moc.h>

class XII_GUIFOUNDATION_DLL xiiQtPropertyEditorExpressionWidget : public xiiQtStandardPropertyWidget
{
  Q_OBJECT

public:
  xiiQtPropertyEditorExpressionWidget();
  ~xiiQtPropertyEditorExpressionWidget();

protected Q_SLOTS:
  void on_TextChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const xiiVariant& value) override;

protected:
  QHBoxLayout*        m_pLayout;
  QTextEdit*          m_pWidget;
  QSyntaxHighlighter* m_pHighlighter = nullptr;
};

//////////////////////////////////////////////////////////////////////////

struct ExpressionTokenType
{
  enum Enum
  {
    Comment,
    Number,
    Bracket,
    Type,
    BuiltIn,

    Count,
  };
};

class ExpressionHighlighter : public QSyntaxHighlighter
{
public:
  ExpressionHighlighter(QTextDocument* pParent = 0);

protected:
  void highlightBlock(const QString& text) override;

private:
  QColor m_Colors[ExpressionTokenType::Count];
};
