# XII Model Context Protocol

XII exposes live application state and Editor authoring through the Model Context Protocol (MCP). Three targets share the same capability registry and protocol implementation.

| Target | Role |
| --- | --- |
| `MCPPlugin` | The `xiiMCPPlugin` engine plugin. It provides the protocol, capability registration, native bridge, HTTP listener and runtime tools. It has no desktop or Qt requirement. |
| `EditorPluginMCP` | The optional `xiiEditorPluginMCP` Editor extension. It reads project state and applies persistent changes through the document command history. |
| `MCP` | The `xiiMCP` standalone gateway. It connects to same-user XII targets and serves stdio or Streamable HTTP. Qt builds also provide a gateway window. |

The gateway forwards tool calls through Foundation's authenticated [Local transport](NetworkLocalTransport.md) and [reflected RPC](NetworkReflection.md). The target sends its own capability descriptors and executes operations on its owning thread. Advertisements use the [diagnostics discovery lease](TelemetryDiagnostics.md) in a dedicated MCP namespace. Each forwarded capability name or URI contains the full target instance UUID. Match this namespace to an endpoint using `xii.targets`. If a target disappears, its capabilities are removed. A restarted process publishes a new instance. Operations sent to older identifiers fail rather than moving to the replacement process.

## Launch the gateway

In XII Editor, choose **Tools → Launch MCP** to open the gateway with your saved desktop preferences.

Run the executable from a XII binary directory with its companion libraries available. Use `.exe` on Windows. In a Qt build, launching without arguments opens the XII-styled management window and restores saved gateway preferences. A Qt-free build defaults to headless HTTP.

```text
xiiMCP
xiiMCP -MCPTransport http -MCPListen 127.0.0.1:8372
xiiMCP -MCPTransport stdio
```

Automatic discovery is on by default. To select explicit local endpoints, use `-MCPTarget NAME` followed by one or more endpoint names. Use `-MCPNoDiscovery` to disable the advertisement scan. Call `xii.targets` for connection status and capability counts. Then use `tools/list`, `resources/list`, `resources/templates/list` and `prompts/list` to discover names scoped to each connected target. Follow `nextCursor` when present. Restart discovery if the registry changes during pagination.

The Qt window has live gateway settings for loopback HTTP, the request deadline, discovery and explicit Local endpoints. Use **Instances** to connect, disconnect or browse an individual target without resetting the listener. The workspace persists ordinary preferences through Qt settings. Bearer tokens and elevated permissions apply only to the current process. The Clients page shows initialized HTTP sessions, recent stateless client profiles and pending requests. Stateless profiles group requests by advertised client name and version, or by User-Agent when client information is absent. Profiles expire after 15 minutes and serve only as local diagnostics. The Clients page can copy an editor-managed VS Code HTTP configuration. With authentication enabled, the configuration prompts for the token.

The Capabilities page shows tools, resources, resource templates and prompts. It generates fields for common tool argument types from each input schema and offers an advanced JSON editor. Invocations enter a bounded worker queue. **Cancel requests** clears waiting work and requests cancellation of pending GUI invocations. Recent activity includes the capability, target, elapsed time and outcome. Target discovery uses independent native RPC exchanges for each instance.

Inspect a capability descriptor or a result in **Tree** or **JSON**. For tool replies, the viewer uses the structured result when available. A reply with one JSON resource shows that resource's decoded content. Use **Response** to view or copy the original MCP message, and **Response history** to revisit recent replies.

Pass `-MCPTimeoutMs` with a value from 100 to 10000 to set the request deadline for the gateway. HTTP defaults to `127.0.0.1:8372/mcp`. The listener accepts numeric IPv4 or IPv6 loopback endpoints. Port zero requests an ephemeral port.

## Connect an MCP client

The server supports stateless MCP requests with `2026-07-28` metadata and the `2025-11-25` initialization handshake. Streamable HTTP supports JSON replies and request-scoped SSE for deferred modern requests. Stdio uses one UTF-8 JSON message per line and writes diagnostics to stderr. [MCP versioning](https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning) · [Streamable HTTP](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http)

