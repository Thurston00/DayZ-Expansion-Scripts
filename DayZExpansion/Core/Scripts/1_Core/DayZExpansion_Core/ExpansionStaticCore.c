class ExpansionStaticCore
{
	string CF_GetISOTime()
	{
		int hour;
		int minute;
		int second;

		GetHourMinuteSecond(hour, minute, second);

		return hour.ToStringLen(2) + ":" + minute.ToStringLen(2) + ":" + second.ToStringLen(2);
	}
}

static void EXPrintToFile(string str, string fileName = "$profile:EXPrintToFile.log", int fileMode = FileMode.APPEND)
{
	FileHandle file = OpenFile(fileName, fileMode);

	if (file)
	{
		FPrintln(file, str);
		CloseFile(file);
	}
}
