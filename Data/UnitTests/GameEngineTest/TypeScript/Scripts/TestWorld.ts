import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestWorld extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    foundObjs: xii.GameObject[] = [];

    FoundObjectCallback = (go: xii.GameObject): boolean => {

        this.foundObjs.push(go);
        return true;
    }

    ExecuteTests(): boolean {

        // FindObjectsInSphere
        {
            this.foundObjs = [];
            xii.World.FindObjectsInSphere("Category1", new xii.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            XII_TEST.INT(this.foundObjs.length, 2);

            this.foundObjs = [];
            xii.World.FindObjectsInSphere("Category2", new xii.Vec3(5, 0, 0), 3, this.FoundObjectCallback);
            XII_TEST.INT(this.foundObjs.length, 1);
        }

        // FindObjectsInBox
        {
            this.foundObjs = [];
            xii.World.FindObjectsInBox("Category1", new xii.Vec3(-10, 0, -5), new xii.Vec3(0, 10, 5), this.FoundObjectCallback);
            XII_TEST.INT(this.foundObjs.length, 3);

            this.foundObjs = [];
            xii.World.FindObjectsInBox("Category2", new xii.Vec3(-10, 0, -5), new xii.Vec3(0, 10, 5), this.FoundObjectCallback);
            XII_TEST.INT(this.foundObjs.length, 2);
        }

        return false;
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestWorld") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }
        
        }
    }

}

