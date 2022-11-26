import xii = require("<PATH-TO-XII-TS>")

//export class NewComponent extends xii.TypescriptComponent {
export class NewComponent extends xii.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgSetColor, "OnMsgSetColor");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(xii.Time.Milliseconds(100));
    }

    OnMsgSetColor(msg: xii.MsgSetColor): void {
        xii.Log.Info("MsgSetColor: " + msg.Color.r + ", " + msg.Color.g + ", " + msg.Color.b + ", " + msg.Color.a);
    }

    Tick(): void {
        // if a regular tick is not needed, remove this and derive directly from xii.TypescriptComponent
        xii.Log.Info("NewComponent.Tick()")
    }
}

