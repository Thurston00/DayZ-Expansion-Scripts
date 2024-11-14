modded class ContaminatedArea_Base
{
	static ref Expansion_EffectAreas s_Expansion_ContaminatedAreas = {};
	static ref ExpansionEffectAreaMergedClusters s_Expansion_MergedClusters = {};

	ref array<eAIBase> m_eAI_AI = {};

	void ContaminatedArea_Base()
	{
		s_Expansion_ContaminatedAreas.Insert(this);
	}

	override void InitZoneServer()
	{
		super.InitZoneServer();

		Expansion_EffectArea_Clusters clusters = s_Expansion_ContaminatedAreas.FindClusters();
		s_Expansion_MergedClusters = clusters.MergeClusters();
	}

	override void OnPlayerEnterServer(PlayerBase player, EffectTrigger trigger)
	{
		super.OnPlayerEnterServer(player, trigger);

		player.Expansion_OnContaminatedAreaEnterServer(this, trigger);

		eAIBase ai;
		if (Class.CastTo(ai, player))
			m_eAI_AI.Insert(ai);
	}

	override void OnPlayerExitServer(PlayerBase player, EffectTrigger trigger)
	{
		super.OnPlayerExitServer(player, trigger);

		player.Expansion_OnContaminatedAreaExitServer(this, trigger);
	}

	override void EEDelete(EntityAI parent)
	{
		EXTrace.Print(EXTrace.AI, this, "::EEDelete");

		super.EEDelete(parent);
		
		if (g_Game.IsServer())
		{
			foreach (eAIBase ai: m_eAI_AI)
			{
				if (ai)
					ai.eAI_ForgetContaminatedArea(this);
			}

			s_Expansion_ContaminatedAreas.RemoveItemUnOrdered(this);

			//! Update cluster containing this area
			foreach (int index, auto cluster: s_Expansion_MergedClusters)
			{
				int areaIndex = cluster.m_Areas.Find(this);

				if (areaIndex > -1)
				{
					if (cluster.m_Areas.Count() > 1)
					{
						cluster.m_Areas.Remove(areaIndex);

						EXTrace.Print(EXTrace.AI, this, "Removed from " + cluster);

						cluster.Update();
					}
					else
					{
						s_Expansion_MergedClusters.Remove(index);

						EXTrace.Print(EXTrace.AI, this, "Removed " + cluster);
					}

					break;
				}
			}
		}
	}
}
