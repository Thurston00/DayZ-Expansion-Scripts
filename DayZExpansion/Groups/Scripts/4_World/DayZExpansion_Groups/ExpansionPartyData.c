/**
 * ExpansionPartyData.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionPartyData
{
	static const int GROUP_TAG_LENGTH = 5;
	static const string GROUP_TAG_START = "[";
	static const string GROUP_TAG_END = "] ";
	const autoptr TStringArray RANDOM_NAMES = {"Blaze","Phoenix","Titan","Shadow","Vortex","Hunter","Ranger","Striker","Knight","Reaper","Ghost","Nomad","Guardian","Wraith","Maverick","Falcon","Rogue","Viper","Tempest","Dragon","Raven","Storm","Sentinel","Specter","Inferno","Thunder","Wolf","Vanguard","Nebula","Eclipse","Fury","Spartan","Raider","Doom","Valkyrie","Titan","Hawk","Legion","Frost","Arcane","Leviathan","Cipher","Zephyr","Sabre","Blitz","Kraken","Chimera","Seraph","Astral","Zenith"};
	const autoptr TStringArray RANDOM_ADJECTIVES = {"Fiery","Eternal","Mystic","Crimson","Iron","Vengeful","Celestial","Savage","Arcane","Dark","Swift","Fearless","Unseen","Shadowed","Radiant","Fallen","Noble","Relentless","Silent","Ruthless","Brave","Infinite","Cunning","Grim","Fierce","Ancient","Frostbound","Luminous","Daring","Ethereal","Thunderous","Ferocious","Mysterious","Reckless","Merciless","Glorious","Ruthless","Zealous","Resolute","Unyielding","Intrepid","Magnificent","Brave","Unstoppable","Heroic","Swift","Tenacious","Vigorous","Resolute","Dreaded"};

	protected int PartyID;

	protected string PartyName;
	protected string PartyTag;
	protected string OwnerUID;
	protected string OwnerName;

	protected ref array< ref ExpansionPartyPlayerData > Players;
	protected ref array< ref ExpansionPartyInviteData > Invites;
	protected ref array< ref ExpansionMarkerData > Markers;

	protected ref map< string, ExpansionPartyPlayerData > PlayersMap;
	protected ref map< string, ExpansionPartyInviteData > InvitesMap;
	protected int LastMemberLeaveTimestamp;
#ifdef EXPANSIONMODNAVIGATION
	protected ref map< string, ExpansionMarkerData > MarkersMap;
	//! Server
	protected ref TStringArray m_SyncMarkersPlayers;
	//! Client
	bool m_MarkersSynced;
#endif

	protected int MoneyDeposited;

	// ------------------------------------------------------------
	// Expansion ExpansionPartyData Consturctor
	// ------------------------------------------------------------
	void ExpansionPartyData( int partyId = -1 )
	{
		PartyID = partyId;

		Players = new array< ref ExpansionPartyPlayerData >;
		Invites = new array< ref ExpansionPartyInviteData >;
		Markers = new array< ref ExpansionMarkerData >;

		PlayersMap = new map< string, ExpansionPartyPlayerData >;
		InvitesMap = new map< string, ExpansionPartyInviteData >;
	#ifdef EXPANSIONMODNAVIGATION
		MarkersMap = new map< string, ExpansionMarkerData >;
		m_SyncMarkersPlayers = new TStringArray;
	#endif
	}

	// ------------------------------------------------------------
	// Expansion ExpansionPartyData Destructor
	// ------------------------------------------------------------
	void ~ExpansionPartyData()
	{
		int i;

		for ( i = 0; i < Players.Count(); ++i )
			delete Players[i];
		for ( i = 0; i < Invites.Count(); ++i )
			delete Invites[i];
		for ( i = 0; i < Markers.Count(); ++i )
			delete Markers[i];

		delete Players;
		delete Invites;

		delete Markers;

		delete PlayersMap;
		delete InvitesMap;
	#ifdef EXPANSIONMODNAVIGATION
		delete MarkersMap;
	#endif
	}

	// ------------------------------------------------------------
	// Expansion InitMaps
	// ------------------------------------------------------------
	void InitMaps()
	{
		PlayersMap.Clear();
		InvitesMap.Clear();
	#ifdef EXPANSIONMODNAVIGATION
		MarkersMap.Clear();
	#endif

		int index = 0;

		for ( index = 0; index < Players.Count(); ++index )
			if ( Players[index] )
				PlayersMap.Insert( Players[index].UID, Players[index] );

		for ( index = 0; index < Invites.Count(); ++index )
			if ( Invites[index] )
				InvitesMap.Insert( Invites[index].UID, Invites[index] );

	#ifdef EXPANSIONMODNAVIGATION
		for ( index = 0; index < Markers.Count(); ++index )
			if ( Markers[index] )
				MarkersMap.Insert( Markers[index].GetUID(), Markers[index] );
	#endif
	}
	// ------------------------------------------------------------
	// Expansion GetPartyID
	// ------------------------------------------------------------
	int GetPartyID()
	{
		return PartyID;
	}

	// ------------------------------------------------------------
	// Expansion GetPartyName
	// ------------------------------------------------------------
	string GetPartyName()
	{
		return PartyName;
	}

	// ------------------------------------------------------------
	// Expansion ChangePartyTag
	// ------------------------------------------------------------
	void SetPartyTag(string tag)
	{
		PartyTag = tag;
	}

	// ------------------------------------------------------------
	// Expansion GetPartyTag
	// ------------------------------------------------------------
	string GetPartyTag()
	{
		return PartyTag;
	}

	/**
	 * @brief If set, return party tag truncated to GROUP_TAG_LENGTH and enclosed in GROUP_TAG_START/END, else empty string
	 */
	string GetPartyTagFormatted()
	{
		if (!PartyTag)
			return "";

		string tag;

		if (PartyTag.Length() > GROUP_TAG_LENGTH)
			tag = PartyTag.Substring(0, GROUP_TAG_LENGTH);
		else
			tag = PartyTag;

		return GROUP_TAG_START + tag + GROUP_TAG_END;
	}

	// ------------------------------------------------------------
	// Expansion ChangePartyName
	// ------------------------------------------------------------
	void SetPartyName(string name)
	{
		PartyName = name;
	}

	// ------------------------------------------------------------
	// Expansion GetPlayers
	// ------------------------------------------------------------
	array< ref ExpansionPartyPlayerData > GetPlayers()
	{
		return Players;
	}

