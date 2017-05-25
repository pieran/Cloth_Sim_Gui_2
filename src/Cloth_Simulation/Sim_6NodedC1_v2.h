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

#define USE_DYNAMIC_MINMAX FALSE


enum Sim_6NodedC1_v2_SubTimer
{
	Sim_6NodedC1_v2_SubTimer_Rotations = 0,
	Sim_6NodedC1_v2_SubTimer_BuildMatrices,
	Sim_6NodedC1_v2_SubTimer_Solver,
	Sim_6NodedC1_v2_SubTimer_MAX
};

typedef Eigen::Matrix<float, 12, 12> StiffnessMatrix;

typedef Eigen::Matrix<float, 3, 18> BMatrix;
typedef Eigen::Matrix<float, 2, 2>  JaMatrix;
typedef Eigen::Matrix<float, 6, 18> GMatrix;
typedef Eigen::Matrix<float, 2, 6> Mat26;
typedef Eigen::Matrix<float, 2, 15> BaseMatrix;
typedef Eigen::Matrix<float, 18, 1> VDisplacement;
typedef Eigen::Matrix<float, 3, 45> BMatrix_C1;


class Sim_6NodedC1_v2 : public Sim_Simulation, Sim_Rendererable, Sim_Integratable
{
	friend class ClothRenderObject;
	friend class MyScene;
public:
	EIGEN_MAKE_ALIGNED_OPERATOR_NEW

	Sim_6NodedC1_v2();
	~Sim_6NodedC1_v2();

	//Simulation
	virtual void Initialize(const Sim_Generator_Output& configuration) override;
	virtual MPCG<SparseRowMatrix<Matrix3>>*  Solver() override { return &m_Solver; }

	virtual bool GetIsStatic(uint idx) override { return (idx < m_PhyxelIsStatic.size()) ? m_PhyxelIsStatic[idx] : false; }
	virtual void SetIsStatic(uint idx, bool is_static) override
	{
		m_PhyxelIsStatic[idx] = is_static;
		UpdateConstraints();
	}

	virtual ProfilingTimer& GetTotalTimer() override { return m_ProfilingTotalTime; }

	virtual int GetNumSubProfilers() override { return Sim_6NodedC1_v2_SubTimer_MAX; }
	virtual const ProfilingTimer& GetSubProfiler(int idx) override { return m_ProfilingSubTimers[idx]; }


	//Integratable
	virtual bool StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt) override;

	void UpdateConstraints();
	bool ValidateVelocityTimestep();

	//Rendering
	virtual int GetNumTris() override {
		return m_NumTriangles;
	}

	virtual void GetVertexWsPos(int triidx, const Vector3& gauss_point, const Vector3* positions, Vector3& out_pos) override;
	virtual void GetVertexRotation(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation) override;
	virtual void GetVertexStressStrain(int triidx, const Vector3& gauss_point, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain) override;

protected:
	void SimpleCorotatedBuildAMatrix(float dt, const Vector3* positions, const Vector3* velocities);

	void InitGaussWeights();

	void BuildNaturalCoordinateBasisVectors(const FETriangle& tri, const Vector3& gaussPoint, BaseMatrix& out_dn);
	void CalcRotationC1(const FETriangle& tri, const Vector3* pos, const Vector3* tans, const Vector3& gaussPoint, float warp_angle, Eigen::Matrix3f& out_rot, JaMatrix& out_ja, BaseMatrix& out_dn);


	void CalcBMatrix(const FETriangle& tri, const Vector3* pos, const Vector3* tans, const Vec3& gaussPoint, const float warp_angle, const Eigen::Matrix<float, 45, 1>& displacements, BMatrix_C1& out_b, JaMatrix& out_ja, Eigen::Matrix<float, 6, 45>& out_g);
protected:

	Eigen::Matrix<float, 12, 3> GaussPoint12;
	Eigen::Matrix<float, 12, 1> GaussWeight12;

	Mat33 E; //Elasticity Matrix!!

	uint m_NumTotal;
	uint m_NumPhyxels, m_NumTangents;
	uint m_NumTriangles;

	float m_TotalArea;

	//Phyxel Data
	std::vector<Vector3>		m_PhyxelsPosInitial;
	std::vector<Vector3>        m_PhyxelsTangentInitial;

	std::vector<Vector3>		m_PhyxelForces;

	std::vector<Vector2>		m_PhyxelTexCoords;
	std::vector<bool>			m_PhyxelIsStatic;
	std::vector<float>			m_PhyxelsMass;

	//Structural Data
	std::vector<FETriangle> m_Triangles;

	MPCG<SparseRowMatrix<Matrix3>> m_Solver;	//Solver

	float angle = 0.0f;


	//Profiling
	ProfilingTimer	  m_ProfilingTotalTime;
	ProfilingTimer    m_ProfilingSubTimers[Sim_6NodedC1_v2_SubTimer_MAX];
};
