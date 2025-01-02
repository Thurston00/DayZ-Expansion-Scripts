modded class SwapOldAndNewMagazine
{
	override void OnEntry (WeaponEventBase e)
	{
		eAIBase p;
		if (e && Class.CastTo(p, e.m_player))
		{
			super.eAI_Vanilla_OnEntry(e);  //! WeaponStateBase::eAI_Vanilla_OnEntry

			if (!m_newMagazine || !m_newDst || !m_newDst.IsValid())
			{
				Error("[wpnfsm] " + Object.GetDebugName(m_weapon) + " SwapOldAndNewMagazine, error - m_newMagazine(" + m_newMagazine + ") or destination(" + InventoryLocation.DumpToStringNullSafe(m_newDst) + ") is not set ");
			}
			else
			{
				e.m_player.GetInventory().ClearInventoryReservationEx( m_newMagazine , m_newDst );
				m_weapon.ShowMagazine();
				InventoryLocation lhand = new InventoryLocation();
				lhand.SetAttachment(e.m_player, m_newMagazine, InventorySlots.LEFTHAND);
			
				if (GameInventory.LocationSyncMoveEntity(lhand, m_newDst))
				{
					if (LogManager.IsWeaponLogEnable()) { wpnDebugPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " SwapOldAndNewMagazine, ok - new magazine removed from inv (LHand->Att)"); }
				}
				else
				{
					InventoryLocation il = new InventoryLocation();
					m_newMagazine.GetInventory().GetCurrentInventoryLocation(il);

					Error("[wpnfsm] " + Object.GetDebugName(m_weapon) + " SwapOldAndNewMagazine, error - cannot remove new mag from LHand and move " + ExpansionStatic.GetHierarchyInfo(m_newMagazine) + " from " + ExpansionStatic.DumpToString(il) + " to " + ExpansionStatic.DumpToString(m_newDst));

					il.Reset();

					bool keepUnlootable;

					//! @note cloning doesn't really help but cloning to ground at least allows the AI to pick another interactable/target
					//! than the mag (unlike cloning to inventory). Always clone mag to ground for now.
					////! Try to clone to cargo, if no free loc, clone to ground
					//if (!e.m_player.GetInventory().FindFreeLocationFor(m_newMagazine, FindInventoryLocationType.CARGO, il))
					//{
						vector m4[4];
						Math3D.MatrixIdentity4(m4);
						
						//! We don't care if a valid transform couldn't be found, we just want to preferably use it instead of placing on the player
						GameInventory.PrepareDropEntityPos(e.m_player, m_newMagazine, m4, false, GameConstants.INVENTORY_ENTITY_DROP_OVERLAP_DEPTH);
						il.SetGround(m_newMagazine, m4);

						keepUnlootable = true;

						EXPrint(this, "Cloning to ground " + ExpansionStatic.DumpToString(il));
					//}
					//else
					//{
						//EXPrint(this, "Cloning to cargo " + ExpansionStatic.DumpToString(il));
					//}

					if (ExpansionItemSpawnHelper.Clone(m_newMagazine, true, il, keepUnlootable) == null)
						Error("Cloning failed " + ExpansionStatic.GetDebugInfo(m_newMagazine) + " " + ExpansionStatic.DumpToString(il));

					GetGame().ObjectDelete(m_newMagazine);  //! Whether cloning succeeded or not, always delete the offending mag to break out of the event (else it will repeat over and over...)
				}
			}

			return;
		}

		super.OnEntry(e);
	}
}

