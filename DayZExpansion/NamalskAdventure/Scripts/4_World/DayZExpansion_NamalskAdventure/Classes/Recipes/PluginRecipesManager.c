/**
 * PluginRecipesManager.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2023 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

modded class PluginRecipesManager
{
	override void RegisterRecipies()
	{
		super.RegisterRecipies();

		RegisterRecipe(new NA_CleanAmmo);
		//RegisterRecipe(new NA_CraftAnomalyBolt);
		RegisterRecipe(new NA_CraftFingerlessWoolGloves);
	}

	void UnregisterRecipeEx(string className)
	{
		UnregisterRecipe(className);
	}
}