import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestLifetime extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    step: number = 0;
    obj1: xii.GameObject = null;
    comp1: xii.MeshComponent = null;
    comp2: xii.MeshComponent = null;

    ExecuteTests(): boolean {

        if (this.step == 0) {

            let d = new xii.GameObjectDesc;
            d.Name = "Jonny";
            d.LocalPosition = new xii.Vec3(1, 2, 3);
            d.Dynamic = true;
            d.Parent = this.GetOwner();

            this.obj1 = xii.World.CreateObject(d);

            XII_TEST.BOOL(this.obj1.GetName() == d.Name);
            XII_TEST.BOOL(this.obj1.GetParent() == this.GetOwner());
            XII_TEST.BOOL(this.obj1.GetLocalPosition().IsEqual(d.LocalPosition));
            XII_TEST.BOOL(this.obj1.GetLocalRotation().IsEqualRotation(xii.Quat.IdentityQuaternion()));
            XII_TEST.BOOL(this.obj1.GetLocalScaling().IsEqual(xii.Vec3.OneVector()));
            XII_TEST.FLOAT(this.obj1.GetLocalUniformScaling(), 1.0, 0.0001);

            this.comp1 = xii.World.CreateComponent(this.obj1, xii.MeshComponent);
            this.comp1.Mesh = "{ 6d619c33-6611-432b-a924-27b1b9bfd8db }"; // Box
            this.comp1.Color = xii.Color.BlueViolet();

            this.comp2 = xii.World.CreateComponent(this.obj1, xii.MeshComponent);
            this.comp2.Mesh = "{ 618ee743-ed04-4fac-bf5f-572939db2f1d }"; // Sphere
            this.comp2.Color = xii.Color.PaleVioletRed();

            return true;
        }

        if (this.step == 1) {
            XII_TEST.BOOL(this.obj1 != null);
            XII_TEST.BOOL(this.obj1.IsValid());

            XII_TEST.BOOL(this.comp1.IsValid());
            XII_TEST.BOOL(this.comp2.IsValid());

            this.obj1.SetLocalUniformScaling(2.0);
            
            xii.World.DeleteComponent(this.comp2);
            
            XII_TEST.BOOL(!this.comp2.IsValid());

            return true;
        }

        if (this.step == 2) {
            XII_TEST.BOOL(this.obj1 != null);
            XII_TEST.BOOL(this.obj1.IsValid());

            XII_TEST.BOOL(this.comp1.IsValid());
            XII_TEST.BOOL(!this.comp2.IsValid());

            xii.World.DeleteObjectDelayed(this.obj1);

            // still valid this frame
            XII_TEST.BOOL(this.obj1.IsValid());
            XII_TEST.BOOL(this.comp1.IsValid());

            return true;
        }

        if (this.step == 3) {
            XII_TEST.BOOL(this.obj1 != null);
            XII_TEST.BOOL(!this.obj1.IsValid());
            XII_TEST.BOOL(!this.comp1.IsValid());
            XII_TEST.BOOL(!this.comp2.IsValid());
        }

        return false;
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestLifetime") {

            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {
                msg.Message = "done";
            }

            this.step += 1;
        }
    }

}

