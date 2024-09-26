/**
 * ScriptedViewBase.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

#ifdef EXPANSIONUI
modded class ScriptedViewBase
{
	void SetSort(int sort, bool immedUpdate = true)
	{
		GetLayoutRoot().SetSort(sort, immedUpdate);
	}

	int GetSort()
	{
		return GetLayoutRoot().GetSort();
	}
};
#endif
