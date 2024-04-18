#define _ARMA_

class CfgPatches
{
	class DayZExpansion_Objects_Firearms_RPG7
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Weapons_Firearms"};
	};
};
class CfgAmmoTypes
{
	class AType_ExpansionRocketRPG
	{
		name = "ExpansionRocketRPG";
	};
};
class CfgAmmo
{
	class Bullet_Base;
	class ExpansionRocketRPG: Bullet_Base
	{
		scope = 2;
		casing = "FxCartridge_Expansion_RPG7";
		round = "FxRound_Expansion_RPG7";
		spawnPileType = "ExpansionAmmoRPG";
		hit = 0;
		model = "\dz\weapons\ammunition\rocket_rpg7_inflight.p3d";
		indirectHit = 0;
		indirectHitRange = 0;
		tracerScale = 2;
		caliber = 0.01;
		deflecting = 5;
		impactBehaviour = 1;
		initSpeed = 300;
		typicalSpeed = 300;
		timeToLive = 30;
		airFriction = -0.00315;
		weight = 0.26;
		supersonicCrackNear[] = {};
		supersonicCrackFar[] = {};
		class DamageApplied
		{
			type = "Projectile";
			dispersion = 0;
			bleedThreshold = 1;
			defaultDamageOverride[] = {{0.5,1}};
			class Health
			{
				damage = 5;
			};
			class Blood
			{
				damage = 0;
			};
			class Shock
			{
				damage = 15;
			};
		};
		class NoiseHit
		{
			strength = 15;
			type = "shot";
		};
	};
	class DefaultAmmo;
	class ExpansionRocket_Ammo: DefaultAmmo
	{
		indirectHit = 1;
		indirectHitRange = 3;
		explosive = 1;
		typicalSpeed = 3;
		initSpeed = 3;
		simulation = "shotShell";
		simulationStep = 0.05;
		soundSetExplosion[] = {"Grenade_explosion_SoundSet","Grenade_Tail_SoundSet"};
		class DamageApplied
		{
			type = "FragGrenade";
			bleedThreshold = 0.4;
			class Health
			{
				damage = 300;
			};
			class Blood
			{
				damage = 0;
			};
			class Shock
			{
				damage = 0;
			};
		};
		class NoiseExplosion
		{
			strength = 100;
			type = "shot";
		};
	};
};
class CfgMagazines
{
	class Ammunition_Base;
	class ExpansionAmmoRPG: Ammunition_Base
	{
		scope = 2;
		itemSize[] = {4,2};
		displayName = "$STR_EXPANSION_RPG7_WARHEAD";
		descriptionShort = "$STR_EXPANSION_RPG7_WARHEAD_DESC";
		model = "DayZExpansion\Objects\Weapons\Firearms\RPG7\fx\rpg7_ammoheat_round.p3d";
		rotationFlags = 34;
		weight = 1800;
		count = 1;
		ammo = "ExpansionRocketRPG";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 100;
					healthLevels[] = {{1.0,{"DZ\weapons\launchers\law\data\law_rocket.rvmat"}},{0.7,{"DZ\weapons\launchers\law\data\law_rocket.rvmat"}},{0.5,{"DZ\weapons\launchers\law\data\law_rocket_damage.rvmat"}},{0.3,{"DZ\weapons\launchers\law\data\law_rocket_damage.rvmat"}},{0.0,{"DZ\weapons\launchers\law\data\law_rocket_destruct.rvmat"}}};
				};
			};
		};
	};
};
class Mode_Safe;
class Mode_Single;
class Mode_Burst;
class Mode_FullAuto;
class OpticsInfoRifle;
class cfgWeapons
{
	class Rifle_Base;
	class ExpansionRPG7Base: Rifle_Base
	{
		scope = 0;
		weight = 3200;
		absorbency = 0;
		repairableWithKits[] = {5,1};
		repairCosts[] = {30,25};
		PPDOFProperties[] = {0,0.0,0,0,0,0};
		ironsightsExcludingOptics[] = {};
		DisplayMagazine = 0;
		WeaponLength = 0.981055;
		chamberSize = 1;
		chamberedRound = "";
		chamberableFrom[] = {"ExpansionAmmoRPG"};
		magazines[] = {};
		ejectType = 3;
		recoilModifier[] = {1,1,1};
		swayModifier[] = {2,2,1};
		reloadAction = "ReloadIZH18";
		shotAction = "";
		hiddenSelections[] = {};
		modes[] = {"Single"};
		class Single: Mode_Single
		{
			soundSetShot[] = {"Expansion_RPG_Shot_SoundSet"};
			reloadTime = 1;
			recoil = "recoil_izh18";
			recoilProne = "recoil_izh18_prone";
			dispersion = 0.001;
			magazineSlot = "magazine";
		};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlash{};
				class SmokeCloud1
				{
					overrideParticle = "weapon_shot_winded_smoke";
				};
				class MuzzleFlash2
				{
					overrideParticle = "Expansion_World_Dust";
					ignoreIfSuppressed = 1;
					illuminateWorld = 1;
				};
				class MuzzleFlashStar
				{
					overrideParticle = "weapon_shot_Flame_3D_4star";
					overridePoint = "StarFlash";
				};
				class GasPistonBurstR
				{
					overrideParticle = "weapon_shot_chamber_smoke";
					overridePoint = "GasPiston";
					overrideDirectionVector[] = {0,0,0};
				};
				class GasPistonBurstL
				{
					overrideParticle = "weapon_shot_chamber_smoke";
					overridePoint = "GasPiston";
					overrideDirectionVector[] = {180,0,0};
				};
				class GasPistonSmokeRaiseR
				{
					overrideParticle = "weapon_shot_chamber_smoke_raise";
					overridePoint = "GasPiston";
					overrideDirectionVector[] = {0,0,0};
				};
				class GasPistonSmokeRaiseL
				{
					overrideParticle = "weapon_shot_chamber_smoke_raise";
					overridePoint = "GasPiston";
					overrideDirectionVector[] = {180,0,0};
				};
			};
			class OnOverheating
			{
				shotsToStartOverheating = 3;
				maxOverheatingValue = 30;
				overheatingDecayInterval = 2;
				class SmokingBarrel1
				{
					overridePoint = "GasPiston";
					positionOffset[] = {0.05,-0.02,0};
					overrideParticle = "smoking_barrel_small";
					onlyWithinOverheatLimits[] = {0.0,0.2};
					onlyWithinRainLimits[] = {0,0.2};
				};
				class SmokingBarrelHot1
				{
					overridePoint = "GasPiston";
					positionOffset[] = {0.12,-0.02,0};
					overrideParticle = "smoking_barrel";
					onlyWithinOverheatLimits[] = {0.2,0.6};
					onlyWithinRainLimits[] = {0,0.2};
				};
				class SmokingBarrelHot3
				{
					overridePoint = "GasPiston";
					positionOffset[] = {0.21,-0.02,0};
					overrideParticle = "smoking_barrel_heavy";
					onlyWithinOverheatLimits[] = {0.6,1};
					onlyWithinRainLimits[] = {0,0.2};
				};
				class Steam
				{
					overrideParticle = "smoking_barrel_steam_small";
					positionOffset[] = {0.4,0.02,0};
					onlyWithinOverheatLimits[] = {0,0.5};
					onlyWithinRainLimits[] = {0.2,1};
				};
				class Steam2
				{
					overrideParticle = "smoking_barrel_steam";
					positionOffset[] = {0.45,0.02,0};
					onlyWithinOverheatLimits[] = {0.5,1};
					onlyWithinRainLimits[] = {0.2,1};
				};
				class ChamberSmokeRaise
				{
					overrideParticle = "smoking_barrel_small";
					overridePoint = "Nabojnicestart";
					onlyWithinOverheatLimits[] = {0.5,1};
				};
			};
		};
		class OpticsInfo: OpticsInfoRifle
		{
			memoryPointCamera = "eye";
			discreteDistance[] = {50,100,200,300,400,500,600,700,800,900,1000};
			discreteDistanceInitIndex = 0;
			modelOptics = "-";
			distanceZoomMin = 50;
			distanceZoomMax = 50;
		};
	};
	class ExpansionRPG7: ExpansionRPG7Base
	{
		scope = 2;
		displayName = "$STR_EXPANSION_RPG7";
		descriptionShort = "$STR_cfgWeapons_RPG71";
		model = "\DayZExpansion\Objects\Weapons\Firearms\RPG7\rpg.p3d";
		itemSize[] = {8,6};
		attachments[] = {};
		hiddenSelectionsTextures[] = {};
		hiddenSelectionsMaterials[] = {};
		simpleHiddenSelections[] = {"ammo"};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlash
				{
					overrideParticle = "Expansion_World_Dust";
					ignoreIfSuppressed = 1;
				};
			};
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 200;
					healthLevels[] = {{1.0,{"DZ\weapons\firearms\Izh18\data\Izh18.rvmat"}},{0.7,{"DZ\weapons\firearms\Izh18\data\Izh18.rvmat"}},{0.5,{"DZ\weapons\firearms\Izh18\data\Izh18_damage.rvmat"}},{0.3,{"DZ\weapons\firearms\Izh18\data\Izh18_damage.rvmat"}},{0.0,{"DZ\weapons\firearms\Izh18\data\Izh18_destruct.rvmat"}}};
				};
			};
		};
	};
};
class cfgVehicles
{
	class FxCartridge;
	class FxRound;
	class HouseNoDestruct;
	class Expansion_RPG_Explosion: HouseNoDestruct
	{
		scope = 2;
		model = "\DZ\data\lightpoint.p3d";
		ammoType = "ExpansionRocket_Ammo";
	};
	class FxCartridge_Expansion_RPG7: FxCartridge
	{
		model = "DayZExpansion\Objects\Weapons\Firearms\RPG7\fx\rpg7_ammoheat_casing.p3d";
	};
	class FxRound_Expansion_RPG7: FxRound
	{
		model = "DayZExpansion\Objects\Weapons\Firearms\RPG7\fx\rpg7_ammoheat_fxround.p3d";
	};
};
class CfgNonAIVehicles
{
	class ProxyWeapon;
	class ProxyRocket_rpg7: ProxyWeapon
	{
		scope = 2;
		simulation = "magazine";
		model = "DayZExpansion\Objects\Weapons\Firearms\RPG7\fx\rpg7_ammoheat_round.p3d";
	};
};
