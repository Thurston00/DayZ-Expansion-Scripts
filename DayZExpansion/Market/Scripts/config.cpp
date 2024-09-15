class CfgPatches
{
	class DayZExpansion_Market_Scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Scripts",
			"DayZExpansion_Core_Scripts"
		};
	};
};
class CfgMods
{
	class DZ_Expansion_Market
	{
		dir="DayZExpansion/Market";
		credits="$STR_MOD_EXPANSION_AUTHOR";
		extra=0;
		type="mod";
		name="DayZ Expansion - Market";
		picture="set:expansion_mod_imageset image:mod_m";
		logo="set:expansion_mod_imageset image:mod_m";
		logoSmall="set:expansion_mod_imageset image:mod_m";
		logoOver="set:expansion_mod_imageset image:mod_m";
		tooltip="DayZ Expansion - Market";
		overview="$STR_MOD_EXPANSION_DESC";
		action="";
		author="$STR_MOD_EXPANSION_AUTHOR";
		authorID="";
		dependencies[]=
		{
			"Game",
			"World",
			"Mission"
		};
		class defs
		{
			class widgetStyles
			{
				files[]={};
			};
			class imageSets
			{
				files[]={};
			};
			class engineScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Market/Scripts/Common",
					"DayZExpansion/Market/Scripts/1_Core"
				};
			};
			class gameLibScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Market/Scripts/Common",
					"DayZExpansion/Market/Scripts/2_GameLib"
				};
			};
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Market/Scripts/Common",
					"DayZExpansion/Market/Scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Market/Scripts/Common",
					"DayZExpansion/Market/Scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Market/Scripts/Common",
					"DayZExpansion/Market/Scripts/5_Mission"
				};
			};
		};
	};
};
