/**
 * ExpansionString.c
 *
 * DayZ Expansion Mod
 * www.dayzexpansion.com
 * © 2022 DayZ Expansion Mod Team
 *
 * This work is licensed under the Creative Commons Attribution-NonCommercial-NoDerivatives 4.0 International License. 
 * To view a copy of this license, visit http://creativecommons.org/licenses/by-nc-nd/4.0/.
 *
*/

class ExpansionString
{
	const string ZERO_WIDTH_SPACE = "​"; //! This may look like an empty string, but it is not. It's a zero-width space as UTF-8;

	protected string m_String;

	void ExpansionString(string str)
	{
		m_String = str;
	}

	string Get()
	{
		return m_String;
	}

	string BaseName()
	{
		return BaseName(m_String);
	}

	static string BaseName(string fileName)
	{
		fileName.Replace("\\", "/");
		int index = LastIndexOf(fileName, "/");
		if (index > -1)
			return fileName.Substring(index + 1, fileName.Length() - index);
		return fileName;
	}

	string DirName()
	{
		return DirName(m_String);
	}

	static string DirName(string fileName)
	{
		fileName.Replace("\\", "/");
		int index = LastIndexOf(fileName, "/");
		if (index > -1)
			return fileName.Substring(0, index);
		return fileName;
	}

	//! Neat little hash function, good for small datasets (< 64 k)
	//! https://stackoverflow.com/a/19661491
	static int Hash16(string str)
	{
		int hash;

		for (int i = 0; i < str.Length(); i++)
		{
			int ch = str[i].Hash() + i;
			hash += (hash << 5) + ch + (ch << 7);
		}

		return (hash ^ (hash >> 16)) & 0xffff;
	}

	//! Like vanilla Hash(), but using different prime (109 instead of 37) for better collision resilience
	static int BetterHash(string str)
	{
		int hash;

		for (int i = 0; i < str.Length(); i++)
		{
			int ascii = str[i].Hash();
			hash = hash * 109 + ascii;
			if (ascii > 127)
				hash -= 256;
		}
		   
		return hash;
	}

	static string CamelCaseToWords(string str, string sep = " ")
	{
		string output;
		string c;
		string cUpper;
		string cLower;
		bool wasLower;

		for (int j = 0; j < str.Length(); j++)
		{
			c = str[j];
			cUpper = c;
			cUpper.ToUpper();
			cLower = c;
			cLower.ToLower();
			if (wasLower && c == cUpper && c != cLower)
				output += sep;
			output += c;
			//! https://feedback.bistudio.com/T173348
			if (c != cUpper && c == cLower)
				wasLower = true;
			else
				wasLower = false;
		}

		return output;
	}

	//! Vanilla string.Replace truncates long text >:-(
	//! https://feedback.bistudio.com/T177558
	static int Replace(inout string text, string search, string replace)
	{
		int count;
		int searchLen = search.Length();
		int replaceLen = replace.Length();
		int index = text.IndexOf(search);

		while (index > -1)
		{
			text = text.Substring(0, index) + replace + text.Substring(index + searchLen, text.Length() - index - searchLen);
			count++;
			index = text.IndexOfFrom(index + replaceLen, search);
		}

		return count;
	}

	bool StartsWith(string prefix)
	{
		return m_String.IndexOf(prefix) == 0;
	}

	static bool StartsWith(string str, string prefix)
	{
		return str.IndexOf(prefix) == 0;
	}

	bool StartsWithIgnoreCase(string prefix)
	{
		return StartsWithIgnoreCase(m_String, prefix);
	}

	static bool StartsWithIgnoreCase(string str, string prefix)
	{
		str.ToLower();
		prefix.ToLower();
		return StartsWith(str, prefix);
	}

	bool EndsWith(string suffix, out int index = -1)
	{
		return EndsWith(m_String, suffix, index);
	}

	static bool EndsWith(string str, string suffix, out int index = -1)
	{
		int suffixLength = suffix.Length();
		if (str.Length() < suffixLength)
			return false;
		index = LastIndexOf(str, suffix);
		return index == str.Length() - suffixLength;
	}

	bool EndsWithIgnoreCase(string suffix, out int index = -1)
	{
		return EndsWithIgnoreCase(m_String, suffix, index);
	}

