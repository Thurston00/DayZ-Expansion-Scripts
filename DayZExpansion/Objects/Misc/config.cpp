class CfgPatches
{
	class DayZExpansion_Objects_Misc
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
	class Inventory_Base;
	class Pliers;
	class ExpansionBoltCutters: Pliers
	{
		scope=2;
		displayName="$STR_EXPANSION_ITEM_BOLTCUTTERS";
		descriptionShort="$STR_EXPANSION_ITEM_BOLTCUTTERS_DESC";
		model="DayZExpansion\Objects\Misc\BoltCutters.p3d";
		weight=4000;
		itemSize[]={2,5};
	};
	class ExpansionPropaneTorch: Inventory_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_ITEM_PROPANETORCH";
		descriptionShort="$STR_EXPANSION_ITEM_PROPANETORCH_DESC";
		model="DayZExpansion\Objects\Misc\PropaneTorch.p3d";
		weight=5000;
		itemBehaviour=2;
		itemSize[]={2,5};
		rotationFlags=17;
		stackedUnit="percentage";
		quantityBar=1;
		varQuantityInit=100;
		varQuantityMin=0;
		varQuantityMax=100;
		varQuantityDestroyOnMin=0;
		animClass="Knife";
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
								""
							}
						},
						
						{
							0.69999999,
							
							{
								""
							}
						},
						
						{
							0.5,
							
							{
								""
							}
						},
						
						{
							0.30000001,
							
							{
								""
							}
						},
						
						{
							0,
							
							{
								""
							}
						}
					};
				};
			};
		};
		class EnergyManager
		{
			hasIcon=1;
			switchOnAtSpawn=0;
			autoSwitchOff=1;
			autoSwitchOffWhenInCargo=1;
			convertEnergyToQuantity=1;
			energyAtSpawn=5000;
			energyStorageMax=5000;
			energyUsagePerSecond=2;
			reduceMaxEnergyByDamageCoef=0.5;
			updateInterval=3;
		};
		class MeleeModes
		{
			class Default
			{
				ammo="MeleeLightBlunt";
				range=1;
			};
			class Heavy
			{
				ammo="MeleeLightBlunt_Heavy";
				range=1;
			};
			class Sprint
			{
				ammo="MeleeLightBlunt_Heavy";
				range=2.8;
			};
		};
	};
	class ExpansionToolBox: Container_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_ITEM_TOOLBOX";
		descriptionShort="$STR_EXPANSION_ITEM_TOOLBOX_DESC";
		model="DayZExpansion\Objects\Misc\ToolBox.p3d";
		rotationFlags=2;
		weight=10000;
		heavyitem=1;
		itemBehaviour=2;
		itemSize[]={5,3};
		inventorySlot[]=
		{
			"WoodenCrate",
			"woodencrate_1",
			"woodencrate_2",
			"woodencrate_3",
			"woodencrate_4",
			"Truck_01_WoodenCrate1",
			"Truck_01_WoodenCrate2",
			"Truck_01_WoodenCrate3",
			"Truck_01_WoodenCrate4",
			"Truck_01_WoodenCrate5",
			"Truck_01_WoodenCrate6"
		};
		physLayer="item_large";
		carveNavmesh=1;
		canBeDigged=1;
		class Cargo
		{
			itemsCargoSize[]={10,3};
			openable=0;
			allowOwnedCargoManipulation=1;
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=500;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								""
							}
						},
						
						{
							0.69999999,
							
							{
								""
							}
						},
						
						{
							0.5,
							
							{
								""
							}
						},
						
						{
							0.30000001,
							
							{
								""
							}
						},
						
						{
							0,
							
							{
								""
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
				class pickUp_Light
				{
					soundSet="pickUpCanisterGasolineLight_SoundSet";
					id=796;
				};
				class pickUp
				{
					soundSet="pickUpCanisterGasoline_SoundSet";
					id=797;
				};
				class drop
				{
					soundset="carradiator_drop_SoundSet";
					id=898;
				};
			};
		};
	};
	class ExpansionParkingMeter: Inventory_Base
	{
		scope=2;
		displayName="Parking Meter";
		descriptionShort="Used to get access to the Virtual Garage.";
		model="DayZExpansion\Objects\Misc\ParkingMeter.p3d";
		bounding="BSphere";
		overrideDrawArea="3.0";
		forceFarBubble="true";
		slopeTolerance=0.2;
		yawPitchRollLimit[]={12,12,12};
		hiddenSelections[]=
		{
			"camo",
			"light"
		};
		hiddenSelectionsTextures[]=
		{
			"DayZExpansion\Objects\Misc\data\parking_meter_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DayZExpansion\Objects\Misc\data\parking_meter.rvmat"
		};
		openable=1;
		carveNavmesh=1;
		weight=1000;
		itemSize[]={4,4};
		itemsCargoSize[]={};
		itemBehaviour=2;
		lootCategory="Crafted";
		placement="ForceSlopeOnTerrain";
		physLayer="item_large";
		repairableWithKits[]={7,10};
		repairCosts[]={25,30};
		attachments[]=
		{
			"CarBattery",
			"Att_ExpansionCircuitBoard"
		};
		rotationFlags=2;
		soundImpactType="metal";
		oldpower=0;
		ChargeEnergyPerSecond=1;
		class EnergyManager
		{
			hasIcon=1;
			autoSwitchOff=1;
			energyUsagePerSecond=0.0099999998;
			plugType=5;
			attachmentAction=1;
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=10000;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DayZExpansion\Objects\Misc\data\parking_meter.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DayZExpansion\Objects\Misc\data\parking_meter.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DayZExpansion\Objects\Misc\data\parking_meter_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DayZExpansion\Objects\Misc\data\parking_meter_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DayZExpansion\Objects\Misc\data\parking_meter_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class AnimationSources
		{
			class camo
			{
				source="user";
				animPeriod=0.0099999998;
				initPhase=0;
			};
			class carbattery
			{
				source="user";
				animPeriod=0.0099999998;
				initPhase=0;
			};
			class battery_installed
			{
				source="user";
				animPeriod=0.0099999998;
				initPhase=1;
			};
			class placing
			{
				source="user";
				animPeriod=0.0099999998;
				initPhase=0;
			};
		};
		class GUIInventoryAttachmentsProps
		{
			class Attachments
			{
				name="$STR_attachment_accessories";
				description="";
				attachmentSlots[]=
				{
					"CarBattery",
					"Att_ExpansionCircuitBoard"
				};
				icon="set:expansion_iconset image:icon_options";
			};
		};
	};
};
