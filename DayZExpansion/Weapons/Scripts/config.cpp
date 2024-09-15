class CfgPatches
{
	class DayZExpansion_Weapons_Scripts
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DayZExpansion_Core_Scripts"
		};
	};
};
class CfgMods
{
	class DZ_Expansion_Weapons
	{
		dir="DayZExpansion/Weapons";
		credits="$STR_MOD_EXPANSION_AUTHOR";
		extra=0;
		type="mod";
		name="DayZ Expansion - Weapons";
		picture="set:expansion_mod_imageset image:mod_w";
		logo="set:expansion_mod_imageset image:mod_w";
		logoSmall="set:expansion_mod_imageset image:mod_w";
		logoOver="set:expansion_mod_imageset image:mod_w";
		tooltip="DayZ Expansion - Weapons";
		overview="$STR_MOD_EXPANSION_DESC";
		action="";
		author="$STR_MOD_EXPANSION_AUTHOR";
		authorID="";
		expansionSkins[]=
		{
			"DayZExpansion/Skins/Weapons/Explosives/",
			"DayZExpansion/Skins/Weapons/Firearms/",
			"DayZExpansion/Skins/Weapons/Melee/",
			"DayZExpansion/Skins/Weapons/Pistols/",
			"DayZExpansion/Skins/Weapons/Shotguns/",
			"DayZExpansion/Skins/Weapons/Archery/",
			"DayZExpansion/Skins/Weapons/Attachements/"
		};
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
					"DayZExpansion/Weapons/Scripts/Common",
					"DayZExpansion/Weapons/Scripts/1_Core"
				};
			};
			class gameLibScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Weapons/Scripts/Common",
					"DayZExpansion/Weapons/Scripts/2_GameLib"
				};
			};
			class gameScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Weapons/Scripts/Common",
					"DayZExpansion/Weapons/Scripts/3_Game"
				};
			};
			class worldScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Weapons/Scripts/Common",
					"DayZExpansion/Weapons/Scripts/4_World"
				};
			};
			class missionScriptModule
			{
				value="";
				files[]=
				{
					"DayZExpansion/Weapons/Scripts/Common",
					"DayZExpansion/Weapons/Scripts/5_Mission"
				};
			};
		};
	};
};
