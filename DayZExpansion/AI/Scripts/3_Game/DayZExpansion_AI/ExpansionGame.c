modded class ExpansionGame
{
	protected ref eAICommandManager m_CommandManager;

	private bool m_InGroup;

	eAICommandManager GetCommandManager()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetCommandManager");
#endif

		return m_CommandManager;
	}

	/**
	 * @note Client only
	 */
	bool InGroup()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "InGroup");
#endif

		return m_InGroup;
	}

	/**
	 * @note Client only
	 */
	void SetInGroup(bool group)
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "SetInGroup");
#endif

		m_InGroup = group;
	}
};
