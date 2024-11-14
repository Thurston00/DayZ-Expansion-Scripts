/**
 * ExpansionMarketModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODMARKET
modded class ExpansionMarketModule
{
	override bool CheckCanUseTrader(PlayerBase player, ExpansionTraderObjectBase trader)
	{
		if (!super.CheckCanUseTrader(player, trader))
			return false;

		if (GetGame().IsServer() && GetExpansionSettings().GetHardline().UseReputation)
		{
			int minRep = trader.GetTraderMarket().MinRequiredReputation;
			int maxRep = trader.GetTraderMarket().MaxRequiredReputation;
			if (!Math.IsInRangeInt(player.Expansion_GetReputation(), minRep, maxRep))
			{
				if (player.GetIdentity())
				{
					if (maxRep != int.MAX)
						ExpansionNotification("STR_EXPANSION_HARDLINE_REPUTATION_OUTOFRANGE", new StringLocaliser("STR_EXPANSION_HARDLINE_REPUTATION_OUTOFRANGE_MINMAX_TRADER", minRep.ToString()), EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.MARKET).Create(player.GetIdentity());
					else
						ExpansionNotification("STR_EXPANSION_HARDLINE_REPUTATION_OUTOFRANGE", new StringLocaliser("STR_EXPANSION_HARDLINE_REPUTATION_OUTOFRANGE_TRADER", minRep.ToString(), maxRep.ToString()), EXPANSION_NOTIFICATION_ICON_ERROR, COLOR_EXPANSION_NOTIFICATION_ERROR, 7, ExpansionNotificationType.MARKET).Create(player.GetIdentity());
				}

				return false;
			}
		}

		return true;
	}

	override private void Exec_RequestPurchase(notnull PlayerBase player, string itemClassName, int count, int currentPrice, ExpansionTraderObjectBase trader, bool includeAttachments = true, int skinIndex = -1, TIntArray attachmentIDs = NULL)
	{
		if (!player)
		{
			return;
		}
		
		if (GetExpansionSettings().GetHardline().UseReputation && GetExpansionSettings().GetHardline().UseItemRarityForMarketPurchase)
		{
			ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(itemClassName);
			int required;
			if (rarity && !HasRepForRarity(player, rarity, required))
			{
				StringLocaliser rankTitle = new StringLocaliser("STR_EXPANSION_HARDLINE_MARKET_REPLOW");
				StringLocaliser rankText = new StringLocaliser("STR_EXPANSION_HARDLINE_MARKET_REPLOW_BUY_DESC", required.ToString());
				ExpansionNotification(rankTitle, rankText, EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_EXPANSION, 3, ExpansionNotificationType.MARKET).Create(player.GetIdentity());
				return;
			}
		}

		super.Exec_RequestPurchase(player, itemClassName, count, currentPrice, trader, includeAttachments, skinIndex, attachmentIDs);
	}
	
	override private void Exec_RequestSell(notnull PlayerBase player, string itemClassName, int count, int playerSentPrice, ExpansionTraderObjectBase trader, ExpansionMarketSellDebug playerSentSellDebug)
	{
		if (!player)
		{
			return;
		}
		
		if (GetExpansionSettings().GetHardline().UseReputation && GetExpansionSettings().GetHardline().UseItemRarityForMarketSell)
		{
			ExpansionHardlineItemRarity rarity = GetExpansionSettings().GetHardline().GetItemRarityByType(itemClassName);
			int required;
			if (rarity && !HasRepForRarity(player, rarity, required))
			{
				StringLocaliser rankTitle = new StringLocaliser("STR_EXPANSION_HARDLINE_MARKET_REPLOW");
				StringLocaliser rankText = new StringLocaliser("STR_EXPANSION_HARDLINE_MARKET_REPLOW_SELL_DESC", required.ToString());
				ExpansionNotification(rankTitle, rankText, EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_EXPANSION, 3, ExpansionNotificationType.MARKET).Create(player.GetIdentity());
				return;
			}
		}
		
		super.Exec_RequestSell(player, itemClassName, count, playerSentPrice, trader, playerSentSellDebug);
	}
	
	bool HasRepForRarity(PlayerBase player, ExpansionHardlineItemRarity rarity, out int required = 0)
	{
		if (rarity == ExpansionHardlineItemRarity.NONE)
		{
			return true;
		}

		required = GetExpansionSettings().GetHardline().GetReputationForRarity(rarity);
		return Math.AbsFloat(player.Expansion_GetReputation()) >= Math.AbsFloat(required);
	}
};
#endif
