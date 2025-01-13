modded class ItemBase
{
	override int GetLiquidType()
	{
		if (m_VarLiquidType != LIQUID_NONE && !Liquid.m_LiquidInfosByType[m_VarLiquidType])
			EXError.ErrorOnce(this, "Item " + GetType() + " is misconfigured - liquid type " + m_VarLiquidType + " is not in CfgLiquidDefinitions!", {});

		return super.GetLiquidType();
	}
}
