# XII Model Context Protocol

XII exposes live application state and Editor authoring through the Model Context Protocol (MCP). Three targets share the same capability registry and protocol implementation.

| Target | Role |
| --- | --- |
| `MCPPlugin` | The `xiiMCPPlugin` engine plugin. It provides the protocol, capability registration, native bridge, HTTP listener and runtime tools. It has no desktop or Qt requirement. |
| `EditorPluginMCP` | The optional `xiiEditorPluginMCP` Editor extension. It reads project state and applies persistent changes through the document command history. |
| `MCP` | The `xiiMCP` standalone gateway. It connects to same-user XII targets and serves stdio or Streamable HTTP. Qt builds also provide a gateway window. |

The gateway forwards tool calls through Foundation's authenticated [Local transport](NetworkLocalTransport.md) and [reflected RPC](NetworkReflection.md). The target sends its own capability descriptors and executes operations on its owning thread. Advertisements use the [diagnostics discovery lease](TelemetryDiagnostics.md) in a dedicated MCP namespace. Each forwarded capability name includes the target endpoint and an instance identifier. If a target disappears, its capabilities are removed. A restarted process publishes a new instance. Operations sent to older identifiers fail rather than moving to the replacement process.

## Launch the gateway

Run the executable from a XII binary directory with its companion libraries available. Use `.exe` on Windows.

```text
xiiMCP -MCPListen 127.0.0.1:8372
xiiMCP -MCPTransport stdio
xiiMCP -gui -MCPListen 127.0.0.1:8372
```

Automatic discovery is on by default. To select explicit local endpoints, use `-MCPTarget NAME` followed by one or more endpoint names. Use `-MCPNoDiscovery` to disable the advertisement scan. Call `xii.targets` for connection status and capability counts. Then use `tools/list`, `resources/list`, `resources/templates/list` and `prompts/list` to discover names scoped to each connected target. Follow `nextCursor` when present. Restart discovery if the registry changes during pagination.

The Qt window displays listener status and sessions, discovered instances, capability descriptors, manual invocation results and recent forwarded operations. Recent activity contains a capability name, target, elapsed time and outcome. Arguments are omitted from the activity list. Window geometry and the capability filter are stored in Qt application settings.

Pass `-MCPTimeoutMs` with a value from 100 to 10000 to set the request deadline for the gateway. HTTP defaults to `127.0.0.1:8372/mcp`. The listener accepts numeric IPv4 or IPv6 loopback endpoints. Port zero requests an ephemeral port.

## Connect an MCP client