Configure a Streamable HTTP server with the URL `http://127.0.0.1:8372/mcp`. For stdio, configure the full executable path with arguments `-MCPTransport stdio`. The following entry can be used in the Copilot user MCP configuration or an MCP client with the same command and argument format. [Copilot MCP configuration reference](https://code.visualstudio.com/docs/agents/reference/mcp-configuration)

```json
{
  "mcpServers": {
    "xii": {
      "type": "stdio",
      "command": "D:/XII/Output/Bin/WinVs2026Debug64/xiiMCP.exe",
      "args": ["-MCPTransport", "stdio"],
      "tools": ["*"]
    }
  }
}
```

For an existing HTTP gateway, Claude Code can register the URL with `claude mcp add --transport http xii http://127.0.0.1:8372/mcp`. Configure any bearer credential through the client's header settings. [Claude Code MCP documentation](https://code.claude.com/docs/en/mcp)

## Enable MCP in a target

To include MCP in a project, select **Model Context Protocol (MCP)** under **Configuration → Plugins** and restart the Editor. The bundle loads the authoring plugin in the Editor and includes the runtime plugin in the Editor game process and packaged builds. Loading the bundle starts read-only, same-user Local discovery by default. Use `-MCPDisable` to suppress automatic MCP startup for a process.

Applications that already load `xiiMCPPlugin` can use `-MCP` to start an advertised native endpoint or `-MCPListen 127.0.0.1:8373` to host HTTP directly. Use both switches to enable both endpoints. `-MCPLocal NAME` selects the Local endpoint name. Applications derived from `xiiGameApplication` load the optional plugin automatically when either switch is set. Custom application loops can instantiate `xiiMCPRuntime`, supply explicit endpoint configuration, and call `Update` on the owning thread after completing any concurrent world updates.

Start the Editor with `-MCP` or `-MCPListen` to load the authoring plugin. Add `-MCPEngine` to expose the Editor's game process as a separate read-only native target. The Editor forwards engine startup flags without forwarding its HTTP listener or authoring credentials. Use capabilities from the corresponding process to inspect live state or change persistent documents.

## Permissions

Read permission is the default. Under **Configuration → Editor Preferences → Project (This User) → Integrations → MCP**, enable **Allow Editor Changes** or **Allow Asset Processing** to grant the corresponding native permission for the open project. The gateway also needs the corresponding permission. For direct HTTP, grant permissions with the explicit launch options and authorize the exact project UUID from `editor.project` using `-MCPProject`. This UUID also overrides native project preferences for the process. Mutating requests carry the current project generation. Document edits, history and saves also carry the document ID, open session ID and content revision returned by `editor.documents` or `editor.document`.

In applications, `-MCPAllowCapture` enables diagnostic captures, `-MCPAllowEdit` enables supported live changes and `-MCPAllowExecute` enables simulation and resource operations. Tool descriptors include the required permission as `_meta["xii/permission"]`.

Set `XII_MCP_TOKEN` in the listener process environment to require an `Authorization: Bearer …` header. HTTP capture, edit and execute permissions require a token. HTTP checks the Host and Origin headers and binds only to loopback. Use an authenticated TLS reverse proxy if clients must connect from another machine. Native discovery provides metadata only. The target authorizes the authenticated operating-system user independently for each reflected RPC invocation.

The registry exposes only explicitly registered capabilities. CVar strings are redacted, and built-in tools have no arbitrary console or filesystem execution surface. A project authorization applies to the project's UUID across openings, while generation and document tokens guard individual operations against stale client state.

## Tool catalog

Live runtime capabilities include:

| Tool | Result |
| --- | --- |
| `runtime.worlds` | Live world IDs, registry slots, object count and simulation timing. |
| `runtime.objects` | A page of objects with name, parent and component-type filters. |
| `runtime.object` | Local and global transforms and a page of attached components. |
| `runtime.component` | Current component ID, supported reflected member values and their schema. |
| `runtime.edit_component` | Attach a concrete component to a dynamic owner, activate or remove an attached component, or set an eligible reflected property. Requires edit permission. |
| `runtime.edit_object` | Create, reposition, rotate, scale, activate, reparent or schedule removal of a live object with edit permission. Reparenting preserves the local transform. |
| `runtime.edit_objects` | Execute up to 24 object edits in order. Reports partial results if an operation fails or expires. |
| `runtime.simulation` | Change enablement, pause and speed with execute permission. |
| `runtime.physics` | Active rigid-body physics module and world gravity. |
| `runtime.physics_query` | Nearest-hit raycast, sphere sweep or sphere overlap through the active physics module. Casts report distance, position, normal and available actor and shape IDs. |
| `runtime.types`, `runtime.type` | Search RTTI by base type, plugin or name and inspect the generated schema for a type. |
| `runtime.resources` | Page registered resources by RTTI type and ID, with content availability, quality levels and memory. |
| `runtime.reload_resource` | Request a standard reload of an existing resource with execute permission. |
| `runtime.status` | Running application and frame timing. |
| `runtime.subsystems` | Owning plugins, startup phases and dependency names for registered subsystems. |
| `runtime.diagnostics` | Allocator usage, loaded plugins and numeric performance counters. |
| `runtime.cvars` | A filtered list of CVars with string values redacted. |
| `runtime.logs` | Recent engine warnings and errors with capture permission. |
| `runtime.graphics` | Active adapter, optional features and graphics memory heap statistics. |
| `runtime.render_views` | The first view for each camera usage in a selected world, with camera, pipeline, render path and resolution. |
| `runtime.screenshot` | Request an image through the game application's normal screenshot pipeline. Requires capture permission and a game window. |
| `runtime.render_graph` | Request and poll a bounded pass and dependency snapshot with available GPU timings. Requires capture permission and an active graphics device. |

Runtime resources include `xii://runtime/status` and the `xii://runtime/worlds/{index}` template. Use the numeric registry slot as `index`. Its result includes the live world ID required by world operations. The `runtime.inspect_scene` prompt describes the world inspection workflow.

To edit a live component member, take the component ID and exact RTTI type from `runtime.object` and inspect its schema with `runtime.component`. Pass `action: "set"`, `world`, `object`, `component`, `type`, `property` and `value` to `runtime.edit_component`. Set accepts top-level members marked `x-xii-runtime-editable` and validates the value against its reflected type and bounds before invoking the setter. Marker radius and volume shape, falloff and sort order support live changes. Component authors can opt in additional members with `xiiRuntimeEditableAttribute` when they support live changes under the world write marker. Live values are transient.

Physics tools appear when a live world has an initialized rigid-body physics module. Queries use world coordinates and distances. A cast requires nonzero `direction` and positive `distance`. Sphere queries also require positive `radius`. Omit `collisionLayer` to use layer 0 or supply a name recognized by the active module.

Editor capabilities include:

| Tool | Result |
| --- | --- |
| `editor.project` | Project manifest, profile, generation, document types, creatable file extensions and engine-process readiness. |
| `editor.cpp_project` | Configured C++ source and output paths, SDK compiler and solution status. |
| `editor.documents` | Open documents with selection, dirty state and revision tokens. |
| `editor.document` | Reflected members and paginated object trees for open documents. |
| `editor.create_document` | Create and open a new document through the manager for its file extension. Requires the current project generation and a new path inside the project. |
| `editor.close_document` | Close a saved writable main document using its current session and revision. Finish transactions and save the main document and its loaded sub-documents first. |
| `editor.set_document_property` | Set a supported reflected value in an undoable transaction. |
| `editor.document_history` | Undo or redo one operation at the current document revision. |
| `editor.viewport` | Inspect the focused or specified document's views, camera, resolution and render mode. Pass a view ID to include its effective rendering settings. |
| `editor.graph` | Page through nodes, pins and links or discover creation templates in a supported graph document. |
| `editor.edit_graph` | Create a node from a template, connect pins, disconnect a link or remove a node through command history. |
| `editor.scene` | Page through scene, prefab or layer objects and inspect supported properties. |
| `editor.edit_scene` | Add or remove objects or components, reparent objects, and set supported members in one undoable transaction. |
| `editor.save` | Queue an asynchronous save. Read the document's dirty state to confirm completion. |
| `editor.assets` | Search indexed assets by name, type or tag. |
| `editor.asset` | Dependencies, output tags, transform state and recent transformation messages. |
| `editor.open_asset` | Open an indexed asset through the existing Editor document manager. |
| `editor.transform_asset` | Queue transformation through the existing background asset processor. |

To author a scene, call `editor.documents` and `editor.scene` first. Send up to 24 operations to `editor.edit_scene`, then use its new revision for `editor.save`. An operation can provide `newId` when it adds an object, and a later operation in the same batch can reference that UUID. A failed operation cancels the whole transaction.

## Extend the registry

Use `xiiMCPRuntime::GetRegistry()` to extend an embedded runtime, or use `xiiMCPRegistry` with the protocol and binding directly. Register tools with an object input schema, a required permission and a callback. Resource callbacks return MCP `contents`, and prompt callbacks return MCP `messages`. A resource template supports one variable in the final path segment. Its callback receives the concrete `uri` and resolved variable. Callbacks receive the authenticated transport context and deadline. Validate arguments before changing engine state. Register and remove callbacks on the owning thread, and remove any plugin-owned callbacks before unloading their code.

`xiiMCPReflection` describes supported RTTI members and converts values without exposing native pointers. It omits pointer properties and members marked hidden or temporary. Inspecting a type does not make any of its functions callable.

## Current limits

- A deferred `2026-07-28` HTTP request starts an SSE stream when `Accept` includes `text/event-stream` with quality at least as high as `application/json`. The stream carries the final JSON-RPC reply and then closes. Closing the stream early requests cancellation. Long-lived subscriptions and progress messages remain separate from request-scoped streaming. Stdio sends capability-list change notifications for initialized clients.
- Target discovery and forwarded invocations share bounded, asynchronous native RPC. Replies can arrive out of order across targets. The gateway accepts `notifications/cancelled` over stdio and initialized HTTP sessions using the `2025-11-25` protocol. Cancellation applies to the originating client and request ID. Foundation cancels native work on a best-effort basis. Completed mutations remain applied.
- The Clients page lists stateful HTTP sessions. Stateless requests do not establish sessions. Applying workspace settings restarts the listener and reconnects targets.
- See [MCP capability coverage](MCPCapabilityCoverage.md) for the existing APIs and the workflows that still require further integration.