#ifdef EXPANSIONMODNAVIGATION
	// ------------------------------------------------------------
	// Expansion GetAllMarkers
	// ------------------------------------------------------------
	array<ref ExpansionMarkerData> GetAllMarkers()
	{
		return Markers;
	}
#endif

	// ------------------------------------------------------------
	// Expansion SetupExpansionPartyData
	// ------------------------------------------------------------
	void SetupExpansionPartyData( PlayerBase pPb, string partyName, string partyTag )
	{
		PartyName = partyName;
		PartyTag = partyTag;

	   	ExpansionPartyPlayerData player = AddPlayer( pPb, true );

		OwnerUID = player.UID;
		OwnerName = player.Name;
	}

	// ------------------------------------------------------------
	// Expansion AddPlayer
	// ------------------------------------------------------------
	ExpansionPartyPlayerData AddPlayer( PlayerBase pPb, bool owner = false )
	{
		ExpansionPartyPlayerData player = new ExpansionPartyPlayerData(this);
		player.UID = pPb.GetIdentityUID();
		player.OnJoin( pPb );
		player.SetupColor();
		OnJoin( player );

	#ifdef EXPANSIONMODNAVIGATION
		player.InitMarker();
	#endif
		player.SetPermissions(ExpansionPartyPlayerPermissions.NONE);

		if (owner)
		{
			player.SetPermissions(ExpansionPartyPlayerPermissions.CAN_EDIT | ExpansionPartyPlayerPermissions.CAN_INVITE | ExpansionPartyPlayerPermissions.CAN_KICK | ExpansionPartyPlayerPermissions.CAN_DELETE | ExpansionPartyPlayerPermissions.CAN_WITHDRAW_MONEY);
		}

		Players.Insert( player );
		PlayersMap.Insert( player.UID, player );

		return player;
	}

	// ------------------------------------------------------------
	// Expansion AcceptInvite
	// ------------------------------------------------------------
	bool AcceptInvite(PlayerBase player)
	{
		if (RemoveInvite(player.GetIdentityUID()))
		{
			ExpansionPartyPlayerData newPlayerData = AddPlayer(player);
			if (!newPlayerData)
			{
				return false;
			}
			else
			{
				return true;
			}
		}
		return false;
	}

	// ------------------------------------------------------------
	// Expansion DeclineInvite
	// ------------------------------------------------------------
	bool DeclineInvite(string uid)
	{
		return RemoveInvite(uid);
	}

	// ------------------------------------------------------------
	// Expansion CancelInvite
	// ------------------------------------------------------------
	bool CancelInvite( string uid )
	{
		return RemoveInvite( uid );
	}

	// ------------------------------------------------------------
	// Expansion RemoveInvite
	// ------------------------------------------------------------
	bool RemoveInvite( string uid )
	{
		ExpansionPartyInviteData invite = InvitesMap.Get( uid );
		if ( invite != NULL )
		{
			Invites.RemoveItem( invite );
			InvitesMap.Remove( uid );
			return true;
		}

		return false;
	}

	bool IsInviteCooldownActive()
	{
		if (GetInviteCooldown() > 0)
			return true;

		return false;
	}

	/**
	 * @brief Get remaining invite cooldown time in seconds
	 */
	int GetInviteCooldown()
	{
		auto now = CF_Date.Now(true);
		int timestamp = now.GetTimestamp();

		int elapsedTime = timestamp - LastMemberLeaveTimestamp;
		if (elapsedTime < GetExpansionSettings().GetParty().InviteCooldown)
			return GetExpansionSettings().GetParty().InviteCooldown - elapsedTime;

		return 0;
	}

	// ------------------------------------------------------------
	// Expansion AddInvite
	// ------------------------------------------------------------
	ExpansionPartyInviteData AddInvite( string playerID )
	{
		ExpansionPartyInviteData invite = InvitesMap.Get( playerID );
		if ( invite != NULL )
		{
			Error("ExpansionPartyData::AddInvite Add a player already exist");
			DumpStack();
			return invite;
		}

		invite = new ExpansionPartyInviteData;
		invite.partyID = PartyID;
		invite.UID = playerID;
		invite.partyName = PartyName;

		Invites.Insert( invite );
		InvitesMap.Insert( playerID, invite );

		return invite;
	}

	// ------------------------------------------------------------
	// Expansion RemoveMember
	// ------------------------------------------------------------
	bool RemoveMember(string uid)
	{
		ExpansionPartyPlayerData player = PlayersMap.Get(uid);
		if (player)
		{
			Players.RemoveItem(player);
			PlayersMap.Remove(uid);
			OnLeave( player );

			auto now = CF_Date.Now(true);
			LastMemberLeaveTimestamp = now.GetTimestamp();
			
			return true;
		}

		return false;
	}

	bool ChangeMemberColor(string uid, int color)
	{
		ExpansionPartyPlayerData player = PlayersMap.Get(uid);
		if (player)
		{
			player.SetColor(color);
			return true;
		}

		return false;
	}

	// ------------------------------------------------------------
	// Expansion GetPlayer
	// ------------------------------------------------------------
	ExpansionPartyPlayerData GetPlayer( string uid )
	{
		return PlayersMap.Get( uid );
	}

	// ------------------------------------------------------------
	// Expansion HasPlayerInvite
	// ------------------------------------------------------------
	bool HasPlayerInvite( string uid )
	{
		return InvitesMap.Get( uid ) != NULL;
	}

	// ------------------------------------------------------------
	// Expansion GetPlayerInvite
	// ------------------------------------------------------------
	ExpansionPartyInviteData GetPlayerInvite( string uid )
	{
		return InvitesMap.Get( uid );
	}

	// ------------------------------------------------------------
	// Expansion GetTerritoryMembers
	// ------------------------------------------------------------
	array<ref ExpansionPartyInviteData> GetPartyInvites()
	{
		return Invites;
	}

