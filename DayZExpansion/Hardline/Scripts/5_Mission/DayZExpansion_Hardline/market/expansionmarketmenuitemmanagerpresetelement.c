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
modded class ExpansionMarketMenuItemManagerPresetElement
{	
	override void OnElementButtonClick()
	{
		if (GetExpansionSettings().GetHardline().UseReputation && GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase)
		{
			int i;
			PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
			array<string> removedAtts = new array<string>;
			array<string> atts = m_Preset.GetAttachments();
			array<string> attsNormal = new array<string>;
			atts.Insert(m_Preset.ClassName);
			
			StringLocaliser title = new StringLocaliser("STR_EXPANSION_HARDLINE_MARKET_REPLOW");
			StringLocaliser text;
			
			for (i = 0; i < atts.Count(); i++)
			{
				string att = atts[i];
				attsNormal.Insert(att);
				att.ToLower();
				ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(att);
				if (rarity && !m_MarketMenuItemManager.GetMarketMenu().GetMarketModule().HasRepForRarity(player, rarity))
				{
					removedAtts.Insert(att);
					atts.Remove(i);
				}
			}
			
			if (removedAtts.Count() > 0)
			{
				string removedText;
				for (i = 0; i < removedAtts.Count(); i++)
				{
					string removedAtt = removedAtts[i];
					foreach (string nAtt: attsNormal)
					{
						string attNormal = nAtt;
						nAtt.ToLower();
						if (nAtt == removedAtt)
						{
							string displayName = ExpansionStatic.GetItemDisplayNameWithType(attNormal);
							if (i == 0)
							{
								removedText = displayName;
							}
							else
							{
								removedText = removedText + " | " + displayName;
							}
						}
					}
				}

				text = new StringLocaliser("The following items have been removed from the preset because you don't have the required reputation to use them: " + removedText);
				ExpansionNotification(title, text, EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_INFO, 7, ExpansionNotificationType.MARKET).Create();
			}
		}
		
		super.OnElementButtonClick();
	}
};
#endif
