/**
 * ExpansionMarketAssaultRifles.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionMarketCrossbows: ExpansionMarketCategory
{
	override void Defaults()
	{
		super.Defaults();
		
		CategoryID = 59;
		DisplayName = "#STR_EXPANSION_MARKET_CATEGORY_CROSSBOWS";
		m_FileName = "Crossbows";
	
		//! 1.21+
		AddItem("crossbow_autumn", 	555, 	930,	1,100,null,{"Crossbow_Black","Crossbow_Summer","Crossbow_Wood"});
	}
};