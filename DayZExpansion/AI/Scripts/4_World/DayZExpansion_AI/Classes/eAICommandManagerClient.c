// This class handles the inputs from the eAICommandMenu locally and shoots RPCs to the server.
class eAICommandManagerClient : eAICommandManager
{
	ref ExpansionRPCManager m_Expansion_RPCManager;

	int m_MovementSpeedLimit = eAIMovementSpeed.SPRINT;  //! Client
	bool m_UnlimitedReload;  //! Server

	void eAICommandManagerClient()
	{
		m_Expansion_RPCManager = new ExpansionRPCManager(this);

		m_Expansion_RPCManager.RegisterServer("RPC_SpawnAI");
		m_Expansion_RPCManager.RegisterServer("RPC_SpawnZombie");
		m_Expansion_RPCManager.RegisterServer("RPC_SpawnWolf");
		m_Expansion_RPCManager.RegisterServer("RPC_SpawnBear");
		m_Expansion_RPCManager.RegisterServer("RPC_ClearAllAI");

		m_Expansion_RPCManager.RegisterServer("RPC_ReqFormationChange");
		m_Expansion_RPCManager.RegisterServer("RPC_ReqFormRejoin");
		m_Expansion_RPCManager.RegisterServer("RPC_ReqFormStop");
		m_Expansion_RPCManager.RegisterServer("RPC_ReqFormFlank");
		m_Expansion_RPCManager.RegisterServer("RPC_ReqFormRoam");

		m_Expansion_RPCManager.RegisterServer("RPC_SetWaypoint");
		m_Expansion_RPCManager.RegisterServer("RPC_ExportPatrol");
		m_Expansion_RPCManager.RegisterServer("RPC_ClearWaypoints");

		m_Expansion_RPCManager.RegisterServer("RPC_SetMovementSpeed");

		m_Expansion_RPCManager.RegisterServer("RPC_DumpState");
		m_Expansion_RPCManager.RegisterServer("RPC_UnlimitedReload");
		m_Expansion_RPCManager.RegisterServer("RPC_DebugObjects");
		m_Expansion_RPCManager.RegisterServer("RPC_DebugDamage");
	}