#ifdef EXPANSIONMODNAVIGATION
	// ------------------------------------------------------------
	// Expansion AddMarker
	// ------------------------------------------------------------
	bool AddMarker(  ExpansionMarkerData marker )
	{
		if ( marker.GetUID() == "" )
		{
			string newUid = "" + Math.RandomInt( 0, int.MAX );
			while ( MarkersMap.Get( newUid ) )
				newUid = "" + Math.RandomInt( 0, int.MAX );

			marker.SetUID( newUid );
		} else if ( MarkersMap.Get( marker.GetUID() ) )
			return false;

		Markers.Insert( marker );
		MarkersMap.Insert( marker.GetUID(), marker );

		SetSyncMarkers();

		return true;
	}

	void SetSyncMarkers()
	{
		m_SyncMarkersPlayers.Clear();
	}

	ExpansionMarkerData GetMarker( string uid )
	{
		return MarkersMap.Get( uid );
	}

	// ------------------------------------------------------------
	// Expansion UpdateMarker
	// ------------------------------------------------------------
	bool UpdateMarker(  ExpansionMarkerData marker )
	{
		ExpansionMarkerData orgi = MarkersMap.Get( marker.GetUID() );
		if ( !orgi )
			return false;

		int dummy_version = int.MAX;
		ScriptReadWriteContext ctx = new ScriptReadWriteContext;
		marker.OnStoreSave( ctx.GetWriteContext() );
		orgi.OnStoreLoad( ctx.GetReadContext(), dummy_version );

		SetSyncMarkers();

		return true;
	}

	// ------------------------------------------------------------
	// Expansion RemoveMarker
	// ------------------------------------------------------------
	bool RemoveMarker( string markerName )
	{
		ExpansionMarkerData marker = MarkersMap.Get( markerName );
		if ( marker )
		{
			int removeIndex = Markers.Find( marker );
			if ( removeIndex != -1 )
				Markers.RemoveOrdered( removeIndex );

			MarkersMap.Remove( markerName );
			delete marker;

			SetSyncMarkers();

			return true;
		}

		return false;
	}

	// ------------------------------------------------------------
	// Expansion UpdateMarkerPosition
	// ------------------------------------------------------------
	bool UpdateMarkerPosition( string markerName, vector markerPosition )
	{
		ExpansionMarkerData marker = MarkersMap.Get( markerName );
		if ( marker )
		{
			marker.SetPosition( markerPosition );

			SetSyncMarkers();

			return true;
		}

		return false;
	}
