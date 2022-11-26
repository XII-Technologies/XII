#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>
#include <EditorPluginAssets/ui_ShaderTemplateDlg.h>
#include <QDialog>

class xiiDocument;

class xiiQtShaderTemplateDlg : public QDialog, public Ui_ShaderTemplateDlg
{
  Q_OBJECT

public:
  xiiQtShaderTemplateDlg(QWidget* parent, const xiiDocument* pDoc);

  xiiString m_sResult;

private Q_SLOTS:
  void on_Buttons_accepted();
  void on_Buttons_rejected();
  void on_Browse_clicked();
  void on_ShaderTemplate_currentIndexChanged(int idx);

private:
  struct Template
  {
    xiiString                     m_sName;
    xiiString                     m_sPath;
    xiiString                     m_sContent;
    xiiHybridArray<xiiString, 16> m_Vars;
  };

  xiiHybridArray<Template, 32> m_Templates;
};
