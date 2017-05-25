#include "Sim_6NodedC0.h"
#include <glcore\NCLDebug.h>




Sim_6NodedC0::Sim_6NodedC0() : Sim_Rendererable()
{
	InitGaussWeights();

	m_ProfilingTotalTime.SetAlias("Total Time");
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Rotations].SetAlias("Geb Rotation");
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_BuildMatrices].SetAlias("Gen Matrix");
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Solver].SetAlias("Solver");

	const float Y = 2500.0f;	//Youngs Modulus
	const float v = 0.3f;		//Poisson coefficient
	E.setZero();
	E(0, 0) = 1.0f;
	E(1, 0) = v;
	E(0, 1) = v;
	E(1, 1) = 1.0f;
	E(2, 2) = (1.0f - v) * 0.5f;
	E *= Y / (1.0f - v * v);
}

Sim_6NodedC0::~Sim_6NodedC0()
{
}

void Sim_6NodedC0::Initialize(const Sim_Generator_Output& config)
{
	m_SumPreviousStrain = 0.f;

	//Gen Vertices
	m_NumPhyxels = config.NumVertices;

	m_Solver.AllocateMemory(m_NumPhyxels);
	m_Solver.m_A.resize(m_NumPhyxels);

	

	m_PhyxelsPosInitial.resize(m_NumPhyxels);
	m_PhyxelForces.resize(m_NumPhyxels);

	m_PhyxelNormals.resize(m_NumPhyxels);
	m_PhyxelIsStatic.resize(m_NumPhyxels);
	m_PhyxelsMass.resize(m_NumPhyxels);
	m_PhyxelTexCoords.resize(m_NumPhyxels);

	memset(&m_PhyxelNormals[0].x, 0, m_NumPhyxels * sizeof(Vector3));
	memcpy(&m_PhyxelsPosInitial[0], &config.Phyxels_Initial[0], m_NumPhyxels * sizeof(Vector3));

	for (unsigned int i = 0; i < m_NumPhyxels; ++i)
	{
		const FEVertDescriptor& v = config.Phyxel_Descriptors[i];

		m_PhyxelIsStatic[i] = v.isStatic;

		if (!m_PhyxelIsStatic[i])
		{
			m_PhyxelForces[i] = Vector3(0.f, 0.f, 0.f);// GRAVITY / float(m_NumPhyxels);
		}


		m_PhyxelTexCoords[i] = v.tCoord;
	}



	//Populate Triangles & Add Additional Mid-Points
	m_NumTriangles = config.Triangles.size();
	m_TriangleStiffness.resize(m_NumTriangles);
	m_Triangles.resize(m_NumTriangles);

	float totalArea = 0.0f;
	memcpy(&m_Triangles[0], &config.Triangles[0], m_NumTriangles * sizeof(FETriangle));

	Matrix3 tmp_rot;
	m_TriMaterialDirections.resize(m_NumTriangles);
	
	for (unsigned int i = 0; i < m_NumTriangles; ++i)
	{
		m_TriMaterialDirections[i] = Mat33::Identity();

		unsigned int i3 = i * 3;
	
		Vector3 a = m_PhyxelsPosInitial[m_Triangles[i].v1];
		Vector3 b = m_PhyxelsPosInitial[m_Triangles[i].v2];
		Vector3 c = m_PhyxelsPosInitial[m_Triangles[i].v3];

		Vector3 e1 = a - c;
		Vector3 e2 = b - c;

		totalArea += 0.5f * abs(e1.x * e2.y - e1.y * e2.x);
	}

	float uniform_mass = (totalArea * mass_density) / m_NumPhyxels;
	for (unsigned int i = 0; i < m_NumPhyxels; ++i)
	{
		m_PhyxelsMass[i] = uniform_mass;
	}

	for (unsigned int i = 0; i < m_NumTriangles; ++i)
	{
		this->GetVertexRotation(i, Vector3(1.f, 0.f, 0.f), &config.Phyxels[0], config.Phyxels[m_Triangles[i].v1], tmp_rot);

		memcpy(&m_TriMaterialDirections[i], &tmp_rot, sizeof(Matrix3));
		m_TriMaterialDirections[i].transposeInPlace();
	}


	//Build Global Matricies (To be deleted at the start of the simulation)
	SimpleCorotatedBuildAMatrix(0.0f, &config.Phyxels[0], NULL);
	m_Solver.m_A.zero_memory();
	m_Solver.ResetMemory();
	m_Solver.m_A.zero_memory();
	UpdateConstraints();
}

