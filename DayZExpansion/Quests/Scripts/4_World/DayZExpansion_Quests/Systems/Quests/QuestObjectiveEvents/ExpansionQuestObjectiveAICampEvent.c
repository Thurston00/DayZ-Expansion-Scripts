/**
 * ExpansionQuestObjectiveAIPatrolEvent.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODAI
class ExpansionQuestObjectiveAICampEvent: ExpansionQuestObjectiveEventBase
{
	private float m_UpdateQueueTimer = 0;
	private const float UPDATE_TICK_TIME = 2.0;
	private ref array<eAIDynamicPatrol> AIPatrols = new array<eAIDynamicPatrol>;
	private int m_TotalKillCount = 0; 
	private int m_UnitsToSpawn = 0;
	private int m_TotalUnitsAmount = 0;
	private eAIGroup CampGroup;
	private ExpansionQuestObjectiveAICamp m_AICamp;
	private bool m_InitCheck = false;
	
	//! Event called when the player starts the quest
	override void OnStart()
	{
		ObjectivePrint(ToString() + "::OnStart - Start");

		CreateAICamp();

		super.OnStart();

		ObjectivePrint(ToString() + "::OnStart - End");
	}

	//! Event called when the player continues the quest after a server restart/reconnect
	override void OnContinue()
	{
		ObjectivePrint(ToString() + "::OnContinue - Start");

		CreateAICamp();
		
		super.OnContinue();

		ObjectivePrint(ToString() + "::OnContinue - End");
	}

	//! Event called when the quest gets cleaned up (server shutdown/player disconnect).
	override void OnCleanup()
	{
		ObjectivePrint(ToString() + "::OnCleanup - Start");

		CleanupPatrol();
		
		super.OnCleanup();

		ObjectivePrint(ToString() + "::OnCleanup - End");
	}

	//! Event called when the quest gets manualy canceled by the player.
	override void OnCancel()
	{
		ObjectivePrint(ToString() + "::OnCancel - Start");

		CleanupPatrol();
		
		super.OnCancel();

		ObjectivePrint(ToString() + "::OnCancel - End");
	}
	
	void CleanupPatrol()
	{
		for (int i = 0; i < AIPatrols.Count(); i++)
		{
			eAIDynamicPatrol patrol = AIPatrols[i];
			if (patrol.m_CanSpawn)
				continue;
			
			eAIGroup group = patrol.m_Group;

			ObjectivePrint(ToString() + "::OnCancel - Patrol: " + patrol.ToString());
			ObjectivePrint(ToString() + "::OnCancel - Patrol group: " + group.ToString());
			ObjectivePrint(ToString() + "::OnCancel - Patrol group members: " + group.Count());

			for (int j = 0; j < group.Count(); j++)
			{
				DayZPlayerImplement member = group.GetMember(j);
				if (!member)
					continue;

				ObjectivePrint(ToString() + "::OnCancel - Delete member: [" + j + "] " + member.ToString());

				GetGame().ObjectDelete(member);
			}

			group.Delete();
			patrol.Delete();
		}
		
		AIPatrols.Clear();
	}
	
	void OnEntityKilled(string typeName, string killer)
	{
	#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.QUESTS, this, "OnEntityKilled");
	#endif

		Print(ToString() + "::OnEntityKilled - Victim type: " + typeName);
		Print(ToString() + "::OnEntityKilled - Killer type: " + killer);

		ExpansionQuestObjectiveAICamp aiCamp = GetObjectiveConfig().GetAICamp();
		if (!aiCamp)
			return;
		
		int findIndex;
		//! If the ai camp need to be killed with a special weapon check incoming killer class type
		if (aiCamp.NeedSpecialWeapon())
		{
			findIndex = -1;
			findIndex = aiCamp.GetAllowedWeapons().Find(killer);
			if (findIndex == -1)
				return;

			Print(ToString() + "::OnEntityKilled - Player has killed with special weapon!");
		}

		int amount = aiCamp.GetPositions().Count();
		m_TotalUnitsAmount = amount;
		findIndex = -1;
		findIndex = aiCamp.GetClassNames().Find(typeName);
		Print(ToString() + "::OnEntityKilled - Target find index: " + findIndex);

		if (findIndex > -1)
		{
			if (m_TotalKillCount < m_TotalUnitsAmount)
				m_TotalKillCount++;
		}
	}
	
	override void OnUpdate(float timeslice)
	{
		if (!IsInitialized() || IsCompleted())
			return;
		
		m_UpdateQueueTimer += timeslice;
		if (m_UpdateQueueTimer >= UPDATE_TICK_TIME)
		{
			CleanupZeds(); //! Cleanups zeds at camp position.
			
			if (m_TotalKillCount >= m_TotalUnitsAmount)
			{
				SetCompleted(true);
				OnComplete();
			}

			m_UpdateQueueTimer = 0.0;
		}
	}

	void CreateAICamp()
	{
		ObjectivePrint(ToString() + "::CreateAICamp - Start");

		if (!GetObjectiveConfig() || !GetQuest() || !GetQuest().GetQuestConfig())
			return;

		ExpansionQuestObjectiveAICamp aiCamp = GetObjectiveConfig().GetAICamp();
		if (!aiCamp)
			return;
		
		m_AICamp = aiCamp;
		m_TotalUnitsAmount = aiCamp.GetPositions().Count();

		//! If all the targets are already killed dont create patrol group
		if (m_TotalKillCount >= m_TotalUnitsAmount)
			return;
		
		m_UnitsToSpawn = m_TotalUnitsAmount - m_TotalKillCount;

		for (int i = 0; i < m_UnitsToSpawn; i++)
		{
			vector pos = aiCamp.GetPositions()[i];
			array<vector> waypoint = new array<vector>;
			waypoint.Insert(pos);

			ExpansionQuestAIGroup group = new ExpansionQuestAIGroup(1, aiCamp.GetNPCSpeed(), aiCamp.GetNPCMode(), aiCamp.GetNPCFaction(), aiCamp.GetNPCLoadoutFile(), pos, waypoint);
			eAIDynamicPatrol patrol = CreateQuestPatrol(group);
			if (!patrol)
				return;

			AIPatrols.Insert(patrol);
		}

	#ifdef EXPANSIONMODNAVIGATION
		string markerName = "[Target] Bandit Camp";
		GetQuest().CreateClientMarker(aiCamp.GetPositions()[0], markerName);
	#endif

		ObjectivePrint(ToString() + "::CreateAICamp - End");
	}

	eAIDynamicPatrol CreateQuestPatrol(ExpansionQuestAIGroup group)
	{
		ObjectivePrint(ToString() + "::CreateQuestPatrol - Start");
		Print("=================== Expansion Quest Camp AI ===================");
		int aiSum;
		if ( group.NumberOfAI != 0 )
		{
			if ( group.NumberOfAI < 0 )
			{
				aiSum = Math.RandomInt(1,-group.NumberOfAI);
			}
			else
			{
				aiSum = group.NumberOfAI;
			}
		}
		else
		{
            Print("[QUESTS] WARNING: NumberOfAI shouldn't be set to 0, skipping this group...");
			return NULL;
		}

        if ( !group.Waypoints )
        {
            Print("!!! ERROR !!!");
            Print("[QUESTS] Couldn't read the Waypoints (validate your file with a json validator)");
            Print("!!! ERROR !!!");
           return NULL;
        }

		vector startpos = group.StartPos;
		if ( !startpos || startpos == "0 0 0" )
		{
		    if ( !group.Waypoints[0] || group.Waypoints[0] == "0 0 0" )
            {
                Print("!!! ERROR !!!");
                Print("[QUESTS] Couldn't find a spawn location. StartPos and at least the first Waypoints are both set to 0 0 0 or cannot be read by the system (validate your file with a json validator)");
                Print("!!! ERROR !!!");
                return NULL;
            }

			startpos = group.Waypoints[0];
		}

		// Safety in case the Y is bellow the ground
		startpos = ExpansionStatic.GetSurfacePosition(startpos);
		if ( startpos[1] < group.StartPos[1] )
			startpos[1] = group.StartPos[1];

		Print("[QUESTS] Spawning "+aiSum+" "+group.Faction+" bots at "+group.StartPos+" and will patrol at "+group.Waypoints);

		eAIDynamicPatrol patrol = eAIDynamicPatrol.Create(startpos, group.Waypoints, GetAIBehaviour(group.Behaviour), group.LoadoutFile, aiSum, -1, GetAIFaction(group.Faction), true, 20, 600, GetAISpeed(group.Speed));

		Print("=================== Expansion Quest Camp AI ===================");
		ObjectivePrint(ToString() + "::CreateQuestPatrol - End and return: " + patrol.ToString());

		return patrol;
	}

	private float GetAISpeed(string speed)
	{
	    switch (speed)
	    {
	        case "WALK":
	        {
	            return 1.0;
	            break;
	        }
	        case "JOG":
	        {
	            return 2.0;
	            break;
	        }
	        case "SPRINT":
	        {
	            return 3.0;
	            break;
	        }
	    }

	    //! Unknown speed, sending default speed
	    return 2.0;
	}

	private int GetAIBehaviour(string beh)
	{
	    switch (beh)
	    {
	        case "ALTERNATE":
	        {
	            return eAIWaypointBehavior.ALTERNATE; // Follow the waypoints in reverse (from end to start)
	            break;
	        }
	        case "HALT":
	        {
	            return eAIWaypointBehavior.HALT; // They just don't move, they stay at their position
	            break;
	        }
	        case "LOOP":
	        {
	            return eAIWaypointBehavior.LOOP; // Follow the waypoint's in the normal order (from start to finish)
	            break;
	        }
	    }

	    //! Unknown Behaviour, sending default behaviour
	    return eAIWaypointBehavior.ALTERNATE;
	}

	private eAIFaction GetAIFaction(string faction)
	{
	    switch (faction)
	    {
	        case "WEST":
	        {
	            return new eAIFactionWest(); // Friendly toward WEST and CIVILIANS
	            break;
	        }
	        case "EAST":
	        {
	            return new eAIFactionEast(); // Friendly toward EAST and CIVILIANS
	            break;
	        }
	        case "INSURGENT":
	        {
	            return new eAIFactionRaiders(); // Hostile toward everyone
	            break;
	        }
	        case "CIVILIAN":
	        {
	            return new eAIFactionCivilian(); // They like everyone
	            break;
	        }
	    }

	    //! Unknown Faction, sending default faction
	    return new eAIFactionCivilian();
	}

	private void CleanupZeds()
	{
		Print(ToString() + "::CleanupZeds - Start");

		ExpansionQuestObjectiveAICamp aiCamp = GetObjectiveConfig().GetAICamp();
		if (!aiCamp)
			return;

		array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition3D(aiCamp.GetPositions()[0], 500, objects, NULL);
		for (int i = 0; i < objects.Count(); i++)
		{
			Object obj = objects[i];
			if (obj.IsInherited(ZombieBase))
				GetGame().ObjectDelete(obj);
		}

		Print(ToString() + "::CleanupZeds - End");
	}
	
	void SetKillCount(int count)
	{
		m_TotalKillCount = count;
	}

	int GetCount()
	{
		return m_TotalKillCount;
	}

	int GetAmount()
	{
		return m_TotalUnitsAmount;
	}
	
	override int GetObjectiveType()
	{
		return ExpansionQuestObjectiveType.AICAMP;
	}
};
#endif