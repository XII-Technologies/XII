import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestVec3 extends xii.TypescriptComponent {

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
        let d = new xii.Vec3();
        XII_TEST.FLOAT(d.x, 0, 0.001);
        XII_TEST.FLOAT(d.y, 0, 0.001);
        XII_TEST.FLOAT(d.z, 0, 0.001);

        // ZeroVector
        XII_TEST.VEC3(new xii.Vec3(), xii.Vec3.ZeroVector(), 0.0001);

        let v = new xii.Vec3(1, 2, 3);
        XII_TEST.FLOAT(v.x, 1, 0.001);
        XII_TEST.FLOAT(v.y, 2, 0.001);
        XII_TEST.FLOAT(v.z, 3, 0.001);

        // Clone
        XII_TEST.VEC3(v.Clone(), v, 0.001);

        // CloneAsVec2
        XII_TEST.VEC2(v.CloneAsVec2(), new xii.Vec2(1, 2), 0.001);

        // OneVector
        XII_TEST.VEC3(new xii.Vec3(1, 1, 1), xii.Vec3.OneVector(), 0.0001);

        // UnitAxisX
        XII_TEST.VEC3(new xii.Vec3(1, 0, 0), xii.Vec3.UnitAxisX(), 0.0001);

        // UnitAxisY
        XII_TEST.VEC3(new xii.Vec3(0, 1, 0), xii.Vec3.UnitAxisY(), 0.0001);

        // UnitAxisZ
        XII_TEST.VEC3(new xii.Vec3(0, 0, 1), xii.Vec3.UnitAxisZ(), 0.0001);

        // Set
        v.Set(4, 5, 6);
        XII_TEST.FLOAT(v.x, 4, 0.001);
        XII_TEST.FLOAT(v.y, 5, 0.001);
        XII_TEST.FLOAT(v.z, 6, 0.001);

        // SetVec3
        let v2 = new xii.Vec3();
        v2.SetVec3(v);
        XII_TEST.VEC3(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        XII_TEST.FLOAT(v2.x, 7, 0.001);
        XII_TEST.FLOAT(v2.y, 7, 0.001);
        XII_TEST.FLOAT(v2.z, 7, 0.001);

        // SetZero
        v2.SetZero();
        XII_TEST.FLOAT(v2.x, 0, 0.001);
        XII_TEST.FLOAT(v2.y, 0, 0.001);
        XII_TEST.FLOAT(v2.z, 0, 0.001);

        // GetLengthSquared
        XII_TEST.FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        XII_TEST.FLOAT(v2.GetLengthSquared(), 3, 0.001);

        // GetLength
        XII_TEST.FLOAT(v2.GetLength(), Math.sqrt(3), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        XII_TEST.FLOAT(l, Math.sqrt(3), 0.001);
        XII_TEST.FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        XII_TEST.BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        XII_TEST.FLOAT(v.GetLength(), 1, 0.001);
        XII_TEST.BOOL(v.IsNormalized());

        // GetNormalized
        v.Set(3, 0, 0);
        XII_TEST.VEC3(v.GetNormalized(), xii.Vec3.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        XII_TEST.BOOL(v.NormalizeIfNotZero(xii.Vec3.UnitAxisZ(), 0.001));
        XII_TEST.VEC3(v, xii.Vec3.UnitAxisX(), 0.0001);

        // IsZero
        XII_TEST.BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        XII_TEST.BOOL(v.IsZero());

        XII_TEST.BOOL(!v.NormalizeIfNotZero(xii.Vec3.UnitAxisZ(), 0.001));
        XII_TEST.VEC3(v, xii.Vec3.UnitAxisZ(), 0.0001);

        // GetNegated
        v.Set(1, 2, 3);
        XII_TEST.VEC3(v.GetNegated(), new xii.Vec3(-1, -2, -3), 0.0001);
        XII_TEST.VEC3(v, new xii.Vec3(1, 2, 3), 0.0001);

        // Negate
        v.Negate();
        XII_TEST.VEC3(v, new xii.Vec3(-1, -2, -3), 0.0001);

        // AddVec3
        v.Set(2, 3, 4);
        v2.Set(5, 6, 7);
        v.AddVec3(v2);
        XII_TEST.VEC3(v, new xii.Vec3(7, 9, 11), 0.0001);

        // SubVec3
        v.SubVec3(v2);
        XII_TEST.VEC3(v, new xii.Vec3(2, 3, 4), 0.0001);

        // MulVec3
        v.MulVec3(v);
        XII_TEST.VEC3(v, new xii.Vec3(4, 9, 16), 0.0001);

        // DivVec3
        v.DivVec3(new xii.Vec3(2, 3, 4));
        XII_TEST.VEC3(v, new xii.Vec3(2, 3, 4), 0.0001);

        // MulNumber
        v.MulNumber(2);
        XII_TEST.VEC3(v, new xii.Vec3(4, 6, 8), 0.0001);

        // DivNumber
        v.DivNumber(2);
        XII_TEST.VEC3(v, new xii.Vec3(2, 3, 4), 0.0001);

        // IsIdentical
        XII_TEST.BOOL(v.IsIdentical(v));
        XII_TEST.BOOL(!v.IsIdentical(v2));

        // IsEqual
        XII_TEST.BOOL(v.IsEqual(new xii.Vec3(2, 3, 4), 0.0001));
        XII_TEST.BOOL(!v.IsEqual(new xii.Vec3(2, 3.5, 4), 0.0001));

        // Dot
        v.Set(2, 3, 4);
        v2.Set(3, 4, 5);
        XII_TEST.FLOAT(v.Dot(v2), 38, 0.001);

        // CrossRH
        v.Set(1, 0, 0);
        v2.Set(0, 1, 0);
        XII_TEST.VEC3(v.CrossRH(v2), new xii.Vec3(0, 0, 1), 0.001);

        // SetCrossRH
        let v3 = new xii.Vec3();
        v3.SetCrossRH(v, v2);
        XII_TEST.VEC3(v3, new xii.Vec3(0, 0, 1), 0.001);

        // GetCompMin
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        XII_TEST.VEC3(v.GetCompMin(v2), new xii.Vec3(1, 4, 6), 0.001);

        // GetCompMax
        v.Set(2, 4, 6);
        v2.Set(1, 5, 7);
        XII_TEST.VEC3(v.GetCompMax(v2), new xii.Vec3(2, 5, 7), 0.001);

        // GetCompClamp
        XII_TEST.VEC3(v.GetCompClamp(new xii.Vec3(3, 4, 5), new xii.Vec3(4, 5, 6)), new xii.Vec3(3, 4, 6), 0.001);

        // GetCompMul
        XII_TEST.VEC3(v.GetCompMul(new xii.Vec3(2, 3, 4)), new xii.Vec3(4, 12, 24), 0.001);

        // GetCompDiv
        XII_TEST.VEC3(v.GetCompDiv(new xii.Vec3(2, 4, 6)), xii.Vec3.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2, -3);
        XII_TEST.VEC3(v.GetAbs(), new xii.Vec3(1, 2, 3), 0.001);

        // SetAbs
        v2.SetAbs(v);
        XII_TEST.VEC3(v2, new xii.Vec3(1, 2, 3), 0.001);

        // CalculateNormal
        XII_TEST.BOOL(v.CalculateNormal(new xii.Vec3(-1, 0, 1), new xii.Vec3(1, 0, 1), new xii.Vec3(0, 0, -1)));
        XII_TEST.VEC3(v, new xii.Vec3(0, 1, 0), 0.001);

        XII_TEST.BOOL(v.CalculateNormal(new xii.Vec3(-1, 0, -1), new xii.Vec3(1, 0, -1), new xii.Vec3(0, 0, 1)));
        XII_TEST.VEC3(v, new xii.Vec3(0, -1, 0), 0.001);

        XII_TEST.BOOL(v.CalculateNormal(new xii.Vec3(-1, 0, 1), new xii.Vec3(1, 0, 1), new xii.Vec3(1, 0, 1)) == false);

        // MakeOrthogonalTo
        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new xii.Vec3(1, 0, 0));
        XII_TEST.VEC3(v, new xii.Vec3(0, 1, 0), 0.001);

        v.Set(1, 1, 0);
        v.MakeOrthogonalTo(new xii.Vec3(0, 1, 0));
        XII_TEST.VEC3(v, new xii.Vec3(1, 0, 0), 0.001);

        // GetOrthogonalVector
        for (let i = 1; i < 360; i += 3.0) {
            v.Set(i, i * 3, i * 7);
            XII_TEST.FLOAT(v.GetOrthogonalVector().Dot(v), 0.0, 0.001);
        }

        // GetReflectedVector
        v.Set(1, 1, 0);
        v2 = v.GetReflectedVector(new xii.Vec3(0, -1, 0));
        XII_TEST.VEC3(v2, new xii.Vec3(1, -1, 0), 0.0001);

        // SetAdd
        v.SetAdd(new xii.Vec3(1, 2, 3), new xii.Vec3(4, 5, 6));
        XII_TEST.VEC3(v, new xii.Vec3(5, 7, 9), 0.0001);

        // SetSub
        v.SetSub(new xii.Vec3(4, 5, 6), new xii.Vec3(1, 2, 3));
        XII_TEST.VEC3(v, new xii.Vec3(3, 3, 3), 0.0001);

        // SetMul
        v.SetMul(new xii.Vec3(1, 2, 3), 2);
        XII_TEST.VEC3(v, new xii.Vec3(2, 4, 6), 0.0001);

        // SetDiv
        v.SetDiv(new xii.Vec3(2, 4, 6), 2);
        XII_TEST.VEC3(v, new xii.Vec3(1, 2, 3), 0.0001);

        // SetLength
        v.Set(0, 2, 0);
        XII_TEST.BOOL(v.SetLength(5, 0.001));
        XII_TEST.VEC3(v, new xii.Vec3(0, 5, 0), 0.0001);

        // CreateRandomPointInSphere
        {
            let avg = new xii.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = xii.Vec3.CreateRandomPointInSphere();

                XII_TEST.BOOL(v.GetLength() <= 1.0);
                XII_TEST.BOOL(!v.IsZero());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            XII_TEST.BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new xii.Vec3();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = xii.Vec3.CreateRandomDirection();

                XII_TEST.BOOL(v.IsNormalized());

                avg.AddVec3(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            XII_TEST.BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestVec3") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

