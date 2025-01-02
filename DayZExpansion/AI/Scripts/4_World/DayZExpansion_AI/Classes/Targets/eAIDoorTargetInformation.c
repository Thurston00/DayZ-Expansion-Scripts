class eAIDoorTargetInformation: eAIEntityTargetInformation
{
	protected BuildingBase m_Building;
	protected int m_DoorIndex;
	protected vector m_Position;

	void eAIDoorTargetInformation(EntityAI target, int doorIndex, vector position)
	{
		Class.CastTo(m_Building, target);
		m_DoorIndex = doorIndex;
		m_Position = position;
	}

	override bool IsInanimate()
	{
		return true;
	}

	override vector GetPosition(eAIBase ai = null, bool actual = false)
	{
		return m_Position;
	}

	override bool IsActive()
	{
		if (!super.IsActive())
			return false;

		return m_Building.GetAllowDamage();
	}

	override float CalculateThreat(eAIBase ai = null)
	{
		if (!m_Building.IsDoorLocked(m_DoorIndex))
			return 0.0;

		if (ai)
		{
			//! If AI was recently hit, ignore this door
			if (GetGame().GetTickTime() - ai.m_eAI_LastHitTime < 3.0)
				return 0.0;

			float distance = Math.Max(GetDistance(ai), 1.0);
			float levelFactor = Math.Pow(2.281109 / distance, 10.0);  //! 0.4 at 2.5m, 3814.7 at 1m

			float threat = ai.GetThreatToSelf(true);
			if (levelFactor >= 0.4 && levelFactor < threat)
			{
				//! If current AI target is not a door and path to target is blocked,
				//! increase threat lvl so AI targets this door
				eAITarget target = ai.GetTarget();
				if (target && !target.info.IsInherited(eAIDoorTargetInformation))
				{
					auto pathHandler = ai.GetPathFinding();
					if (pathHandler.m_IsBlocked)
						levelFactor = threat * 2.0;
				}
			}

			return levelFactor;
		}

		return 0.0;
	}

	override bool ShouldRemove(eAIBase ai = null)
	{
		return GetThreat(ai) <= 0.1;
	}

	override float GetMinDistance(eAIBase ai = null, float distance = 0.0)
	{
		return 1.0;
	}
};
