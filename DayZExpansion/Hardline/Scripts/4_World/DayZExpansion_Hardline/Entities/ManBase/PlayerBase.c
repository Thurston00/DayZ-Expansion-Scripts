/**
 * PlayerBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2021 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class PlayerBase
{
	private int m_Expansion_Reputation;
	private int m_Expansion_PersonalStorageLevel;
	ref ExpansionHardlinePlayerData m_Expansion_HardlineData;

	override void Init()
	{
		super.Init();

		m_Expansion_Reputation = -1;

		RegisterNetSyncVariableInt("m_Expansion_Reputation");
		RegisterNetSyncVariableInt("m_Expansion_PersonalStorageLevel", 0, 255);
	}

	//! Only to be called on server!
	bool Expansion_SaveHardlineData(bool force = false)
	{
		if (!GetIdentity())
			return false;

		bool save;
		if (force || m_Expansion_HardlineData.Reputation != m_Expansion_Reputation || m_Expansion_HardlineData.PersonalStorageLevel != m_Expansion_PersonalStorageLevel)
			save = true;
	#ifdef EXPANSIONMODAI
		else if (m_Expansion_HardlineData.FactionID != eAI_GetFactionTypeID())
			save = true;
	#endif
		if (save)
		{
			m_Expansion_HardlineData.Reputation = m_Expansion_Reputation;
			m_Expansion_HardlineData.PersonalStorageLevel = m_Expansion_PersonalStorageLevel;
			m_Expansion_HardlineData.Save(GetIdentity().GetId());
		}

		return save;
	}

	//! Only to be called on server!
	void Expansion_SetHardlineData(ExpansionHardlinePlayerData data)
	{
		m_Expansion_HardlineData = data;
	#ifdef EXPANSIONMODAI
		//! @note this takes precedence over random faction from AISettings.json
		if (m_Expansion_HardlineData.FactionID != -1)
		{
			typename factionType = eAIFaction.GetTypeByID(m_Expansion_HardlineData.FactionID);
			if (factionType)
				SetGroup(eAIGroup.CreateGroup(eAIFaction.Cast(factionType.Spawn())));
		}
	#endif
		m_Expansion_PersonalStorageLevel = m_Expansion_HardlineData.PersonalStorageLevel;
		//! If data was successfully loaded, player rep will be set to value from file, else zero
		Expansion_SetReputation(m_Expansion_HardlineData.Reputation);
		if (GetIdentity() && m_Expansion_HardlineData.FactionReputation.Count())
			m_Expansion_HardlineData.SendFactionReputation(GetIdentity());
	}

	//! Only to be called on server!
	bool Expansion_SetReputation(int rep, bool forceSave = false)
	{
		rep = GetExpansionSettings().GetHardline().ClampReputation(rep);

		m_Expansion_Reputation = rep;

		SetSynchDirty();

		return Expansion_SaveHardlineData(forceSave);
	}
	
#ifdef EXPANSIONMODAI
	//! Only to be called on server!
	void Expansion_SetFactionReputation(int rep, int factionID, bool forceSave = false)
	{
		rep = GetExpansionSettings().GetHardline().ClampReputation(rep);
		
		m_Expansion_HardlineData.SetFactionReputation(factionID, rep);
		if (m_Expansion_HardlineData.FactionID == factionID)
		{
			m_Expansion_Reputation = rep;
			SetSynchDirty();
		}

		Expansion_SaveHardlineData(true);
		
		if (GetIdentity())
			m_Expansion_HardlineData.SendFactionReputation(GetIdentity());
	}
	
	//! Only to be called on server!
	void Expansion_AddFactionReputation(int rep, int factionID)
	{
		int currentRep = m_Expansion_HardlineData.GetReputationByFactionID(factionID);
		Expansion_SetFactionReputation(currentRep + rep, factionID);
	}
	
	//! Only to be called on server!
	void Expansion_DecreaseFactionReputation(int rep, int factionID)
	{
		Expansion_AddFactionReputation(-rep, factionID);
	}
	
	int Expansion_GetFactionReputation(int factionID)
	{
		if (m_Expansion_HardlineData)
		{
			int rep = m_Expansion_HardlineData.GetReputationByFactionID(factionID);
			if (rep > 0)
				return rep;
		}
		
		return 0;
	}
#endif

	//! Only to be called on server!
	void Expansion_AddReputation(int rep)
	{
		Expansion_SetReputation(m_Expansion_Reputation + rep);
	}

	//! Only to be called on server!
	void Expansion_DecreaseReputation(int rep)
	{
		Expansion_AddReputation(-rep);
	}

	int Expansion_GetReputation()
	{
		return m_Expansion_Reputation;
	}

	//! Only to be called on server!
	void Expansion_SetPersonalStorageLevel(int lvl)
	{
		m_Expansion_PersonalStorageLevel = lvl;
		SetSynchDirty();
		Expansion_SaveHardlineData();
	}

	int Expansion_GetPersonalStorageLevel()
	{
		return m_Expansion_PersonalStorageLevel;
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		if (GetExpansionSettings().GetHardline().UseReputation)
			ExpansionHardlineModule.GetModuleInstance().OnEntityKilled(this, killer);
	}

#ifdef EXPANSION_MODSTORAGE
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_Hardline];
		if (!ctx) return true;

		if (ctx.GetVersion() < 47 || ctx.GetVersion() >= 48)
			return true;

		if (!ctx.Read(m_Expansion_Reputation))
			return false;

		return true;
	}
#endif

#ifdef EXPANSIONAI_ONFACTIONCHANGE
	//! Server
	//! @note oldFactionTypeID will be -1 if player didn't have a group/faction before (e.g. initial assignment),
	//! newFactionTypeID will be -1 if removing group/faction
	override void eAI_OnFactionChange(int oldFactionTypeID, int newFactionTypeID)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(ExpansionTracing.HARDLINE, this);
		EXTrace.Add(trace, eAIFaction.GetTypeByID(oldFactionTypeID));
		EXTrace.Add(trace, oldFactionTypeID);
		EXTrace.Add(trace, eAIFaction.GetTypeByID(newFactionTypeID));
		EXTrace.Add(trace, newFactionTypeID);
	#endif

		if (!GetGame().IsServer())
			return;

		if (!GetIdentity())
			return;

		auto hardlineSettings = GetExpansionSettings().GetHardline();
		if (!hardlineSettings.UseReputation || !hardlineSettings.UseFactionReputation)
			return;

		bool persistFaction = hardlineSettings.EnableFactionPersistence;

		if (persistFaction)
		{
			oldFactionTypeID = m_Expansion_HardlineData.FactionID;
			EXTrace.Print(EXTrace.HARDLINE, this, "eAI_OnFactionChange - faction persistence enabled - old faction: " + eAIFaction.GetTypeByID(oldFactionTypeID) + " ID " + oldFactionTypeID);
		}

		if (newFactionTypeID != oldFactionTypeID)
		{
			bool saved;

			m_Expansion_HardlineData.FactionID = newFactionTypeID;

			if (oldFactionTypeID != -1)
			{
				//! Store current reputation for old faction
				m_Expansion_HardlineData.SetFactionReputation(oldFactionTypeID, Expansion_GetReputation(), true);
				EXTrace.Print(EXTrace.HARDLINE, this, "eAI_OnFactionChange - old faction rep: " + Expansion_GetReputation());

				if (newFactionTypeID != -1)
				{
					//! Replace the player's current reputation with the one from the hashmap
					//! for the given faction if found, else zero
					saved = Expansion_SetReputation(m_Expansion_HardlineData.FactionReputation[newFactionTypeID]);
					EXTrace.Print(EXTrace.HARDLINE, this, "eAI_OnFactionChange - assigning faction rep: " + m_Expansion_HardlineData.FactionReputation[newFactionTypeID]);
				}
			}
			else
			{
				//! Player didn't have a faction before, carry over current reputation
				m_Expansion_HardlineData.SetFactionReputation(newFactionTypeID, Expansion_GetReputation());
				EXTrace.Print(EXTrace.HARDLINE, this, "eAI_OnFactionChange - assigning rep: " + Expansion_GetReputation());
			}

			if (!saved)
				Expansion_SaveHardlineData(true);

			MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
			if (missionWorld)
			{
				missionWorld.Expansion_OnPlayerFactionChange(this, oldFactionTypeID, newFactionTypeID);
			}

			m_Expansion_HardlineData.SendFactionReputation(GetIdentity());
		}
	}
#endif
};
