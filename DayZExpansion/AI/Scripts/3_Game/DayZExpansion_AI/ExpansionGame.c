modded class ExpansionGame
{
	protected ref eAICommandManager m_CommandManager;

	private bool m_InGroup;

	Object m_FirearmFXSource;
	Object m_FirearmFXHitObject;

	void ExpansionGame()
	{
		m_RPCManager.RegisterClient("RPC_SetInGroup");
	}

	eAICommandManager GetCommandManager()
	{
#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetCommandManager");
#endif

		return m_CommandManager;
	}

	ExpansionRPCManager GetRPCManager()
	{
		return m_RPCManager;
	}

	void RPC_SetInGroup(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		bool group;
		if (ctx.Read(group))
			SetInGroup(group);
	}

	/**
	 * @note Client only
	 */
	bool InGroup()
	{
		return m_InGroup;
	}

	/**
	 * @note Client only
	 */
	void SetInGroup(bool group)
	{
		m_InGroup = group;
	}

	void SpectateAI(PlayerIdentity sender, Object target, ParamsReadContext ctx);

	void FixAIFirearmFX(inout Object source, inout Object directHit, inout int componentIndex, inout string surface, inout vector pos, inout vector surfNormal, inout vector exitPos, inout vector inSpeed, inout vector outSpeed, inout bool isWater, inout bool deflected, inout string ammoType);
};