The server supports stateless MCP requests with `2026-07-28` metadata and the `2025-11-25` initialization handshake. Streamable HTTP uses JSON replies. Stdio uses one UTF-8 JSON message per line and writes diagnostics to stderr. [MCP versioning](https://modelcontextprotocol.io/specification/2026-07-28/basic/versioning) · [Streamable HTTP](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports/streamable-http)

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

Applications that already load `xiiMCPPlugin` can use `-MCP` to start an advertised native endpoint or `-MCPListen 127.0.0.1:8373` to host HTTP directly. Use both switches to enable both endpoints. `-MCPLocal NAME` selects the Local endpoint name. Applications derived from `xiiGameApplication` load the optional plugin automatically when either switch is set. Custom application loops can instantiate `xiiMCPRuntime`, supply explicit endpoint configuration, and call `Update` on the owning thread after completing any concurrent world updates.

Start the Editor with `-MCP` or `-MCPListen` to load the authoring plugin. Add `-MCPEngine` to expose the Editor's game process as a separate read-only native target. The Editor forwards engine startup flags without forwarding its HTTP listener or authoring credentials. Use capabilities from the corresponding process to inspect live state or change persistent documents.

## Permissions

Read permission is the default. `-MCPAllowCapture` enables runtime diagnostic captures, `-MCPAllowEdit` enables supported changes and `-MCPAllowExecute` enables simulation control or Editor asset transformation. A forwarded operation needs the appropriate permission at both the gateway and the target. Editor changes also require `-MCPProject` with the exact project UUID returned by `editor.project`. Mutating requests carry the current project generation. Scene edits and saves also carry the document ID, open session ID and content revision returned by `editor.documents` or `editor.scene`.

Set `XII_MCP_TOKEN` in the listener process environment to require an `Authorization: Bearer …` header. HTTP capture, edit and execute permissions require a token. HTTP checks the Host and Origin headers and binds only to loopback. Use an authenticated TLS reverse proxy if clients must connect from another machine. Native discovery provides metadata only. The target authorizes the authenticated operating-system user independently for each reflected RPC invocation.

The registry exposes only explicitly registered capabilities. CVar strings are redacted, and built-in tools have no arbitrary console or filesystem execution surface. A project authorization applies to the project's UUID across openings, while generation and document tokens guard individual operations against stale client state.

## Tool catalog

Live runtime capabilities include:

| Tool | Result |
| --- | --- |
| `runtime.worlds` | Live world IDs, registry slots, object count and simulation timing. |
| `runtime.objects` | A page of objects with name, parent and component-type filters. |
| `runtime.object` | Local and global transforms and a page of attached components. |
| `runtime.component` | Supported reflected member values and their schema. |
| `runtime.edit_object` | Create, reposition, rotate, scale, activate or schedule removal of a live object with edit permission. |
| `runtime.simulation` | Change enablement, pause and speed with execute permission. |
| `runtime.resources` | A bounded loaded-resource snapshot filtered by RTTI type. |
| `runtime.reload_resource` | Request a standard reload of an existing resource with execute permission. |
| `runtime.status` | Running application and frame timing. |
| `runtime.diagnostics` | Allocator usage, loaded plugins and numeric performance counters. |
| `runtime.cvars` | A filtered list of CVars with string values redacted. |
| `runtime.logs` | Recent engine warnings and errors with capture permission. |
| `runtime.graphics` | Graphics device heap statistics when a graphics device is active. |
| `runtime.screenshot` | Request an image through the game application's normal screenshot pipeline. Requires capture permission and a game window. |
| `runtime.render_graph` | Request and poll a bounded pass and dependency snapshot with available GPU timings. Requires capture permission and an active graphics device. |

Runtime resources include `xii://runtime/status` and the `xii://runtime/worlds/{index}` template. Use the numeric registry slot as `index`. Its result includes the live world ID required by world operations. The `runtime.inspect_scene` prompt describes the world inspection workflow.

Editor capabilities include:

| Tool | Result |
| --- | --- |
| `editor.project` | Project manifest, profile, generation, document types and engine-process readiness. |
| `editor.documents` | Open documents with selection, dirty state and revision tokens. |
| `editor.scene` | Page through scene, prefab or layer objects and inspect supported properties. |
| `editor.edit_scene` | Add or remove objects or components, reparent objects, and set supported members in one undoable transaction. |
| `editor.save` | Queue an asynchronous save. Read the document's dirty state to confirm completion. |
| `editor.assets` | Search indexed assets by name, type or tag. |
| `editor.asset` | Dependencies, output tags, transform state and recent transformation messages. |
| `editor.transform_asset` | Queue transformation through the existing background asset processor. |

To author a scene, call `editor.documents` and `editor.scene` first. Send up to 24 operations to `editor.edit_scene`, then use its new revision for `editor.save`. An operation can provide `newId` when it adds an object, and a later operation in the same batch can reference that UUID. A failed operation cancels the whole transaction.

## Extend the registry

Use `xiiMCPRuntime::GetRegistry()` to extend an embedded runtime, or use `xiiMCPRegistry` with the protocol and binding directly. Register tools with an object input schema, a required permission and a callback. Resource callbacks return MCP `contents`, and prompt callbacks return MCP `messages`. A resource template supports one variable in the final path segment. Its callback receives the concrete `uri` and resolved variable. Callbacks receive the authenticated transport context and deadline. Validate arguments before changing engine state. Register and remove callbacks on the owning thread, and remove any plugin-owned callbacks before unloading their code.

`xiiMCPReflection` describes supported RTTI members and converts values without exposing native pointers. It omits pointer properties and members marked hidden or temporary. Inspecting a type does not make any of its functions callable.

## Current limits

- HTTP serves JSON responses. SSE subscriptions, protocol progress streaming, OAuth and direct non-loopback listeners are outside this implementation. Stdio sends capability-list change notifications for initialized clients.
- The gateway holds multiple target connections and serializes forwarded calls on its owning thread. Foundation cancels expired native RPC requests on a best-effort basis. Completed mutations are not rolled back on a transport disconnect.
- Live resource enumeration is bounded to a prefix. CPU profiling snapshots and higher-level renderer configuration editing are outside the tool catalog.
- The Qt workspace shows connection and listener status. Connection endpoints, permissions, and listener settings are configured at launch.
