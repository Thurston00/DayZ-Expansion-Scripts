/**
 * DayZExpansion.c
 * 
 * Partly based on Enfusion AI Project Copyright 2021 William Bowers
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class DayZExpansion
{
	private static DayZExpansion m_Instance_5; //! weak ref
	
	eAIBase m_eAI_SpectatedAI;

    void DayZExpansion()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "DayZExpansion");
		#endif

		m_Instance_5 = this;
    }

	static DayZExpansion Get5()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0("DayZExpansion", "Get5");
		#endif
		
		return m_Instance_5;
	}

	override void eAI_Spectate(DayZPlayer player, PlayerIdentity sender)
	{
	#ifdef JM_COT
		JMPlayerModule module;
		if (CF_Modules<JMPlayerModule>.Get(module))
		{
			eAIBase ai;
			if (Class.CastTo(ai, player) && ai != m_eAI_SpectatedAI)
			{
				m_eAI_SpectatedAI = ai;
				module.StartSpectatingAI(ai, sender);
			}
			else if (CurrentActiveCamera)
			{
				if (CurrentActiveCamera.IsInherited(JMSpectatorCamera) || (COT_PreviousActiveCamera && COT_PreviousActiveCamera.IsInherited(JMSpectatorCamera)))
				{
					m_eAI_SpectatedAI = null;
					module.EndSpectating();
				}
			}
		}
	#endif
	}
};