	static bool EndsWithIgnoreCase(string str, string suffix, out int index = -1)
	{
		str.ToLower();
		suffix.ToLower();
		return EndsWith(str, suffix, index);
	}

	bool Equals(string cmp)
	{
		return m_String == cmp;
	}

	//! DEPRECATED, alias for EqualsIgnoreCase
	bool EqualsCaseInsensitive(string cmp)
	{
		Error("DEPRECATED, use EqualsIgnoreCase");
		return EqualsIgnoreCase(m_String, cmp);
	}

	bool EqualsIgnoreCase(string cmp)
	{
		return EqualsIgnoreCase(m_String, cmp);
	}

	static bool EqualsIgnoreCase(string str, string cmp)
	{
		str.ToLower();
		cmp.ToLower();
		return str == cmp;
	}

	//! Reimplement LastIndexOf (vanilla string.LastIndexOf is broken...)
	int LastIndexOf(string sample)
	{
		return LastIndexOf(m_String, sample);
	}

	static int LastIndexOf(string str, string sample)
	{
		if (sample == string.Empty)
			return 0;
		int idx = -1;
		int lastIdx = -1;
		while (true)
		{
			idx = str.IndexOfFrom(idx + 1, sample);
			if (idx > -1)
				lastIdx = idx;
			else
				break;
		}
		return lastIdx;
	}

	string JustifyLeft(int length, string pad)
	{
		return JustifyLeft(m_String, length, pad);
	}

	static string JustifyLeft(string str, int length, string pad)
	{
		length = length - str.Length();

		for (int index = 0; index < length; index++)
		{
			str += pad;
		}

		return str;
	}

	string JustifyRight(int length, string pad)
	{
		return JustifyRight(m_String, length, pad);
	}

	static string JustifyRight(string str, int length, string pad)
	{
		length = length - str.Length();

		for (int index = 0; index < length; index++)
		{
			str = pad + str;
		}

		return str;
	}

	//! Reimplement ToAscii (vanilla string.ToAscii is broken...)
	int ToAscii(int index = 0)
	{
		return m_String[index].Hash();
	}

	static int ToAscii(string str, int index = 0)
	{
		return str[index].Hash();
	}

	/**
	 * @brief Removes the first character from the string
	 * 
	 * @param str input string
	 * 
	 * @return a copy of str with the first character removed
	 */
	string RemoveFirstChar() 
	{
		return m_String.Substring( 1, m_String.Length() - 1 );
	}

	static string RemoveFirstChar( string str ) 
	{
		return str.Substring( 1, str.Length() - 1 );
	}

	string StripExtension(string ext = string.Empty)
	{
		return StripExtension(m_String, ext);
	}

	static string StripExtension(string str, string ext = string.Empty)
	{
		if (ext == string.Empty)
		{
			int index = LastIndexOf(str, ".");

			if (index > -1)
				return str.Substring(0, index);

			return str;
		}

		if (EndsWithIgnoreCase(str, ext))
			return str.Substring( 0, str.Length() - ext.Length() );

		return str;
	}

	/**
	 * @brief Removes the last character from the string
	 * 
	 * @param str input string
	 * 
	 * @return a copy of str with the last character removed
	 */
	string RemoveLastChar() 
	{
		return m_String.Substring( 0, m_String.Length() - 1 );
	}

	static string RemoveLastChar( string str ) 
	{
		return str.Substring( 0, str.Length() - 1 );
	}

	static string JoinStrings(array<ExpansionString> strings, string glue = ", ")
	{
		string output = "";
		for (int i = 0; i < strings.Count(); i++)
		{
			if (glue && output)
				output += glue;
			output += strings[i].Get();
		}
		return output;
	}

	static string JoinStrings(TStringArray strings, string glue = ", ")
	{
		string output = "";
		for (int i = 0; i < strings.Count(); i++)
		{
			if (glue && output)
				output += glue;
			output += strings[i];
		}
		return output;
	}

	static int StrCmp(string a, string b)
	{
		for (int i = 0; i < Math.Min(a.Length(), b.Length()); i++)
		{
			if (a[i] < b[i])
				return -1;
			else if (a[i] > b[i])
				return 1;
		}

		return a.Length() - b.Length();
	}

	static int StrCaseCmp(string a, string b)
	{
		a.ToLower();
		b.ToLower();

		return StrCmp(a, b);
	}
}
