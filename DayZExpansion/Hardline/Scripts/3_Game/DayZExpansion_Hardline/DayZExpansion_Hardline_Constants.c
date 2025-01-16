/**
 * DayZExpansion_Hardline_Constants.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2020 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

/**
 * Expansion hardline system folder paths
 * @{
 */
static const string EXPANSION_HARDLINE_FOLDER = EXPANSION_FOLDER + "Hardline\\";
static const string EXPANSION_HARDLINE_PLAYERDATA_FOLDER = EXPANSION_HARDLINE_FOLDER + "PlayerData\\";

static const string EXPANSION_HARDLINE_SETTINGS = EXPANSION_MISSION_SETTINGS_FOLDER + "HardlineSettings.json";

class ExpansionHardlineItemRarityColor
{
	static const int Poor = ARGB(255, 157, 157, 157);
	static const int Common = ARGB(255, 128, 128, 128);
	static const int Uncommon = ARGB(255, 30, 255, 0);
	static const int Rare = ARGB(255, 0, 112, 221);
	static const int Epic = ARGB(255, 163, 53, 238);
	static const int Legendary = ARGB(255, 255, 128, 0);
	static const int Mythic = ARGB(255, 230, 204, 128);
	static const int Exotic = ARGB(255, 199, 38, 81);
	static const int Quest = ARGB(255, 241, 196, 15);
	static const int Collectable = ARGB(255, 224, 86, 253);
	static const int Ingredient = ARGB(255, 158, 158, 158);
}
