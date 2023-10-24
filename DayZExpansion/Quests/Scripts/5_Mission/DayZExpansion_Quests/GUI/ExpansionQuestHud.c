/**
 * ExpansionQuestHUD.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODQUESTS_HUD_ENABLE
class ExpansionQuestHUD: ExpansionScriptView
{
	protected ref ExpansionQuestHUDController m_QuestHUDController;
	protected ref map<int, bool> m_QuestEntriesVisibilityPreference = new map<int, bool>;
	protected WrapSpacerWidget QuestEntriesWraper;
	protected ref map<int, ref ExpansionQuestHUDEntry> m_QuestEntries = new map<int, ref ExpansionQuestHUDEntry>;
	
	void ExpansionQuestHUD()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS & EXTrace.UI, this);
		
		m_QuestHUDController = ExpansionQuestHUDController.Cast(GetController());
		
		ExpansionQuestModule.GetModuleInstance().GetQuestHUDCallbackSI().Insert(UpdateView);
	}

	void ~ExpansionQuestHUD()
	{
		if (ExpansionQuestModule.GetModuleInstance())
			ExpansionQuestModule.GetModuleInstance().GetQuestHUDCallbackSI().Remove(UpdateView);
	}

	void UpdateView()
	{
		auto trace = EXTrace.Start(true, this);
		
		if (!GetGame().GetPlayer())
		{
			EXTrace.Print(true, this, "no player - skipping");
			return;
		}

		if (!IsVisible())
		{
			EXTrace.Print(true, this, "not visible - skipping");
			return;
		}
		
		map<int, bool> visStates = GetExpansionClientSettings().QuestVisibilityStates;
		Print(ToString() + "::UpdateView - Quest entry visibility states: " + visStates.Count());
		foreach (int visQuestID, bool visState: visStates)
		{
			Print(ToString() + "::UpdateView - Set quest entry visibility state for quest entry: " + visQuestID + " | State: " + visState);
			m_QuestEntriesVisibilityPreference[visQuestID] = visState;
		}

		ExpansionQuestPersistentData playerData = ExpansionQuestModule.GetModuleInstance().GetClientQuestData();
		if (!playerData)
		{
			EXTrace.Print(true, this, "no client quest data - skipping");
			return;
		}

		array<int> updatedIDs = new array<int>;
		bool isVisible;
		foreach (ExpansionQuestPersistentQuestData data: playerData.QuestData)
		{
			int questID = data.QuestID;
			ExpansionQuestState state = data.State;

			if (state == ExpansionQuestState.NONE || state == ExpansionQuestState.COMPLETED)
				continue;

			ExpansionQuestConfig questConfig = ExpansionQuestModule.GetModuleInstance().GetQuestConfigByID(questID);
			if (!questConfig)
			{
				continue;
			}

			if (questConfig.IsAchievement())
				continue;

			ExpansionQuestHUDEntry entry;
			if (m_QuestEntries.Find(questConfig.GetID(), entry))
			{
				entry.UpdateQuestData(data);
			}
			else
			{
				entry = new ExpansionQuestHUDEntry(questConfig, data);
				QuestEntriesWraper.AddChild(entry.GetLayoutRoot());
				m_QuestEntries.Insert(questConfig.GetID(), entry);
				entry.SetEntry();
	
				if (!m_QuestEntriesVisibilityPreference.Find(questID, isVisible) || isVisible)
				{
					Print(ToString() + "::UpdateView - Show entry for quest with ID " + questID + ".");
					entry.Show();
				}
				else
				{
					Print(ToString() + "::UpdateView - Hide entry for quest with ID " + questID + ".");
					entry.Hide();
				}
			}
			
			updatedIDs.Insert(questID);
		}

		array<int> toRemoveIds = {};
		foreach (int entryQuestID, ExpansionQuestHUDEntry hudEntry: m_QuestEntries)
		{
			if (updatedIDs.Find(entryQuestID) == -1)
			{
				hudEntry.Destroy();
				toRemoveIds.Insert(entryQuestID);
			}
		}

		foreach (int toRemoveId: toRemoveIds)
		{
			m_QuestEntries.Remove(toRemoveId);
			m_QuestEntriesVisibilityPreference.Remove(toRemoveId);
			
			ExpansionQuestState questState = ExpansionQuestModule.GetModuleInstance().GetClientQuestData().GetQuestStateByQuestID(toRemoveId);
			if (questState == ExpansionQuestState.NONE || questState == ExpansionQuestState.COMPLETED)
				GetExpansionClientSettings().SetQuestVisibilityState(toRemoveId, true);
		}
	}

	void ShowHud(bool state)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS & EXTrace.UI, this);
		
		if (state)
		{
			Show();
		}
		else
		{
			Hide();
		}
	}
	
	override void OnShow()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS & EXTrace.UI, this);
		
		super.OnShow();
		
		UpdateView();
	}
	
	override void OnHide()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS & EXTrace.UI, this);
		
		super.OnHide();
	}

	override string GetLayoutFile()
	{
		return "DayZExpansion/Quests/GUI/layouts/quests/expansion_quest_hud.layout";
	}

	override typename GetControllerType()
	{
		return ExpansionQuestHUDController;
	}

	void ToggleQuestEntryVisibilityByID(int questID)
	{
		ExpansionQuestHUDEntry entry = m_QuestEntries[questID];
		bool isEntryVisible = IsEntryHidden(questID);
		if (!isEntryVisible)
		{
			m_QuestEntriesVisibilityPreference[questID] = false;
			entry.Hide();
		}
		else
		{
			m_QuestEntriesVisibilityPreference[questID] = true;
			entry.Show();
		}
		
		GetExpansionClientSettings().SetQuestVisibilityState(questID, isEntryVisible);
	}

	bool IsEntryHidden(int questID)
	{
		bool isVisible;
		return m_QuestEntriesVisibilityPreference.Find(questID, isVisible) && !isVisible;
	}
};

class ExpansionQuestHUDController: ExpansionViewController {};
#endif