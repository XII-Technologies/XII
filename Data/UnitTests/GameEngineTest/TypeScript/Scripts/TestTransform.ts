import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestTransform extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        // constructor
        {
            let t = new xii.Transform();

            XII_TEST.VEC3(t.position, xii.Vec3.ZeroVector());
            XII_TEST.QUAT(t.rotation, xii.Quat.IdentityQuaternion());
            XII_TEST.VEC3(t.scale, xii.Vec3.OneVector());
        }

        // Clone
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(xii.Vec3.UnitAxisZ(), xii.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = t.Clone();
            XII_TEST.BOOL(t != c);
            XII_TEST.BOOL(t.position != c.position);
            XII_TEST.BOOL(t.rotation != c.rotation);
            XII_TEST.BOOL(t.scale != c.scale);

            XII_TEST.VEC3(t.position, c.position);
            XII_TEST.QUAT(t.rotation, c.rotation);
            XII_TEST.VEC3(t.scale, c.scale);
        }

        // SetTransform
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(xii.Vec3.UnitAxisZ(), xii.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new xii.Transform();
            c.SetTransform(t);

            XII_TEST.BOOL(t != c);
            XII_TEST.BOOL(t.position != c.position);
            XII_TEST.BOOL(t.rotation != c.rotation);
            XII_TEST.BOOL(t.scale != c.scale);

            XII_TEST.VEC3(t.position, c.position);
            XII_TEST.QUAT(t.rotation, c.rotation);
            XII_TEST.VEC3(t.scale, c.scale);
        }

        // SetIdentity
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(xii.Vec3.UnitAxisZ(), xii.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            t.SetIdentity();

            XII_TEST.VEC3(t.position, xii.Vec3.ZeroVector());
            XII_TEST.QUAT(t.rotation, xii.Quat.IdentityQuaternion());
            XII_TEST.VEC3(t.scale, xii.Vec3.OneVector());
        }

        // IdentityTransform
        {
            let t = xii.Transform.IdentityTransform();

            XII_TEST.VEC3(t.position, xii.Vec3.ZeroVector());
            XII_TEST.QUAT(t.rotation, xii.Quat.IdentityQuaternion());
            XII_TEST.VEC3(t.scale, xii.Vec3.OneVector());
        }

        // IsIdentical
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(xii.Vec3.UnitAxisZ(), xii.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new xii.Transform();
            c.SetTransform(t);

            XII_TEST.BOOL(t.IsIdentical(c));

            c.position.x += 0.0001;

            XII_TEST.BOOL(!t.IsIdentical(c));
        }

        // IsEqual
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(xii.Vec3.UnitAxisZ(), xii.Angle.DegreeToRadian(90));
            t.scale.Set(4, 5, 6);

            let c = new xii.Transform();
            c.SetTransform(t);

            XII_TEST.BOOL(t.IsEqual(c));

            c.position.x += 0.0001;

            XII_TEST.BOOL(t.IsEqual(c, 0.001));
            XII_TEST.BOOL(!t.IsEqual(c, 0.00001));
        }

        // Translate
        {
            let t = new xii.Transform();
            t.Translate(new xii.Vec3(1, 2, 3));

            XII_TEST.VEC3(t.position, new xii.Vec3(1, 2, 3));
            XII_TEST.QUAT(t.rotation, xii.Quat.IdentityQuaternion());
            XII_TEST.VEC3(t.scale, xii.Vec3.OneVector());
        }

        // SetMulTransform / MulTransform
        {
            let tParent = new xii.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new xii.Transform();
            tToChild.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            // this is exactly the same as SetGlobalTransform
            let tChild = new xii.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            XII_TEST.BOOL(tChild.position.IsEqual(new xii.Vec3(13, 12, -5), 0.0001));

            let q1 = new xii.Quat();
            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            XII_TEST.BOOL(tChild.rotation.IsEqualRotation(q1, 0.0001));

            XII_TEST.VEC3(tChild.scale, new xii.Vec3(8, 8, 8));

            tChild = tParent.Clone();
            tChild.MulTransform(tToChild);

            XII_TEST.BOOL(tChild.position.IsEqual(new xii.Vec3(13, 12, -5), 0.0001));

            q1.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            XII_TEST.QUAT(tChild.rotation, q1);
            XII_TEST.VEC3(tChild.scale, new xii.Vec3(8, 8, 8));

            let a = new xii.Vec3(7, 8, 9);
            let b = a.Clone();
            tToChild.TransformPosition(b);
            tParent.TransformPosition(b);

            let c = a.Clone();
            tChild.TransformPosition(c);

            XII_TEST.VEC3(b, c);
        }

        // Invert / GetInverse
        {
            let tParent = new xii.Transform();
            tParent.position.Set(1, 2, 3);

            tParent.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new xii.Transform();
            tParent.position.Set(4, 5, 6);

            tToChild.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new xii.Transform();
            tChild.SetMulTransform(tParent, tToChild);

            // invert twice -> get back original
            let t2 = tToChild.Clone();
            t2.Invert();
            XII_TEST.BOOL(!t2.IsEqual(tToChild, 0.0001));
            t2 = t2.GetInverse();
            XII_TEST.BOOL(t2.IsEqual(tToChild, 0.0001));

            let tInvToChild = tToChild.GetInverse();

            let tParentFromChild = new xii.Transform();
            tParentFromChild.SetMulTransform(tChild, tInvToChild);

            XII_TEST.BOOL(tParent.IsEqual(tParentFromChild, 0.0001));
        }

        // SetLocalTransform
        {
            let q = new xii.Quat();
            q.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));

            let tParent = new xii.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tChild = new xii.Transform();
            tChild.position.Set(13, 12, -5);
            tChild.rotation.SetConcatenatedRotations(tParent.rotation, q);
            tChild.scale.SetAll(8);

            let tToChild = new xii.Transform();
            tToChild.SetLocalTransform(tParent, tChild);

            XII_TEST.VEC3(tToChild.position, new xii.Vec3(4, 5, 6));
            XII_TEST.QUAT(tToChild.rotation, q);
            XII_TEST.VEC3(tToChild.scale, new xii.Vec3(4, 4, 4));
        }

        // SetGlobalTransform
        {
            let tParent = new xii.Transform();
            tParent.position.Set(1, 2, 3);
            tParent.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));
            tParent.scale.SetAll(2);

            let tToChild = new xii.Transform();
            tToChild.position.Set(4, 5, 6);
            tToChild.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));
            tToChild.scale.SetAll(4);

            let tChild = new xii.Transform();
            tChild.SetGlobalTransform(tParent, tToChild);

            XII_TEST.VEC3(tChild.position, new xii.Vec3(13, 12, -5));

            let q = new xii.Quat();
            q.SetConcatenatedRotations(tParent.rotation, tToChild.rotation);
            XII_TEST.QUAT(tChild.rotation, q);
            XII_TEST.VEC3(tChild.scale, new xii.Vec3(8, 8, 8));
        }

        // TransformPosition / TransformDirection
        {
            let qRotX = new xii.Quat();
            let qRotY = new xii.Quat();

            qRotX.SetFromAxisAndAngle(new xii.Vec3(1, 0, 0), xii.Angle.DegreeToRadian(90));
            qRotY.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));

            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetConcatenatedRotations(qRotY, qRotX);
            t.scale.Set(2, -2, 4);

            let v = new xii.Vec3(4, 5, 6);
            t.TransformPosition(v);
            XII_TEST.VEC3(v, new xii.Vec3((5 * -2) + 1, (-6 * 4) + 2, (-4 * 2) + 3));

            v.Set(4, 5, 6);
            t.TransformDirection(v);
            XII_TEST.VEC3(v, new xii.Vec3((5 * -2), (-6 * 4), (-4 * 2)));
        }

        // ConcatenateRotations / ConcatenateRotationsReverse
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(90));
            t.scale.SetAll(2);

            let q = new xii.Quat();
            q.SetFromAxisAndAngle(new xii.Vec3(0, 0, 1), xii.Angle.DegreeToRadian(90));

            let t2 = t.Clone();
            let t4 = t.Clone();
            t2.ConcatenateRotations(q);
            t4.ConcatenateRotationsReverse(q);

            let t3 = t.Clone();
            t3.ConcatenateRotations(q);
            XII_TEST.BOOL(t2.IsEqual(t3));
            XII_TEST.BOOL(!t3.IsEqual(t4));

            let a = new xii.Vec3(7, 8, 9);
            let b = a.Clone();
            t2.TransformPosition(b);

            let c = a.Clone();
            q.RotateVec3(c);
            t.TransformPosition(c);

            XII_TEST.VEC3(b, c);
        }

        // GetAsMat4
        {
            let t = new xii.Transform();
            t.position.Set(1, 2, 3);
            t.rotation.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(34));
            t.scale.Set(2, -1, 5);

            let m = t.GetAsMat4();

            // reference
            {
                let q = new xii.Quat();
                q.SetFromAxisAndAngle(new xii.Vec3(0, 1, 0), xii.Angle.DegreeToRadian(34));

                let referenceTransform = new xii.Transform();
                referenceTransform.position.Set(1, 2, 3);
                referenceTransform.rotation.SetQuat(q);
                referenceTransform.scale.Set(2, -1, 5);

                let refM = referenceTransform.GetAsMat4();

                XII_TEST.BOOL(m.IsEqual(refM));
            }

            let p: xii.Vec3[] = [new xii.Vec3(- 4, 0, 0), new xii.Vec3(5, 0, 0), new xii.Vec3(0, -6, 0), new xii.Vec3(0, 7, 0),
            new xii.Vec3(0, 0, -8), new xii.Vec3(0, 0, 9), new xii.Vec3(1, -2, 3), new xii.Vec3(-4, 5, 7)];

            for (let i = 0; i < 8; ++i) {

                let pt = p[i].Clone();
                t.TransformPosition(pt);

                let pm = p[i].Clone();
                m.TransformPosition(pm);

                XII_TEST.VEC3(pt, pm);
            }
        }

        // SetFromMat4
        {
            let mRot = new xii.Mat3();
            mRot.SetRotationMatrix((new xii.Vec3(1, 2, 3)).GetNormalized(), xii.Angle.DegreeToRadian(42));

            let mTrans = new xii.Mat4();
            mTrans.SetTransformationMatrix(mRot, new xii.Vec3(1, 2, 3));

            let t = new xii.Transform();
            t.SetFromMat4(mTrans);
            XII_TEST.VEC3(t.position, new xii.Vec3(1, 2, 3), 0);
            XII_TEST.BOOL(t.rotation.GetAsMat3().IsEqual(mRot, 0.001));
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestTransform") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

