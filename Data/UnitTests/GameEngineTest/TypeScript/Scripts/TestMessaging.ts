import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")
import shared = require("./Shared")
import helper = require("./HelperComponent")

export class TestMessaging extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
        xii.TypescriptComponent.RegisterMessageHandler(shared.MyMessage, "OnMyMessage");
        xii.TypescriptComponent.RegisterMessageHandler(shared.MyMessage2, "OnMyMessage2");
    }

    step: number = 0;
    msgCount: number = 0;
    gotEvent: boolean = false;

    ExecuteTests(): boolean {

        if (this.step == 0) {
            let m = new shared.MyMessage();

            m.text = "hello 1";
            this.SendMessage(m, true);
            XII_TEST.BOOL(m.text == "Got: hello 1");

            m.text = "hello 2";
            this.SendMessage(m, false);
            XII_TEST.BOOL(m.text == "Got: hello 2");

            m.text = "hello 3";
            this.GetOwner().SendMessage(m, true);
            XII_TEST.BOOL(m.text == "Got: hello 3");

            m.text = "hello 4";
            this.GetOwner().GetParent().SendMessageRecursive(m, true);
            XII_TEST.BOOL(m.text == "Got: hello 4");

            return true;
        }

        if (this.step == 1) {
            let m = new shared.MyMessage2;
            m.value = 1;

            this.PostMessage(m);

            XII_TEST.INT(this.msgCount, 0);

            return true;
        }

        if (this.step == 2) {

            XII_TEST.INT(this.msgCount, 1);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().PostMessage(m);

            XII_TEST.INT(this.msgCount, 1);

            return true;
        }

        if (this.step == 3) {

            XII_TEST.INT(this.msgCount, 2);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().GetParent().PostMessage(m);

            XII_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 4) {

            XII_TEST.INT(this.msgCount, 2);

            let m = new shared.MyMessage2;
            m.value = 1;

            this.GetOwner().GetParent().PostMessageRecursive(m);

            XII_TEST.INT(this.msgCount, 2);

            return true;
        }

        if (this.step == 5) {

            XII_TEST.INT(this.msgCount, 3);

            let children = this.GetOwner().GetChildren();
            XII_TEST.INT(children.length, 1);

            let hc: helper.HelperComponent = children[0].TryGetScriptComponent("HelperComponent");
            XII_TEST.BOOL(hc != null);

            XII_TEST.BOOL(!this.gotEvent);

            let te = new xii.MsgGenericEvent;
            te.Message = "Event1";

            hc.SendMessage(te);

            XII_TEST.BOOL(this.gotEvent);
            this.gotEvent = false;

            hc.RaiseEvent("e1");
            XII_TEST.BOOL(this.gotEvent);

            return true;
        }

        if (this.step == 6) {

            XII_TEST.INT(this.msgCount, 3);

            return true;
        }

        return false;
    }

    OnMyMessage(msg: shared.MyMessage) {

        msg.text = "Got: " + msg.text;
    }

    OnMyMessage2(msg: shared.MyMessage2) {
        this.msgCount += msg.value;
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestMessaging") {


            if (this.ExecuteTests()) {
                msg.Message = "repeat";
            }
            else {

                XII_TEST.INT(this.msgCount, 3);

                msg.Message = "done";
            }

            this.step += 1;
        }

        if (msg.Message == "e1") {
            this.gotEvent = true;
        }
    }
}

