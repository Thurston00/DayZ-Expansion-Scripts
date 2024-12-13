/**
 * ExpansionDebugObjects.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionDebugObject: Building
{
	float m_Expansion_DebugOriginX;
	float m_Expansion_DebugOriginY;
	float m_Expansion_DebugOriginZ;
	vector m_Expansion_DebugOrigin;
	bool m_Expansion_DrawDebugSignal;
	int m_Expansion_DebugColor;
	Shape m_Expansion_DebugLine;
	int m_Expansion_DebugLineFlags;

	void ExpansionDebugObject()
	{
		RegisterNetSyncVariableFloat("m_Expansion_DebugOriginX", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_Expansion_DebugOriginY", 0, 0, 2);
		RegisterNetSyncVariableFloat("m_Expansion_DebugOriginZ", 0, 0, 2);
		RegisterNetSyncVariableInt("m_Expansion_DebugLineFlags");
		RegisterNetSyncVariableBool("m_Expansion_DrawDebugSignal");

#ifndef SERVER
		ExpansionString es = GetType();

		if (es.EndsWith("_Red"))
			m_Expansion_DebugColor = 0xFFFF0000;
		else if (es.EndsWith("_Orange"))
			m_Expansion_DebugColor = 0xFFFF8000;
		else if (es.EndsWith("_Yellow"))
			m_Expansion_DebugColor = 0xFFFFFF00;
		else if (es.EndsWith("_Blue"))
			m_Expansion_DebugColor = 0xFF0000FF;
		else if (es.EndsWith("_Cyan"))
			m_Expansion_DebugColor = 0xFF00FFFF;
		else if (es.EndsWith("_Purple"))
			m_Expansion_DebugColor = 0xFF8000FF;
		else if (es.EndsWith("_White"))
			m_Expansion_DebugColor = 0xFFFFFFFF;
		else if (es.EndsWith("_Black"))
			m_Expansion_DebugColor = 0xFF000000;
		else
			m_Expansion_DebugColor = 0xFF00FF00;
#endif
	}

	override void EEInit()
	{
		super.EEInit();

		if (dBodyIsSet(this))
			dBodyDestroy(this);  //! Remove physics
	}

	void ~ExpansionDebugObject()
	{
#ifndef SERVER
		Debug.RemoveShape(m_Expansion_DebugLine);
#endif
	}

	override void OnCEUpdate()
	{
		super.OnCEUpdate();

		if (GetLifetime() <= 0)
			Delete();
	}

	override bool CanObstruct()
	{
		return false;
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		m_Expansion_DebugOrigin = Vector(m_Expansion_DebugOriginX, m_Expansion_DebugOriginY, m_Expansion_DebugOriginZ);

		Expansion_DrawDebugLine(m_Expansion_DebugOrigin);
	}

	void Expansion_SetDebugLineFlags(int flags)
	{
		m_Expansion_DebugLineFlags = flags;

		SetSynchDirty();
	}

	void Expansion_DrawDebugLine(vector origin)
	{
#ifdef SERVER
		m_Expansion_DebugOriginX = origin[0];
		m_Expansion_DebugOriginY = origin[1];
		m_Expansion_DebugOriginZ = origin[2];

		m_Expansion_DrawDebugSignal = !m_Expansion_DrawDebugSignal;

		SetSynchDirty();
#else
		Debug.RemoveShape(m_Expansion_DebugLine);
		m_Expansion_DebugLine = Debug.DrawLine(origin, GetPosition(), m_Expansion_DebugColor, m_Expansion_DebugLineFlags);
#endif
	}
}
