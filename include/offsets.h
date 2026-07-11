#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// Roblox memory offsets
// Organised as: namespace ClassName { constexpr uintptr_t FieldName = 0x…; }
// ---------------------------------------------------------------------------
namespace offsets {

// ── Global static pointers (module-relative addresses) ─────────────────────
namespace PlayerConfigurer { constexpr uintptr_t Pointer                = 0x0; }
namespace MouseService     { constexpr uintptr_t SensitivityPointer     = 0x0;
                             constexpr uintptr_t InputObject             = 0xf0;
                             constexpr uintptr_t InputObject2            = 0x100;
                             constexpr uintptr_t MousePosition           = 0xd4; }
namespace ScriptContext    { constexpr uintptr_t RequireBypass           = 0x0; }
namespace ModuleScript     { constexpr uintptr_t IsCoreScript            = 0x0;
                             constexpr uintptr_t GUID                    = 0xd0;
                             constexpr uintptr_t Hash                    = 0x148;
                             constexpr uintptr_t ByteCode                = 0x138; }

namespace TaskScheduler {
    constexpr uintptr_t Pointer   = 0x8041ec8;
    constexpr uintptr_t JobStart  = 0xc8;
    constexpr uintptr_t JobEnd    = 0xd0;
    constexpr uintptr_t JobName   = 0x18;
    constexpr uintptr_t MaxFPS    = 0xb0;
}
namespace VisualEngine {
    constexpr uintptr_t Pointer       = 0x81d61c8;
    constexpr uintptr_t Dimensions    = 0xab0;
    constexpr uintptr_t ViewMatrix    = 0x150;
    constexpr uintptr_t RenderView    = 0xbb8;
    constexpr uintptr_t FakeDataModel = 0xa90;
}
namespace FakeDataModel {
    constexpr uintptr_t Pointer       = 0x84a9e98;
    constexpr uintptr_t RealDataModel = 0x1d0;
}

// ── RenderView ──────────────────────────────────────────────────────────────
namespace RenderView {
    constexpr uintptr_t LightingValid = 0x150;
    constexpr uintptr_t SkyValid      = 0x28d;
    constexpr uintptr_t VisualEngine  = 0x10;
    constexpr uintptr_t DeviceD3D11   = 0x8;
}
namespace RenderJob {
    constexpr uintptr_t RenderView    = 0x1d0;
    constexpr uintptr_t FakeDataModel = 0x38;
    constexpr uintptr_t RealDataModel = 0x1c8;
}

// ── Instance base fields ────────────────────────────────────────────────────
namespace Instance {
    constexpr uintptr_t This             = 0x8;
    constexpr uintptr_t Name             = 0x98;
    constexpr uintptr_t ChildrenStart    = 0x70;
    constexpr uintptr_t ChildrenEnd      = 0x8;   // offset from ChildrenStart
    constexpr uintptr_t Parent           = 0x68;
    constexpr uintptr_t ClassDescriptor  = 0x18;
    constexpr uintptr_t ClassName        = 0x8;   // offset from ClassDescriptor
    constexpr uintptr_t ClassBase        = 0x230;
    constexpr uintptr_t ComponentMap     = 0x38;
}
namespace Misc {
    constexpr uintptr_t StringLength = 0x10;
    constexpr uintptr_t Adornee      = 0xf0;
    constexpr uintptr_t Value        = 0xb8;
    constexpr uintptr_t AnimationId  = 0xc0;
}

// ── DataModel ────────────────────────────────────────────────────────────────
namespace DataModel {
    constexpr uintptr_t PlaceId        = 0x190;
    constexpr uintptr_t GameId         = 0x188;
    constexpr uintptr_t CreatorId      = 0x180;
    constexpr uintptr_t GameLoaded     = 0x668;
    constexpr uintptr_t JobId          = 0x120;
    constexpr uintptr_t Workspace      = 0x160;
    constexpr uintptr_t ScriptContext  = 0x440;
    constexpr uintptr_t PlaceVersion   = 0x1ac;
    constexpr uintptr_t ServerIP       = 0x650;
    constexpr uintptr_t PrimitiveCount = 0x498;
    constexpr uintptr_t ToRenderView1  = 0x1c8;
    constexpr uintptr_t ToRenderView2  = 0x8;
    constexpr uintptr_t ToRenderView3  = 0x28;
}

// ── Workspace / World ────────────────────────────────────────────────────────
namespace Workspace {
    constexpr uintptr_t World                 = 0x3e0;
    constexpr uintptr_t ReadOnlyGravity       = 0x998;
    constexpr uintptr_t DistributedGameTime   = 0x4a8;
    constexpr uintptr_t CurrentCamera         = 0x488;
}
namespace World {
    constexpr uintptr_t Gravity                  = 0x210;
    constexpr uintptr_t worldStepsPerSec         = 0x680;
    constexpr uintptr_t FallenPartsDestroyHeight = 0x208;
    constexpr uintptr_t AirProperties            = 0x218;
    constexpr uintptr_t Primitives               = 0x288;
}
namespace AirProperties {
    constexpr uintptr_t AirDensity  = 0x18;
    constexpr uintptr_t GlobalWind  = 0x3c;
}

// ── RunService ───────────────────────────────────────────────────────────────
namespace RunService {
    constexpr uintptr_t HeartbeatTask = 0xc08;
    constexpr uintptr_t HeartbeatFPS  = 0xb4;
}

// ── Camera ───────────────────────────────────────────────────────────────────
namespace Camera {
    constexpr uintptr_t Position       = 0xfc;
    constexpr uintptr_t Rotation       = 0xd8;
    constexpr uintptr_t CameraSubject  = 0xc8;
    constexpr uintptr_t FieldOfView    = 0x140;
    constexpr uintptr_t ImagePlaneDepth= 0x2d0;
    constexpr uintptr_t CameraType     = 0x138;
    constexpr uintptr_t Viewport       = 0x28c;
    constexpr uintptr_t ViewportSize   = 0x2c8;
}

// ── Player ───────────────────────────────────────────────────────────────────
namespace Player {
    constexpr uintptr_t LocalPlayer          = 0x130;
    constexpr uintptr_t UserId               = 0x300;
    constexpr uintptr_t DisplayName          = 0x138;
    constexpr uintptr_t HealthDisplayDistance= 0x390;
    constexpr uintptr_t NameDisplayDistance  = 0x3a0;
    constexpr uintptr_t ModelInstance        = 0x298;
    constexpr uintptr_t Team                 = 0x2d8;
    constexpr uintptr_t TeamColor            = 0x3ac;
    constexpr uintptr_t LocaleId             = 0x118;
    constexpr uintptr_t AccountAge           = 0x35c;
    constexpr uintptr_t MinZoomDistance      = 0x36c;
    constexpr uintptr_t MaxZoomDistance      = 0x368;
    constexpr uintptr_t CameraMode           = 0x370;
    constexpr uintptr_t Mouse                = 0x11d8;
}
namespace Team {
    constexpr uintptr_t BrickColor = 0xb8;
}
namespace PlayerMouse {
    constexpr uintptr_t Workspace = 0x150;
    constexpr uintptr_t Icon      = 0xc8;
}

// ── Humanoid ─────────────────────────────────────────────────────────────────
namespace Humanoid {
    constexpr uintptr_t Health                   = 0x188;
    constexpr uintptr_t MaxHealth                = 0x1a8;
    constexpr uintptr_t Walkspeed                = 0x1d0;
    constexpr uintptr_t WalkspeedCheck           = 0x3bc;
    constexpr uintptr_t JumpPower                = 0x1a4;
    constexpr uintptr_t JumpHeight               = 0x1a0;
    constexpr uintptr_t HipHeight                = 0x194;
    constexpr uintptr_t MaxSlopeAngle            = 0x1ac;
    constexpr uintptr_t SeatPart                 = 0x108;
    constexpr uintptr_t HumanoidRootPart         = 0x478;
    constexpr uintptr_t CameraOffset             = 0x128;
    constexpr uintptr_t HealthDisplayDistance    = 0x18c;
    constexpr uintptr_t NameDisplayDistance      = 0x1b0;
    constexpr uintptr_t DisplayDistanceType      = 0x180;
    constexpr uintptr_t HealthDisplayType        = 0x190;
    constexpr uintptr_t NameOcclusion            = 0x1b4;
    constexpr uintptr_t DisplayName              = 0xb8;
    constexpr uintptr_t MoveDirection            = 0x140;
    constexpr uintptr_t RigType                  = 0x1c0;
    constexpr uintptr_t Jump                     = 0x1da;
    constexpr uintptr_t Sit                      = 0x1dd;
    constexpr uintptr_t PlatformStand            = 0x1dc;
    constexpr uintptr_t UseJumpPower             = 0x1e0;
    constexpr uintptr_t AutomaticScalingEnabled  = 0x1d6;
    constexpr uintptr_t BreakJointsOnDeath       = 0x1d7;
    constexpr uintptr_t EvaluateStateMachine     = 0x1d8;
    constexpr uintptr_t RequiresNeck             = 0x1dd;
    constexpr uintptr_t AutoJumpEnabled          = 0x1d4;
    constexpr uintptr_t AutoRotate               = 0x1d5;
    constexpr uintptr_t IsWalking                = 0x917;
    constexpr uintptr_t MoveToPoint              = 0x164;
    constexpr uintptr_t MoveToPart               = 0x118;
    constexpr uintptr_t WalkTimer                = 0x408;
    constexpr uintptr_t HumanoidState            = 0x898;
    constexpr uintptr_t HumanoidStateID          = 0x20;
    constexpr uintptr_t FloorMaterial            = 0x184;
    constexpr uintptr_t TargetPoint              = 0x14c;
}

// ── BasePart / Primitive ─────────────────────────────────────────────────────
namespace BasePart {
    constexpr uintptr_t Primitive    = 0x128;
    constexpr uintptr_t Transparency = 0xd0;
    constexpr uintptr_t Color3       = 0x148;
    constexpr uintptr_t Shape        = 0x159;
    constexpr uintptr_t Massless     = 0xd7;
    constexpr uintptr_t CastShadow   = 0xd5;
    constexpr uintptr_t Locked       = 0xd6;
    constexpr uintptr_t Reflectance  = 0xcc;
}
namespace Primitive {
    constexpr uintptr_t Position                = 0xec;
    constexpr uintptr_t Validate               = 0x6;
    constexpr uintptr_t Owner                  = 0x208;
    constexpr uintptr_t Size                   = 0x1b8;
    constexpr uintptr_t Rotation               = 0xc8;
    constexpr uintptr_t Flags                  = 0x1b6;
    constexpr uintptr_t Material               = 0x0;
    constexpr uintptr_t AssemblyLinearVelocity = 0xf8;
    constexpr uintptr_t AssemblyAngularVelocity= 0x104;
}
namespace PrimitiveFlags {
    constexpr uintptr_t Anchored   = 0x2;
    constexpr uintptr_t CanCollide = 0x8;
    constexpr uintptr_t CanTouch   = 0x10;
    constexpr uintptr_t CanQuery   = 0x20;
}

// ── Model / Weld / Attachment ────────────────────────────────────────────────
namespace Model {
    constexpr uintptr_t PrimaryPart = 0x258;
    constexpr uintptr_t Scale       = 0x144;
}
namespace SpecialMesh {
    constexpr uintptr_t Scale  = 0xc4;
    constexpr uintptr_t MeshId = 0xf8;
}
namespace Attachment {
    constexpr uintptr_t Position = 0xc4;
}
namespace Weld {
    constexpr uintptr_t Part0 = 0x118;
    constexpr uintptr_t Part1 = 0x128;
}
namespace WeldConstraint {
    constexpr uintptr_t Part0 = 0xb8;
    constexpr uintptr_t Part1 = 0xc8;
}
namespace UnionOperation {
    constexpr uintptr_t AssetId = 0x288;
}

// ── MeshPart / MeshContentProvider ──────────────────────────────────────────
namespace MeshPart {
    constexpr uintptr_t MeshId  = 0x290;
    constexpr uintptr_t Texture = 0x2c0;
}
namespace MeshContentProvider {
    constexpr uintptr_t Cache      = 0xf0;
    constexpr uintptr_t LRUCache   = 0x20;
    constexpr uintptr_t MeshData   = 0x40;
    constexpr uintptr_t ToMeshData = 0x40;
    constexpr uintptr_t AssetID    = 0x10;
}
namespace MeshData {
    constexpr uintptr_t FaceEnd    = 0x38;
    constexpr uintptr_t FaceStart  = 0x30;
    constexpr uintptr_t VertexEnd  = 0x8;
    constexpr uintptr_t VertexStart= 0x0;
}

// ── Attributes ───────────────────────────────────────────────────────────────
namespace AttributesMap {
    constexpr uintptr_t Length     = 0x0;
    constexpr uintptr_t Attributes = 0x10;
}
namespace Attribute {
    constexpr uintptr_t Key   = 0x0;
    constexpr uintptr_t Value = 0x18;
    constexpr uintptr_t Size  = 0x58;
}

// ── Seat / VehicleSeat ───────────────────────────────────────────────────────
namespace Seat {
    constexpr uintptr_t Occupant = 0x1b0;
}
namespace VehicleSeat {
    constexpr uintptr_t MaxSpeed     = 0x1c8;
    constexpr uintptr_t SteerFloat   = 0x1d0;
    constexpr uintptr_t ThrottleFloat= 0x1d8;
    constexpr uintptr_t Torque       = 0x1dc;
    constexpr uintptr_t TurnSpeed    = 0x1e0;
}

// ── StatsItem / Tool / Clothing / CharacterMesh ──────────────────────────────
namespace StatsItem { constexpr uintptr_t Value = 0xc8; }
namespace Tool {
    constexpr uintptr_t Tooltip             = 0x468;
    constexpr uintptr_t TextureId           = 0x360;
    constexpr uintptr_t Grip                = 0x4ac;
    constexpr uintptr_t Enabled             = 0x4b9;
    constexpr uintptr_t CanBeDropped        = 0x4b8;
    constexpr uintptr_t ManualActivationOnly= 0x4ba;
    constexpr uintptr_t RequiresHandle      = 0x4bb;
}
namespace Clothing {
    constexpr uintptr_t Template = 0x100;
    constexpr uintptr_t Color3   = 0x120;
}
namespace CharacterMesh {
    constexpr uintptr_t BaseTextureId    = 0xc8;
    constexpr uintptr_t OverlayTextureId = 0x128;
    constexpr uintptr_t MeshId           = 0xf8;
    constexpr uintptr_t BodyPart         = 0x148;
}

// ── SpawnLocation ────────────────────────────────────────────────────────────
namespace SpawnLocation {
    constexpr uintptr_t AllowTeamChangeOnTouch = 0x3d;
    constexpr uintptr_t Enabled               = 0x189;
    constexpr uintptr_t Neutral               = 0x18a;
    constexpr uintptr_t ForcefieldDuration    = 0x180;
    constexpr uintptr_t TeamColor             = 0x184;
}

// ── Sound ────────────────────────────────────────────────────────────────────
namespace Sound {
    constexpr uintptr_t SoundId            = 0xc8;
    constexpr uintptr_t RollOffMaxDistance = 0x120;
    constexpr uintptr_t RollOffMinDistance = 0x124;
    constexpr uintptr_t PlaybackSpeed      = 0x11c;
    constexpr uintptr_t Volume             = 0x130;
    constexpr uintptr_t SoundGroup         = 0xe8;
    constexpr uintptr_t Looped             = 0x13d;
}

// ── SurfaceAppearance ────────────────────────────────────────────────────────
namespace SurfaceAppearance {
    constexpr uintptr_t AlphaMode            = 0x288;
    constexpr uintptr_t Color                = 0x270;
    constexpr uintptr_t ColorMap             = 0xc8;
    constexpr uintptr_t EmissiveMaskContent  = 0xf8;
    constexpr uintptr_t EmissiveStrength     = 0x28c;
    constexpr uintptr_t EmissiveTint         = 0x27c;
    constexpr uintptr_t MetalnessMap         = 0x128;
    constexpr uintptr_t NormalMap            = 0x158;
    constexpr uintptr_t RoughnessMap         = 0x188;
}

// ── ParticleEmitter / Beam ───────────────────────────────────────────────────
namespace ParticleEmitter {
    constexpr uintptr_t Brightness          = 0x21c;
    constexpr uintptr_t LightEmission       = 0x238;
    constexpr uintptr_t LightInfluence      = 0x23c;
    constexpr uintptr_t Texture             = 0x1c0;
    constexpr uintptr_t ZOffset             = 0x264;
    constexpr uintptr_t Lifetime            = 0x1f4;
    constexpr uintptr_t Rate                = 0x248;
    constexpr uintptr_t Rotation            = 0x204;
    constexpr uintptr_t RotSpeed            = 0x1fc;
    constexpr uintptr_t Speed               = 0x20c;
    constexpr uintptr_t SpreadAngle         = 0x214;
    constexpr uintptr_t Acceleration        = 0x1e0;
    constexpr uintptr_t Drag                = 0x220;
    constexpr uintptr_t TimeScale           = 0x25c;
    constexpr uintptr_t VelocityInheritance = 0x260;
}
namespace Beam {
    constexpr uintptr_t Brightness    = 0x180;
    constexpr uintptr_t LightEmission = 0x18c;
    constexpr uintptr_t LightInfluence= 0x190;
    constexpr uintptr_t Texture       = 0x140;
    constexpr uintptr_t TextureLength = 0x19c;
    constexpr uintptr_t TextureSpeed  = 0x1a4;
    constexpr uintptr_t ZOffset       = 0x1b0;
    constexpr uintptr_t Attachment0   = 0x160;
    constexpr uintptr_t Attachment1   = 0x170;
    constexpr uintptr_t CurveSize0    = 0x184;
    constexpr uintptr_t CurveSize1    = 0x188;
    constexpr uintptr_t Width0        = 0x1a8;
    constexpr uintptr_t Width1        = 0x1ac;
}

// ── Terrain ──────────────────────────────────────────────────────────────────
namespace Terrain {
    constexpr uintptr_t GrassLength      = 0x188;
    constexpr uintptr_t WaterReflectance = 0x190;
    constexpr uintptr_t WaterTransparency= 0x194;
    constexpr uintptr_t WaterWaveSize    = 0x198;
    constexpr uintptr_t WaterWaveSpeed   = 0x19c;
    constexpr uintptr_t WaterColor       = 0x178;
    constexpr uintptr_t MaterialColors   = 0x438;
}
namespace MaterialColors {
    constexpr uintptr_t Asphalt    = 0x30;
    constexpr uintptr_t Basalt     = 0x27;
    constexpr uintptr_t Brick      = 0xf;
    constexpr uintptr_t Cobblestone= 0x33;
    constexpr uintptr_t Concrete   = 0xc;
    constexpr uintptr_t CrackedLava= 0x2d;
    constexpr uintptr_t Glacier    = 0x1b;
    constexpr uintptr_t Grass      = 0x6;
    constexpr uintptr_t Ground     = 0x2a;
    constexpr uintptr_t Ice        = 0x36;
    constexpr uintptr_t LeafyGrass = 0x39;
    constexpr uintptr_t Limestone  = 0x3f;
    constexpr uintptr_t Mud        = 0x24;
    constexpr uintptr_t Pavement   = 0x42;
    constexpr uintptr_t Rock       = 0x18;
    constexpr uintptr_t Salt       = 0x3c;
    constexpr uintptr_t Sand       = 0x12;
    constexpr uintptr_t Sandstone  = 0x21;
    constexpr uintptr_t Slate      = 0x9;
    constexpr uintptr_t Snow       = 0x1e;
    constexpr uintptr_t WoodPlanks = 0x15;
}

// ── Lighting / Sky / Atmosphere ──────────────────────────────────────────────
namespace Lighting {
    constexpr uintptr_t ClockTime               = 0x1a8;
    constexpr uintptr_t Brightness              = 0x110;
    constexpr uintptr_t EnvironmentDiffuseScale = 0x114;
    constexpr uintptr_t EnvironmentSpecularScale= 0x118;
    constexpr uintptr_t FogStart                = 0x128;
    constexpr uintptr_t FogEnd                  = 0x124;
    constexpr uintptr_t FogColor                = 0xec;
    constexpr uintptr_t Ambient                 = 0xc8;
    constexpr uintptr_t OutdoorAmbient          = 0xf8;
    constexpr uintptr_t ColorShift_Top          = 0xd4;
    constexpr uintptr_t ColorShift_Bottom       = 0xe0;
    constexpr uintptr_t ExposureCompensation    = 0x11c;
    constexpr uintptr_t GeographicLatitude      = 0x180;
    constexpr uintptr_t LightColor              = 0x14c;
    constexpr uintptr_t GradientTop             = 0x140;
    constexpr uintptr_t LightDirection          = 0x158;
    constexpr uintptr_t GradientBottom          = 0x184;
    constexpr uintptr_t GlobalShadows           = 0x138;
    constexpr uintptr_t MoonPosition            = 0x174;
    constexpr uintptr_t SunPosition             = 0x168;
    constexpr uintptr_t Source                  = 0x164;
    constexpr uintptr_t Sky                     = 0x1c8;
}
namespace Sky {
    constexpr uintptr_t SkyboxBk        = 0xf8;
    constexpr uintptr_t SkyboxDn        = 0x128;
    constexpr uintptr_t SkyboxFt        = 0x158;
    constexpr uintptr_t SkyboxLf        = 0x188;
    constexpr uintptr_t SkyboxRt        = 0x1b8;
    constexpr uintptr_t SkyboxUp        = 0x1e8;
    constexpr uintptr_t SunAngularSize  = 0x23c;
    constexpr uintptr_t MoonAngularSize = 0x244;
    constexpr uintptr_t SunTextureId    = 0x218;
    constexpr uintptr_t MoonTextureId   = 0xc8;
    constexpr uintptr_t SkyboxOrientation= 0x238;
    constexpr uintptr_t StarCount       = 0x248;
}
namespace Atmosphere {
    constexpr uintptr_t Density = 0xd0;
    constexpr uintptr_t Offset  = 0xdc;
    constexpr uintptr_t Color   = 0xb8;
    constexpr uintptr_t Decay   = 0xc4;
    constexpr uintptr_t Glare   = 0xd4;
    constexpr uintptr_t Haze    = 0xd8;
}

// ── Post-processing effects ───────────────────────────────────────────────────
namespace BloomEffect {
    constexpr uintptr_t Intensity = 0xb8;
    constexpr uintptr_t Size      = 0xbc;
    constexpr uintptr_t Threshold = 0xc0;
    constexpr uintptr_t Enabled   = 0xb0;
}
namespace DepthOfFieldEffect {
    constexpr uintptr_t FocusDistance = 0xbc;
    constexpr uintptr_t FarIntensity  = 0xb8;
    constexpr uintptr_t NearIntensity = 0xc4;
    constexpr uintptr_t InFocusRadius = 0xc0;
    constexpr uintptr_t Enabled       = 0xb0;
}
namespace SunRaysEffect {
    constexpr uintptr_t Intensity = 0xb8;
    constexpr uintptr_t Spread    = 0xbc;
    constexpr uintptr_t Enabled   = 0xb0;
}
namespace ColorCorrectionEffect {
    constexpr uintptr_t Brightness = 0xc4;
    constexpr uintptr_t Contrast   = 0xc8;
    constexpr uintptr_t TintColor  = 0xb8;
    constexpr uintptr_t Enabled    = 0xb0;
}
namespace ColorGradingEffect {
    constexpr uintptr_t TonemapperPreset = 0xb8;
    constexpr uintptr_t Enabled          = 0xb0;
}
namespace BlurEffect {
    constexpr uintptr_t Size    = 0xb8;
    constexpr uintptr_t Enabled = 0xb0;
}

// ── GUI ──────────────────────────────────────────────────────────────────────
namespace GuiObject {
    constexpr uintptr_t ScreenGui_Enabled    = 0x4c4;
    constexpr uintptr_t Position             = 0x510;
    constexpr uintptr_t Size                 = 0x530;
    constexpr uintptr_t Visible              = 0x5ad;
    constexpr uintptr_t Image                = 0x988;
    constexpr uintptr_t Text                 = 0xda0;
    constexpr uintptr_t RichText             = 0xb50;
    constexpr uintptr_t BackgroundColor3     = 0x540;
    constexpr uintptr_t BorderColor3         = 0x54c;
    constexpr uintptr_t TextColor3           = 0xe50;
    constexpr uintptr_t LayoutOrder          = 0x580;
    constexpr uintptr_t ZIndex               = 0x18b;
    constexpr uintptr_t BackgroundTransparency= 0x54c;
    constexpr uintptr_t Rotation             = 0x178;
}
namespace GuiBase2D {
    constexpr uintptr_t AbsoluteSize     = 0x100;
    constexpr uintptr_t AbsolutePosition = 0xf8;
    constexpr uintptr_t AbsoluteRotation = 0x178;
}

// ── UserInputService ──────────────────────────────────────────────────────────
namespace UserInputService {
    constexpr uintptr_t WindowInputState = 0x2c0;
}
namespace WindowInputState {
    constexpr uintptr_t CurrentTextBox = 0x48;
    constexpr uintptr_t CapsLock       = 0x40;
}

// ── Textures ──────────────────────────────────────────────────────────────────
namespace Textures {
    constexpr uintptr_t Decal_Texture   = 0x180;
    constexpr uintptr_t Texture_Texture = 0x180;
}

// ── Detectors / Prompts ───────────────────────────────────────────────────────
namespace ProximityPrompt {
    constexpr uintptr_t ActionText            = 0xb0;
    constexpr uintptr_t ObjectText            = 0xd0;
    constexpr uintptr_t HoldDuration          = 0x120;
    constexpr uintptr_t MaxActivationDistance = 0x128;
    constexpr uintptr_t KeyCode               = 0x124;
    constexpr uintptr_t GamepadKeyCode        = 0x11c;
    constexpr uintptr_t Enabled               = 0x136;
    constexpr uintptr_t RequiresLineOfSight   = 0x137;
}
namespace ClickDetector {
    constexpr uintptr_t MaxActivationDistance = 0xe8;
    constexpr uintptr_t MouseIcon             = 0xc8;
}
namespace DragDetector {
    constexpr uintptr_t ReferenceInstance      = 0x1f0;
    constexpr uintptr_t MaxActivationDistance  = 0xe8;
    constexpr uintptr_t MaxDragAngle           = 0x2a8;
    constexpr uintptr_t MaxDragTranslation     = 0x26c;
    constexpr uintptr_t MinDragAngle           = 0x2b4;
    constexpr uintptr_t MinDragTranslation     = 0x278;
    constexpr uintptr_t ActivatedCursorIcon    = 0x1c0;
    constexpr uintptr_t CursorIcon             = 0xc8;
    constexpr uintptr_t MaxForce              = 0x2ac;
    constexpr uintptr_t MaxTorque             = 0x2b0;
    constexpr uintptr_t Responsiveness        = 0x2c0;
}

// ── Animation ────────────────────────────────────────────────────────────────
namespace AnimationTrack {
    constexpr uintptr_t Animation     = 0xb8;
    constexpr uintptr_t Animator      = 0x108;
    constexpr uintptr_t Speed         = 0xd4;
    constexpr uintptr_t TimePosition  = 0xd8;
    constexpr uintptr_t Looped        = 0xe5;
    constexpr uintptr_t IsPlaying     = 0xa90;
}
namespace Animator {
    constexpr uintptr_t ActiveAnimations = 0xa20;
}

// ── Scripts ───────────────────────────────────────────────────────────────────
namespace LocalScript {
    constexpr uintptr_t GUID     = 0xd0;
    constexpr uintptr_t Hash     = 0x1a0;
    constexpr uintptr_t ByteCode = 0x190;
}
namespace Script {
    constexpr uintptr_t GUID     = 0xd0;
    constexpr uintptr_t Hash     = 0x1a0;
    constexpr uintptr_t ByteCode = 0x190;
}
namespace ByteCode {
    constexpr uintptr_t Size    = 0x20;
    constexpr uintptr_t Pointer = 0x10;
}

} // namespace offsets
