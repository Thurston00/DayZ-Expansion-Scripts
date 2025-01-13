/**
 * ExpansionSyncedPlayerStats.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionSyncedPlayerStats
 * @brief		This class handles the sycronised player stats values
 **/

enum ExpansionPlayerStanceStatus
{
	UNKNOWN,
	STAND,
	CROUCH,
	PRONE,
	CAR,
	HELICOPTER,
	BOAT,
	DEAD,
	UNCONSCIOUS
}

class ExpansionSyncedPlayerStates
{
	//! ID not synched in 'OnSend', applied in 'RPC_SendPlayerStats'
	string m_PlainID;
	
	int m_Bones;
	int m_Sick;
	int m_Cholera;
	int m_Influenza;
	int m_Salmonella;
	int m_Poison;
	int m_Infection;
	int m_Cuts;
	ExpansionPlayerStanceStatus m_Stance;

	void OnSend(ParamsWriteContext ctx)
	{
		ctx.Write(m_Bones);
		ctx.Write(m_Sick);
		ctx.Write(m_Cholera);
		ctx.Write(m_Influenza);
		ctx.Write(m_Salmonella);
		ctx.Write(m_Poison);
		ctx.Write(m_Infection);
		ctx.Write(m_Cuts);
		ctx.Write(m_Stance);
	}

	bool OnRecieve(ParamsReadContext ctx)
	{
		if (!ctx.Read(m_Bones))
			return false;
		
		if (!ctx.Read(m_Sick))
			return false;
		
		if (!ctx.Read(m_Cholera))
			return false;
		
		if (!ctx.Read(m_Influenza))
			return false;
		
		if (!ctx.Read(m_Salmonella))
			return false;
		
		if (!ctx.Read(m_Poison))
			return false;
		
		if (!ctx.Read(m_Infection))
			return false;
		
		if (!ctx.Read(m_Cuts))
			return false;
		
		if (!ctx.Read(m_Stance))
			return false;

		return true;
	}
};

class ExpansionSyncedPlayerStats
{
	//! ID not synched in 'OnSend', applied in 'RPC_SendPlayerStats'
	string m_PlainID;

	bool m_HasBaseStats;
	int m_Health;
	int m_Blood;
	int m_Water;
	int m_Energy;
	vector m_Position;
	int m_Stamina; //! @note not synced for client player (can get synced values from StaminaHandler)
	
	bool m_HasRegisteredStats;
	float m_Distance;
	float m_Playtime;
	int m_PlayersKilled;
	int m_AIKilled;
	int m_InfectedKilled;
	int m_AnimalsKilled;
	float m_LongestShot;
	float m_Weight; //! @note not synced (can calculate on client)

	void OnSend(ParamsWriteContext ctx, bool includeRegisteredStats = false, bool includeBaseStats = true)
	{
		if (includeBaseStats)
		{
			ctx.Write(m_Health);
			ctx.Write(m_Blood);
			ctx.Write(m_Water);
			ctx.Write(m_Energy);
			ctx.Write(m_Position);
		}

		if (includeRegisteredStats)
		{
			//ctx.Write(m_Stamina);
			ctx.Write(m_Distance);
			ctx.Write(m_Playtime);
			ctx.Write(m_PlayersKilled);
		#ifdef ENFUSION_AI_PROJECT
			ctx.Write(m_AIKilled);
		#endif
			ctx.Write(m_InfectedKilled);
			ctx.Write(m_AnimalsKilled);
			ctx.Write(m_LongestShot);
		}
	}

	bool OnRecieve(ParamsReadContext ctx, bool includeRegisteredStats = false, bool includeBaseStats = true )
	{
		if (includeBaseStats)
		{
			if (!ctx.Read(m_Health))
				return false;
			
			if (!ctx.Read(m_Blood))
				return false;
			
			if (!ctx.Read(m_Water))
				return false;
			
			if (!ctx.Read(m_Energy))
				return false;
			
			if (!ctx.Read(m_Position))
				return false;

			m_HasBaseStats = true;
		}
		
		if (includeRegisteredStats)
		{
			//if (!ctx.Read(m_Stamina))
				//return false;
			
			if (!ctx.Read(m_Distance))
				return false;
			
			if (!ctx.Read(m_Playtime))
				return false;
			
			if (!ctx.Read(m_PlayersKilled))
				return false;
			
		#ifdef ENFUSION_AI_PROJECT
			if (!ctx.Read(m_AIKilled))
				return false;
		#endif
			
			if (!ctx.Read(m_InfectedKilled))
				return false;
			
			if (!ctx.Read(m_AnimalsKilled))
				return false;
			
			if (!ctx.Read(m_LongestShot))
				return false;

			m_HasRegisteredStats = true;
		}

		return true;
	}

	void Acquire(PlayerBase player)
	{
		m_Stamina = player.m_StaminaHandler.GetSyncedStaminaNormalized() * 100;
		player.UpdateWeight();
		m_Weight = player.GetWeight();
	}
};