void  Sim_6NodedC0::UpdateConstraints()
{
#pragma omp parallel for
	for (int i = 0; i < (int)m_NumPhyxels; ++i)
	{
		if (m_PhyxelIsStatic[i])
		{
			m_Solver.m_Constraints[i] = Matrix3::ZeroMatrix;
		}
		else
		{
			m_Solver.m_Constraints[i] = Matrix3::Identity;
		}
	}
}


bool Sim_6NodedC0::StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt)
{
	m_ProfilingTotalTime.BeginTiming();
	for (int i = 0; i < Sim_6Noded_SubTimer_MAX; ++i)
	{
		m_ProfilingSubTimers[i].ResetTotalMs();
	}
	m_Solver.ResetProfilingData();

	Vector3 sub_grav = gravity / m_NumPhyxels;
#pragma omp parallel for
	for (int i = 0; i < (int)m_NumPhyxels; ++i)
	{
		m_PhyxelForces[i] = sub_grav;
	}

	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Rotations].BeginTiming();
	//	BuildRotationAndNormals();
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Rotations].EndTimingAdditive();

	

	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Solver].BeginTiming();
	m_Solver.ResetMemory();
	m_Solver.m_A.zero_memory();
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Solver].EndTimingAdditive();

	m_ProfilingSubTimers[Sim_6Noded_SubTimer_BuildMatrices].BeginTiming();
	SimpleCorotatedBuildAMatrix(dt, in_x, in_dxdt);
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_BuildMatrices].EndTimingAdditive();

	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Solver].BeginTiming();
	//m_Solver.SolveWithPreviousResult();
	m_Solver.SolveWithGuess(in_dxdt);
	m_ProfilingSubTimers[Sim_6Noded_SubTimer_Solver].EndTimingAdditive();


#pragma omp parallel for
	for (int i = 0; i < (int)m_NumPhyxels; ++i)
	{
		out_dxdt[i] = m_Solver.m_X[i];
	}

	//Reset All Accumulative Data & Update Positions	
	m_ProfilingTotalTime.EndTimingAdditive();

	bool valid_timestep = true;
	return valid_timestep;
}

bool Sim_6NodedC0::ValidateVelocityTimestep(const Vector3* pos_tmp)
{
	BMatrix B_nl;
	JaMatrix Ja;
	GMatrix G;
	VDisplacement d_g;

	float sum_stress = 0.f;
	for (uint i = 0; i < m_NumTriangles; ++i)
	{
		FETriangle& tri = m_Triangles[i];

		for (int j = 0; j < 6; ++j)
		{
			d_g(j * 3 + 0, 0) = pos_tmp[tri.phyxels[j]].x - m_PhyxelsPosInitial[tri.phyxels[j]].x;
			d_g(j * 3 + 1, 0) = pos_tmp[tri.phyxels[j]].y - m_PhyxelsPosInitial[tri.phyxels[j]].y;
			d_g(j * 3 + 2, 0) = pos_tmp[tri.phyxels[j]].z - m_PhyxelsPosInitial[tri.phyxels[j]].z;
		}

		for (uint j = 0; j < 12; ++j)
		{
			CalcBMatrix(tri, &m_PhyxelsPosInitial[0], GaussPoint12.row(j), 0.0f, d_g, B_nl, Ja, G);

			Vec3 strain = B_nl * d_g;
			sum_stress += strain.norm();
		}
	}

	printf("Stress New: %f\n", sum_stress);
	bool valid = (fabs(m_SumPreviousStrain - sum_stress) < 0.5f);
	m_SumPreviousStrain = sum_stress;
	return valid;
}

