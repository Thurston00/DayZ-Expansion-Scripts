/**
 * ExpansionChat.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class Chat
{
	ref ExpansionChatUIWindow m_ExChatUI;
	private bool m_HideChatToggle = true;
		
	void Chat()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "Chat");
#endif
	}
	
	override void Init(Widget root_widget)
	{
		auto trace = EXTrace.Start(ExpansionTracing.CHAT);

		m_RootWidget = root_widget;

		if (m_RootWidget && !m_ExChatUI)
		{
			m_ExChatUI = new ExpansionChatUIWindow(m_RootWidget, this);
			m_ExChatUI.Hide();
		}
	}
	
	void OnChatInputShow()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "OnChatInputShow");
#endif
		
		m_ExChatUI.Show();
	}

	void OnChatInputHide()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "OnChatInputHide");
#endif

		m_ExChatUI.Hide();
	}
	
	void HideChatToggle()
	{
		m_HideChatToggle = !m_HideChatToggle;
		m_ExChatUI.GetLayoutRoot().Show(m_HideChatToggle);
	}

	bool GetChatToggleState()
	{
		return m_HideChatToggle;
	}
		
	override void Add(ChatMessageEventParams params)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "Add");
#endif

		if (m_ExChatUI)
			m_ExChatUI.Add(params);
	}

	override void AddInternal(ChatMessageEventParams params)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "AddInternal");
#endif
		
		if (m_ExChatUI)
			m_ExChatUI.AddInternal(params);
	}
		
	override void Clear()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.CHAT, this, "Clear");
#endif

		m_ExChatUI.ClearChatLines();
	}
		
	ExpansionChatUIWindow GetChatWindow()
	{
		return m_ExChatUI;
	}
};