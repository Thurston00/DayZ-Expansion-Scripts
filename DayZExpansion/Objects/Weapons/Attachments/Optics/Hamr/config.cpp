class CfgPatches
{
	class DayZExpansion_HAMROptic
	{
		units[]={};
		weapons[]={};
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Weapons_Optics"
		};
	};
};
class cfgVehicles
{
	class ItemOptics_Base;
	class ExpansionHAMROptic: ItemOptics_Base
	{
		scope=2;
		displayName="$STR_EXPANSION_HAMR";
		descriptionShort="$STR_EXPANSION_HAMR_DESC";
		model="DayZExpansion\Objects\Weapons\Attachments\Optics\Hamr\Hamr.p3d";
		animClass="Binoculars";
		rotationFlags=16;
		reversed=0;
		attachments[]=
		{
			"pistolOptics",
			"BatteryD"
		};
		weight=440;
		itemSize[]={2,1};
		inventorySlot="WeaponOptics";
		selectionFireAnim="zasleh";
		simulation="itemoptics";
		dispersionModifier=-0.00015000001;
		dispersionCondition="true";
		recoilModifier[]={1,1,1};
		memoryPointCamera="eyeScope";
		cameraDir="cameraDir";
		hiddenSelections[]=
		{
			"reddot"
		};
		hiddenSelectionsTextures[]=
		{
			""
		};
		s_pipRadius=0.33000001;
		s_pipMagnification=0.30000001;
		s_pipBlur=0.0099999998;
		s_pipChromAber=0.029999999;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=100;
					healthLevels[]=
					{
						
						{
							1,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_clear_ca.paa",
								"DZ\weapons\attachments\optics\data\lensglass_standard.rvmat",
								"DZ\weapons\attachments\data\acog.rvmat"
							}
						},
						
						{
							0.69999999,
							{}
						},
						
						{
							0.5,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_damaged_ca.paa",
								"DZ\weapons\attachments\optics\data\lensglass_standard_damage.rvmat",
								"DZ\weapons\attachments\data\acog_damage.rvmat"
							}
						},
						
						{
							0.30000001,
							{}
						},
						
						{
							0,
							
							{
								"DZ\weapons\attachments\data\scope_alpha_destroyed_ca.paa",
								"DZ\weapons\attachments\optics\data\lensglass_standard_destruct.rvmat",
								"DZ\weapons\attachments\data\acog_destruct.rvmat"
							}
						}
					};
				};
			};
		};
		class OpticsInfo
		{
			memoryPointCamera="eyeScope";
			cameraDir="cameraDir";
			modelOptics="-";
			opticsDisablePeripherialVision=0.67000002;
			opticsFlare=1;
			opticsPPEffects[]={};
			opticsZoomMin="0.3926/4";
			opticsZoomMax="0.3926/4";
			opticsZoomInit="0.3926/4";
			distanceZoomMin=100;
			distanceZoomMax=600;
			discreteDistance[]={100,200,300,400,500,600};
			discreteDistanceInitIndex=0;
			PPMaskProperties[]={0.5,0.5,0.375,0.050000001};
			PPLensProperties[]={0.5,0.15000001,0,0};
			PPBlurProperties=0.60000002;
			opticSightTexture="#(argb,8,8,3)color(1,0,0,1.0,co)";
		};
		class OpticsInfoWeaponOverride
		{
			memoryPointCamera="eyeIronsights";
			cameraDir="cameraDirIronsights";
			opticsZoomMin=0.52359998;
			opticsZoomMax=0.52359998;
			opticsZoomInit=0.52359998;
			distanceZoomMin=200;
			distanceZoomMax=200;
			discreteDistance[]={200,600};
			discreteDistanceInitIndex=0;
			PPMaskProperties[]={0.5,0.5,0.375,0.050000001};
			PPLensProperties[]={0.5,0.15000001,0,0};
			PPBlurProperties=0.60000002;
			PPDOFProperties[]={1,0.1,20,200,4,10};
		};
		class EnergyManager
		{
			hasIcon=1;
			energyUsagePerSecond=0.059999999;
			plugType=1;
			attachmentAction=1;
		};
	};
};
