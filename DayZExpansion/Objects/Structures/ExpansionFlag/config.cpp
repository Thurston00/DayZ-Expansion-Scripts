#define _ARMA_

class CfgPatches
{
	class DayZExpansion_Objects_Structures_ExpansionFlag
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {"DZ_Data"};
	};
};
class CfgVehicles
{
	class Inventory_Base;
	class HouseNoDestruct;
	class ExpansionFlagProp: HouseNoDestruct
	{
		scope = 1;
		shadow = 0;
		model = "\DayZExpansion\Objects\Structures\ExpansionFlag\ExpansionFlagProp.p3d";
		vehicleClass = "Expansion_Static";
	};
};
