/**
 * ExpansionMarketFilters.c
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
modded class ExpansionMarketFilters
{
	override void GetAttachmentsByClassNameAndTypesEx(ExpansionMarketOutput output, array<ExpansionMarketAttachType> attachTypes, TStringArray attachments)
	{
		super.GetAttachmentsByClassNameAndTypesEx(output, attachTypes, attachments);
		
		auto settings = GetExpansionSettings().GetHardline();

		if (!settings.UseReputation)
			return;

		if ((m_Menu.ShowPurchasables() && settings.UseItemRarityForMarketPurchase) || (m_Menu.ShowSellables() && settings.UseItemRarityForMarketSell))
		{
			for (int i = 0; i < attachments.Count(); i++)
			{
				PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
				ExpansionHardlineItemRarity rarity = settings.GetItemRarityByType(attachments[i]);
				if (rarity && !m_MarketModule.HasRepForRarity(player, rarity))
					attachments.Remove(i);
			}
		}
	}
};
#endif
