# Render configuration

Render configuration layers platform-profile settings, the active scene's
Render World Settings component, the selected camera component, and explicit
view overrides. Each scope supplies only the fields it authors. Resetting a
field in the property grid restores inheritance for that field. When every
scope inherits, the selected pipeline and view supply their ordinary defaults.

## Authoring settings

- In project settings, select an asset profile and expand the render pipeline
  configuration. Set render policy independently of **Main Render Pipeline**.
  Named profiles can reuse the same pipeline and select different quality
  values.
- Add one **Render World Settings** component to a scene and expand
  **Render Configuration**. Save the scene to persist the configuration.
- Expand **Render Configuration** on a camera to override individual scene
  values. Render-target cameras and camera previews use the bound camera.
- Open **Rendering → Viewport Render Settings** to preview a temporary
  override. **Apply to Scene** copies explicit fields to the scene component in
  one undoable command. **Save to Project Profile** applies them to the active
  asset profile. Both operations restore viewport inheritance. Cancel
  restores the viewport settings present when the panel opened.

Bloom strength zero suppresses the normal bloom workload. The dedicated
Bloom visualization can still inspect it. Render scale is an internal
resolution multiplier, with zero selecting inheritance. It leaves the output
extent unchanged.

**Shadows** controls the authored directional and spotlight shadow branches.
**Screen Space Reflections** controls the screen trace and its reconstruction.
Environment and probe reflections remain available when screen reflections
are disabled. **Pipeline Default** restores the authored feature behavior
when an earlier scope disabled the branch.

## Indirect lighting

**Pipeline Default** restores the pipeline's authored GI behavior.
**Disabled**, **Screen Space**, **DDGI**, and **Ray Traced** select explicit
branches in the shipped Scene and Editor pipelines. DDGI combines a
directional probe clipmap with the screen-space contribution. Its probe
dimensions, update budget, spacing, hysteresis and relocation distance can
inherit independently. An unsupported selection produces a diagnostic and
retains the prior valid configuration of the same loaded pipeline.

Runtime settings do not replace an explicitly selected pipeline resource.
Applications can call `xiiView::SetRenderConfiguration` with the same sparse
structure used by the editor. Pass a default-constructed configuration to
clear the view's overrides. `GetEffectiveRenderConfiguration` returns the
resolved scope values published for the view. Inherited fields in that
snapshot continue to use the consuming pass's authored defaults.

## Pipeline authoring and publication

An authored operation can specify `GlobalIlluminationBranches` in its
**Parameter Overrides**. Store an unsigned 8-bit mask with bit 2 for
Disabled, bit 3 for Screen Space, bit 4 for DDGI and bit 5 for Ray Traced.
An absent or zero mask leaves the operation available to every variant.
`ExplicitGlobalIlluminationProperties` is a property dictionary applied
to that operation in explicit variants. For example, it enables the
shipped irradiance subgraph's depth forwarding operation without changing
its authored default or its debug visualization settings.

Use the unsigned 8-bit `OptionalRenderFeatures` parameter to mark an optional
operation or a subgraph occurrence. Bit 0 marks shadow work and bit 1 marks
screen-space reflections. An occurrence's parameter applies to its expanded
operations. The compiler omits these operations when the corresponding
feature is disabled, including when an explicit GI variant is selected.
Public outputs from a disabled optional feature remain visible as optional
interfaces with no binding. Downstream required inputs still need a
producer in every selected variant.

The compiler expands reusable subgraphs and validates their pin contracts
before pruning inactive operations. It validates required connections and
ordering on the remaining graph. Author optional inputs or shared selectors
where an implementation can be absent. Preserve required root outputs.
Re-transform changed pipeline assets to publish the authored branch metadata.

The pipeline resource caches compiled variants against its immutable source
snapshot. Views prepare replacement pass instances before publishing them
at the frame boundary. Repeated selections reuse the compilation while each
view owns its pass state. A DDGI update-budget or hysteresis edit keeps the
probe volume and pass instances. A spatial edit rebuilds the probe volume
and invalidates temporal history. Existing render graph and GAL lifetime
rules govern the retired resources.

Add new scope properties to the relevant reflected settings group with their
default and range metadata. Validate a candidate against the pass's authored
settings before publication if the combined values have additional
constraints. Consume the resolved policy during graph construction and retain
any required values in pass or frame data before execution.
