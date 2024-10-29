class ExpansionVehicleBaseBase : Transport
{
	ref map<int, ref ExpansionTowConnection> m_Expansion_Connections = new map<int, ref ExpansionTowConnection>();
	int m_Expansion_TowConnectionSynchMask;
	int m_Expansion_TowConnectionMask;

	int m_Expansion_TowConnectionIndex;
	bool m_Expansion_IsTowing;

	EntityAI m_Expansion_ChildTow;

	int m_Expansion_ChildTowNetworkIDLow;
	int m_Expansion_ChildTowNetworkIDHigh;

	bool m_Expansion_IsBeingPushed;

	void ExpansionVehicleBaseBase()
	{
		RegisterNetSyncVariableBool("m_Expansion_IsTowing");

		RegisterNetSyncVariableInt("m_Expansion_TowConnectionSynchMask");

		RegisterNetSyncVariableInt("m_Expansion_ChildTowNetworkIDLow");
		RegisterNetSyncVariableInt("m_Expansion_ChildTowNetworkIDHigh");

		for (int i = 0; i < Expansion_NumberTowConnections(); i++)
		{
			m_Expansion_Connections[i] = new ExpansionTowConnection();
		}
	}

	override void EEDelete(EntityAI parent)
	{
		super.EEDelete(parent);

		Expansion_DestroyTow();
	}

	override void EEKilled(Object killer)
	{
		super.EEKilled(killer);

		Expansion_DestroyTow();
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
	}

	bool Expansion_IsTowing()
	{
		return m_Expansion_IsTowing;
	}

	bool Expansion_IsTowPointFree(int index)
	{
		return !m_Expansion_Connections[index].m_Attached;
	}

	int Expansion_NumberTowConnections()
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_0(ExpansionTracing.VEHICLES, this, "GetAutoHoverTargetHeight");
#endif

		return 2;
	}

	void Expansion_GetTowConnection(int index, out vector position, out vector size)
	{
#ifdef EXPANSIONTRACE
		auto trace = CF_Trace_1(ExpansionTracing.VEHICLES, this, "Expansion_GetTowConnection").Add(index);
#endif

		vector minMax[2];
		GetCollisionBox(minMax);

		if (index == 0)
		{
			position = Vector(0.0, -minMax[0][1] * 0.15, minMax[1][2] + Expansion_GetTowLength());
		}

		if (index == 1)
		{
			position = Vector(0.0, -minMax[0][1] * 0.15, minMax[0][2] - Expansion_GetTowLength());
		}

		position[0] = dBodyGetCenterOfMass(this)[0];
		position[2] = position[2] - dBodyGetCenterOfMass(this)[2];

		size = "0.5 0.5 0.5";
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
		return Vector(0.0, minMax[0][1] * 0.15, minMax[0][2] - dBodyGetCenterOfMass(this)[2]);
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

	void Expansion_DBGTowing()
	{
		vector transform[4];
		GetTransform(transform);

		for (int i = 0; i < Expansion_NumberTowConnections(); i++)
		{
			vector position;
			vector size;
			Expansion_GetTowConnection(i, position, size);

			vector minMax[2];

			minMax[0] = position - size;
			minMax[1] = position + size;

			//DGBDrawBoundingBox(transform, minMax);
		}

		//DBGDrawSphere(ModelToWorld(Expansion_GetTowPosition()), Expansion_GetTowLength());
	}
};
