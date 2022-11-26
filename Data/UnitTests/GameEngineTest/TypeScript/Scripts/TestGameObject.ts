import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestGameObject extends xii.TypescriptComponent {

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
        let child1 = owner.FindChildByName("Child1");
        let child2 = owner.FindChildByPath("Child2");


        // IsValid
        {
            XII_TEST.BOOL(owner.IsValid());
            XII_TEST.BOOL(child1.IsValid());
            XII_TEST.BOOL(child2.IsValid());
        }

        // GetName / SetName
        {
            XII_TEST.BOOL(owner.GetName() == "GameObject");
            owner.SetName("TestGameObject");
            XII_TEST.BOOL(owner.GetName() == "TestGameObject");
        }

        // Active Flag / Active State
        {
            XII_TEST.BOOL(child1.GetActiveFlag());
            XII_TEST.BOOL(child1.IsActive());
            XII_TEST.BOOL(child2.GetActiveFlag());
            XII_TEST.BOOL(child2.IsActive());

            child2.SetActiveFlag(false);

            XII_TEST.BOOL(child1.GetActiveFlag());
            XII_TEST.BOOL(child1.IsActive());
            XII_TEST.BOOL(!child2.GetActiveFlag());
            XII_TEST.BOOL(!child2.IsActive());

            child2.SetActiveFlag(true);

            XII_TEST.BOOL(child1.GetActiveFlag());
            XII_TEST.BOOL(child1.IsActive());
            XII_TEST.BOOL(child2.GetActiveFlag());
            XII_TEST.BOOL(child2.IsActive());
        }

        // Local Position
        {
            XII_TEST.VEC3(child1.GetLocalPosition(), new xii.Vec3(1, 2, 3));
            XII_TEST.VEC3(child2.GetLocalPosition(), new xii.Vec3(4, 5, 6));

            child1.SetLocalPosition(new xii.Vec3(11, 22, 33));
            XII_TEST.VEC3(child1.GetLocalPosition(), new xii.Vec3(11, 22, 33));
        }

        // Local Rotation
        {
            XII_TEST.QUAT(child1.GetLocalRotation(), xii.Quat.IdentityQuaternion());

            let nr = new xii.Quat();
            nr.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(45));

            child1.SetLocalRotation(nr);
            XII_TEST.QUAT(child1.GetLocalRotation(), nr);
        }

        // Local Scaling
        {
            XII_TEST.VEC3(child2.GetLocalScaling(), new xii.Vec3(2, 3, 4));
            XII_TEST.FLOAT(child2.GetLocalUniformScaling(), 5);

            child2.SetLocalScaling(new xii.Vec3(22, 33, 44));
            child2.SetLocalUniformScaling(55);

            XII_TEST.VEC3(child2.GetLocalScaling(), new xii.Vec3(22, 33, 44));
            XII_TEST.FLOAT(child2.GetLocalUniformScaling(), 55);
        }

        // Global Position
        {
            XII_TEST.VEC3(child1.GetGlobalPosition(), new xii.Vec3(1, 2, 3));
            XII_TEST.VEC3(child2.GetGlobalPosition(), new xii.Vec3(4, 5, 6));

            child1.SetGlobalPosition(new xii.Vec3(11, 22, 33));
            XII_TEST.VEC3(child1.GetGlobalPosition(), new xii.Vec3(11, 22, 33));

        }

        // Global Rotation
        {
            XII_TEST.QUAT(child1.GetGlobalRotation(), xii.Quat.IdentityQuaternion());

            let nr = new xii.Quat();
            nr.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(30));

            child1.SetGlobalRotation(nr);
            XII_TEST.QUAT(child1.GetGlobalRotation(), nr);
        }

        // Global Scaling
        {
            XII_TEST.VEC3(child2.GetGlobalScaling(), new xii.Vec3(2 * 5, 3 * 5, 4 * 5));

            child2.SetGlobalScaling(new xii.Vec3(1, 2, 3));

            XII_TEST.VEC3(child2.GetGlobalScaling(), new xii.Vec3(1, 2, 3));
            XII_TEST.FLOAT(child2.GetLocalUniformScaling(), 1);
        }

        // Global Dirs
        {
            child1.SetGlobalRotation(xii.Quat.IdentityQuaternion());

            XII_TEST.VEC3(child1.GetGlobalDirForwards(), new xii.Vec3(1, 0, 0));
            XII_TEST.VEC3(child1.GetGlobalDirRight(), new xii.Vec3(0, 1, 0));
            XII_TEST.VEC3(child1.GetGlobalDirUp(), new xii.Vec3(0, 0, 1));

            let r = new xii.Quat();
            r.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));

            child1.SetGlobalRotation(r);

            XII_TEST.VEC3(child1.GetGlobalDirForwards(), new xii.Vec3(0, 1, 0));
            XII_TEST.VEC3(child1.GetGlobalDirRight(), new xii.Vec3(-1, 0, 0));
            XII_TEST.VEC3(child1.GetGlobalDirUp(), new xii.Vec3(0, 0, 1));
        }

        // Velocity
        {
            XII_TEST.VEC3(child1.GetVelocity(), xii.Vec3.ZeroVector());

            child1.SetVelocity(new xii.Vec3(1, 2, 3));
            XII_TEST.VEC3(child1.GetVelocity(), new xii.Vec3(1, 2, 3));
        }

        // Team ID
        {
            XII_TEST.FLOAT(child1.GetTeamID(), 0);
            child1.SetTeamID(11);
            XII_TEST.FLOAT(child1.GetTeamID(), 11);
        }

        // FindChildByName
        {
            let c = owner.FindChildByName("Child1_Child1", false);
            XII_TEST.BOOL(c == null);

            c = owner.FindChildByName("Child1_Child1", true);
            XII_TEST.BOOL(c != null);
            XII_TEST.BOOL(c.IsValid());
            XII_TEST.BOOL(c.GetName() == "Child1_Child1");
        }

        // FindChildByName
        {
            let c = owner.FindChildByPath("Child2_Child1");
            XII_TEST.BOOL(c == null);

            c = owner.FindChildByPath("Child2/Child2_Child1");
            XII_TEST.BOOL(c != null);
            XII_TEST.BOOL(c.IsValid());
            XII_TEST.BOOL(c.GetName() == "Child2_Child1");
        }

        // SearchForChildByNameSequence
        {
            let c = owner.SearchForChildByNameSequence("Child1_Child1/A");
            XII_TEST.BOOL(c != null && c.IsValid());
            XII_TEST.FLOAT(c.GetLocalUniformScaling(), 2);

            c = owner.SearchForChildWithComponentByNameSequence("Child2/A", xii.PointLightComponent);
            XII_TEST.BOOL(c != null && c.IsValid());
            XII_TEST.FLOAT(c.GetLocalUniformScaling(), 3);
        }

        // TryGetComponentOfBaseType
        {
            let sl = child1.TryGetComponentOfBaseType(xii.SpotLightComponent);
            XII_TEST.BOOL(sl != null && sl.IsValid());

            let pl = child1.TryGetComponentOfBaseTypeName<xii.SpotLightComponent>("xiiPointLightComponent");
            XII_TEST.BOOL(pl != null && pl.IsValid());
        }

        // Tags
        {
            XII_TEST.BOOL(owner.HasAllTags("AutoColMesh"));
            XII_TEST.BOOL(owner.HasAllTags("CastShadow"));
            XII_TEST.BOOL(owner.HasAllTags("AutoColMesh", "CastShadow"));
            XII_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "NOTAG"));
            XII_TEST.BOOL(owner.HasAnyTags("CastShadow", "NOTAG"));
            XII_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));

            owner.RemoveTags("CastShadow", "AutoColMesh");
            XII_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow"));

            owner.AddTags("CastShadow", "TAG1");
            XII_TEST.BOOL(owner.HasAnyTags("AutoColMesh", "CastShadow"));
            XII_TEST.BOOL(!owner.HasAllTags("AutoColMesh", "CastShadow"));

            owner.SetTags("TAG");
            XII_TEST.BOOL(owner.HasAnyTags("TAG"));
            XII_TEST.BOOL(!owner.HasAnyTags("AutoColMesh", "CastShadow", "TAG1"));
        }

        // Global Key
        {
            let obj = xii.World.TryGetObjectWithGlobalKey("Tests");
            XII_TEST.BOOL(obj != null);
            XII_TEST.BOOL(obj.GetName() == "All Tests");

            this.GetOwner().SetGlobalKey("TestGameObjects");
            XII_TEST.BOOL(this.GetOwner().GetGlobalKey() == "TestGameObjects");
            let tgo = xii.World.TryGetObjectWithGlobalKey("TestGameObjects");
            XII_TEST.BOOL(tgo == this.GetOwner());
            this.GetOwner().SetGlobalKey("");
            let tgo2 = xii.World.TryGetObjectWithGlobalKey("TestGameObjects");
            XII_TEST.BOOL(tgo2 == null);
        }

        // GetChildren
        {
            XII_TEST.INT(this.GetOwner().GetChildCount(), 3);

            let children = this.GetOwner().GetChildren();
            XII_TEST.INT(this.GetOwner().GetChildCount(), children.length);

            this.GetOwner().DetachChild(children[0]);
            XII_TEST.INT(this.GetOwner().GetChildCount(), 2);
            XII_TEST.BOOL(children[0].GetParent() == null);

            children[0].SetParent(this.GetOwner());
            XII_TEST.INT(this.GetOwner().GetChildCount(), 3);
            XII_TEST.BOOL(children[0].GetParent() == this.GetOwner());

            this.GetOwner().DetachChild(children[2]);
            XII_TEST.BOOL(children[2].GetParent() == null);
            this.GetOwner().AddChild(children[2]);
            XII_TEST.BOOL(children[2].GetParent() == this.GetOwner());
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestGameObject") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

