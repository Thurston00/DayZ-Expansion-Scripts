/**
 * ExpansionMarketMenu.c
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
modded class ExpansionMarketMenu
{
	override bool ShouldShowItemEx(string className)
	{
		if ( !super.ShouldShowItemEx(className) )
			return false;

		auto settings = GetExpansionSettings().GetHardline();

		if (!settings.UseReputation)
			return true;

		if ((ShowPurchasables() && settings.UseItemRarityForMarketPurchase) || (ShowSellables() && settings.UseItemRarityForMarketSell))
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			ExpansionHardlineItemRarity rarity = settings.GetItemRarityByType(className);
			if (rarity && !m_MarketModule.HasRepForRarity(player, rarity))
				return false;
		}

		return true;
	}
};
#endif
