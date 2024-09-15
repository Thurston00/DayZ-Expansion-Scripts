modded class ItemBase
{
	ref map<int, ref ExpansionTowConnection> m_Expansion_Connections = new map<int, ref ExpansionTowConnection>();
	int m_Expansion_TowConnectionSynchMask;
	int m_Expansion_TowConnectionMask;

	int m_Expansion_TowConnectionIndex;
	bool m_Expansion_IsTowing;

	EntityAI m_Expansion_ChildTow;

	int m_Expansion_ChildTowNetworkIDLow;
	int m_Expansion_ChildTowNetworkIDHigh;

	void ItemBase()
	{
		int numTowConnections = Expansion_NumberTowConnections();

		if (numTowConnections > 0)
		{
			RegisterNetSyncVariableBool("m_Expansion_IsTowing");

			RegisterNetSyncVariableInt("m_Expansion_TowConnectionSynchMask");

			RegisterNetSyncVariableInt("m_Expansion_ChildTowNetworkIDLow");
			RegisterNetSyncVariableInt("m_Expansion_ChildTowNetworkIDHigh");
		}

		for (int i = 0; i < numTowConnections; i++)
		{
			m_Expansion_Connections[i] = new ExpansionTowConnection();
		}
	}

	void Expansion_CreateTow(Object tow, int index)
	{
		if (m_Expansion_IsTowing)
			return;

		CarScript car;
		ItemBase item;

		if (!Class.CastTo(car, tow) && !Class.CastTo(item, tow))
			return;

		m_Expansion_ChildTow = EntityAI.Cast(tow);
		m_Expansion_IsTowing = true;
		m_Expansion_TowConnectionIndex = index;

		bool success = false;

		if (car)
			success = car.Expansion_OnTowCreated(this, Expansion_GetTowPosition(), m_Expansion_TowConnectionIndex);
		else if (item)
			success = item.Expansion_OnTowCreated(this, Expansion_GetTowPosition(), m_Expansion_TowConnectionIndex);

		if (!success)
		{
			m_Expansion_ChildTow = null;
			m_Expansion_IsTowing = false;
			m_Expansion_TowConnectionIndex = -1;
			return;
		}

		if (!GetGame().IsClient())
		{
			m_Expansion_ChildTow.GetNetworkID(m_Expansion_ChildTowNetworkIDLow, m_Expansion_ChildTowNetworkIDHigh);

			SetSynchDirty();
		}
	}

	/*private*/ bool Expansion_OnTowCreated(Object parent, vector towPos, int index)
	{
		if (index < 0 || index >= Expansion_NumberTowConnections())
			return false;

		vector connectionPoint;
		vector connectionSize;

		Expansion_GetTowConnection(index, connectionPoint, connectionSize);

		m_Expansion_TowConnectionSynchMask |= 1 << index;
		m_Expansion_TowConnectionMask = m_Expansion_TowConnectionSynchMask;

		m_Expansion_Connections[index].m_Joint = dJointCreateBallSocket(parent, this, towPos, connectionPoint, false, 0.1);
		m_Expansion_Connections[index].m_Parent = EntityAI.Cast(parent);
		m_Expansion_Connections[index].m_Attached = true;

		if (GetGame().IsServer())
			SetSynchDirty();

		return true;
	}

	EntityAI Expansion_GetTowedEntity()
	{
		if (m_Expansion_IsTowing)
			return m_Expansion_ChildTow;

		return NULL;
	}

	void Expansion_DestroyTow()
	{
		if (!m_Expansion_IsTowing)
			return;

		CarScript car;
		ItemBase item;

		if (Class.CastTo(car, m_Expansion_ChildTow))
		{
			car.Expansion_OnTowDestroyed(this, m_Expansion_TowConnectionIndex);
		}

		if (Class.CastTo(item, m_Expansion_ChildTow))
		{
			item.Expansion_OnTowDestroyed(this, m_Expansion_TowConnectionIndex);
		}

		m_Expansion_ChildTow = NULL;
		m_Expansion_IsTowing = false;
		m_Expansion_TowConnectionIndex = -1;

		if (GetGame().IsServer())
			SetSynchDirty();
	}

	/*private*/ void Expansion_OnTowDestroyed(EntityAI parent, int connectionIndex)
	{
		dJointDestroy(m_Expansion_Connections[connectionIndex].m_Joint);
		m_Expansion_Connections[connectionIndex].m_Parent = null;
		m_Expansion_Connections[connectionIndex].m_Attached = false;

		m_Expansion_TowConnectionSynchMask &= ~(1 << connectionIndex);
		m_Expansion_TowConnectionMask = m_Expansion_TowConnectionSynchMask;

		if (GetGame().IsServer())
			SetSynchDirty();
	}

	bool Expansion_IsTowing()
	{
		return m_Expansion_IsTowing;
	}

	int Expansion_NumberTowConnections()
	{
		return 0;
	}

	void Expansion_GetTowConnection(int index, out vector position, out vector size)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.VEHICLES, this, "Expansion_GetTowConnection").Add(index);
