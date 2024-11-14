modded class AnimalBase
{
	private ref eAICreatureTargetInformation m_TargetInformation = new eAICreatureTargetInformation(this);
	ref eAIDamageHandler m_eAI_DamageHandler = new eAIDamageHandler(this, m_TargetInformation);
	bool m_Expansion_Airborne;
	float m_Expansion_AirbornePeakAltitude;
	bool m_Expansion_IsBigGame;

	void AnimalBase()
	{
		vector minMax[2];
		float radius;

		//! @note bounding radius returned by ClippingInfo is very inaccurate for animals, so we use collision box to calculate it
		if (GetCollisionBox(minMax))
		{
			float w = minMax[1][0] - minMax[0][0];
			float h = minMax[1][1] - minMax[0][1];
			float d = minMax[1][2] - minMax[0][2];

			radius = Math.Sqrt(Math.Pow(w, 2.0) + Math.Pow(h, 2.0) + Math.Pow(d, 2.0)) * 0.5;

			if (radius > 1.3)
				m_Expansion_IsBigGame = true;
		}

	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.AI, this, "Bounding radius: " + radius + " | Big game: " + m_Expansion_IsBigGame);
	#endif
	}

	eAICreatureTargetInformation GetTargetInformation()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetTargetInformation");
#endif

		return m_TargetInformation;
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		if (!super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		if (!m_eAI_DamageHandler.OnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
			return false;

		return true;
	}

	override void EEKilled(Object killer)
	{
		m_TargetInformation.OnDeath(killer);

		super.EEKilled(killer);
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		m_TargetInformation.OnHit(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	}

	override void EEHealthLevelChanged(int oldLevel, int newLevel, string zone)
	{
		m_TargetInformation.OnHealthLevelChanged(oldLevel, newLevel, zone);

		super.EEHealthLevelChanged(oldLevel, newLevel, zone);
	}

	override bool ModCommandHandlerBefore(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (m_Expansion_Airborne)
		{
			float altitude = GetPosition()[1];
			if (altitude > m_Expansion_AirbornePeakAltitude)
				m_Expansion_AirbornePeakAltitude = altitude;
			return false;
		}

		return super.ModCommandHandlerBefore(pDt, pCurrentCommandID, pCurrentCommandFinished);
	}

	override protected void EOnContact(IEntity other, Contact extra)
	{
		if (!IsAlive() || !m_Expansion_Airborne || m_Expansion_AirbornePeakAltitude == 0)
			return;

		float falldmg = (m_Expansion_AirbornePeakAltitude - GetPosition()[1]) * 0.333333;

		EXTrace.Print(EXTrace.AI, this, "::EOnContact falldmg " + falldmg);

		m_Expansion_Airborne = false;
		m_Expansion_AirbornePeakAltitude = 0.0;

		ProcessDirectDamage(DT_CUSTOM, this, "Zone_Belly", "FallDamageHealth", "0 0 0", falldmg);
	}

	void Expansion_SetAirborne(bool state)
	{
		m_Expansion_Airborne = state;
	}
};