	override bool Send(eAICommands cmd)
	{
		ExpansionScriptRPC rpc;
		Object target;

		switch (cmd)
		{
			case eAICommands.DEB_SPAWNALLY:
			case eAICommands.DEB_SPAWNSENTRY:
			case eAICommands.DEB_SPAWNGUARD:
			case eAICommands.DEB_SPAWNPASSIVE:
			case eAICommands.DEB_SPAWNSHAMAN:
			case eAICommands.DEB_SPAWNYEET:
				m_Expansion_RPCManager.SendRPC("RPC_SpawnAI", new Param3<int, vector, int>(cmd, ExpansionStatic.GetCursorHitPos(), m_MovementSpeedLimit));
				return true;
			
			case eAICommands.DEB_CLEARALL:
				m_Expansion_RPCManager.SendRPC("RPC_ClearAllAI");
				return true;
			
			case eAICommands.DEB_SPAWNZOM:
				m_Expansion_RPCManager.SendRPC("RPC_SpawnZombie", new Param1<vector>(ExpansionStatic.GetCursorHitPos()));
				return true;
			case eAICommands.DEB_SPAWNWOLF:
				m_Expansion_RPCManager.SendRPC("RPC_SpawnWolf", new Param1<vector>(ExpansionStatic.GetCursorHitPos()));
				return true;
			case eAICommands.DEB_SPAWNBEAR:
				m_Expansion_RPCManager.SendRPC("RPC_SpawnBear", new Param1<vector>(ExpansionStatic.GetCursorHitPos()));
				return true;

			case eAICommands.DEB_UNLIMITEDRELOAD:
				m_Expansion_RPCManager.SendRPC("RPC_UnlimitedReload");
				return true;

			case eAICommands.DEB_DBGOBJECTS:
				m_Expansion_RPCManager.SendRPC("RPC_DebugObjects");
				return true;

			case eAICommands.DEB_DBGDAMAGE:
				m_Expansion_RPCManager.SendRPC("RPC_DebugDamage");
				return true;

			case eAICommands.FOR_VEE:
			case eAICommands.FOR_INVVEE:
			case eAICommands.FOR_FILE:
			case eAICommands.FOR_INVFILE:
			case eAICommands.FOR_WALL:
			case eAICommands.FOR_COL:
			case eAICommands.FOR_INVCOL:
			case eAICommands.FOR_CIRCLE:
			case eAICommands.FOR_CIRCLEDOT:
			case eAICommands.FOR_STAR:
			case eAICommands.FOR_STARDOT:
				m_Expansion_RPCManager.SendRPC("RPC_ReqFormationChange", new Param1<int>(cmd));
				return true;
			
			case eAICommands.MOV_STOP:
				m_Expansion_RPCManager.SendRPC("RPC_ReqFormStop");
				return true;
			
			case eAICommands.MOV_FLANK:
				m_Expansion_RPCManager.SendRPC("RPC_ReqFormFlank");
				return true;
			
			case eAICommands.MOV_ROAM:
				m_Expansion_RPCManager.SendRPC("RPC_ReqFormRoam");
				return true;
			
			case eAICommands.MOV_RTF:
				m_Expansion_RPCManager.SendRPC("RPC_ReqFormRejoin");
				return true;
			
			case eAICommands.MOV_SETWP:
				vector position = ExpansionStatic.GetCursorHitPos();
				if (position != vector.Zero)
					m_Expansion_RPCManager.SendRPC("RPC_SetWaypoint", new Param1<vector>(position));
				return true;
			
			case eAICommands.DEB_EXPORTPATROL:
				m_Expansion_RPCManager.SendRPC("RPC_ExportPatrol");
				return true;
			
			case eAICommands.MOV_CLEARWP:
				m_Expansion_RPCManager.SendRPC("RPC_ClearWaypoints");
				return true;
			
			case eAICommands.MOV_WALK:
			case eAICommands.MOV_JOG:
			case eAICommands.MOV_SPRINT:
				m_MovementSpeedLimit = cmd - eAICommands.MOV_WALK + 1;
				m_Expansion_RPCManager.SendRPC("RPC_SetMovementSpeed", new Param1<int>(m_MovementSpeedLimit));
				return true;
			
			case eAICommands.DEB_TARGET_CREATE:
				eAIBase.Get(0).CreateDebugApple();
				return true;
			
			case eAICommands.DEB_TARGET_DESTROY:
				eAIBase.Get(0).DestroyDebugApple();
				return true;

			case eAICommands.STA_DUMP:
				rpc = m_Expansion_RPCManager.CreateRPC("RPC_DumpState");
				rpc.Expansion_Send(GetAIAtCursorOrNearest(), true);
				return true;

			case eAICommands.DEB_SPECTATE:
				GetDayZGame().GetExpansionGame().SpectateAI(null, GetAIAtCursorOrNearest(), null);
				return true;
		}
		
		return false;
	}

	eAIBase GetAIAtCursorOrNearest()
	{
		Object target;
		ExpansionStatic.GetCursorHitPos(target);

		eAIBase targetAI;
		if (!Class.CastTo(targetAI, target))
		{
			//! Dump state of nearest AI
			vector cameraPosition = GetGame().GetCurrentCameraPosition();
			float minDistSq = float.MAX;
			array<eAIBase> allAI = eAIBase.eAI_GetAll();
			foreach (eAIBase ai: allAI)
			{
				float distSq = vector.DistanceSq(cameraPosition, ai.GetPosition());
				if (distSq < minDistSq)
				{
					minDistSq = distSq;
					targetAI = ai;
				}
			}
		}

		return targetAI;
	}

	eAIBase SpawnAI_Helper(PlayerBase leader, string loadout = "HumanLoadout.json")
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto group = leader.GetGroup();
		if (group)
		{
			leader.Expansion_SetFormerGroup(group);
			loadout = group.GetFaction().GetDefaultLoadout();
		}
		else
		{
			group = eAIGroup.GetGroupByLeader(leader);
		}

		auto form = group.GetFormation();
		vector pos = ExpansionAISpawnBase.GetPlacementPosition(form.ToWorld(form.GetPosition(group.Count())));
		auto ai = SpawnAIEx(pos, loadout);

		if (ai)
			ai.SetGroup(group);

