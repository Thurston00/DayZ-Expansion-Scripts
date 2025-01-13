/**
 * ExpansionSpawSelectionMenuMapMarker.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionSpawSelectionMenuMapMarker : ExpansionMapWidgetBase
{
	protected int m_Index;
	protected ref ExpansionSpawnLocation m_Location;
	protected ExpansionRespawnHandlerModule m_RespawnModule;
	protected bool m_HasCooldown = false;
	protected bool m_IsTerritory = false;
	protected bool m_IsLocked = false;
	protected bool m_IsDeathMarker = false;
	protected bool m_IsSelected = false;
	protected string m_LocationKey;
		
	void ExpansionSpawSelectionMenuMapMarker(Widget parent, MapWidget mapWidget, bool autoInit = true)
	{
		CF_Modules<ExpansionRespawnHandlerModule>.Get(m_RespawnModule);
	}
	
	void SetIsTerritory(bool state)
	{
		m_IsTerritory = state;
	}
	
	void UpdateCooldown()
	{
		if (m_RespawnModule)
		{
			int respawnCooldown = GetExpansionSettings().GetSpawn().GetCooldown(m_IsTerritory);
			ExpansionRespawnDelayTimers playerCooldowns = m_RespawnModule.m_PlayerRespawnDelays.GetElement(0);
			if (playerCooldowns)
			{
				ExpansionRespawnDelayTimer timer = playerCooldowns[m_LocationKey];
				if (!timer)
					return;

				if (timer.HasCooldown())
				{
					int cooldownTime = timer.GetTimeDiff();
					int cooldown = respawnCooldown;
					if (GetExpansionSettings().GetSpawn().PunishMultispawn)
						cooldown += timer.GetPunishment();

					if (cooldownTime < cooldown)
						m_HasCooldown = true;

					if (m_HasCooldown)
					{
						if (!m_IsLocked)
							SetLocked();
					}
				}
				else
				{
					OnCooldownEnd();
				}
			}
		}
	}
	
	void OnCooldownEnd()
	{
		m_HasCooldown = false;
		SetUnlocked();
	}
	
	void SetLocked()
	{
		m_IsLocked = true;
		m_Icon.LoadImageFile(0, "set:dayz_gui image:icon_locked_sb");
		SetPrimaryColor(ARGB(255,106,0,0));
	}
	
	void SetUnlocked()
	{
		m_IsLocked = false;
		SetIcon(m_CurrentExpansionIcon);
		if (!m_IsTerritory)
			SetPrimaryColor(ARGB(255,226,65,66));
		else
			SetPrimaryColor(ARGB(255, 0, 102, 204));
	}
	
	void SetLocation(int index, ExpansionSpawnLocation location)
	{
		m_Index = index;
		m_Location = location;
		SetPosition(m_Location.Positions[0]);
	}

	override void Update(float pDt)
	{
		float x, y;
		GetLayoutRoot().GetParent().GetScreenPos(x, y);
		vector mapPos = GetMapWidget().MapToScreen(m_WorldPosition);	
		GetLayoutRoot().SetPos(mapPos[0] - x, mapPos[1] - y, true);
	}
	
	override void Show()
	{
		super.Show();
		
		if (!m_IsDeathMarker)
			UpdateCooldown();
	}
	
	override bool OnDoubleClick(Widget w, int x, int y, int button)
	{
		if (w != NULL && IsEditButton(w) && button == MouseState.LEFT)
		{
			ExpansionSpawnSelectionMenu menu = ExpansionSpawnSelectionMenu.Cast(GetDayZExpansion().GetExpansionUIManager().GetMenu());
			if (menu && !m_HasCooldown)
			{
				menu.Spawn();
				return true;
			}
		}
		
		return super.OnDoubleClick(w, x, y, button);
	}
	
	override void OnMarkerClick()
	{
		if (m_IsDeathMarker || m_IsSelected)
			return;
		
		ExpansionSpawnSelectionMenu spawnSelectionMenu = ExpansionSpawnSelectionMenu.Cast(GetDayZExpansion().GetExpansionUIManager().GetMenu());
		if (!spawnSelectionMenu)
			return;
		
		if (m_HasCooldown)
		{
			spawnSelectionMenu.GetConfirmButton().Show(false);
			spawnSelectionMenu.GetConfirmButton().Enable(false);
			spawnSelectionMenu.ClearSpawnPoint();
			ExpansionNotification(new StringLocaliser("STR_EXPANSION_SPAWNSELECTION_POINT_LOCKED"), new StringLocaliser("STR_EXPANSION_SPAWNSELECTION_POINT_LOCKED_DESC"), EXPANSION_NOTIFICATION_ICON_INFO, COLOR_EXPANSION_NOTIFICATION_ERROR).Create();
			return;
		}
		
		spawnSelectionMenu.SetSpawnPoint(m_Index, m_Location, false);
	}
	
	int GetIndex()
	{
		return m_Index;
	}
	
	override bool CanDrag()
	{
		return false;
	}
	
	void SetIsDeathMarker(bool state)
	{
		m_IsDeathMarker = state;
	}
	
	bool IsDeathMarker()
	{
		return m_IsDeathMarker;
	}
	
	void Lock()
	{
		m_IsSelected = true;
	}
	
	void Unlock()
	{
		m_IsSelected = false;
	}
	
	void SetLocationKey(string key)
	{
		m_LocationKey = key;
	}
	
	string GetLocationKey()
	{
		return m_LocationKey;
	}
};
