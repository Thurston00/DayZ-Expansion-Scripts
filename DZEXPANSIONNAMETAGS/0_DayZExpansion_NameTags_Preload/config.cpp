		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {};
	};
};
class CfgMods
{
	class DZ_Expansion_NameTags_Preload
	{
		type = "mod";
		dependencies[] = {"Game","World","Mission"};
		class defs
		{
			class engineScriptModule
			{
				files[] = {"0_DayZExpansion_NameTags_Preload/Common","0_DayZExpansion_NameTags_Preload/1_Core"};
			};
			class gameLibScriptModule
			{
				files[] = {"0_DayZExpansion_NameTags_Preload/Common","0_DayZExpansion_NameTags_Preload/2_GameLib"};
			};
			class gameScriptModule
			{
				files[] = {"0_DayZExpansion_NameTags_Preload/Common","0_DayZExpansion_NameTags_Preload/3_Game"};
			};
			class worldScriptModule
			{
				files[] = {"0_DayZExpansion_NameTags_Preload/Common","0_DayZExpansion_NameTags_Preload/4_World"};
			};
			class missionScriptModule
			{
				files[] = {"0_DayZExpansion_NameTags_Preload/Common","0_DayZExpansion_NameTags_Preload/5_Mission"};
			};
		};
	};
};