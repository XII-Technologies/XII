import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestComponent extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        let owner = this.GetOwner();

        let mesh = owner.TryGetComponentOfBaseType(xii.MeshComponent);
        let text = owner.TryGetComponentOfBaseType(xii.DebugTextComponent);

        // IsValid
        {
            XII_TEST.BOOL(mesh != null && mesh.IsValid());
            XII_TEST.BOOL(text != null && text.IsValid());
        }

        // GetOWner
        {
            XII_TEST.BOOL(mesh.GetOwner() == owner);
            XII_TEST.BOOL(text.GetOwner() == owner);
        }

        // Active Flag / Active State
        {
            XII_TEST.BOOL(mesh.IsActive());
            XII_TEST.BOOL(mesh.IsActiveAndInitialized());
            XII_TEST.BOOL(mesh.IsActiveAndSimulating());
            
            XII_TEST.BOOL(!text.GetActiveFlag());
            XII_TEST.BOOL(!text.IsActive());
            XII_TEST.BOOL(!text.IsActiveAndInitialized());
            
            text.SetActiveFlag(true);
            XII_TEST.BOOL(text.GetActiveFlag());
            XII_TEST.BOOL(text.IsActive());
            XII_TEST.BOOL(text.IsActiveAndInitialized());
            
            mesh.SetActiveFlag(false);
            XII_TEST.BOOL(!mesh.GetActiveFlag());
            XII_TEST.BOOL(!mesh.IsActive());
            XII_TEST.BOOL(!mesh.IsActiveAndInitialized());
            XII_TEST.BOOL(!mesh.IsActiveAndSimulating());
        }

        // GetUniqueID
        {
            // xiiInvalidIndex
            XII_TEST.INT(mesh.GetUniqueID(), 4294967295);
            XII_TEST.INT(text.GetUniqueID(), 4294967295);
        }

        // TryGetScriptComponent
        {
            let sc = this.GetOwner().TryGetScriptComponent("TestComponent");

            XII_TEST.BOOL(sc == this);
        }

        // interact with C++ components
        {
            let c = xii.World.CreateComponent(this.GetOwner(), xii.MoveToComponent);

            // execute function
            c.SetTargetPosition(new xii.Vec3(1, 2, 3));

            // get/set properties
            c.TranslationSpeed = 23;
            XII_TEST.FLOAT(c.TranslationSpeed, 23);
            
            c.TranslationSpeed = 17;
            XII_TEST.FLOAT(c.TranslationSpeed, 17);
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestComponent") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

