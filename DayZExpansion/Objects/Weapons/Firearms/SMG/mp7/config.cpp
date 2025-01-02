class CfgPatches
{
	class DayZExpansion_Weapons_SMG_MP7
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Firearms_cz61"
		};
	};
};
class Mode_Safe;
class Mode_SemiAuto;
class Mode_Burst;
class Mode_FullAuto;
class CfgWeapons
{
	class OpticsInfoRifle;
	class CZ61;
	class Expansion_MP7_Base: CZ61
	{
		scope=0;
		weight=1900;
		absorbency=0.1;
		repairableWithKits[]={5,1};
		repairCosts[]={30,25};
		discreteDistance[]={75,150};
		discreteDistanceInitIndex=0;
		modelOptics="-";
		distanceZoomMin=100;
		distanceZoomMax=100;
		PPDOFProperties[]={1,0.5,10,190,4,10};
		optics=1;
		value=0;
		ironsightsExcludingOptics[]=
		{
			"M4_CarryHandleOptic",
			"BUISOptic",
			"M68Optic",
			"M4_T3NRDSOptic",
			"ReflexOptic",
			"ACOGOptic"
		};
		chamberSize=1;
		chamberedRound="";
		chamberableFrom[]=
		{
			"Ammo_Expansion_46x30",
			"TTC_Ammo_4630",
			"SNAFU_Ammo_46x30"
		};
		magazines[]=
		{
			"Mag_Expansion_MP7_40Rnd"
		};
		magazineSwitchTime=0.30000001;
		WeaponLength=0.66352999;
		barrelArmor=1.35;
		ejectType=1;
		recoilModifier[]={1,1,1};
		reloadAction="ReloadCZ61";
		hiddenSelections[]=
		{
			"camo"
		};
		modes[]=
		{
			"SemiAuto",
			"FullAuto"
		};
		class SemiAuto: Mode_SemiAuto
		{
			soundSetShot[]=
			{
				"CZ75_Shot_SoundSet",
				"CZ75_Tail_SoundSet",
				"CZ75_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"CZ61_silencer_SoundSet",
					"CZ61_silencerTail_SoundSet",
					"CZ61_silencerInteriorTail_SoundSet"
				},
				
