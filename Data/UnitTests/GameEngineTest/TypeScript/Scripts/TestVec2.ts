import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestVec2 extends xii.TypescriptComponent {

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
        let d = new xii.Vec2();
        XII_TEST.FLOAT(d.x, 0, 0.001);
        XII_TEST.FLOAT(d.y, 0, 0.001);

        // ZeroVector
        XII_TEST.VEC2(new xii.Vec2(), xii.Vec2.ZeroVector(), 0.0001);

        let v = new xii.Vec2(1, 2);
        XII_TEST.FLOAT(v.x, 1, 0.001);
        XII_TEST.FLOAT(v.y, 2, 0.001);

        // Clone
        XII_TEST.VEC2(v.Clone(), v, 0.001);

        // TODO: CloneAsVec3
        //XII_TEST.VEC3(v.CloneAsVec3(3), new xii.Vec3(1, 2, 3), 0.001);

        // OneVector
        XII_TEST.VEC2(new xii.Vec2(1, 1), xii.Vec2.OneVector(), 0.0001);

        // UnitAxisX
        XII_TEST.VEC2(new xii.Vec2(1, 0), xii.Vec2.UnitAxisX(), 0.0001);

        // UnitAxisY
        XII_TEST.VEC2(new xii.Vec2(0, 1), xii.Vec2.UnitAxisY(), 0.0001);

        // Set
        v.Set(4, 5);
        XII_TEST.FLOAT(v.x, 4, 0.001);
        XII_TEST.FLOAT(v.y, 5, 0.001);

        // SetVec2
        let v2 = new xii.Vec2();
        v2.SetVec2(v);
        XII_TEST.VEC2(v, v2, 0.0001);

        // SetAll
        v2.SetAll(7);
        XII_TEST.FLOAT(v2.x, 7, 0.001);
        XII_TEST.FLOAT(v2.y, 7, 0.001);

        // SetZero
        v2.SetZero();
        XII_TEST.FLOAT(v2.x, 0, 0.001);
        XII_TEST.FLOAT(v2.y, 0, 0.001);

        // GetLengthSquared
        XII_TEST.FLOAT(v2.GetLengthSquared(), 0, 0.001);
        v2.SetAll(1);

        XII_TEST.FLOAT(v2.GetLengthSquared(), 2, 0.001);

        // GetLength
        XII_TEST.FLOAT(v2.GetLength(), Math.sqrt(2), 0.001);

        // GetLengthAndNormalize
        let l = v2.GetLengthAndNormalize();
        XII_TEST.FLOAT(l, Math.sqrt(2), 0.001);
        XII_TEST.FLOAT(v2.GetLength(), 1, 0.001);

        // IsNormalized
        XII_TEST.BOOL(!v.IsNormalized());

        // Normalize
        v.Normalize();

        XII_TEST.FLOAT(v.GetLength(), 1, 0.001);
        XII_TEST.BOOL(v.IsNormalized());


        // GetNormalized
        v.Set(3, 0);
        XII_TEST.VEC2(v.GetNormalized(), xii.Vec2.UnitAxisX(), 0.0001);

        // NormalizeIfNotZero
        XII_TEST.BOOL(v.NormalizeIfNotZero(xii.Vec2.UnitAxisY(), 0.001));
        XII_TEST.VEC2(v, xii.Vec2.UnitAxisX(), 0.0001);

        // IsZero
        XII_TEST.BOOL(!v.IsZero());

        // SetZero
        v.SetZero();
        XII_TEST.BOOL(v.IsZero());

        XII_TEST.BOOL(!v.NormalizeIfNotZero(xii.Vec2.UnitAxisY(), 0.001));
        XII_TEST.VEC2(v, xii.Vec2.UnitAxisY(), 0.0001);

        // GetNegated
        v.Set(1, 2);
        XII_TEST.VEC2(v.GetNegated(), new xii.Vec2(-1, -2), 0.0001);
        XII_TEST.VEC2(v, new xii.Vec2(1, 2), 0.0001);

        // Negate
        v.Negate();
        XII_TEST.VEC2(v, new xii.Vec2(-1, -2), 0.0001);

        // AddVec2
        v.Set(2, 3);
        v2.Set(5, 6);
        v.AddVec2(v2);
        XII_TEST.VEC2(v, new xii.Vec2(7, 9), 0.0001);

        // SubVec2
        v.SubVec2(v2);
        XII_TEST.VEC2(v, new xii.Vec2(2, 3), 0.0001);

        // MulVec2
        v.MulVec2(v);
        XII_TEST.VEC2(v, new xii.Vec2(4, 9), 0.0001);

        // DivVec2
        v.DivVec2(new xii.Vec2(2, 3));
        XII_TEST.VEC2(v, new xii.Vec2(2, 3), 0.0001);

        // MulNumber
        v.MulNumber(2);
        XII_TEST.VEC2(v, new xii.Vec2(4, 6), 0.0001);

        // DivNumber
        v.DivNumber(2);
        XII_TEST.VEC2(v, new xii.Vec2(2, 3), 0.0001);

        // IsIdentical
        XII_TEST.BOOL(v.IsIdentical(v));
        XII_TEST.BOOL(!v.IsIdentical(v2));

        // IsEqual
        XII_TEST.BOOL(v.IsEqual(new xii.Vec2(2, 3), 0.0001));
        XII_TEST.BOOL(!v.IsEqual(new xii.Vec2(2, 3.5), 0.0001));

        // Dot
        v.Set(2, 3);
        v2.Set(3, 4);
        XII_TEST.FLOAT(v.Dot(v2), 18, 0.001);

        // GetCompMin
        v.Set(2, 4);
        v2.Set(1, 5);
        XII_TEST.VEC2(v.GetCompMin(v2), new xii.Vec2(1, 4), 0.001);

        // GetCompMax
        v.Set(2, 4);
        v2.Set(1, 5);
        XII_TEST.VEC2(v.GetCompMax(v2), new xii.Vec2(2, 5), 0.001);

        // GetCompClamp
        XII_TEST.VEC2(v.GetCompClamp(new xii.Vec2(3, 4), new xii.Vec2(4, 5)), new xii.Vec2(3, 4), 0.001);

        // GetCompMul
        XII_TEST.VEC2(v.GetCompMul(new xii.Vec2(2, 3)), new xii.Vec2(4, 12), 0.001);

        // GetCompDiv
        XII_TEST.VEC2(v.GetCompDiv(new xii.Vec2(2, 4)), xii.Vec2.OneVector(), 0.001);

        // GetAbs
        v.Set(-1, -2);
        XII_TEST.VEC2(v.GetAbs(), new xii.Vec2(1, 2), 0.001);

        // SetAbs
        v2.SetAbs(v);
        XII_TEST.VEC2(v2, new xii.Vec2(1, 2), 0.001);

        // GetReflectedVector
        v.Set(1, 1);
        v2 = v.GetReflectedVector(new xii.Vec2(0, -1));
        XII_TEST.VEC2(v2, new xii.Vec2(1, -1), 0.0001);

        // SetAdd
        v.SetAdd(new xii.Vec2(1, 2), new xii.Vec2(4, 5));
        XII_TEST.VEC2(v, new xii.Vec2(5, 7), 0.0001);

        // SetSub
        v.SetSub(new xii.Vec2(4, 5), new xii.Vec2(1, 2));
        XII_TEST.VEC2(v, new xii.Vec2(3, 3), 0.0001);

        // SetMul
        v.SetMul(new xii.Vec2(1, 2), 2);
        XII_TEST.VEC2(v, new xii.Vec2(2, 4), 0.0001);

        // SetDiv
        v.SetDiv(new xii.Vec2(2, 4), 2);
        XII_TEST.VEC2(v, new xii.Vec2(1, 2), 0.0001);

        // CreateRandomPointInCircle
        {
            let avg = new xii.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = xii.Vec2.CreateRandomPointInCircle();

                XII_TEST.BOOL(v.GetLength() <= 1.0);
                XII_TEST.BOOL(!v.IsZero());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            XII_TEST.BOOL(avg.IsZero(0.1));
        }

        // CreateRandomDirection
        {
            let avg = new xii.Vec2();

            const uiNumSamples = 1000;
            for (let i = 0; i < uiNumSamples; ++i) {
                v = xii.Vec2.CreateRandomDirection();

                XII_TEST.BOOL(v.IsNormalized());

                avg.AddVec2(v);
            }

            avg.DivNumber(uiNumSamples);

            // the average point cloud center should be within at least 10% of the sphere's center
            // otherwise the points aren't equally distributed
            XII_TEST.BOOL(avg.IsZero(0.1));
        }
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestVec2") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

