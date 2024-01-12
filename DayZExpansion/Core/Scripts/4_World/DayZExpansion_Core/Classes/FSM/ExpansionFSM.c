typedef Class ExpansionFSMOwnerType;

class ExpansionFSM
{
	static const int EXIT = 0;
	static const int CONTINUE = 1;
	static const int RESTART = 2;

	private autoptr array<ref ExpansionState> m_States;
	private autoptr array<ref ExpansionTransition> m_Transitions;

	private ExpansionState m_CurrentState;
	private ExpansionState m_ParentState;

	protected ExpansionFSMOwnerType m_Owner;

	protected string m_Name;
	protected string m_DefaultState;

	int m_NoTransitionCount;

	void ExpansionFSM(ExpansionFSMOwnerType owner, ExpansionState parentState)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "ExpansionFSM").Add(parentState);
		#endif

		m_Owner = owner;
		m_ParentState = parentState;

		m_States = new array<ref ExpansionState>();
		m_Transitions = new array<ref ExpansionTransition>();
	}

	#ifdef CF_DEBUG
	bool CF_OnDebugUpdate(CF_Debug instance, CF_DebugUI_Type type)
	{
		int i;

		instance.Add("Active State", m_CurrentState);
		if (m_CurrentState)
		{
			instance.IncrementTab();
			instance.Add(m_CurrentState);
			instance.DecrementTab();
		}

		instance.Add("Transitions", m_Transitions.Count());
		for (i = 0; i < m_Transitions.Count(); i++)
			instance.Add(m_Transitions[i]);

		return false;
	}
	#endif

	ExpansionFSMOwnerType GetOwner()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetOwner");
		#endif

		return m_Owner;
	}

	string GetName()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetName");
		#endif

		return m_Name;
	}

	ExpansionState GetParent()
	{
		return m_ParentState;
	}

	void AddState(ExpansionState state)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "AddState").Add(state);
		#endif

		m_States.Insert(state);
	}

	void AddTransition(ExpansionTransition transition)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "AddTransition").Add(transition);
		#endif

		m_Transitions.Insert(transition);
	}
	
	ExpansionState GetState()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "GetState");
		#endif

		return m_CurrentState;
	}

	ExpansionState GetState(string type)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetState").Add(type);
		#endif

		for (int i = 0; i < m_States.Count(); i++) if (m_States[i].ClassName() == type) return m_States[i];

		return null;
	}

	ExpansionState GetState(typename type)
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "GetState").Add(type.ToString());
		#endif

		for (int i = 0; i < m_States.Count(); i++) if (m_States[i].Type() == type) return m_States[i];

		return null;
	}

	bool IsInState(string name)
	{
		return m_CurrentState && m_CurrentState.GetName() == name;
	}
	
	bool StartDefault()
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_0(this, "StartDefault");
		#endif

		if (m_DefaultState == "") return Start();

		ExpansionState src = m_CurrentState;
		ExpansionState dst = GetState(m_DefaultState);

		if (src)
		{
			CF_Log.Debug("StartDefault - Exiting state: " + src);
			src.OnExit("", true, dst);
		}
	
		m_CurrentState = dst;
		
		if (m_CurrentState)
		{
			CF_Log.Debug("StartDefault - Starting state: " + m_CurrentState);
			m_CurrentState.OnEntry("", src);
			return true;
		}
		
		CF_Log.Warn("StartDefault - No valid state found.");
		
		return false;
	}

	bool Start(string e = "")
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "Start").Add(e);
		#endif

		ExpansionState new_state = FindSuitableTransition(m_CurrentState, "");

		ExpansionState src = m_CurrentState;
		ExpansionState dst = new_state;

		if (m_CurrentState && m_CurrentState != dst)
		{
			CF_Log.Debug("Start - Exiting state: " + m_CurrentState);
			m_CurrentState.OnExit(e, true, dst);
		}

		m_CurrentState = dst;

		if (m_CurrentState && src != m_CurrentState)
		{
			CF_Log.Debug("Start - Starting state: " + m_CurrentState);
			m_CurrentState.OnEntry(e, src);
			return true;
		}

		CF_Log.Warn("Start - No valid state found.");

		return false;
	}

	bool Abort(string e = "")
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_1(this, "Abort").Add(e);
		#endif

		if (m_CurrentState)
		{
			CF_Log.Debug("Abort - Exiting state: " + m_CurrentState);
			m_CurrentState.OnExit(e, true, null);
			return true;
		}

		return false;
	}

	/**
	 * @return EXIT Tell the parent FSM that the child FSM is complete
	 * @return CONTINUE Tell the parent FSM that the child FSM is still running
	 */
	int Update(float pDt, int pSimulationPrecision)
	{
#ifdef DIAG
		auto trace = EXTrace.Profile(EXTrace.AI, this);
#endif

		if (!m_Owner)
			return EXIT;

		//CF_Log.Debug("m_CurrentState: %1", "" + m_CurrentState);

		if (m_CurrentState)
		{
			switch (m_CurrentState.OnUpdate(pDt, pSimulationPrecision))
			{
				case RESTART:
					m_CurrentState.OnEntry("", m_CurrentState);
					return CONTINUE;

				case CONTINUE:
					return CONTINUE;
			}
		}
		
		ExpansionState new_state = FindSuitableTransition(m_CurrentState, "");
		if (new_state && m_CurrentState == new_state)
		{
			return CONTINUE;
		}

		ExpansionState src = m_CurrentState;

		if (m_CurrentState) m_CurrentState.OnExit("", false, new_state);

		m_CurrentState = new_state;

		if (m_CurrentState == null)
		{
			if (src)
				CF_Log.Info("State transition exit " + src);
			return EXIT;
		}
		
		CF_Log.Info("State transition " + src + " -> " + m_CurrentState.GetName());

		m_CurrentState.OnEntry("", src);

		return CONTINUE;
	}
	
	ExpansionState FindSuitableTransition(ExpansionState s, string e = "")
	{
		#ifdef EAI_TRACE
		auto trace = CF_Trace_2(this, "FindSuitableTransition").Add(s).Add(e);
		#endif

		// returns tuple as a valid destination can still be null

		//TODO: store a reference to the transitions inside the state for that state

		foreach (auto t: m_Transitions)
		{
			if ((t.GetSource() == s || t.GetSource() == null) && (e == "" || t.GetEvent() == e))
			{
				switch (t.Guard())
				{
				case ExpansionTransition.SUCCESS:
#ifdef EAI_DEBUG_TRANSITION
					if (s != t.GetDestination())
						EXPrint(m_Owner.ToString() + " transition " + s + " -> " + t.GetDestination());
					m_NoTransitionCount = 0;
#endif
					return t.GetDestination();
				case ExpansionTransition.FAIL:
					break;
				}
			}
#ifdef EAI_DEBUG_TRANSITION
			if (m_NoTransitionCount > 0)
				EXPrint(m_Owner.ToString() + " not a suitable transition: " + t.GetSource() + " -> " + t.GetDestination());
#endif
		}

#ifdef EAI_DEBUG_TRANSITION
		if (m_NoTransitionCount == 0)
		{
			m_NoTransitionCount++;
			EXPrint(m_Owner.ToString() + " no suitable transition found in " + m_Transitions.Count() + " transitions!");
		}
		else
		{
			m_NoTransitionCount = -1;
		}
#endif

		return null;
	}
};