				{
					"CZ61_silencerHomeMade_SoundSet",
					"CZ61_silencerHomeMadeTail_SoundSet",
					"CZ61_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			reloadTime=0.055;
			recoil="recoil_cz61";
			recoilProne="recoil_cz61_prone";
			dispersion=0.003;
			magazineSlot="magazine";
		};
		class FullAuto: Mode_FullAuto
		{
			soundSetShot[]=
			{
				"CZ75_Shot_SoundSet",
				"CZ75_Tail_SoundSet",
				"CZ75_InteriorTail_SoundSet"
			};
			soundSetShotExt[]=
			{
				
				{
					"CZ61_silencer_SoundSet",
					"CZ61_silencerTail_SoundSet",
					"CZ61_silencerInteriorTail_SoundSet"
				},
				
				{
					"CZ61_silencerHomeMade_SoundSet",
					"CZ61_silencerHomeMadeTail_SoundSet",
					"CZ61_silencerInteriorHomeMadeTail_SoundSet"
				}
			};
			reloadTime=0.059999999;
			recoil="recoil_cz61";
			recoilProne="recoil_cz61_prone";
			dispersion=0.003;
			magazineSlot="magazine";
		};
		class Particles
		{
			class OnFire
			{
				class MuzzleFlash
				{
					overrideParticle="weapon_shot_cz61_01";
					ignoreIfSuppressed=1;
					illuminateWorld=1;
				};
				class SmokeCloud
				{
					overrideParticle="weapon_shot_winded_smoke_small";
				};
			};
			class OnOverheating
			{
				shotsToStartOverheating=3;
				maxOverheatingValue=10;
				overheatingDecayInterval=1;
				class SmokingBarrel
				{
					overrideParticle="smoking_barrel_small";
					onlyWithinOverheatLimits[]={0,0.5};
					onlyWithinRainLimits[]={0,0.2};
					ignoreIfSuppressed=1;
				};
				class SmokingBarrelHotSteam
				{
					overrideParticle="smoking_barrel_steam_small";
					positionOffset[]={0.1,0.02,0};
					onlyWithinOverheatLimits[]={0,1};
					onlyWithinRainLimits[]={0.2,1};
				};
				class OpenChamberSmoke
				{
					onlyIfBoltIsOpen=1;
					overrideParticle="smoking_barrel_small";
					overridePoint="Nabojnicestart";
				};
			};
			class OnBulletCasingEject
			{
				class ChamberSmokeRaise
				{
					overrideParticle="weapon_shot_chamber_smoke_raise";
					overridePoint="Nabojnicestart";
				};
			};
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class walkErc_L
				{
					soundSet="Weapon_Movement_Rifle_walkErc_L_SoundSet";
					id=1;
				};
				class walkErc_R
				{
					soundSet="Weapon_Movement_Rifle_walkErc_R_SoundSet";
					id=2;
				};
				class runErc_L
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=3;
				};
				class runErc_R
				{
					soundSet="Weapon_Movement_Rifle_runErc_R_SoundSet";
					id=4;
				};
				class sprintErc_L
				{
					soundSet="Weapon_Movement_Rifle_sprintErc_SoundSet";
					id=5;
				};
				class sprintErc_R
				{
					soundSet="Weapon_Movement_Rifle_sprintErc_SoundSet";
					id=6;
				};
				class walkCro_L
				{
					soundSet="Weapon_Movement_Rifle_walkCro_SoundSet";
					id=21;
				};
				class walkCro_R
				{
					soundSet="Weapon_Movement_Rifle_walkCro_SoundSet";
					id=22;
				};
				class runCro_L
				{
					soundSet="Weapon_Movement_Rifle_runCro_SoundSet";
					id=23;
				};
				class runCro_R
				{
					soundSet="Weapon_Movement_Rifle_runCro_SoundSet";
					id=24;
				};
				class walkProne_L
				{
					soundSet="Weapon_Movement_Rifle_walkCro_SoundSet";
					id=31;
				};
				class walkProne_R
				{
					soundSet="Weapon_Movement_Rifle_walkCro_SoundSet";
					id=32;
				};
				class runProne_L
				{
					soundSet="Weapon_Movement_Rifle_runCro_SoundSet";
					id=33;
				};
				class runProne_R
				{
					soundSet="Weapon_Movement_Rifle_runCro_SoundSet";
					id=34;
				};
				class jumpErc_L
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=35;
				};
				class jumpErc_R
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=36;
				};
				class landFootErc_L
				{
					soundSet="Weapon_Movement_Rifle_landFootErc_SoundSet";
					id=37;
				};
				class landFootErc_R
				{
					soundSet="Weapon_Movement_Rifle_landFootErc_SoundSet";
					id=38;
				};
				class walkRasErc_L
				{
					soundSet="Weapon_Movement_Rifle_walkRasErc_SoundSet";
					id=51;
				};
				class walkRasErc_R
				{
					soundSet="Weapon_Movement_Rifle_walkRasErc_SoundSet";
					id=52;
				};
				class runRasErc_L
				{
					soundSet="Weapon_Movement_Rifle_runRasErc_SoundSet";
					id=53;
				};
				class runRasErc_R
				{
					soundSet="Weapon_Movement_Rifle_runRasErc_SoundSet";
					id=54;
				};
				class HandStep_L
				{
					soundSet="Weapon_Movement_Rifle_walkErc_L_SoundSet";
					id=61;
				};
				class HandStep_R
				{
					soundSet="Weapon_Movement_Rifle_walkErc_L_SoundSet";
					id=62;
				};
				class HandStep_Hard_L
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=63;
				};
				class HandStep_Hard_R
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=64;
				};
				class landFeetErc
				{
					soundSet="Weapon_Movement_Rifle_landFeetErc_SoundSet";
					id=100;
				};
				class Weapon_Movement_Rifle_Walk
				{
					soundSet="Weapon_Movement_Rifle_walkErc_L_SoundSet";
					id=101;
				};
				class Weapon_Movement_Rifle_Run
				{
					soundSet="Weapon_Movement_Rifle_runErc_L_SoundSet";
					id=102;
				};
				class Weapon_Movement_Rifle_Sprint
				{
					soundSet="Weapon_Movement_Rifle_sprintErc_L_SoundSet";
					id=103;
				};
				class Weapon_Movement_Rifle_Land
				{
					soundSet="Weapon_Movement_Rifle_landFeetErc_SoundSet";
					id=104;
				};
				class Char_Gestures_Hand_Grab_Rifle
				{
					soundSet="Char_Gestures_Hand_Grab_FabricRifle_SoundSet";
					id=892;
				};
			};
		};
	};
	class Expansion_MP7: Expansion_MP7_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_MP7";
		descriptionShort="$STR_EXPANSION_MP7_DESC";
		model="\DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\MP7.p3d";
		attachments[]=
		{
			"pistolMuzzle",
			"weaponFlashlight",
			"weaponOptics"
		};
		itemSize[]={4,3};
		dexterity=5.3000002;
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"dayzexpansion\objects\weapons\firearms\smg\mp7\data\mp7_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"dayzexpansion\objects\weapons\firearms\smg\mp7\data\mp7_body.rvmat"
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=175;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\data\mp7_body.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\data\mp7_body.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\data\mp7_body_damaged.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\data\mp7_body_damaged.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\data\mp7_body_ruined.rvmat"
							}
						}
					};
				};
			};
		};
	};
};
class cfgMagazines
{
	class Magazine_Base;
	class Mag_Expansion_MP7_40Rnd: Magazine_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_MP7_MAG";
		descriptionShort="$STR_EXPANSION_MP7_MAG_DESC";
		model="DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\mp7_mag.p3d";
		weight=170;
		itemSize[]={1,3};
		count=40;
		ammo="Bullet_Expansion_46x30";
		ammoItems[]=
		{
			"Ammo_Expansion_46x30",
			"TTC_Ammo_4630",
			"SNAFU_Ammo_46x30"
		};
		tracersEvery=0;
		mass=10;
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
								"DZ\weapons\attachments\data\mp5k_mag.rvmat"
							}
						},
						
						{
							0.69999999,
							{}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\attachments\data\mp5k_mag_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							{}
						},
						
						{
							0,
							
							{
								"DZ\weapons\attachments\data\mp5k_mag_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class MagRifle_fill_in
				{
					soundSet="MagRifle_fill_in_SoundSet";
					id=1;
				};
				class MagRifle_fill_loop
				{
					soundSet="MagRifle_fill_loop_SoundSet";
					id=2;
				};
				class MagRifle_fill_out
				{
					soundSet="MagRifle_fill_out_SoundSet";
					id=3;
				};
				class MagRifle_empty_in
				{
					soundSet="MagRifle_empty_in_SoundSet";
					id=4;
				};
				class MagRifle_empty_loop
				{
					soundSet="MagRifle_empty_loop_SoundSet";
					id=5;
				};
				class MagRifle_empty_out
				{
					soundSet="MagRifle_empty_out_SoundSet";
					id=6;
				};
				class MagPistol_fill_in
				{
					soundSet="MagPistol_fill_in_SoundSet";
					id=7;
				};
				class MagPistol_fill_loop
				{
					soundSet="MagPistol_fill_loop_SoundSet";
					id=8;
				};
				class MagPistol_fill_out
				{
					soundSet="MagPistol_fill_out_SoundSet";
					id=9;
				};
				class MagPistol_empty_in
				{
					soundSet="MagPistol_empty_in_SoundSet";
					id=10;
				};
				class MagPistol_empty_loop
				{
					soundSet="MagPistol_empty_loop_SoundSet";
					id=11;
				};
				class MagPistol_empty_out
				{
					soundSet="MagPistol_empty_out_SoundSet";
					id=12;
				};
			};
		};
	};
	class Ammunition_Base;
	class Ammo_Expansion_46x30: Ammunition_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_AMMO_46x30";
		descriptionShort="$STR_EXPANSION_AMMO_46x30_DESC";
		model="DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\46x30_looserounds.p3d";
		weight=12;
		count=25;
		ammo="Bullet_Expansion_46x30";
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
								"DZ\weapons\ammunition\data\45cal_loose.rvmat"
							}
						},
						
						{
							0.69999999,
							{}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\ammunition\data\45cal_loose_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							{}
						},
						
						{
							0,
							
							{
								"DZ\weapons\ammunition\data\45cal_loose_destruct.rvmat"
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
	class AType_Bullet_Expansion_46x30
	{
		name="Bullet_Expansion_46x30";
	};
};
class CfgAmmo
{
	class Bullet_Base;
	class Bullet_Expansion_46x30: Bullet_Base
	{
		scope=2;
		lootCategory="Crafted";
		cartridge="FxCartridge_9mm";
		spawnPileType="Ammo_Expansion_46x30";
		muzzleFlashParticle="weapon_shot_mp5k_02_boris";
		hit=11;
		indirectHit=0;
		indirectHitRange=0;
		tracerScale=1;
		caliber=0.65000004;
		deflecting=45;
		visibleFire=5;
		audibleFire=9;
		typicalSpeed=735;
		airFriction=-0.00119;
		supersonicCrackNear[]={};
		supersonicCrackFar[]={};
		initSpeed=735;
		weight=0.0149;
		class DamageApplied
		{
			type="Projectile";
			dispersion=0;
			bleedThreshold=0.60000002;
			defaultDamageOverride[]=
			{
				{0.5,1}
			};
			class Health
			{
				damage=42;
				armorDamage=3;
			};
			class Blood
			{
				damage=100;
			};
			class Shock
			{
				damage=40;
			};
		};
		class NoiseHit
		{
			strength=40;
			type="shot";
		};
	};
};
class cfgVehicles
{
	class Box_Base;
	class AmmoBox_Expansion_46x30_25rnd: Box_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_AMMOBOX_46x30";
		descriptionShort="$STR_EXPANSION_AMMOBOX_46x30_DESC";
		model="\DayZExpansion\Objects\Weapons\Firearms\SMG\mp7\Ammobox_46x30.p3d";
		rotationFlags=17;
		lootCategory="Ammo";
		weight=305;
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
								"DZ\weapons\ammunition\data\45cal_box.rvmat"
							}
						},
						
						{
							0.69999999,
							{}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\ammunition\data\45cal_box_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							{}
						},
						
						{
							0,
							
							{
								"DZ\weapons\ammunition\data\45cal_box_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class Resources
		{
			class Ammo_Expansion_46x30
			{
				value=25;
				variable="quantity";
			};
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class interact
				{
					soundset="ammoboxUnpack_SoundSet";
					id=70;
				};
			};
		};
	};
};
