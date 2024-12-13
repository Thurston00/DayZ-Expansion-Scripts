class CfgPatches
{
	class DayZExpansion_Core_Objects_CircuitBoard
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
class CfgSlots
{
	class Slot_Att_ExpansionCircuitBoard
	{
		name="Att_ExpansionCircuitBoard";
		displayName="Circuit Board";
		selection="att_circuit_board";
		ghostIcon="set:expansion_inventory image:circuit_board";
	};
};
class CfgVehicles
{
	class Inventory_Base;
	class ExpansionCircuitBoardBase: Inventory_Base
	{
		scope=1;
		displayName="Circuit Board - Color Base";
		model="DayZExpansion\Core\Objects\CircuitBoard\CircuitBoard.p3d";
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
		};
		weight=180;
		itemSize[]={2,2};
		absorbency=0;
		inventorySlot[]=
		{
			"Att_ExpansionCircuitBoard"
		};
		physLayer="item_small";
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=20;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
							}
						},
						
						{
							0.69999999,
							
							{
								"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
							}
						},
						
						{
							0.5,
							
							{
								"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							
							{
								"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_damage.rvmat"
							}
						},
						
						{
							0,
							
							{
								"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		soundImpactType="plastic";
	};
	class ExpansionCircuitBoard_MK1: ExpansionCircuitBoardBase
	{
		scope=2;
		displayName="Circuit Board - MK1";
		descriptionShort="Used to modify various electronic devices. This is the first generation of a PCB from an unknown device manufacturer. Various variants of these devices were shipped to Chernarus from an undisclosed island between Siberia and Alaska for unknown purposes. The product description number is: NTL-PCB-MK-001";
		model="DayZExpansion\Core\Objects\CircuitBoard\CircuitBoard.p3d";
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
		};
	};
	class ExpansionCircuitBoard_MK2: ExpansionCircuitBoardBase
	{
		scope=2;
		displayName="Circuit Board - MK2";
		descriptionShort="Used to modify various electronic devices. This is the second generation of a PCB from an unknown device manufacturer. Various variants of these devices were shipped to Chernarus from an undisclosed island between Siberia and Alaska for unknown purposes. The product description number is: NTL-PCB-MK-002";
		model="DayZExpansion\Core\Objects\CircuitBoard\CircuitBoard.p3d";
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_mk2_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
		};
	};
	class ExpansionCircuitBoard_MK3: ExpansionCircuitBoardBase
	{
		scope=2;
		displayName="Circuit Board - MK3";
		descriptionShort="Used to modify various electronic devices. This is the second generation of a PCB from an unknown device manufacturer. Various variants of these devices were shipped to Chernarus from an undisclosed island between Siberia and Alaska for unknown purposes. The product description number is: NTL-PCB-MK-003";
		model="DayZExpansion\Core\Objects\CircuitBoard\CircuitBoard.p3d";
		hiddenSelections[]=
		{
			"camo"
		};
		hiddenSelectionsTextures[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board_mk3_co.paa"
		};
		hiddenSelectionsMaterials[]=
		{
			"DayZExpansion\Core\Objects\CircuitBoard\data\circuit_board.rvmat"
		};
	};
};
