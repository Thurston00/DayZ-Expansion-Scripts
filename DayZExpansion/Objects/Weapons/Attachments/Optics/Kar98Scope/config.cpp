class CfgPatches
{
	class DayZExpansion_Kar98ScopeOptic
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
	class ExpansionKar98ScopeOptic: ItemOptics_Base
	{
		scope=2;
		displayName="$STR_KAR98_SCOPE";
		descriptionShort="$STR_KAR98_SCOPE_DESC";
		model="\DayZExpansion\Objects\Weapons\Attachments\Optics\Kar98Scope\karscope.p3d";
		animClass="Binoculars";
		rotationFlags=16;
		reversed=0;
		weight=800;
		itemSize[]={3,1};
		inventorySlot[]=
		{
			"ExpansionKar98Optics"
		};
		simulation="itemoptics";
		dispersionModifier=-0.0015;
		dispersionCondition="true";
		recoilModifier[]={1,1,1};
		swayModifier[]={1,1,1};
		memoryPointCamera="eyeScope";
		cameraDir="cameraDir";
		s_pipRadius=0.30000001;
		s_pipMagnification=0.40000001;
		s_pipBlur=0.0099999998;
		s_pipChromAber=0.029999999;
		class DamageSystem
		{
			class GlobalHealth
			{
				class Health
				{
					hitpoints=50;
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
		class OpticsInfo
		{
			memoryPointCamera="eyeScope";
			cameraDir="cameraDir";
			modelOptics="-";
			opticsDisablePeripherialVision=0.67000002;
			opticsFlare=1;
			opticsPPEffects[]=
			{
				"OpticsCHAbera3",
				"OpticsBlur1"
			};
			opticsZoomMin="0.3926/6";
			opticsZoomMax="0.3926/6";
			opticsZoomInit="0.3926/6";
			distanceZoomMin=100;
			distanceZoomMax=1300;
			discreteDistance[]={100,200,300,400,500,600,700,800,900,1000,1100,1200,1300};
			discreteDistanceInitIndex=0;
			PPMaskProperties[]={0.5,0.5,0.34999999,0.050000001};
			PPLensProperties[]={0.5,0.15000001,0,0};
			PPBlurProperties=0.60000002;
		};
	};
};
class CfgSlots
{
	class Slot_ExpansionKar98Optics
	{
		name="ExpansionKar98Optics";
		displayName="$STR_KAR98_SCOPE";
		ghostIcon="weaponoptics";
	};
};
class CfgNonAIVehicles
{
	class ProxyAttachment;
	class Proxykarscope: ProxyAttachment
	{
		scope=2;
		inventorySlot="ExpansionKar98Optics";
		model="DayZExpansion\Objects\Weapons\Attachments\Optics\Kar98Scope\karscope.p3d";
	};
};
