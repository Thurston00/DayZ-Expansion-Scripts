// This class handles the inputs from the eAICommandMenu locally and shoots RPCs to the server.
class eAICommandManagerClient : eAICommandManager
{
	ref ExpansionRPCManager m_Expansion_RPCManager;

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

		m_Expansion_RPCManager.RegisterServer("RPC_SetWaypoint");
		m_Expansion_RPCManager.RegisterServer("RPC_ExportPatrol");
		m_Expansion_RPCManager.RegisterServer("RPC_ClearWaypoints");

		m_Expansion_RPCManager.RegisterServer("RPC_DumpState");
	}

	override bool Send(eAICommands cmd)
	{
		switch (cmd)
		{
			case eAICommands.DEB_SPAWNALLY:
			case eAICommands.DEB_SPAWNSENTRY:
			case eAICommands.DEB_SPAWNGUARD:
			case eAICommands.DEB_SPAWNPASSIVE:
			case eAICommands.DEB_SPAWNSHAMAN:
				m_Expansion_RPCManager.SendRPC("RPC_SpawnAI", new Param2<int, vector>(cmd, ExpansionStatic.GetCursorHitPos()));
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
			
			case eAICommands.DEB_TARGET_CREATE:
				eAIBase.Get(0).CreateDebugApple();
				return true;
			
			case eAICommands.DEB_TARGET_DESTROY:
				eAIBase.Get(0).DestroyDebugApple();
				return true;

			case eAICommands.STA_DUMP:
				auto rpc = m_Expansion_RPCManager.CreateRPC("RPC_DumpState");
				Object target;
				ExpansionStatic.GetCursorHitPos(target);
				rpc.Expansion_Send(target, true);
				return true;
		}
		
		return false;
	}

	//! @param owner Who is the manager of this AI
	//! @param formOffset Where should this AI follow relative to the formation?
	eAIBase SpawnAI_Helper(PlayerBase owner, string loadout = "HumanLoadout.json")
	{
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto group = owner.GetGroup();
		if (group)
		{
			owner.Expansion_SetFormerGroup(group);
			loadout = group.GetFaction().GetDefaultLoadout();
		}
		else
		{
			group = eAIGroup.GetGroupByLeader(owner);
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
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		return SpawnAIEx(pos, loadout);
	}
	
	eAIBase SpawnAI_Patrol(vector pos, string loadout = "HumanLoadout.json")
	{
	#ifdef DIAG
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
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		int command;
		if (!ctx.Read(command)) return;

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

		eAIBase sentry;
		switch (command)
		{
			case eAICommands.DEB_SPAWNALLY:
				SpawnAI_Helper(player);
				break;
			case eAICommands.DEB_SPAWNSENTRY:
				sentry = SpawnAI_Sentry(pos);
				sentry.GetGroup().SetFaction(new eAIFactionMercenaries());
				break;
			case eAICommands.DEB_SPAWNGUARD:
				sentry = SpawnAI_Sentry(pos);
				sentry.GetGroup().SetFaction(new eAIFactionGuards());
				break;
			case eAICommands.DEB_SPAWNPASSIVE:
				sentry = SpawnAI_Sentry(pos);
				sentry.GetGroup().SetFaction(new eAIFactionPassive());
				break;
			case eAICommands.DEB_SPAWNSHAMAN:
				sentry = SpawnAI_Sentry(pos);
				sentry.GetGroup().SetFaction(new eAIFactionShamans());
				break;
		}
	}
	
	// Server Side: This RPC spawns a zombie. It's actually not the right way to do it. But it's only for testing.
	// BUG: this has sometimes crashed us before. Not sure why yet.
	void RPC_SpawnZombie(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
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
	#ifdef DIAG
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
	#ifdef DIAG
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
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		if (GetGame().IsMultiplayer())
		{
			if (!GetExpansionSettings().GetAI().IsAdmin(sender))
				return;
		}
	
		eAIGroup.Admin_ClearAllAI();
	}
	
	void RPC_ReqFormRejoin(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		g.SetFormationState(eAIGroupFormationState.IN);
		g.SetWaypointBehaviourAuto(eAIWaypointBehavior.ONCE);
	}
	
	void RPC_ReqFormStop(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
		g.SetFormationState(eAIGroupFormationState.NONE);
		g.SetWaypointBehaviour(eAIWaypointBehavior.HALT);
	}
	
	void RPC_ReqFormationChange(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
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
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		vector position;
		if (!ctx.Read(position))
			return;

		position = ExpansionAISpawnBase.GetPlacementPosition(position);

	#ifdef DIAG
		EXTrace.Add(trace, position);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		g.AddWaypoint(position);
		g.SetWaypointBehaviourAuto(eAIWaypointBehavior.ONCE);

		eAIBase ai;
		if (Class.CastTo(ai, g.GetFormationLeader()))
			ai.SetMovementSpeedLimit(2);
	}

	void RPC_ExportPatrol(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
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
	#ifdef DIAG
		auto trace = EXTrace.Start(EXTrace.AI, this);
	#endif

		auto player = PlayerBase.ExpansionGetPlayerByIdentity(sender);

		eAIGroup g = eAIGroup.GetGroupByLeader(player, false);

		eAIBase ai;
		if (Class.CastTo(ai, g.GetFormationLeader()))
			ai.SetMovementSpeedLimit(3);

		g.ClearWaypoints();
	}

	void RPC_DumpState(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
	#ifdef DIAG
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
			eAIGroup g = eAIGroup.GetGroupByLeader(player, false);
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
