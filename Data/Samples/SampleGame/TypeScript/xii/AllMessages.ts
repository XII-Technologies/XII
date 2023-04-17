// AUTO-GENERATED FILE

import __Message = require("TypeScript/xii/Message")
export import Message = __Message.Message;
export import EventMessage = __Message.EventMessage;

import __Vec2 = require("TypeScript/xii/Vec2")
export import Vec2 = __Vec2.Vec2;

import __Vec3 = require("TypeScript/xii/Vec3")
export import Vec3 = __Vec3.Vec3;

import __Mat3 = require("TypeScript/xii/Mat3")
export import Mat3 = __Mat3.Mat3;

import __Mat4 = require("TypeScript/xii/Mat4")
export import Mat4 = __Mat4.Mat4;

import __Quat = require("TypeScript/xii/Quat")
export import Quat = __Quat.Quat;

import __Transform = require("TypeScript/xii/Transform")
export import Transform = __Transform.Transform;

import __Color = require("TypeScript/xii/Color")
export import Color = __Color.Color;

import __Time = require("TypeScript/xii/Time")
export import Time = __Time.Time;

import __Angle = require("TypeScript/xii/Angle")
export import Angle = __Angle.Angle;

import Enum = require("./AllEnums")
import Flags = require("./AllFlags")


export class EventMsgPathChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 2034644494; }
  constructor() { super(); this.TypeNameHash = 2034644494; }
}

export class MsgAnimationPosePreparing extends Message
{
  public static GetTypeNameHash(): number { return 603069670; }
  constructor() { super(); this.TypeNameHash = 603069670; }
}

export class MsgAnimationPoseProposal extends Message
{
  public static GetTypeNameHash(): number { return 442796103; }
  constructor() { super(); this.TypeNameHash = 442796103; }
}

export class MsgAnimationPoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 2309337655; }
  constructor() { super(); this.TypeNameHash = 2309337655; }
}

export class MsgAnimationReachedEnd extends EventMessage
{
  public static GetTypeNameHash(): number { return 1395133569; }
  constructor() { super(); this.TypeNameHash = 1395133569; }
}

export class MsgApplyRootMotion extends Message
{
  public static GetTypeNameHash(): number { return 1080940775; }
  constructor() { super(); this.TypeNameHash = 1080940775; }
  Translation: Vec3 = new Vec3(0, 0, 0);
  RotationX: number = 0;
  RotationY: number = 0;
  RotationZ: number = 0;
}

export class MsgBlackboardEntryChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 2816185467; }
  constructor() { super(); this.TypeNameHash = 2816185467; }
  Name: string;
  OldValue: any;
  NewValue: any;
}

export class MsgBuildStaticMesh extends Message
{
  public static GetTypeNameHash(): number { return 2877979812; }
  constructor() { super(); this.TypeNameHash = 2877979812; }
}

export class MsgChildrenChanged extends Message
{
  public static GetTypeNameHash(): number { return 385419898; }
  constructor() { super(); this.TypeNameHash = 385419898; }
}

export class MsgCollision extends Message
{
  public static GetTypeNameHash(): number { return 4222123698; }
  constructor() { super(); this.TypeNameHash = 4222123698; }
}

export class MsgComponentInternalTrigger extends Message
{
  public static GetTypeNameHash(): number { return 2103644154; }
  constructor() { super(); this.TypeNameHash = 2103644154; }
  Message: string;
  Payload: number = 0;
}

export class MsgComponentsChanged extends Message
{
  public static GetTypeNameHash(): number { return 2464833325; }
  constructor() { super(); this.TypeNameHash = 2464833325; }
}

export class MsgDamage extends EventMessage
{
  public static GetTypeNameHash(): number { return 1647720812; }
  constructor() { super(); this.TypeNameHash = 1647720812; }
  Damage: number = 0;
  HitObjectName: string;
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  ImpactDirection: Vec3 = new Vec3(0, 0, 0);
}

export class MsgDeleteGameObject extends Message
{
  public static GetTypeNameHash(): number { return 685615647; }
  constructor() { super(); this.TypeNameHash = 685615647; }
}

export class MsgExtractGeometry extends Message
{
  public static GetTypeNameHash(): number { return 2119582475; }
  constructor() { super(); this.TypeNameHash = 2119582475; }
}

export class MsgExtractOccluderData extends Message
{
  public static GetTypeNameHash(): number { return 3852097655; }
  constructor() { super(); this.TypeNameHash = 3852097655; }
}

export class MsgExtractRenderData extends Message
{
  public static GetTypeNameHash(): number { return 1036716223; }
  constructor() { super(); this.TypeNameHash = 1036716223; }
}

export class MsgGenericEvent extends EventMessage
{
  public static GetTypeNameHash(): number { return 2595993530; }
  constructor() { super(); this.TypeNameHash = 2595993530; }
  Message: string;
  Value: any = 0;
}

export class MsgInputActionTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 735344585; }
  constructor() { super(); this.TypeNameHash = 735344585; }
  InputAction: string;
  KeyPressValue: number = 0;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgMoveCharacterController extends Message
{
  public static GetTypeNameHash(): number { return 516338324; }
  constructor() { super(); this.TypeNameHash = 516338324; }
  MoveForwards: number = 0;
  MoveBackwards: number = 0;
  StrafeLeft: number = 0;
  StrafeRight: number = 0;
  RotateLeft: number = 0;
  RotateRight: number = 0;
  Run: boolean = false;
  Jump: boolean = false;
  Crouch: boolean = false;
}