#endif

	// ------------------------------------------------------------
	// Expansion GetOwnerName
	// ------------------------------------------------------------
	string GetOwnerName()
	{
		return OwnerName;
	}

	// ------------------------------------------------------------
	// Expansion GetOwnerUID
	// ------------------------------------------------------------
	string GetOwnerUID()
	{
		return OwnerUID;
	}

	// ------------------------------------------------------------
	// Expansion SetOwnerUID
	// ------------------------------------------------------------
	void SetOwnerUID(string playerUID)
	{
		OwnerUID = playerUID;
	}

	// ------------------------------------------------------------
	// Expansion Save
	// ------------------------------------------------------------
	void Save()
	{
		if ( !IsMissionHost() )
			return;

		FileSerializer file = new FileSerializer;

		if (file.Open(EXPANSION_GROUPS_FOLDER + PartyID + ".bin", FileMode.WRITE))
		{
			file.Write(EXPANSION_VERSION_MAP_MARKER_SAVE);
			OnStoreSave(file);

			file.Close();
		}
	}

	// ------------------------------------------------------------
	// Expansion Delete
	// ------------------------------------------------------------
	void Delete()
	{
		if ( !IsMissionHost() )
			return;

		if (EXPANSION_VERSION_MAP_MARKER_SAVE >= 4)
		{
			DeleteFile( EXPANSION_GROUPS_FOLDER + PartyID + ".bin" );
		}
		else
		{
			DeleteFile( EXPANSION_GROUPS_FOLDER + PartyID + ".json" );
		}
	}

	// ------------------------------------------------------------
	// Expansion CountPlayers
	// ------------------------------------------------------------
	private int CountPlayers()
	{
		int nmb = 0;

		for (int i = 0; i < Players.Count(); ++i)
		{
			if (!Players[i])
				continue;

			nmb++;
		}

		return nmb;
	}

	// ------------------------------------------------------------
	// Expansion CountInvites
	// ------------------------------------------------------------
	private int CountInvites()
	{
		int nmb = 0;

		for (int i = 0; i < Invites.Count(); ++i)
		{
			if (!Invites[i])
				continue;

			nmb++;
		}

		return nmb;
	}