#endif

	}

	bool Expansion_GetOverlappingTowConnection(vector towPosition, float towRadius, out int index)
	{
		index = -1;

		towPosition = WorldToModel(towPosition);

		for (int i = 0; i < Expansion_NumberTowConnections(); i++)
		{
			if (m_Expansion_Connections[i].m_Attached)
				continue;

			vector conPos, conSize;
			Expansion_GetTowConnection(i, conPos, conSize);

			if (Math3D.IntersectSphereBox(towPosition, towRadius, conPos - conSize, conPos + conSize))
			{
				index = i;
				return true;
			}
		}

		return false;
	}

	vector Expansion_GetTowPosition()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Expansion_GetTowPosition");
#endif

		vector minMax[2];
		GetCollisionBox(minMax);
		return Vector(0.0, minMax[0][1], minMax[0][2] - dBodyGetCenterOfMass(this)[2]);
	}

	vector Expansion_GetTowDirection()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Expansion_GetTowDirection");
#endif

		return -GetDirection();
	}

	float Expansion_GetTowLength()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "Expansion_GetTowLength");
#endif

		return 0.4;
	}

	bool Expansion_CanConnectTow(notnull Object other)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.VEHICLES, this, "Expansion_CanConnectTow").Add(other);
#endif

		return false;
	}

	bool Expansion_IsPlane()
	{
		return false;
	}

	bool Expansion_IsBike()
	{
		return false;
	}

	bool Expansion_IsBoat()
	{
		return false;
	}

	bool Expansion_IsHelicopter()
	{
		return false;
	}

	bool Expansion_IsCar()
	{
		return false;
	}

	bool Expansion_IsDuck()
	{
		return false;
	}

	override bool Expansion_CanObjectAttach(Object obj)
	{
		if (obj.IsMan())
			return Expansion_CanPlayerAttach();

		return false;
	}

	override void OnVariablesSynchronized()
	{
		super.OnVariablesSynchronized();

		if (!IsMissionOffline())
		{
			m_Expansion_ChildTow = NULL;
			if (m_Expansion_IsTowing)
			{
				m_Expansion_ChildTow = EntityAI.Cast(GetGame().GetObjectByNetworkId(m_Expansion_ChildTowNetworkIDLow, m_Expansion_ChildTowNetworkIDHigh));
			}

			if (m_Expansion_TowConnectionMask != m_Expansion_TowConnectionSynchMask)
			{
				m_Expansion_TowConnectionMask = m_Expansion_TowConnectionSynchMask;

				for (int i = 0; i < Expansion_NumberTowConnections(); i++)
				{
					int attached = (m_Expansion_TowConnectionMask >> i) & 1;
					m_Expansion_Connections[i].m_Attached = attached;
				}
			}
		}
	}
};
