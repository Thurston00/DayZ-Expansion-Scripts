/**
 * ExpansionMarketVehicleParts.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketVehicleParts: ExpansionMarketCategory
{
	override void Defaults()
	{
		super.Defaults();

		CategoryID = 57;
		DisplayName = "#STR_EXPANSION_MARKET_CATEGORY_VEHICLE_PARTS";
		m_FileName = "Vehicle_Parts";

		AddItem("HeadlightH7_Box", 							30,		60,		1,		100);
		AddItem("HeadlightH7", 								30,		60,		1,		100);
		AddItem("CarRadiator", 								200,	400,	1,		100);

		AddItem("TireRepairKit", 							220,	440,	1,		100);
		AddItem("SparkPlug", 								60,		120,	1,		100);
		AddItem("GlowPlug", 								60,		120,	1,		100);
	#ifdef EXPANSIONMODVEHICLE
		AddItem("ExpansionIgniterPlug", 					60,		120,	1,		100);
		AddItem("ExpansionHydraulicHoses", 					250,	500,	1,		100);
	#endif

		AddItem("HatchbackHood",		 					300,	600,	1,		100, null, {"HatchbackHood_Blue", "HatchbackHood_White"});

		AddItem("HatchbackTrunk",		 					380,	760,	1,		100, null, {"HatchbackTrunk_Blue", "HatchbackTrunk_White"});

		AddItem("HatchbackDoors_Driver",		 			450,	900,	1,		100, null, {"HatchbackDoors_Driver_Blue", "HatchbackDoors_Driver_White"});

		AddItem("HatchbackDoors_CoDriver",		 			450,	900,	1,		100, null, {"HatchbackDoors_CoDriver_Blue", "HatchbackDoors_CoDriver_White"});
		
		AddItem("HatchbackWheel", 							500,	1000,	1,		100);
	
		AddItem("Hatchback_02_Hood",		 				300,	600,	1,		100, null, {"Hatchback_02_Hood_Black", "Hatchback_02_Hood_Blue"});

		AddItem("Hatchback_02_Trunk",		 				380,	760,	1,		100, null, {"Hatchback_02_Trunk_Black", "Hatchback_02_Trunk_Blue"});

		AddItem("Hatchback_02_Door_1_1",		 			450,	900,	1,		100, null, {"Hatchback_02_Door_1_1_Black", "Hatchback_02_Door_1_1_Blue"});

		AddItem("Hatchback_02_Door_1_2",		 			450,	900,	1,		100, null, {"Hatchback_02_Door_1_2_Black", "Hatchback_02_Door_1_2_Blue"});

		AddItem("Hatchback_02_Door_2_1",		 			450,	900,	1,		100, null, {"Hatchback_02_Door_2_1_Black", "Hatchback_02_Door_2_1_Blue"});

		AddItem("Hatchback_02_Door_2_2",		 			450,	900,	1,		100, null, {"Hatchback_02_Door_2_2_Black", "Hatchback_02_Door_2_2_Blue"});
		
		AddItem("Hatchback_02_Wheel", 						500,	1000,	1,		100);
		
		AddItem("CivSedanHood",		 						400,	800,	1,		100, null, {"CivSedanHood_Wine", "CivSedanHood_Black"});
		
		AddItem("CivSedanTrunk",		 					450,	900,	1,		100, null, {"CivSedanTrunk_Wine", "CivSedanTrunk_Black"});

		AddItem("CivSedanDoors_Driver",		 				500,	1000,	1,		100, null, {"CivSedanDoors_Driver_Wine", "CivSedanDoors_Driver_Black"});
		
		AddItem("CivSedanDoors_CoDriver",		 			500,	1000,	1,		100, null, {"CivSedanDoors_CoDriver_Wine", "CivSedanDoors_CoDriver_Black"});

		AddItem("CivSedanDoors_BackLeft",					500,	1000,	1,		100, null, {"CivSedanDoors_BackLeft_Wine", "CivSedanDoors_BackLeft_Black"});

		AddItem("CivSedanDoors_BackRight",					500,	1000,	1,		100, null, {"CivSedanDoors_BackRight_Wine", "CivSedanDoors_BackRight_Black"});
		
		AddItem("CivSedanWheel", 							600,	1200,	1,		100);
	
		AddItem("Sedan_02_Hood",		 					300,	600,	1,		100, null, {"Sedan_02_Hood_Red", "Sedan_02_Hood_Grey"});

		AddItem("Sedan_02_Trunk",		 					450,	900,	1,		100, null, {"Sedan_02_Trunk_Red", "Sedan_02_Trunk_Grey"});

		AddItem("Sedan_02_Door_1_1",		 				450,	900,	1,		100, null, {"Sedan_02_Door_1_1_Red", "Sedan_02_Door_1_1_Grey"});

		AddItem("Sedan_02_Door_1_2",		 				450,	900,	1,		100, null, {"Sedan_02_Door_1_2_Red", "Sedan_02_Door_1_2_Grey"});
		
		AddItem("Sedan_02_Door_2_1",		 				450,	900,	1,		100, null, {"Sedan_02_Door_2_1_Red", "Sedan_02_Door_2_1_Grey"});

		AddItem("Sedan_02_Door_2_2",		 				450,	900,	1,		100, null, {"Sedan_02_Door_2_2_Red", "Sedan_02_Door_2_2_Grey"});
		
		AddItem("Sedan_02_Wheel", 							500,	1000,	1,		100);
	
		AddItem("Truck_01_Hood",		 					400,	800,	1,		100, null, {"Truck_01_Hood_Blue", "Truck_01_Hood_Orange"});
		
		AddItem("Truck_01_Door_1_1",		 				500,	1000,	1,		100, null, {"Truck_01_Door_1_1_Blue", "Truck_01_Door_1_1_Orange"});

		AddItem("Truck_01_Door_2_1",		 				500,	1000,	1,		100, null, {"Truck_01_Door_2_1_Blue", "Truck_01_Door_2_1_Orange"});
		
		AddItem("Truck_01_Wheel", 							600,	1200,	1,		100);
		AddItem("Truck_01_WheelDouble", 					1000,	2000,	1,		100);
	
		AddItem("Offroad_02_Hood",		 				300,	600,	1,		100);

		AddItem("Offroad_02_Trunk",		 				380,	760,	1,		100);

		AddItem("Offroad_02_Door_1_1",		 			450,	900,	1,		100);

		AddItem("Offroad_02_Door_1_2",		 			450,	900,	1,		100);

		AddItem("Offroad_02_Door_2_1",		 			450,	900,	1,		100);

		AddItem("Offroad_02_Door_2_2",		 			450,	900,	1,		100);
		
		AddItem("Offroad_02_Wheel", 					500,	1000,	1,		100);
		//! Vanilla - Not released yet
		//AddItem("Truck_02_Hood",		 					400,	800,	1,		100);
		//AddItem("Truck_02_Door_1_1",		 				500,	1000,	1,		100);
		//AddItem("Truck_02_Door_1_2",		 				500,	1000,	1,		100);
		//AddItem("Truck_02_Wheel", 						600,	1200,	1,		100);
	
		//AddItem("Van_01_Trunk_1",		 					450,	900,	1,		100);
		//AddItem("Van_01_Trunk_2",		 					450,	900,	1,		100);
		//AddItem("Van_01_Door_1_1",		 				500,	1000,	1,		100);
		//AddItem("Van_01_Door_1_2",		 				500,	1000,	1,		100);
		//AddItem("Van_01_Door_2_1",		 				500,	1000,	1,		100);
		//AddItem("Van_01_Door_2_2",		 				500,	1000,	1,		100);
		//AddItem("Van_01_Wheel", 							600,	1200,	1,		100);
	
	#ifdef EXPANSIONMODVEHICLE
		AddItem("ExpansionTractorDoorsDriver", 				400,	800,	1,		100);
		AddItem("ExpansionTractorDoorsCodriver",			400,	800,	1,		100);
		AddItem("ExpansionTractorBackWheel", 				500,	1000,	1,		100);
		AddItem("ExpansionTractorFrontWheel", 				500,	1000,	1,		100);

		AddItem("ExpansionUAZDoorHood", 					400,	800,	1,		100);
		AddItem("ExpansionUAZDoorDriver", 					500,	1000,	1,		100);
		AddItem("ExpansionUAZDoorCoDriver", 				500,	1000,	1,		100);
		AddItem("ExpansionUAZDoorCargo1", 					500,	1000,	1,		100);
		AddItem("ExpansionUAZDoorCargo2", 					500,	1000,	1,		100);
		AddItem("ExpansionUAZWheel", 						600,	1200,	1,		100);

		AddItem("Expansion_Landrover_Hood", 				400,	800,	1,		100);
		AddItem("Expansion_Landrover_Driverdoor", 			500,	1000,	1,		100);
		AddItem("Expansion_Landrover_Codriverdoor", 		500,	1000,	1,		100);
		AddItem("Expansion_Landrover_Left", 				500,	1000,	1,		100);
		AddItem("Expansion_Landrover_Right", 				500,	1000,	1,		100);
		AddItem("Expansion_Landrover_Trunk", 				400,	800,	1,		100);
		AddItem("Expansion_Landrover_Wheel", 				600,	1200,	1,		100);
	
		AddItem("ExpansionVodnikDoorDriver", 				500,	1000,	1,		100);
		AddItem("ExpansionVodnikDoorCoDriver", 				500,	1000,	1,		100);
		AddItem("ExpansionVodnikWheel", 					600,	1200,	1,		100);

		AddItem("ExpansionMerlinBackWheel", 				600,	1200,	1,		100);
		AddItem("ExpansionMerlinFrontWheel", 				600,	1200,	1,		100);

		AddItem("ExpansionUh1hDoor_1_1", 					500,	1000,	1,		100);
		AddItem("ExpansionUh1hDoor_1_2",					500,	1000,	1,		100);
		AddItem("ExpansionUh1hDoor_2_1", 					500,	1000,	1,		100);
		AddItem("ExpansionUh1hDoor_2_2", 					500,	1000,	1,		100);

		AddItem("Expansion_Mh6_Door_1_1", 					500,	1000,	1,		100, null, {"Expansion_Mh6_Door_1_1_Black"});
		AddItem("Expansion_Mh6_Door_1_2",					500,	1000,	1,		100, null, {"Expansion_Mh6_Door_1_2_Black"});
		AddItem("Expansion_Mh6_Door_2_1", 					500,	1000,	1,		100, null, {"Expansion_Mh6_Door_2_1_Black"});
		AddItem("Expansion_Mh6_Door_2_2",					500,	1000,	1,		100, null, {"Expansion_Mh6_Door_2_2_Black"});
		
		AddItem("ExpansionBusWheel",						600,	1200,	1,		100);
		AddItem("ExpansionBusWheelDouble",					600,	1200,	1,		100);
		
		AddItem("ExpansionCarKey",							250,	500,	1,		100);
		
		AddItem("ExpansionKeyChain_Red",					100,	200,	1,		100, null, {"ExpansionKeyChain_White", "ExpansionKeyChain_Black", "ExpansionKeyChain_Blue", "ExpansionKeyChain_Green", "ExpansionKeyChain_Grey", "ExpansionKeyChain_Orange", "ExpansionKeyChain_Pink", "ExpansionKeyChain_Purple", "ExpansionKeyChain_Yellow"});
	#endif
	}
};