/**
 * ServerBrowserMenuNew.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum ExpansionTabType: TabType
{
	DIRECT,
	EXPANSION
}

modded class ServerBrowserMenuNew
{
	protected ref ExpansionDirectConnectTab m_DirectTab;

	protected int EXPANSION_DIRECT_TAB_INDEX;
	protected bool m_IsDirect;

	// ------------------------------------------------------------
	// Override Init
	// ------------------------------------------------------------
	override Widget Init()
	{
		layoutRoot = super.Init();

		EXPANSION_DIRECT_TAB_INDEX = m_Tabber.AddTab("DIRECT");
		m_DirectTab = new ExpansionDirectConnectTab(layoutRoot.FindAnyWidget("Tab_" + EXPANSION_DIRECT_TAB_INDEX), this, ExpansionTabType.DIRECT);

#ifdef DAYZ_1_21
		GetDayZGame().Expansion_SetGameVersionText(m_Version);
#endif

		return layoutRoot;
	}

	// ------------------------------------------------------------
	// Override GetSelectedTab
	// ------------------------------------------------------------
	override ServerBrowserTab GetSelectedTab()
	{
		switch (m_Tabber.GetSelectedIndex())
		{
			case EXPANSION_DIRECT_TAB_INDEX:
			{
				IsInDirect(true);
				return m_DirectTab;
			}
		}

		IsInDirect(false);
		//! Returning super.GetSelectedTab() directly returns NULL for some reason!?!
		//! Assigning to local var first then returning that works.
		ServerBrowserTab tab = super.GetSelectedTab();
		return tab;
	}

#ifdef DAYZ_1_21
	override void Refresh()
	{
		super.Refresh();

		GetDayZGame().Expansion_SetGameVersionText(m_Version);
	}
#endif

	// ------------------------------------------------------------
	// Expansion ShowPlayPanel
	// ------------------------------------------------------------
	void IsInDirect(bool state)
	{
		m_IsDirect = state;
	}

	// ------------------------------------------------------------
	// Expansion ConnectDirect
	// ------------------------------------------------------------
	void ConnectDirect()
	{
		string ip;
		int port;
		int connected;

		ip = m_DirectTab.GetIP();
		port = m_DirectTab.GetPort();

		if (ip && port)
		{
			GetGame().SetProfileString("ExpansionDirectConnectIP", ip);
			GetGame().SetProfileString("ExpansionDirectConnectPort", port.ToString());
			GetGame().SaveProfile();

			g_Game.ConnectFromServerBrowser( ip, port, "" );
		}
		else if ( !ip || !port )
		{
			GetGame().GetUIManager().ShowDialog( "#STR_EXPANSION_DIRECT_CONNECT_DIALOG_TITLE", "#STR_EXPANSION_DIRECT_CONNECT_ERROR", EXPANSION_IDC_Int_DIRECT, DBT_OK, DBB_NONE, DMT_INFO, GetGame().GetUIManager().GetMenu() );
		}
	}

	// ------------------------------------------------------------
	// Override OnClick
	// ------------------------------------------------------------
	override bool OnClick( Widget w, int x, int y, int button )
	{
		if (button == MouseState.LEFT)
		{
			if (w == m_Play)
			{
				if (m_IsDirect)
				{
					ConnectDirect();

					return true;
				}
			}
		}

		return super.OnClick(w, x, y, button);
	}
}