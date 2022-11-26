import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")
import shared = require("./Shared")

export class HelperComponent extends xii.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    // Initialize(): void { }
    // Deinitialize(): void { }
    // OnActivated(): void { }
    // OnDeactivated(): void { }

    OnSimulationStarted(): void {
        this.SetTickInterval(xii.Time.Milliseconds(0));
    }

    RaiseEvent(text: string): void {
        let e = new xii.MsgGenericEvent;
        e.Message = text;
        this.BroadcastEvent(e);
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "Event1") {

            this.RaiseEvent("e1");
        }

        // should not reach itself
        XII_TEST.BOOL(msg.Message != "e1");
    }

    Tick(): void {
    }
}

