class CfgPatches
{
	class DayZExpansion_Quests_Objects_Containers
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Data"
		};
	};
};
class CfgVehicles
{
	class Container_Base;
	class HouseNoDestruct;
	class ExpansionQuestStaticObject: HouseNoDestruct
	{
		forceFarBubble="true";
	};
	class ExpansionQuestObjectBoard: ExpansionQuestStaticObject
	{
		displayName="Quest Board";
		descriptionShort="Quest Board";
		model="DZ\structures\Furniture\School_equipment\blackboard.p3d";
	};
	class ExpansionQuestContainerBase: Container_Base
	{
		scope=2;
		displayName="#STR_EXPANSION_TEMPORARY_STORAGE";
		descriptionShort="#STR_EXPANSION_TEMPORARY_STORAGE_DESC";
		model="\DZ\gear\camping\wooden_case.p3d";
		itemSize[]={10,5};
		attachments[]=
		{
			"Shoulder",
			"Melee",
			"Vest",
			"Body",
			"Hips",
			"Legs",
			"Back",
			"Headgear",
			"Mask",
			"Eyewear",
			"Gloves",
			"Feet",
			"Armband"
		};
	};
	class ExpansionQuestSeaChest: ExpansionQuestContainerBase
	{
		scope=2;
		displayName="$STR_CfgVehicles_SeaChest0";
		descriptionShort="$STR_CfgVehicles_SeaChest1";
		model="\DZ\gear\camping\sea_chest.p3d";
		class Cargo
		{
			itemsCargoSize[]={10,100};
			openable=0;
			allowOwnedCargoManipulation=1;
		};
		class AnimEvents
		{
			class SoundWeapon
			{
				class movement
				{
					soundSet="seachest_movement_SoundSet";
					id=1;
				};
				class pickUpItem_Light
				{
					soundSet="pickUpSeaChest_SoundSet";
					id=796;
				};
				class pickUpItem
				{
					soundSet="pickUpSeaChest_SoundSet";
					id=797;
				};
				class drop
				{
					soundset="seachest_drop_SoundSet";
					id=898;
				};
			};
		};
	};
	class ExpansionQuestDryBag_ColorBase: ExpansionQuestContainerBase
	{
		displayName="$STR_cfgVehicles_DryBag_ColorBase0";
		descriptionShort="$STR_cfgVehicles_DryBag_ColorBase1";
		model="\dz\characters\backpacks\drybag_g.p3d";
		debug_ItemCategory=9;
		attachments[]=
		{
			"Chemlight",
			"WalkieTalkie"
		};
		rotationFlags=16;
		itemSize[]={5,6};
		itemsCargoSize[]={9,7};
		weight=600;
		canBeDigged=1;
		soundAttType="Outdoor";
		randomQuantity=4;
		hiddenSelections[]=
		{
			"camoGround",
			"camoMale",
			"camoFemale"
		};
		hiddenSelectionsTextures[]=
		{
			"",
			"",
			""
		};
		class ClothingTypes
		{
			male="\DZ\characters\backpacks\drybag_m.p3d";
			female="\DZ\characters\backpacks\drybag_f.p3d";
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=140;
					transferToAttachmentsCoef=0.5;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\Characters\backpacks\data\dryBag.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DZ\Characters\backpacks\data\dryBag.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DZ\Characters\backpacks\data\dryBag_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DZ\Characters\backpacks\data\dryBag_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DZ\Characters\backpacks\data\dryBag_destruct.rvmat"
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
				class pickUpItem_Light
				{
					soundSet="pickUpBackPack_Plastic_Light_SoundSet";
					id=796;
				};
				class pickUpItem
				{
					soundSet="pickUpBackPack_Plastic_SoundSet";
					id=797;
				};
				class drop
				{
					soundset="taloonbag_drop_SoundSet";
					id=898;
				};
			};
		};
	};
	class ExpansionQuestDryBag_Orange: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_Orange_co.paa",
			"DZ\characters\backpacks\data\DryBag_Orange_co.paa",
			"DZ\characters\backpacks\data\DryBag_Orange_co.paa"
		};
	};
	class ExpansionQuestDryBag_Yellow: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_Yellow_co.paa",
			"DZ\characters\backpacks\data\DryBag_Yellow_co.paa",
			"DZ\characters\backpacks\data\DryBag_Yellow_co.paa"
		};
	};
	class ExpansionQuestDryBag_Blue: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		visibilityModifier=0.94999999;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_Blue_co.paa",
			"DZ\characters\backpacks\data\DryBag_Blue_co.paa",
			"DZ\characters\backpacks\data\DryBag_Blue_co.paa"
		};
	};
	class ExpansionQuestDryBag_Green: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		visibilityModifier=0.89999998;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_Green_co.paa",
			"DZ\characters\backpacks\data\DryBag_Green_co.paa",
			"DZ\characters\backpacks\data\DryBag_Green_co.paa"
		};
	};
	class ExpansionQuestDryBag_Black: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		visibilityModifier=0.80000001;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_black_co.paa",
			"DZ\characters\backpacks\data\DryBag_black_co.paa",
			"DZ\characters\backpacks\data\DryBag_black_co.paa"
		};
	};
	class ExpansionQuestDryBag_Red: ExpansionQuestDryBag_ColorBase
	{
		scope=2;
		hiddenSelectionsTextures[]=
		{
			"DZ\characters\backpacks\data\DryBag_Red_co.paa",
			"DZ\characters\backpacks\data\DryBag_Red_co.paa",
			"DZ\characters\backpacks\data\DryBag_Red_co.paa"
		};
	};
};
