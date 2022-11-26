import xii = require("TypeScript/xii")

export class MyMessage extends xii.Message {
    XII_DECLARE_MESSAGE_TYPE;

    text: string = "hello";
}

export class MyMessage2 extends xii.Message {
    XII_DECLARE_MESSAGE_TYPE;

    value: number = 0;
}