		return ai;
	}
	
	eAIBase SpawnAIEx(vector pos, string loadout = "HumanLoadout.json")
	{
		eAIBase ai;
		if (Class.CastTo(ai, GetGame().CreateObject(GetRandomAI(), pos)))
			ExpansionHumanLoadout.Apply(ai, loadout, true);

		EXTrace.Print(EXTrace.AI, null, "Spawned AI at " + pos);

		return ai;
	}

	eAIBase SpawnAI_Sentry(vector pos, string loadout = "WestLoadout.json")
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		return SpawnAIEx(pos, loadout);
	}
	
	eAIBase SpawnAI_Patrol(vector pos, string loadout = "HumanLoadout.json")
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		return SpawnAIEx(pos, loadout);
	}
	
	vector GetEntitySpawnPosition(DayZPlayer player)
	{
		vector position, direction, rotation;
		player.GetCurrentCameraTransform(position, direction, rotation);
		position = position + direction * 20;
		float surfaceY = GetGame().SurfaceY(position[0], position[2]);
		if (surfaceY > position[1])
			position[1] = surfaceY;
		return ExpansionAISpawnBase.GetPlacementPosition(position);
	}

	// Server Side: This RPC spawns a helper AI next to the player, and tells them to join the player's formation.
	void RPC_SpawnAI(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		int command;
		if (!ctx.Read(command)) return;

		vector pos;
		if (!ctx.Read(pos)) return;

		int speed;
		if (!ctx.Read(speed)) return;

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		if (pos == vector.Zero)
			pos = GetEntitySpawnPosition(player);
		else
			pos = ExpansionAISpawnBase.GetPlacementPosition(pos);

		eAIBase ai;
		switch (command)
		{
			case eAICommands.DEB_SPAWNALLY:
				ai = SpawnAI_Helper(player);
				ai.eAI_SetSpeedLimitPreference(speed);
				break;
			case eAICommands.DEB_SPAWNSENTRY:
				ai = SpawnAI_Sentry(pos);
				ai.GetGroup().SetFaction(new eAIFactionMercenaries());
				break;
			case eAICommands.DEB_SPAWNGUARD:
				ai = SpawnAI_Sentry(pos);
				ai.GetGroup().SetFaction(new eAIFactionGuards());
				break;
			case eAICommands.DEB_SPAWNPASSIVE:
				ai = SpawnAI_Sentry(pos);
				ai.GetGroup().SetFaction(new eAIFactionPassive());
				break;
			case eAICommands.DEB_SPAWNSHAMAN:
				ai = SpawnAI_Sentry(pos);
				ai.GetGroup().SetFaction(new eAIFactionShamans());
				break;
			case eAICommands.DEB_SPAWNYEET:
				ai = SpawnAI_Sentry(pos, "YeetBrigadeLoadout");
				ai.GetGroup().SetFaction(new eAIFactionYeetBrigade());
				break;
		}
	}
	
	void RPC_SpawnZombie(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		vector pos;
		if (!ctx.Read(pos)) return;

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		if (pos == vector.Zero)
			pos = GetEntitySpawnPosition(player);
		else
			pos = ExpansionAISpawnBase.GetPlacementPosition(pos);

		GetGame().CreateObject(ExpansionStatic.GetWorkingZombieClasses().GetRandomElement(), pos, false, true, true);
	}

	void RPC_SpawnWolf(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		vector pos;
		if (!ctx.Read(pos)) return;

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		if (pos == vector.Zero)
			pos = GetEntitySpawnPosition(player);
		else
			pos = ExpansionAISpawnBase.GetPlacementPosition(pos);
		
		GetGame().CreateObject("Animal_CanisLupus_Grey", pos, false, true, true);
	}

	void RPC_SpawnBear(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		vector pos;
		if (!ctx.Read(pos)) return;

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		if (pos == vector.Zero)
			pos = GetEntitySpawnPosition(player);
		else
			pos = ExpansionAISpawnBase.GetPlacementPosition(pos);
	
		GetGame().CreateObject("Animal_UrsusArctos", pos, false, true, true);
	}
	
	// Server Side: Delete AI.
	void RPC_ClearAllAI(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}
	
		eAIGroup.Admin_ClearAllAI();
	}
	
	void RPC_UnlimitedReload(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}
	
		eAIBase.s_eAI_UnlimitedReload = !eAIBase.s_eAI_UnlimitedReload;

		string onOff;
		if (eAIBase.s_eAI_UnlimitedReload)
			onOff = "ON";
		else
			onOff = "OFF";

		ExpansionNotification("EXPANSION AI", "Unlimited reload " + onOff).Info(sender);
	}
	
	void RPC_DebugObjects(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}
	
		DayZPlayerImplement.s_Expansion_DebugObjects_Enabled = !DayZPlayerImplement.s_Expansion_DebugObjects_Enabled;

		string onOff;
		if (DayZPlayerImplement.s_Expansion_DebugObjects_Enabled)
		{
			onOff = "ON";

			if (GetGame().IsMultiplayer())
				onOff += "<br/>\nWARNING: Debug objects are server-side and thus visible to anyone!";
		}
		else
		{
			onOff = "OFF";

			CF_DoublyLinkedNode_WeakRef<PlayerBase> node = PlayerBase.s_Expansion_AllPlayers.m_Head;
			while (node)
			{
				node.m_Value.Expansion_DeleteDebugObjects();
				node = node.m_Next;
			}
		}

		ExpansionNotification("EXPANSION AI", "Debug objects " + onOff).Info(sender);
	}
	
	void RPC_DebugDamage(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}
	
		DayZPlayerImplement.s_eAI_DebugDamage = !DayZPlayerImplement.s_eAI_DebugDamage;

		string onOff;
		if (DayZPlayerImplement.s_eAI_DebugDamage)
			onOff = "ON";
		else
			onOff = "OFF";

		ExpansionNotification("EXPANSION AI", "Debug damage " + onOff).Info(sender);
	}
	
	void RPC_ReqFormRejoin(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		g.SetFormationState(eAIGroupFormationState.IN);
		g.SetWaypointBehaviourAuto(eAIWaypointBehavior.ONCE);
	}
	
	void RPC_ReqFormStop(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		g.SetFormationState(eAIGroupFormationState.NONE);
		g.SetWaypointBehaviour(eAIWaypointBehavior.HALT);

		eAIBase ai;
		for (int i = 0; i < g.Count(); i++)
		{
			if (Class.CastTo(ai, g.GetMember(i)))
				ai.eAI_SetLootingBehavior(eAILootingBehavior.DEFAULT);
		}
	}
	
	void RPC_ReqFormFlank(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		eAIBase ai;
		for (int i = 0; i < g.Count(); i++)
		{
			if (Class.CastTo(ai, g.GetMember(i)) && ai.GetTarget() && ai.eAI_HasLOS(ai.GetTarget()) && ai.eAI_IsInFlankRange(ai.GetTarget()))
			{
				g.SetFormationState(eAIGroupFormationState.FLANK);
				return;
			}
		}

		ExpansionNotification("EXPANSION AI", "No valid targets to flank").Info(sender);
	}
	
	void RPC_ReqFormRoam(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		g.SetFormationState(eAIGroupFormationState.IN);
		g.SetWaypointBehaviour(eAIWaypointBehavior.ROAMING);

		eAIBase ai;
		for (int i = 0; i < g.Count(); i++)
		{
			if (Class.CastTo(ai, g.GetMember(i)))
				ai.eAI_SetLootingBehavior(eAILootingBehavior.ALL);
		}
	}
	
	void RPC_ReqFormationChange(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		int command;
        if (!ctx.Read(command)) return;

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		eAIFormation newForm;
		switch (command)
		{
			case eAICommands.FOR_INVVEE:
				newForm = new eAIFormationInvVee(g);
				break;
			case eAICommands.FOR_FILE:
				newForm = new eAIFormationFile(g);
				break;
			case eAICommands.FOR_INVFILE:
				newForm = new eAIFormationInvFile(g);
				break;
			case eAICommands.FOR_WALL:
				newForm = new eAIFormationWall(g);
				break;
			case eAICommands.FOR_COL:
				newForm = new eAIFormationColumn(g);
				break;
			case eAICommands.FOR_INVCOL:
				newForm = new eAIFormationInvColumn(g);
				break;
			case eAICommands.FOR_CIRCLE:
				newForm = new eAIFormationCircle(g);
				break;
			case eAICommands.FOR_CIRCLEDOT:
				newForm = new eAIFormationCircleDot(g);
				break;
			case eAICommands.FOR_STAR:
				newForm = new eAIFormationStar(g);
				break;
			case eAICommands.FOR_STARDOT:
				newForm = new eAIFormationStarDot(g);
				break;
			case eAICommands.FOR_VEE:
			default:
				newForm = new eAIFormationVee(g);
				break;
		}
		g.SetFormation(newForm);
	}

	void RPC_SetWaypoint(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		vector position;
		if (!ctx.Read(position))
			return;

		position = ExpansionAISpawnBase.GetPlacementPosition(position);

	#ifdef EXTRACE
		EXTrace.Add(trace, position);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		g.AddWaypoint(position);
		g.SetWaypointBehaviourAuto(eAIWaypointBehavior.ONCE);

		eAIBase ai;
		if (g.GetWaypoints().Count() == 1 && Class.CastTo(ai, g.GetFormationLeader()))
			ai.GetPathFinding().ForceRecalculate(true);
	}

	void RPC_ExportPatrol(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		eAIWaypointBehavior bhv = g.GetWaypointBehaviour();

		if (bhv == eAIWaypointBehavior.ONCE)
			bhv = eAIWaypointBehavior.ALTERNATE;

		string beh = typename.EnumToString(eAIWaypointBehavior, bhv);

		eAIFaction faction = g.GetFaction();

		ExpansionAIPatrol patrol = new ExpansionAIPatrol(g.Count(), "JOG", "SPRINT", beh, faction.GetName(), faction.GetDefaultLoadout());
		string formationClassName = g.GetFormation().ClassName();
		patrol.Formation = formationClassName.Substring(12, formationClassName.Length() - 12);
		patrol.FormationLooseness = 0.1;
		patrol.Waypoints = g.GetWaypoints();

	#ifdef SERVER
		ExpansionJsonFileParser<ExpansionAIPatrol>.Save("$profile:Expansion_AI_Patrol.json", patrol);
		ExpansionNotification("EXPANSION AI", "Patrol written to \"$profile:Expansion_AI_Patrol.json\".").Info(sender);
	#else
		GetGame().CopyToClipboard(ExpansionJsonFileParser<ExpansionAIPatrol>.AsString(patrol));
		ExpansionNotification("EXPANSION AI", "Patrol copied to clipboard.").Info();
	#endif
	}

	void RPC_ClearWaypoints(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		g.ClearWaypoints();
	}
	
	void RPC_SetMovementSpeed(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		int speed;
		if (!ctx.Read(speed))
			return;

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		
		eAIBase ai;

		DayZPlayerImplement formationLeader = g.GetFormationLeader();
		if (formationLeader != g.GetLeader())
		{
			if (Class.CastTo(ai, formationLeader))
			{
				ai.eAI_SetSpeedLimitPreference(speed);
				speed = eAIMovementSpeed.SPRINT;  //! Allow other members to catch up with leader
			}
		}

		for (int i = 0; i < g.Count(); i++)
		{
			if (Class.CastTo(ai, g.GetMember(i)) && ai != formationLeader)
				ai.eAI_SetSpeedLimitPreference(speed);
		}
	}

	void RPC_DumpState(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.AI, this, "" + target);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		TStringArray report;

		eAIBase ai;
		if (Class.CastTo(ai, target))
		{
			if (GetExpansionSettings().GetAI().IsAdmin(sender) || player.GetGroup() == ai.GetGroup())
				report = ai.eAI_DumpState(sender);
		}

		if (!report)
		{
			eAIGroup g = player.GetGroup();

			if (!g)
			{
				ExpansionNotification("EXPANSION AI", "No group and no AI selected, nothing to dump").Error(sender);
				return;
			}

			report = g.DumpState(sender);
		}

		string fileName = string.Format("$profile:Expansion_AI_State_%1.txt", ExpansionStatic.GetISODate(false));
		bool fileExisted = FileExist(fileName);

		FileHandle file = OpenFile(fileName, FileMode.APPEND);
		if (file)
		{
			if (fileExisted)
				FPrintln(file, "");

			FPrintln(file, string.Format("State dump started at %1", ExpansionStatic.GetISODateTime(false, " ", ":", true)));
		}

		int chatLineLimit = 100;
		if (report.Count() > chatLineLimit)
			chatLineLimit = 99;

		foreach (int i, string msg: report)
		{
			if (file)
				FPrintln(file, msg);

			if (i < chatLineLimit)
				GetGame().ChatMP(player, msg, "colorDefault");
		}

		if (report.Count() > chatLineLimit)
			GetGame().ChatMP(player, "WARNING: State dump is too large for chat", "colorAction");

		if (file)
		{
			CloseFile(file);
			ExpansionNotification("EXPANSION AI", string.Format("State dumped to %1", fileName)).Info(sender);
		}
	}
};
