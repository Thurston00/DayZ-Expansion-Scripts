/**
 * ExpansionMarketMenuItemManagerPresetElement.c
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
modded class ExpansionMarketMenuItemManagerCategoryItem
{
	protected Widget item_element_overlay;
	protected int m_ReqReputation;

	override bool ShowTooltipCheck()
	{
		bool showTooltip = super.ShowTooltipCheck();
		
		if (GetExpansionSettings().GetHardline().UseReputation && GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase)
		{
			string itemTypeName = m_ItemClassName;
			itemTypeName.ToLower();
			
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(itemTypeName);
			if (rarity && !m_MarketMenu.GetMarketModule().HasRepForRarity(player, rarity, m_ReqReputation))
			{
				showTooltip = true;
				item_element_overlay.Show(true);
				item_element_increment.Show(false);
				item_element_decrement.Show(false);
			}
		}
		
		item_element_tooltip.Show(showTooltip);
		return showTooltip;
	}
	
	override void UpdateTooltip()
	{
		if (GetExpansionSettings().GetHardline().UseReputation && GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase)
		{
			if (!m_Tooltip) 
			{
				m_Tooltip = new ExpansionMarketMenuTooltip();
			}
			
			m_Tooltip.SetTitle("#STR_EXPANSION_HARDLINE_MARKET_REPLOW");
			m_Tooltip.SetText(string.Format("#STR_EXPANSION_MARKET_ITEM_REP (%1)", m_ReqReputation.ToString()));
			m_Tooltip.SetContentOffset(0.019531, 0);
			m_Tooltip.Show();
		}
		else
		{
			super.UpdateTooltip();
		}
	}
}
#endif
