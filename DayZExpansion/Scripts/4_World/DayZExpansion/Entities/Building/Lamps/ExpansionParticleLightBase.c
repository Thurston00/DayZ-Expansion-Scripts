/**
 * ExpansionParticleLightBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**@class		ExpansionParticleLightBase
 * @brief		
 **/
class ExpansionParticleLightBase: ExpansionLampLightBase
{
	Particle m_Flare;
	
	// ------------------------------------------------------------
	// Constructor
	// ------------------------------------------------------------
	void ExpansionParticleLightBase()
	{
		if ( IsMissionClient() )
		{
			ExpansionSettings.SI_General.Insert( OnSettingsUpdated );
			GetExpansionClientSettings().SI_UpdateSetting.Insert( OnSettingsUpdated );
			ExpansionWorldObjectsModule.SI_LampEnable.Insert( Enable );
			ExpansionWorldObjectsModule.SI_LampDisable.Insert( Disable );
		}
		
	}

	// ------------------------------------------------------------
	// Destructor
	// ------------------------------------------------------------
	void ~ExpansionParticleLightBase()
	{
		if ( IsMissionClient() )
		{
			ExpansionSettings.SI_General.Remove( OnSettingsUpdated );
			GetExpansionClientSettings().SI_UpdateSetting.Remove( OnSettingsUpdated );
			ExpansionWorldObjectsModule.SI_LampEnable.Remove( Enable );
			ExpansionWorldObjectsModule.SI_LampDisable.Remove( Disable );
		}
	}
	
	// ------------------------------------------------------------
	// OnSettingsUpdated
	// ------------------------------------------------------------
	override void OnSettingsUpdated()
	{
		if ( IsMissionClient() )
		{
			m_CastShadow = GetExpansionClientSettings().CastLightShadows;
			
			//! https://feedback.bistudio.com/T173348
			if (GetExpansionSettings().GetGeneral().EnableLamps != LampModeEnum.Disabled)
				m_CanBeEnabled = true;
			else
				m_CanBeEnabled = false;
	
			if ( m_CanBeEnabled )
			{
				OnEnable();
			} 
			else
			{
				OnDisable();
			}
		}
	}

	// ------------------------------------------------------------
	// Expansion OnEnable
	// ------------------------------------------------------------
	protected override void OnEnable()
	{
		super.OnEnable();

		if ( IsMissionClient() )
		{
			if ( !IsEnabled() && m_CanBeEnabled )
			{
				if ( !m_Flare )
				{
					if ( MemoryPointExists( "zluty pozicni" ) )
					{	
						m_Flare = Particle.PlayOnObject( ParticleList.EXPANSION_LIGHT_YELLOW, this, GetMemoryPointPos("zluty pozicni") + "0 0.1 0" );
					} 
					else 
					{
						m_Flare = Particle.PlayOnObject( ParticleList.EXPANSION_LIGHT_YELLOW, this, GetLampPosition() + "0 0.1 0" );
					}
				}
			}
		}
	}

	// ------------------------------------------------------------
	// Expansion OnDisable
	// ------------------------------------------------------------
	protected override void OnDisable()
	{
		super.OnDisable();

		if ( IsMissionClient() )
		{
			if ( m_Flare )
			{
				m_Flare.Stop();

				GetGame().ObjectDelete( m_Flare );
			}
		}
	}
};