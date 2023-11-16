#define _ARMA_

class CfgPatches
{
	class DayZExpansion_Vehicles_Air_Mh6
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DayZExpansion_Vehicles_Data"};
	};
};
class CfgSlots
{
	class Slot_mh6door_1_1
	{
		name = "mh6door_1_1";
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_LEFT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_LEFT_DESC";
		selection = "mh6door_1_1";
		ghostIcon = "doorfront";
	};
	class Slot_mh6door_1_2
	{
		name = "mh6door_1_2";
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_RIGHT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_RIGHT_DESC";
		selection = "mh6door_1_2";
		ghostIcon = "doorfront";
	};
	class Slot_mh6door_2_1
	{
		name = "mh6door_2_1";
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_LEFT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_LEFT_DESC";
		selection = "mh6door_2_1";
		ghostIcon = "doorrear";
	};
	class Slot_mh6door_2_2
	{
		name = "mh6door_2_2";
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_RIGHT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_RIGHT_DESC";
		selection = "mh6door_2_2";
		ghostIcon = "doorrear";
	};
};
class CfgVehicles
{
	class Inventory_Base;
	class ExpansionWreck;
	class SimulationModule;
	class Axles;
	class Suspension;
	class Front;
	class Rear;
	class AnimationSources;
	class Crew;
	class Driver;
	class CoDriver;
	class ExpansionHelicopterScript;
	class ExpansionVehicleHelicopterBase;
	class GUIInventoryAttachmentsProps;
	class Body;
	class DamageSystem;
	class DamageZones;
	class GlobalHealth;
	class HatchbackWheel;
	class HatchbackDoors_Driver;
	class Expansion_Mh6_Door_1_1: HatchbackDoors_Driver
	{
		scope = 2;
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_LEFT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_LEFT_DESC";
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_1_1.p3d";
		weight = 15000;
		itemSize[] = {10,10};
		itemBehaviour = 0;
		inventorySlot = "mh6door_1_1";
		rotationFlags = 8;
		physLayer = "item_large";
		hiddenSelections[] = {"camo"};
		hiddenSelectionsTextures[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa"};
		hiddenSelectionsMaterials[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext.rvmat"};
	};
	class Expansion_Mh6_Door_1_2: Expansion_Mh6_Door_1_1
	{
		scope = 2;
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_RIGHT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_FRONT_RIGHT_DESC";
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_1_2.p3d";
		inventorySlot = "mh6door_1_2";
	};
	class Expansion_Mh6_Door_2_1: Expansion_Mh6_Door_1_1
	{
		scope = 2;
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_LEFT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_LEFT_DESC";
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_2_1.p3d";
		inventorySlot = "mh6door_2_1";
	};
	class Expansion_Mh6_Door_2_2: Expansion_Mh6_Door_1_1
	{
		scope = 2;
		displayName = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_RIGHT";
		descriptionShort = "$STR_EXPANSION_VEHICLE_MH6_DOOR_BACK_RIGHT_DESC";
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_2_2.p3d";
		inventorySlot = "mh6door_2_2";
	};
	class ExpansionMh6Wreck: ExpansionWreck
	{
		scope = 2;
		displayname = "$STR_EXPANSION_VEHICLE_MH6";
		model = "\DayZExpansion\Vehicles\Air\Mh6\Mh6Wreck.p3d";
		class Cargo
		{
			itemsCargoSize[] = {10,20};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
	};
	class ExpansionMh6: ExpansionHelicopterScript
	{
		scope = 2;
		displayname = "$STR_EXPANSION_VEHICLE_MH6";
		model = "\DayZExpansion\Vehicles\Air\Mh6\Mh6.p3d";
		modelZeroPointDistanceFromGround = 0.0;
		vehicleClass = "Expansion_Helicopter";
		wreck = "ExpansionMh6Wreck";
		fuelCapacity = 192;
		fuelConsumption = 91;
		attachments[] = {"ExpansionHelicopterBattery","Reflector_1_1","ExpansionIgniterPlug","ExpansionHydraulicHoses","mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2","NivaWheel_1_1","NivaWheel_1_2","NivaWheel_2_1","NivaWheel_2_2","CamoNet","KeyChain"};
		doors[] = {"mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2"};
		applySkinsTo[] = {"Expansion_Mh6_Door_1_1","Expansion_Mh6_Door_1_2","Expansion_Mh6_Door_2_1","Expansion_Mh6_Door_2_2"};
		hiddenSelections[] = {"light_1_1","light_2_1","camo"};
		hiddenSelectionsTextures[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa"};
		hiddenSelectionsMaterials[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext.rvmat"};
		defaultSkin = "Green";
		class Cargo
		{
			itemsCargoSize[] = {10,20};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
		frontReflectorMatOn = "dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_on.rvmat";
		frontReflectorMatOff = "dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat";
		class GUIInventoryAttachmentsProps
		{
			class Engine
			{
				name = "$STR_attachment_Engine0";
				description = "";
				icon = "cat_vehicle_engine";
				attachmentSlots[] = {"ExpansionHelicopterBattery","ExpansionIgniterPlug","ExpansionHydraulicHoses"};
			};
			class Body
			{
				name = "$STR_attachment_Body0";
				description = "";
				icon = "cat_vehicle_body";
				attachmentSlots[] = {"Reflector_1_1","mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2","CamoNet","KeyChain"};
			};
		};
		class AnimationSources
		{
			class rotor
			{
				source = "user";
				animPeriod = 0.025;
				initPhase = 0;
			};
			class rearrotor
			{
				source = "user";
				animPeriod = 0.025;
				initPhase = 0;
			};
			class hiderotor
			{
				source = "user";
				animPeriod = 0.0009999999;
				initPhase = 0;
			};
			class hiderotorblur
			{
				source = "user";
				animPeriod = 0.0009999999;
				initPhase = 0;
			};
			class mh6door_1_1
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_1_2
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_2_1
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_2_2
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class cyclicForward
			{
				source = "user";
				animPeriod = 0.25;
				initPhase = 0;
			};
			class cyclicAside
			{
				source = "user";
				animPeriod = 0.25;
				initPhase = 0;
			};
		};
		class Sounds
		{
			thrust = 0.6;
			thrustTurbo = 1;
			thrustGentle = 0.3;
			thrustSmoothCoef = 0.1;
			camposSmoothCoef = 0.03;
			soundSetsFilter[] = {"Expansion_Mh6_Engine_Ext_SoundSet","Expansion_Mh6_Rotor_Ext_SoundSet"};
			soundSetsInt[] = {"Expansion_Mh6_Engine_Int_SoundSet","Expansion_Mh6_Rotor_Int_SoundSet"};
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 5000;
					healthLevels[] = {{1.0,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0.0,{}}};
				};
			};
			class DamageZones
			{
				class Chassis
				{
					class Health
					{
						hitpoints = 3000;
						transferToGlobalCoef = 0;
					};
					fatalInjuryCoef = -1;
					componentNames[] = {};
					inventorySlots[] = {};
				};
				class Engine
				{
					class Health
					{
						hitpoints = 1000;
						transferToGlobalCoef = 1;
						healthLevels[] = {{1.0,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva.rvmat"}},{0.7,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva.rvmat"}},{0.5,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}},{0.3,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}},{0.0,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}}};
					};
					fatalInjuryCoef = 0.001;
					memoryPoints[] = {"dmgZone_engine"};
					componentNames[] = {"dmgZone_engine"};
					inventorySlots[] = {"ExpansionHelicopterBattery","ExpansionIgniterPlug"};
					inventorySlotsCoefs[] = {0.2,0.2,0.01,0.4};
				};
				class FuelTank
				{
					class Health
					{
						hitpoints = 600;
						transferToGlobalCoef = 0;
						healthLevels[] = {{1.0,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0.0,{}}};
					};
					fatalInjuryCoef = -1;
					componentNames[] = {"dmgZone_fuelTank"};
					inventorySlots[] = {};
				};
				class Reflector_1_1
				{
					fatalInjuryCoef = -1;
					memoryPoints[] = {"dmgZone_lights_1_1"};
					componentNames[] = {"dmgZone_lights_1_1"};
					class Health
					{
						hitpoints = 20;
						transferToGlobalCoef = 0;
						healthLevels[] = {{1.0,{},{}},{0.7,{}},{0.5,{},{}},{0.3,{}},{0.0,{},{}}};
					};
					inventorySlots[] = {"Reflector_1_1"};
					inventorySlotsCoefs[] = {1.0};
				};
			};
		};
		class Crew: Crew
		{
			class Driver: Driver
			{
				actionSel = "seat_driver";
				proxyPos = "crewDriver";
				getInPos = "pos_driver";
				getInDir = "pos_driver_dir";
			};
			class CoDriver: CoDriver
			{
				actionSel = "seat_codriver";
				proxyPos = "crewCoDriver";
				getInPos = "pos_codriver";
				getInDir = "pos_codriver_dir";
			};
			class Cargo1
			{
				actionSel = "seat_cargo1";
				proxyPos = "crewCargo1";
				getInPos = "pos cargo l";
				getInDir = "pos cargo dir l";
			};
			class Cargo2
			{
				actionSel = "seat_cargo2";
				proxyPos = "crewCargo2";
				getInPos = "pos cargo r";
				getInDir = "pos cargo dir r";
			};
		};
		class SimulationModule: SimulationModule
		{
			maxSpeed = 220;
			altitudeFullForce = 1000;
			altitudeNoForce = 2000;
			bodyFrictionCoef = 1.1;
			liftForceCoef = 1.3;
			bankForceCoef = 0.05;
			tailForceCoef = 2.0;
			linearFrictionCoef[] = {16.0,0.04,0.04};
			angularFrictionCoef = 1.5;
			class Rotor
			{
				minAutoRotateSpeed = 2.0;
				maxAutoRotateSpeed = 10.0;
				startUpTime = 15;
			};
			class AntiTorque
			{
				speed = 1.5;
				max = 0.16;
			};
			class Cyclic
			{
				forceCoefficient = 1.3;
				class Forward
				{
					hydraulic[] = {0.0,0.0,0.3,8.0,1.0,10.0};
					max = 0.7;
					coefficient = 1.0;
					animation = "cyclicForward";
				};
				class Side
				{
					hydraulic[] = {0.0,0.0,0.3,8.0,1.0,10.0};
					max = 0.7;
					coefficient = 1.0;
					animation = "cyclicAside";
				};
			};
		};
	};
	class Vehicle_ExpansionMh6: ExpansionVehicleHelicopterBase
	{
		scope = 0;
		displayname = "$STR_EXPANSION_VEHICLE_MH6";
		model = "\DayZExpansion\Vehicles\Air\Mh6\Mh6.p3d";
		modelZeroPointDistanceFromGround = 0.0;
		vehicleClass = "Expansion_Helicopter";
		wreck = "ExpansionMh6Wreck";
		fuelCapacity = 192;
		fuelConsumption = 91;
		attachments[] = {"ExpansionHelicopterBattery","Reflector_1_1","CarRadiator","ExpansionIgniterPlug","ExpansionHydraulicHoses","mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2"};
		doors[] = {"mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2"};
		applySkinsTo[] = {"Expansion_Mh6_Door_1_1","Expansion_Mh6_Door_1_2","Expansion_Mh6_Door_2_1","Expansion_Mh6_Door_2_2"};
		hiddenSelections[] = {"light_1_1","light_2_1","camo"};
		hiddenSelectionsTextures[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_co.paa"};
		hiddenSelectionsMaterials[] = {"dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat","dayzexpansion\vehicles\air\mh6\data\heli_light01_ext.rvmat"};
		defaultSkin = "Green";
		class Cargo
		{
			itemsCargoSize[] = {10,20};
			allowOwnedCargoManipulation = 1;
			openable = 0;
		};
		frontReflectorMatOn = "dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_on.rvmat";
		frontReflectorMatOff = "dayzexpansion\vehicles\air\mh6\data\heli_light01_ext_off.rvmat";
		class GUIInventoryAttachmentsProps
		{
			class Engine
			{
				name = "$STR_attachment_Engine0";
				description = "";
				icon = "cat_vehicle_engine";
				attachmentSlots[] = {"ExpansionHelicopterBattery","ExpansionIgniterPlug","ExpansionHydraulicHoses"};
			};
			class Body
			{
				name = "$STR_attachment_Body0";
				description = "";
				icon = "cat_vehicle_body";
				attachmentSlots[] = {"Reflector_1_1","mh6door_1_1","mh6door_1_2","mh6door_2_1","mh6door_2_2"};
			};
		};
		class AnimationSources
		{
			class rotor
			{
				source = "user";
				animPeriod = 0.025;
				initPhase = 0;
			};
			class rearrotor
			{
				source = "user";
				animPeriod = 0.025;
				initPhase = 0;
			};
			class hiderotor
			{
				source = "user";
				animPeriod = 0.0009999999;
				initPhase = 0;
			};
			class hiderotorblur
			{
				source = "user";
				animPeriod = 0.0009999999;
				initPhase = 0;
			};
			class mh6door_1_1
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_1_2
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_2_1
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class mh6door_2_2
			{
				source = "user";
				animPeriod = 0.5;
				initPhase = 0;
			};
			class cyclicForward
			{
				source = "user";
				animPeriod = 0.25;
				initPhase = 0;
			};
			class cyclicAside
			{
				source = "user";
				animPeriod = 0.25;
				initPhase = 0;
			};
		};
		class Sounds
		{
			thrust = 0.6;
			thrustTurbo = 1;
			thrustGentle = 0.3;
			thrustSmoothCoef = 0.1;
			camposSmoothCoef = 0.03;
			soundSetsFilter[] = {"Expansion_Mh6_Engine_Ext_SoundSet","Expansion_Mh6_Rotor_Ext_SoundSet"};
			soundSetsInt[] = {"Expansion_Mh6_Engine_Int_SoundSet","Expansion_Mh6_Rotor_Int_SoundSet"};
		};
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints = 5000;
					healthLevels[] = {{1.0,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0.0,{}}};
				};
			};
			class DamageZones
			{
				class Chassis
				{
					class Health
					{
						hitpoints = 3000;
						transferToGlobalCoef = 0;
					};
					fatalInjuryCoef = -1;
					componentNames[] = {};
					inventorySlots[] = {};
				};
				class Engine
				{
					class Health
					{
						hitpoints = 1000;
						transferToGlobalCoef = 1;
						healthLevels[] = {{1.0,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva.rvmat"}},{0.7,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva.rvmat"}},{0.5,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}},{0.3,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}},{0.0,{"dz\vehicles\wheeled\offroadhatchback\data\engine_niva_destruct.rvmat"}}};
					};
					fatalInjuryCoef = 0.001;
					memoryPoints[] = {"dmgZone_engine"};
					componentNames[] = {"dmgZone_engine"};
					inventorySlots[] = {"ExpansionHelicopterBattery","ExpansionIgniterPlug"};
					inventorySlotsCoefs[] = {0.2,0.2,0.01,0.4};
				};
				class FuelTank
				{
					class Health
					{
						hitpoints = 600;
						transferToGlobalCoef = 0;
						healthLevels[] = {{1.0,{}},{0.7,{}},{0.5,{}},{0.3,{}},{0.0,{}}};
					};
					fatalInjuryCoef = -1;
					componentNames[] = {"dmgZone_fuelTank"};
					inventorySlots[] = {};
				};
			};
		};
		class Crew: Crew
		{
			class Driver: Driver
			{
				actionSel = "seat_driver";
				proxyPos = "crewDriver";
				getInPos = "pos_driver";
				getInDir = "pos_driver_dir";
			};
			class CoDriver: CoDriver
			{
				actionSel = "seat_codriver";
				proxyPos = "crewCoDriver";
				getInPos = "pos_codriver";
				getInDir = "pos_codriver_dir";
			};
			class Cargo1
			{
				actionSel = "seat_cargo1";
				proxyPos = "crewCargo1";
				getInPos = "pos cargo l";
				getInDir = "pos cargo dir l";
			};
			class Cargo2
			{
				actionSel = "seat_cargo2";
				proxyPos = "crewCargo2";
				getInPos = "pos cargo r";
				getInDir = "pos cargo dir r";
			};
		};
		class SimulationModule: SimulationModule
		{
			maxSpeed = 220;
			altitudeFullForce = 1000;
			altitudeNoForce = 2000;
			bodyFrictionCoef = 1.1;
			liftForceCoef = 1.3;
			bankForceCoef = 0.05;
			tailForceCoef = 2.0;
			linearFrictionCoef[] = {16.0,0.04,0.04};
			angularFrictionCoef = 1.5;
			class Rotor
			{
				minAutoRotateSpeed = 2.0;
				maxAutoRotateSpeed = 10.0;
				startUpTime = 15;
			};
			class AntiTorque
			{
				speed = 1.5;
				max = 0.16;
			};
			class Cyclic
			{
				forceCoefficient = 1.3;
				class Forward
				{
					speed = 10.0;
					max = 0.7;
					coefficient = 1.0;
					animation = "cyclicForward";
				};
				class Side
				{
					speed = 10.0;
					max = 0.7;
					coefficient = 1.0;
					animation = "cyclicAside";
				};
			};
			class Axles: Axles
			{
				class Front: Front
				{
					class Suspension: Suspension
					{
						swayBar = 1;
						stiffness = 1;
						compression = 1;
						damping = 1;
						travelMaxUp = 0;
						travelMaxDown = 0;
					};
				};
				class Rear: Rear
				{
					class Suspension: Suspension
					{
						swayBar = 1;
						stiffness = 1;
						compression = 1;
						damping = 1;
						travelMaxUp = 0;
						travelMaxDown = 0;
					};
				};
			};
		};
	};
	class ExpansionSpraycanBase;
	class ExpansionSpraycanGreen: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Green_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Green_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_green_co.paa"};
		skinName = "Green";
	};
	class ExpansionMh6_Green: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Green";
	};
	class Vehicle_ExpansionMh6_Green: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Green";
	};
	class Expansion_Mh6_Door_1_1_Green: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Green";
	};
	class Expansion_Mh6_Door_1_2_Green: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Green";
	};
	class Expansion_Mh6_Door_2_1_Green: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Green";
	};
	class Expansion_Mh6_Door_2_2_Green: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Green";
	};
	class ExpansionSpraycanBlueline: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Blueline_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Blueline_DESC";
		hiddenSelectionsTextures[] = {"\DayZExpansion\Objects\Gear\Spraycans\data\spraycan_blue_co.paa"};
		skinName = "Blueline";
	};
	class ExpansionMh6_Blueline: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Blueline";
	};
	class Vehicle_ExpansionMh6_Blueline: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Blueline";
	};
	class Expansion_Mh6_Door_1_1_Blueline: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Blueline";
	};
	class Expansion_Mh6_Door_1_2_Blueline: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Blueline";
	};
	class Expansion_Mh6_Door_2_1_Blueline: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Blueline";
	};
	class Expansion_Mh6_Door_2_2_Blueline: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Blueline";
	};
	class ExpansionSpraycanDigital: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Digital_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Digital_DESC";
		hiddenSelectionsTextures[] = {"\DayZExpansion\Objects\Gear\Spraycans\data\spraycan_grey_co.paa"};
		skinName = "Digital";
	};
	class ExpansionMh6_Digital: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Digital";
	};
	class Vehicle_ExpansionMh6_Digital: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Digital";
	};
	class Expansion_Mh6_Door_1_1_Digital: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Digital";
	};
	class Expansion_Mh6_Door_1_2_Digital: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Digital";
	};
	class Expansion_Mh6_Door_2_1_Digital: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Digital";
	};
	class Expansion_Mh6_Door_2_2_Digital: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Digital";
	};
	class ExpansionSpraycanElliptical: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Elliptical_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Elliptical_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Elliptical";
	};
	class ExpansionMh6_Elliptical: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Elliptical";
	};
	class Vehicle_ExpansionMh6_Elliptical: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Elliptical";
	};
	class Expansion_Mh6_Door_1_1_Elliptical: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Elliptical";
	};
	class Expansion_Mh6_Door_1_2_Elliptical: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Elliptical";
	};
	class Expansion_Mh6_Door_2_1_Elliptical: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Elliptical";
	};
	class Expansion_Mh6_Door_2_2_Elliptical: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Elliptical";
	};
	class ExpansionSpraycanFurious: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Furious_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Furious_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Furious";
	};
	class ExpansionMh6_Furious: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Furious";
	};
	class Vehicle_ExpansionMh6_Furious: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Furious";
	};
	class Expansion_Mh6_Door_1_1_Furious: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Furious";
	};
	class Expansion_Mh6_Door_1_2_Furious: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Furious";
	};
	class Expansion_Mh6_Door_2_1_Furious: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Furious";
	};
	class Expansion_Mh6_Door_2_2_Furious: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Furious";
	};
	class ExpansionSpraycanGrayWatcher: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_GrayWatcher_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_GrayWatcher_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "GrayWatcher";
	};
	class ExpansionMh6_GrayWatcher: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "GrayWatcher";
	};
	class Vehicle_ExpansionMh6_GrayWatcher: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "GrayWatcher";
	};
	class Expansion_Mh6_Door_1_1_GrayWatcher: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "GrayWatcher";
	};
	class Expansion_Mh6_Door_1_2_GrayWatcher: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "GrayWatcher";
	};
	class Expansion_Mh6_Door_2_1_GrayWatcher: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "GrayWatcher";
	};
	class Expansion_Mh6_Door_2_2_GrayWatcher: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "GrayWatcher";
	};
	class ExpansionSpraycanJeans: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Jeans_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Jeans_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Jeans";
	};
	class ExpansionMh6_Jeans: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Jeans";
	};
	class Vehicle_ExpansionMh6_Jeans: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Jeans";
	};
	class Expansion_Mh6_Door_1_1_Jeans: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Jeans";
	};
	class Expansion_Mh6_Door_1_2_Jeans: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Jeans";
	};
	class Expansion_Mh6_Door_2_1_Jeans: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Jeans";
	};
	class Expansion_Mh6_Door_2_2_Jeans: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Jeans";
	};
	class ExpansionSpraycanResistance: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Resistance_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Resistance_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Resistance";
	};
	class ExpansionMh6_Resistance: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Resistance";
	};
	class Vehicle_ExpansionMh6_Resistance: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Resistance";
	};
	class Expansion_Mh6_Door_1_1_Resistance: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Resistance";
	};
	class Expansion_Mh6_Door_1_2_Resistance: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Resistance";
	};
	class Expansion_Mh6_Door_2_1_Resistance: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Resistance";
	};
	class Expansion_Mh6_Door_2_2_Resistance: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Resistance";
	};
	class ExpansionSpraycanShadow: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Shadow_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Shadow_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Shadow";
	};
	class ExpansionMh6_Shadow: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Shadow";
	};
	class Vehicle_ExpansionMh6_Shadow: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Shadow";
	};
	class Expansion_Mh6_Door_1_1_Shadow: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Shadow";
	};
	class Expansion_Mh6_Door_1_2_Shadow: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Shadow";
	};
	class Expansion_Mh6_Door_2_1_Shadow: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Shadow";
	};
	class Expansion_Mh6_Door_2_2_Shadow: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Shadow";
	};
	class ExpansionSpraycanSheriff: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Sheriff_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Sheriff_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Sheriff";
	};
	class ExpansionMh6_Sheriff: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Sheriff";
	};
	class Vehicle_ExpansionMh6_Sheriff: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Sheriff";
	};
	class Expansion_Mh6_Door_1_1_Sheriff: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Sheriff";
	};
	class Expansion_Mh6_Door_1_2_Sheriff: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Sheriff";
	};
	class Expansion_Mh6_Door_2_1_Sheriff: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Sheriff";
	};
	class Expansion_Mh6_Door_2_2_Sheriff: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Sheriff";
	};
	class ExpansionSpraycanSpeedy: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Speedy_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Speedy_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Speedy";
	};
	class ExpansionMh6_Speedy: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Speedy";
	};
	class Vehicle_ExpansionMh6_Speedy: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Speedy";
	};
	class Expansion_Mh6_Door_1_1_Speedy: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Speedy";
	};
	class Expansion_Mh6_Door_1_2_Speedy: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Speedy";
	};
	class Expansion_Mh6_Door_2_1_Speedy: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Speedy";
	};
	class Expansion_Mh6_Door_2_2_Speedy: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Speedy";
	};
	class ExpansionSpraycanSunset: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Sunset_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Sunset_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Sunset";
	};
	class ExpansionMh6_Sunset: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Sunset";
	};
	class Vehicle_ExpansionMh6_Sunset: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Sunset";
	};
	class Expansion_Mh6_Door_1_1_Sunset: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Sunset";
	};
	class Expansion_Mh6_Door_1_2_Sunset: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Sunset";
	};
	class Expansion_Mh6_Door_2_1_Sunset: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Sunset";
	};
	class Expansion_Mh6_Door_2_2_Sunset: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Sunset";
	};
	class ExpansionSpraycanVrana: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Vrana_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Vrana_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Vrana";
	};
	class ExpansionMh6_Vrana: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Vrana";
	};
	class Vehicle_ExpansionMh6_Vrana: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Vrana";
	};
	class Expansion_Mh6_Door_1_1_Vrana: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Vrana";
	};
	class Expansion_Mh6_Door_1_2_Vrana: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Vrana";
	};
	class Expansion_Mh6_Door_2_1_Vrana: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Vrana";
	};
	class Expansion_Mh6_Door_2_2_Vrana: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Vrana";
	};
	class ExpansionSpraycanWasp: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Wasp_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Wasp_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Wasp";
	};
	class ExpansionMh6_Wasp: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Wasp";
	};
	class Vehicle_ExpansionMh6_Wasp: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Wasp";
	};
	class Expansion_Mh6_Door_1_1_Wasp: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Wasp";
	};
	class Expansion_Mh6_Door_1_2_Wasp: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Wasp";
	};
	class Expansion_Mh6_Door_2_1_Wasp: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Wasp";
	};
	class Expansion_Mh6_Door_2_2_Wasp: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Wasp";
	};
	class ExpansionSpraycanWave: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Wave_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Wave_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Wave";
	};
	class ExpansionMh6_Wave: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Wave";
	};
	class Vehicle_ExpansionMh6_Wave: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Wave";
	};
	class Expansion_Mh6_Door_1_1_Wave: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Wave";
	};
	class Expansion_Mh6_Door_1_2_Wave: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Wave";
	};
	class Expansion_Mh6_Door_2_1_Wave: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Wave";
	};
	class Expansion_Mh6_Door_2_2_Wave: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Wave";
	};
	class ExpansionSpraycanBlack: ExpansionSpraycanBase
	{
		scope = 2;
		displayName = "$STR_EXPANSION_SPRAYCAN_Black_NAME";
		descriptionShort = "$STR_EXPANSION_SPRAYCAN_Black_DESC";
		hiddenSelectionsTextures[] = {"DZ\gear\consumables\data\spraycan_black_co.paa"};
		skinName = "Black";
	};
	class ExpansionMh6_Black: ExpansionMh6
	{
		skinBase = "ExpansionMh6";
		skinName = "Black";
	};
	class Vehicle_ExpansionMh6_Black: Vehicle_ExpansionMh6
	{
		skinBase = "Vehicle_ExpansionMh6";
		skinName = "Black";
	};
	class Expansion_Mh6_Door_1_1_Black: Expansion_Mh6_Door_1_1
	{
		skinBase = "Expansion_Mh6_Door_1_1";
		skinName = "Black";
	};
	class Expansion_Mh6_Door_1_2_Black: Expansion_Mh6_Door_1_2
	{
		skinBase = "Expansion_Mh6_Door_1_2";
		skinName = "Black";
	};
	class Expansion_Mh6_Door_2_1_Black: Expansion_Mh6_Door_2_1
	{
		skinBase = "Expansion_Mh6_Door_2_1";
		skinName = "Black";
	};
	class Expansion_Mh6_Door_2_2_Black: Expansion_Mh6_Door_2_2
	{
		skinBase = "Expansion_Mh6_Door_2_2";
		skinName = "Black";
	};
};
class CfgNonAIVehicles
{
	class ProxyVehiclePart;
	class Proxymh6door_1_1: ProxyVehiclePart
	{
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_1_1.p3d";
		inventorySlot = "mh6door_1_1";
	};
	class Proxymh6door_1_2: ProxyVehiclePart
	{
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_1_2.p3d";
		inventorySlot = "mh6door_1_2";
	};
	class Proxymh6door_2_1: ProxyVehiclePart
	{
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_2_1.p3d";
		inventorySlot = "mh6door_2_1";
	};
	class Proxymh6door_2_2: ProxyVehiclePart
	{
		model = "\DayZExpansion\Vehicles\Air\Mh6\proxy\mh6door_2_2.p3d";
		inventorySlot = "mh6door_2_2";
	};
};
