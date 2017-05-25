#include "Sim_PBD.h"
#include <glcore\NCLDebug.h>
#include <algorithm>
#include <random>



Sim_PBD::Sim_PBD() : Sim_Rendererable()
{
	m_ProfilingTotalTime.SetAlias("Total Time");
	m_ProfilingSubTimers[Sim_PBD3Noded_SubTimer_Solver].SetAlias("Solver");

}

Sim_PBD::~Sim_PBD()
{
}

void Sim_PBD::Initialize(const Sim_Generator_Output& config)
{
	//Gen Vertices
	m_NumPhyxels = config.NumVertices;

	m_PhyxelForces.resize(m_NumPhyxels);
	m_PhyxelPosTmp.resize(m_NumPhyxels);
	m_PhyxelRotations.resize(m_NumPhyxels);
	m_PhyxelIsStatic.resize(m_NumPhyxels);
	m_PhyxelsInvMass.resize(m_NumPhyxels);
	m_PhyxelTexCoords.resize(m_NumPhyxels);

	memset(&m_PhyxelRotations[0], 0, m_NumPhyxels * sizeof(Matrix3));

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



	SetupConstraints(config);

	m_TriangleRotationsInitial.resize(m_NumTriangles);
#pragma omp parallel for
	for (int i = 0; i < m_Triangles.size(); ++i)
	{
		Vector3 r[3];
		r[0] = config.Phyxels_Initial[m_Triangles[i].v3] - config.Phyxels_Initial[m_Triangles[i].v1];
		r[1] = config.Phyxels_Initial[m_Triangles[i].v3] - config.Phyxels_Initial[m_Triangles[i].v2];
		r[2] = Vector3::Cross(r[1], r[0]);

		m_TriangleRotationsInitial[i] = Matrix3::OrthoNormalise(r, 2);
	}


	double totalArea = 0.0;
	for (unsigned int i = 0; i < m_NumTriangles; ++i)
	{
		unsigned int i3 = i * 3;

		Vector3 a = config.Phyxels[m_Triangles[i].v1];
		Vector3 b = config.Phyxels[m_Triangles[i].v2];
		Vector3 c = config.Phyxels[m_Triangles[i].v3];

		Vector3 e1 = a - c;
		Vector3 e2 = b - c;

		totalArea += 0.5f * abs(e1.x * e2.y - e1.y * e2.x);
	}

	float uniform_mass = (float)m_NumPhyxels / (float)(totalArea * mass_density);
	for (unsigned int i = 0; i < m_NumPhyxels; ++i)
	{
		m_PhyxelsInvMass[i] = uniform_mass;
	}

}

void Sim_PBD::SetupConstraints(const Sim_Generator_Output& config)
{
	const float k_weft = 0.5f;
	const float k_warp = 0.5f;
	const float k_shear = 0.5f;
	const float k_damp = 0.25f;
	const float k_weft_bend = 0.125f;
	const float k_warp_bend = 0.125f;


	uint numWidth = (uint)sqrt(config.NumVertices);
	m_NumTriangles = (numWidth - 1) * (numWidth - 1) * 2;
	m_Triangles.clear();
	m_Constraints_Distance.clear();
	m_Constraints_Bending.clear();

	for (uint x = 0; x < numWidth - 1; ++x)
	{
		for (uint y = 0; y < numWidth - 1; ++y)
		{
			uint a = ((y) * numWidth) + (x);
			uint b = ((y)* numWidth) + (x + 1);
			uint c = ((y + 1)* numWidth) + (x);
			uint d = ((y + 1)* numWidth) + (x + 1);

			m_Triangles.push_back(Sim_3Noded_Triangle(c, a, b));
			m_Triangles.push_back(Sim_3Noded_Triangle(c, b, d));

			InitDConstraint(
				&config.Phyxels[0],
				a,
				d,
				k_shear);

			InitDConstraint(
				&config.Phyxels[0],
				b,
				c,
				k_shear);
		}
	}

	for (uint x = 0; x < numWidth; ++x)
		for (uint y = 0; y < numWidth - 1; ++y)
		{
			uint a = ((y)* numWidth) + (x);
			uint b = ((y + 1)* numWidth) + (x);

			InitDConstraint(
				&config.Phyxels[0],
				(y * numWidth) + x,
				(y * numWidth) + x + 1,
				k_warp);
		}

	for (uint x = 0; x < numWidth - 1; ++x)
		for (uint y = 0; y < numWidth; ++y)
		{
			InitDConstraint(
				&config.Phyxels[0],
				(y * numWidth) + x,
				(y * numWidth) + x + 1,
				k_weft);
		}

	//BENDING
	for (uint x = 0; x < numWidth - 2; ++x)
		for (uint y = 0; y < numWidth; ++y)
		{
			InitBConstraint(
				&config.Phyxels[0],
				(y * numWidth) + x,
				(y * numWidth) + x + 1,
				(y * numWidth) + x + 2,
				k_weft_bend);
		}
	for (uint x = 0; x < numWidth; ++x)
		for (uint y = 0; y < numWidth - 2; ++y)
		{
			InitBConstraint(
				&config.Phyxels[0],
				(y * numWidth) + x,
				((y + 1) * numWidth) + x,
				((y + 2) * numWidth) + x,
				k_warp_bend);
		}
}