void Sim_6NodedC0::InitGaussWeights()
{

	//Define Gauss points for numerical intergration
	GaussPoint12(0, 0) = 0.873821971016996;
	GaussPoint12(0, 1) = 0.063089014491502;
	GaussPoint12(0, 2) = 0.063089014491502;
	GaussPoint12(1, 0) = 0.063089014491502;
	GaussPoint12(1, 1) = 0.063089014491502;
	GaussPoint12(1, 2) = 0.873821971016996;
	GaussPoint12(2, 0) = 0.063089014491502;
	GaussPoint12(2, 1) = 0.873821971016996;
	GaussPoint12(2, 2) = 0.063089014491502;

	for (int i = 0; i < 3; ++i)
		GaussWeight12(i) = 0.050844906370207;

	GaussPoint12(3, 0) = 0.501426509658179;
	GaussPoint12(3, 1) = 0.249286745170910;
	GaussPoint12(3, 2) = 0.249286745170910;
	GaussPoint12(4, 0) = 0.249286745170910;
	GaussPoint12(4, 1) = 0.249286745170910;
	GaussPoint12(4, 2) = 0.501426509658179;
	GaussPoint12(5, 0) = 0.249286745170910;
	GaussPoint12(5, 1) = 0.501426509658179;
	GaussPoint12(5, 2) = 0.249286745170910;

	for (int i = 3; i < 6; ++i)
		GaussWeight12(i) = 0.116786275726379;


	GaussPoint12(6, 0) = 0.636502499121399;
	GaussPoint12(6, 1) = 0.310352451033784;
	GaussPoint12(6, 2) = 0.053145049844817;
	GaussPoint12(7, 0) = 0.636502499121399;
	GaussPoint12(7, 1) = 0.053145049844817;
	GaussPoint12(7, 2) = 0.310352451033784;
	GaussPoint12(8, 0) = 0.310352451033784;
	GaussPoint12(8, 1) = 0.636502499121399;
	GaussPoint12(8, 2) = 0.053145049844817;

	GaussPoint12(9, 0) = 0.310352451033784;
	GaussPoint12(9, 1) = 0.053145049844817;
	GaussPoint12(9, 2) = 0.636502499121399;
	GaussPoint12(10, 0) = 0.053145049844817;
	GaussPoint12(10, 1) = 0.310352451033784;
	GaussPoint12(10, 2) = 0.636502499121399;
	GaussPoint12(11, 0) = 0.053145049844817;
	GaussPoint12(11, 1) = 0.636502499121399;
	GaussPoint12(11, 2) = 0.310352451033784;

	for (int i = 6; i < 12; ++i)
		GaussWeight12(i) = 0.082851075618374;

}

