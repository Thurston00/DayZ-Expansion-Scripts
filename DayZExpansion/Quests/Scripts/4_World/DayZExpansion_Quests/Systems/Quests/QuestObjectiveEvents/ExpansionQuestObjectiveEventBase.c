/**
 * ExpansionQuestObjectiveEventBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

//! Instance and methods are handled on server side only!
class ExpansionQuestObjectiveEventBase
{
	protected int m_Index = -1;
	protected ExpansionQuest m_Quest;
	protected bool m_Completed;
	protected bool m_Initialized;
	protected bool m_Active;
	protected ref ExpansionQuestObjectiveConfig m_ObjectiveConfig;
	protected int m_TimeLimit = -1;
	protected bool m_IsAssigned;

	void ExpansionQuestObjectiveEventBase(ExpansionQuest quest)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		m_Quest = quest;
	}

	void ~ExpansionQuestObjectiveEventBase()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		if (GetGame())
			DeassignObjectiveOnClasses();
	}

	void SetIndex(int index)
	{
		m_Index = index;
	}

	int GetIndex()
	{
		return m_Index;
	}

	ExpansionQuest GetQuest()
	{
		return m_Quest;
	}

	bool IsInitialized()
	{
		return m_Initialized;
	}

	bool IsActive()
	{
		return m_Active;
	}

	bool IsAssigned()
	{
		return m_IsAssigned;
	}

	void SetIsActive(bool state)
	{
		m_Active = state;
	}

	bool IsCompleted()
	{
		return m_Completed;
	}

	void SetCompleted(bool state)
	{
		m_Completed = state;
	}

	void SetInitialized(bool state)
	{
		m_Initialized = state;
	}

	void SetObjectiveConfig(ExpansionQuestObjectiveConfig config)
	{
		m_ObjectiveConfig = config;
	}

	void SetTimeLimit(int time)
	{
		m_TimeLimit = time;
	}

	int GetTimeLimit()
	{
		return m_TimeLimit;
	}

	//! Event called when objective time-limit is reached.
	void OnTimeLimitReached()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
				
		//! Call MissionBase::Expansion_OnObjectiveTimeLimitReached for override in init.c
		MissionBaseWorld.Cast(GetGame().GetMission()).Expansion_OnObjectiveTimeLimitReached(this);
		
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.OnTimeLimitReached);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.SyncTimeLimitTime);
		m_TimeLimit = -1;

		//! @Steve - Note: We call this in the next frame because otherwise calling m_Quest.CancelQuest() in the same frame causes a server crash for no simple reason?!
		//! Lava said it might be infinite recursion but i cant see the source.
		if (m_Quest)
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Call(m_Quest.CancelQuest);
	}

	//! Event called when objective has a time-limit to update the current remainig time in the persistent quest data of the quest players.
	void SyncTimeLimitTime()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		m_TimeLimit = GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).GetRemainingTime(this.OnTimeLimitReached) / 1000;

		if (m_Quest)
			m_Quest.UpdateQuest(false);
	}

	//! Event called when the player starts or continues the quest.
	bool OnStart(bool continues)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		if (m_TimeLimit > -1)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.OnTimeLimitReached, m_TimeLimit * 1000);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.SyncTimeLimitTime, 10 * 1000, true);
		}

		AssignObjectiveOnClasses();

		if (!continues)
		{
			if (!OnEventStart())
			{
				SetInitialized(false);
				SetIsActive(false);
				return false;
			}
		}
		else
		{
			if (!OnContinue())
			{
				SetInitialized(false);
				SetIsActive(false);
				return false;
			}
		}

		return true;
	}

	//! Assign objective to allocated classes.
	//! ToDo: Dokumentation
	void AssignObjectiveOnClasses()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		set<typename> allocationTypeNames = m_ObjectiveConfig.GetAllocationTypeNames();
		if (!allocationTypeNames)
			return;

		foreach (typename typeName: allocationTypeNames)
		{
			switch (typeName)
			{
				case ActionBase:
					ActionBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to ActionBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;

				case ZombieBase:
					ZombieBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to ZombieBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;

				case AnimalBase:
					AnimalBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to AnimalBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;

				case PlayerBase:
					PlayerBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to PlayerBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;

				case ItemBase:
					ItemBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to ItemBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;

				case RecipeBase:
					RecipeBase.AssignQuestObjective(this);
					m_IsAssigned = true;
					ObjectivePrint("Assign to RecipeBase. Type: " + GetObjectiveType() + " | ID: " + m_ObjectiveConfig.GetID());
					break;
			}
		}
	}

	//! Deassign objective from allocated classes.
	//! ToDo: Dokumentation
	void DeassignObjectiveOnClasses()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		set<typename> allocationTypeNames = m_ObjectiveConfig.GetAllocationTypeNames();
		if (!allocationTypeNames)
			return;

		foreach (typename typeName: allocationTypeNames)
		{
			switch (typeName)
			{
				case ActionBase:
					ActionBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;

				case ZombieBase:
					ZombieBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;

				case AnimalBase:
					AnimalBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;

				case PlayerBase:
					PlayerBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;

				case ItemBase:
					ItemBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;

				case RecipeBase:
					RecipeBase.DeassignQuestObjective(this);
					m_IsAssigned = false;
					break;
			}
		}
	}

	//! Event called when the player starts the quest.
	bool OnEventStart()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetInitialized(true);
		SetIsActive(true);

		return true;
	}

	//! Event called when the player continues the quest after a server restart/reconnect.
	bool OnContinue()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetInitialized(true);
		SetIsActive(true);

		return true;
	}

	void OnEntityKilled(EntityAI victim, EntityAI killer, Man killerPlayer = null);

	//! Event called when quest is completed and turned-in.
	bool OnTurnIn(string playerUID, int selectedObjItemIndex = -1)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetInitialized(false);
		SetIsActive(false);

		return true;
	}

	//! Control condition method that controlls if the objective can be completed or not.
	//! Used in ExpansionQuest::QuestCompletionCheck.
	bool CanComplete()
	{
		return true;
	}

	//! Event called when objective is completed
	bool OnComplete()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetCompleted(true);
	#ifdef EXPANSIONMODNAVIGATION
		RemoveObjectiveMarkers();
	#endif

		if (m_TimeLimit > -1)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.OnTimeLimitReached);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.SyncTimeLimitTime);
			m_TimeLimit = -1;
		}

		return true;
	}

	bool OnIncomplete()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetCompleted(false);
		
	#ifdef EXPANSIONMODNAVIGATION
		if (CanCreateMarkers())
			CreateMarkers();
	#endif

		return true;
	}

	bool OnCancel()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		SetInitialized(false);
		SetIsActive(false);

		if (!OnCleanup())
			return false;

		DeassignObjectiveOnClasses();

		return true;
	}

	//! Event called when the quest gets cleaned up.
	bool OnCleanup()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		if (m_TimeLimit > -1)
		{
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.OnTimeLimitReached);
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.SyncTimeLimitTime);
			m_TimeLimit = -1;
		}

		DeassignObjectiveOnClasses();
	#ifdef EXPANSIONMODNAVIGATION
		RemoveObjectiveMarkers();
	#endif

		return true;
	}

	//! Event called for group quests only when a group member joins/rejoins the quest group
#ifdef EXPANSIONMODGROUPS
	void OnGroupMemberJoined(string playerUID);

	//! Event called for group quests only when a group member leaves the quest group
	void OnGroupMemberLeave(string playerUID);
#endif

	ExpansionQuestObjectiveConfig GetObjectiveConfig()
	{
		return m_ObjectiveConfig;
	}

	void QuestDebug()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		ObjectivePrint("------------------------------------------------------------");
		ObjectivePrint("Objective index: " + m_Index);
		ObjectivePrint("Objective quest: " + m_Quest);
		ObjectivePrint("Objective completed: " + m_Completed);
		ObjectivePrint("Objective Initialized: " + m_Initialized);
		ObjectivePrint("Objective active: " + m_Active);
		ObjectivePrint("Objective config: " + m_ObjectiveConfig);
		ObjectivePrint("Objective time limit: " + m_TimeLimit);
		ObjectivePrint("------------------------------------------------------------");
	#endif
	}

	void ObjectivePrint(string text)
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		EXTrace.Print(EXTrace.QUESTS, this, text);
	#endif
	}

	int GetObjectiveType()
	{
		return ExpansionQuestObjectiveType.NONE;
	}

#ifdef EXPANSIONMODNAVIGATION
	void CreateObjectiveMarker(vector pos, string name, int visibility = 6)
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		if (!m_Quest.GetQuestConfig().IsGroupQuest())
		{
			PlayerBase player = m_Quest.GetPlayer();
			if (!player)
				return;

			m_Quest.GetQuestModule().CreateClientMarker(pos, name, m_Quest.GetQuestConfig().GetID(), player.GetIdentity(), m_Index, visibility);
		}
	#ifdef EXPANSIONMODGROUPS
		else
		{
			set<string> members = m_Quest.GetPlayerUIDs();
			foreach (string memberUID: members)
			{
				PlayerBase groupPlayer = PlayerBase.GetPlayerByUID(memberUID);
				if (!groupPlayer)
					continue;

				m_Quest.GetQuestModule().CreateClientMarker(pos, name, m_Quest.GetQuestConfig().GetID(), groupPlayer.GetIdentity(), m_Index, visibility);
			}
		}
	#endif
	}

	void RemoveObjectiveMarkers()
	{
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);

		if (!m_Quest.GetQuestConfig().IsGroupQuest())
		{
			PlayerBase player = m_Quest.GetPlayer();
			if (!player)
				return;

			m_Quest.GetQuestModule().RemoveClientMarkers(m_Quest.GetQuestConfig().GetID(), player.GetIdentity(), m_Index);
		}
	#ifdef EXPANSIONMODGROUPS
		else
		{
			set<string> members = m_Quest.GetPlayerUIDs();
			foreach (string memberUID: members)
			{
				PlayerBase groupPlayer = PlayerBase.GetPlayerByUID(memberUID);
				if (!groupPlayer)
					continue;

				m_Quest.GetQuestModule().RemoveClientMarkers(m_Quest.GetQuestConfig().GetID(), groupPlayer.GetIdentity(), m_Index);
			}
		}
	#endif
	}

	void CreateMarkers();

	bool CanCreateMarkers()
	{
		return true;
	}
#endif
};
