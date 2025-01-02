modded class Trigger
{
#ifdef DIAG_DEVELOPER
	override protected Shape DrawDebugShape(vector pos, vector min, vector max, float radius, int color)
	{
		pos[1] = pos[1] + (max[1] - min[1]) * 0.5;  //! Fix vanilla debug shape origin

		return super.DrawDebugShape(pos, min, max, radius, color);
	}
#endif
}
