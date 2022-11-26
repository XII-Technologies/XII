import xii = require("TypeScript/xii")

export class Prefab extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    NumberVar: number = 11;
    BoolVar: boolean = true;
    StringVar: string = "Hello";
    Vec3Var: xii.Vec3 = new xii.Vec3(1, 2, 3);
    ColorVar: xii.Color = new xii.Color(0.768151, 0.142913, 0.001891, 1);
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

    }
}