modded class AttachNewMagazine
{
	override void OnEntry (WeaponEventBase e)
	{
		eAIBase p;
		if (e && Class.CastTo(p, e.m_player))
		{
			super.eAI_Vanilla_OnEntry(e);  //! WeaponStateBase::eAI_Vanilla_OnEntry

			if (m_newMagazine && m_newDst)
			{
				EntityAI directParent;

				InventoryLocation il = new InventoryLocation();
				if (!m_newMagazine.GetInventory().GetCurrentInventoryLocation(il))
				{
					//! Getting current location of new mag sometimes fails. Reason seems to be that WeaponAttachMagazine::OnAbort
					//! doesn't seem to get called for AI when needed, which would deal with moving previous mag/ammo in LHand back to 
					//! inventory/ground, and then any attempt to sync move new mag to already taken LHand loc in
					//! WeaponAttachMagazine::OnEntry seems to break getting loc for new mag.
					//! This is now dealt with in a (hacky) way in WeaponAttachMagazine::OnEntry, so the whole shenanigans below
					//! is probably not needed. Keeping it just in case.

					string notFoundLocInfo = ExpansionStatic.DumpToString(il);

					//! Gather as much info as possible about mag + its parent(s). Note that so far when the issue happened,
					//! neither were any parent or mag pending deletion nor was their inventory (or their parents' inventory) locked.

					string dbgInfoMag = ExpansionStatic.GetDebugInfo(m_newMagazine);

					EntityAI parent = m_newMagazine.GetHierarchyParent();
					directParent = parent;
					EntityAI child = m_newMagazine;

					if (parent)
					{
						//! Try and brute force find the mag in cargo of direct parent
						CargoBase cargo = parent.GetInventory().GetCargo();
						if (cargo)
						{
							int cargoWidth = cargo.GetWidth();
							int cargoHeight = cargo.GetHeight();

							bool found;
							int idx;

							for (int row = 0; row < cargoWidth; row++)
							{
								for (int col = 0; col < cargoHeight; col++)
								{
									if (parent.GetInventory().HasEntityInCargoEx(m_newMagazine, idx, row, col))
									{
										found = true;
										il.SetCargo(parent, m_newMagazine, idx, row, col, false);
										break;
									}

									idx++;
								}

								if (found)
									break;
							}
						}

						while (parent)
						{
							if (parent == p)
								dbgInfoMag += " on ";
							else
								dbgInfoMag += " in ";

							for (int i = 0; i < parent.GetInventory().GetAttachmentSlotsCount(); i++)
							{
								int slotId = parent.GetInventory().GetAttachmentSlotId(i);

								if (parent.GetInventory().FindAttachment(slotId) == child)
									dbgInfoMag += string.Format("slot %1 of ", InventorySlots.GetSlotName(slotId));
							}

							dbgInfoMag += ExpansionStatic.GetDebugInfo(parent, false);

							child = parent;
							parent = parent.GetHierarchyParent();
						}
					}

					if (il.IsValid())
					{
						EXError.Warn(this, ExpansionStatic.GetDebugInfo(e.m_player) + " [wpnfsm] " + ExpansionStatic.GetDebugInfo(m_weapon) + " AttachNewMagazine, warning - cannot get curr location of " + dbgInfoMag + " " + notFoundLocInfo + " but found via brute force " + ExpansionStatic.DumpToString(il));
					}
					else
					{
						Error(ExpansionStatic.GetDebugInfo(e.m_player) + " [wpnfsm] " + ExpansionStatic.GetDebugInfo(m_weapon) + " AttachNewMagazine, error - cannot get curr location of " + dbgInfoMag + " " + ExpansionStatic.DumpToString(il));
					}
				}

				bool success;

				if (il.IsValid())
				{
					if (GameInventory.LocationSyncMoveEntity(il, m_newDst))
					{
						m_weapon.ShowMagazine();
						if (LogManager.IsWeaponLogEnable()) { wpnDebugPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " AttachNewMagazine, ok - attached new magazine (LHand->dst)"); }

						success = true;
					}
					else
					{
						if (LogManager.IsWeaponLogEnable()) { wpnDebugPrint("[wpnfsm] " + Object.GetDebugName(m_weapon) + " AttachNewMagazine, error - cannot attach new magazine!"); }

						Error("[wpnfsm] " + ExpansionStatic.GetDebugInfo(m_weapon) + " AttachNewMagazine, error - cannot attach new magazine! src=" + ExpansionStatic.DumpToString(il) + " dst=" + ExpansionStatic.DumpToString(m_newDst));
					}
				}

				if (!success && GetGame().IsServer())
				{
					il.Reset();

					bool keepUnlootable;

					//! @note cloning doesn't really help but cloning to ground at least allows the AI to pick another interactable/target
					//! than the mag (unlike cloning to inventory). Always clone mag to ground for now.
					////! Try to clone to cargo, if no free loc, clone to ground
					//if (!e.m_player.GetInventory().FindFreeLocationFor(m_newMagazine, FindInventoryLocationType.CARGO, il))
					//{
						vector m4[4];
						Math3D.MatrixIdentity4(m4);
						
						//! We don't care if a valid transform couldn't be found, we just want to preferably use it instead of placing on the player
						GameInventory.PrepareDropEntityPos(e.m_player, m_newMagazine, m4, false, GameConstants.INVENTORY_ENTITY_DROP_OVERLAP_DEPTH);
						il.SetGround(m_newMagazine, m4);

						keepUnlootable = true;

						EXPrint(this, "Cloning to ground " + ExpansionStatic.DumpToString(il));
					//}
					//else
					//{
						//EXPrint(this, "Cloning to cargo " + ExpansionStatic.DumpToString(il));
					//}

					if (ExpansionItemSpawnHelper.Clone(m_newMagazine, true, il, keepUnlootable) == null)
						Error("Cloning failed " + ExpansionStatic.GetDebugInfo(m_newMagazine) + " " + ExpansionStatic.DumpToString(il));

					GetGame().ObjectDelete(m_newMagazine);  //! Whether cloning succeeded or not, always delete the offending mag to break out of the event (else it will repeat over and over...)

				/*
					if (directParent)
					{
						il.Reset();

						//! To rule out the problem being the direct parent, clone it to ground as well
						il.SetGround(directParent, m4);

						EXPrint(this, "Cloning to ground " + ExpansionStatic.DumpToString(il));

						if (ExpansionItemSpawnHelper.Clone(directParent, true, il, keepUnlootable) == null)
							Error("Cloning failed " + ExpansionStatic.GetDebugInfo(directParent) + " " + ExpansionStatic.DumpToString(il));

						GetGame().ObjectDelete(directParent);  //! Whether cloning succeeded or not, always delete the offending direct parent to break out of the event (else it will repeat over and over...)
					}
				*/
				}
			}
			else
				Error("[wpnfsm] " + Object.GetDebugName(m_weapon) + " AttachNewMagazine, error - no magazines configured for replace (m_new=NULL)");

			return;
		}

		super.OnEntry(e);
	}
}
