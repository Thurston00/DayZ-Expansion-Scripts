/**
 * ItemBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class ItemBase
{
	protected static ref TTypeNameStringMap s_Expansion_CodeLockSlotNames = new TTypeNameStringMap;

	protected ref ExpansionElectricityConnection m_Expansion_ElectricitySource;
	protected ref array< ItemBase > m_Expansion_ElectricityConnections;

	protected bool m_Expansion_IsOpenable;
	protected bool m_Expansion_IsOpened;
	protected bool m_Expansion_Locked;
	protected string m_Expansion_Code;  //! Only set on server, not synced to client. NEVER set this directly. Use SetCode()
	protected int m_Expansion_CodeLength;  //! Unlike m_Expansion_Code, this should be synched to clients for items that use codes

	protected ref TStringArray m_Expansion_KnownUIDs;
	protected bool m_Expansion_KnownUIDsRequested;
	protected bool m_Expansion_KnownUIDsSet;

	static int s_Expansion_ChangeCode_RPCID;
	static int s_Expansion_Lock_RPCID;
	static int s_Expansion_ReceiveKnownUIDs_RPCID;
	static int s_Expansion_SendKnownUIDs_RPCID;
	static int s_Expansion_ServerReply_RPCID;
	static int s_Expansion_SetCode_RPCID;
	static int s_Expansion_Unlock_RPCID;

	//============================================
	// ItemBase Constructor
	//============================================
	void ItemBase()
	{
		//! Only register for netsync if config value is true, don't use ExpansionIsOpenable() here
		//! for compatibility with 3rd party modded items which may not wish to set the config value
		//! and have their own means of syncing open/closed state but still want to override
		//! ExpansionIsOpenable for other means (e.g. action conditions)
		m_Expansion_IsOpenable = ConfigGetBool("expansionIsOpenable");
		if (m_Expansion_IsOpenable)
			RegisterNetSyncVariableBool( "m_Expansion_IsOpened" );

		m_Expansion_ElectricitySource = new ExpansionElectricityConnection( this );
		m_Expansion_ElectricityConnections = new array< ItemBase >();
		if ( ExpansionCanRecievePower() )
		{
			//RegisterNetSyncVariableBool( "m_IsPairedSynch" );
			//RegisterNetSyncVariableBool( "m_IsOnSynch" );
			//RegisterNetSyncVariableInt( "m_SourceNetLow" );
			//RegisterNetSyncVariableInt( "m_SourceNetHigh" );
		}

		if (IsInherited(ExpansionCodeLock) || IsInherited(ExpansionSafeBase))
			Expansion_RegisterLockRPCs();
	}

	void Expansion_RegisterLockRPCs()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.BASEBUILDING, this);
#endif

		if (!m_Expansion_RPCManager)
			m_Expansion_RPCManager = new ExpansionRPCManager(this, ItemBase);

		if (!s_Expansion_ChangeCode_RPCID)
			s_Expansion_ChangeCode_RPCID = m_Expansion_RPCManager.RegisterServer("RPC_Expansion_ChangeCode");
		if (!s_Expansion_Lock_RPCID)
			s_Expansion_Lock_RPCID = m_Expansion_RPCManager.RegisterServer("RPC_Expansion_Lock");
		if (!s_Expansion_ReceiveKnownUIDs_RPCID)
			s_Expansion_ReceiveKnownUIDs_RPCID = m_Expansion_RPCManager.RegisterClient("RPC_Expansion_ReceiveKnownUIDs");
		if (!s_Expansion_SendKnownUIDs_RPCID)
			s_Expansion_SendKnownUIDs_RPCID = m_Expansion_RPCManager.RegisterServer("RPC_Expansion_SendKnownUIDs");
		if (!s_Expansion_ServerReply_RPCID)
			s_Expansion_ServerReply_RPCID = m_Expansion_RPCManager.RegisterClient("RPC_Expansion_ServerReply");
		if (!s_Expansion_SetCode_RPCID)
			s_Expansion_SetCode_RPCID = m_Expansion_RPCManager.RegisterServer("RPC_Expansion_SetCode");
		if (!s_Expansion_Unlock_RPCID)
			s_Expansion_Unlock_RPCID = m_Expansion_RPCManager.RegisterServer("RPC_Expansion_Unlock");
	}

	override void EEItemAttached(EntityAI item, string slot_name)
	{
		super.EEItemAttached(item, slot_name);

		if (item.IsInherited(ExpansionCodeLock))
			Expansion_RegisterLockRPCs();
	}

	override void EEInit()
	{
		super.EEInit();

		if (m_Expansion_IsOpenable && !ExpansionIsOpened() && GetInventory() && GetInventory().GetCargo())
			GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);
	}

	override void AfterStoreLoad()
	{
		super.AfterStoreLoad();

		if (m_Expansion_IsOpenable && ExpansionIsOpened() && GetInventory() && GetInventory().GetCargo())
			GetInventory().UnlockInventory(HIDE_INV_FROM_SCRIPT);
	}

	override bool CanPutInCargo(EntityAI parent)
	{
		if (!super.CanPutInCargo(parent))
			return false;

		if (ExpansionIsLocked())
			return false;

		return true;
	}

	override bool CanPutIntoHands(EntityAI parent)
	{
		if (!super.CanPutIntoHands(parent))
			return false;
		
		if (ExpansionIsLocked())
			return false;

		return true;
	}

	override bool CanReceiveAttachment(EntityAI attachment, int slotId)
	{
		if (!super.CanReceiveAttachment(attachment, slotId))
			return false;

		//! Check for m_Initialized set by vanilla DeferredInit() to make sure already attached items can be loaded from storage even if this is locked
		if (m_Initialized && ExpansionIsLocked())
			return false;

		return true;
	}

	override bool CanReceiveItemIntoCargo(EntityAI item)
	{
		if (!super.CanReceiveItemIntoCargo(item))
			return false;

		if (m_Expansion_IsOpenable && !ExpansionIsOpened())
			return false;

		//! Check for m_Initialized set by vanilla DeferredInit() to make sure cargo items can be loaded from storage even if this is locked
		if (m_Initialized && ExpansionIsLocked())
			return false;

		return true;
	}

	override bool CanReleaseAttachment(EntityAI attachment)
	{
		if (!super.CanReleaseAttachment(attachment))
			return false;

		//! Check for m_Initialized set by vanilla DeferredInit() to make sure already attached items can be loaded from storage even if this is locked
		if (m_Initialized && ExpansionIsLocked())
			return false;

		return true;
	}

    override bool CanReleaseCargo(EntityAI cargo)
	{
		if (!super.CanReleaseCargo(cargo))
			return false;

		if (m_Expansion_IsOpenable && !ExpansionIsOpened())
			return false;

		//! Check for m_Initialized set by vanilla DeferredInit() to make sure cargo items can be loaded from storage even if this is locked
		if (m_Initialized && ExpansionIsLocked())
			return false;

		return true;
	}

	/**
	 * /note	This is a static boolean, do not change during runtime
	 * 
	 * /brief	Override this to determine if the item can be powered using the expansion power system
	 */
	bool ExpansionCanRecievePower()
	{
		return false;
	}

	/**
	 * /note	This is a static boolean, do not change during runtime
	 * 
	 * /brief	Override this to determine if the item powers other items
	 */
	bool ExpansionIsPowerSource()
	{
		return false;
	}

	bool ExpansionIsConnected( ItemBase source = NULL )
	{
		return m_Expansion_ElectricitySource.IsConnected( source );
	}

	void ExpansionDisconnect()
	{
		m_Expansion_ElectricitySource.Disconnect();

		SetSynchDirty();
	}

	void ExpansionAddConnection( ItemBase item )
	{
		Print( "[" + this + "] ExpansionAddConnection item=" + item );
		if ( !ExpansionIsPowerSource() || !item.ExpansionCanRecievePower() )
			return;

		item.m_Expansion_ElectricitySource.Pair( this );

		SetSynchDirty();
		
		Print( "[" + this + "] ~ExpansionAddConnection" );
	}

	void _ExpansionAddConnection( ItemBase item )
	{
		int idx = m_Expansion_ElectricityConnections.Find( item );
		if ( idx == -1 )
			m_Expansion_ElectricityConnections.Insert( item );
	}

	void _ExpansionRemoveConnection( ItemBase item )
	{
		int idx = m_Expansion_ElectricityConnections.Find( item );
		if ( idx != -1 )
			m_Expansion_ElectricityConnections.Remove( idx );
	}

	override void OnWorkStart()
	{
		super.OnWorkStart();

		for ( int i = 0; i < m_Expansion_ElectricityConnections.Count(); ++i )
		{
			m_Expansion_ElectricityConnections[i].OnWorkStart();
		}
	}

	override void OnWorkStop()
	{
		super.OnWorkStop();
		
		for ( int i = 0; i < m_Expansion_ElectricityConnections.Count(); ++i )
		{
			m_Expansion_ElectricityConnections[i].OnWorkStop();
		}
	}
	
	override void SetActions()
	{
		super.SetActions();

		if (IsInherited(ExpansionSafeBase) || IsInherited(TentBase))
			return;

		if (m_Expansion_IsOpenable)
			AddAction(ExpansionActionOpen);

		if (!IsInherited(Fence) && ExpansionFindCodeLockSlot())
		{
			//! Order matters. Have ExpansionActionEnterCodeLock AFTER ExpansionActionOpen
			//! so that "Open locked" shows on locked items without having to cycle through actions in the UI.
			AddAction(ExpansionActionEnterCodeLock);
			AddAction(ExpansionActionChangeCodeLock);
		}

		//! Order matters. Have ExpansionActionClose AFTER ExpansionActionEnterCodeLock
		//! so that "Lock" shows on opened items without having to cycle through actions in the UI.
		if (m_Expansion_IsOpenable)
			AddAction(ExpansionActionClose);
	}

	//============================================
	// IsBasebuilding
	//============================================	
	/**
	\brief Returning if can be built
		\param 	
	*/
	bool IsBasebuilding()
	{
		return false;
	}
	
	//============================================
	// ExpansionIsOpenable
	//============================================	
	/**
	\brief Returning if openable in principle (regardless if player can actually open in current state)
		\param 	
	*/
	bool ExpansionIsOpenable()
	{
		return m_Expansion_IsOpenable;
	}

	/**
	\brief Returning if selection is openable in principle (regardless if player can actually open in current state)
		\param 	
	*/
	bool ExpansionIsOpenable( string selection )
	{
		return ExpansionIsOpenable();
	}
	
	/**
	\brief Returning if item is open
		\param 	
	*/
	bool ExpansionIsOpened()
	{
		return m_Expansion_IsOpened;
	}

	//! All actions dealing with opened/closed state should use IsOpen() for best compatibility with vanilla and 3rd party modded items,
	//! not ExpansionIsOpened(), although the latter is preferred when it is purely an Expansion item we are dealing with
	override bool IsOpen()
	{
		if (!super.IsOpen())
			return false;

		if (!IsInherited(BaseBuildingBase) && ExpansionIsOpenable())
			return ExpansionIsOpened();

		//! @note vanilla by default returns true, we need to keep this compatible
		return true;
	}

	/**
	\brief Returning if player can open item from selection
		\param 
	*/
	bool ExpansionCanOpen( PlayerBase player, string selection )
	{
		return ExpansionIsOpenable( selection ) && !IsOpen() && ( !ExpansionIsLocked() || IsKnownUser( player ) );
	}

	/**
	\brief Returning if player can close item from selection
		\param 
	*/
	bool ExpansionCanClose( PlayerBase player, string selection )
	{
		return ExpansionIsOpenable() && CanClose( selection );
	}
	
	/**
	\brief Returning if player can close item from selection
		\param 	
	*/
	bool CanClose( string selection )
	{
		return IsOpen();
	}

	/**
	\brief Opening item on defined selection
		\param 	
	*/
	void Expansion_Open( string selection ) 
	{
		Open();
	}

	override void Open()
	{
		super.Open();

		if (!m_Expansion_IsOpenable)
			return;
	
		if (GetInventory() && GetInventory().GetCargo())
			GetInventory().UnlockInventory(HIDE_INV_FROM_SCRIPT);

		m_Expansion_IsOpened = true;
		SetSynchDirty();
	}

	void UnlockAndOpen( string selection ) 
	{
		ExpansionUnlock();

		Expansion_Open( selection );
	}
	
	/**
	\brief Closing item on defined selection
		\param 	
	*/
	void Expansion_Close( string selection ) 
	{
		Close();
	}

	override void Close()
	{
		super.Close();

		if (!m_Expansion_IsOpenable)
			return;
		
		if (GetInventory() && GetInventory().GetCargo())
			GetInventory().LockInventory(HIDE_INV_FROM_SCRIPT);

		m_Expansion_IsOpened = false;
		SetSynchDirty();
	}

	void CloseAndLock( string selection )
	{
		Expansion_Close( selection );

		ExpansionLock();
	}
	
	/**
	\brief Set code of item
		\param 	
	*/
	void SetCode( string code, PlayerBase player = NULL, bool setUser = true, bool updateLock = true )
	{
		if (!GetGame().IsServer())
		{
			Error("ERROR: ItemBase::SetCode called on client!");
			return;
		}

		//! Check for m_Expansion_Code allows to set empty code on parent after migration of code to attached codelock
		if (!m_Expansion_Code && !IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
			{
				codelock.SetCode(code, player, setUser, updateLock);
			}
		}
		else
		{
			//! All zeroes code = unlock attachment slot and set no code
			if (code.ToInt() == 0)
			{
				code = "";

				if (GetHierarchyParent() && GetInventory().IsAttachment())
				{
					SetSlotLock( GetHierarchyParent(), false );
					SetTakeable( true );
				}
			}

			m_Expansion_Code = code;
			m_Expansion_CodeLength = code.Length();
			m_Expansion_Locked = false;

			if (m_Expansion_KnownUIDs && setUser && GetExpansionSettings().GetBaseBuilding().RememberCode)
				SetUser( player );
		}

		if (IsOpen() || !updateLock)
			SetSynchDirty();
		else if (HasCode())
			ExpansionLock();  //! Will call SetSynchDirty
		else
			ExpansionUnlock();  //! Will call SetSynchDirty
	}
	
	/**
	\brief Returning code of item
		\param 	
		@note Remember that m_Expansion_Code only exists on server!
		
	*/
	string GetCode()
	{
		if (!m_Expansion_Code && !IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
				return codelock.GetCode();
		}

		return m_Expansion_Code;
	}

	/**
	\brief Returning code length of item
		\param 	
	*/
	int GetCodeLength()
	{
		if (!m_Expansion_CodeLength && !IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
				return codelock.GetCodeLength();
		}

		return m_Expansion_CodeLength;
	}

	/**
	\brief Returning if the item has a code
		\param 	
		@note Remember that m_Expansion_Code only exists on server! Use m_Expansion_CodeLength > 0 to check for code on both server and client
	*/
	bool HasCode()
	{
		if (!m_Expansion_CodeLength && !IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
				return codelock.HasCode();
		}

		return m_Expansion_CodeLength > 0;
	}

	override bool ExpansionIsLocked()
	{
		if (!m_Expansion_CodeLength && !IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
				return codelock.ExpansionIsLocked();
		}

		return m_Expansion_CodeLength > 0 && m_Expansion_Locked;
	}

	/**
	\brief Returning if the item has a codelock attached
		\param "selection" selection for codelock
	*/
	bool ExpansionHasCodeLock( string selection )
	{
		return ExpansionHasCodeLock();
	}

	/**
	\brief Returning if the item has a codelock attached
	*/
	bool ExpansionHasCodeLock()
	{
		return ExpansionGetCodeLock() != NULL;
	}

	bool ExpansionFindCodeLockSlot(out string slotName = "")
	{
		if (GetInventory() && !GetInventory().GetAttachmentSlotsCount())
			return false;

		if (s_Expansion_CodeLockSlotNames.Find(this.Type(), slotName))
			return slotName != "";

		TStringArray attachments = Expansion_GetAttachmentSlots();

		if (attachments.Count())
		{
			TStringArray slots = ExpansionCodeLock.Expansion_GetCodeLockInventorySlots();

			foreach (string slot: slots)
			{
				if (ExpansionStatic.StringArrayContainsIgnoreCase(attachments, slot))
				{
					slotName = slot;
					break;
				}
			}
		}

		s_Expansion_CodeLockSlotNames.Insert(this.Type(), slotName);

		return slotName != "";
	}

	/**
	\brief Returning codelock entity
		\param
	*/
	ExpansionCodeLock ExpansionGetCodeLock()
	{
		return ExpansionCodeLock.Cast(GetAttachmentByConfigTypeName("ExpansionCodeLock"));
	}

	void SetSlotLock( EntityAI parent, bool state )
	{
		InventoryLocation inventory_location = new InventoryLocation;
		GetInventory().GetCurrentInventoryLocation( inventory_location );			
		parent.GetInventory().SetSlotLock( inventory_location.GetSlot(), state );
	}

	/**
	\brief Locking base build/safe
		\param 	
	*/
	void ExpansionLock()
	{
		if (!IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
			{
				codelock.ExpansionLock();
				return;
			}
		}

		if (m_Expansion_Code)
		{
			m_Expansion_Locked = true;

			if (GetHierarchyParent() && GetInventory().IsAttachment())
			{
				SetSlotLock( GetHierarchyParent(), true );
				SetTakeable( false );
			}
		}

		SetSynchDirty();
	}
	
	/**
	\brief Unlocking base build/safe
		\param 	
	*/
	void ExpansionUnlock()
	{
		if (!IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if (codelock)
			{
				codelock.ExpansionUnlock();
				return;
			}
		}

		m_Expansion_Locked = false;

		SetSynchDirty();
	}

	/**
	\brief Failed attempt to unlock item
		\param 	
	*/
	void FailedUnlock()
	{
		SoundCodeLockFailedUnlock();
	}
	
	protected void SoundCodeLockFailedUnlock()
	{
		if ( !IsMissionClient() )
			return;

		string SOUND_CODE_DENIED = "";		

		if ( GetExpansionSettings().GetBaseBuilding().DoDamageWhenEnterWrongCodeLock )
		{
			SOUND_CODE_DENIED = "Expansion_Shocks_SoundSet";
		} else {
			SOUND_CODE_DENIED = "Expansion_Denied_SoundSet";
		}

		if ( !GetGame().IsDedicatedServer() ) // client side
		{
			EffectSound sound = SEffectManager.PlaySound(SOUND_CODE_DENIED, GetPosition());
			sound.SetSoundAutodestroy( true );
		}
	}

	/**
	\brief Return if player is a known user of (attached) code lock or safe.
	
	NOTE: If IsKnownUser is called on the client instead of the server, initial calls will return false
	until the asynchronous request for known user data completes (unless the player had earlier set, changed,
	or entered the correct code while in the entity's network bubble).
	This is fine though, as IsKnownUser is primarily meant to be used by action conditions,
	and will function as intended in that context.

		\param 	player
	*/
	bool IsKnownUser( PlayerBase player )
	{
		auto bbSettings = GetExpansionSettings().GetBaseBuilding(false);
		auto ttSettings = GetExpansionSettings().GetTerritory(false);

		if ( !player || !player.GetIdentity() || !bbSettings.IsLoaded() || !ttSettings.IsLoaded() || !bbSettings.RememberCode )
			return false;

		if (!IsInherited(ExpansionCodeLock) && !IsInherited(ExpansionSafeBase))
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if ( codelock )
				return codelock.IsKnownUser( player );
		}

		if (ttSettings.AuthenticateCodeLockIfTerritoryMember && player.IsInsideOwnTerritory())
			return true;

		if (!m_Expansion_KnownUIDs)
			return false;

		if ( GetGame().IsClient() && !m_Expansion_KnownUIDsSet && !m_Expansion_KnownUIDsRequested )
			RequestKnownUIDs();

		return m_Expansion_KnownUIDs.Find( player.GetIdentityUID() ) > -1;
	}

	void AddUser(PlayerBase player, TStringArray knownUIDsToForget = null)
	{
		if ( player && player.GetIdentity() && !IsKnownUser( player ) )
		{
			string uid = player.GetIdentityUID();
			EXPrint("ItemBase::AddUser " + this + " (parent=" + GetHierarchyParent() + ") " + uid);
			m_Expansion_KnownUIDs.Insert(uid);
			SendKnownUIDs(player.GetIdentity(), knownUIDsToForget);
		}
	}

	void SetUser( PlayerBase player )
	{
		EXPrint("ItemBase::SetUser " + this + " (parent=" + GetHierarchyParent() + ")");

		string uid;

		if (player)
			uid = player.GetIdentityUID();

		//! All existing known users need to be updated, else their info becomes stale
		TStringArray knownUIDsToForget = {};
		foreach (string knownUID: m_Expansion_KnownUIDs)
		{
			if (knownUID != uid)
				knownUIDsToForget.Insert(knownUID);
		}

		m_Expansion_KnownUIDs.Clear();

		if (player)
			AddUser(player, knownUIDsToForget);
		else if (knownUIDsToForget)
			SendKnownUIDs(null, knownUIDsToForget);
	}

	//! Request known UIDs (players that know the code and have entered it correctly once) from server
	void RequestKnownUIDs()
	{
		EXPrint("ItemBase::RequestKnownUIDs " + this + " (parent=" + GetHierarchyParent() + ")");
		auto rpc = ExpansionScriptRPC.Create(s_Expansion_SendKnownUIDs_RPCID);
		rpc.Expansion_Send(this, true);
		m_Expansion_KnownUIDsRequested = true;
	}

	//! Send known UIDs (players that know the code and have entered it correctly once) to client
	void SendKnownUIDs(PlayerIdentity recipient, TStringArray knownUIDsToForget = null)
	{
		EXPrint("ItemBase::SendKnownUIDs " + this + " (parent=" + GetHierarchyParent() + ")");

		ExpansionScriptRPC rpc;

		if (knownUIDsToForget)
		{
			rpc = ExpansionScriptRPC.Create(s_Expansion_ReceiveKnownUIDs_RPCID);

			rpc.Write(false);

			foreach (string forgetUID: knownUIDsToForget)
			{
				auto player = PlayerBase.GetPlayerByUID(forgetUID);
				if (player && player.GetIdentity())
					rpc.Expansion_Send(this, true, player.GetIdentity());
			}
		}

		if (!recipient)
			return;

		rpc = ExpansionScriptRPC.Create(s_Expansion_ReceiveKnownUIDs_RPCID);

		if (m_Expansion_KnownUIDs.Find(recipient.GetId()) > -1)
			rpc.Write(true);
		else
			rpc.Write(false);

		rpc.Expansion_Send(this, true, recipient);
	}
	
	//============================================
	// SendServerLockReply
	//============================================	
	private void SendServerLockReply(bool reply, bool injuring, PlayerIdentity sender)
	{
		auto rpc = ExpansionScriptRPC.Create(s_Expansion_ServerReply_RPCID);
		rpc.Write( reply );
		rpc.Write( injuring );
		rpc.Expansion_Send(this, true, sender);
	}

	string Expansion_GetPlayerDesc(PlayerBase player)
	{
		string playerId;
		string playerName;
		string playerDesc;

		if ( player )
		{
			playerId = player.GetIdentityUID();
			playerName = player.GetIdentityName();
			playerDesc = "player \"" + playerName + "\" (ID = \"" + playerId + "\" at " + player.GetPosition() + ")";
		}

		return playerDesc;
	}

	void RPC_Expansion_Lock(PlayerIdentity sender, ParamsReadContext ctx)
	{
		string selection;
		if ( !ctx.Read( selection ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.LOCK can't read selection");
			SendServerLockReply( false, false, sender );
			ExpansionNotification("STR_EXPANSION_ERROR_TITLE", new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_SELECTION", "STR_EXPANSION_BB_CODE_CLOSE_LOCK")).Error(sender);
			return;
		}
		
		if ( !HasCode() || ExpansionIsLocked() )
		{
			SendServerLockReply( false, false, sender );
			return;
		}

		CloseAndLock( selection );
		
		SendServerLockReply( true, false, sender );
	}

	void RPC_Expansion_Unlock(PlayerIdentity sender, ParamsReadContext ctx)
	{
		if (!GetExpansionSettings().GetBaseBuilding())
			return;
		
		string code;
		if ( !ctx.Read( code ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.UNLOCK can't read code");
			SendServerLockReply( false, false, sender );
			ExpansionNotification("STR_EXPANSION_ERROR_TITLE", new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_BADREAD", "STR_EXPANSION_BB_CODE_UNLOCK")).Error(sender);
			return;
		}

		string selection;
		if ( !ctx.Read( selection ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.UNLOCK can't read selection");
			SendServerLockReply( false, false, sender );
			ExpansionNotification("STR_EXPANSION_ERROR_TITLE", new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_SELECTION", "STR_EXPANSION_BB_CODE_UNLOCK")).Error(sender);
			return;
		}
		
		if ( !HasCode() || !ExpansionIsLocked() )
		{
			CF_Log.Debug("ItemBase::OnRPC ExpansionLockRPC.UNLOCK !HasCode() || !IsLocked()");
			
			SendServerLockReply( false, false, sender );
			return;
		}

		PlayerBase player = PlayerBase.Cast(sender.GetPlayer());

		if ( !IsKnownUser( player ) && GetCode() != code )
		{
			CF_Log.Debug("ItemBase::OnRPC ExpansionLockRPC.UNLOCK GetCode() != code");
			
			bool InjuryPlayer = GetExpansionSettings().GetBaseBuilding().DoDamageWhenEnterWrongCodeLock;

			SendServerLockReply( false, InjuryPlayer, sender );

			if ( player )
			{
				string playerDesc = Expansion_GetPlayerDesc(player);
				if ( InjuryPlayer )
				{
					FailedUnlock();

					if ( GetExpansionSettings().GetLog().CodeLockRaiding )
						GetExpansionSettings().GetLog().PrintLog( "[BaseRaiding] " + GetType() + " (" + GetPosition() + ") Damaged \"" + playerDesc + "\" by " + GetExpansionSettings().GetBaseBuilding().DamageWhenEnterWrongCodeLock + " health points. Reason: Failed to enter the correct code." );

					//! Vanilla EnviroDmg is 1 0 1 (health blood shock)
					player.ProcessDirectDamage( DT_CUSTOM, player, "", "EnviroDmg", "0.5 0.5 0.5", GetExpansionSettings().GetBaseBuilding().DamageWhenEnterWrongCodeLock );
				} else {
					if ( GetExpansionSettings().GetLog().CodeLockRaiding )
						GetExpansionSettings().GetLog().PrintLog( "[BaseRaiding] " + GetType() + " (" + GetPosition() + ") \"" + playerDesc + "\" failed to enter the correct code." );
				}
				if ( GetExpansionSettings().GetLog().CodeLockRaiding )
					GetExpansionSettings().GetLog().PrintLog( "[BaseRaiding] " + GetType() + " (" + GetPosition() + ") The correct code was " + GetCode() + " and the player tried " + code );
			}

			return;
		}

		if ( GetExpansionSettings().GetBaseBuilding().RememberCode )
		{
			ExpansionCodeLock codelock = ExpansionGetCodeLock();
			if ( codelock )
				codelock.AddUser( player );
			else if ( IsInherited( ExpansionSafeBase ) )
				AddUser( player );
		}

		ExpansionUnlock();
		SendServerLockReply( true, false, sender );
	}

	void RPC_Expansion_SetCode(PlayerIdentity sender, ParamsReadContext ctx)
	{
		string code;
		if ( !ctx.Read( code ) || code.Length() == 0 || code.Length() > GetExpansionSettings().GetBaseBuilding().CodeLockLength )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SET can't read code");
			SendServerLockReply( false, false, sender );
			ExpansionNotification("STR_EXPANSION_ERROR_TITLE", new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_BADREAD", "STR_EXPANSION_BB_CODE_SET")).Error(sender);
			return;
		}

		string selection;
		if ( !ctx.Read( selection ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SET can't read selection");
			SendServerLockReply( false, false, sender );
			ExpansionNotification("STR_EXPANSION_ERROR_TITLE", new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_SELECTION", "STR_EXPANSION_BB_CODE_SET")).Error(sender);
			return;
		}
		
		if ( HasCode() )
		{
			// Base already has code so don't try setting it to another.
			SendServerLockReply( false, false, sender );
			return;
		}

		PlayerBase player = PlayerBase.Cast(sender.GetPlayer());

		SetCode( code, player );

		if ( GetExpansionSettings().GetLog().CodeLockRaiding )
			GetExpansionSettings().GetLog().PrintLog( "[BaseRaiding] " + GetType() + " ("+ GetPosition() + ") Code set by \"" + Expansion_GetPlayerDesc(player) + "\" and the code is "+ code );

		SendServerLockReply( true, false, sender );
	}

	void RPC_Expansion_ChangeCode(PlayerIdentity sender, ParamsReadContext ctx)
	{
		string code;
		if ( !ctx.Read( code ) || code.Length() == 0 || code.Length() > GetExpansionSettings().GetBaseBuilding().CodeLockLength )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SET can't read code");
			SendServerLockReply( false, false, sender );
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_BADREAD", "STR_EXPANSION_BB_CODE_LOCK_CHANGE")).Error(sender);
			return;
		}

		string selection;
		if ( !ctx.Read( selection ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SET can't read selection");
			SendServerLockReply( false, false, sender );
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_ERROR_TITLE"), new StringLocaliser("STR_EXPANSION_ERROR_DESC_CODE_SELECTION", "STR_EXPANSION_BB_CODE_LOCK_CHANGE")).Error(sender);
			return;
		}

		PlayerBase player = PlayerBase.Cast(sender.GetPlayer());
		
		SetCode( code, player );

		if ( GetExpansionSettings().GetLog().CodeLockRaiding )
			GetExpansionSettings().GetLog().PrintLog( "[BaseRaiding] " + GetType() + " ("+ GetPosition() + ") Code changed by \"" + Expansion_GetPlayerDesc(player) + "\" and the new code is "+ code );

		SendServerLockReply( true, false, sender );
	}

	//! client
	void RPC_Expansion_ServerReply(PlayerIdentity sender, ParamsReadContext ctx)
	{
		bool reply;
		if ( !ctx.Read( reply ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SERVERREPLY can't read reply");
			return;
		}

		bool injuring;
		if ( !ctx.Read( injuring ) )
		{
			Error("ItemBase::OnRPC ExpansionLockRPC.SERVERREPLY can't read injuring");
			return;
		}

		if ( injuring )
			FailedUnlock();

		ExpansionLockUIBase menu;
		if ( !Class.CastTo( menu, GetGame().GetUIManager().FindMenu( MENU_EXPANSION_CODELOCK_MENU ) ) && !Class.CastTo( menu, GetGame().GetUIManager().FindMenu( MENU_EXPANSION_NUMPAD_MENU ) ) )
			return;
			
		menu.OnServerResponse( reply, injuring );
	}

	//! server
	void RPC_Expansion_SendKnownUIDs(PlayerIdentity sender, ParamsReadContext ctx)
	{
		SendKnownUIDs(sender);
	}

	//! client
	void RPC_Expansion_ReceiveKnownUIDs(PlayerIdentity sender, ParamsReadContext ctx)
	{
		m_Expansion_KnownUIDs.Clear();

		if (!ctx.Read(m_Expansion_KnownUIDsSet))
			Error("ItemBase::RPC_Expansion_ReceiveKnownUIDs can't read m_Expansion_KnownUIDsSet");

		if (m_Expansion_KnownUIDsSet)
			m_Expansion_KnownUIDs.Insert(GetGame().GetPlayer().GetIdentity().GetId());  //! We only need to know this client's player identity
	}

	#ifdef EXPANSION_MODSTORAGE
	override void CF_OnStoreSave(CF_ModStorageMap storage)
	{
		super.CF_OnStoreSave(storage);

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return;

		m_Expansion_ElectricitySource.OnStoreSave(ctx);
	}
	
	override bool CF_OnStoreLoad(CF_ModStorageMap storage)
	{
		if (!super.CF_OnStoreLoad(storage))
			return false;

		auto ctx = storage[DZ_Expansion_BaseBuilding];
		if (!ctx) return true;

		if (!m_Expansion_ElectricitySource.OnStoreLoad(ctx))
			return false;

		return true;
	}
	#endif
	
	//============================================
	// EEOnAfterLoad
	//============================================
	override void EEOnAfterLoad()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GENERAL_ITEMS, this, "EEOnAfterLoad");
#endif

		super.EEOnAfterLoad();

		m_Expansion_ElectricitySource.OnAfterLoad();
	}

	//============================================
	// GetHiddenSelectionIndex
	//============================================
	override int GetHiddenSelectionIndex( string selection )
	{
		array<string> config_selections	= new array<string>;
		ConfigGetTextArray( "hiddenSelections", config_selections );
		
		for ( int i = 0; i < config_selections.Count(); ++i )
		{
			if ( config_selections.Get( i ) == selection )
			{
				return i;
			}
		}
		
		return -1;
	}

	//============================================
	// OnVariablesSynchronized
	//============================================
	override void OnVariablesSynchronized()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.GENERAL_ITEMS, this, "OnVariablesSynchronized");
#endif

		super.OnVariablesSynchronized();

		m_Expansion_ElectricitySource.OnVariablesSynchronized();
	}

	override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
	{
#ifdef DIAG_DEVELOPER
		EXTrace.PrintHit(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated", damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
#endif

		if (!super.EEOnDamageCalculated( damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef))
		{
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated - !super");
#endif
			return false;
		}

		if (!CanBeDamaged())
		{
#ifdef DIAG_DEVELOPER
			EXTrace.Print(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated - cannot be damaged");
#endif
			return false;
		}

		if (IsInherited(BaseBuildingBase) || IsInherited(ExpansionSafeBase))
		{
			if (damageType == DT_EXPLOSION || damageType == DT_FIRE_ARM)
			{
				if (IsInherited(ExpansionSafeBase))
				{
					if (damageType == DT_EXPLOSION)
						m_Expansion_DamageMultiplier = GetExpansionSettings().GetRaid().SafeExplosionDamageMultiplier;
					else
						m_Expansion_DamageMultiplier = GetExpansionSettings().GetRaid().SafeProjectileDamageMultiplier;
				}
				else
				{
					if (damageType == DT_EXPLOSION)
						m_Expansion_DamageMultiplier = GetExpansionSettings().GetRaid().ExplosionDamageMultiplier;
					else
						m_Expansion_DamageMultiplier = GetExpansionSettings().GetRaid().ProjectileDamageMultiplier;
				}

				if (!m_Expansion_DamageMultiplier)
				{
#ifdef DIAG_DEVELOPER
					EXTrace.Print(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated - damage multiplier is zero");
#endif
					return false;
				}

				if (damageType == DT_EXPLOSION)
				{
					if (GetExpansionSettings().GetRaid().EnableExplosiveWhitelist)
					{
						if (!source)
						{
#ifdef DIAG_DEVELOPER
							EXTrace.Print(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated - no explosion source");
#endif
							return false;
						}

						if (!ExpansionStatic.IsAnyOf(source, GetExpansionSettings().GetRaid().ExplosiveDamageWhitelist, true))
						{
#ifdef DIAG_DEVELOPER
							EXTrace.Print(EXTrace.BASEBUILDING, this, "EEOnDamageCalculated - explosive is not whitelisted");
#endif
							return false;
						}

						if (GetExpansionSettings().GetLog().BaseBuildingRaiding)
							GetExpansionSettings().GetLog().PrintLog("[BaseBuildingRaiding] BaseRaiding: " + ToString() + " at " + GetPosition() + " has been hit by whitelisted explosive " + source);
					}
					else
					{

						if (GetExpansionSettings().GetLog().BaseBuildingRaiding)
							GetExpansionSettings().GetLog().PrintLog("[BaseBuildingRaiding] BaseRaiding: " + ToString() + " at " + GetPosition() + " has been hit by explosive " + source);
					}
				}
			}
		}

		return true;
	}

	void ExpansionDropServer( PlayerBase player )
	{
		if ( !IsMissionHost() )
			return;

		EntityAI parent = GetHierarchyParent();

		if ( !parent )
			return;

		InventoryLocation inventory_location = new InventoryLocation;
		GetInventory().GetCurrentInventoryLocation( inventory_location );

		if ( inventory_location.IsValid() )
			parent.GetInventory().SetSlotLock( inventory_location.GetSlot(), false );

		if ( player )
			player.ServerDropEntity( this );
		else
			parent.GetInventory().DropEntity( InventoryMode.SERVER, parent, this );

		BaseBuildingBase base_building = BaseBuildingBase.Cast( parent );
		if ( base_building )
			SetPosition( base_building.GetKitSpawnPosition() );
		else
			SetPosition( parent.GetPosition() );
		PlaceOnSurface();

		SetTakeable( true );
		SetSynchDirty();
	}

	bool IsNonExpansionOpenable()
	{
		return false;
	}
};
