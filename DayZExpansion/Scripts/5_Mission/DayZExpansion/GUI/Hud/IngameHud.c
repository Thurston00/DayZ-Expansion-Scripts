/**
 * IngameHud.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class IngameHud
{
	protected bool m_ExpansionHudNVState;
	protected bool m_ExpansionEarplugState;
	protected bool m_ExpansionNVSetting;

	protected Widget m_NVPanel;
	protected ImageWidget m_NVBatteryIcon;
	protected TextWidget m_NVBatteryVal;
	protected int m_NVBatteryState;

	protected int BATTERY_EMPTY_COLOR = ARGB(255, 231, 76, 60);
	protected int BATTERY_LOW_COLOR = ARGB(255, 243, 156, 18);
	protected int BATTERY_MED_COLOR = ARGB(255, 113, 204, 46);
	protected int BATTERY_FULL_COLOR = ARGB(255, 46, 204, 113);

	protected int m_StaminaBarColor = ARGB(255, 255, 255, 255);
	protected int m_NotifierDividerColor = ARGB(255, 220, 220, 220);
	protected int m_TemperatureBurningColor = ARGB(255, 220, 0, 0);
	protected int m_TemperatureHotColor = ARGB(255, 220, 220, 0);
	protected int m_TemperatureIdealColor = ARGB(255, 220, 220, 220);
	protected int m_TemperatureColdColor = ARGB(255, 0, 206, 209);
	protected int m_TemperatureFreezingColor = ARGB(255, 30, 144, 220);
	protected int m_NotifiersIdealColor = ARGB(255, 220, 220, 220);
	protected int m_NotifiersHalfColor = ARGB(255, 220, 220, 0);
	protected int m_NotifiersLowColor = ARGB(255, 220, 0, 0);

	protected Widget m_EarPlugsPanel;

	void IngameHud()
	{
		ExpansionSettings.SI_General.Insert(Expansion_OnGeneralSettingsUpdated);
	}

	void ~IngameHud()
	{
		ExpansionSettings.SI_General.Remove(Expansion_OnGeneralSettingsUpdated);
	}

	override void Init(Widget hud_panel_widget)
	{
		super.Init(hud_panel_widget);

		m_NVPanel = GetGame().GetWorkspace().CreateWidgets("DayZExpansion/GUI/layouts/expansion_night_vision.layout");
		if (m_NVPanel)
		{
			m_NVPanel.Show(false);

			m_NVBatteryIcon = ImageWidget.Cast(m_NVPanel.FindAnyWidget("NVBatteryIcon"));

			m_NVBatteryIcon.LoadImageFile(0, "DayZExpansion/Core/GUI/icons/hud/battery_empty_64x64.edds");
			m_NVBatteryIcon.LoadImageFile(1, "DayZExpansion/Core/GUI/icons/hud/battery_low_64x64.edds");
			m_NVBatteryIcon.LoadImageFile(2, "DayZExpansion/Core/GUI/icons/hud/battery_med_64x64.edds");
			m_NVBatteryIcon.LoadImageFile(3, "DayZExpansion/Core/GUI/icons/hud/battery_high_64x64.edds");

			m_NVBatteryVal = TextWidget.Cast(m_NVPanel.FindAnyWidget("NVBatteryVal"));
		}

		m_EarPlugsPanel = GetGame().GetWorkspace().CreateWidgets("DayZExpansion/GUI/layouts/expansion_earplugs.layout");
	}

	override void Update(float timeslice)
	{
		super.Update(timeslice);

		if (m_NVPanel.IsVisible())
		{
			UpdateNV();
		}
	}

	override void DisplayTendencyNormal(int key, int tendency, int status)
	{
		ImageWidget w;
		Class.CastTo(w,  m_Notifiers.FindAnyWidget(String("Icon" + m_StatesWidgetNames.Get(key))));

		if (w)
		{
			w.SetImage(Math.Clamp(status - 1, 0, 4));
			float alpha = w.GetAlpha();

			switch (status)
			{
				case 3:
					w.SetColor(m_NotifiersHalfColor);
					m_TendencyStatusCritical.Remove(w);				//! Remove from blinking group
                break;
				case 4:
					w.SetColor(m_NotifiersLowColor);
					m_TendencyStatusCritical.Remove(w);				//! Remove from blinking group
                break;
				case 5:
					if (!m_TendencyStatusCritical.Contains(w))
						m_TendencyStatusCritical.Insert(w, m_NotifiersLowColor);	//! Add to blinking group
                break;
				default:
					w.SetColor(m_NotifiersIdealColor);
					m_TendencyStatusCritical.Remove(w);
                break;
			}
		}
	}

	override void DisplayTendencyTemp( int key, int tendency, int status )
	{
		ImageWidget w = ImageWidget.Cast(m_Notifiers.FindAnyWidget(String("Icon" + m_StatesWidgetNames.Get(key))));
		TextWidget temp_top = TextWidget.Cast(m_Notifiers.FindAnyWidget( "TemperatureValueTop"));
		TextWidget temp_bot = TextWidget.Cast(m_Notifiers.FindAnyWidget( "TemperatureValueBottom"));
		float alpha = w.GetAlpha();

		if (tendency < 0)
		{
			temp_top.Show(true);
			temp_bot.Show(false);
		}
		else
		{
			temp_top.Show(false);
			temp_bot.Show(true);
		}

		switch (status)
		{
			case 2:
				w.SetColor(m_TemperatureHotColor);		//WARNING_PLUS
				m_TendencyStatusCritical.Remove(w);
				w.SetImage(1);
				break;
			case 3:
				w.SetColor(m_TemperatureBurningColor);		//CRITICAL_PLUS
				m_TendencyStatusCritical.Remove(w);
				w.SetImage(0);
				break;
			case 4:
				if (!m_TendencyStatusCritical.Contains(w))		//BLINKING_PLUS
				{
					m_TendencyStatusCritical.Insert(w, m_TemperatureBurningColor);
				}
				w.SetImage(0);
				break;
			case 5:
				w.SetColor(m_TemperatureColdColor);		//WARNING_MINUS
				m_TendencyStatusCritical.Remove(w);
				w.SetImage(3);
				break;
			case 6:
				w.SetColor(m_TemperatureFreezingColor);	//CRITICAL_MINUS
				m_TendencyStatusCritical.Remove(w);
				w.SetImage(4);
				break;
			case 7:													//BLINKING_MINUS
				if (!m_TendencyStatusCritical.Contains(w))
				{
					m_TendencyStatusCritical.Insert(w, m_TemperatureFreezingColor);
				}
				w.SetImage(4);
				break;
			default:
				w.SetColor(m_TemperatureIdealColor);
				m_TendencyStatusCritical.Remove(w);
				w.SetImage(2);
				break;
		}
	}

	void Expansion_OnGeneralSettingsUpdated()
	{
		ExpansionGeneralSettings settings = GetExpansionSettings().GetGeneral();

		m_ExpansionNVSetting = settings.EnableHUDNightvisionOverlay;
		m_StaminaBarColor = settings.HUDColors.Get("StaminaBarColor");
		m_NotifierDividerColor = settings.HUDColors.Get("NotifierDividerColor");
		m_TemperatureBurningColor = settings.HUDColors.Get("TemperatureBurningColor");
		m_TemperatureHotColor = settings.HUDColors.Get("TemperatureHotColor");
		m_TemperatureIdealColor = settings.HUDColors.Get("TemperatureIdealColor");
		m_TemperatureColdColor = settings.HUDColors.Get("TemperatureColdColor");
		m_TemperatureFreezingColor = settings.HUDColors.Get("TemperatureFreezingColor");
		m_NotifiersIdealColor = settings.HUDColors.Get("NotifiersIdealColor");
		m_NotifiersHalfColor = settings.HUDColors.Get("NotifiersHalfColor");
		m_NotifiersLowColor = settings.HUDColors.Get("NotifiersLowColor");

        m_Stamina.SetColor(m_StaminaBarColor);
		m_BadgeNotifierDivider.SetColor(m_NotifierDividerColor);
	}

	override void RefreshHudVisibility()
	{
		super.RefreshHudVisibility();

		if (m_NVPanel)
			m_NVPanel.Show(m_HudState && m_ExpansionHudNVState && m_ExpansionNVSetting);

		if (m_EarPlugsPanel)
			m_EarPlugsPanel.Show(m_HudState && m_ExpansionEarplugState);
	}

	void ShowNV(bool show)
	{
		m_ExpansionHudNVState = show;
		RefreshHudVisibility();
	}

	bool GetNVState()
	{
		return m_ExpansionHudNVState;
	}

	protected void RefreshNVBatteryState(int percent)
	{
		 if (percent <= 10)
		{
			//! 0 - 10% Energy
			m_NVBatteryIcon.SetImage(0);
			m_NVBatteryIcon.SetColor(BATTERY_EMPTY_COLOR);
		}
		else if (percent <= 33)
		{
			//! 10 - 33% Energy
			m_NVBatteryIcon.SetImage(1);
			m_NVBatteryIcon.SetColor(BATTERY_LOW_COLOR);
		}
		else if (percent <= 66)
		{
			//! 33 - 66% Energy
			m_NVBatteryIcon.SetImage(2);
			m_NVBatteryIcon.SetColor(BATTERY_MED_COLOR);
		}
		else
		{
			//! 66 - 100% Energy
			m_NVBatteryIcon.SetImage(3);
			m_NVBatteryIcon.SetColor(BATTERY_FULL_COLOR);
		}

		m_NVBatteryVal.SetText(percent.ToString() + "%");
	}

	void SetNVBatteryState(int percent)
	{
		m_NVBatteryState = percent;
	}

	protected void UpdateNV()
	{
		RefreshNVBatteryState(m_NVBatteryState);
	}

	bool GetEarplugsState()
	{
		return m_ExpansionEarplugState;
	}

	void ToggleEarplugs()
	{
		m_ExpansionEarplugState = !m_ExpansionEarplugState;

		if (m_ExpansionEarplugState)
		{
			if (GetExpansionClientSettings())
			{
				GetGame().GetSoundScene().SetSoundVolume(GetExpansionClientSettings().EarplugLevel, 1);
			}
			else
			{
				GetGame().GetSoundScene().SetSoundVolume(0.5, 1);
			}
		}
		else
		{
			GetGame().GetSoundScene().SetSoundVolume(g_Game.m_volume_sound, 1);
		}

		RefreshHudVisibility();
	}

	void UpdateEarplugs()
	{
		if (GetExpansionClientSettings())
		{
			GetGame().GetSoundScene().SetSoundVolume(GetExpansionClientSettings().EarplugLevel, 1);
			RefreshHudVisibility();
		}
	}
};
