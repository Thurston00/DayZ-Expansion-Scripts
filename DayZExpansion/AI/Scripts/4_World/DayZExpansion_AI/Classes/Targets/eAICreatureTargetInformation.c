class eAICreatureTargetInformation: eAIEntityTargetInformation
{
	private AnimalBase m_Creature;

	void eAICreatureTargetInformation(EntityAI target)
	{
		Class.CastTo(m_Creature, target);
	}

	override vector GetAimOffset(eAIBase ai = null)
	{
		vector pos;

		int index = m_Creature.GetBoneIndexByName("Chest");
		if (index > -1)
			pos = m_Creature.GetBonePositionWS(index);
		else
			pos = m_Creature.GetCenter();

		pos = pos - m_Creature.GetPosition();

	#ifdef DIAG_DEVELOPER
		if (EXTrace.AI && ai)
			ai.Expansion_DebugObject(1234567890, m_Creature.GetPosition() + pos, "ExpansionDebugSphereSmall_Orange", m_Creature.GetDirection(), ai.GetPosition() + "0 1.5 0", 3.0, ShapeFlags.NOZBUFFER);
	#endif

		return pos;
	}

	override float CalculateThreat(eAIBase ai = null)
	{
		if (m_Creature.IsDamageDestroyed())
			return 0.0;

		if (!m_Creature.Expansion_IsDanger())
			return 0.0;

		float levelFactor = 0.5;

		if (ai)
		{
#ifdef EXTRACE_DIAG
			auto hitch = new EXHitch(ai.ToString() + " eAICreatureTargetInformation::CalculateThreat ", 20000);
#endif

			// the further away the creature, the less likely it will be a threat
			float distance = GetDistance(ai, true) + 0.1;
			levelFactor = 10 / distance;
			if (levelFactor > 1.0)
				levelFactor = Math.Pow(levelFactor, 2.0);

			if (levelFactor >= 0.4)
			{
				levelFactor *= 2.0;
				auto hands = ai.GetHumanInventory().GetEntityInHands();
				if (hands && hands.IsWeapon())
					eAIPlayerTargetInformation.AdjustThreatLevelBasedOnWeapon(hands, distance, levelFactor);
			}

			levelFactor *= ai.Expansion_GetVisibility(distance);
		}

		return Math.Clamp(levelFactor, 0.0, 1000000.0);
	}

	override bool ShouldRemove(eAIBase ai = null)
	{
		return GetThreat(ai) <= 0.1;
	}

	override float GetMinDistance(eAIBase ai = null, float distance = 0.0)
	{
		if (ai && (ai.m_eAI_AcuteDangerTargetCount > 2 || ai.eAI_IsLowVitals() || (!ai.m_eAI_HasProjectileWeaponInHands && m_Creature.m_Expansion_IsBigGame && !ai.GetGroup().GetFaction().GetMeleeDamageMultiplier() < 100)))
			return 100.0;  //! Flee

		return m_MinDistance;
	}

	override bool ShouldAvoid(eAIBase ai = null, float distance = 0.0)
	{
		if (ai && !ai.IsRaised() && GetMinDistance(ai, distance) > 0.0)
			return true;

		return false;
	}
};
