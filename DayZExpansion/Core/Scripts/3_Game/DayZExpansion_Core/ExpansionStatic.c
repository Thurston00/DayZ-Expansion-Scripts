/**
 * ExpansionStatic.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License.
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

static bool Expansion_Assert_False( bool check, string message )
{
	if ( check == false )
	{
		Error( message );
		return true;
	}

	return false;
}

static void Expansion_Error(string s, inout bool check = false)
{
	if (!check)
	{
		check = true;
		Error(s);
	}
}

class EXTee
{
	FileHandle m_File;
	autoptr TStringArray m_PrintBuffer = {};
	int m_BufferLength;

	void EXTee(string fileName, FileMode mode)
	{
		m_File = OpenFile(fileName, mode);
	}

	void ~EXTee()
	{
		Flush();
		if (m_File)
			CloseFile(m_File);
	}

	bool CanWrite()
	{
		if (m_File)
			return true;

		return false;
	}

	void WriteLine(string str, bool print = true)
	{
		if (m_File)
			FPrintln(m_File, str);

		if (print)
		{
			int strLen = str.Length();
			//! @note max script log line length 256 characters
			if (m_BufferLength + strLen >= 256)
				Flush();
			m_PrintBuffer.Insert(str);
			m_BufferLength += strLen;
		}
	}

	void Flush()
	{
		Print(ExpansionString.JoinStrings(m_PrintBuffer, "\n"));
		m_PrintBuffer.Clear();
		m_BufferLength = 0;
	}
}

class EXError
{
	static ref CF_Date s_Start = CF_Date.Now();
	static ref map<ErrorExSeverity, string> s_BaseNames = new map<ErrorExSeverity, string>;
	static ref map<string, int> s_Suppressed = new map<string, int>;

	static void Info(Class instance, string msg, TStringArray stack = null)
	{
		Log(instance, msg, stack, ErrorExSeverity.INFO);
	}

	static void Warn(Class instance, string msg, TStringArray stack = null)
	{
		Log(instance, msg, stack, ErrorExSeverity.WARNING);
	}

	static void Error(Class instance, string msg, TStringArray stack = null)
	{
		Log(instance, msg, stack, ErrorExSeverity.ERROR);
	}

	static bool ErrorFalse(Class instance, string msg, TStringArray stack = null)
	{
		EXError.Error(instance, msg, stack);
		return false;
	}

	static void InfoOnce(Class instance, string msg, TStringArray stack = null)
	{
		LogOnce(instance, msg, stack, ErrorExSeverity.INFO);
	}

	static void WarnOnce(Class instance, string msg, TStringArray stack = null)
	{
		LogOnce(instance, msg, stack, ErrorExSeverity.WARNING);
	}

	static void ErrorOnce(Class instance, string msg, TStringArray stack = null)
	{
		LogOnce(instance, msg, stack, ErrorExSeverity.ERROR);
	}

	static void LogOnce(Class instance, string msg, TStringArray stack = null, ErrorExSeverity severity = ErrorExSeverity.ERROR)
	{
		int count;

		if (!s_Suppressed.Find(msg, count))
			Log(instance, msg, stack, severity);
		else
			count++;

		s_Suppressed[msg] = count;
	}

	static void Log(Class instance, string msg, TStringArray stack = null, ErrorExSeverity severity = ErrorExSeverity.ERROR)
	{
		string baseName;

		if (!s_BaseNames)
			s_BaseNames = new map<ErrorExSeverity, string>;

		if (!s_BaseNames.Find(severity, baseName))
		{
			string prefix = typename.EnumToString(ErrorExSeverity, severity);
			prefix.ToLower();

			if (!s_Start)
				s_Start = CF_Date.Now();

			baseName = string.Format("%1_%2.log", prefix, s_Start.GetISODateTime("_", "-"));
			s_BaseNames[severity] = baseName;
		}

		TStringArray filtered;
		if (!stack)
		{
			string tmp;
			DumpStackString(tmp);
			TStringArray lines = {};
			tmp.Split("\n", lines);

			filtered = {};
			bool insert;
			foreach (string line: lines)
			{
				if (insert)
					filtered.Insert(line);
				else if (line.IndexOf("Info()") > -1)
					insert = true;
				else if (line.IndexOf("Warn()") > -1)
					insert = true;
				else if (line.IndexOf("Error()") > -1)
					insert = true;
				else if (line.IndexOf("InfoOnce()") > -1)
					insert = true;
				else if (line.IndexOf("WarnOnce()") > -1)
					insert = true;
				else if (line.IndexOf("ErrorOnce()") > -1)
					insert = true;
			}

			stack = filtered;
		}

		LogToFile("$profile:" + baseName, instance, msg, stack, severity);
	}

	static void LogToFile(string fileName, Class instance, string msg, TStringArray stack = null, ErrorExSeverity severity = ErrorExSeverity.ERROR)
	{
		string now = CF_Date.Now().Format(CF_Date.DATETIME);

		//! Try to mimic vanilla error output

		EXTee tee;

		if (FileExist(fileName))
		{
			tee = new EXTee(fileName, FileMode.APPEND);
		}
		else
		{
			tee = new EXTee(fileName, FileMode.WRITE);

			tee.WriteLine("---------------------------------------------", false);
			tee.WriteLine(string.Format("Log %1 started at %2", s_BaseNames[severity], now), false);
			tee.WriteLine("", false);
		}

		tee.WriteLine("", false);
		tee.WriteLine("", false);
		tee.WriteLine("------------------------------------", false);
		tee.WriteLine(string.Format("%1, %2", GetMachineName(), now), false);
		tee.WriteLine("", false);

		tee.WriteLine(msg);

		if (instance)
		{
			tee.WriteLine(string.Format("Class:      '%1'", instance.ClassName()));
			IEntity entity;
			if (Class.CastTo(entity, instance))
				tee.WriteLine(string.Format("Entity id:%1", entity.GetID()));
			tee.WriteLine("");
		}

		if (stack.Count())
		{
			tee.WriteLine(string.Format("Function: '%1'", stack[0].Substring(0, stack[0].IndexOf("("))));

			tee.WriteLine("Stack trace:");

			bool print = true;
			foreach (string line: stack)
			{
				line.Replace(" : ", ":");
				int index = line.IndexOf(")") + 2;
				tee.WriteLine(line.Substring(index, line.Length() - index), print);
				print = false;
			}
		}
	}
}

// -----------------------------------------------------------
// Expansion EXPrint
// -----------------------------------------------------------
static void EXPrint( string s, string prefix = " [EXPANSION DEBUG]: " )
{
	PrintFormat("%1%2%3", ExpansionStatic.GetISOTime(), prefix, s );
}

static void EXPrint( Class instance, string s, string prefix = " [EXPANSION DEBUG]: " )
{
	PrintFormat("%1%2%3 %4", ExpansionStatic.GetISOTime(), prefix, instance.ToString(), s );
}

// -----------------------------------------------------------
// Expansion EXLogPrint
// -----------------------------------------------------------
static void EXLogPrint( string s )
{
	PrintFormat("%1%2%3", ExpansionStatic.GetISOTime(), " [EXPANSION LOG]: ", s );
}

static void EXLogPrint( Class instance, string s )
{
	PrintFormat("%1%2%3 %4", ExpansionStatic.GetISOTime(), " [EXPANSION LOG]: ", instance.ToString(), s );
}

// -----------------------------------------------------------
// Expansion EXLogPrint
// -----------------------------------------------------------
static void EXLogPrint( float s )
{
	EXLogPrint( s.ToString() );
}

// -----------------------------------------------------------
// Expansion EXLogPrint
// -----------------------------------------------------------
static void EXLogPrint( int s )
{
	EXLogPrint( s.ToString() );
}

//! Prints a hitch warning if elapsed time since startTime is above threshold (DEPRECATED, use EXHitch)
static void EXPrintHitch( string msgPrefix, float startTime, float threshold = 0.0125 )
{
	float elapsedTime = GetGame().GetTickTime() - startTime;
	if (elapsedTime > threshold)
		EXPrint(msgPrefix + "HITCH: " + (elapsedTime * 1000) + "ms");
}

class EXTimeIt
{
	protected int m_Ticks;

	void EXTimeIt()
	{
		m_Ticks = TickCount(0);
	}

	int GetElapsed()
	{
		return TickCount(m_Ticks);
	}
}

class EXHitch: EXTimeIt
{
	protected int m_Threshold;
	protected string m_MsgPrefix;

	void EXHitch(string msgPrefix, int threshold = 125000)
	{
		m_Threshold = threshold;
		m_MsgPrefix = msgPrefix;
	}

	void ~EXHitch()
	{
		int elapsed = TickCount(m_Ticks);
		if (elapsed > m_Threshold)
			PrintFormat("%1 %2HITCH: %3ms", ExpansionStatic.GetISOTime(), m_MsgPrefix, (elapsed / 10000.0).ToString());
	}
}

enum ExpansionVectorToString
{
	Plain,
	Beautify,
	Labels
}

//! Bitmask
enum ExpansionFindFileMode
{
	FILES = 1,
	DIRECTORIES = 2
}

class ExpansionStatic: ExpansionStaticCore
{
	static const string BASE16 = "0123456789ABCDEF";
	static const typename NULLTYPE;

	static ref TStringArray s_VehicleClassNames = {"CarScript", "BoatScript", "ExpansionVehicleBase", "HypeTrain_PartBase"};

	static string BitmaskEnumToString(typename e, int enumValue)
	{
		int cnt = e.GetVariableCount();
		int val;

		string ret;
		for (int i = 0; i < cnt; i++)
		{
			if (e.GetVariableType(i) == int && e.GetVariableValue(null, i, val) && (enumValue & val) == val)
			{
				if (ret.Length() > 0)
					ret += "|";
				ret += e.GetVariableName(i);
			}
		}

		if (ret.Length() > 0)
			return ret;

		return "unknown";
	}

	/**
	 * @brief Set value to enum value if it exists
	 * 
	 * @param e typename/enum
	 * @param enumName name
	 * @param [out] value (if found)
	 * 
	 * @return true if found, false if not
	 * 
	 * @note use this instead of typename.StringToEnum if you need to know if value exists or not
	 */
	static bool StringToEnumEx(typename e, string enumName, out int value)
	{
		int count = e.GetVariableCount();
	   
		for (int i = 0; i < count; i++)
		{
			if (e.GetVariableType(i) == int && e.GetVariableName(i) == enumName && e.GetVariableValue(null, i, value))
				return true;
		}
		
		return false;
	}

	/**
	 * @brief Float to string conversion
	 * 
	 * @param nmb Float number
	 * 
	 * @note unlike float::ToString(), returns non-scientific notation for any number
	 */
	static string FloatToString(float nmb)
	{
		return FormatFloat(nmb, 6, false, false);
	}

	/**
	 * @brief Format float number as string with given precision
	 * 
	 * @param nmb Float number
	 * @param precision Decimal places after decimal separator
	 * @param useLocale Use localized decimal/thousands separator
	 * @param formatInt Format integer part of number (using localized or SI thousands separator)
	 * @param enableIncreasedAccuracy Gain 1-3 additional digits of precision before and after the decimal point,
	 *        but at roughly 50% performance cost (avg)
	 *
	 * @note unlike float::ToString(), returns non-scientific notation for any number
	*/
	static string FormatFloat(float number, int precision = 6, bool useLocale = true, bool formatInt = true, bool enableIncreasedAccuracy = false)
	{
		int i;

		if (number > (float) int.MIN && number < (float) int.MAX)
			i = number;

		float f = i;

		if (number == f)
		{
			if (formatInt)
				return FormatInt(i, false, useLocale);
			else
				return i.ToString();
		}

		/*
		//! XXX: It's easier and should be sufficient to just not do rounding at all and always truncate
		//! Leaving this here just in case I change my mind
		if (precision > 0)
		{
			float m = Math.Pow(10, precision);
			if (Math.AbsFloat(number) <= float.MAX / m)
				number = Math.Round(number * m) / m;
		}
		*/

		float abs = Math.AbsFloat(number);
		//float floor = Math.Floor(abs);  //! It's broken, e.g. float.MAX gets turned into 1.70141e+38, half of what it should be
		float floor = -Math.Ceil(-abs);  //! Gives correct floor
		float frac = abs - floor;

		string tmp = abs.ToString();
		string real;
		string zeros;

		bool isNegativeExp;
		bool useComponents;

		int count;
		int digit;

		string str;

		int expIdx = tmp.IndexOf("e");

		if (expIdx > -1)
		{
			int exp = tmp.Substring(expIdx + 1, tmp.Length() - expIdx - 1).ToInt();

			if (exp < 0)
				isNegativeExp = true;
			else if (Math.AbsFloat(f) != floor)
				useComponents = true;

			if (isNegativeExp || useComponents)
			{
				count = Math.AbsInt(exp);

				if (enableIncreasedAccuracy)
				{
					if (useComponents)
					{
						while (floor >= 1.0)
						{
							float fdigit = ExpansionMath.FMod(floor, 10.0);
							digit = fdigit;
							if (digit < 0)
								digit = 0;
							//PrintFormat("DIGIT %1 %2", fdigit, digit);
							real = digit.ToString() + real;
							floor = (floor - digit) / 10.0;
						}

						if (real.Length() > ++count)
							real = real.Substring(0, count);
					}
					else
					{
						count += tmp.Substring(0, expIdx).Length() - 1;

						if (precision > count)
							precision = count;
					}
				}
				else
				{
					int decimalLen;

					if (expIdx >= 3)
					{
						//! coefficient has decimal point, e.g. 1.2e-05
						decimalLen = expIdx - 2;
						real = tmp.Substring(0, 1) + tmp.Substring(2, decimalLen);
					}
					else
					{
						//! coefficient has no decimal point, e.g. 1e-05
						decimalLen = 1;
						real = tmp.Substring(0, expIdx);
					}

					if (isNegativeExp)
						count--;
					else
						count -= decimalLen;

					while (count--)
					{
						zeros += "0";
					}
				}

				if (number < 0)
					str = "-";
			}
		}

		if (isNegativeExp)
		{
			str += "0";
		}
		else if (useComponents)
		{
			if (formatInt)
				str += FormatIntStr(real + zeros, useLocale);
			else
				str += real + zeros;
		}
		else
		{
			if (number < 0 && i >= 0)
				str = "-";

			if (formatInt)
				str += FormatInt(i, false, useLocale);
			else
				str += i.ToString();
		}

		if (frac)
		{
			if (useLocale)
				str += Widget.TranslateString("#STR_EXPANSION_NUMBER_SEPARATOR_DECIMAL");
			else
				str += ".";

			if (enableIncreasedAccuracy)
			{
				if (!isNegativeExp)
				{
					count = frac.ToString().Length() - 1;

					if (precision > count)
						precision = count;
				}

				while (precision--)
				{
					frac *= 10;
					digit = frac;
					str += digit.ToString();
					frac -= digit;
				}
			}
			else if (isNegativeExp)
			{
				tmp = zeros + real;
				str += tmp.Substring(0, Math.Min(tmp.Length(), precision));
			}
			else
			{
				tmp = frac.ToString();
				str += tmp.Substring(2, Math.Min(tmp.Length() - 2, precision));
			}
		}

		return str;
	}

	static int GetPrecision(int n)
	{
		n = Math.AbsInt(n);

		if (n)
			return FloatToString(1.0 / n).Length() - 2;

		return 0;
	}

	static string FloatFixedDecimals(float f, int decimals = 4)
	{
		string str = f.ToString();
		int i = str.IndexOf(".");

		if (i > -1)
		{
			while (str.Length() - i < decimals + 1)
			{
				str += "0";
			}

			int len = str.Length() - i - 1;
			if (len > decimals)
				str = str.Substring(0, i + decimals + 1);
		}

		return str;
	}

	// -----------------------------------------------------------
	// Expansion Float FloatNewPrecision
	// -----------------------------------------------------------
	static float FloatNewPrecision(float n, float i)
	{
		float m = Math.Pow(10, i);
		return Math.Floor(m * n) / m;
	}

	// -----------------------------------------------------------
	// Expansion TStringArray GetWorkingZombieClasses
	// -----------------------------------------------------------
	static TStringArray GetWorkingZombieClasses()
	{
		return { "ZmbM_HermitSkinny_Beige","ZmbM_HermitSkinny_Black","ZmbM_HermitSkinny_Green",
				 "ZmbM_HermitSkinny_Red","ZmbM_FarmerFat_Beige","ZmbM_FarmerFat_Blue","ZmbM_FarmerFat_Brown",
				 "ZmbM_FarmerFat_Green","ZmbF_CitizenANormal_Beige","ZmbF_CitizenANormal_Brown",
				 "ZmbF_CitizenANormal_Blue","ZmbM_CitizenASkinny_Blue","ZmbM_CitizenASkinny_Brown",
				 "ZmbM_CitizenASkinny_Grey","ZmbM_CitizenASkinny_Red","ZmbM_CitizenBFat_Blue","ZmbM_CitizenBFat_Red",
				 "ZmbM_CitizenBFat_Green","ZmbF_CitizenBSkinny","ZmbM_PrisonerSkinny",
				 "ZmbM_FirefighterNormal","ZmbM_FishermanOld_Blue","ZmbM_FishermanOld_Green",
				 "ZmbM_FishermanOld_Grey","ZmbM_FishermanOld_Red","ZmbM_JournalistSkinny",
				 "ZmbF_JournalistNormal_Blue","ZmbF_JournalistNormal_Green","ZmbF_JournalistNormal_Red","ZmbF_JournalistNormal_White",
				 "ZmbM_ParamedicNormal_Blue","ZmbM_ParamedicNormal_Green","ZmbM_ParamedicNormal_Red",
				 "ZmbM_ParamedicNormal_Black","ZmbF_ParamedicNormal_Blue","ZmbF_ParamedicNormal_Green",
				 "ZmbF_ParamedicNormal_Red","ZmbM_HikerSkinny_Blue","ZmbM_HikerSkinny_Green","ZmbM_HikerSkinny_Yellow",
				 "ZmbF_HikerSkinny_Blue","ZmbF_HikerSkinny_Grey","ZmbF_HikerSkinny_Green","ZmbF_HikerSkinny_Red",
				 "ZmbM_HunterOld_Autumn","ZmbM_HunterOld_Spring","ZmbM_HunterOld_Summer","ZmbM_HunterOld_Winter",
				 "ZmbF_SurvivorNormal_Blue","ZmbF_SurvivorNormal_Orange","ZmbF_SurvivorNormal_Red",
				 "ZmbF_SurvivorNormal_White",
				 "ZmbM_PolicemanFat","ZmbF_PoliceWomanNormal",
				 "ZmbM_PolicemanSpecForce","ZmbM_SoldierNormal",
				 "ZmbM_usSoldier_normal_Woodland","ZmbM_usSoldier_normal_Desert","ZmbM_CommercialPilotOld_Blue",
				 "ZmbM_CommercialPilotOld_Olive","ZmbM_CommercialPilotOld_Brown","ZmbM_CommercialPilotOld_Grey",
				 "ZmbM_PatrolNormal_PautRev","ZmbM_PatrolNormal_Autumn","ZmbM_PatrolNormal_Flat","ZmbM_PatrolNormal_Summer",
				 "ZmbM_JoggerSkinny_Blue","ZmbM_JoggerSkinny_Green","ZmbM_JoggerSkinny_Red","ZmbF_JoggerSkinny_Blue",
				 "ZmbF_JoggerSkinny_Brown","ZmbF_JoggerSkinny_Green","ZmbF_JoggerSkinny_Red","ZmbM_MotobikerFat_Beige",
				 "ZmbM_MotobikerFat_Black","ZmbM_MotobikerFat_Blue","ZmbM_VillagerOld_Blue","ZmbM_VillagerOld_Green",
				 "ZmbM_VillagerOld_White","ZmbM_SkaterYoung_Blue","ZmbM_SkaterYoung_Brown","ZmbM_SkaterYoung_Green",
				 "ZmbM_SkaterYoung_Grey","ZmbF_SkaterYoung_Brown","ZmbF_SkaterYoung_Striped","ZmbF_SkaterYoung_Violet",
				 "ZmbF_DoctorSkinny","ZmbF_BlueCollarFat_Blue","ZmbF_BlueCollarFat_Green",
				 "ZmbF_BlueCollarFat_Red","ZmbF_BlueCollarFat_White","ZmbF_MechanicNormal_Beige","ZmbF_MechanicNormal_Green",
				 "ZmbF_MechanicNormal_Grey","ZmbF_MechanicNormal_Orange","ZmbM_MechanicSkinny_Blue","ZmbM_MechanicSkinny_Grey",
				 "ZmbM_MechanicSkinny_Green","ZmbM_MechanicSkinny_Red","ZmbM_ConstrWorkerNormal_Beige",
				 "ZmbM_ConstrWorkerNormal_Black","ZmbM_ConstrWorkerNormal_Green","ZmbM_ConstrWorkerNormal_Grey",
				 "ZmbM_HeavyIndustryWorker","ZmbM_OffshoreWorker_Green","ZmbM_OffshoreWorker_Orange","ZmbM_OffshoreWorker_Red",
				 "ZmbM_OffshoreWorker_Yellow","ZmbF_NurseFat","ZmbM_HandymanNormal_Beige",
				 "ZmbM_HandymanNormal_Blue","ZmbM_HandymanNormal_Green","ZmbM_HandymanNormal_Grey","ZmbM_HandymanNormal_White",
				 "ZmbM_DoctorFat","ZmbM_Jacket_beige","ZmbM_Jacket_black","ZmbM_Jacket_blue","ZmbM_Jacket_bluechecks",
				 "ZmbM_Jacket_brown","ZmbM_Jacket_greenchecks","ZmbM_Jacket_grey","ZmbM_Jacket_khaki","ZmbM_Jacket_magenta","ZmbM_Jacket_stripes",
				 "ZmbF_PatientOld","ZmbM_PatientSkinny","ZmbF_ShortSkirt_beige",
				 "ZmbF_ShortSkirt_black","ZmbF_ShortSkirt_brown","ZmbF_ShortSkirt_green","ZmbF_ShortSkirt_grey","ZmbF_ShortSkirt_checks",
				 "ZmbF_ShortSkirt_red","ZmbF_ShortSkirt_stripes","ZmbF_ShortSkirt_white","ZmbF_ShortSkirt_yellow",
				 "ZmbF_VillagerOld_Blue","ZmbF_VillagerOld_Green","ZmbF_VillagerOld_Red","ZmbF_VillagerOld_White",
				 "ZmbF_MilkMaidOld_Beige","ZmbF_MilkMaidOld_Black","ZmbF_MilkMaidOld_Green","ZmbF_MilkMaidOld_Grey",
				 "ZmbM_priestPopSkinny","ZmbM_ClerkFat_Brown","ZmbM_ClerkFat_Grey","ZmbM_ClerkFat_Khaki","ZmbM_ClerkFat_White",
				 "ZmbF_Clerk_Normal_Blue","ZmbF_Clerk_Normal_White","ZmbF_Clerk_Normal_Green","ZmbF_Clerk_Normal_Red",
				 "ZmbM_NBC_Grey", "ZmbM_NBC_Yellow", "ZmbM_PolicemanSpecForce_Heavy", "ZmbM_Santa", "ZmbM_usSoldier_Heavy_Woodland",
				 "ZmbM_usSoldier_Officer_Desert" };
	}

	/**
	 * @brief get the hex part of an instance's representational string,
	 * e.g. if instance repr string is "SomeObject<4461795a>", returns "4461795a"
	 */
	static string GetInstanceID(Class instance)
	{
		string repr = instance.ToString();
		int start = repr.IndexOf("<") + 1;
		int len = repr.Length() - start - 1;
		return repr.Substring(start, len);
	}

	/**
	 * @brief Get instance dbg info as string (null-safe if instance is managed)
	 * 
	 * @param instance
	 * @param checkParent  When determining if inventory locked, don't check parent if set to false
	 * 
	 * @return instance dbg name + instance id, followed by indicators for pending deletion and inventory locked (if applicable)
	 * 
	 * @code
	 * 	string info = ExpansionStatic.GetEntityDebugInfo(GetGame().GetPlayer());
	 * 	Print(info);
	 * 	
	 * 	>> info = 'PlayerBase:1:INSTANCETYPE_CLIENT<59d0b5f5>'
	 * @endcode
	 */
	static string GetDebugInfo(Class instance, bool checkParent = true)
	{
		if (!instance)
			return "null";

		string dbgInfo = string.Format("%1<%2>", instance.GetDebugName(), GetInstanceID(instance));

		Object obj;
		if (Class.CastTo(obj, instance))
		{
			if (obj.IsDamageDestroyed())
				dbgInfo += "[DEAD]";

			//! Is object marked to be deleted soon?
			if (obj.ToDelete())
				dbgInfo += "[TO_DELETE]";

			//! Is parent being deleted?
			if (obj.IsPendingDeletion())
				dbgInfo += "[PENDING_DELETION]";
		}

		EntityAI entity;
		if (Class.CastTo(entity, instance))
		{
			//! Has EntityAI::SetPrepareToDelete been called?
			if (entity.IsPreparedToDelete())
				dbgInfo += "[PREPARED_TO_DELETE]";

			//! Has EntityAI::EEDelete been called?
			if (entity.m_PendingDelete)
				dbgInfo += "[EE_DELETE]";

			int lockType;
			EntityAI lockedEntity;
			if (IsInventoryLocked(entity, lockType, lockedEntity, checkParent))
			{
				string lockTypeInfo;

				switch (lockType)
				{
					case LOCK_FROM_SCRIPT:
						lockTypeInfo = "LOCK";
						break;

					case HIDE_INV_FROM_SCRIPT:
						lockTypeInfo = "HIDE_INV";
						break;
				}

				if (lockedEntity != entity)
					dbgInfo += string.Format("[%1=%2]", lockTypeInfo, lockedEntity);
				else
					dbgInfo += string.Format("[%1]", lockTypeInfo);
			}
		}

		return dbgInfo;
	}

	static string GetHierarchyInfo(Class instance)
	{
		string hierarchyInfo = GetDebugInfo(instance, false);

		EntityAI entity;
		if (!Class.CastTo(entity, instance))
			return hierarchyInfo;

		EntityAI parent = entity.GetHierarchyParent();
		auto il = new InventoryLocation;

		while (parent)
		{
			if (parent.IsMan())
				hierarchyInfo += " on ";
			else
				hierarchyInfo += " in ";

			if (entity.GetInventory().GetCurrentInventoryLocation(il))
			{
				if (il.GetType() == InventoryLocationType.ATTACHMENT)
					hierarchyInfo += string.Format("slot %1 of ", InventorySlots.GetSlotName(il.GetSlot()));
			}

			hierarchyInfo += GetDebugInfo(parent, false);

			entity = parent;
			parent = parent.GetHierarchyParent();
		}

		return hierarchyInfo;
	}

	//! Better version of InventoryLocation.DumpToString (includes item/parent dbg info, slot name instead of ID, and whether loc is valid)
	static string DumpToString(InventoryLocation loc)
	{
		if (!loc)
			return "{ null }";

		string res = "{ type=" + typename.EnumToString(InventoryLocationType, loc.GetType());

		if (loc.GetType() != InventoryLocationType.UNKNOWN)
		{
			res += " item=" + GetDebugInfo(loc.GetItem());

			if (loc.GetType() != InventoryLocationType.GROUND)
				res += " parent=" + GetDebugInfo(loc.GetParent());
		}

		switch (loc.GetType())
		{
			case InventoryLocationType.UNKNOWN:
				break;
			case InventoryLocationType.GROUND:
				vector pos = loc.GetPos();
				float dir[4];
				loc.GetDir( dir );
				res += " pos=(" + pos[0] + ", " + pos[1] + ", " + pos[2] + ")";
				res += " dir=(" + dir[0] + ", " + dir[1] + ", " + dir[2] + ", " + dir[3] + ")";
				break;
			case InventoryLocationType.ATTACHMENT:
				res += " slot=" + InventorySlots.GetSlotName(loc.GetSlot());
				break;
			case InventoryLocationType.CARGO:
			case InventoryLocationType.PROXYCARGO:
				res += " idx=" + loc.GetIdx() + " row=" + loc.GetRow() + " col=" + loc.GetCol() + " f=" + loc.GetFlip();
				break;
			case InventoryLocationType.HANDS:
				break;
		#ifndef DAYZ_1_25
			case InventoryLocationType.VEHICLE:
				res += " idx=" + loc.GetIdx();
				break;
		#endif
			default:
				res += "??";
				break;
		}

		res += " valid=" + loc.IsValid().ToString();
		res += " }";

		return res;
	}

	string GetDisplayNameRaw(Object obj)
	{
		string path;
		string displayName;

		if (obj.IsWeapon())
			path = CFG_WEAPONSPATH;
		else if (obj.IsMagazine())
			path = CFG_MAGAZINESPATH;
		else
			path = CFG_VEHICLESPATH;

		if (GetGame().ConfigGetTextRaw(path + " " + obj.GetType() + " displayName", displayName))
			GetGame().FormatRawConfigStringKeys(displayName);

		return displayName;
	}

	// -----------------------------------------------------------
	// Expansion String GetItemDisplayNameWithType
	// -----------------------------------------------------------
	//! @note this is for the rare cases where we only have a classname, not an object.
	//! If we have an object, should use <object>.GetDisplayName() instead.
	static string GetItemDisplayNameWithType( string type_name, map<string, string> cache = NULL )
	{
		string cfg_name;
		string cfg_name_path;

		if ( cache && cache.Find( type_name, cfg_name ) )
		{
			return cfg_name;
		}
		else if ( GetGame().ConfigIsExisting( CFG_WEAPONSPATH + " " + type_name ) )
		{
			cfg_name_path = CFG_WEAPONSPATH + " " + type_name + " displayName";
			GetGame().ConfigGetText( cfg_name_path, cfg_name );
		}
		else if ( GetGame().ConfigIsExisting( CFG_VEHICLESPATH + " " + type_name ) )
		{
			cfg_name_path = CFG_VEHICLESPATH + " " + type_name + " displayName";
			GetGame().ConfigGetText( cfg_name_path, cfg_name );
		}
		else if ( GetGame().ConfigIsExisting( CFG_MAGAZINESPATH + " " + type_name ) )
		{
			cfg_name_path = CFG_MAGAZINESPATH + " " + type_name + " displayName";
			GetGame().ConfigGetText( cfg_name_path, cfg_name );
		}
		if (!cfg_name)
		{
			cfg_name = type_name;
		}

		if (cfg_name.IndexOf("$UNT$") == 0)
			cfg_name.Replace("$UNT$", "");

		if ( cache )
		{
			cache.Insert( type_name, cfg_name );
		}

		return cfg_name;
	}

	// -----------------------------------------------------------
	// Expansion String GetItemDescriptionWithType
	// -----------------------------------------------------------
	//! @note this is for the rare cases where we only have a classname, not an object.
	//! If we have an object, should use <object>.ConfigGetString('descriptionShort') instead.
	static string GetItemDescriptionWithType( string type_name, map<string, string> cache = NULL )
	{
		string cfg_des;
		string cfg_des_path;

		if ( cache && cache.Find( type_name, cfg_des ) )
		{
			return cfg_des;
		}
		else if ( GetGame().ConfigIsExisting( CFG_WEAPONSPATH + " " + type_name ) )
		{
			cfg_des_path = CFG_WEAPONSPATH + " " + type_name + " descriptionShort";
			GetGame().ConfigGetText( cfg_des_path, cfg_des );
		}
		else if ( GetGame().ConfigIsExisting( CFG_VEHICLESPATH + " " + type_name ) )
		{
			cfg_des_path = CFG_VEHICLESPATH + " " + type_name + " descriptionShort";
			GetGame().ConfigGetText( cfg_des_path, cfg_des );
		}
		else if ( GetGame().ConfigIsExisting( CFG_MAGAZINESPATH + " " + type_name ) )
		{
			cfg_des_path = CFG_MAGAZINESPATH + " " + type_name + " descriptionShort";
			GetGame().ConfigGetText( cfg_des_path, cfg_des );
		}

		if ( cache )
		{
			cache.Insert( type_name, cfg_des );
		}

		return cfg_des;
	}

	//! Will return sensible result on server for all objects, including ItemBase
	//! (vanilla ItemBase::CanObstruct will always return true on server as it is only meant to be called on client because it relies on g_Game.GetPlayer)
	static bool CanObstruct(Object obj)
	{
		if (obj.IsItemBase())
		{
			if (obj.ConfigGetString("physLayer") == "item_large")
				return true;

			return false;
		}

		return obj.CanObstruct();
	}

	static float GetBoundingRadius(string className)
	{
		vector minMax[2];
		return ClippingInfo(className, minMax);
	}

	static float ClippingInfo(string className, out vector minMax[2])
	{
		float radius;

		Object obj = GetGame().CreateObjectEx(className, "0 0 0", ECE_LOCAL);

		if (obj)
		{
			radius = obj.ClippingInfo(minMax);
			GetGame().ObjectDelete(obj);
		}

		return radius;
	}

	//! If collision box does not exist, minMax is set to clipping info (bounding box)
	static bool GetCollisionBox(string className, out vector minMax[2])
	{
		bool hasCollisionBox;

		Object obj = GetGame().CreateObjectEx(className, "0 0 0", ECE_LOCAL);

		if (obj)
		{
			hasCollisionBox = GetCollisionBox(obj, minMax);
			GetGame().ObjectDelete(obj);
		}

		return hasCollisionBox;
	}

	static bool GetCollisionBox(Object obj, out vector minMax[2])
	{
		bool hasCollisionBox = obj.GetCollisionBox(minMax);
		if (!hasCollisionBox)
			obj.ClippingInfo(minMax);
		return hasCollisionBox;
	}

	static bool IsColliding(Object obj, vector point)
	{
		vector minMax[2];
		if (obj.GetCollisionBox(minMax))
		{
			vector localPoint = obj.WorldToModel(point);

			for (int i = 0; i < 3; i++)
			{
				if (localPoint[i] < minMax[0][i])
					return false;

				if (localPoint[i] > minMax[1][i])
					return false;
			}

			return true;
		}

		return false;
	}

	static bool HasQuantity(string item_name)
	{
		float min;
		float max;

		if (GetGame().IsKindOf(item_name, "Magazine_Base"))
		{
			max = GetGame().ConfigGetInt("CfgMagazines " + item_name + " count");
		}
		else
		{
			min = GetGame().ConfigGetInt("CfgVehicles " + item_name + " varQuantityMin");
			max = GetGame().ConfigGetFloat("CfgVehicles " + item_name + " varStackMax");
			if (!max)
				max = GetGame().ConfigGetInt("CfgVehicles " + item_name + " varQuantityMax");
		}

		return max - min > 0;
	}

	static bool ItemExists(string type_name)
	{
		return GetGame().ConfigIsExisting( CFG_VEHICLESPATH + " " + type_name ) || GetGame().ConfigIsExisting( CFG_WEAPONSPATH + " " + type_name ) || GetGame().ConfigIsExisting( CFG_MAGAZINESPATH + " " + type_name );
	}

	static bool TypeExists(string type_name)
	{
		typename nullType;
		return type_name.ToType() != nullType;
	}

	//! Inheritance check based on rvConfig class name or EnforceScript typename
	static bool IsAnyOf(string className, TStringArray parentNames)
	{
		foreach (string parentName: parentNames)
		{
			if (Is(className, parentName))
				return true;
		}
		return false;
	}

	static bool IsAnyOf(string className, TTypenameArray parent_types)
	{
		foreach (typename parent_type: parent_types)
		{
			if (Is(className, parent_type))
				return true;
		}
		return false;
	}

	//! Inheritance check based on rvConfig class name or EnforceScript typename
	//! @note `check_script_inheritance` is a legacy parameter and no longer used (script inheritance is always checked).
	static bool IsAnyOf(Object obj, TStringArray parentNames, bool check_script_inheritance = false)
	{
		foreach (string parentName: parentNames)
		{
			typename parentType = parentName.ToType();
			if (parentType && obj.IsInherited(parentType))
				return true;
			if (g_Game.ObjectIsKindOf(obj, parentName))
				return true;
		}
		return false;
	}

	//! Inheritance check based on typename
	static bool IsAnyOf(typename type, TTypenameArray parent_types)
	{
		foreach (typename parent_type: parent_types)
		{
			if (type.IsInherited(parent_type))
				return true;
		}
		return false;
	}

	//! Inheritance check based on instance
	static bool IsAnyOf(Class instance, TTypenameArray parent_types)
	{
		foreach (typename parent_type: parent_types)
		{
			if (instance.IsInherited(parent_type))
				return true;
		}
		return false;
	}

	//! Inheritance check based on rvConfig class name or EnforceScript typename
	static bool Is(string className, string parentName)
	{
		typename classType = className.ToType();
		if (classType)
		{
			typename parentType = parentName.ToType();

			if (parentType && classType.IsInherited(parentType))
				return true;
		}

		return g_Game.IsKindOf(className, parentName);
	}

	//! Inheritance check based on rvConfig class name or EnforceScript typename
	static bool Is(Object obj, string parentName)
	{
		typename parentType = parentName.ToType();

		if (parentType && obj.IsInherited(parentType))
			return true;

		return g_Game.ObjectIsKindOf(obj, parentName);
	}

	//! Inheritance check based on rvConfig class name or EnforceScript typename
	static bool Is(string className, typename parentType)
	{
		typename classType = className.ToType();
		if (classType && classType.IsInherited(parentType))
			return true;

		return g_Game.IsKindOf(className, parentType.ToString());
	}

	static bool IsVehicle(string className)
	{
		if (ExpansionStatic.IsAnyOf(className, s_VehicleClassNames))
			return true;

		return false;
	}

	//! Special case: Any inventory item (rvConfig Inventory_Base, CfgWeapons, CfgMagazines or EnforceScript ItemBase type)
	static bool IsInventoryItem(string className)
	{
		typename classType = className.ToType();
		if (classType)
		{
			string itemClass = "ItemBase";
			typename itemType = itemClass.ToType();

			if (classType.IsInherited(itemType))
				return true;
		}

		if (g_Game.IsKindOf(className, "Inventory_Base"))
			return true;

		if (g_Game.ConfigIsExisting("CfgMagazines " + className))
			return true;

		if (g_Game.ConfigIsExisting("CfgWeapons " + className))
			return true;

		return false;
	}

	//! Special case: Any player (rvConfig SurvivorBase or EnforceScript PlayerBase type)
	static bool IsPlayer(string className)
	{
		typename classType = className.ToType();
		if (classType)
		{
			string playerClass = "PlayerBase";
			typename playerType = playerClass.ToType();

			if (classType.IsInherited(playerType))
				return true;
		}

		return g_Game.IsKindOf(className, "SurvivorBase");
	}

	//! TODO: Maybe use CF_Byte after next CF update?
	static TIntArray IntToByteArray(int n)
	{
		TIntArray bytes();

		for (int i = 0; i < 4; i++)
			bytes.Insert((n >> (24 - i * 8)) & 255);

		return bytes;
	}

	//! TODO: Maybe remove and use CF_Encoding::ToHex instead after next CF update?
	static string ByteArrayToHex(TIntArray bytes)
	{
		string hex;

		foreach (int byte: bytes)
			hex += ByteToHex(byte);

		return hex;
	}

	static int ByteArrayToInt(TIntArray bytes)
	{
		int n;

		for (int i = 0; i < 4; i++)
			n |= bytes[i] << (24 - i * 8);

		return n;
	}

	//! TODO: Maybe remove and use CF_Byte::ToHex instead after next CF update?
	static string ByteToHex(int byte)
	{
		return BASE16[(int) Math.Floor(byte / 16)] + BASE16[byte % 16];
	}

	static string IntToHex(int n)
	{
		return ByteArrayToHex(IntToByteArray(n));
	}

	static string IntToHex(TIntArray ints)
	{
		string hex;
		foreach (int n: ints)
		{
			hex += IntToHex(n);
		}
		return hex;
	}

	static int HexToInt(string hex)
	{
		hex.ToUpper();

		int n;

		for (int i = 0; i < hex.Length(); i++)
		{
			//! Snatched from Dabs Framework
			n += BASE16.IndexOf(hex[i]) * Math.Pow(16, Math.AbsInt(i - hex.Length() + 1));
		}

		return n;
	}

	//! TODO: These should be moved to ExpansionColor
	// ------------------------------------------------------------
	// Expansion RGBtoInt
	// ------------------------------------------------------------
	static int RGBtoInt(int r, int g, int b)
	{
		return ( r << 0 ) | ( g << 8 ) | ( b << 16 );
	}


	// ------------------------------------------------------------
	// Expansion ARGBtoInt
	// ------------------------------------------------------------
	static int ARGBtoInt(int a, int r, int g, int b)
	{
		return ( a << 24 ) | ( r << 16 ) | ( g << 8 ) | b;
	}

	// ------------------------------------------------------------
	// Expansion IntToRGB
	// ------------------------------------------------------------
	static void IntToRGB(int value, out int red, out int green, out int blue)
	{
		red =   ( value >>  0 ) & 255;
		green = ( value >>  8 ) & 255;
		blue =  ( value >> 16 ) & 255;
	}

	// ------------------------------------------------------------
	// Expansion IntToARGB
	// ------------------------------------------------------------
	static void IntToARGB(int value, out int alpha, out int red, out int green, out int blue)
	{
		alpha = ( value >> 24 ) & 255;
		red =   ( value >>  16 ) & 255;
		green = ( value >>  8 ) & 255;
		blue =  ( value ) & 255;
	}

	static bool StringArrayContainsIgnoreCase(TStringArray elements, string search)
	{
		search.ToLower();

		foreach (string element: elements)
		{
			element.ToLower();
			if (element == search)
				return true;
		}

		return false;
	}

	static string GetISODate(bool useUTC = false)
	{
		int year;
		int month;
		int day;

		if (useUTC)
			GetYearMonthDayUTC(year, month, day);
		else
			GetYearMonthDay(year, month, day);

		return year.ToStringLen(4) + "-" + month.ToStringLen(2) + "-" + day.ToStringLen(2);
	}

	static string GetISODateTime(bool useUTC = false, string delim = "T", string delimHMS = ":", bool include_ms = false)
	{
		return GetISODate(useUTC) + delim + GetISOTime(useUTC, include_ms, delimHMS);
	}

	/*!
	 * @brief format time (value in seconds)
	 * @return one of the following formats as string (depending on arguments):
	 * "HH:MM:SS.SSS" (default)
	 * "HH:MM:SS"     (include_ms = false)
	 * "MM:SS.SSS"    (include_hours = false)
	 * "MM:SS"        (include_ms = false, include_hours = false)
	 * @note HH is the total number of hours and can exceed 24 unless `discard_days` is set to true!
	*/
	static string FormatTime(float time, bool include_ms = true, bool include_hours = true, bool discard_days = false, string delimHMS = ":")
	{
		if (include_hours)
		{
			int hours = (int) time / 3600;
			time -= hours * 3600;

			if (discard_days && hours >= 24)
				hours = hours % 24;
		}

		int minutes = (int) time / 60;
		time -= minutes * 60;
		int seconds = (int) time;

		string timestring;

		if (include_hours)
			timestring = hours.ToStringLen(2) + delimHMS + minutes.ToStringLen(2) + delimHMS + seconds.ToStringLen(2);
		else
			timestring = minutes.ToStringLen(2) + delimHMS + seconds.ToStringLen(2);

		if ( include_ms )
		{
			time -= seconds;
			int ms = time * 1000;
			timestring += "." + ms.ToStringLen(3);
		}

		return timestring;
	}

	static string GetISOTime(bool useUTC = false, bool include_ms = true, string delimHMS = ":")
	{
		if ( include_ms && GetDayZGame() )
		{
			//! Accurate, including milliseconds
			return FormatTime(GetDayZGame().ExpansionGetStartTime(useUTC) + GetDayZGame().GetTickTime(), true, true, true, delimHMS);
		}
		else
		{
			//! Next best thing
			int hour;
			int minute;
			int second;

			if (useUTC)
				GetHourMinuteSecondUTC(hour, minute, second);
			else
				GetHourMinuteSecond(hour, minute, second);

			return hour.ToStringLen(2) + delimHMS + minute.ToStringLen(2) + delimHMS + second.ToStringLen(2);
		}
	}

	override string CF_GetISOTime()
	{
		return GetISOTime();
	}

	//! Fast timestamp, use this instead of CF_Date.Now().GetTimestamp() when performance counts
	static int GetTimestamp(bool useUTC = false)
	{
		return GetDayZGame().ExpansionGetStartTimestamp(useUTC) + (int) GetDayZGame().GetTickTime();
	}

	//! Fast accurate time in seconds, not including years/months/days
	static float GetTime(bool useUTC = false)
	{
		return GetDayZGame().ExpansionGetStartTime(useUTC) + GetDayZGame().GetTickTime();
	}

	static string GetTimeString( float total_time, bool include_seconds = false )
	{
		string time_string;

		if ( total_time <= 0 )
		{
			time_string =  "0 #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_SECONDS";
			return time_string;
		}

		int time_seconds = total_time;	//! Convert total time to int

		int days = time_seconds / 86400;
		if ( days > 0 )
		{
			time_string += GetValueString( days ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_DAYS";	//! Days
		}

		int hours = ( time_seconds % 86400 ) / 3600;
		if ( hours > 0 )
		{
			if (time_string)
				time_string += " ";	//! Separator
			time_string += GetValueString( hours ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_HOURS";	//! Hours
		}

		int minutes = ( time_seconds % 3600 ) / 60;
		if ( minutes > 0 )
		{
			if (time_string)
				time_string += " ";	//! Separator
			time_string += GetValueString( minutes ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_MINUTES";	//! Minutes
		}

		if ( include_seconds )
		{
			int seconds = time_seconds - days * 86400 - hours * 3600 - minutes * 60;
			if ( seconds > 0 )
			{
				if (time_string)
					time_string += " ";	//! Separator
				time_string += GetValueString( seconds ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_SECONDS";
			}
		}

		return time_string;
	}

	static string GetWeightString(int weight, bool grams_only = false)
	{
		string weight_string;

		if( weight < 0 )
		{
			weight_string =  "0 #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_GRAMS";
			return weight_string;
		}

		int weight_grams = weight;

		int kilograms = weight_grams / 1000;
		if ( kilograms > 0 && !grams_only )
		{
			weight_string = GetValueString( kilograms ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_KG";				//kilograms
		}
		else
		{
			weight_string = GetValueString( weight_grams ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_GRAMS";	//grams
		}

		return weight_string;
	}

	static string GetDistanceString( float total_distance, bool meters_only = false )
	{
		string distance_string;

		if( total_distance < 0 )
		{
			distance_string =  "0 #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_METERS";
			return distance_string;
		}

		int distance_meters = total_distance;

		int kilometers = distance_meters / 1000;
		if ( kilometers > 0 && !meters_only )
		{
			distance_string = GetValueString( kilometers ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_KM";				//kilometers
		}
		else
		{
			distance_string = GetValueString( distance_meters ) + " #STR_EXPANSION_BOOK_STATUS_CHARACTER_STATS_METERS";	//meters
		}

		return distance_string;
	}

	static string GetValueString( float total_value )
	{
		if( total_value < 0 )
			return "0";

		int value = total_value;
		string out_string;

		if ( value >= 1000 )
		{
			string value_string = value.ToString();

			int count;
			int first_length = value_string.Length() % 3;		//calculate position of the first separator
			if ( first_length > 0 )
			{
				count = 3 - first_length;
			}

			for ( int i = 0; i < value_string.Length(); ++i )
			{
				out_string += value_string.Get( i );
				count ++;

				if ( count >= 3 )
				{
					out_string += " ";			//separator
					count = 0;
				}
			}
		}
		else
		{
			out_string = value.ToString();
		}

		return out_string.Trim();
	}

	static string GetPersistentIDString(EntityAI entity)
	{
		int b1, b2, b3, b4;
		entity.GetPersistentID(b1, b2, b3, b4);
		return GetPersistentIDString(b1, b2, b3, b4);
	}

	static string GetPersistentIDString(int b1, int b2, int b3, int b4)
	{
		string id;
		id += IntToHex(b1);
		id += IntToHex(b2);
		id += IntToHex(b3);
		id += IntToHex(b4);
		return id;
	}

	// ------------------------------------------------------------
	// Expansion GetWeightedRandom
	// ------------------------------------------------------------
	//! Returns an index into the 'weights' array, or -1 if all weights are zero
	static int GetWeightedRandom( array< float > weights )
	{
		float weightSum = 0;
		for ( int i = 0; i < weights.Count(); i++ )
		{
			weightSum += weights[i];
		}

		if ( weightSum == 0 )
			return -1;

		float rnd = Math.RandomFloat( 0, weightSum );
		int index = 0;
		int count = weights.Count();
		while ( index < count )
		{
			if ( rnd < weights[ index ] )
			{
				return index;
			}

			rnd -= weights[index++];
		}

		//! Should never get here
		return -1;
	}

	static array< string > FindInLocation( string folder, string ext = "", int mode = ExpansionFindFileMode.FILES, bool recursive = false )
	{
#ifdef PLATFORM_LINUX
	#ifdef DAYZ_1_25
		return FindInLocationImpl_Linux_T179707(folder, ext, mode, recursive);
	#else
		return FindInLocationImpl(folder, ext, mode, recursive);
	#endif
#else
		return FindInLocationImpl(folder, ext, mode, recursive);
#endif
	}

	static array< string > FindInLocationImpl( string folder, string ext = "", int mode = ExpansionFindFileMode.FILES, bool recursive = false )
	{
		array< string > files = new array< string >;
		if (!FileExist(folder))
			return files;
		string fileName;
		FileAttr fileAttr;
		string pattern = folder + "*";
		if (!recursive)
			pattern += ext;
		FindFileHandle findFileHandle = FindFile( pattern, fileName, fileAttr, 0 );
		if ( findFileHandle )
		{
			bool isValid = true;

			bool includeFiles;
			if (mode & ExpansionFindFileMode.FILES)
				includeFiles = true;

			bool includeDirs;
			if (mode & ExpansionFindFileMode.DIRECTORIES)
				includeDirs = true;

			while (isValid)
			{
				bool isDir = false;
				if (fileAttr & FileAttr.DIRECTORY)
					isDir = true;

				if (fileName.Length() > 0)
				{
					if ((includeFiles && !isDir) || (includeDirs && isDir))
					{
						if (!recursive || ExpansionString.EndsWithIgnoreCase(fileName, ext))
							files.Insert( fileName );
					}

					if (recursive && isDir)
					{
						TStringArray subFolderFileNames = FindInLocationImpl(folder + fileName + "\\", ext, mode, true);
						foreach (string subFolderFileName: subFolderFileNames)
						{
							files.Insert( fileName + "\\" + subFolderFileName );
						}
					}
				}

				isValid = FindNextFile(findFileHandle, fileName, fileAttr);
			}

			CloseFindFile( findFileHandle );
		}
	#ifdef DIAG_DEVELOPER
		EXTrace.Print(EXTrace.MISC, null, "FindInLocationImpl " + folder + "*" + ext + " mode=" + typename.EnumToString(ExpansionFindFileMode, mode) + " recursive=" + recursive.ToString());
		foreach (string file: files)
		{
			EXTrace.Print(EXTrace.MISC, null, folder + file);
		}
	#endif
		return files;
	}

	//! Workaround for https://feedback.bistudio.com/T179707 (will be fixed in 1.26)
	//! @note will only find files, not directories
	static array<string> FindInLocationImpl_Linux_T179707(string folder, string ext = "", int mode = ExpansionFindFileMode.FILES, bool recursive = false)
	{
		array<string> files = new array< string >;
		if (!FileExist(folder))
			return files;
		folder.Replace("\\", "/");
		string fileName;
		//! Under Linux, fileAttr cannot be used, see https://feedback.bistudio.com/T179707
		FileAttr fileAttr;
		FindFileHandle findFileHandle = FindFile( folder + "*", fileName, fileAttr, 0 );
		//! Under Linux, FindFile always returns zero, see https://feedback.bistudio.com/T182004
		//if (findFileHandle)
		//{
			bool isValid = true;

			bool includeFiles;
			if (mode & ExpansionFindFileMode.FILES)
				includeFiles = true;

			bool includeDirs;
			if (mode & ExpansionFindFileMode.DIRECTORIES)
				includeDirs = true;

			FileHandle file;
			while (isValid)
			{
				if (fileName.Length() > 0)
				{
					if (!ext || ExpansionString.EndsWithIgnoreCase(fileName, ext))
						files.Insert(fileName);

					if (recursive)
					{
						TStringArray subFolderFileNames = FindInLocationImpl_Linux_T179707(folder + fileName + "\\", ext, mode, true);
						foreach (string subFolderFileName: subFolderFileNames)
						{
							files.Insert( fileName + "\\" + subFolderFileName );
						}
					}
				}

				isValid = FindNextFile(findFileHandle, fileName, fileAttr);
			}

			CloseFindFile(findFileHandle);
		//}
		return files;
	}

	static array< string > FindFilesInLocation( string folder, string ext = "", bool recursive = false )
	{
		return FindInLocation(folder, ext, ExpansionFindFileMode.FILES, recursive);
	}

	static array< string > FindDirectoriesInLocation( string folder, string ext = "", bool recursive = false )
	{
		return FindInLocation(folder, ext, ExpansionFindFileMode.DIRECTORIES, recursive);
	}

	static bool MakeDirectoryRecursive(string path)
	{
		TStringArray parts();
		path.Split("\\", parts);
		path = "";
		foreach (string part: parts)
		{
			path += part + "\\";
			if (part.IndexOf(":") == part.Length() - 1)
				continue;
			if (!FileExist(path) && !MakeDirectory(path))
			{
				EXError.Error(null, "Couldn't create directory " + path);
				return false;
			}
		}
		return true;
	}

	//! Copies the directory tree rooted in srcDir to dstDir.
	//! If `ext` is given, limit to files and directories with that extension.
	//! If `move` is true and copying is successful, removes srcDir afterwards if empty.
	static bool CopyDirectoryTree(string srcDir, string dstDir, string ext = "", bool move = false)
	{
		ExpansionString srcDirEx = srcDir;
		if (srcDirEx.LastIndexOf("\\") != srcDir.Length() - 1)
			srcDir += "\\";

		ExpansionString dstDirEx = dstDir;
		if (dstDirEx.LastIndexOf("\\") != dstDir.Length() - 1)
			dstDir += "\\";

		if (!FileExist(dstDir) && !MakeDirectoryRecursive(dstDir))
			return false;

		string fileName;
		FileAttr fileAttr;
		FindFileHandle findFileHandle = FindFile(srcDir + "*" + ext, fileName, fileAttr, 0);

		if (!findFileHandle)
		{
			//! FindFileHandle of NULL means there were no files found
			return false;
		}

		if (fileName.Length() > 0 && !CopyFileOrDirectoryTree(srcDir + fileName, dstDir + fileName, ext, move, fileAttr))
		{
			EXPrint("ERROR: CopyDirectoryTree " + srcDir + fileName + " " + dstDir + fileName + " ext " + ext + " move " + move + " failed");
			CloseFindFile(findFileHandle);
			return false;
		}

		while (FindNextFile(findFileHandle, fileName, fileAttr))
		{
			if (fileName.Length() > 0 && !CopyFileOrDirectoryTree(srcDir + fileName, dstDir + fileName, ext, move, fileAttr))
			{
				EXPrint("ERROR: CopyDirectoryTree " + srcDir + fileName + " " + dstDir + fileName + " ext " + ext + " move " + move + " failed");
				CloseFindFile(findFileHandle);
				return false;
			}
		}

		CloseFindFile(findFileHandle);

		if (move && (!ext || !FindFilesInLocation(srcDir).Count()) && !DeleteFile(srcDir))
		{
			EXPrint("ERROR: CopyDirectoryTree remove src failed");
			return false;
		}

		return true;
	}

	/**
	 * @brief delete whole directory structure rooted at path.
	 * If `ext` is given, only files matching that extension are deleted (else all files).
	 * Directories that still contain files after all matching files have been deleted are kept.
	 * 
	 * @note passed in path needs to end with backslash!
	 */
	static bool DeleteDirectoryStructureRecursive(string path, string ext = string.Empty)
	{
		if (FileExist(path))
		{
			TStringArray files = ExpansionStatic.FindFilesInLocation(path, ext);
			foreach (string baseName: files)
			{
				DeleteFile(path + baseName);
			}

			TStringArray dirs = ExpansionStatic.FindDirectoriesInLocation(path);
			foreach (string dirBaseName: dirs)
			{
				DeleteDirectoryStructureRecursive(path + dirBaseName + "\\", ext);
			}

			return DeleteFile(path);
		}

		return false;
	}

	//! @note if copying a directory, make sure paths end with "\\" or provide fileAttr parameter
	static bool CopyFileOrDirectoryTree(string srcPath, string dstPath, string ext = "", bool move = false, FileAttr fileAttr = 0)
	{
		bool isDir = (fileAttr & FileAttr.DIRECTORY);
		ExpansionString srcPathEx = srcPath;
		if (!isDir && srcPathEx.LastIndexOf("\\") == srcPath.Length() - 1)
			isDir = true;

		if (isDir)
		{
			if (!CopyDirectoryTree(srcPath, dstPath, ext, move))
				return false;
		}
		else
		{
			ExpansionString dstPathEx = dstPath;
			string dstDir = dstPath.Substring(0, dstPathEx.LastIndexOf("\\"));
			if (!FileExist(dstDir) && !MakeDirectoryRecursive(dstDir))
				return false;

			if (!CopyFile(srcPath, dstPath))
			{
				EXPrint("ERROR: CopyFileOrDirectoryTree copy failed");
				return false;
			}

			if (move && !DeleteFile(srcPath))
			{
				EXPrint("ERROR: CopyFileOrDirectoryTree remove src failed");
				return false;
			}
		}

		return true;
	}

	#ifdef ENFUSION_AI_PROJECT
	static TStringArray GetWorkingAIClasses()
	{
		return { "eAI_SurvivorM_Mirek", "eAI_SurvivorM_Denis", "eAI_SurvivorM_Boris",
		"eAI_SurvivorM_Cyril", "eAI_SurvivorM_Elias", "eAI_SurvivorM_Francis", "eAI_SurvivorM_Guo",
		"eAI_SurvivorM_Hassan", "eAI_SurvivorM_Indar", "eAI_SurvivorM_Jose",
		"eAI_SurvivorM_Kaito", "eAI_SurvivorM_Lewis", "eAI_SurvivorM_Manua",
		"eAI_SurvivorM_Niki", "eAI_SurvivorM_Oliver", "eAI_SurvivorM_Peter",
		"eAI_SurvivorM_Quinn", "eAI_SurvivorM_Rolf", "eAI_SurvivorM_Seth",
		"eAI_SurvivorM_Taiki", "eAI_SurvivorF_Linda", "eAI_SurvivorF_Maria",
		"eAI_SurvivorF_Frida", "eAI_SurvivorF_Gabi", "eAI_SurvivorF_Helga",
		"eAI_SurvivorF_Irena", "eAI_SurvivorF_Judy", "eAI_SurvivorF_Keiko",
		"eAI_SurvivorF_Eva", "eAI_SurvivorF_Naomi", "eAI_SurvivorF_Baty"};
	}
	#endif

	//! DEPRECATED, use FormatInt
	static string IntToCurrencyString(int number, string separator, bool shorten = false)
	{
		return FormatInt(number, shorten);
	}

	/**
	 * @brief Format integer number as string
	 * 
	 * @param number
	 * @param shorten If true, use K and M suffixes to denote thousands and millions
	 * @param useLocale Use localized thousands separator (otherwise, scientific)
	 */
	static string FormatInt(int number, bool shorten = false, bool useLocale = true)
	{
		int abs;

		//! Protect against integer overflow in AbsInt
		if (number > int.MIN)
		{
			abs = Math.AbsInt(number);

			if (abs < 1000)
				return number.ToString();
		}
		else
		{
			abs = int.MAX;
		}

		if (shorten)
		{
			float dec = number;
			string suffix;

			if (abs >= 1000000)
			{
				dec /= 1000000;
				suffix = "M";
			}
			else
			{
				//! abs >= 1000
				dec /= 1000;
				suffix = "K";
			}

			dec = Math.Round(dec * 100) / 100;

			string str = dec.ToString();

			if (useLocale)
				str.Replace(".", Widget.TranslateString("#STR_EXPANSION_NUMBER_SEPARATOR_DECIMAL"));

			return str + suffix;
		}

		return FormatIntStr(number.ToString(), useLocale);
	}

	static string FormatIntStr(string numberStr, bool useLocale = true)
	{
		int len = numberStr.Length();
		int digits = len;

		if (numberStr[0] == "-")
			digits--;

		if (digits < 4)
			return numberStr;

		int processedCount = 0;

		string str;

		string separator;

		if (useLocale)
			separator = Widget.TranslateString("#STR_EXPANSION_NUMBER_SEPARATOR_THOUSANDS");

		if (!separator)
			separator = " ";

		for (int i = (len - 1); i >= 0; i--)
		{
			str = numberStr[i] + str;

			if (digits == len || i)
				processedCount += 1;

			if ((processedCount % 3) == 0 && processedCount < digits)
				str = separator + str;
		}

		return str;
	}

	static bool Key_SHIFT()
	{
		return( ( KeyState( KeyCode.KC_LSHIFT ) > 0 ) || ( KeyState( KeyCode.KC_RSHIFT ) > 0 ) );
	}

	static bool Key_CTRL()
	{
		return( ( KeyState( KeyCode.KC_LCONTROL ) > 0 ) || ( KeyState( KeyCode.KC_RCONTROL ) > 0 ) );
	}

	static bool Key_ALT()
	{
		return( ( KeyState( KeyCode.KC_LMENU ) > 0 ) || ( KeyState( KeyCode.KC_RMENU ) > 0 ) );
	}

	static bool Key_Y()
	{
		return KeyState(KeyCode.KC_Y) > 0;
	}

	static bool Key_Z()
	{
		return KeyState(KeyCode.KC_Z) > 0;
	}

	static bool Key_C()
	{
		return KeyState(KeyCode.KC_C) > 0;
	}

	static bool Key_B()
	{
		return KeyState(KeyCode.KC_B) > 0;
	}

	static bool Key_S()
	{
		return KeyState(KeyCode.KC_S) > 0;
	}

	static bool Key_UP()
	{
		return KeyState(KeyCode.KC_UP) > 0;
	}

	static bool Key_DOWN()
	{
		return KeyState(KeyCode.KC_DOWN) > 0;
	}

	static bool Key_LEFT()
	{
		return KeyState(KeyCode.KC_LEFT) > 0;
	}

	static bool Key_RIGHT()
	{
		return KeyState(KeyCode.KC_RIGHT) > 0;
	}

	static bool INPUT_FORWARD()
	{
   		return GetGame().GetInput().LocalPress( "UAMoveForward", false );
	}

	static bool INPUT_BACK()
	{
   		return GetGame().GetInput().LocalPress( "UAMoveBack", false );
	}

	static bool INPUT_LEFT()
	{
   		return GetGame().GetInput().LocalPress( "UAMoveLeft", false );
	}

	static bool INPUT_RIGHT()
	{
   		return GetGame().GetInput().LocalPress( "UAMoveRight", false );
	}

	static bool INPUT_GETOVER()
	{
   		return GetGame().GetInput().LocalPress( "UAGetOver", false );
	}

	static bool INPUT_STANCE()
	{
   		return GetGame().GetInput().LocalPress( "UAStance", false );
	}

	static void MessageNearPlayers(vector position, float radius, string msg)
	{
		float radiusSq = radius * radius;
		array<Man> players();
		GetGame().GetPlayers(players);
		foreach (Man player: players)
		{
			if (vector.DistanceSq(position, player.GetPosition()) < radiusSq)
			{
				GetGame().RPCSingleParam(player, ERPCs.RPC_USER_ACTION_MESSAGE, new Param1<string>(msg), true, player.GetIdentity());
			}
		}
	}

	static vector GetCursorHitPos(out Object hitObject = null, float fallbackDistance = 20.0)
	{
	#ifdef EXTRACE_DIAG
		auto trace = EXTrace.Start(EXTrace.MISC, ExpansionStatic);
	#endif

		vector begPos = GetGame().GetCurrentCameraPosition();
		vector endPos = begPos + GetGame().GetCurrentCameraDirection() * 1000.0;

	#ifdef EXTRACE_DIAG
		EXTrace.Add(trace, begPos);
		EXTrace.Add(trace, endPos);
	#endif

		vector hitPosition, hitNormal;
		int hitComponent;
		set<Object> results = new set<Object>;

		if (DayZPhysics.RaycastRV(begPos, endPos, hitPosition, hitNormal, hitComponent, results, null, GetGame().GetPlayer()))
		{
			if (results.Count())
				hitObject = results[0];
		}
		else if (fallbackDistance)
		{
			hitPosition = begPos + GetGame().GetCurrentCameraDirection() * fallbackDistance;
			float surfaceY = GetGame().SurfaceY(hitPosition[0], hitPosition[2]);
			if (surfaceY > hitPosition[1])
				hitPosition[1] = surfaceY;
		}

	#ifdef EXTRACE_DIAG
		EXTrace.Add(trace, hitPosition);
	#endif

		return hitPosition;
	}

	static vector GetSurfacePosition(vector position)
	{
		return GetSurfacePosition(position[0], position[2]);
	}

	static vector GetSurfacePosition(float x, float z)
	{
		return Vector(x, GetGame().SurfaceY(x, z), z);
	}

	static vector GetSurfaceRoadPosition(vector position, RoadSurfaceDetection rsd = RoadSurfaceDetection.UNDER)
	{
		return GetSurfaceRoadPosition(position[0], position[1], position[2], rsd);
	}

	static vector GetSurfaceRoadPosition(float x, float y, float z, RoadSurfaceDetection rsd = RoadSurfaceDetection.UNDER)
	{
		return Vector(x, GetGame().SurfaceRoadY3D(x, y + 0.1, z, rsd), z);
	}

	static vector GetSurfaceRoadPosition(float x, float z, RoadSurfaceDetection rsd = RoadSurfaceDetection.LEGACY)
	{
		return Vector(x, GetGame().SurfaceRoadY(x, z, rsd), z);
	}

	static float GetSurfaceWaterDepth(vector position)
	{
		return GetSurfaceWaterDepth(position[0], position[2]);
	}

	static float GetSurfaceWaterDepth(float x, float z)
	{
		return GetGame().GetWaterDepth(GetSurfacePosition(x, z));
	}

	//! Get ground surface position or water surface position, whichever has higher elevation
	static vector GetSurfaceWaterPosition(vector position)
	{
		return GetSurfaceWaterPosition(position[0], position[2]);
	}

	//! Get ground surface position or water surface position, whichever has higher elevation
	static vector GetSurfaceWaterPosition(float x, float z)
	{
		vector position = GetSurfacePosition(x, z);
		float waterDepth = GetGame().GetWaterDepth(position);
		if (waterDepth > 0)
			position[1] = position[1] + waterDepth;
		return position;
	}

	static bool SurfaceIsWater(vector position)
	{
		string type;
		GetGame().SurfaceGetType3D(position[0], position[1] + 0.1, position[2], type);
		if (type.Contains("water"))
			return true;

		return SurfaceIsWater(position[0], position[2]);
	}

	static bool SurfaceIsWater(float x, float z)
	{
		return GetGame().SurfaceIsSea(x, z) || GetGame().SurfaceIsPond(x, z);
	}

	static string GetImpactSurfaceType(Object directHit, vector hitPosition, vector relativeVelocityBefore)
	{
		string surfaceImpact;

		if (directHit)
		{
			vector add = relativeVelocityBefore.Normalized() * 0.1;
			if (DayZPhysics.GetHitSurface(
				directHit,
				hitPosition - add,
				hitPosition + add,
				surfaceImpact) && surfaceImpact != string.Empty)
			{
			#ifdef DIAG_DEVELOPER
				EXTrace.Print(EXTrace.AI, null, "GetHitSurface " + surfaceImpact);
			#endif

				return surfaceImpact;
			}

			//! Sometimes, GetHitSurface fails to return something useful
			if (directHit.IsMan() || directHit.IsInherited(DayZCreature))
				surfaceImpact = "Hit_MeatBones";
			else if (directHit.IsBuilding())
				surfaceImpact = "Hit_Concrete";
			else if (directHit.IsBush())
				surfaceImpact = "Hit_Foliage";
			else if (directHit.IsTree())
				surfaceImpact = "Hit_Wood";
			else if (directHit.IsRock())
				surfaceImpact = "Hit_Gravel";
			else if (directHit.IsWeapon() || directHit.IsMagazine() || directHit.IsTransport())
				surfaceImpact = "Hit_Metal";
			else if (directHit.IsClothing())
				surfaceImpact = "Hit_Textile";
			else if (SurfaceIsWater(hitPosition))
				surfaceImpact = "Hit_Water";
			else
				surfaceImpact = "Hit_Dirt";
		}
		else
		{
			if (SurfaceIsWater(hitPosition))
				surfaceImpact = "Hit_Water";
			else
				surfaceImpact = "Hit_Dirt";
		}

		return surfaceImpact;
	}

	//! @brief return lowercase game world name, with "gloom" suffix removed (if present)
	static string GetCanonicalWorldName(CGame game = NULL)
	{
		if (!game)
			game = GetGame();

		string worldName;
		game.GetWorldName(worldName);
		worldName.ToLower();

		if (worldName.Length() < 5)
			return worldName;

		if (worldName.IndexOf("gloom") == worldName.Length() - 5)
			worldName = worldName.Substring(0, worldName.Length() - 5);

		return worldName;
	}

	static string VectorToString(vector vec, ExpansionVectorToString format = ExpansionVectorToString.Beautify, int precision = 2)
	{
		float m = Math.Pow(10, precision);
		TStringArray output();

		for (int i = 0; i < 3; i++)
			output.Insert((Math.Round(vec[i] * m) / m).ToString());

		switch (format)
		{
			case ExpansionVectorToString.Beautify:
				return "<" + ExpansionString.JoinStrings(output, ", ") + ">";
			case ExpansionVectorToString.Labels:
				return "X: " + output[0] + " Y: " + output[1] + " Z: " + output[2];
		}

		return ExpansionString.JoinStrings(output, " ");
	}

	static bool GetVariableIntByName(typename type, string name, out int val)
	{
		return StringToEnumEx(type, name, val);
	}

	static void LockInventoryRecursive(EntityAI entity, int lockType = HIDE_INV_FROM_SCRIPT)
	{
		int attcount = entity.GetInventory().AttachmentCount();
		for (int att = 0; att < attcount; att++)
		{
			EntityAI attachmentEntity = entity.GetInventory().GetAttachmentFromIndex(att);
			if (attachmentEntity)
				LockInventoryRecursive(attachmentEntity, lockType);
		}
		entity.GetInventory().LockInventory(lockType);
	}

	static void UnlockInventoryRecursive(EntityAI entity, int lockType = HIDE_INV_FROM_SCRIPT)
	{
		int attcount = entity.GetInventory().AttachmentCount();
		for (int att = 0; att < attcount; att++)
		{
			EntityAI attachmentEntity = entity.GetInventory().GetAttachmentFromIndex(att);
			if (attachmentEntity)
				UnlockInventoryRecursive(attachmentEntity, lockType);
		}
		entity.GetInventory().UnlockInventory(lockType);
	}

	/**
	 * @brief Check whether inventory of entity or its parent is locked
	 * 
	 * @param [out] lockType      Set to LOCK_FROM_SCRIPT or HIDE_INV_FROM_SCRIPT if locked
	 * @param [out] lockedEntity  Set to entity whose inventory is locked (i.e. entity itself or parent)
	 * 
	 * @param checkParent         Don't check parent if set to false
	 * 
	 * @return true if entity or parent is locked, else false
	 */
	static bool IsInventoryLocked(EntityAI entity, out int lockType = 0, out EntityAI lockedEntity = null, bool checkParent = true)
	{
		auto inventory = entity.GetInventory();

		if (inventory && inventory.IsInventoryLocked())
		{
			if (inventory.IsInventoryLockedForLockType(HIDE_INV_FROM_SCRIPT))
				lockType = HIDE_INV_FROM_SCRIPT;
			else
				lockType = LOCK_FROM_SCRIPT;

			lockedEntity = entity;

			return true;
		}

		if (checkParent && entity.GetHierarchyParent())
			return IsInventoryLocked(entity.GetHierarchyParent(), lockType, lockedEntity, false);

		return false;
	}

	//! Only kept for compatibility with old mods, remove after next update
	void LoadFlagTextures()
	{
		Error("DEPRECATED - please use ExpansionFlagTextures::Load instead!");
	}

	//! Only kept for compatibility with old mods, remove after next update
	void AddFlagTexture( string path, string name )
	{
		Error("DEPRECATED - please use ExpansionFlagTextures::Add instead!");
	}

	//! Only kept for compatibility with old mods, remove after next update
	bool RemoveFlagTexture( string path )
	{
		Error("DEPRECATED - please use ExpansionFlagTextures::Remove instead!");
		return false;
	}
}
