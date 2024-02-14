/**
 * AnimalBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class AnimalBase
{
	protected autoptr ExpansionZoneActor m_Expansion_SafeZoneInstance = new ExpansionZoneEntity<AnimalBase>(this);

	protected bool m_Expansion_IsInSafeZone;
	protected bool m_Expansion_IsInSafeZone_DeprecationWarning;

	bool m_Expansion_LobotomyInProgress;
	bool m_Expansion_IsLobotomized;

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();

		if (!GetAIAgent())
			GetDayZGame().GetExpansionGame().LobotomySync(this);
	}

	bool Expansion_IsDanger()
	{
		if (!IsDanger())
			return false;

		if (!GetAIAgent() || m_Expansion_LobotomyInProgress || m_Expansion_IsLobotomized)
			return false;

		return true;
	}

	void OnEnterZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.ZONES, this, "OnEnterZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			m_Expansion_IsInSafeZone = true;

			if (IsDanger())
			{
				SetHealth(0);
				GetGame().GetCallQueue( CALL_CATEGORY_SYSTEM ).CallLater( GetGame().ObjectDelete, 5000, false, this );
			}
		}
	}

	void OnExitZone(ExpansionZoneType type)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.ZONES, this, "OnExitZone");
#endif

		if (type == ExpansionZoneType.SAFE)
		{
			m_Expansion_IsInSafeZone = false;
		}
	}

	bool IsInSafeZone()
	{
		Expansion_Error("DEPRECATED: Please use Expansion_IsInSafeZone", m_Expansion_IsInSafeZone_DeprecationWarning);
		return Expansion_IsInSafeZone();
	}

	bool Expansion_IsInSafeZone()
	{
		return m_Expansion_IsInSafeZone;
	}

	override void EEHitBy(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
		super.EEHitBy(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

		if (damageType == DT_EXPLOSION && ExpansionDamageSystem.IsEnabledForExplosionTarget(this))
		{
			ExpansionDamageSystem.OnExplosionHit(source, this, ammo, true, damageResult);
		}
		else if (damageType == DT_FIRE_ARM && ammo == "Bullet_Expansion_LobotomyDart")
		{
			if (GetAIAgent() && !m_Expansion_LobotomyInProgress && !m_Expansion_IsLobotomized)
				Expansion_StartLobotomy();
		}
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		//! Will CTD if targeted by another creature after health of this one is depleted while it has no AI agent,
		//! so remove the body to prevent that
		if (!GetAIAgent())
			GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(Delete, 1500);
	}

	override bool ModCommandHandlerBefore(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (m_Expansion_IsLobotomized && IsAlive())
		{
			DayZAnimalInputController controller = GetInputController();

			if (HandleDeath(pCurrentCommandID, controller))
				return true;

			if (HandleDamageHit(pCurrentCommandID))
				return true;

			return true;
		}

		if (super.ModCommandHandlerBefore(pDt, pCurrentCommandID, pCurrentCommandFinished))
			return true;

		return false;
	}

	override void CommandHandler(float dt, int currentCommandID, bool currentCommandFinished)
	{
		super.CommandHandler(dt, currentCommandID, currentCommandFinished);

		if (m_Expansion_LobotomyInProgress /*|| m_Expansion_IsLobotomized || !GetAIAgent()*/)
		{
			if (currentCommandFinished && !m_Expansion_IsLobotomized)
				GetDayZGame().GetExpansionGame().Lobotomize(this);
		}
	}

	void Expansion_StartLobotomy()
	{
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.MISC, this);
	#endif

		m_Expansion_LobotomyInProgress = true;

		auto controller = GetInputController();

		controller.OverrideBehaviourSlot(true, DayZAnimalBehaviourSlot.NON_SPECIFIC_THREAT);
		controller.OverrideBehaviourAction(true, DayZAnimalBehaviourAction.IDLE1_INPUT);

		controller.OverrideTurnSpeed(true, 0.0);
		controller.OverrideMovementSpeed(true, 0.0);
		controller.OverrideHeading(true, GetOrientation()[0] * Math.DEG2RAD);
		controller.OverrideAlertLevel(true, true, 0, 0.0);
	}

	void Expansion_SetLobotomized(bool isLobotomized)
	{
		m_Expansion_IsLobotomized = isLobotomized;
	}

	bool Expansion_IsLobotomized()
	{
		if (!GetAIAgent())
			return true;

		return m_Expansion_IsLobotomized;
	}

	void Expansion_EndLobotomy()
	{
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.MISC, this);
	#endif

		m_Expansion_LobotomyInProgress = false;

		if (GetAIAgent() && !m_Expansion_IsLobotomized)
		{
		#ifdef DIAG
			EXTrace.Print(EXTrace.MISC, this, "Resetting input controller...");
		#endif

			auto controller = GetInputController();

			controller.OverrideBehaviourSlot(false, DayZAnimalBehaviourSlot.NON_SPECIFIC_THREAT);
			controller.OverrideBehaviourAction(false, DayZAnimalBehaviourAction.TRAVELING_INPUT);

			controller.OverrideTurnSpeed(false, 0.0);
			controller.OverrideMovementSpeed(false, 0.0);
			controller.OverrideHeading(false, GetOrientation()[0] * Math.DEG2RAD);
			controller.OverrideAlertLevel(false, false, 0, 0.0);
		}
	}
}
