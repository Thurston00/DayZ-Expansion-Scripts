class CfgPatches
{
	class DayZExpansion_Crossbow
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms"
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
	class ExpansionCrossbow_Base: Rifle_Base
	{
		scope=0;
		rotationFlags=3;
		displayName="$STR_EXPANSION_CROSSBOW";
		descriptionShort="USE THE VANILLA CROSSBOW - THIS ITEM WILL BE REMOVED IN A FUTUR EXPANSION UPDATE";
		model="\DZ\weapons\archery\crossbow\crossbow.p3d";
		attachments[]=
		{
			"weaponOpticsCrossbow"
		};
		itemSize[]={9,3};
		weight=1500;
		absorbency=0;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		PPDOFProperties[]={1,0.5,10,210,4,10};
		ironsightsExcludingOptics[]=
		{
			"HuntingOptic"
		};
		DisplayMagazine=0;
		WeaponLength=0.98105502;
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Expansion_Ammo_ArrowBolt"
		};
		magazines[]={};
		ejectType=3;
		recoilModifier[]={1,1,1};
		swayModifier[]={2,2,1};
		reloadAction="ReloadIZH18";
		shotAction="";
		hiddenSelections[]=
		{
			"camo"
		};
		modes[]=
		{
			"Single"
		};
		class Single: Mode_Single
		{
			soundSetShot[]={};
			reloadTime=1;
			recoil="recoil_izh18";
			recoilProne="recoil_izh18_prone";
			dispersion=0.001;
			magazineSlot="magazine";
		};
		class NoiseShoot
		{
			strength=0;
			type="shot";
		};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlash
				{
					overrideParticle="weapon_shot_vss_01";
				};
			};
		};
		class OpticsInfo: OpticsInfoRifle
		{
			memoryPointCamera="eye";
			discreteDistance[]={25};
			discreteDistanceInitIndex=0;
			modelOptics="-";
			distanceZoomMin=25;
			distanceZoomMax=25;
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=200;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\weapons\firearms\Izh18\data\Izh18.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DZ\weapons\firearms\Izh18\data\Izh18.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\firearms\Izh18\data\Izh18_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DZ\weapons\firearms\Izh18\data\Izh18_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\firearms\Izh18\data\Izh18_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
	class Expansion_Crossbow: ExpansionCrossbow_Base
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"dz\weapons\archery\crossbow\data\crossbow_leaves_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dz\weapons\archery\crossbow\data\crossbow.rvmat"
		};
	};
};
class cfgMagazines
{
	class Ammunition_Base;
	class Expansion_Ammo_ArrowBolt: Ammunition_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_AMMO_ARROWBOLT";
		descriptionShort="$STR_EXPANSION_AMMO_ARROWBOLT_DESC";
		model="\dz\weapons\projectiles\bolt_biggame.p3d";
		rotationFlags=34;
		weight=24;
		itemSize[]={2,1};
		count=5;
		ammo="Expansion_Arrow_Bolt";
		emptySound="empty_arrows";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=100;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\weapons\projectiles\data\arrows.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DZ\weapons\projectiles\data\arrows.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\projectiles\data\arrows_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DZ\weapons\projectiles\data\arrows_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\weapons\projectiles\data\arrows_destruct.rvmat"
							}
						}
					};
				};
			};
		};
	};
};
class cfgAmmoTypes
{
	class Expansion_Arrow_Bolt
	{
		name="Expansion_Arrow_Bolt";
	};
};
class CfgAmmo
{
	class Bullet_Base;
	class Expansion_Arrow_Bolt: Bullet_Base
	{
		scope=2;
		model="\dz\weapons\projectiles\bolt_flying.p3d";
		spawnPileType="AmmoExpansion_ArrowBolt";
		proxyShape="\dz\weapons\projectiles\bolt_biggame.p3d";
		hit=12;
		indirectHit=0;
		indirectHitRange=0;
		casing="";
		round="FxRound_Expansion_arrow_bolt";
		deflecting=20;
		caliber=0.0099999998;
		airFriction=-0.0021500001;
		typicalSpeed=90;
		soundFly[]={};
		supersonicCrackNear[]={};
		supersonicCrackFar[]={};
		initSpeed=90;
		weight=0.025;
		tracerScale=1;
		tracerStartTime=9.9999997e-06;
		tracerEndTime=-1;
		class DamageApplied
		{
			type="Projectile";
			dispersion=0;
			bleedThreshold=1;
			defaultDamageOverride[]=
			{
				{0.5,1}
			};
			class Health
			{
				damage=60;
			};
			class Blood
			{
				damage=500;
			};
			class Shock
			{
				damage=0;
			};
		};
	};
};
class CfgVehicles
{
	class FxRound;
	class FxRound_Expansion_arrow_bolt: FxRound
	{
		model="\dz\weapons\projectiles\bolt_biggame.p3d";
	};
};