void Sim_PBD::InitDConstraint(const Vector3* positions, uint v1, uint v2, float k)
{
	Sim_PBD_DConstraint c = Sim_PBD_DConstraint(v1, v2, k);
	c.length = (positions[v2] - positions[v1]).Length();

	c.kPrime = 1.0f - pow((1.0f - c.k), 1.0f / (float)m_SolverSteps);  //1.0f-pow((1.0f-c.k), 1.0f/ns);
	if (c.kPrime>1.0)
		c.kPrime = 1.0;

	m_Constraints_Distance.push_back(c);
}

void Sim_PBD::InitBConstraint(const Vector3* positions, uint v1, uint v2, uint v3, float k)
{
	float w1 = m_PhyxelIsStatic[v1] ? 0.f : 1.f;// m_PhyxelsInvMass[v1];
	float w2 = m_PhyxelIsStatic[v2] ? 0.f : 1.f;//m_PhyxelsInvMass[v2];
	float w3 = m_PhyxelIsStatic[v3] ? 0.f : 1.f;// m_PhyxelsInvMass[v3];

	float w = w1 + w2 + 2.f * w3;

	Vector3 centre = (positions[v1] + positions[v2] + positions[v3]) * 0.3333f;

	Sim_PBD_BConstraint c = Sim_PBD_BConstraint(v1, v2, v3, k, w);
	c.length = (positions[v3] - centre).Length();

	c.kPrime = 1.0f - pow((1.0f - c.k), 1.0f / (float)m_SolverSteps);  //1.0f-pow((1.0f-c.k), 1.0f/ns);
	if (c.kPrime>1.0)
		c.kPrime = 1.0;

	m_Constraints_Bending.push_back(c);
}

void  Sim_PBD::UpdateConstraints()
{

}

void Sim_PBD::ComputePhyxelRotations(const Vector3* positions)
{
	m_TriangleRotations.resize(m_Triangles.size());
	memset(&m_PhyxelRotations[0], 0, m_NumPhyxels * sizeof(Matrix3));


#pragma omp parallel for
	for (int i = 0; i < m_Triangles.size(); ++i)
	{
		Vector3 r[3];	
		r[0] = positions[m_Triangles[i].v3] - positions[m_Triangles[i].v1];
		r[1] = positions[m_Triangles[i].v3] - positions[m_Triangles[i].v2];
		r[2] = Vector3::Cross(r[0], r[1]);

		m_TriangleRotations[i] = m_TriangleRotationsInitial[i] * Matrix3::Transpose(Matrix3::OrthoNormalise(r, 2));
	}

	for (int i = 0; i < m_Triangles.size(); ++i)
	{
		Sim_3Noded_Triangle& tri = m_Triangles[i];

		m_PhyxelRotations[tri.v1] += m_TriangleRotations[i];
		m_PhyxelRotations[tri.v2] += m_TriangleRotations[i];
		m_PhyxelRotations[tri.v3] += m_TriangleRotations[i];
	}

#pragma omp parallel for
	for (int i = 0; i < m_NumPhyxels; ++i)
	{
		Vector3* rows = reinterpret_cast<Vector3*>(&m_PhyxelRotations[i]);
		m_PhyxelRotations[i] = Matrix3::OrthoNormalise(rows, 2);
	}
}

bool Sim_PBD::StepSimulation(float dt, const Vector3& gravity, const Vector3* in_x, const Vector3* in_dxdt, Vector3* out_dxdt)
{
	m_Rotations_Invalidated = true;
	Vector3 sub_grav = gravity;

	bool valid_timestep = true;

	//for (size_t i = 0; i < m_Constraints_Distance.size(); ++i)
	//{
	//	Sim_PBD_DConstraint& c = m_Constraints_Distance[i];

	//	NCLDebug::DrawThickLineNDT(in_x[c.c1], in_x[c.c2], c.k * 0.1f);
	//}


	//Update/Compute Future X
#pragma omp parallel for
	for (int i = 0; i < m_NumPhyxels; ++i)
	{
		m_PhyxelPosTmp[i] = in_x[i];
		out_dxdt[i] = Vector3(0.f, 0.f, 0.f);

		if (!m_PhyxelIsStatic[i])
		{
			out_dxdt[i] = in_dxdt[i] + sub_grav * dt;
			m_PhyxelPosTmp[i] += out_dxdt[i] * dt;
		}
	}

	std::random_shuffle(m_Constraints_Distance.begin(), m_Constraints_Distance.end());
	std::random_shuffle(m_Constraints_Bending.begin(), m_Constraints_Bending.end());
	//memcpy(out_dxdt, in_dxdt, m_NumPhyxels * sizeof(Vector3));
	
	//Solve X
	for (uint i = 0; i < m_SolverSteps; ++i)
	{
		for (uint j = 0; j < m_Constraints_Distance.size(); ++j)
		{
			SolveDistanceConstraint(m_Constraints_Distance[j]);
		}
		for (uint j = 0; j < m_Constraints_Bending.size(); ++j)
		{
			SolveBendingConstraint(m_Constraints_Bending[j]);
		}
	}

	//Compute dxdt based on pos-postmp
#pragma omp parallel for
	for (int i = 0; i < m_NumPhyxels; ++i)
	{
		Vector3 tmpdxdt = (m_PhyxelPosTmp[i] - in_x[i]) / dt;
		Vector3 diff = tmpdxdt - out_dxdt[i];

		//TODO!!!!
		out_dxdt[i] = out_dxdt[i] + diff * 0.9f;
	}



	return valid_timestep;
}

