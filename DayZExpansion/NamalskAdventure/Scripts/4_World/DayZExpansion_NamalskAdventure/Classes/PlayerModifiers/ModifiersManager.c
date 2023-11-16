/**
 * ModifiersManager.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

enum eModifiersNamalskAdventure: eModifiers 
{
	NA_MDF_RADIATION = 208,
}

modded class ModifiersManager
{
	override void Init()
	{
		super.Init();

		AddModifier(new Radiation);
	}
};