void Sim_6NodedC0::BuildTransformationMatrix(const FETriangle& tri, const Vector3* pos, const Vec3& gaussPoint, const float warp_angle, Mat33& out_t, JaMatrix& out_ja, Mat26& out_dn)
{
	//Build local fill direction (local warp direction)
	Vec3 vFill(-sin(warp_angle), cos(warp_angle), 0.0f);

	//Build natural coordinate basis vectors
	out_dn(0, 0) = 4 * gaussPoint.x() - 1.f;
	out_dn(0, 1) = 0.0f;
	out_dn(0, 2) = -4 * gaussPoint.z() + 1.f;
	out_dn(0, 3) = 4 * gaussPoint.y();
	out_dn(0, 4) = -4 * gaussPoint.y();
	out_dn(0, 5) = 4 * (gaussPoint.z() - gaussPoint.x());

	out_dn(1, 0) = 0.f;
	out_dn(1, 1) = 4 * gaussPoint.y() - 1.f;
	out_dn(1, 2) = -4 * gaussPoint.z() + 1.f;
	out_dn(1, 3) = 4 * gaussPoint.x();
	out_dn(1, 4) = 4 * (gaussPoint.z() - gaussPoint.y());
	out_dn(1, 5) = -4 * gaussPoint.x();


	Vec3 V_xi(0, 0, 0), V_eta(0, 0, 0);
	Vector3 tmp;
	for (int i = 0; i < 6; ++i)
	{
		tmp = pos[tri.phyxels[i]] * out_dn(0, i);
		V_xi += Vec3(tmp.x, tmp.y, tmp.z);

		tmp = pos[tri.phyxels[i]] * out_dn(1, i);
		V_eta += Vec3(tmp.x, tmp.y, tmp.z);
	}

	Vec3 V_z = V_xi.cross(V_eta);
	V_z.normalize();

	//if (V_z.z() < 0.0f)
	//{
	//	V_z = V_eta.cross(V_xi);
	//	V_z.normalize();
	//}



	Vec3 V_tx = V_eta.cross(V_z);
	Vec3 V_ty = V_xi.cross(V_z);

	Vec3 avg = (V_tx + V_ty) * 0.5;

	Vec3 V_x = V_eta.cross(V_z);
	V_x.normalize();

	Vec3 V_y = V_z.cross(V_x);
	V_y.normalize();

	//Vec3 V_x = V_eta.cross(V_z);
	//V_x.normalize();
	//Vec3 V_y = V_z.cross(V_x);
	//V_y.normalize();





	//Build the transformation matrix
	out_t(0, 0) = V_x(0); out_t(0, 1) = V_x(1); out_t(0, 2) = V_x(2);
	out_t(1, 0) = V_y(0); out_t(1, 1) = V_y(1); out_t(1, 2) = V_y(2);
	out_t(2, 0) = V_z(0); out_t(2, 1) = V_z(1); out_t(2, 2) = V_z(2);


	//Build the Jacobian Matrix
	out_ja(0, 0) = V_xi.dot(V_x);
	out_ja(0, 1) = V_xi.dot(V_y);
	out_ja(1, 0) = V_eta.dot(V_x);
	out_ja(1, 1) = V_eta.dot(V_y);
}


void Sim_6NodedC0::CalcBMatrix(const FETriangle& tri, const Vector3* pos, const Vec3& gaussPoint, const float warp_angle, const VDisplacement& displacements, BMatrix& out_b, JaMatrix& out_ja, GMatrix& out_g)
{

	Mat26 DN;
	Mat33 T;
	BMatrix B_l, B_nl;

	BuildTransformationMatrix(tri, pos, gaussPoint, warp_angle, T, out_ja, DN);

	Mat22 Ja_inv = out_ja.inverse();

	Eigen::Matrix<float, 2, 6> a = Ja_inv * DN; //34%



	for (int i = 0; i < 6; ++i)
	{
		B_l(0, i * 3) = T(0, 0) * a(0, i);
		B_l(0, i * 3 + 1) = T(0, 1) * a(0, i);
		B_l(0, i * 3 + 2) = T(0, 2) * a(0, i);

		B_l(1, i * 3) = T(1, 0) * a(1, i);
		B_l(1, i * 3 + 1) = T(1, 1) * a(1, i);
		B_l(1, i * 3 + 2) = T(1, 2) * a(1, i);

		B_l(2, i * 3) = T(0, 0) * a(1, i) + T(1, 0) * a(0, i);
		B_l(2, i * 3 + 1) = T(0, 1) * a(1, i) + T(1, 1) * a(0, i);
		B_l(2, i * 3 + 2) = T(0, 2) * a(1, i) + T(1, 2) * a(0, i);
	}

	//Build G Matrix
	for (int n = 0; n < 6; ++n)
	{
		for (int i = 0; i < 3; ++i)
		{
			out_g(i, n * 3) = T(i, 0) * a(0, n);
			out_g(i, n * 3 + 1) = T(i, 1) * a(0, n);
			out_g(i, n * 3 + 2) = T(i, 2) * a(0, n);
		}

		for (int i = 3; i < 6; ++i)
		{
			out_g(i, n * 3) = T(i - 3, 0) * a(1, n);
			out_g(i, n * 3 + 1) = T(i - 3, 1) * a(1, n);
			out_g(i, n * 3 + 2) = T(i - 3, 2) * a(1, n);
		}
	}

	//Find displacement dependant terms of local Bmatrix
	Eigen::Matrix<float, 1, 6> delta = out_g * displacements;

	Eigen::Matrix<float, 3, 6> derivatives;
	derivatives.setZero();
	derivatives(0, 0) = delta[0];
	derivatives(0, 1) = delta[1];
	derivatives(0, 2) = delta[2];
	derivatives(1, 3) = delta[3];
	derivatives(1, 4) = delta[4];
	derivatives(1, 5) = delta[5];

	derivatives(2, 0) = delta[3];
	derivatives(2, 1) = delta[4];
	derivatives(2, 2) = delta[5];
	derivatives(2, 3) = delta[0];
	derivatives(2, 4) = delta[1];
	derivatives(2, 5) = delta[2];

	B_nl = 0.5 * derivatives * out_g;

	out_b = B_l + B_nl;
}

