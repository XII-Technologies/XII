import xii = require("TypeScript/xii")

export class TargetSphere extends xii.TickedTypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {

        // you can only call "RegisterMessageHandler" from within this function
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgDamage, "OnMsgDamage");
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgInputActionTriggered, "OnMsgInputActionTriggered");
    }

    curDamage = 0;
    fireFX: xii.ParticleComponent = null;

    OnSimulationStarted(): void {
        this.SetTickInterval(xii.Time.Milliseconds(100));

        this.fireFX = this.GetOwner().TryGetComponentOfBaseType(xii.ParticleComponent);
    }

    OnMsgDamage(msg: xii.MsgDamage): void {

        this.curDamage += msg.Damage;
    }

    OnMsgInputActionTriggered(msg: xii.MsgInputActionTriggered) {
     
        if (msg.TriggerState == xii.TriggerState.Activated) {
            if (msg.InputAction == "Heal") {
                this.curDamage = 0;
            }
        }
    }

    Tick(): void {

        this.curDamage = xii.Utils.Clamp(this.curDamage - 1.0, 0, 1000);
        const dmg = this.curDamage / 100.0;

        let msgCol = new xii.MsgSetColor();
        msgCol.Color.SetLinearRGBA(dmg, dmg * 0.05, dmg * 0.05);

        this.GetOwner().SendMessageRecursive(msgCol);

        if (this.fireFX != null && this.fireFX.IsValid()) {

            if (dmg > 1.0) {
                if (!this.fireFX.IsEffectActive()) {
                    this.fireFX.StartEffect();
                }
            }
            else if (dmg < 0.8) {
                this.fireFX.StopEffect();
            }
        }
    }
}

