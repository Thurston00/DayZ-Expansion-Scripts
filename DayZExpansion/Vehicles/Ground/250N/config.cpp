class CfgPatches
{
	class DayZExpansion_Vehicles_Ground_250N
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DayZExpansion_Vehicles_Data"
		};
	};
};
class CfgSlots
{
	class Slot_Expansion_250N_wheel_1
	{
		name="expansion_250N_wheel_1";
		displayName="$STR_EXPANSION_VEHICLE_250N_WHEEL";
		selection="wheel_1";
		ghostIcon="wheel";
	};
	class Slot_Expansion_250N_wheel_2
	{
		name="expansion_250N_wheel_2";
		displayName="$STR_EXPANSION_VEHICLE_250N_WHEEL";
		selection="wheel_2";
		ghostIcon="wheel";
	};
};
class CfgVehicles
{
	class ExpansionWheelBase;
	class ExpansionVehicleBikeBase;
	class Inventory_Base;
	class SimulationModule;
	class Axles;
	class Front;
	class Wheels;
	class Rear;
	class Left;
	class Right;
	class AnimationSources;
	class Crew;
	class Driver;
	class Window;
	class DamageSystem;
	class GlobalHealth;
	class DamageZones;
	class Doors;
	class Health;
	class Vehicle_Expansion250NWheel: ExpansionWheelBase
	{
		scope=0;
		displayName="$STR_EXPANSION_VEHICLE_250N_WHEEL";
		descriptionShort="$STR_EXPANSION_VEHICLE_250N_WHEEL_DESC";
		model="\DayZExpansion\Vehicles\Ground\250N\proxy\Expansion_250N_wheel_front.p3d";
		itemSize[]={6,6};
		inventorySlot[]=
		{
			"Expansion_250N_wheel_1",
			"Expansion_250N_wheel_2"
		};
		rotationFlags=4;
		radiusByDamage[]={0,0.28,0.31,0.28,0.99980003,0.25,0.99989998,0.2};
		radius=0.38;
		width=0.16;
		mass=20;
	};
	class Vehicle_Expansion250N: ExpansionVehicleBikeBase
	{
		scope=0;
		displayname="$STR_EXPANSION_VEHICLE_250N";
		model="\DayZExpansion\Vehicles\Ground\250N\250N.p3d";
		attachments[]=
		{
			"Expansion_250N_wheel_1",
			"Expansion_250N_wheel_2",
			"Reflector_1_1",
			"CarBattery",
			"SparkPlug"
		};
		fuelCapacity=20;
		fuelConsumption=15;
		batterySlot="CarBattery";
		electricPowerResName="power";
		electricConsumptionIgnition=3001;
		electricConsumptionEngine=0;
		electricConsumptionLights=0;
		electricOutputEngine=5;
		hiddenSelections[]=
		{
			"light_left",
			"light_right",
			"light_brake_1_2",
			"light_brake_2_2",
			"light_reverse_1_2",
			"light_reverse_2_2",
			"light_1_2",
			"light_2_2",
			"light_dashboard",
			"zbytek"
		};
		hiddenSelectionsTextures[]=
		{
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"DayZExpansion\Vehicles\Ground\250N\data\old_moto_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"",
			"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
		};
		frontReflectorMatOn="dz\vehicles\wheeled\Truck_01\data\Truck_01_cab_lights.rvmat";
		frontReflectorMatOff="DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat";
		brakeReflectorMatOn="dz\vehicles\wheeled\Truck_01\data\Truck_01_cab_lights.rvmat";
		brakeReflectorMatOff="DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat";
		ReverseReflectorMatOn="";
		ReverseReflectorMatOff="";
		TailReflectorMatOn="dz\vehicles\wheeled\Truck_01\data\Truck_01_cab_lights.rvmat";
		TailReflectorMatOff="DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat";
		soundController="Expansion_SoundController_250N";
		class Crew
		{
			class Driver
			{
				isDriver=1;
				actionSel="seat_driver";
				proxyPos="crewDriver";
				getInPos="pos_driver";
				getInDir="pos_driver_dir";
			};
			class CoDriver
			{
				actionSel="seat_codriver";
				proxyPos="crewCoDriver";
				getInPos="pos_codriver";
				getInDir="pos_codriver_dir";
			};
		};
		class SimulationModule
		{
			class Steering
			{
				increaseSpeed[]={0,45,60,23,100,12};
				decreaseSpeed[]={0,80,60,40,90,20};
				centeringSpeed[]={0,0,15,25,60,40,100,60};
			};
			class Throttle
			{
				reactionTime=1;
				defaultThrust=0.85000002;
				gentleThrust=0.69999999;
				turboCoef=4;
				gentleCoef=0.75;
			};
			braking[]={0,0.1,1,0.80000001,2.5,0.89999998,3,1};
			class Engine
			{
				inertia=0.15000001;
				torqueMax=130;
				torqueRpm=3700;
				powerMax=80;
				powerRpm=3500;
				rpmIdle=650;
				rpmMin=700;
				rpmClutch=1500;
				rpmRedline=4000;
				rpmMax=5000;
			};
			class Gearbox
			{
				reverse=3.51;
				ratios[]={3.5,2.26,1.45,1};
				timeToUncoupleClutch=0.30000001;
				timeToCoupleClutch=0.44999999;
				maxClutchTorque=260;
			};
			class Axles
			{
				class Front
				{
					maxSteeringAngle=45;
					finalRatio=0;
					brakeBias=0.69999999;
					brakeForce=4000;
					wheelHubMass=10;
					wheelHubRadius=0.15000001;
					class Suspension
					{
						swayBar=0;
						stiffness=15000;
						compression=3100;
						damping=4700;
						travelMaxUp=0.16;
						travelMaxDown=0.16;
					};
					class Wheels
					{
						class Center
						{
							inventorySlot="expansion_250n_wheel_1";
							animTurn="turnfront";
							animRotation="wheelfront";
							animDamper="damper_1";
							wheelHub="wheel_1_damper_land";
							axis_start="suspension_1_start";
							axis_end="suspension_1_end";
						};
					};
				};
				class Rear
				{
					maxSteeringAngle=0;
					finalRatio=3;
					brakeBias=0.69999999;
					brakeForce=4000;
					wheelHubMass=10;
					wheelHubRadius=0.15000001;
					class Suspension
					{
						swayBar=0;
						stiffness=15000;
						compression=3000;
						damping=4500;
						travelMaxUp=0.16;
						travelMaxDown=0.16;
					};
					class Wheels
					{
						class Center
						{
							inventorySlot="expansion_250n_wheel_2";
							animTurn="";
							animRotation="wheelback";
							animDamper="damper_2";
							wheelHub="wheel_2_damper_land";
						};
					};
				};
			};
		};
		class AnimationSources
		{
			class Damper_1
			{
				source="user";
				initPhase=0;
				animPeriod=9.9999997e-06;
			};
			class Damper_2: Damper_1
			{
			};
			class wheelfront
			{
				source="user";
				initPhase=0;
				animPeriod=9.9999997e-06;
			};
			class wheelback: wheelfront
			{
			};
			class DrivingWheel
			{
				source="user";
				initPhase=0;
				animPeriod=9.9999997e-06;
			};
		};
		class Cargo
		{
			itemsCargoSize[]={4,4};
			allowOwnedCargoManipulation=1;
			openable=0;
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
			class DamageZones
			{
				class Chassis
				{
					class Health
					{
						hitpoints=400;
						transferToGlobalCoef=0;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.69999999,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.5,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_destruct.rvmat"
								}
							}
						};
					};
					componentNames[]=
					{
						"dmgZone_chassis"
					};
					fatalInjuryCoef=-1;
					inventorySlots[]=
					{
						"Engine",
						"SparkPlug",
						"CarBattery"
					};
				};
				class Front
				{
					class Health
					{
						hitpoints=200;
						transferToGlobalCoef=0;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.69999999,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.5,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_destruct.rvmat"
								}
							}
						};
					};
					transferToZonesNames[]=
					{
						"Engine",
						"dmgZone_light_front"
					};
					transferToZonesThreshold[]={0.5,0.5};
					transferToZonesCoefs[]={0.69999999,0.69999999};
					memoryPoints[]=
					{
						"dmgZone_front"
					};
					componentNames[]=
					{
						"dmgZone_front"
					};
					fatalInjuryCoef=-1;
					inventorySlotsCoefs[]={0.30000001,0.30000001};
					inventorySlots[]=
					{
						"Reflector_1_1"
					};
				};
				class Reflector_1_1
				{
					class Health
					{
						hitpoints=10;
						transferToGlobalCoef=0;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"dz\vehicles\wheeled\offroadhatchback\data\headlights_glass.rvmat"
								}
							},
							
							{
								0.69999999,
								{}
							},
							
							{
								0.5,
								
								{
									"dz\vehicles\wheeled\offroadhatchback\data\glass_i_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								{}
							},
							
							{
								0,
								
								{
									"dz\vehicles\wheeled\offroadhatchback\data\glass_i_destruct.rvmat"
								}
							}
						};
					};
					transferToZonesNames[]=
					{
						"Front"
					};
					transferToZonesCoefs[]={0.1};
					memoryPoints[]=
					{
						"dmgZone_light_front"
					};
					componentNames[]=
					{
						"dmgZone_light_front"
					};
					fatalInjuryCoef=-1;
					inventorySlotsCoefs[]={1,1};
					inventorySlots[]=
					{
						"Reflector_1_1"
					};
				};
				class Back
				{
					class Health
					{
						hitpoints=200;
						transferToGlobalCoef=0;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.69999999,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.5,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_destruct.rvmat"
								}
							}
						};
					};
					transferToZonesNames[]=
					{
						"chassis"
					};
					transferToZonesCoefs[]={0.30000001};
					memoryPoints[]=
					{
						"dmgZone_back"
					};
					componentNames[]=
					{
						"dmgZone_back"
					};
					fatalInjuryCoef=-1;
					inventorySlotsCoefs[]={0.5};
					inventorySlots[]={};
				};
				class Engine
				{
					class Health
					{
						hitpoints=500;
						transferToGlobalCoef=1;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.69999999,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.5,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_destruct.rvmat"
								}
							}
						};
					};
					memoryPoints[]=
					{
						"dmgZone_engine"
					};
					componentNames[]=
					{
						"dmgZone_engine"
					};
					fatalInjuryCoef=0.001;
					inventorySlotsCoefs[]={0.2};
					inventorySlots[]=
					{
						"Engine",
						"SparkPlug",
						"CarBattery"
					};
				};
				class FuelTank
				{
					class Health
					{
						hitpoints=200;
						transferToGlobalCoef=0;
						healthLevels[]=
						{
							
							{
								1,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.69999999,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto.rvmat"
								}
							},
							
							{
								0.5,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0.30000001,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_damage.rvmat"
								}
							},
							
							{
								0,
								
								{
									"DayZExpansion\Vehicles\Ground\250N\data\old_moto_destruct.rvmat"
								}
							}
						};
					};
					componentNames[]=
					{
						"dmgZone_fuelTank"
					};
					fatalInjuryCoef=-1;
					inventorySlots[]={};
				};
			};
		};
		class GUIInventoryAttachmentsProps
		{
			class Engine
			{
				name="$STR_attachment_Engine0";
				description="";
				icon="cat_vehicle_engine";
				attachmentSlots[]=
				{
					"CarBattery",
					"SparkPlug"
				};
			};
			class Body
			{
				name="$STR_attachment_Body0";
				description="";
				icon="cat_vehicle_body";
				attachmentSlots[]=
				{
					"Reflector_1_1"
				};
			};
			class Chassis
			{
				name="$STR_attachment_Chassis0";
				description="";
				icon="cat_vehicle_chassis";
				attachmentSlots[]=
				{
					"Expansion_250N_wheel_1",
					"Expansion_250N_wheel_2"
				};
			};
		};
	};
};
class CfgNonAIVehicles
{
	class ProxyAttachment;
	class ProxyVehiclePart: ProxyAttachment
	{
		scope=2;
		simulation="ProxyInventory";
		autocenter=0;
		animated=0;
		shadow=1;
		reversed=0;
	};
	class ProxyExpansion_250N_wheel_front: ProxyVehiclePart
	{
		model="\DayZExpansion\Vehicles\Ground\250N\proxy\Expansion_250N_wheel_front.p3d";
		inventorySlot[]=
		{
			"Expansion_250N_wheel__1"
		};
	};
	class ProxyExpansion_250N_wheel_front_destroyed: ProxyVehiclePart
	{
		model="\DayZExpansion\Vehicles\Ground\250N\proxy\Expansion_250N_wheel_front.p3d";
		inventorySlot[]=
		{
			"Expansion_250N_wheel_1"
		};
	};
	class ProxyExpansion_250N_wheel_back: ProxyVehiclePart
	{
		model="\DayZExpansion\Vehicles\Ground\250N\proxy\Expansion_250N_wheel_back.p3d";
		inventorySlot[]=
		{
			"Expansion_250N_wheel_2"
		};
	};
	class ProxyExpansion_250N_wheel_back_destroyed: ProxyVehiclePart
	{
		model="\DayZExpansion\Vehicles\Ground\250N\proxy\Expansion_250N_wheel_back.p3d";
		inventorySlot[]=
		{
			"Expansion_250N_wheel_2"
		};
	};
};
