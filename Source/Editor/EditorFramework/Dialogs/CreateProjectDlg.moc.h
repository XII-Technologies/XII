#include <EditorFramework/EditorFrameworkDLL.h>

#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/ui_CreateProjectDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class XII_EDITORFRAMEWORK_DLL xiiQtCreateProjectDlg : public QDialog, public Ui_xiiQtCreateProjectDlg
{
public:
  Q_OBJECT

public:
  xiiQtCreateProjectDlg(QWidget* pParent);

  xiiString GetFullTargetPath() const;

  xiiString m_sTargetFolder;
  xiiString m_sTargetName;

private Q_SLOTS:
  void on_BrowseFolder_clicked();
  void on_ProjectName_textChanged(QString text);
  void on_Prev_clicked();
  void on_Next_clicked();

private:
  void UpdateUI();
  void FindProjectTemplates(xiiDynamicArray<xiiString>& out_Projects);
  void FillProjectTemplatesList();
  void CreateProject();

  enum class State
  {
    Basics,
    Templates,
    Plugins,
    Summary,
    Create,
  };

  State              m_State = State::Basics;
  xiiString          m_sProjectTemplate;
  xiiPluginBundleSet m_LocalPluginSet;
};
