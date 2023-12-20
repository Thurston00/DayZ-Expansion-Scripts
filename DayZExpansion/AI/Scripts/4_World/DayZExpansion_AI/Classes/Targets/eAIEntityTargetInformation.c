class eAIEntityTargetInformation: eAITargetInformation
{
	private EntityAI m_Target;
	private string m_TargetDebugName;

	void eAIEntityTargetInformation(EntityAI target)
	{
		m_Target = target;
		m_TargetDebugName = "" + m_Target;  //! Useful for logging even after entity has been deleted
	}

	override string GetDebugName()
	{
		string str = super.GetDebugName();
		
		str += ", ";
		str += "entity=" + GetEntityDebugName();

		return str;
	}

	override EntityAI GetEntity()
	{
		return m_Target;
	}

	override string GetEntityDebugName()
	{
		string str = m_TargetDebugName;

		if (!m_Target)
			str += "(deleted)";
		
		return str;
	}

	override bool IsActive()
	{
		if (!m_Target)
			return false;

		return !m_Target.IsDamageDestroyed();
	}

	override vector GetPosition(eAIBase ai = null, bool actual = false)
	{
		if (actual || !ai)
			return m_Target.GetPosition();

		auto state = ai.eAI_GetTargetInformationState(this);
		state.UpdatePosition();

		return state.m_SearchPosition;
	}

	override float GetThreat(eAIBase ai = null, out eAITargetInformationState state = null)
	{
		if (!IsActive())
			return 0.0;

		return super.GetThreat(ai, state);
	}

	override float GetDistance(eAIBase ai, bool actual = false)
	{
		vector position = GetPosition(ai, actual);
		return vector.Distance(ai.GetPosition(), position);
	}

	override float GetDistanceSq(eAIBase ai, bool actual = false)
	{
		vector position = GetPosition(ai, actual);
		return vector.DistanceSq(ai.GetPosition(), position);
	}
};

class eAIEntity<Class T>
{
	static eAIEntityTargetInformation GetTargetInformation(T entity)
	{
		return entity.GetTargetInformation();
	}
};
