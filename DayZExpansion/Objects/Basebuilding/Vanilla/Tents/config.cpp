class CfgPatches
{
	class DayZExpansion_Objects_BaseBuilding_Vanilla_Tents
	{
		requiredVersion=0.1;
		requiredAddons[]=
		{
			"DZ_Gear_Camping"
		};
	};
};
class CfgVehicles
{
	class TentBase;
	class GUIInventoryAttachmentsProps;
	class MediumTent: TentBase
	{
		attachments[]+=
		{
			"Att_ExpansionCodeLock"
		};
		class GUIInventoryAttachmentsProps: GUIInventoryAttachmentsProps
		{
			class Attachments
			{
				attachmentSlots[]+=
				{
					"Att_ExpansionCodeLock"
				};
			};
		};
	};
	class LargeTent: TentBase
	{
		attachments[]+=
		{
			"Att_ExpansionCodeLock"
		};
		class GUIInventoryAttachmentsProps: GUIInventoryAttachmentsProps
		{
			class Attachments
			{
				attachmentSlots[]+=
				{
					"Att_ExpansionCodeLock"
				};
			};
		};
	};
	class CarTent: TentBase
	{
		attachments[]+=
		{
			"Att_ExpansionCodeLock"
		};
		class GUIInventoryAttachmentsProps: GUIInventoryAttachmentsProps
		{
			class Attachments
			{
				attachmentSlots[]+=
				{
					"Att_ExpansionCodeLock"
				};
			};
		};
	};
	class PartyTent: TentBase
	{
		attachments[]+=
		{
			"Att_ExpansionCodeLock"
		};
		class GUIInventoryAttachmentsProps: GUIInventoryAttachmentsProps
		{
			class Attachments
			{
				attachmentSlots[]+=
				{
					"Att_ExpansionCodeLock"
				};
			};
		};
	};
};
