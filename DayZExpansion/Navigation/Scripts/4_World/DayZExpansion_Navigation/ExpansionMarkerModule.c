/**
 * ExpansionMarkerModule.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

[CF_RegisterModule(ExpansionMarkerModule)]
class ExpansionMarkerModule: CF_ModuleWorld
{
	protected static ExpansionMarkerModule s_Instance;
	protected ExpansionMarkerClientData m_CurrentData;
	protected ref array<ref ExpansionMarkerClientData> m_AllData;
	protected int m_Visibility[5];
	protected int m_PreviousVisibility[5];
	protected float m_TimeAccumulator = 0;
	private ref array<ref Expansion3DMarker> m_3DMarkers;
	private string m_DeathMarkerUID;
	
	void ExpansionMarkerModule()
	{
		s_Instance = this;

		ExpansionSettings.SI_Map.Insert(OnSettingsUpdated);
#ifdef EXPANSIONMODGROUPS
		ExpansionSettings.SI_Party.Insert(OnSettingsUpdated);
#endif
	}

	// ------------------------------------------------------------
	// ExpansionMarkerModule Destructor
	// ------------------------------------------------------------
	void ~ExpansionMarkerModule()
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::~ExpansionMarkerModule - Start");
		#endif
		
		delete m_3DMarkers;
		
		ExpansionSettings.SI_Map.Remove(OnSettingsUpdated);
#ifdef EXPANSIONMODGROUPS
		ExpansionSettings.SI_Party.Remove(OnSettingsUpdated);
#endif

		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::~ExpansionMarkerModule - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule OnInit
	// ------------------------------------------------------------
	override void OnInit()
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnInit - Start");
		#endif
		
		super.OnInit();

		EnableMissionFinish();
		EnableMissionLoaded();
		Expansion_EnableRPCManager();
		Expansion_RegisterClientRPC("RPC_CreateDeathMarker");
		//EnableSettingsChanged();
		#ifndef SERVER
		EnableUpdate();
		#endif

		m_3DMarkers = new array<ref Expansion3DMarker>();
		m_AllData = new array<ref ExpansionMarkerClientData>();

		m_CurrentData = NULL;
		
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnInit - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule OnMissionLoaded
	// ------------------------------------------------------------
	override void OnMissionLoaded(Class sender, CF_EventArgs args)
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnMissionLoaded - Start");
		#endif

		super.OnMissionLoaded(sender, args);
		
		if (GetGame().IsDedicatedServer())
			return;

		if (m_AllData.Count() == 0)
			ReadLocalServerMarkers();

		if ( IsMissionOffline() )
		{
			if ( m_AllData.Count() == 0 )
				m_AllData.Insert( new ExpansionMarkerClientData() );

			m_CurrentData = m_AllData[0];
		} else if ( IsMissionClient() )
		{
			string address;
			int port;
			if ( GetGame().GetHostAddress( address, port ) )
			{
				bool found = false;

				foreach (ExpansionMarkerClientData clientData: m_AllData)
				{
					if ( clientData.Equals( address, port ) )
					{
						m_CurrentData = clientData;

						found = true;
						break;
					}
				}

				if ( !found )
				{
					int index = m_AllData.Insert( new ExpansionMarkerClientData() );
					m_AllData[index].m_IP = address;
					m_AllData[index].m_Port = port;
					m_CurrentData = m_AllData[index];
				}
			} else
			{
				Error( "[ExpansionMarkerModule] Failed to get the host address." );
			}
		}

		SaveLocalServerMarkers();

		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnMissionLoaded - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule OnMissionFinish
	// ------------------------------------------------------------
	override void OnMissionFinish(Class sender, CF_EventArgs args)
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnMissionFinish - Start");
		#endif

		super.OnMissionFinish(sender, args);
		
		if (GetGame().IsDedicatedServer())
			return;

		//! @note non-persistent personal markers won't be saved (this is handled in ExpansionMarkerClientData::OnStoreSave),
		//! but we need to remove them from the m_CurrentData cache else they will still be there when (re)connecting to a server
		if (m_CurrentData)
		{
			for (int i = m_CurrentData.m_PersonalMarkers.Count() - 1; i >= 0; i--)
			{
				ExpansionMarkerData markerData = m_CurrentData.m_PersonalMarkers[i];
				if (!markerData.m_Persist)
					m_CurrentData.m_PersonalMarkers.RemoveOrdered(i);
			}
		}

		SaveLocalServerMarkers();

		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnMissionFinish - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule OnSettingsUpdated
	// ------------------------------------------------------------
	
	void OnSettingsUpdated()
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnSettingsUpdated - Start");
		#endif

		Refresh();

		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::OnSettingsUpdated - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule SaveLocalServerMarkers
	// ------------------------------------------------------------
	void SaveLocalServerMarkers()
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::SaveLocalServerMarkers - Start");
		#endif
		
		FileSerializer file = new FileSerializer;
		
		if ( file.Open( EXPANSION_CLIENT_MARKERS, FileMode.WRITE ) )
		{
			file.Write( EXPANSION_VERSION_MAP_MARKER_SAVE );

			file.Write( 5 );
			file.Write( m_Visibility[0] );
			file.Write( m_Visibility[1] );
			file.Write( m_Visibility[2] );
			file.Write( m_Visibility[3] );
			file.Write( m_Visibility[4] );
			
			file.Write( m_AllData.Count() );
			
			for ( int i = 0; i < m_AllData.Count(); ++i )
				m_AllData[i].OnStoreSave( file );
			
			file.Close();
		}
		
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::SaveLocalServerMarkers - End");
		#endif
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule ReadLocalServerMarkers
	// ------------------------------------------------------------	
	bool ReadLocalServerMarkers()
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - Start");
		#endif
		
		FileSerializer ctx = new FileSerializer;

		if ( ctx.Open( EXPANSION_CLIENT_MARKERS, FileMode.READ ) )
		{
			int version;
			if ( !ctx.Read( version ) )
			{
				ctx.Close();
				return false;
			}

			if ( version >= 8 )
			{
				int countVis;
				if ( Expansion_Assert_False( ctx.Read( countVis ), "[" + this + "] Failed reading m_Visibility array length" ) )
				{
					ctx.Close();
					
					#ifdef EXPANSION_MARKER_MODULE_DEBUG
					EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
					#endif
					
					return false;
				}

				for ( int j = 0; j < countVis; ++j )
				{
					int vis;
					if ( Expansion_Assert_False( ctx.Read( vis ), "[" + this + "] Failed reading m_Visibility[" + j + "]" ) )
					{
						ctx.Close();
						
						#ifdef EXPANSION_MARKER_MODULE_DEBUG
						EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
						#endif
						
						return false;
					}

					m_Visibility[j] = vis;
				}
			}

			int countArray;
			if ( Expansion_Assert_False( ctx.Read( countArray ), "[" + this + "] Failed reading countArray" ) )
			{
				ctx.Close();
				
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
				#endif
				
				return false;
			}

			if ( version < 8 )
			{
				m_AllData.Insert( new ExpansionMarkerClientData() );
			}

			for ( int u = 0; u < countArray; ++u )
			{
				string ip;
				int port;
				
				if ( Expansion_Assert_False( ctx.Read( ip ), "[" + this + "] Failed reading ip" ) )
				{
					#ifdef EXPANSION_MARKER_MODULE_DEBUG
					EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
					#endif
					
					return false;
				}
				
				if ( Expansion_Assert_False( ctx.Read( port ), "[" + this + "] Failed reading port" ) )
				{
					#ifdef EXPANSION_MARKER_MODULE_DEBUG
					EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
					#endif
					
					return false;
				}

				ExpansionMarkerClientData newServer = null;
				for ( int i = 0; i < m_AllData.Count(); ++i )
				{
					if ( m_AllData[i].Equals( ip, port ) )
					{
						newServer = m_AllData[i];
						break;
					}
				}
				
				if ( !newServer )
				{
					newServer = new ExpansionMarkerClientData();
					newServer.m_IP = ip;
					newServer.m_Port = port;
					m_AllData.Insert( newServer );
				}
				
				if ( !newServer.OnStoreLoad( ctx, version ) )
				{
					ctx.Close();
					
					#ifdef EXPANSION_MARKER_MODULE_DEBUG
					EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return false");
					#endif
					
					return false;
				}
			}
			
			if ( version < 10 )
			{
				for ( int l1 = 1; l1 < m_AllData.Count(); ++l1 )
					for ( int l2 = l1; l2 < m_AllData.Count(); ++l2 )
						if ( m_AllData[l1].Equals( m_AllData[l2] ) )
						{
							m_AllData.Remove(l2);
							l2 -= 1;
						}
			}
		}

		//! Override quick markers, so they are always visible no matter what
		SetVisibility( ExpansionMapMarkerType.PARTY_QUICK, EXPANSION_MARKER_VIS_WORLD | EXPANSION_MARKER_VIS_MAP );

		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::ReadLocalServerMarkers - End and return true");
		#endif
		
		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule CreateServerMarker
	// ------------------------------------------------------------
	ExpansionMarkerData CreateServerMarker( string name, string icon, vector position, int color, bool marker3D, string uid = "" )
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::CreateServerMarker - Start");
		#endif
		
		if ( uid == "" )
			uid = name + Math.RandomInt( 0, int.MAX );

		ExpansionMarkerData marker = ExpansionMarkerData.Create( ExpansionMapMarkerType.SERVER, uid );

		marker.SetName( name );
		marker.SetIconName( icon );
		marker.SetPosition(position );
		marker.SetColor( color );
		marker.Set3D( marker3D );

		if ( IsMissionHost() )
		{
			if ( !GetExpansionSettings().GetMap() )
			{
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::CreateServerMarker - End and return NULL");
				#endif
				
				return NULL;
			}

			GetExpansionSettings().GetMap().AddServerMarker( marker );

			#ifdef EXPANSION_MARKER_MODULE_DEBUG
			EXPrint("ExpansionMarkerModule::CreateServerMarker - End and return marker: " + marker.ToString());
			#endif
			
			return marker;
		}

		// TODO: send rpc with permission system for creating on the client.
		
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::CreateServerMarker - End and return NULL");
		#endif
		
		return NULL;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule RemoveServerMarker
	// ------------------------------------------------------------	
	void RemoveServerMarker( string uid )
	{
		if ( !IsMissionHost() || !GetExpansionSettings().GetMap() )		
			return;

		GetExpansionSettings().GetMap().RemoveServerMarker( uid );
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule CreateMarker
	// ------------------------------------------------------------
	bool CreateMarker(  ExpansionMarkerData data )
	{
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::CreateMarker - Start");
		#endif
		
		int type = data.GetType();
		switch ( type )
		{
		case ExpansionMapMarkerType.PERSONAL:
			m_CurrentData.PersonalInsert( data );
			break;
#ifdef EXPANSIONMODGROUPS
		case ExpansionMapMarkerType.PARTY:
			m_CurrentData.PartyInsert( data );
			break;
#endif
		default:
			Error( "Marker type " + typename.EnumToString( ExpansionMapMarkerType, type ) + " not supported." );
			return false;
		}

		Refresh();
		
		#ifdef EXPANSION_MARKER_MODULE_DEBUG
		EXPrint("ExpansionMarkerModule::CreateMarker - End and return");
		#endif

		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule RemovePersonalMarkerByUID
	// ------------------------------------------------------------
	bool RemovePersonalMarkerByUID(string uid)
	{
		int index;
		ExpansionMarkerData currentData;
		for (index = 0; index < m_CurrentData.PersonalGet().Count(); ++index)
		{
			currentData = m_CurrentData.PersonalGet()[index];
			if (currentData.GetUID() == uid)
			{
				m_CurrentData.PersonalRemove(index);
				Refresh();
				
				return true;
			}
		}

		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule UpdateMarker
	// ------------------------------------------------------------
	bool UpdateMarker(  ExpansionMarkerData data )
	{		
		int type = data.GetType();
		switch ( type )
		{
		case ExpansionMapMarkerType.PERSONAL:
			// doesn't have to do anything
			break;
#ifdef EXPANSIONMODGROUPS
		case ExpansionMapMarkerType.PARTY:
			m_CurrentData.PartyUpdate( data );
			break;
#endif
		default:
			Error( "Marker type " + typename.EnumToString( ExpansionMapMarkerType, type ) + " not supported." );
			return false;
		}

		Refresh();

		return true;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule GetData
	// ------------------------------------------------------------
	ExpansionMarkerClientData GetData()
	{
		return m_CurrentData;
	}
	
	#ifndef SERVER
	override void OnUpdate(Class sender, CF_EventArgs args)
	{
		super.OnUpdate(sender, args);

		auto update = CF_EventUpdateArgs.Cast(args);

		m_TimeAccumulator += update.DeltaTime;
		if ( m_TimeAccumulator > 1.0 )
		{
			Refresh();
			
			m_TimeAccumulator = 0;
		}

		for ( int i = m_3DMarkers.Count() - 1; i >= 0; i-- )
		{
			if ( !m_3DMarkers[i] || !m_3DMarkers[i].Update( update.DeltaTime ) )
			{
				Expansion3DMarker marker = m_3DMarkers[i];
				m_3DMarkers.Remove(i);
				delete marker;
			} 
		}
	}
	#endif
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule CanCreateMarker
	// ------------------------------------------------------------
	private bool CanCreateMarker( ExpansionMarkerData other, out Expansion3DMarker marker )
	{		
		if ( other.GetType() != ExpansionMapMarkerType.PARTY_QUICK )
		{
			if ( !other.Is3D() )
			{
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::CanCreateMarker - End and return true");
				#endif
				return true;
			}
			
			if ( !other.IsWorldVisible() )
			{
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::CanCreateMarker - End and return true");
				#endif
				return true;
			}
			if ( !IsWorldVisible( other.GetType() ) )
			{
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::CanCreateMarker - End and return true");
				#endif
				return true;
			}
		}

		for ( int i = 0; i < m_3DMarkers.Count(); ++i )
		{
			if ( !m_3DMarkers[i] )
				continue;

			ExpansionMarkerData data = m_3DMarkers[i].GetMarkerData();
			if ( data != NULL && data == other )
			{
				marker = m_3DMarkers[i];
				
				#ifdef EXPANSION_MARKER_MODULE_DEBUG
				EXPrint("ExpansionMarkerModule::CanCreateMarker - End and return true");
				#endif
				
				return true;
			}
		}

		return false;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule Refresh
	// ------------------------------------------------------------
	void Refresh()
	{		
		if ( !m_CurrentData )
			return;

		if (!GetExpansionSettings().GetMap(false).IsLoaded())
			return;
#ifdef EXPANSIONMODGROUPS
		if (!GetExpansionSettings().GetParty(false).IsLoaded())
			return;
#endif

		//Print( "ExpansionMarkerModule::Refresh" );

		m_CurrentData.OnRefresh();

		array< ExpansionMarkerData > markers = m_CurrentData.GetAll();
		for ( int i = 0; i < markers.Count(); ++i )
		{
			Expansion3DMarker marker;
			if ( !CanCreateMarker( markers[i], marker ) )
			{
				marker = new Expansion3DMarker( markers[i] );
				m_3DMarkers.Insert( marker );
			}
		}

		ExpansionUIScriptedMenu menu;
		if ( Class.CastTo( menu, GetGame().GetUIManager().FindMenu( MENU_EXPANSION_MAP ) ) )
		{
			menu.Refresh();
		}

		// CanCreateMarker != removal
		// CanCreate3DMarker != removal
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule SetVisibility
	// ------------------------------------------------------------
	int SetVisibility(  ExpansionMarkerData data, int vis )
	{
		int type = data.GetType();

		switch ( type )
		{
		case ExpansionMapMarkerType.PERSONAL:
			return m_CurrentData.PersonalSetVisibility( data, vis );
#ifdef EXPANSIONMODGROUPS
		case ExpansionMapMarkerType.PARTY:
			return m_CurrentData.PartySetVisibility( data.GetUID(), vis );
		case ExpansionMapMarkerType.PLAYER:
			return m_CurrentData.PartyPlayerSetVisibility( data.GetUID(), vis );
#endif
		case ExpansionMapMarkerType.SERVER:
			return m_CurrentData.ServerSetVisibility( data.GetUID(), vis );
		}

		return 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule FlipVisibility
	// ------------------------------------------------------------
	int FlipVisibility(  ExpansionMarkerData data, int vis )
	{
		int type = data.GetType();
		
		switch ( type )
		{
		case ExpansionMapMarkerType.PERSONAL:
			return m_CurrentData.PersonalFlipVisibility( data, vis );
#ifdef EXPANSIONMODGROUPS
		case ExpansionMapMarkerType.PARTY:
			return m_CurrentData.PartyFlipVisibility( data.GetUID(), vis );
		case ExpansionMapMarkerType.PLAYER:
			return m_CurrentData.PartyPlayerFlipVisibility( data.GetUID(), vis );
#endif
		case ExpansionMapMarkerType.SERVER:
			return m_CurrentData.ServerFlipVisibility( data.GetUID(), vis );
		}

		return 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule RemoveVisibility
	// ------------------------------------------------------------
	int RemoveVisibility(  ExpansionMarkerData data, int vis )
	{
		int type = data.GetType();
		
		switch ( type )
		{
		case ExpansionMapMarkerType.PERSONAL:
			return m_CurrentData.PersonalRemoveVisibility( data, vis );
#ifdef EXPANSIONMODGROUPS
		case ExpansionMapMarkerType.PARTY:
			return m_CurrentData.PartyRemoveVisibility( data.GetUID(), vis );
		case ExpansionMapMarkerType.PLAYER:
			return m_CurrentData.PartyPlayerRemoveVisibility( data.GetUID(), vis );
#endif
		case ExpansionMapMarkerType.SERVER:
			return m_CurrentData.ServerRemoveVisibility( data.GetUID(), vis );
		}

		return 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule SetVisibility
	// ------------------------------------------------------------
	int SetVisibility( int type, int vis )
	{
		type -= 1;

		m_Visibility[type] = m_Visibility[type] | vis;

		return m_Visibility[type];
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule FlipVisibility
	// ------------------------------------------------------------
	int FlipVisibility( int type, int vis )
	{
		type -= 1;

		if ( ( m_Visibility[type] & vis ) != 0 )
		{
			m_Visibility[type] = m_Visibility[type] & ~vis;
		} else
		{
			m_Visibility[type] = m_Visibility[type] | vis;
		}

		return m_Visibility[type];
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule RemoveVisibility
	// ------------------------------------------------------------
	int RemoveVisibility( int type, int vis )
	{
		type -= 1;
		
		m_PreviousVisibility[type] = m_Visibility[type];
		m_Visibility[type] = m_Visibility[type] & ~vis;

		return m_Visibility[type];
	}

	int RestoreVisibility( int type, int vis )
	{
		type -= 1;

		m_Visibility[type] = m_Visibility[type] | (m_PreviousVisibility[type] & vis);

		return m_Visibility[type];
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule ClearVisibility
	// ------------------------------------------------------------
	int ClearVisibility( int type )
	{
		type -= 1;
		
		m_Visibility[type] = 0;
		return m_Visibility[type];
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule GetVisibility
	// ------------------------------------------------------------
	int GetVisibility( int type )
	{
		type -= 1;
		
		return m_Visibility[type];
	}
	
	int GetPreviousVisibility( int type )
	{
		type -= 1;
		
		return m_PreviousVisibility[type];
	}

	// ------------------------------------------------------------
	// ExpansionMarkerModule IsVisible
	// ------------------------------------------------------------
	bool IsVisible( int type )
	{
		type -= 1;
		
		return m_Visibility[type] != 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule IsWorldVisible
	// ------------------------------------------------------------
	bool IsWorldVisible( int type )
	{
		type -= 1;

		if ( type == ExpansionMapMarkerType.SERVER && !GetExpansionSettings().GetMap().EnableServerMarkers )
			return false;
	#ifdef EXPANSIONMODGROUPS
		if ( type == ExpansionMapMarkerType.PARTY_QUICK && !GetExpansionSettings().GetParty().EnableQuickMarker )
			return false;
		if ( type == ExpansionMapMarkerType.PLAYER && !GetExpansionSettings().GetParty().ShowPartyMember3DMarkers )
			return false;
	#endif
		
		return (m_Visibility[type] & EXPANSION_MARKER_VIS_WORLD) != 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule IsMapVisible
	// ------------------------------------------------------------
	bool IsMapVisible( int type )
	{
		type -= 1;

		if ( type == ExpansionMapMarkerType.SERVER && !GetExpansionSettings().GetMap().EnableServerMarkers )
			return false;
	#ifdef EXPANSIONMODGROUPS
		if ( type == ExpansionMapMarkerType.PARTY_QUICK && !GetExpansionSettings().GetParty().EnableQuickMarker )
			return false;
		if ( type == ExpansionMapMarkerType.PLAYER && !GetExpansionSettings().GetParty().ShowPartyMemberMapMarkers )
			return false;
	#endif

		return (m_Visibility[type] & EXPANSION_MARKER_VIS_MAP) != 0;
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule CreateDeathMarkerServer
	// Called on server
	// ------------------------------------------------------------	
	void CreateDeathMarkerServer(vector pos, PlayerIdentity identity)
	{
		if (!IsMissionHost())
			return;
		
		auto rpc = Expansion_CreateRPC("RPC_CreateDeathMarker");
		rpc.Write(pos);
		rpc.Expansion_Send(true, identity);
	}
		
	// ------------------------------------------------------------
	// ExpansionMarkerModule RPC_CreateDeathMarker
	// Called on client
	// ------------------------------------------------------------
	private void RPC_CreateDeathMarker(PlayerIdentity sender, Object target, ParamsReadContext ctx)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_2(ExpansionTracing.MARKER, this, "RPC_CreateDeathMarker").Add(sender).Add(ctx);
#endif
		
		vector pos;
		if (!ctx.Read(pos))
		{
			Error(ToString() + "::RPC_CreateDeathMarker - Could not read position");
		}
		
		CreateDeathMarkerClient(pos);
	}
	
	// ------------------------------------------------------------
	// ExpansionMarkerModule CreateDeathMarker
	// Called on client
	// ------------------------------------------------------------
	void CreateDeathMarkerClient(vector pos)
	{
		ExpansionMarkerData markerData;
		if (m_DeathMarkerUID != string.Empty)
		{
			markerData = GetData().PersonalGetByUID(m_DeathMarkerUID);
			if (markerData)
			{
				markerData.SetPosition(pos);
				UpdateMarker(markerData);
			}
		}
		else
		{
			markerData = ExpansionMarkerData.Create(ExpansionMapMarkerType.PERSONAL);
			markerData.SetName("You died here!");
			markerData.SetIcon("Skull 3");
			markerData.SetColor(ARGB(255,106,0,0));
			markerData.SetPosition(pos);
			markerData.Set3D(GetExpansionSettings().GetMap().CanCreate3DMarker);
			markerData.SetLockState(true);
			CreateMarker(markerData);
			m_DeathMarkerUID = markerData.GetUID();
		}
	}
	
	static ExpansionMarkerModule GetModuleInstance()
	{
		return s_Instance;
	} 
};
