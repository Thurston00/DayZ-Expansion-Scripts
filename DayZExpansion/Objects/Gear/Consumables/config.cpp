class CfgPatches
{
	class DayZExpansion_Objects_Gear_Consumables
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Gear_Drinks",
			"DZ_Gear_Food"
		};
	};
};
class CfgLiquidDefinitions
{
	class ExpansionMilk
	{
		type=65536;
		displayName="$STR_EXPANSION_MILK_LIQUID";
		flammability=10;
		liquidFreezeThreshold=0;
		liquidThawThreshold=0;
		liquidBoilingThreshold=150;
		color="EXPANSION_MILK";
		class Nutrition
		{
			fullnessIndex=1;
			energy=50;
			water=100;
			nutritionalIndex=100;
			toxicity=-5;
			digestibility=2;
		};
	};
};
class BaseFoodStageTransitions
{
};
class FruitStageTransitions: BaseFoodStageTransitions
{
};
class CfgVehicles
{
	class WaterBottle;
	class Edible_Base;
	class ExpansionMilkBottle: WaterBottle
	{
		scope=2;
		displayName="$STR_EXPANSION_MILK_BOTTLE";
		descriptionShort="$STR_EXPANSION_MILK_BOTTLE_DESC";
		model="\dz\gear\drinks\MilkBottle.p3d";
		weight=25;
		itemSize[]={1,3};
		destroyOnEmpty=0;
		varQuantityDestroyOnMin=0;
		varLiquidTypeInit=65536;
		liquidContainerType="1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256 + 512 + 1024 + 2048 + 4096 + 8192 + 16384 + 32768 + 65536  - (1 + 2 + 4 + 8 + 16 + 32 + 64 + 128 + 256)";
		varQuantityInit=500;
		varQuantityMin=0;
		varQuantityMax=500;
		isMeleeWeapon=1;
	};
	class Expansion_FoodBase: Edible_Base
	{
		scope=0;
		rotationFlags=2;
		weight=0;
		stackedUnit="g";
		absorbency=0.2;
		varQuantityInit=125;
		varQuantityMin=0;
		varQuantityMax=125;
		quantityBar=1;
		ammoType="G_GrenadeHand";
		inventorySlot[]=
		{
			"Ingredient",
			"DirectCookingA",
			"DirectCookingB",
			"DirectCookingC"
		};
		containsSeedsType="";
		containsSeedsQuantity="0";
		class MeleeModes
		{
			class Default
			{
				ammo="MeleeFist";
				range=1;
			};
			class Heavy
			{
				ammo="MeleeFist_Heavy";
				range=1;
			};
			class Sprint
			{
				ammo="MeleeFist_Heavy";
				range=2.8;
			};
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
							{}
						},
						
						{
							0.69999999,
							{}
						},
						
						{
							0.5,
							{}
						},
						
						{
							0.30000001,
							{}
						},
						
