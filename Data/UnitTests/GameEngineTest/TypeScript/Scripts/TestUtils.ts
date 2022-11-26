import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")
import prefab = require("./Prefab")

export class TestUtils extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // IsNumberEqual
        {
            XII_TEST.BOOL(xii.Utils.IsNumberEqual(13, 14, 0.9) == false);
            XII_TEST.BOOL(xii.Utils.IsNumberEqual(13, 14, 1.01) == true);
        }

        // IsNumberZero
        {
            XII_TEST.BOOL(xii.Utils.IsNumberZero(0.1, 0.09) == false);
            XII_TEST.BOOL(xii.Utils.IsNumberZero(0.1, 0.11) == true);

            XII_TEST.BOOL(xii.Utils.IsNumberZero(-0.1, 0.09) == false);
            XII_TEST.BOOL(xii.Utils.IsNumberZero(-0.1, 0.11) == true);
        }

        // StringToHash
        {
            XII_TEST.BOOL(xii.Utils.StringToHash("a") != xii.Utils.StringToHash("b"));
        }

        // Clamp
        {
            XII_TEST.INT(xii.Utils.Clamp(13, 8, 11), 11);
            XII_TEST.INT(xii.Utils.Clamp(6, 8, 11), 8);
            XII_TEST.INT(xii.Utils.Clamp(9, 8, 11), 9);
        }

        // Saturate
        {
            XII_TEST.FLOAT(xii.Utils.Saturate(-0.7), 0, 0.001);
            XII_TEST.FLOAT(xii.Utils.Saturate(0.3), 0.3, 0.001);
            XII_TEST.FLOAT(xii.Utils.Saturate(1.3), 1.0, 0.001);
        }

        // FindPrefabRootNode / FindPrefabRootScript / Exposed Script Parameters
        {
            let p1 = this.GetOwner().FindChildByName("Prefab1");
            let p2 = this.GetOwner().FindChildByName("Prefab2");

            XII_TEST.BOOL(p1 != null);
            XII_TEST.BOOL(p2 != null);

            {
                let p1r = xii.Utils.FindPrefabRootNode(p1);
                let p1s: prefab.Prefab = xii.Utils.FindPrefabRootScript(p1, "Prefab");

                XII_TEST.BOOL(p1r != null);
                XII_TEST.BOOL(p1r.GetName() == "root");

                XII_TEST.BOOL(p1s != null);
                XII_TEST.FLOAT(p1s.NumberVar, 11, 0.001);
                XII_TEST.BOOL(p1s.BoolVar);
                XII_TEST.BOOL(p1s.StringVar == "Hello");
                XII_TEST.BOOL(p1s.Vec3Var.IsEqual(new xii.Vec3(1, 2, 3)));

                let c = new xii.Color();
                c.SetGammaByteRGBA(227, 106, 6, 255);
                XII_TEST.BOOL(p1s.ColorVar.IsEqualRGBA(c));
            }

            {
                let p2r = xii.Utils.FindPrefabRootNode(p2);
                let p2s: prefab.Prefab = xii.Utils.FindPrefabRootScript(p2, "Prefab");

                XII_TEST.BOOL(p2r != null);
                XII_TEST.BOOL(p2r.GetName() == "root");

                XII_TEST.BOOL(p2s != null);
                XII_TEST.FLOAT(p2s.NumberVar, 2, 0.001);
                XII_TEST.BOOL(p2s.BoolVar == false);
                XII_TEST.BOOL(p2s.StringVar == "Bye");
                XII_TEST.BOOL(p2s.Vec3Var.IsEqual(new xii.Vec3(4, 5, 6)));

                let c = new xii.Color();
                c.SetGammaByteRGBA(6, 164, 227, 255);
                XII_TEST.BOOL(p2s.ColorVar.IsEqualRGBA(c));
            }
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestUtils") {

            this.ExecuteTests();
            msg.Message = "done";
        }
    }

}