export class MsgObjectGrabbed extends Message
{
  public static GetTypeNameHash(): number { return 886514815; }
  constructor() { super(); this.TypeNameHash = 886514815; }
  GotGrabbed: boolean = false;
}

export class MsgOnlyApplyToObject extends Message
{
  public static GetTypeNameHash(): number { return 3594874705; }
  constructor() { super(); this.TypeNameHash = 3594874705; }
}

export class MsgParentChanged extends Message
{
  public static GetTypeNameHash(): number { return 800218897; }
  constructor() { super(); this.TypeNameHash = 800218897; }
}

export class MsgPhysicsAddForce extends Message
{
  public static GetTypeNameHash(): number { return 2232314321; }
  constructor() { super(); this.TypeNameHash = 2232314321; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Force: Vec3 = new Vec3(0, 0, 0);
}

export class MsgPhysicsAddImpulse extends Message
{
  public static GetTypeNameHash(): number { return 302348364; }
  constructor() { super(); this.TypeNameHash = 302348364; }
  GlobalPosition: Vec3 = new Vec3(0, 0, 0);
  Impulse: Vec3 = new Vec3(0, 0, 0);
  ObjectFilterID: number = 0;
}

export class MsgPhysicsJointBroke extends EventMessage
{
  public static GetTypeNameHash(): number { return 80086628; }
  constructor() { super(); this.TypeNameHash = 80086628; }
}

export class MsgQueryAnimationSkeleton extends Message
{
  public static GetTypeNameHash(): number { return 1154169994; }
  constructor() { super(); this.TypeNameHash = 1154169994; }
}

export class MsgReleaseObjectGrab extends Message
{
  public static GetTypeNameHash(): number { return 2127815804; }
  constructor() { super(); this.TypeNameHash = 2127815804; }
}

export class MsgRetrieveBoneState extends Message
{
  public static GetTypeNameHash(): number { return 481061292; }
  constructor() { super(); this.TypeNameHash = 481061292; }
}

export class MsgRopePoseUpdated extends Message
{
  public static GetTypeNameHash(): number { return 262905676; }
  constructor() { super(); this.TypeNameHash = 262905676; }
}

export class MsgSensorDetectedObjectsChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 2197834590; }
  constructor() { super(); this.TypeNameHash = 2197834590; }
}

export class MsgSetColor extends Message
{
  public static GetTypeNameHash(): number { return 3159067097; }
  constructor() { super(); this.TypeNameHash = 3159067097; }
  Color: Color = new Color(1, 1, 1, 1);
  Mode: Enum.SetColorMode = 0;
}

export class MsgSetDoubleParameter extends Message
{
  public static GetTypeNameHash(): number { return 3543954005; }
  constructor() { super(); this.TypeNameHash = 3543954005; }
  Name: string;
  Value: number = 0;
}

export class MsgSetFloatParameter extends Message
{
  public static GetTypeNameHash(): number { return 1798132902; }
  constructor() { super(); this.TypeNameHash = 1798132902; }
  Name: string;
  Value: number = 0;
}

export class MsgSetMeshMaterial extends Message
{
  public static GetTypeNameHash(): number { return 3324481330; }
  constructor() { super(); this.TypeNameHash = 3324481330; }
  Material: string;
  MaterialSlot: number = 0;
}

export class MsgSetPlaying extends Message
{
  public static GetTypeNameHash(): number { return 3682949865; }
  constructor() { super(); this.TypeNameHash = 3682949865; }
  Play: boolean = true;
}

export class MsgSetRealParameter extends Message
{
  public static GetTypeNameHash(): number { return 4118799405; }
  constructor() { super(); this.TypeNameHash = 4118799405; }
  Name: string;
  Value: number = 0;
}

export class MsgSetText extends Message
{
  public static GetTypeNameHash(): number { return 3919971841; }
  constructor() { super(); this.TypeNameHash = 3919971841; }
}

export class MsgStateMachineStateChanged extends EventMessage
{
  public static GetTypeNameHash(): number { return 4002777761; }
  constructor() { super(); this.TypeNameHash = 4002777761; }
  OldStateName: string;
  NewStateName: string;
}

export class MsgTransformChanged extends Message
{
  public static GetTypeNameHash(): number { return 2829794685; }
  constructor() { super(); this.TypeNameHash = 2829794685; }
}

export class MsgTriggerTriggered extends EventMessage
{
  public static GetTypeNameHash(): number { return 932027644; }
  constructor() { super(); this.TypeNameHash = 932027644; }
  Message: string;
  TriggerState: Enum.TriggerState = 0;
}

export class MsgTypeScriptMsgProxy extends Message
{
  public static GetTypeNameHash(): number { return 21713039; }
  constructor() { super(); this.TypeNameHash = 21713039; }
}

export class MsgUpdateLocalBounds extends Message
{
  public static GetTypeNameHash(): number { return 290984384; }
  constructor() { super(); this.TypeNameHash = 290984384; }
}

