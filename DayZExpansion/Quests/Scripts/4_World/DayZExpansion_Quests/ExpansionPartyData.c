/**
 * ExpansionPartyData.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODGROUPS
modded class ExpansionPartyData
{
	//! We send all the group quests to the new group member
	//! ToDo: Might want to check if the joned player has already
	//! completed the quests that are active for this group so he cant
	//! redo it again?!
	override void OnJoin(ExpansionPartyPlayerData player)
	{
		super.OnJoin(player);

		ExpansionQuestModule questModule;
		if (!Class.CastTo(questModule,CF_ModuleCoreManager.Get(ExpansionQuestModule)))
		{
			Error(ToString() + "::OnJoin - Could not get quest module!");
			return;
		}

		string playerUID = player.GetID();
		ExpansionQuestPlayerData playerQuestData = questModule.GetPlayerQuestDataByUID(playerUID);
		if (!playerQuestData)
		{
			Error(ToString() + "::OnJoin - Could not get quest player data!");
			return;
		}

		for (int a = 0; a < questModule.GetActiveQuests().Count(); a++)
		{
			ExpansionQuest activeQuestInstance = questModule.GetActiveQuests()[a];
		#ifdef EXPANSIONMODQUESTSMODULEDEBUG
			Print(ToString() + "::OnJoin - Quest: " + activeQuestInstance);
		#endif
			if (activeQuestInstance.IsGroupQuest() && GetPartyID() == activeQuestInstance.GetGroupID())
			{
				Print(ToString() + "::OnJoin - There is a active group quest instance for this player! Add quest.");
				//! Make sure player has the correct quest state for this quest in his quest data.
				playerQuestData.UpdateQuestState(activeQuestInstance.GetQuestConfig().GetID(), activeQuestInstance.GetQuestState());
				playerQuestData.Save(playerUID);

				activeQuestInstance.OnGroupMemberJoined(playerUID);
			}
		}

		PlayerBase playerBase = PlayerBase.GetPlayerByUID(playerUID);
		if (!playerBase)
		{
		#ifdef EXPANSIONMODQUESTSMODULEDEBUG
			Print(ToString() + "::OnJoin - Could not get player base. Player is offline!");
		#endif
			return;
		}

		questModule.SendPlayerQuestData(playerBase.GetIdentity());
	}

	//! We send all the group quests to the leaving member
	//! ToDo: Might want to check if the leaving player had already
	//! quest states for the quests that are active for this group so Well
	//! can recover these sates.
	override void OnLeave(ExpansionPartyPlayerData player)
	{
		super.OnLeave(player);

		ExpansionQuestModule questModule;
		if (!Class.CastTo(questModule,CF_ModuleCoreManager.Get(ExpansionQuestModule)))
		{
			Error(ToString() + "::OnLeave - Could not get quest module!");
			return;
		}

		string playerUID = player.GetID();
		ExpansionQuestPlayerData playerQuestData = questModule.GetPlayerQuestDataByUID(playerUID);
		if (!playerQuestData)
		{
			Error(ToString() + "::OnLeave - Could not get quest player data!");
			return;
		}

		for (int a = 0; a < questModule.GetActiveQuests().Count(); a++)
		{
			ExpansionQuest activeQuestInstance = questModule.GetActiveQuests()[a];
		#ifdef EXPANSIONMODQUESTSMODULEDEBUG
			Print(ToString() + "::OnLeave - Quest: " + activeQuestInstance);
		#endif
			if (activeQuestInstance.IsGroupQuest() && GetPartyID() == activeQuestInstance.GetGroupID())
			{
			#ifdef EXPANSIONMODQUESTSMODULEDEBUG
				Print(ToString() + "::OnLeave - There is a active group quest instance for this player! Remove quest.");
			#endif
				//! Make sure player has the correct quest state for this quest in his quest data.
				playerQuestData.UpdateQuestState(activeQuestInstance.GetQuestConfig().GetID(), 0);
				playerQuestData.Save(playerUID);

				activeQuestInstance.OnGroupMemberLeave(playerUID);
			}
		}

		PlayerBase playerBase = PlayerBase.GetPlayerByUID(playerUID);
		if (!playerBase)
		{
		#ifdef EXPANSIONMODQUESTSMODULEDEBUG
			Print(ToString() + "::OnLeave - Could not get player base. Player is offline!");
		#endif
			return;
		}

		questModule.SendPlayerQuestData(playerBase.GetIdentity());
	}
};
#endif