#ifdef EXPANSIONMODNAVIGATION
	// ------------------------------------------------------------
	// Expansion CountMarkers
	// ------------------------------------------------------------
	private int CountMarkers()
	{
		int nmb = 0;

		for (int i = 0; i < Markers.Count(); ++i)
		{
			if ( !Markers[i] )
				continue;

			nmb++;
		}

		return Markers.Count();
	}
#endif

	//! TODO: Most party data should only be sent if changed
	//! The only thing to be synced periodically should be member positions
	void OnSend( ParamsWriteContext ctx, bool syncMarkers = true, string playerID = string.Empty )
	{
		ctx.Write( PartyID );
		ctx.Write( PartyName );
		ctx.Write( OwnerUID );

		int count = 0;
		int index = 0;

	#ifdef EXPANSIONMODNAVIGATION
		auto settings = GetExpansionSettings().GetParty();
	#endif

		count = Players.Count();
		ctx.Write( count );
		for ( index = 0; index < count; ++index )
		{
			ctx.Write( Players[index].UID );
			ctx.Write( Players[index].Name );
			ctx.Write( Players[index].Permissions );

		#ifdef EXPANSIONMODNAVIGATION
			bool hasMarker = false;

			if ( Players[index].Marker && (settings.ShowPartyMemberMapMarkers || settings.ShowPartyMember3DMarkers) )
			{
				if ( Players[index].Marker.GetObject() )
				{
					hasMarker = true;
					Players[index].Marker.Update();
				}
			}

			ctx.Write( hasMarker );

			if ( hasMarker )
				Players[index].Marker.OnSend( ctx );
			else
				ctx.Write(Players[index].m_TempMarkerData.GetColor());

			if ( Players[index].QuickMarker && settings.EnableQuickMarker )
			{
				ctx.Write( true );

				Players[index].QuickMarker.OnSend( ctx );
			} else
			{
				ctx.Write( false );
			}
		#endif
		}

		count = Invites.Count();
		ctx.Write( count );
		for ( index = 0; index < count; ++index )
		{
			ctx.Write( Invites[index].UID );
		}

	#ifdef EXPANSIONMODNAVIGATION
		if (settings.CanCreatePartyMarkers)
		{
			bool syncMarkersPlayer;
			if (playerID && m_SyncMarkersPlayers.Find(playerID) > -1)
				syncMarkersPlayer = true;
			if (!syncMarkersPlayer && syncMarkers)
			{
				count = Markers.Count();
				m_SyncMarkersPlayers.Insert(playerID);
			}
			else
			{
				count = -1;
			}
		}
		else
		{
			count = 0;
		}

		ctx.Write( count );
		for ( index = 0; index < count; ++index )
		{
			ctx.Write( Markers[index].GetUID() );
			Markers[index].OnSend( ctx );
		}
	#endif

	//! Do not need to network send money if market not loaded (but be sure to still write in OnStoreSave!)
	#ifdef EXPANSIONMODMARKET
		ctx.Write( MoneyDeposited );
	#endif

		ctx.Write( PartyTag );
	}

	bool OnRecieve( ParamsReadContext ctx )
	{
		// PartyID is read in ExpansionPartyModule
		if ( !ctx.Read( PartyName ) )
			return false;
		if ( !ctx.Read( OwnerUID ) )
			return false;

		string uid = "";

		int count = 0;
		int index = 0;
		int removeIndex = 0;
		array< string > checkArr;
		if ( !ctx.Read( count ) )
			return false;

	#ifdef EXPANSIONMODNAVIGATION
		auto settings = GetExpansionSettings().GetParty();
	#endif

		ExpansionPartyPlayerData player;
		checkArr = PlayersMap.GetKeyArray();
		for ( index = 0; index < count; ++index )
		{
			if ( !ctx.Read( uid ) )
				return false;

			removeIndex = checkArr.Find( uid );
			if ( removeIndex != -1 )
				checkArr.Remove( removeIndex );

			player = PlayersMap.Get( uid );
			if ( !player )
			{
				player = new ExpansionPartyPlayerData(this);
				Players.Insert( player );
				PlayersMap.Insert( uid, player );
			}

			player.UID = uid;

			if ( !ctx.Read( player.Name ) )
				return false;

			if ( uid == OwnerUID )
				OwnerName = player.Name;

			if ( !ctx.Read( player.Permissions ) )
				return false;

		#ifdef EXPANSIONMODNAVIGATION
			bool hasMarker;
			if ( !ctx.Read( hasMarker ) )
				return false;

			if ( hasMarker )
			{
				if ( !player.Marker )
					player.Marker = new ExpansionPlayerMarkerData( uid );

				if ( !player.Marker.OnRecieve( ctx ) )
					return false;

				if (GetExpansionClientSettings() && GetExpansionClientSettings().ShowMemberNameMarker)
					player.Marker.SetName(player.Name);
				player.Marker.Set3D(settings.ShowPartyMember3DMarkers);
				player.Marker.SetIcon(ExpansionIcons.Get("Persona"));
			}
			else
			{
				if ( player.Marker )
					delete player.Marker;

				int color;
				if (!ctx.Read(color))
					return false;

				player.SetColor(color);
			}

			bool hasQuickMarker;
			if ( !ctx.Read( hasQuickMarker ) )
				return false;

			if ( hasQuickMarker )
			{
				if ( !player.QuickMarker )
					player.QuickMarker = new ExpansionPartyQuickMarkerData( "QuickMarker" + uid );

				if ( !player.QuickMarker.OnRecieve( ctx ) )
					return false;

				player.QuickMarker.SetName(player.Name);
				player.QuickMarker.Set3D(true);
				player.QuickMarker.SetIcon(ExpansionIcons.Get("Map Marker"));
			} else
			{
				if ( player.QuickMarker )
					delete player.QuickMarker;
			}
		#endif

		}

		for ( index = 0; index < checkArr.Count(); ++index )
		{
			player = PlayersMap.Get( checkArr[index] );
			if ( player )
			{
				removeIndex = Players.Find( player );
				if ( removeIndex != -1 )
					Players.Remove( removeIndex );

				PlayersMap.Remove( checkArr[index] );
				delete player;
			}
		}

		if ( !ctx.Read( count ) )
			return false;

		ExpansionPartyInviteData invite;
		checkArr = InvitesMap.GetKeyArray();
		for ( index = 0; index < count; ++index )
		{
			if ( !ctx.Read( uid ) )
				return false;

			removeIndex = checkArr.Find( uid );
			if ( removeIndex != -1 )
				checkArr.Remove( removeIndex );

			invite = InvitesMap.Get( uid );
			if ( !invite )
			{
				invite = new ExpansionPartyInviteData();
				InvitesMap.Insert( uid, invite );
				Invites.Insert( invite );
			}
		}
		for ( index = 0; index < checkArr.Count(); ++index )
		{
			invite = InvitesMap.Get( checkArr[index] );
			if ( invite )
			{
				removeIndex = Invites.Find( invite );
				if ( removeIndex != -1 )
					Invites.Remove( removeIndex );

				InvitesMap.Remove( checkArr[index] );
				delete invite;
			}
		}

	#ifdef EXPANSIONMODNAVIGATION
		if ( !ctx.Read( count ) )
			return false;

		if (count > -1)
		{
			ExpansionMarkerData marker;
			checkArr = MarkersMap.GetKeyArray();
			for ( index = 0; index < count; ++index )
			{
				if ( !ctx.Read( uid ) )
					return false;

				removeIndex = checkArr.Find( uid );
				if ( removeIndex != -1 )
					checkArr.Remove( removeIndex );

				marker = MarkersMap.Get( uid );
				if ( !marker )
				{
					marker = ExpansionMarkerData.Create( ExpansionMapMarkerType.PARTY, uid );
					MarkersMap.Insert( uid, marker );
					Markers.Insert( marker );
				}

				if ( !marker.OnRecieve( ctx ) )
					return false;
			}
			for ( index = 0; index < checkArr.Count(); ++index )
			{
				marker = MarkersMap.Get( checkArr[index] );
				if ( marker )
				{
					removeIndex = Markers.Find( marker );
					if ( removeIndex != -1 )
						Markers.RemoveOrdered( removeIndex );

					MarkersMap.Remove( checkArr[index] );
					delete marker;
				}
			}

			m_MarkersSynced = true;
		}
	#endif

	//! Do not need to network receive money if market not loaded (but be sure to still load in OnStoreLoad!)
	#ifdef EXPANSIONMODMARKET
		if ( !ctx.Read( MoneyDeposited ) )
			return false;
	#endif

		if ( !ctx.Read( PartyTag ) )
			return false;

		return true;
	}

	// ------------------------------------------------------------
	// Expansion OnStoreSave
	// ------------------------------------------------------------
	void OnStoreSave( ParamsWriteContext ctx )
	{
		ctx.Write(PartyID);
		ctx.Write(PartyName);
		ctx.Write(OwnerUID);
		ctx.Write(OwnerName);

		int i = 0;

		ctx.Write( Players.Count() );
		for ( i = 0; i < Players.Count(); ++i )
			Players[i].OnStoreSave( ctx );

		ctx.Write( Invites.Count() );
		for ( i = 0; i < Invites.Count(); ++i)
			Invites[i].OnStoreSave( ctx );

		//! Always write markers so you don't loose them if Navigation mod is not loaded
		ctx.Write( Markers.Count() );
		for ( i = 0; i < Markers.Count(); ++i )
		{
			ctx.Write( Markers[i].GetUID() );
			Markers[i].OnStoreSave( ctx );
		}

		//! Always write money deposited so it doesn't cause issues when market is added later
		ctx.Write(MoneyDeposited);
		ctx.Write(PartyTag);

		ctx.Write(LastMemberLeaveTimestamp);
	}

	// ------------------------------------------------------------
	// Expansion OnStoreLoad
	// ------------------------------------------------------------
	bool OnStoreLoad( ParamsReadContext ctx, int version )
	{
		if ( Expansion_Assert_False( ctx.Read( PartyID ), "Failed reading party uid" ) )
			return false;

		if ( Expansion_Assert_False( ctx.Read( PartyName ), "Failed reading party name" ) )
			return false;

		if ( Expansion_Assert_False( ctx.Read( OwnerUID ), "Failed reading owner uid" ) )
			return false;

		if ( Expansion_Assert_False( ctx.Read( OwnerName ), "Failed reading owner name" ) )
			return false;

		int i;

		int countPlayers;
		if ( Expansion_Assert_False( ctx.Read( countPlayers ), "Failed reading player count" ) )
			return false;

		for ( i = 0; i < countPlayers; ++i )
		{
			auto player = new ExpansionPartyPlayerData(this);

			if ( Expansion_Assert_False( player.OnStoreLoad( ctx, version ), "Failed reading player [" + i + "]" ) )
				return false;

			Players.Insert(player);
		}

		int countInvites;
		if ( Expansion_Assert_False( ctx.Read( countInvites ), "Failed reading invite count" ) )
			return false;

		for ( i = 0; i < countInvites; ++i )
		{
			auto invite = new ExpansionPartyInviteData;

			if ( Expansion_Assert_False( invite.OnStoreLoad( ctx, version ), "Failed reading invite [" + i + "]" ) )
				return false;

			Invites.Insert(invite);
		}

		//! Always read markers so you don't loose them if Navigation mod is not loaded
		int countMarkers;
		if ( Expansion_Assert_False( ctx.Read( countMarkers ), "Failed reading marker count" ) )
			return false;

		for ( i = 0; i < countMarkers; ++i )
		{
			string marker_uid;

			if ( version >= 8 )
			{
				if ( Expansion_Assert_False( ctx.Read( marker_uid ), "Failed reading marker_uid [" + i + "]" ) )
					return false;
			} else
			{
				marker_uid = "marker_old_" + i;
			}

			auto marker = ExpansionMarkerData.Create( ExpansionMapMarkerType.PARTY );

			marker.SetUID( marker_uid );

			if ( Expansion_Assert_False( marker.OnStoreLoad( ctx, version ), "Failed reading marker [" + i + "]" ) )
				return false;

			Markers.Insert(marker);
		}

		//! Read money deposited so it doesn't cause issues when market is added later
		bool readMoneyDeposited;
	#ifdef EXPANSIONMODMARKET
		readMoneyDeposited = version >= 26;
	#else
		readMoneyDeposited = version >= 39;
	#endif
		if ( readMoneyDeposited && Expansion_Assert_False( ctx.Read( MoneyDeposited ), "Failed reading party money deposit data" ) )
			return false;

		if ( version >= 40 )
		{
			if ( Expansion_Assert_False( ctx.Read( PartyTag ), "Failed reading party tag" ) )
				return false;
		}
		else if ( GetExpansionSettings().GetParty().ForcePartyToHaveTags )
		{
			PartyTag = PartyName.Substring(0, GROUP_TAG_LENGTH);
		}

		if (version >= 42)
		{
			if ( Expansion_Assert_False( ctx.Read( LastMemberLeaveTimestamp ), "Failed reading LastMemberLeaveTimestamp" ) )
				return false;
		}

		return true;
	}

	void OnJoin(ExpansionPartyPlayerData player)
	{
		EXPrint(ToString() + "::OnJoin party " + PartyName + " (ID " + PartyID + ") player " + player.Name + " (ID " + player.GetID() + ")");
	#ifdef EXPANSIONMODNAVIGATION
		SyncMarkers_RemovePlayer(player);
	#endif
		
		if (GetExpansionSettings().GetLog().Party)
			GetExpansionSettings().GetLog().PrintLog("[Party] Player \"" + player.Name + "\" (id=" + player.GetID() + ")" + " joinded the party \"" + PartyName + "\" (id=" + PartyID + " ownerUID=" + OwnerUID + " ownerName=" + OwnerName + ")");
	}

	void OnLeave(ExpansionPartyPlayerData player)
	{
		EXPrint(ToString() + "::OnLeave party " + PartyName + " (ID " + PartyID + ") player " + player.Name + " (ID " + player.GetID() + ")");
	#ifdef EXPANSIONMODNAVIGATION
		SyncMarkers_RemovePlayer(player);
	#endif

		player.OnLeave();
		
		if (GetExpansionSettings().GetLog().Party)
			GetExpansionSettings().GetLog().PrintLog("[Party] Player \"" + player.Name + "\" (id=" + player.GetID() + ")" + " left the party \"" + PartyName + "\" (id=" + PartyID + " ownerUID=" + OwnerUID + " ownerName=" + OwnerName + ")");
	}

#ifdef EXPANSIONMODNAVIGATION
	void SyncMarkers_RemovePlayer(ExpansionPartyPlayerData player)
	{
		int idx = m_SyncMarkersPlayers.Find(player.GetID());
		if (idx > -1)
			m_SyncMarkersPlayers.Remove(idx);
	}
#endif

	bool IsMember(string uid)
	{
		if (GetPlayer(uid))
			return true;

		return false;
	}

	bool HasInvite(string uid)
	{
		for (int i = 0; i < Invites.Count(); ++i)
		{
			if (Invites[i] && Invites[i].GetID() == uid)
			{
				return true;
			}
		}

		return false;
	}

	int GetMoneyDeposited()
	{
		return MoneyDeposited;
	}

	void RemoveMoney(int amount)
	{
		MoneyDeposited -= amount;
	}

	void AddMoney(int amount)
	{
		MoneyDeposited += amount;
	}

	void SetMoney(int amount)
	{
		MoneyDeposited = amount;
	}
};
