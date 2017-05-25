#pragma once


#include "mpcg.h"

#include "EigenDefines.h"
#include "SimulationDefines.h"

#include <glcore\Matrix4.h>
#include <glcore\Matrix3.h>
#include <glcore\Vector3.h>
#include <glcore\Vector2.h>
#include "ProfilingTimer.h"

#include "Sim_Renderer.h"
#include "Sim_Integrator.h"
#include "Sim_Manager.h"

struct Sim_3Noded_Triangle
{
	Sim_3Noded_Triangle() : v1(0), v2(0), v3(0) {}
	Sim_3Noded_Triangle(uint a, uint b, uint c) : v1(a), v2(b), v3(c) {}

	union
	{
		struct
		{
			uint v1, v2, v3;
		};
		uint verts[3];
	};
};

struct Sim_PBD_DConstraint
{
	Sim_PBD_DConstraint(uint a, uint b, float _k) : c1(a), c2(b), k(_k), kPrime(0.f), length(0.f) {}

	uint c1, c2;
	float k, kPrime, length;
};

struct Sim_PBD_BConstraint
{
	Sim_PBD_BConstraint(uint a, uint b, uint c, float _k, float _w) : c1(a), c2(b), c3(c), k(_k), w(_w), kPrime(0.f), length(0.f) {}

	uint c1, c2, c3;
	float k, kPrime, length, w;
};

enum Sim_PBD_SubTimer
{
	Sim_PBD3Noded_SubTimer_Solver,
	Sim_PBD3Noded_SubTimer_MAX
};

class Sim_PBD : public Sim_Simulation, Sim_Rendererable, Sim_Integratable
{
	friend class ClothRenderObject;
	friend class MyScene;
public:
	Sim_PBD();
	~Sim_PBD();

	//Simulation
	virtual void Initialize(const Sim_Generator_Output& configuration);
	virtual MPCG<SparseRowMatrix<Matrix3>>*  Solver() override { return NULL; }

	virtual bool GetIsStatic(uint idx) { return (idx < m_PhyxelIsStatic.size()) ? m_PhyxelIsStatic[idx] : false; }
	virtual void SetIsStatic(uint idx, bool is_static)
	{
		m_PhyxelIsStatic[idx] = is_static;
	}

	virtual ProfilingTimer& GetTotalTimer() { return m_ProfilingTotalTime; }

	virtual int GetNumSubProfilers() { return Sim_PBD3Noded_SubTimer_MAX; }
	virtual const ProfilingTimer& GetSubProfiler(int idx) { return m_ProfilingSubTimers[idx]; }


	//Intergratable
	virtual bool StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt);

	void UpdateConstraints();
	bool ValidateVelocityTimestep(const Vector3* pos_tmp);


	//Renderable
	virtual int GetNumTris() {
		return m_NumTriangles;
	}
	virtual void GetVertexWsPos(int triidx, const Vector3& gauss_point, const Vector3* positions, Vector3& out_pos);
	virtual void GetVertexRotation(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation);
	virtual void GetVertexStressStrain(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain);




protected:
	
	void SetupConstraints(const Sim_Generator_Output& configuration);
	void InitDConstraint(const Vector3* positions, uint v1, uint v2, float k);
	void InitBConstraint(const Vector3* positions, uint v1, uint v2, uint v3, float k);

	void ComputePhyxelRotations(const Vector3* positions);

	void SolveDistanceConstraint(const Sim_PBD_DConstraint& constraint);
	void SolveBendingConstraint(const Sim_PBD_BConstraint& constraint);
protected:
	uint m_NumPhyxels, m_NumTriangles;
	float m_TotalArea;

	//Phyxel Data
	std::vector<Vector3>        m_PhyxelPosTmp;
	std::vector<Vector3>		m_PhyxelForces;
	std::vector<Vector2>		m_PhyxelTexCoords;
	std::vector<bool>			m_PhyxelIsStatic;
	std::vector<float>			m_PhyxelsInvMass;
	std::vector<Matrix3>        m_PhyxelRotations;

	std::vector<Sim_PBD_DConstraint>  m_Constraints_Distance;
	std::vector<Sim_PBD_BConstraint>  m_Constraints_Bending;

	//Structural Data
	std::vector<Sim_3Noded_Triangle> m_Triangles;
	std::vector<Matrix3> m_TriangleRotations;
	std::vector<Matrix3> m_TriangleRotationsInitial;

	int m_SolverSteps = 20;
	bool m_Rotations_Invalidated = true;

	//Profiling
	ProfilingTimer	  m_ProfilingTotalTime;
	ProfilingTimer    m_ProfilingSubTimers[Sim_PBD3Noded_SubTimer_MAX];
};