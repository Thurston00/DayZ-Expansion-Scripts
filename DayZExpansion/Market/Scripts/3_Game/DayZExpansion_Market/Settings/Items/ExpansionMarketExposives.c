/**
 * ExpansionMarketExplosives.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketExplosives: ExpansionMarketCategory
{
	override void Defaults()
	{
		super.Defaults();

		CategoryID = 20;
		DisplayName = ("#STR_EXPANSION_MARKET_CATEGORY_EXPLOSIVES" + " & " + "#STR_EXPANSION_MARKET_CATEGORY_GRENADES");
		m_FileName = "Explosives_And_Grenades";
		
		// EXPLOSIVE GRENADES
		AddItem("RGD5Grenade", 				300,		600,		1,		100);
		AddItem("M67Grenade", 				300,		600,		1,		100);
		
		// FLASHBANGS
		AddItem("FlashGrenade", 			250,		500,		1,		100);

		//Base Raiding
		AddItem("RemoteDetonator", 				300,		600,		1,		100);
		AddItem("RemoteDetonatorTrigger", 		300,		600,		1,		100);
		AddItem("ImprovisedExplosive", 			300,		600,		1,		100);
		AddItem("Plastic_Explosive", 			300,		600,		1,		100);
		AddItem("Grenade_ChemGas", 				300,		600,		1,		100);

		// SMOKE GRANADES
	#ifdef EXPANSIONMODWEAPONS
		AddItem("M18SmokeGrenade_Red", 		200,		400,		1,		100, null, {"M18SmokeGrenade_Green", "M18SmokeGrenade_Yellow", "M18SmokeGrenade_Purple", "M18SmokeGrenade_White", "Expansion_M18SmokeGrenade_Teargas"});
		AddItem("Expansion_M18SmokeGrenade_Teargas", 	260,	520,	1,		100);
	#else
		AddItem("M18SmokeGrenade_Red", 		200,		400,		1,		100, null, {"M18SmokeGrenade_Green", "M18SmokeGrenade_Yellow", "M18SmokeGrenade_Purple", "M18SmokeGrenade_White"});
	#endif
		/*AddItem("M18SmokeGrenade_Green", 	200,		400,		1,		100);
		AddItem("M18SmokeGrenade_Yellow", 	200,		400,		1,		100);
		AddItem("M18SmokeGrenade_Purple", 	200,		400,		1,		100);
		AddItem("M18SmokeGrenade_White", 	200,		400,		1,		100);*/
		AddItem("RDG2SmokeGrenade_Black", 	200,		400,		1,		100, null, {"RDG2SmokeGrenade_White"});
		//AddItem("RDG2SmokeGrenade_White", 	200,		400,		1,		100);
		
	#ifdef EXPANSIONMODMISSIONS
		// Airdrop smoke
		AddItem("ExpansionSupplySignal", 	1000000,	2000000,	1,		100);
	#endif

	#ifdef EXPANSIONMODBASEBUILDING

		// EXPLOSIVE CHARGES
		AddItem("ExpansionSatchel", 		500,		1000,		1,		100);
	#endif

		//! MINES
		AddItem("LandMineTrap", 		500,		1000,		1,		100);
		AddItem("ClaymoreMine", 		500,		1000,		1,		100);
	}
};