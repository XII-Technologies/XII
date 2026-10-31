/// Copyright (c) Theophilus Eriata. All Rights Reserved.

#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAsset.h>
#include <EditorPluginAssets/MaterialAsset/MaterialAssetWindow.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QPushButton>
#include <QSignalBlocker>

void xiiQtMaterialAssetDocumentWindow::CreatePreviewControls()
{
  auto* panel = new xiiQtDocumentPanel(GetContainerWindow()->GetDockManager(), this, GetDocument());
  panel->setObjectName("MaterialPreviewControls");
  panel->setWindowTitle("Preview Scene and Lighting");
  auto* widget = new QWidget(panel);
  auto* form   = new QFormLayout(widget);
  auto* preset = new QComboBox(widget);
  preset->setObjectName("MaterialPreviewLightingPreset");
  preset->addItems({"Neutral Studio", "Outdoor Daylight", "Low-Light Studio", "Unlit / Diagnostic"});
  form->addRow("Lighting preset", preset);

  const auto number = [form, widget](const char* label, double low, double high, double value, const char* suffix) {
    auto* spin = new QDoubleSpinBox(widget);
    spin->setRange(low, high);
    spin->setDecimals(2);
    spin->setValue(value);
    spin->setSuffix(suffix);
    spin->setKeyboardTracking(false);
    form->addRow(label, spin);
    return spin;
  };
  auto* intensity    = number("Illuminance", 0, 200000, m_PreviewLighting.m_fIlluminance, " lx");
  auto* ambient      = number("Ambient radiance", 0, 10000, m_PreviewLighting.m_fAmbientRadiance, "");
  auto* yaw          = number("Light azimuth", -180, 180, 93, " deg");
  auto* pitch        = number("Light elevation", -89, 89, 43, " deg");
  auto* temperature  = number("Color temperature", 1000, 40000, m_PreviewLighting.m_uiTemperature, " K");
  auto* compensation = number("Exposure compensation", -20, 20, 0, " EV");
  auto* color        = new QPushButton("Choose Light Color", widget);
  form->addRow(color);
  auto* sky     = new QCheckBox("Atmosphere background", widget);
  auto* shadows = new QCheckBox("Shadows", widget);
  shadows->setChecked(true);
  auto* automatic = new QCheckBox("Automatic exposure", widget);
  form->addRow(sky);
  form->addRow(shadows);
  form->addRow(automatic);

  connect(intensity, &QDoubleSpinBox::valueChanged, this, [this](double value) { m_PreviewLighting.m_fIlluminance = static_cast<float>(value); });
  connect(ambient, &QDoubleSpinBox::valueChanged, this, [this](double value) { m_PreviewLighting.m_fAmbientRadiance = static_cast<float>(value); });
  const auto rotate = [this, yaw, pitch](double) {
    m_PreviewLighting.m_Rotation = xiiQuat::MakeFromEulerAngles(xiiAngle::MakeFromDegree(-15), xiiAngle::MakeFromDegree(static_cast<float>(pitch->value())), xiiAngle::MakeFromDegree(static_cast<float>(yaw->value())));
  };
  connect(yaw, &QDoubleSpinBox::valueChanged, this, rotate);
  connect(pitch, &QDoubleSpinBox::valueChanged, this, rotate);
  connect(temperature, &QDoubleSpinBox::valueChanged, this, [this](double value) { m_PreviewLighting.m_uiTemperature = static_cast<xiiUInt32>(value); });
  connect(compensation, &QDoubleSpinBox::valueChanged, this, [this](double value) { m_PreviewLighting.m_fExposureCompensation = static_cast<float>(value); });
  connect(sky, &QCheckBox::toggled, this, [this](bool value) { m_PreviewLighting.m_bSky = value; });
  connect(shadows, &QCheckBox::toggled, this, [this](bool value) { m_PreviewLighting.m_bShadows = value; });
  connect(automatic, &QCheckBox::toggled, this, [this](bool value) { m_PreviewLighting.m_bAutomaticExposure = value; });
  connect(color, &QPushButton::clicked, this, [this, widget]() {
    const xiiColorGammaUB current(m_PreviewLighting.m_Color);
    const QColor          selected = QColorDialog::getColor(QColor(current.r, current.g, current.b), widget, "Preview Light Color");
    if (selected.isValid())
      m_PreviewLighting.m_Color = xiiColorGammaUB(selected.red(), selected.green(), selected.blue());
  });

  auto* diagnostic = new QComboBox(widget);
  diagnostic->setObjectName("MaterialPreviewDiagnosticView");
  diagnostic->addItem("Final shading", xiiViewRenderMode::None);
  diagnostic->addItem("Base color", xiiViewRenderMode::BaseColor);
  diagnostic->addItem("World normal", xiiViewRenderMode::NormalMap);
  diagnostic->addItem("Roughness", xiiViewRenderMode::Roughness);
  diagnostic->addItem("Metallic", xiiViewRenderMode::Metallic);
  form->addRow("View", diagnostic);
  connect(diagnostic, &QComboBox::currentIndexChanged, this, [this, diagnostic](int) { m_ViewConfig.m_RenderMode = static_cast<xiiViewRenderMode::Enum>(diagnostic->currentData().toInt()); });

  const auto applyPreset = [this, intensity, ambient, yaw, pitch, temperature, compensation, sky, shadows, automatic, diagnostic](int index) {
    m_PreviewLighting = xiiPreviewLightingSettings::MakePreset(static_cast<xiiPreviewLightingSettings::Preset>(index));
    intensity->setValue(m_PreviewLighting.m_fIlluminance);
    ambient->setValue(m_PreviewLighting.m_fAmbientRadiance);
    yaw->setValue(93);
    pitch->setValue(43);
    temperature->setValue(m_PreviewLighting.m_uiTemperature);
    compensation->setValue(m_PreviewLighting.m_fExposureCompensation);
    sky->setChecked(m_PreviewLighting.m_bSky);
    shadows->setChecked(m_PreviewLighting.m_bShadows);
    automatic->setChecked(m_PreviewLighting.m_bAutomaticExposure);
    diagnostic->setCurrentIndex(index == 3 ? 1 : 0);
  };
  connect(preset, &QComboBox::currentIndexChanged, this, applyPreset);
  auto* resetLighting = new QPushButton("Reset Lighting", widget);
  form->addRow(resetLighting);
  connect(resetLighting, &QPushButton::clicked, this, [preset, applyPreset]() {
    const QSignalBlocker blocker(preset);
    preset->setCurrentIndex(0);
    applyPreset(0);
  });

  auto* geometry = new QComboBox(widget);
  geometry->addItems({"Sphere", "Box", "Plane"});
  form->addRow("Geometry", geometry);
  connect(geometry, &QComboBox::currentIndexChanged, this, [this](int index) { GetMaterialDocument()->m_PreviewModel.SetValue(index); });
  auto* orientation = number("Object rotation", -180, 180, 0, " deg");
  connect(orientation, &QDoubleSpinBox::valueChanged, this, [this](double value) { m_fPreviewOrientation = static_cast<float>(value); });
  auto* grid = new QCheckBox("Grid", widget);
  form->addRow(grid);
  connect(grid, &QCheckBox::toggled, this, [this](bool value) { m_bPreviewGrid = value; });
  auto* camera = new QPushButton("Reset Camera / Frame Object", widget);
  form->addRow(camera);
  connect(camera, &QPushButton::clicked, this, [this]() { m_pViewWidget->GetOrbitCamera()->MoveCameraToDefaultPosition(); });
  auto* resetScene = new QPushButton("Reset Scene", widget);
  form->addRow(resetScene);
  connect(resetScene, &QPushButton::clicked, this, [this, geometry, orientation, grid, diagnostic]() {
    geometry->setCurrentIndex(0);
    GetMaterialDocument()->m_PreviewModel = xiiMaterialAssetPreview::Sphere;
    orientation->setValue(0);
    grid->setChecked(false);
    diagnostic->setCurrentIndex(0);
    m_pViewWidget->GetOrbitCamera()->MoveCameraToDefaultPosition();
  });
  panel->setWidget(widget);
  m_pDockManager->addDockWidgetTab(ads::RightDockWidgetArea, panel);
}