void Sim_PBD::SolveDistanceConstraint(const Sim_PBD_DConstraint& c)
{
	float w1 = m_PhyxelIsStatic[c.c1] ? 0.f : 1.f;// m_PhyxelsInvMass[c.c1];
	float w2 = m_PhyxelIsStatic[c.c2] ? 0.f : 1.f;//m_PhyxelsInvMass[c.c2];

	Vector3& p1 = m_PhyxelPosTmp[c.c1];
	Vector3& p2 = m_PhyxelPosTmp[c.c2];

	Vector3 dir = (p1 - p2);
	float w = w1 + w2;

	Vector3 dP = Vector3(0.f, 0.f, 0.f);

	if (w > 0.f)
	{
		float len = dir.Length();
		if (len > 0.f)
		{
			float lambda = (1.0f / w) * (len - c.length)  * c.kPrime;

			dP = (dir / len) * lambda;

			p1 -= dP * w1;
			p2 += dP * w2;
		}
	}
}
void Sim_PBD::SolveBendingConstraint(const Sim_PBD_BConstraint& c)
{
	float w1 = m_PhyxelIsStatic[c.c1] ? 0.f : 1.f;// m_PhyxelsInvMass[c.c1];
	float w2 = m_PhyxelIsStatic[c.c2] ? 0.f : 1.f;//m_PhyxelsInvMass[c.c2];
	float w3 = m_PhyxelIsStatic[c.c3] ? 0.f : 1.f;//m_PhyxelsInvMass[c.c2];

	Vector3& p1 = m_PhyxelPosTmp[c.c1];
	Vector3& p2 = m_PhyxelPosTmp[c.c2];
	Vector3& p3 = m_PhyxelPosTmp[c.c3];

	//Using the paper suggested by DevO
	//http://image.diku.dk/kenny/download/kelager.niebe.ea10.pdf

	Vector3 centre =  (p1 + p2 + p3) * 0.3333f;

	Vector3 dir_centre = p3 - centre;
	float dist_centre = dir_centre.Length();

	float diff = 1.0f - (c.length / dist_centre);
	Vector3 dir_force = dir_centre * diff;
	Vector3 fa = dir_force * (c.kPrime * ((2.0f*w1) / c.w));
	Vector3 fb = dir_force * (c.kPrime * ((2.0f*w2) / c.w));
	Vector3 fc = dir_force  * (-c.kPrime * ((4.0f*w3) / c.w));

	if (w1 > 0.0) {
		p1 += fa;
	}
	if (w2 > 0.0) {
		p2 += fb;
	}
	if (w3 > 0.0) {
		p3 += fc;
	}
}

bool Sim_PBD::ValidateVelocityTimestep(const Vector3* pos_tmp)
{
	return true;
}




void Sim_PBD::GetVertexWsPos(int triidx, const Vector3& gp, const Vector3* positions, Vector3& out_pos)
{
	//Linear Interpolation

	Sim_3Noded_Triangle& tri = m_Triangles[triidx];

	out_pos = positions[tri.v1] * gp.x
		+ positions[tri.v2] * gp.y
		+ positions[tri.v3] * gp.z;
}

void Sim_PBD::GetVertexRotation(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Matrix3& out_rotation)
{
	if (m_Rotations_Invalidated)
	{
		ComputePhyxelRotations(positions);
		m_Rotations_Invalidated = false;
	}
	Sim_3Noded_Triangle& tri = m_Triangles[triidx];

	Matrix3& r1 = m_PhyxelRotations[tri.v1];
	Matrix3& r2 = m_PhyxelRotations[tri.v2];
	Matrix3& r3 = m_PhyxelRotations[tri.v3];

	Matrix3 rot = r1 * gp.x + r2 * gp.y + r3 * gp.z;

	Vector3* rows = reinterpret_cast<Vector3*>(&rot);
	out_rotation = Matrix3::OrthoNormalise(rows, 2);
}

void Sim_PBD::GetVertexStressStrain(int triidx, const Vector3& gp, const Vector3* positions, const Vector3& wspos, Vector3& out_stress, Vector3& out_strain)
{
	out_stress = Vector3(0.f, 0.f, 0.f);
	out_strain = Vector3(0.f, 0.f, 0.f);
}