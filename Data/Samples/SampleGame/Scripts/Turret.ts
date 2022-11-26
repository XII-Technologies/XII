import xii = require("TypeScript/xii")

export class Turret extends xii.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    Range: number = 3;
    /* END AUTO-GENERATED: VARIABLES */

    allTargets: xii.GameObject[] = [];
    lastDamageTime: number;

    constructor() {
        super()
    }

    OnSimulationStarted(): void {

        // update this component every frame
        this.SetTickInterval(xii.Time.Zero());
        this.lastDamageTime = xii.Time.GetGameTime();
    }

    FoundTargetCallback = (go: xii.GameObject): boolean => {
        this.allTargets.push(go);
        return true;
    }

    Tick(): void {

        let owner = this.GetOwner();

        // find all objects with the 'TurretTarget' marker that are close by
        this.allTargets = [];
        xii.World.FindObjectsInSphere("TurretTarget", owner.GetGlobalPosition(), this.Range, this.FoundTargetCallback);

        this.DrawLinesToTargets();

        if (xii.Time.GetGameTime() - this.lastDamageTime > xii.Time.Milliseconds(40)) {
            
            this.lastDamageTime = xii.Time.GetGameTime();

            this.DamageAllTargets(4);
        }
    }

    DrawLinesToTargets(): void {

        const startPos = this.GetOwner().GetGlobalPosition();

        let lines: xii.Debug.Line[] = [];

        for (let i = 0; i < this.allTargets.length; ++i) {

            const target = this.allTargets[i];
            const endPos = target.GetGlobalPosition();

            let line = new xii.Debug.Line();
            line.startX = startPos.x;
            line.startY = startPos.y;
            line.startZ = startPos.z;
            line.endX = endPos.x;
            line.endY = endPos.y;
            line.endZ = endPos.z;

            lines.push(line);
        }

        xii.Debug.DrawLines(lines, xii.Color.OrangeRed());
    }

    DamageAllTargets(damage: number): void {

        let dmgMsg = new xii.MsgDamage();
        dmgMsg.Damage = damage;

        for (let i = 0; i < this.allTargets.length; ++i) {

            this.allTargets[i].SendMessage(dmgMsg);
        }
    }
}

