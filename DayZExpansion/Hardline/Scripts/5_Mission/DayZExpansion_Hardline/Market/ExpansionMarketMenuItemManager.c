/**
 * ExpansionMarketMenuItemManager.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODMARKET
modded class ExpansionMarketMenuItemManager
{
	override void AddCategoryEx(string title, TStringArray atts, string icon)
	{
		if (GetExpansionSettings().GetHardline().UseReputation && (GetExpansionSettings().GetHardline().UseItemRarityForMarketSell || GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase))
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			for (int i = 0; i < atts.Count(); i++)
			{
				string att = atts[i];
				att.ToLower();
				ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(att);
				if (rarity && !m_MarketMenu.GetMarketModule().HasRepForRarity(player, rarity))
					atts.Remove(i);
			}
		}
		
		super.AddCategoryEx(title, atts, icon);
	}
};
#endif
