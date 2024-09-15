/**
 * ExpansionLocatorArray.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/


/**@class		ExpansionLocatorArray
 * @brief		This class handle expansion locator array
 **/
class ExpansionLocatorArray
{
	vector position;
	string classname;
	string name;
	string type;
	Object object;
	int index;
	
	void ExpansionLocatorArray( vector pos, string className, string displayName, string typeName, Object obj = null, int idx = -1 ) 
	{
		this.position = pos;
		this.classname = className;
		this.name = displayName;
		this.type = typeName;
		this.object = obj;
		this.index = idx;
	}
}