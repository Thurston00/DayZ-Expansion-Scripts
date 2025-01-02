/**
 * ExpansionPlayerState.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2024 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionPlayerState
{
	float m_Health;
	float m_Energy;
	float m_Water;
	float m_HeatComfort;
	float m_Wetness;
	float m_Stamina;
	float m_HeatBuffer;
	vector m_Position;

	void ExpansionPlayerState(PlayerBase player)
	{
		AcquireFrom(player);
	}

	void AcquireFrom(PlayerBase player)
	{
		m_Health = player.GetHealth("","");
		m_Energy = player.GetStatEnergy().Get();
		m_Water = player.GetStatWater().Get();
		m_HeatComfort = player.GetStatHeatComfort().Get();
		m_Wetness = player.GetStatWet().Get();
		m_Stamina = player.GetStatStamina().Get();
		m_HeatBuffer = player.GetStatHeatBuffer().Get();
		m_Position = player.GetPosition();
	}

	void ApplyTo(PlayerBase player)
	{
		player.SetHealth(m_Health);
		player.GetStatEnergy().Set(m_Energy);
		player.GetStatWater().Set(m_Water);
		player.GetStatHeatComfort().Set(m_HeatComfort);
		player.GetStatWet().Set(m_Wetness);
		player.GetStatStamina().Set(m_Stamina);
		player.GetStatHeatBuffer().Set(m_HeatBuffer);
	}
}