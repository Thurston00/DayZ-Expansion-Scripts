/**
 * ExpansionTeleporterModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionTeleporterModuleRPC
{
	INVALID = 40100,
	RequestOpenTeleporterMenu,
	RequestTeleport,
	PlayTeleportSound
	COUNT
};

enum ExpansionTeleporterSound
{
	TELEPORT_ACTIVE = 1,
	TELEPORT_DESTINATION = 2
};

[CF_RegisterModule(ExpansionTeleporterModule)]
class ExpansionTeleporterModule: CF_ModuleWorld
{
	protected static ExpansionTeleporterModule s_Instance;
	static string s_TeleporterDataFolderPath = "$mission:expansion\\teleporter\\";

	protected ref ScriptInvoker m_TeleporterMenuInvoker; //! Client
	protected ref ScriptInvoker m_TeleporterMenuCallbackInvoker; //! Client

	protected ref map<int, ref ExpansionTeleportData> m_TeleporterData; //! Server
	protected ref ExpansionTeleportData m_TeleporterClientData;
	
#ifdef EXPANSION_NAMALSK_ADVENTURE
	protected ref map<int, ref array<string>> m_PlayerTeleporterMap;
#endif

	void ExpansionTeleporterModule()
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		s_Instance = this;
	}

	override void OnInit()
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		EnableMissionStart();
		EnableRPC();
	}

	protected void CreateDirectoryStructure()
	{
		if (!FileExist(s_TeleporterDataFolderPath))
			ExpansionStatic.MakeDirectoryRecursive(s_TeleporterDataFolderPath);
	}

	override void OnMissionStart(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		m_TeleporterData = new map<int, ref ExpansionTeleportData>;

		if (GetGame().IsServer() && GetGame().IsMultiplayer())
		{
			CreateDirectoryStructure();
			LoadTeleporterServerData();
		}

		if (GetGame().IsClient())
			ClientModuleInit();
	}

	protected void ClientModuleInit()
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (GetGame().IsClient())
		{
			if (!m_TeleporterMenuInvoker)
				m_TeleporterMenuInvoker = new ScriptInvoker(); //! Client

			if (!m_TeleporterMenuCallbackInvoker)
				m_TeleporterMenuCallbackInvoker = new ScriptInvoker(); //! Client
		}
	}

	protected void LoadTeleporterServerData()
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		array<string> teleporterFiles = ExpansionStatic.FindFilesInLocation(s_TeleporterDataFolderPath, ".json");
		if (teleporterFiles && teleporterFiles.Count() > 0)
		{
			foreach (string fileName: teleporterFiles)
			{
				GetTeleporterData(fileName, s_TeleporterDataFolderPath);
			}
		}
		else
		{
			CreateDefaultTeleporterData();
		}
	}

	protected void CreateDefaultTeleporterData()
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		ExpansionTeleportData teleporterData = new ExpansionTeleportData();
		teleporterData.SetID(1);
		teleporterData.SetDisplayName("Sebjan Reservoir");
	#ifdef EXPANSIONMODAI
		teleporterData.SetFaction("Resistance");
	#endif
		teleporterData.SetObjectPosition(Vector(6030.101563, 5.685052, 10047.874023));
		teleporterData.SetObjectOrientation(Vector(77.317390, 0, 0));

		ExpansionTeleportPosition teleportPos = new ExpansionTeleportPosition();
		teleportPos.SetData("Jalovisco Camp", "Resistance");
		teleportPos.AddPosition(Vector(6023.755371, 5.852886, 10041.519531), Vector(-81.172432, 0.000000, 0.000000));
		teleportPos.AddPosition(Vector(6021.498047, 5.871239, 10050.499023), Vector(-81.172432, 0.000000, 0.000000));

		teleporterData.AddTeleportPosition(teleportPos);

		AddTeleporterData(teleporterData);

		teleporterData.Save();
		teleporterData.SpawnTeleporter();

	#ifdef EXPANSION_NAMALSK_ADVENTURE
		teleporterData = new ExpansionTeleportData();
		teleporterData.SetID(2);
		teleporterData.SetDisplayName("Science Society");
	#ifdef EXPANSIONMODAI
		teleporterData.SetFaction("ScienceSociety");
	#endif
		teleporterData.SetObjectPosition(Vector(6030.101563, 5.685052, 10047.874023));
		teleporterData.SetObjectOrientation(Vector(77.317390, 0, 0));

		teleportPos = new ExpansionTeleportPosition();
		teleportPos.SetData("Science Society - Secret Base", "ScienceSociety");
		teleportPos.AddPosition(Vector(5079.959961, 2085.610107, 11720.700195), Vector(0.000000, 0.000000, 0.000000));
		teleportPos.AddPosition(Vector(5075.359863, 2085.610107, 11715.299805), Vector(0.000000, 0.000000, 0.000000));

		teleporterData.AddTeleportPosition(teleportPos);

		AddTeleporterData(teleporterData);

		teleporterData.Save();
		teleporterData.SpawnTeleporter();
	#endif
	}

	protected void GetTeleporterData(string fileName, string path)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		ExpansionTeleportData teleporterData = ExpansionTeleportData.Load(path + fileName);
		if (!teleporterData)
			return;

		AddTeleporterData(teleporterData);
		teleporterData.SpawnTeleporter(); //! Spawn the teleporter.
	}
	
#ifdef EXPANSION_NAMALSK_ADVENTURE
	void AddPlayerToTeleporter(int teleporterID, string playerUID)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);
		EXTrace.Add(trace, teleporterID);
		EXTrace.Add(trace, playerUID);
		
		array<string> playerUIDs = m_PlayerTeleporterMap[teleporterID];
		if (playerUIDs.Find(playerUID) == -1)
		{
			playerUIDs.Insert(playerUID);
			m_PlayerTeleporterMap[teleporterID] = playerUIDs;
		}
	}
	
	void RemovePlayerFromTeleporter(int teleporterID, string playerUID)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);
		
		array<string> playerUIDs = m_PlayerTeleporterMap[teleporterID];
		int foundIndex = -1;
		foundIndex = playerUIDs.Find(playerUID);
		
		if (foundIndex > -1)
		{
			playerUIDs.RemoveOrdered(foundIndex);
			m_PlayerTeleporterMap[teleporterID] = playerUIDs;
		}
	}
	
	bool CanUseTeleporter(int teleporterID, string playerUID)
	{
		array<string> playerUIDs = m_PlayerTeleporterMap[teleporterID];
		if (playerUIDs.Find(playerUID) > -1)
			return true;
		
		return false;
	}
	
	void OnTeleporterKeyCardUsed(Expansion_Teleporter_Big teleporterObj, Expansion_KeyCard_Teleporter keyCard)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);
		
		PlayerBase player = PlayerBase.Cast(keyCard.GetHierarchyRootPlayer());
		if (!player)
			return;
		
		int teleporterID = teleporterObj.GetTeleporterID();
		string playerUID = player.GetIdentity().GetId();
		if (teleporterID > -1)
			AddPlayerToTeleporter(teleporterID, playerUID);
		
		keyCard.OnCardUsed();
		
		int remaining;
		if (!teleporterObj.IsActive())
		{
			teleporterObj.SetActive(true);
			remaining = GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).GetRemainingTime(this.DeactivateTeleporter);
			if (remaining <= 0)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeactivateTeleporter, 30000, false, teleporterObj);
			}
		}
		else
		{
			remaining = GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).GetRemainingTime(this.DeactivateTeleporter);
			if (remaining > 0)
			{
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(this.DeactivateTeleporter);
				GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.DeactivateTeleporter, 30000, false, teleporterObj);
			}
		}
		
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(this.RemovePlayerFromTeleporter, 30000, false, teleporterID, playerUID);
	}
	
	void DeactivateTeleporter(Expansion_Teleporter_Big teleporterObj)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);
		
		if (teleporterObj.IsActive())
		{
			teleporterObj.SetActive(false);
		}
	}
#endif

	override int GetRPCMin()
	{
		return ExpansionTeleporterModuleRPC.INVALID;
	}

	override int GetRPCMax()
	{
		return ExpansionTeleporterModuleRPC.COUNT;
	}

	override void OnRPC(Class sender, CF_EventArgs args)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		super.OnRPC(sender, args);
		auto rpc = CF_EventRPCArgs.Cast(args);

		switch (rpc.ID)
		{
			case ExpansionTeleporterModuleRPC.RequestOpenTeleporterMenu:
			{
				RPC_RequestOpenTeleporterMenu(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionTeleporterModuleRPC.RequestTeleport:
			{
				RPC_RequestTeleport(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
			case ExpansionTeleporterModuleRPC.PlayTeleportSound:
			{
				RPC_PlayTeleportSound(rpc.Context, rpc.Sender, rpc.Target);
				break;
			}
		}
	}

	//! Server
	void RequestOpenTeleporterMenu(Expansion_Teleporter_Base target, PlayerIdentity identity, ExpansionTeleportData teleporterData)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RequestOpenTeleporterMenu - Tryed to call RequestOpenTeleporterMenu on Client!");
			return;
		}

		if (!identity)
		{
			Error(ToString() + "::RequestOpenTeleporterMenu - identity is NULL!");
			return;
		}

		if (!target)
		{
			Error(ToString() + "::RequestOpenTeleporterMenu - Teleporter object is NULL!");
			return;
		}

		if (!teleporterData)
		{
			Error(ToString() + "::RequestOpenTeleporterMenu - Could not get teleporter data!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		teleporterData.OnSend(rpc);
		rpc.Send(target, ExpansionTeleporterModuleRPC.RequestOpenTeleporterMenu, true, identity);
	}

	//! Client
	protected void RPC_RequestOpenTeleporterMenu(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RPC_RequestOpenTeleporterMenu - Tryed to call RPC_RequestOpenTeleporterMenu on Server!");
			return;
		}

		if (m_TeleporterClientData)
			m_TeleporterClientData = null;

		m_TeleporterClientData = new ExpansionTeleportData();
		if (!m_TeleporterClientData.OnRecieve(ctx))
		{
			Error(ToString() + "::RPC_RequestOpenTeleporterMenu - Could not get teleporter data!");
			return;
		}

		TeleporterModulePrint("Teleporter data: " + m_TeleporterClientData.ToString());
		TeleporterModulePrint("Teleporter ID: " + m_TeleporterClientData.GetID());

		//! Open teleporter menu
		GetDayZGame().GetExpansionGame().GetExpansionUIManager().CreateSVMenu("ExpansionTeleporterMenu");

		//! Populate teleporter menu with needed client data.
		m_TeleporterMenuInvoker.Invoke();
	}

	//! Client
	void RequestTeleport(ExpansionTeleportPositionEntry pos, vector teleporterObjPos)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RequestTeleport - Tryed to call RequestTeleport on Server!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(teleporterObjPos);
		pos.OnSend(rpc);
		rpc.Send(null, ExpansionTeleporterModuleRPC.RequestTeleport, true);
	}

	//! Server
	protected void RPC_RequestTeleport(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
			return;

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::RPC_RequestTeleport - Tryed to call RPC_RequestTeleport on Client!");
			return;
		}

		PlayerBase player = PlayerBase.ExpansionGetPlayerByIdentity(senderRPC);
		if (!player)
			return;

		vector teleporterObjPos;
		if (!ctx.Read(teleporterObjPos))
		{
			Error(ToString() + "::RPC_PlayTeleportSound - Could not read teleporterObjPos");
			return;
		}

		ExpansionTeleportPositionEntry pos = new ExpansionTeleportPositionEntry();
		if (!pos.OnRecieve(ctx))
		{
			Error(ToString() + "::RPC_RequestTeleport - Could not get teleport position!");
			return;
		}

		vector playerPos = player.GetPosition();
		vector position = pos.GetPosition();
		vector orientation = pos.GetOrientation();
		if (position[1] == 0)
			position[1] = GetGame().SurfaceY(position[0], position[2]);

		PlayTeleportSound(teleporterObjPos, ExpansionTeleporterSound.TELEPORT_ACTIVE);
		PlayTeleportSound(position, ExpansionTeleporterSound.TELEPORT_ACTIVE);
		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(TeleportPlayer, 9000, false, position, orientation, player, teleporterObjPos);
	}

	void TeleportPlayer(vector pos, vector ori, PlayerBase player, vector teleporterObjPos = vector.Zero)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (teleporterObjPos != vector.Zero)
		{
			vector playerPos = player.GetPosition();
			int currentDistance = vector.Distance(playerPos, teleporterObjPos);
			if (currentDistance > 3.0)
				return;
		}

		DayZPlayerSyncJunctures.ExpansionTeleport(player, pos, ori);
		PlayTeleportSound(teleporterObjPos, ExpansionTeleporterSound.TELEPORT_DESTINATION);
		PlayTeleportSound(pos, ExpansionTeleporterSound.TELEPORT_DESTINATION);
	}

	//! Server
	void ExitTeleport(PlayerBase player, ExpansionTeleportData teleportData)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::ExitTeleport - Tryed to call ExitTeleport on Client!");
			return;
		}

		array<ref ExpansionTeleportPosition> teleportPositions = teleportData.GetTeleportPositions();
		if (!teleportPositions)
			return;

		ExpansionTeleportPosition randomTeleportPos = teleportPositions.GetRandomElement();
		if (!randomTeleportPos)
			return;

		array<ref ExpansionTeleportPositionEntry> positions = randomTeleportPos.GetPositions();
		if (!positions)
			return;

		ExpansionTeleportPositionEntry randomPos = positions.GetRandomElement();

		vector position = randomPos.GetPosition();
		vector orientation = randomPos.GetOrientation();
		if (position[1] == 0)
			position[1] = GetGame().SurfaceY(position[0], position[2]);

		GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(TeleportPlayer, 200, false, position, orientation, player);
	}

	void PlayTeleportSound(vector position, int sound)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!GetGame().IsServer() && !GetGame().IsMultiplayer())
		{
			Error(ToString() + "::PlayTeleportSound - Tryed to call PlayTeleportSound on Client!");
			return;
		}

		auto rpc = ExpansionScriptRPC.Create();
		rpc.Write(position);
		rpc.Write(sound);

		array<Object> objects = new array<Object>;
		GetGame().GetObjectsAtPosition(position, 300, objects, null);

		foreach (Object obj: objects)
		{
			PlayerBase player = PlayerBase.Cast(obj);
			if (player && player.GetIdentity())
				rpc.Send(NULL, ExpansionTeleporterModuleRPC.PlayTeleportSound, true, player.GetIdentity());
		}
	}

	protected void RPC_PlayTeleportSound(ParamsReadContext ctx, PlayerIdentity senderRPC, Object target)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!ExpansionScriptRPC.CheckMagicNumber(ctx))
		{
			Error(ToString() + "::RPC_PlayTeleportSound - Magic number check failed!");
			return;
		}

		if (!GetGame().IsClient())
		{
			Error(ToString() + "::RPC_PlayTeleportSound - Tryed to call RPC_PlayTeleportSound on Server!");
			return;
		}

		vector position;
		if (!ctx.Read(position))
		{
			Error(ToString() + "::RPC_PlayTeleportSound - Could not read position");
			return;
		}

		int sound;
		if (!ctx.Read(sound))
		{
			Error(ToString() + "::RPC_PlayTeleportSound - Could not read sound");
			return;
		}

		string soundShader;
		switch (sound)
		{
			case ExpansionTeleporterSound.TELEPORT_ACTIVE:
				soundShader = "Expansion_Teleporter_Active_Soundset";
				break;
			case ExpansionTeleporterSound.TELEPORT_DESTINATION:
				soundShader = "Blowout_Teleport";
				break;
		}

	#ifndef EDITOR
		EffectSound soundEffect = SEffectManager.PlaySound(soundShader, position, 0, 0, false);
		if (!soundEffect)
			return;

		//soundEffect.SetParent(target);
		soundEffect.SetSoundAutodestroy(true);
	#endif
	}

	void AddTeleporterData(ExpansionTeleportData data)
	{
		auto trace = EXTrace.Start(EXTrace.TELEPORTER, this);

		if (!m_TeleporterData.Contains(data.GetID()))
			m_TeleporterData.Insert(data.GetID(), data);
		
	#ifdef EXPANSION_NAMALSK_ADVENTURE
		if (!m_PlayerTeleporterMap)
			m_PlayerTeleporterMap = new map<int, ref array<string>>;

		array<string> playerUIDs;
		if (!m_PlayerTeleporterMap.Find(data.GetID(), playerUIDs))
		{
			playerUIDs = new array<string>;
			m_PlayerTeleporterMap.Insert(data.GetID(), playerUIDs);
		}
	#endif
	}

	//! Server
	ExpansionTeleportData GetTeleporterDataByID(int id)
	{
		ExpansionTeleportData foundData;
		if (m_TeleporterData.Find(id, foundData))
			return foundData;

		return NULL;
	}

	//! Client
	ExpansionTeleportData GetTeleporterClientData()
	{
		return m_TeleporterClientData;
	}

	//! Client
	ScriptInvoker GetTeleporterMenuSI()
	{
		return m_TeleporterMenuInvoker;
	}

	//! Client
	ScriptInvoker GetTeleporterMenuCallbackSI()
	{
		return m_TeleporterMenuCallbackInvoker;
	}

	void TeleporterModulePrint(string text)
	{
	#ifdef EXPANSIONMODTELEPORTERDEBUG
		EXTrace.Print(EXTrace.TELEPORTER, this, text);
	#endif
	}

	static ExpansionTeleporterModule GetModuleInstance()
	{
		return s_Instance;
	}
};
