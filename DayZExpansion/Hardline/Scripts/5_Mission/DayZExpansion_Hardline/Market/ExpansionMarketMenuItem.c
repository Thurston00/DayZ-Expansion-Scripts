/**
 * ExpansionMarketMenuItem.c
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
modded class ExpansionMarketMenuItem
{
	override void OnSetView()
	{
		super.OnSetView();
		
		if (GetExpansionSettings().GetHardline().UseReputation && (GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase || GetExpansionSettings().GetHardline().UseItemRarityForMarketSell))
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(GetMarketItem().ClassName);
			if (rarity && !m_MarketModule.HasRepForRarity(player, rarity))
			{
				if (GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase)
					m_HasRepBuy = false;
				
				if (GetExpansionSettings().GetHardline().UseItemRarityForMarketSell)
					m_HasRepSell = false;
			}
		}
		
	#ifdef EXPANSIONMODMARKET_DEBUG
		EXLogPrint(string.Format("%1::SetView - Has rep for item %2: Has rep to buy: %3 | Has rep to sell: %4 | Can buy: %5 | Can sell: %6", ToString(), GetMarketItem().ClassName, m_HasRepBuy.ToString(), m_HasRepSell.ToString(), m_CanBuy.ToString(), m_CanSell.ToString()));
	#endif
	}

	override void UpdatePreviewObject()
	{
		if (GetExpansionSettings().GetHardline().UseReputation && (GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase || GetExpansionSettings().GetHardline().UseItemRarityForMarketSell))
		{
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			for (int i = 0; i < GetMarketItem().SpawnAttachments.Count(); i++)
			{
				ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(GetMarketItem().SpawnAttachments[i]);
				if (rarity && !m_MarketModule.HasRepForRarity(player, rarity))
					GetMarketItem().SpawnAttachments.Remove(i);
			}
		}

		super.UpdatePreviewObject();
	}
};
#endif
