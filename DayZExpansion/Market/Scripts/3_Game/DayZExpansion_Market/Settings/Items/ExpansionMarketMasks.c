/**
 * ExpansionMarketMasks.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketMasks: ExpansionMarketCategory
{
	override void Defaults()
	{
		super.Defaults();

		CategoryID = 38;
		DisplayName = "#STR_EXPANSION_MARKET_CATEGORY_MASKS";
		m_FileName = "Masks";

		AddItem("SurgicalMask", 					15,		30,		1,		100);
				
		AddItem("NioshFaceMask", 					40,		80,		1,		100);
		
		AddItem("HockeyMask", 						40,		80,		1,		100);

		AddItem("BalaclavaMask_Beige", 				50,		100,	1,		100, null, {"BalaclavaMask_Blue", "BalaclavaMask_Pink", "BalaclavaMask_White", "BalaclavaMask_Blackskull", "BalaclavaMask_Black", "BalaclavaMask_Green"});
		AddItem("BalaclavaMask_Blackskull",			55,		110,	1,		100);
		AddItem("BalaclavaMask_Black", 				55,		110,	1,		100);
		AddItem("BalaclavaMask_Green", 				60,		120,	1,		100);
				
		AddItem("Balaclava3Holes_Beige", 			50,		100,	1,		100, null, {"Balaclava3Holes_Blue", "Balaclava3Holes_Black", "Balaclava3Holes_Green"});
		AddItem("Balaclava3Holes_Black", 			55,		110,	1,		100);
		AddItem("Balaclava3Holes_Green", 			60,		120,	1,		100);
		
		AddItem("WeldingMask", 						70,		140,	1,		100);
		
		AddItem("GasMask", 							120,	240,	1,		100);
		AddItem("GP5GasMask", 						120,	240,	1,		100);
		AddItem("AirborneMask", 					200,	400,	1,		100);

		AddItem("MimeMask_Female", 					60,		160,	1,		100, null, {"MimeMask_Male"});
	}
};
