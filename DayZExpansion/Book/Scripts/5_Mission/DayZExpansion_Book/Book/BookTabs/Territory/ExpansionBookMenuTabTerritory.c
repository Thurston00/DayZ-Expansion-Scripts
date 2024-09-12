/**
 * ExpansionBookMenuTabTerritory.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONMODBASEBUILDING
class ExpansionBookMenuTabTerritory: ExpansionBookMenuTabBase
{
	ref ExpansionTerritoryModule m_TerritoryModule;
	TerritoryFlag m_Flag;
	ref ExpansionTerritory m_Territory;
	ref ExpansionBookMenuTabTerritoryController m_TerritoryTabController;
	
	//! Main page widgets
	private Widget book_territory_tab;
	private Widget book_menu_content_left;
	private TextWidget member_list_info;
	private TextWidget player_list_info;
	
	private ButtonWidget territory_delete_button;
	private TextWidget territory_delete_button_label;
	
	private EditBoxWidget player_list_filter_editbox;
	private ButtonWidget player_list_filter_clear;
	private ImageWidget player_list_filter_clear_icon;
	
	private int m_PlayerSearchRadius = 25;
	
	private bool m_IsEditingMember = false;
	private bool m_ChangeList = false;
	private bool m_ShowInvitesTab = false;
	
	ref ExpansionBookMenuTabTerritoryMemberEdit m_TerritoryTabMemberEdit;
	ref ExpansionDialog_DeleteTerritory m_DeleteDialog;
	ref ExpansionDialog_LeaveTerritory m_LeaveDialog;
	ref ExpansionDialog_InviteTerritory m_InviteDialog;
	
	ref SyncPlayer m_Player;
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory Constructor
	// ------------------------------------------------------------
	void ExpansionBookMenuTabTerritory(ExpansionBookMenu book_menu)
	{
		m_BookMenu = book_menu;
		
		if (!m_TerritoryModule)
			m_TerritoryModule = ExpansionTerritoryModule.Cast(CF_ModuleCoreManager.Get(ExpansionTerritoryModule));
		
		if (!m_TerritoryTabController)
			m_TerritoryTabController = ExpansionBookMenuTabTerritoryController.Cast(GetController());
				
		RegisterTerritoryTabs();
		
		SetTerritory();
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory RegisterTerritoryTabs
	// ------------------------------------------------------------
	void RegisterTerritoryTabs()
	{
		if (!m_TerritoryTabMemberEdit)
		{
			m_TerritoryTabMemberEdit = new ExpansionBookMenuTabTerritoryMemberEdit(m_BookMenu);
			m_TerritoryTabMemberEdit.SetParentTab(this);
			m_BookMenu.RegisterTab(m_TerritoryTabMemberEdit);
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory UnregisterTerritoryTabs
	// ------------------------------------------------------------	
	void UnregisterTerritoryTabs()
	{
		if (m_TerritoryTabMemberEdit)
		{
			m_BookMenu.UnregisterTab(m_TerritoryTabMemberEdit);
			m_TerritoryTabMemberEdit.Destroy();
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetMemberEditTab
	// ------------------------------------------------------------	
	ExpansionBookMenuTabTerritoryMemberEdit GetMemberEditTab()
	{
		return m_TerritoryTabMemberEdit;
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetLayoutFile
	// ------------------------------------------------------------
	override string GetLayoutFile() 
	{
		return "DayZExpansion/Book/GUI/layouts/tabs/territory/expansion_book_tab_territory.layout";
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetControllerType
	// ------------------------------------------------------------	
	override typename GetControllerType() 
	{
		return ExpansionBookMenuTabTerritoryController;
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetTabIconName
	// ------------------------------------------------------------	
	override string GetTabIconName()
	{
		return "Territory";
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetTabName
	// ------------------------------------------------------------	
	override string GetTabName()
	{
		return "#STR_EXPANSION_BOOK_TAB_TERRITORY_MANAGER";
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetTabColor
	// ------------------------------------------------------------	
	override int GetTabColor()
	{
		return ARGB(255,0,0,0);
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory SetTerritory
	// ------------------------------------------------------------
	void SetTerritory()
	{
		PlayerBase player;
		if (Class.CastTo(player, GetGame().GetPlayer()))
			m_Flag = m_TerritoryModule.GetFlagAtPosition3D(player.GetPosition());
		
		if (!m_Flag)
		{
			m_Territory = null;
			return;
		}
		
		map<int, ref ExpansionTerritory> territories = m_TerritoryModule.GetTerritories();
		m_Territory = territories.Get(m_Flag.GetTerritoryID());
		
		if (!m_Territory)
		{
			m_Flag = null;
			m_Territory = null;
			return;
		}
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory SetView
	// ------------------------------------------------------------	
	void SetView()
	{			
		SetTerritoryView();
	}
	
	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory SetTerritoryView
	// ------------------------------------------------------------	
	void SetTerritoryView()
	{
		if (!m_Territory)
			return;

		m_TerritoryTabController.TerritroyName = m_Territory.GetTerritoryName() + " #STR_EXPANSION_BOOK_MEMBERS";
		m_TerritoryTabController.NotifyPropertyChanged("TerritroyName");
		
		if (GetPlayerRank() == ExpansionTerritoryRank.ADMIN)
		{
			m_TerritoryTabController.DeleteButtonLabel = "#STR_EXPANSION_BOOK_TERRITORY_DELETE";
			m_TerritoryTabController.NotifyPropertyChanged("DeleteButtonLabel");
		}
		else
		{
			m_TerritoryTabController.DeleteButtonLabel = "#STR_EXPANSION_BOOK_TERRITORY_LEAVE";
			m_TerritoryTabController.NotifyPropertyChanged("DeleteButtonLabel");
		}
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory LoadMembers
	// ------------------------------------------------------------	
	void LoadMembers()
	{
		if (m_Territory && m_Territory.GetTerritoryMembers().Count() > 0)
		{
			array<ref ExpansionTerritoryMember> members = m_Territory.GetTerritoryMembers();
			TStringArray memberUIDs = m_Territory.GetTerritoryMemberIDs();
			foreach (ExpansionTerritoryMember memberData: members)
			{
				ExpansionBookMenuTabTerritoryMemberEntry entry = null;
				for (int e = m_TerritoryTabController.TerritoryMemberEntrys.Count() - 1; e >= 0; e--)
				{
					ExpansionBookMenuTabTerritoryMemberEntry existingEntry = m_TerritoryTabController.TerritoryMemberEntrys[e];
					
					if (existingEntry.m_Member && existingEntry.m_Member.GetID() == memberData.GetID())
						entry = existingEntry;
					//! If the entry is related to an old member that is no longer in the territory then remove the entry
					else if (!existingEntry.m_Member || memberUIDs.Find(existingEntry.m_Member.GetID()) == -1)
						m_TerritoryTabController.TerritoryMemberEntrys.RemoveOrdered(e);
				}
				//! If the teritory member has an entry already then skip the entry creation
				if (!entry)
				{
					entry = new ExpansionBookMenuTabTerritoryMemberEntry(this, memberData, m_Territory.GetTerritoryID());
					m_TerritoryTabController.TerritoryMemberEntrys.Insert(entry);
				}
				else
				{
					entry.SetEntry(memberData, false);
				}
			}
		}
		else
		{
			ClearMembers();
		}
		
		/*if (m_Territory && m_Territory.GetTerritoryMembers().Count() == 1)
		{
			member_list_info.Show(true);
			if (m_TerritoryTabController.MemberListInfo != "#STR_EXPANSION_BOOK_TERRITORY_ERROR_NO_ONLINE_MEMBERS")
			{
				m_TerritoryTabController.MemberListInfo = "#STR_EXPANSION_BOOK_TERRITORY_ERROR_NO_ONLINE_MEMBERS";
				m_TerritoryTabController.NotifyPropertyChanged("MemberListInfo");
			}
		}
		else if (m_Territory && m_Territory.GetTerritoryMembers().Count() >= 2)
		{
			member_list_info.Show(false);
		}*/
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory ClearMembers
	// ------------------------------------------------------------	
	void ClearMembers()
	{
		for (int i = 0; i < m_TerritoryTabController.TerritoryMemberEntrys.Count(); ++i)
		{
			ExpansionBookMenuTabTerritoryMemberEntry entry = m_TerritoryTabController.TerritoryMemberEntrys[i];
			entry.Hide();
		}

		m_TerritoryTabController.TerritoryMemberEntrys.Clear();
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory LoadPlayers
	// ------------------------------------------------------------
	void LoadPlayers(string filter)
	{
		if (!m_Territory || !book_menu_content_left.IsVisible()) 
			return;		
		
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		if (!player) 
			return;
	
		filter.ToLower();
			
		string playerName;
		
		set<ref SyncPlayer> players;
		if (GetExpansionSettings().GetTerritory().UseWholeMapForInviteList)
		{
			players = SyncPlayer.Expansion_GetAll();
		}
		else
		{
			players = SyncPlayer.Expansion_GetInSphere(player.GetPosition(), m_PlayerSearchRadius);
		}

		array<ref SyncPlayer> filteredPlayers();
		TStringArray filteredUIDs();

		foreach (SyncPlayer playerSync: players)
		{
			if (playerSync.m_RUID == player.GetIdentity().GetId())
				continue;

			if (IsMember(playerSync.m_RUID) || HasInvite(playerSync.m_RUID))
			{
				continue;
			}

			if (filter != "")
			{
				playerName = playerSync.m_PlayerName;
				playerName.ToLower();
	
				if (playerName.IndexOf(filter) == -1)
					continue;
			}

			filteredPlayers.Insert(playerSync);
			filteredUIDs.Insert(playerSync.m_RUID);
		}

		if (!filteredPlayers.Count())
			ClearPlayers();

		foreach (SyncPlayer filteredPlayer: filteredPlayers)
		{
			bool isInList = false;
			for (int i = m_TerritoryTabController.TerritoryPlayerEntrys.Count() - 1; i >= 0; i--)
			{
				ExpansionBookMenuTabTerritoryPlayerEntry entry = m_TerritoryTabController.TerritoryPlayerEntrys[i];
				if (entry.m_Player.m_RUID == filteredPlayer.m_RUID)
					isInList = true;
				else if (filteredUIDs.Find(entry.m_Player.m_RUID) == -1)
					m_TerritoryTabController.TerritoryPlayerEntrys.RemoveOrdered(i);
			}
			
			if (!isInList)
			{
				ExpansionBookMenuTabTerritoryPlayerEntry newEntry = new ExpansionBookMenuTabTerritoryPlayerEntry(this, filteredPlayer);
				m_TerritoryTabController.TerritoryPlayerEntrys.Insert(newEntry);
			}
		}
		
		/*if (filteredPlayers.Count() > 0)
		{
			m_TerritoryTabController.PlayerListInfo = "#STR_EXPANSION_BOOK_TERRITORY_INVITE_SELECT";
			m_TerritoryTabController.NotifyPropertyChanged("PlayerListInfo");
		}
		else
		{
			m_TerritoryTabController.PlayerListInfo = "#STR_EXPANSION_BOOK_TERRITORY_INVITE_EMPTY";
			m_TerritoryTabController.NotifyPropertyChanged("PlayerListInfo");
		}*/
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory ClearPlayers
	// ------------------------------------------------------------	
	void ClearPlayers()
	{
		for (int i = 0; i < m_TerritoryTabController.TerritoryPlayerEntrys.Count(); ++i)
		{
			ExpansionBookMenuTabTerritoryPlayerEntry entry = m_TerritoryTabController.TerritoryPlayerEntrys[i];
			entry.Hide();
		}
		
		m_TerritoryTabController.TerritoryPlayerEntrys.Clear();
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory IsMember
	// ------------------------------------------------------------
	bool IsMember(string uid)
	{
		return m_Territory.IsMember(uid);
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory HasInvite
	// ------------------------------------------------------------	
	bool HasInvite(string uid)
	{
		if (m_TerritoryModule)
		{
			foreach (ExpansionTerritoryInvite invite: m_TerritoryModule.GetTerritoryInvites())
			{
				if (invite.UID == uid)
					return true;
			}
		}
		
		return false;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetNearbyPlayerIdentitys
	// ------------------------------------------------------------
	array<PlayerIdentity> GetNearbyPlayerIdentitys(vector position, int radius)
	{
		array<Object> objects = new array<Object>;
		array<CargoBase> proxyCargos = new array<CargoBase>;
		array<PlayerIdentity> identitys = new array<PlayerIdentity>;
		
		GetGame().GetObjectsAtPosition(position, radius, objects, proxyCargos);
		
		if (objects.Count() > 0)
		{
			foreach (Object obj : objects)
			{
				PlayerBase player;
				if (Class.CastTo(player, obj))
				{
					identitys.Insert(player.GetIdentity());
				}
			}
		}
		
		return identitys;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnDeleteButtonClick
	// ------------------------------------------------------------
	void OnDeleteButtonClick()
	{
		if (GetPlayerRank() == ExpansionTerritoryRank.ADMIN)
		{
			if (!m_DeleteDialog)
				m_DeleteDialog = new ExpansionDialog_DeleteTerritory(this);
				
			m_DeleteDialog.Show();
		}
		else
		{
			if (!m_LeaveDialog)
				m_LeaveDialog = new ExpansionDialog_LeaveTerritory(this);
			
			m_LeaveDialog.Show();
		}
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetFlag
	// ------------------------------------------------------------	
	TerritoryFlag GetFlag()
	{
		return m_Flag;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetTerritory
	// ------------------------------------------------------------	
	ExpansionTerritory GetTerritory()
	{
		return m_Territory;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetPlayerRank
	// ------------------------------------------------------------	
	ExpansionTerritoryRank GetPlayerRank()
	{
		ExpansionTerritoryRank rank;
		PlayerBase player = PlayerBase.Cast(GetGame().GetPlayer());
		ExpansionTerritoryMember member = m_Territory.GetMember(player.GetIdentity().GetId());
		rank = member.GetRank();
		return rank;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory InvitePlayer
	// ------------------------------------------------------------
	void InvitePlayer(SyncPlayer player)
	{
		m_Player = player;
		
		if (!m_InviteDialog)
			m_InviteDialog = new ExpansionDialog_InviteTerritory(this);
		
		m_InviteDialog.Show();
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetSelectedPlayer
	// ------------------------------------------------------------	
	SyncPlayer GetSelectedPlayer()
	{
		return m_Player;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnRefreshButtonClick
	// ------------------------------------------------------------	
	void OnRefreshButtonClick()
	{
		m_TerritoryTabController.SearchFilter = "#STR_USRACT_SEARCH";
		m_TerritoryTabController.NotifyPropertyChanged("SearchFilter");
		
		LoadPlayers("");
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory IsParentTab
	// ------------------------------------------------------------
	override bool IsParentTab()
	{
		return true;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory CanShow
	// ------------------------------------------------------------	
	override bool CanShow()
	{
		return (m_Flag && m_Territory && GetExpansionSettings().GetBook().EnableTerritoryTab);
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnShow
	// ------------------------------------------------------------
	override void OnShow()
	{
		super.OnShow();
		
		SetView();
		LoadMembers();
		SwitchMovementLockState(true);
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory GetUpdateTickRate
	// ------------------------------------------------------------	
	override float GetUpdateTickRate()
	{
		return 1.0;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory Update
	// ------------------------------------------------------------	
	override void Expansion_Update()
	{
		super.Expansion_Update();

		if (!IsVisible())
			return;
		
		LoadPlayers(player_list_filter_editbox.GetText());
	}

	override void Refresh()
	{
#ifdef EXTRACE
		auto trace = EXTrace.Start(EXTrace.BASEBUILDING, this);
#endif

		SetTerritory();

		if (!m_Territory)
		{
			Hide();
		}
		else
		{
			SetView();
			LoadMembers();
		}
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnMouseEnter
	// ------------------------------------------------------------
	override bool OnMouseEnter(Widget w, int x, int y)
	{
		switch (w)
		{
		case territory_delete_button:
			territory_delete_button_label.SetColor(ARGB(255, 220, 220, 220));
			break;
		case player_list_filter_clear:
			player_list_filter_clear_icon.SetColor(ARGB(255, 220, 220, 220));
			break;
		}
		
		return super.OnMouseEnter(w, x, y);;
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnMouseLeave
	// ------------------------------------------------------------	
	override bool OnMouseLeave(Widget w, Widget enterW, int x, int y)
	{
		switch (w)
		{
		case territory_delete_button:
			territory_delete_button_label.SetColor(ARGB(255, 0, 0, 0));
			break;
		case player_list_filter_clear:
			player_list_filter_clear_icon.SetColor(ARGB(255, 0, 0, 0));
			break;
		}
		
		return super.OnMouseLeave(w, enterW, x, y);
	}

	// ------------------------------------------------------------
	// ExpansionBookMenuTabTerritory OnHide
	// ------------------------------------------------------------
	override void OnHide()
	{
		SwitchMovementLockState(false);

		super.OnHide();
		
		if (m_InviteDialog && m_InviteDialog.IsVisible())
			m_InviteDialog.Hide();
	}
};

class ExpansionBookMenuTabTerritoryController: ExpansionViewController 
{
	//! TODO: These two collections should really be dictionaries mapping UID to entry
	ref ObservableCollection<ref ExpansionBookMenuTabTerritoryMemberEntry> TerritoryMemberEntrys = new ObservableCollection<ref ExpansionBookMenuTabTerritoryMemberEntry>(this);
	ref ObservableCollection<ref ExpansionBookMenuTabTerritoryPlayerEntry> TerritoryPlayerEntrys = new ObservableCollection<ref ExpansionBookMenuTabTerritoryPlayerEntry>(this);
	string MemberListInfo;
	string PlayerListInfo;
	string TerritroyName;
	string DeleteButtonLabel;
	string SearchFilter;
};
#endif
