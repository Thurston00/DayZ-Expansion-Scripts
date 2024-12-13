/**
 * PlayerBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class PlayerBase
{
	bool m_Expansion_SpawnSelect;

	override void UpdatePlayerMeasures()
	{
		if (!m_Expansion_SpawnSelect)
			super.UpdatePlayerMeasures();
	}

	override void Expansion_OnSyncJuncture(int pJunctureID, ParamsReadContext pCtx)
	{
		super.Expansion_OnSyncJuncture(pJunctureID, pCtx);

		switch (pJunctureID)
		{
			case DayZPlayerSyncJunctures.EXPANSION_SJ_TELEPORT:
				if (m_Expansion_SpawnSelect)
				{
					MissionBaseWorld missionWorld = MissionBaseWorld.Cast(GetGame().GetMission());
					if (missionWorld)
					{
						missionWorld.Expansion_EndSpawnSelection(this);
					}
				}
				break;
		}
	}
};
