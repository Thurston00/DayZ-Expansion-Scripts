/**
 * ExpansionQuestObjectiveDeliveryEvent.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionQuestObjectiveDeliveryEvent: ExpansionQuestObjectiveCollectionEventBase
{
	protected ExpansionTravelObjectiveSphereTrigger m_ObjectiveTrigger;
	protected bool m_DestinationReached;
#ifdef EXPANSIONMODNAVIGATION
	protected bool m_CreatedMarker;
#endif
	protected vector m_Position;

	override bool OnEventStart()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!super.OnEventStart())
			return false;

		if (!Class.CastTo(m_Config, m_ObjectiveConfig))
			return false;

		if (!CreateObjectiveTrigger())
			return false;

		if (!GetObjectiveDataFromConfig())
			return false;

		if (!SpawnObjectiveDeliveryItems())
			return false;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ObjectiveCheck, 500);

		return true;
	}

	override bool OnContinue()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!super.OnContinue())
			return false;

		if (!Class.CastTo(m_Config, m_ObjectiveConfig))
			return false;

		if (!CreateObjectiveTrigger())
			return false;

		if (!GetObjectiveDataFromConfig())
			return false;

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ObjectiveCheck, 500);

		return true;
	}

	override bool OnCleanup()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!super.OnCleanup())
			return false;

		DeleteObjectiveTrigger();

		return true;
	}

	protected void DeleteObjectiveTrigger()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!m_ObjectiveTrigger)
			return;

		GetGame().ObjectDelete(m_ObjectiveTrigger);
	}

	void SetReachedLocation(bool state)
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif
		ObjectivePrint("State: " + state);
		m_DestinationReached = state;
		m_Quest.QuestCompletionCheck(true);
	}

	bool GetLocationState()
	{
		return m_DestinationReached;
	}

	protected bool CreateObjectiveTrigger()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!m_Quest || !m_Quest.GetQuestConfig())
			return false;
		
		PlayerBase player = m_Quest.GetPlayer();
		if (!player || !player.GetIdentity())
			return false;
		
		//! If no turn-in NPCs are configured we just dont create the trigger and set the destination check condition to true as the whole world will be a valid place to complete this objective.
		array<int> questNPCTurnInIDs = m_Quest.GetQuestConfig().GetQuestTurnInIDs();
		if (!questNPCTurnInIDs || questNPCTurnInIDs.Count() == 0)
		{
			m_DestinationReached = true;
			return true;
		}

		vector playerPos = player.GetPosition();
		//! If we cant find a valid quest NPC entity to turn this quest in we inform the player about this issue and return false to prevent abuse?!
		Object npcObj = m_Quest.GetQuestModule().GetClosestQuestNPCForQuest(questNPCTurnInIDs, playerPos);
		if (!npcObj)
		{
			string objectiveType = typename.EnumToString(ExpansionQuestObjectiveType, m_Config.GetObjectiveType());
			StringLocaliser text = new StringLocaliser("The system could not find a valid quest NPC entity for the quest objective %1 with ID %2. The entities class name/s are probably invalid and the entities are not spawned. Please inform the server Admin about this problem.", objectiveType, m_Config.GetID().ToString());
			ExpansionNotification("Quest Objective Error", text).Error(player.GetIdentity());
			return false;
		}

		vector npcPos = npcObj.GetPosition();
		if (!TriggerCreationCheck(npcPos))
		{
			m_DestinationReached = true;
			return true;
		}

		m_Position = npcPos;

		Object trigger = GetGame().CreateObjectEx("ExpansionTravelObjectiveSphereTrigger", npcPos, ECE_NONE);
		if (!Class.CastTo(m_ObjectiveTrigger, trigger))
		{
			GetGame().ObjectDelete(trigger);
			return false;
		}

		m_ObjectiveTrigger.SetObjectiveData(this);
		m_ObjectiveTrigger.SetPosition(npcPos);

		return true;
	}
	
	bool TriggerCreationCheck(vector npcPos)
	{
		//! If trigger radius is large enough to cover the whole map, don't create it
		float worldSize = GetGame().GetWorld().GetWorldSize();
		//! Choose world pos at edge of map that is farthest from NPC pos
		vector worldPos;
		if (npcPos[0] < worldSize * 0.5)
			worldPos[0] = worldSize;

		worldPos[1] = npcPos[1];

		if (npcPos[2] < worldSize * 0.5)
			worldPos[2] = worldSize;

		float maxDistance = m_Config.GetMaxDistance();
		if (maxDistance <= 0 || maxDistance * maxDistance >= vector.DistanceSq(npcPos, worldPos))
			return false;
		
		return true;
	}

	override bool CanComplete()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif
		ObjectivePrint("m_ObjectiveItemsCount: " + m_ObjectiveItemsCount);
		ObjectivePrint("m_ObjectiveItemsAmount: " + m_ObjectiveItemsAmount);
		ObjectivePrint("m_DestinationReached: " + m_DestinationReached);

		bool conditionsResult;
		if (m_ObjectiveItemsAmount != 0 && m_ObjectiveItemsCount >= m_ObjectiveItemsAmount && m_DestinationReached)
			conditionsResult = true;

		return conditionsResult;
	}

	override bool OnCancel()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!super.OnCancel())
			return false;

		CheckQuestPlayersForObjectiveItems();
		DeleteRemainingObjectiveItems();

		return true;
	}

	protected void DeleteRemainingObjectiveItems()
	{
		for (int i = m_ObjectiveItems.Count() - 1; i >= 0; i--)
		{
			ExpansionQuestObjectiveItem objItem = m_ObjectiveItems[i];
			objItem.GetItem().Expansion_SetQuestID(-1);
			GetGame().ObjectDelete(objItem.GetItem());
			m_ObjectiveItems.RemoveOrdered(i);
		}
	}

	protected bool SpawnObjectiveDeliveryItems()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		PlayerBase player = m_Quest.GetPlayer();
		if (!player || !player.GetIdentity())
			return false;

		vector position = player.GetPosition();
		vector orientation = player.GetOrientation();

		array<ref ExpansionQuestObjectiveDelivery> objectiveDeliveries = m_Config.GetCollections();
		foreach (ExpansionQuestObjectiveDelivery delivery: objectiveDeliveries)
		{
			if (!SpawnDeliveryItems(delivery, player, player, position, orientation))
				return false;
		}

		return true;
	}

	protected bool SpawnDeliveryItems(ExpansionQuestObjectiveDelivery delivery, PlayerBase player, EntityAI parent, vector position, vector orientation)
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (m_Quest.GetQuestConfig().GetID() == -1)
			return false;

		int quantityPercent = delivery.GetQuantity();
	    int amountToSpawn = delivery.GetAmount();
		while (amountToSpawn > 0)
	    {
	        Object obj = ExpansionItemSpawnHelper.SpawnOnParent(delivery.GetClassName(), player, parent, amountToSpawn, quantityPercent, null, -1, false);
	        if (!obj)
	            break;

	        ItemBase questItem;
	        if (!Class.CastTo(questItem, obj))
	        {
	            GetGame().ObjectDelete(obj);
	            return false;
	        }

			Magazine mag;
			if (Class.CastTo(mag, questItem))
			{
				if (!mag.IsAmmoPile())
					mag.ServerSetAmmoCount(0);
			}

	        questItem.Expansion_SetQuestID(m_Quest.GetQuestConfig().GetID());
	    }

		return true;
	}

	override protected bool CheckQuestPlayersForObjectiveItems(bool continues = false)
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif

		if (!super.CheckQuestPlayersForObjectiveItems(continues))
			return false;

		if (continues && m_ObjectiveItemsCount < m_ObjectiveItemsAmount)
			return false;

		ObjectivePrint("End and return TRUE");

		return true;
	}

	override int GetObjectiveType()
	{
		return ExpansionQuestObjectiveType.DELIVERY;
	}

	protected void DestinationCheck()
	{
	#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	#endif
		
		//! If no turn-in NPCs are configured we just set the condition to true as the whole world will be our location to complete this objective.
		array<int> questNPCTurnInIDs = m_Quest.GetQuestConfig().GetQuestTurnInIDs();
		if (!questNPCTurnInIDs || questNPCTurnInIDs.Count() == 0)
		{
			SetReachedLocation(true);
			return;
		}
		
		PlayerBase player = m_Quest.GetPlayer();
		if (!player)
			return;
		
		vector playerPos = player.GetPosition();
		//! If we cant find a valid quest NPC entity to turn this quest in we inform the player about this issue and return false to prevent abuse?!
		Object npcObj = m_Quest.GetQuestModule().GetClosestQuestNPCForQuest(questNPCTurnInIDs, playerPos);
		if (!npcObj)
		{
			string objectiveType = typename.EnumToString(ExpansionQuestObjectiveType, m_Config.GetObjectiveType());
			StringLocaliser text = new StringLocaliser("The system could not find a valid quest NPC entity for the quest objective %1 with ID %2. The entities class name/s are probably invalid and the entities are not spawned. Please inform the server Admin about this problem.", objectiveType, m_Config.GetID().ToString());
			ExpansionNotification("Quest Objective Error", text).Error(player.GetIdentity());
			SetReachedLocation(false);
			return;
		}

		vector npcPos = npcObj.GetPosition();
		if (!TriggerCreationCheck(npcPos))
		{
			SetReachedLocation(true);
		}
		else
		{
			float currentDistanceSq;
			if (!GetQuest().GetQuestConfig().IsGroupQuest())
			{
				currentDistanceSq = vector.DistanceSq(playerPos, m_Position);
			}
		#ifdef EXPANSIONMODGROUPS
			else
			{
				//! Set the position of the group member that has the shortest distance to the target location
				//! as our current position if the quest is a group quest.
				set<string> memberUIDs = m_Quest.GetPlayerUIDs();
				float smallestDistanceSq = float.MAX;
				foreach (string memberUID: memberUIDs)
				{
					PlayerBase groupPlayer = PlayerBase.GetPlayerByUID(memberUID);
					if (!groupPlayer)
						continue;
	
					float distSq = vector.DistanceSq(groupPlayer.GetPosition(), m_Position);
					if (distSq < smallestDistanceSq)
					{
						smallestDistanceSq = distSq;
					}
				}
	
				currentDistanceSq = smallestDistanceSq;
			}
		#endif
	
			float maxDistance = m_Config.GetMaxDistance();
			if (currentDistanceSq <= maxDistance * maxDistance)
			{
				ObjectivePrint("End and return TRUE");
				SetReachedLocation(true);
				return;
			}
	
			ObjectivePrint("End and return FALSE");
			SetReachedLocation(false);
		}
	}

#ifdef EXPANSIONMODNAVIGATION
	override void CreateMarkers()
	{
		if (!Class.CastTo(m_Config, m_ObjectiveConfig))
			return;

	//#ifdef EXPANSIONMODQUESTSOBJECTIVEDEBUG
		auto trace = EXTrace.Start(EXTrace.QUESTS, this);
	//#endif
		string markerName = m_Config.GetMarkerName();
		array<int> questNPCTurnInIDs = m_Quest.GetQuestConfig().GetQuestTurnInIDs();
		if (!questNPCTurnInIDs || questNPCTurnInIDs.Count() == 0)
			return;

		PlayerBase player = m_Quest.GetPlayer();
		if (!player)
			return;
		vector playerPos = player.GetPosition();
		vector npcPos = m_Quest.GetClosestQuestNPCPosition(questNPCTurnInIDs, playerPos);
		CreateObjectiveMarker(npcPos, markerName);
		m_CreatedMarker = true;
	}
	
	override void RemoveObjectiveMarkers()
	{
		super.RemoveObjectiveMarkers();
		m_CreatedMarker = false;
	}
	
	override bool CanCreateMarkers()
	{
		bool markerConditionResult = false;
		if (!m_Config.NeedAnyCollection())
		{
			if (m_ObjectiveItemsAmount != 0 && m_ObjectiveItemsCount >= m_ObjectiveItemsAmount && !m_DestinationReached && !m_CreatedMarker)
			{
				markerConditionResult = true;
				ObjectivePrint("::CanCreateMarkers - " + markerConditionResult.ToString());
			}
		}
		else
		{
			if (HasAnyCollectionCompleted() && !m_DestinationReached && !m_CreatedMarker)
			{
				markerConditionResult = true;
				ObjectivePrint("::CanCreateMarkers - " + markerConditionResult.ToString());
			}
		}
		
		return markerConditionResult;
	}
#endif

	override protected void ObjectiveCheck()
	{
		CheckQuestPlayersForObjectiveItems();
		UpdateDeliveryData();
		DestinationCheck();
	}
};