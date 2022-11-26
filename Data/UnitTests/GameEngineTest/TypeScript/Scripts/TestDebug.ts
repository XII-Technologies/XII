import xii = require("TypeScript/xii")
import XII_TEST = require("./TestFramework")

export class TestDebug extends xii.TypescriptComponent {

    /* BEGIN AUTO-GENERATED: VARIABLES */
    /* END AUTO-GENERATED: VARIABLES */

    constructor() {
        super()
    }

    static RegisterMessageHandlers() {
        xii.TypescriptComponent.RegisterMessageHandler(xii.MsgGenericEvent, "OnMsgGenericEvent");
    }

    ExecuteTests(): void {

        
        xii.Debug.RegisterCVar_Boolean("test.bool", true, "bool");
        XII_TEST.BOOL(xii.Debug.ReadCVar_Boolean("test.bool") == true);
        xii.Debug.WriteCVar_Boolean("test.bool", false);
        XII_TEST.BOOL(xii.Debug.ReadCVar_Boolean("test.bool") == false);
        
        xii.Debug.RegisterCVar_Int("test.int", 12, "int");
        XII_TEST.BOOL(xii.Debug.ReadCVar_Int("test.int") == 12);
        xii.Debug.WriteCVar_Int("test.int", -12);
        XII_TEST.BOOL(xii.Debug.ReadCVar_Int("test.int") == -12);
        
        xii.Debug.RegisterCVar_Float("test.float", 19, "float");
        XII_TEST.BOOL(xii.Debug.ReadCVar_Float("test.float") == 19);
        xii.Debug.WriteCVar_Float("test.float", -19);
        XII_TEST.BOOL(xii.Debug.ReadCVar_Float("test.float") == -19);

        xii.Debug.RegisterCVar_String("test.string", "hello", "string");
        XII_TEST.BOOL(xii.Debug.ReadCVar_String("test.string") == "hello");
        xii.Debug.WriteCVar_String("test.string", "world");
        XII_TEST.BOOL(xii.Debug.ReadCVar_String("test.string") == "world");
    }

    OnMsgGenericEvent(msg: xii.MsgGenericEvent): void {

        if (msg.Message == "TestDebug") {

            this.ExecuteTests();

            msg.Message = "done";
        }
    }

}