void Sim_6NodedC0::SimpleCorotatedBuildAMatrix(float dt, const Vector3* positions, const Vector3* velocities)
{
	const float dt2 = dt * dt;
	const float dt2_viscos = dt2 + V_SCALAR * dt;

	if (velocities == NULL)
	{
#pragma omp parallel for
		for (int i = 0; i < (int)m_NumPhyxels; ++i)
		{
			m_Solver.m_A(i, i) = Matrix3::Identity * m_PhyxelsMass[i];
			m_Solver.m_B[i] = m_PhyxelForces[i] * dt;
		}
	}
	else
	{
#pragma omp parallel for
		for (int i = 0; i < (int)m_NumPhyxels; ++i)
		{
			m_Solver.m_A(i, i) = Matrix3::Identity * m_PhyxelsMass[i];
			m_Solver.m_B[i] = velocities[i] * m_PhyxelsMass[i] + m_PhyxelForces[i] * dt;
		}
	}


	Matrix3 RK, RKR, ReT;
	Vector3 posAB, posIAB;




	BMatrix B_nl, B_0;
	JaMatrix Ja;
	GMatrix G;

	VDisplacement d_g, d_0, force;
	d_0.setZero();

	Eigen::Matrix<float, 18, 18> K_E, K_S, K_T;
	Eigen::Matrix<float, 6, 6> M; M.setZero();

	for (uint i = 0; i < m_NumTriangles; ++i)
	{
		FETriangle& tri = m_Triangles[i];

		K_E.setZero();
		K_S.setZero();

		for (int j = 0; j < 6; ++j)
		{
			d_g(j * 3 + 0, 0) = positions[tri.phyxels[j]].x - m_PhyxelsPosInitial[tri.phyxels[j]].x;
			d_g(j * 3 + 1, 0) = positions[tri.phyxels[j]].y - m_PhyxelsPosInitial[tri.phyxels[j]].y;
			d_g(j * 3 + 2, 0) = positions[tri.phyxels[j]].z - m_PhyxelsPosInitial[tri.phyxels[j]].z;
		}

		force.setZero();
		for (uint j = 0; j < 12; ++j)
		{
			CalcBMatrix(tri, &m_PhyxelsPosInitial[0], GaussPoint12.row(j), 0.0f, d_g, B_nl, Ja, G);

			Vec3 strain = B_nl * d_g;
			Vec3 stress = ( E) * strain;

			CalcBMatrix(tri, positions, GaussPoint12.row(j), 0.0f, d_0, B_0, Ja, G);

			float area = Ja.determinant() * 0.5f;
			if (area < 0)
			{
				printf("ERROR:: Element %d:%d has a negative area!!!\n", i, j);
			}

			float tfactor = GaussWeight12(j) * area;


			M(0, 0) = stress.x();
			M(1, 1) = stress.x();
			M(2, 2) = stress.x();

			M(0, 3) = stress.z();
			M(1, 4) = stress.z();
			M(2, 5) = stress.z();
			M(3, 0) = stress.z();
			M(4, 1) = stress.z();
			M(5, 2) = stress.z();

			M(3, 3) = stress.y();
			M(4, 4) = stress.y();
			M(5, 5) = stress.y();



			K_E += B_0.transpose() * E * B_nl * tfactor;
			K_S += G.transpose() * M * G * tfactor;

			force += B_0.transpose() * stress * tfactor;
		}

		K_T = K_E + K_S;

		//Convert Eigen Matrices back to global A Matrix + B Vectors for global solver
		for (uint j = 0; j < 6; ++j)
		{
			m_Solver.m_B[tri.phyxels[j]] -= Vector3(force(j * 3), force(j * 3 + 1), force(j * 3 + 2)) * dt;

			for (uint k = 0; k < 6; ++k)
			{
				uint idxJ = tri.phyxels[j];
				uint idxK = tri.phyxels[k];
				Matrix3 submtx;

				submtx._11 = K_T(j * 3 + 0, k * 3 + 0);
				submtx._12 = K_T(j * 3 + 0, k * 3 + 1);
				submtx._13 = K_T(j * 3 + 0, k * 3 + 2);
				submtx._21 = K_T(j * 3 + 1, k * 3 + 0);
				submtx._22 = K_T(j * 3 + 1, k * 3 + 1);
				submtx._23 = K_T(j * 3 + 1, k * 3 + 2);
				submtx._31 = K_T(j * 3 + 2, k * 3 + 0);
				submtx._32 = K_T(j * 3 + 2, k * 3 + 1);
				submtx._33 = K_T(j * 3 + 2, k * 3 + 2);

				m_Solver.m_A(idxJ, idxK) += submtx * dt * dt;
			}
		}

	}
}




