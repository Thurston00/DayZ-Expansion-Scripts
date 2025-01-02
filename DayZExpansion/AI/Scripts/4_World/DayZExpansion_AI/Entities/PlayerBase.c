modded class PlayerBase
{
	ref TIntArray m_eAI_FactionModifiers;

	override void Init()
	{
		super.Init();

		if (!IsAI())
			RegisterNetSyncVariableFloat("m_eAI_LastAggressionTimeout", 0.0, 16777215.0, 2);
	}

	override void SetActions(out TInputActionMap InputActionMap)
	{
		super.SetActions(InputActionMap);

		AddAction(ExpansionActionRecruitAI, InputActionMap);
		AddAction(ExpansionActionDismissAI, InputActionMap);
	}

	//! @note Only called for players not eAIBase since the latter overrides the commandhandler completely,
	//! formation update if AI is leader is dealt with in eAICommandMove
	override bool ModCommandHandlerInside(float pDt, int pCurrentCommandID, bool pCurrentCommandFinished)
	{
		if (pCurrentCommandID == DayZPlayerConstants.COMMANDID_MOVE && !pCurrentCommandFinished)
		{
			eAIGroup group = GetGroup();
			if (group && group.GetFormationLeader() == this && group.Count() > 1)
				group.GetFormation().Update(pDt);
		}

		return super.ModCommandHandlerInside(pDt, pCurrentCommandID, pCurrentCommandFinished);
	}

	override void OnCommandSwimStart()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 

		super.OnCommandSwimStart();

		if (GetGame().IsServer() && GetGroup())
			GetGroup().EnableSwimming(true);  //! Enable swimming for the whole group so they can follow the leader
	}

	override void OnCommandSwimFinish()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
#endif 

		super.OnCommandSwimFinish();

		//if (GetGame().IsServer() && GetGroup())
			//GetGroup().EnableSwimming(false);  //! Disable swimming for the whole group (unless they are currently swimming)
	}

	override void eAI_SetFactionTypeID(int id)
	{
		super.eAI_SetFactionTypeID(id);

		if (GetGame().IsServer())
		{
			eAI_RemoveFactionModifiers();
			if (GetGroup())
				eAI_ApplyFactionModifiers(GetGroup().GetFaction());
		}
	}

	/**
	 * @brief Apply faction modifiers (if any) e.g. brain disease, flies
	 */
	void eAI_ApplyFactionModifiers(eAIFaction faction)
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI,this);
#endif 

		TIntArray modifiers = faction.GetModifiers();

		if (!modifiers || !modifiers.Count())
			return;

#ifdef EXTRACE
		EXTrace.Add(trace, modifiers.Count());
#endif 

		m_eAI_FactionModifiers = {};

		foreach (int modifier: modifiers)
		{
			//! Don't track if already active
			ModifierBase modifierBase = m_ModifiersManager.GetModifier(modifier);
			if (modifierBase && modifierBase.IsActive())
				continue;

			EXTrace.Print(EXTrace.AI, this, typename.EnumToString(eModifiers, modifier));

			switch (modifier)
			{
				case eModifiers.MDF_BRAIN:
					InsertAgent(eAgents.BRAIN, PluginTransmissionAgents.GetAgentMaxCount(eAgents.BRAIN));
					break;
			}

			m_ModifiersManager.ActivateModifier(modifier);
			m_eAI_FactionModifiers.Insert(modifier);
		}
	}

	/**
	 * @brief Remove faction modifiers (if any) e.g. brain disease, flies
	 */
	void eAI_RemoveFactionModifiers()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI,this);
#endif 

		if (!m_eAI_FactionModifiers)
			return;

#ifdef EXTRACE
		EXTrace.Add(trace, m_eAI_FactionModifiers.Count());
#endif 

		foreach (int modifier: m_eAI_FactionModifiers)
		{
			if (!m_ModifiersManager.GetModifier(modifier).IsActive())
				continue;

			switch (modifier)
			{
				case eModifiers.MDF_BRAIN:
					RemoveAgent(eAgents.BRAIN);
					break;
			}

			m_ModifiersManager.DeactivateModifier(modifier);
		}

		m_eAI_FactionModifiers = null;
	}

	void Expansion_OnDangerousAreaEnterServer(EffectArea area, EffectTrigger trigger)
	{
	}

	void Expansion_OnDangerousAreaExitServer(EffectArea area, EffectTrigger trigger)
	{
	}

	static set<PlayerBase> Expansion_GetInCircle(vector center, float radius)
	{
		set<PlayerBase> players = new set<PlayerBase>;

		auto node = s_Expansion_AllPlayers.m_Head;

		while (node)
		{
			if (Math.IsPointInCircle(center, radius, node.m_Value.GetPosition()))
				players.Insert(node.m_Value);

			node = node.m_Next;
		}

		return players;
	}

	/**
	 * @brief Check if other player is considered a helper.
	 * 
	 * Currently, AI that are friendly towards the player as well as guards not actively threatened by the player are considered helpers.
	 */
	override bool Expansion_IsHelper(PlayerBase other, bool checkIfWeAreHelper = false)
	{
		if (super.Expansion_IsHelper(other, checkIfWeAreHelper))
			return true;

		eAIBase ai;
		if (Class.CastTo(ai, other))
		{
			if (!ai.PlayerIsEnemy(this))
				return true;

			if (ai.GetGroup().GetFaction().IsGuard() && ai.eAI_GetTargetThreat(GetTargetInformation()) < 0.4)
				return true;
		}
		else if (checkIfWeAreHelper && Class.CastTo(ai, this))
		{
			if (!ai.PlayerIsEnemy(other))
				return true;

			if (ai.GetGroup().GetFaction().IsGuard() && ai.eAI_GetTargetThreat(other.GetTargetInformation()) < 0.4)
				return true;
		}

		return false;
	}
};