						{
							0,
							{}
						}
					};
				};
			};
		};
	};
	class ExpansionBread1: Expansion_FoodBase
	{
		scope=2;
		rotationFlags=2;
		displayName="$STR_EXPANSION_BAGUETTE";
		descriptionShort="$STR_EXPANSION_BAGUETTE_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Bread1.p3d";
		weight=0;
		itemSize[]={2,5};
		class Nutrition
		{
			fullnessIndex=2;
			energy=548;
			water=12.5;
			nutritionalIndex=1;
			toxicity=0;
		};
		class Food
		{
			class FoodStages
			{
				class Raw
				{
					visual_properties[]={0,0,0};
					nutrition_properties[]={2.5,50,80,1,0};
					cooking_properties[]={0,0};
				};
				class Rotten
				{
					visual_properties[]={-1,-1,5};
					nutrition_properties[]={2,37.5,40,1,0};
					cooking_properties[]={0,0};
				};
				class Baked
				{
					visual_properties[]={1,1,1};
					nutrition_properties[]={1.75,87.5,60,1,0};
					cooking_properties[]={70,35};
				};
				class Dried
				{
					visual_properties[]={2,3,3};
					nutrition_properties[]={0.75,50,8,1,0};
					cooking_properties[]={70,30,80};
				};
				class Burned
				{
					visual_properties[]={1,4,4};
					nutrition_properties[]={2,37.5,20,1,0};
					cooking_properties[]={100,20};
				};
			};
			class FoodStageTransitions: FruitStageTransitions
			{
			};
		};
	};
	class ExpansionBread2: Expansion_FoodBase
	{
		scope=2;
		rotationFlags=2;
		displayName="$STR_EXPANSION_BREAD";
		descriptionShort="$STR_EXPANSION_BREAD_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Bread2.p3d";
		weight=0;
		itemSize[]={2,4};
		class Nutrition
		{
			fullnessIndex=2;
			energy=548;
			water=12.5;
			nutritionalIndex=1;
			toxicity=0;
		};
		class Food
		{
			class FoodStages
			{
				class Raw
				{
					visual_properties[]={0,0,0};
					nutrition_properties[]={2.5,50,80,1,0};
					cooking_properties[]={0,0};
				};
				class Rotten
				{
					visual_properties[]={-1,-1,5};
					nutrition_properties[]={2,37.5,40,1,0};
					cooking_properties[]={0,0};
				};
				class Baked
				{
					visual_properties[]={1,1,1};
					nutrition_properties[]={1.75,87.5,60,1,0};
					cooking_properties[]={70,35};
				};
				class Dried
				{
					visual_properties[]={2,3,3};
					nutrition_properties[]={0.75,50,8,1,0};
					cooking_properties[]={70,30,80};
				};
				class Burned
				{
					visual_properties[]={1,4,4};
					nutrition_properties[]={2,37.5,20,1,0};
					cooking_properties[]={100,20};
				};
			};
			class FoodStageTransitions: FruitStageTransitions
			{
			};
		};
	};
	class ExpansionBread3: Expansion_FoodBase
	{
		scope=2;
		displayName="$STR_EXPANSION_BREAD";
		descriptionShort="$STR_EXPANSION_BREAD_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Bread3.p3d";
		itemSize[]={2,3};
		class Nutrition
		{
			fullnessIndex=2;
			energy=548;
			water=12.5;
			nutritionalIndex=1;
			toxicity=0;
		};
		class Food
		{
			class FoodStages
			{
				class Raw
				{
					visual_properties[]={0,0,0};
					nutrition_properties[]={2.5,50,80,1,0};
					cooking_properties[]={0,0};
				};
				class Rotten
				{
					visual_properties[]={-1,-1,5};
					nutrition_properties[]={2,37.5,40,1,0};
					cooking_properties[]={0,0};
				};
				class Baked
				{
					visual_properties[]={1,1,1};
					nutrition_properties[]={1.75,87.5,60,1,0};
					cooking_properties[]={70,35};
				};
				class Dried
				{
					visual_properties[]={2,3,3};
					nutrition_properties[]={0.75,50,8,1,0};
					cooking_properties[]={70,30,80};
				};
				class Burned
				{
					visual_properties[]={1,4,4};
					nutrition_properties[]={2,37.5,20,1,0};
					cooking_properties[]={100,20};
				};
			};
			class FoodStageTransitions: FruitStageTransitions
			{
			};
		};
	};
	class ExpansionCheese1: Expansion_FoodBase
	{
		scope=2;
		displayName="$STR_EXPANSION_CHEESE";
		descriptionShort="$STR_EXPANSION_CHEESE_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Cheese1.p3d";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=2;
			energy=248;
			water=192.5;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ExpansionCheese2: Expansion_FoodBase
	{
		scope=2;
		rotationFlags=2;
		displayName="$STR_EXPANSION_CHEESE";
		descriptionShort="$STR_EXPANSION_CHEESE_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Cheese2.p3d";
		itemSize[]={2,3};
		class Nutrition
		{
			fullnessIndex=2;
			energy=248;
			water=192.5;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ExpansionCheese3: Expansion_FoodBase
	{
		scope=2;
		displayName="$STR_EXPANSION_CHEESE";
		descriptionShort="$STR_EXPANSION_CHEESE_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Cheese3.p3d";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=2;
			energy=248;
			water=192.5;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
	class ExpansionCheese4: Expansion_FoodBase
	{
		scope=2;
		displayName="$STR_EXPANSION_CHEESE";
		descriptionShort="$STR_EXPANSION_CHEESE_DESC";
		model="\DayZExpansion\Objects\Gear\Consumables\Cheese4.p3d";
		itemSize[]={2,2};
		class Nutrition
		{
			fullnessIndex=2;
			energy=248;
			water=192.5;
			nutritionalIndex=1;
			toxicity=0;
		};
	};
};