void Sim_6NodedC0::GetVertexWsPos(int triidx, const Vector3& gp, const Vector3* positions, Vector3& out_pos)
{
	const auto& t = m_Triangles[triidx];

	Vector3 gp2 = gp * gp;
	Vector3 gp3 = gp * gp * gp;
	Vector3 gp4 = gp2 * gp2;
	Vector3 gp5 = gp3 * gp2;

	float gxy = gp.x * gp.y;
	float gx2y = gp2.x * gp.y;
	float gxy2 = gp.x * gp2.y;
	float gx3y = gp3.x * gp.y;
	float gx2y2 = gp2.x * gp2.y;
	float gxy3 = gp.x * gp3.y;

	float FormFunction[]{
		gp.x * (2 * gp.x - 1.f),	//PA
		gp.y * (2 * gp.y - 1.f),	//PB
		gp.z * (2 * gp.z - 1.f),	//PC
		4 * gp.x * gp.y,			//PAB
		4 * gp.y * gp.z,			//PBC
		4 * gp.x * gp.z,			//PAC
	};

	Vector3 ws_pos = Vector3(0.f, 0.f, 0.f);
	for (int i = 0; i < 6; ++i)
	{
		ws_pos += positions[t.phyxels[i]] * FormFunction[i];
	}

	out_pos = ws_pos;
}

void Sim_6NodedC0::GetVertexRotation(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation)
{
	Mat26 Dn;
	Eigen::Matrix3f rot;
	JaMatrix Ja;
	const auto& t = m_Triangles[triidx];
	BuildTransformationMatrix(t, positions, Eigen::Vector3f(gp.x, gp.y, gp.z), 0.f, rot, Ja, Dn);
	//rot = m_TriMaterialDirections[triidx] * rot;
	memcpy(&out_rotation._11, &rot, sizeof(Matrix3));
}

void Sim_6NodedC0::GetVertexStressStrain(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain)
{
	const auto& t = m_Triangles[triidx];

	Eigen::Matrix<float, 18, 1> d_g;
	BMatrix B_nl;
	JaMatrix Ja;
	Eigen::Matrix<float, 6, 18> G;

	for (int j = 0; j < 6; ++j)
	{
		Vector3 pos = (positions[t.phyxels[j]] - m_PhyxelsPosInitial[t.phyxels[j]]);

		d_g(j * 3 + 0, 0) = pos.x; d_g(j * 3 + 1, 0) = pos.y; d_g(j * 3 + 2, 0) = pos.z;
	}

	CalcBMatrix(t, &m_PhyxelsPosInitial[0], Vec3(gp.x, gp.y, gp.z), 0.0f, d_g, B_nl, Ja, G);

	Vec3 strain = B_nl * d_g;
	Vec3 stress = E * strain;

	memcpy(&out_stress.x, &stress, sizeof(Vector3));
	memcpy(&out_strain.x, &strain, sizeof(Vector3));
}