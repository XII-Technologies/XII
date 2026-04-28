/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Dialogs/ExtractGeometryDlg.moc.h>
#include <GraphicsCore/Utils/WorldGeoExtractionUtil.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

#include <QFileDialog>

QString xiiQtExtractGeometryDlg::s_sDestinationFile;
bool    xiiQtExtractGeometryDlg::s_bOnlySelection    = false;
int     xiiQtExtractGeometryDlg::s_iExtractionMode   = (int)xiiWorldGeoExtractionUtil::ExtractionMode::RenderMesh;
int     xiiQtExtractGeometryDlg::s_iCoordinateSystem = 1;

xiiQtExtractGeometryDlg::xiiQtExtractGeometryDlg(QWidget* pParent)

  :
  QDialog(pParent)
{
  setupUi(this);

  ExtractionMode->clear();
  ExtractionMode->addItem("Render Mesh");
  ExtractionMode->addItem("Collision Mesh");
  ExtractionMode->addItem("Navmesh Obstacles");

  CoordinateSystem->clear();
  CoordinateSystem->addItem("Forward: +X, Right: +Y, Up: +Z (xii)");
  CoordinateSystem->addItem("Forward: -Z, Right: +X, Up: +Y (OpenGL/Maya)");
  CoordinateSystem->addItem("Forward: +Z, Right: +X, Up: +Y (D3D)");

  UpdateUI();
}

void xiiQtExtractGeometryDlg::UpdateUI()
{
  DestinationFile->setText(s_sDestinationFile);
  ExtractOnlySelection->setChecked(s_bOnlySelection);
  ExtractionMode->setCurrentIndex(s_iExtractionMode);
  CoordinateSystem->setCurrentIndex(s_iCoordinateSystem);
}

void xiiQtExtractGeometryDlg::QueryUI()
{
  s_sDestinationFile  = DestinationFile->text();
  s_bOnlySelection    = ExtractOnlySelection->isChecked();
  s_iExtractionMode   = ExtractionMode->currentIndex();
  s_iCoordinateSystem = CoordinateSystem->currentIndex();
}

void xiiQtExtractGeometryDlg::on_ButtonBox_clicked(QAbstractButton* button)
{
  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Ok))
  {
    QueryUI();

    if (!xiiPathUtils::IsAbsolutePath(s_sDestinationFile.toUtf8().data()))
    {
      xiiQtUiServices::GetSingleton()->MessageBoxWarning("Only absolute paths are allowed for the destination file.");
      return;
    }

    accept();
    return;
  }

  if (button == ButtonBox->button(QDialogButtonBox::StandardButton::Cancel))
  {
    reject();
    return;
  }
}

void xiiQtExtractGeometryDlg::on_BrowseButton_clicked()
{
  QString allFilters = "OBJ (*.obj)";
  QString sFile      = QFileDialog::getSaveFileName(QApplication::activeWindow(), QLatin1String("Destination file"), s_sDestinationFile, allFilters,
                                                    nullptr, QFileDialog::Option::DontResolveSymlinks);

  if (sFile.isEmpty())
    return;

  DestinationFile->setText(sFile);
}

xiiMat3 xiiQtExtractGeometryDlg::GetCoordinateSystemTransform()
{
  xiiMat3 m;
  m.SetIdentity();

  switch (s_iCoordinateSystem)
  {
    case 0:
      break;

    case 1:
      m.SetRow(2, xiiVec3(-1, 0, 0)); // forward
      m.SetRow(1, xiiVec3(0, 0, 1));  // up
      m.SetRow(0, xiiVec3(0, 1, 0));  // right
      break;

    case 2:
      m.SetRow(2, xiiVec3(1, 0, 0)); // forward
      m.SetRow(1, xiiVec3(0, 0, 1)); // up
      m.SetRow(0, xiiVec3(0, 1, 0)); // right
      break;
  }

  return m;
}
