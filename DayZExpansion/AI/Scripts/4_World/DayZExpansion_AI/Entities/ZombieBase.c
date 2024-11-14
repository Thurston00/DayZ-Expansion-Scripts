modded class ZombieBase
{
	private ref eAIZombieTargetInformation m_TargetInformation = new eAIZombieTargetInformation(this);
	ref eAIDamageHandler m_eAI_DamageHandler = new eAIDamageHandler(this, m_TargetInformation);
	bool m_Expansion_Airborne;
	float m_Expansion_AirbornePeakAltitude;

	eAIZombieTargetInformation GetTargetInformation()
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
	#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.AI, this, "EEHitBy[" + m_eAI_DamageHandler.m_HitCounter + "]", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
	#endif

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

		if (pCurrentCommandID == DayZInfectedConstants.COMMANDID_CRAWL)
			m_TargetInformation.m_Crawling = true;

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

		ProcessDirectDamage(DT_CUSTOM, this, "Torso", "FallDamageHealth", "0 0 0", falldmg);
	}

	void Expansion_SetAirborne(bool state)
	{
		m_Expansion_Airborne = state;
	}

	EntityAI Expansion_GetActualTarget()
	{
		return m_ActualTarget;   //! Vanilla variable, but no vanilla getter
	}

	int Expansion_GetMindState()
	{
		return m_MindState;   //! Vanilla variable, but no vanilla getter
	}
};
