#include "Generator_Square_Grid.h"

Generator_Square_Grid::Generator_Square_Grid()
{
	Transform = Matrix4();
	SubDivisions = 3;
}

void Generator_Square_Grid::Generate(Sim_Generator_Output& out)
{	
	uint num_total = GetNumVertices() + GetNumTangents();
	out.NumVertices = GetNumVertices();
	out.NumTangents = GetNumTangents();

	out.Phyxels.resize(num_total);
	out.Phyxels_Initial.resize(num_total);

	GeneratePositions(out.Phyxel_Descriptors, &out.Phyxels[0], &out.Phyxels_Initial[0]);
	GenerateTriangles(out.Triangles);
	GenerateTangents(out, &out.Phyxels[out.NumVertices], &out.Phyxels_Initial[out.NumVertices]);
}

void Generator_Square_Grid::GenerateTriangles(std::vector<FETriangle>& triangles)
{
	uint num_quads = (SubDivisions - 1) * (SubDivisions - 1) / 4;
	uint num_tris = num_quads * 2;
	triangles.resize(num_tris);

	uint tcount = 0;
	uint qcount = 0;
	uint tans[15];

	for (uint x = 2; x < SubDivisions; x += 2)
	{
		for (uint y = 2; y < SubDivisions; y += 2)
		{
			uint a = SubDivisions*(y - 2) + x - 2;
			uint b = SubDivisions*(y - 2) + x;
			uint c = SubDivisions*(y)+x;
			uint d = SubDivisions*(y)+x - 2;

			uint ab = SubDivisions*(y - 2) + x - 1;
			uint dc = SubDivisions*(y)+x - 1;
			uint ad = SubDivisions*(y - 1) + x - 2;
			uint bc = SubDivisions*(y - 1) + x;

			uint mid = SubDivisions*(y - 1) + x - 1;

			//Alternate triangle orientation to allow FEM to better represent the forces
			//if ((y ^ x) & 0x2) == 0)

			triangles[tcount++] = FETriangle(a, c, b, mid, bc, ab);
			triangles[tcount++] = FETriangle(a, d, c, ad, dc, mid);
		}
	}
}

unsigned int  Generator_Square_Grid::GetNumVertices()
{
	return SubDivisions * SubDivisions;
}

void Generator_Square_Grid::GeneratePositions(std::vector<FEVertDescriptor>& vert_descriptors, Vector3* phyxels_transformed, Vector3* phyxels_initial)
{
	uint num_verts = GetNumVertices();
	vert_descriptors.resize(num_verts);
	const float divisor_scalar = 1.f / (SubDivisions - 1);

	unsigned int count = 0;
	for (unsigned int y = 0; y < SubDivisions; ++y)
	{
		for (unsigned int x = 0; x < SubDivisions; ++x)
		{
			vert_descriptors[count].tCoord = Vector2(
				(float(x) * divisor_scalar),
				(float(y) * divisor_scalar)
				);

			vert_descriptors[count].isStatic = false;// (y == 0 || y == SubDivisions - 1) && (x == 0 || x == SubDivisions - 1);



			phyxels_transformed[count] = Transform * Vector3(
				((float(x) * divisor_scalar) - 0.5f),
				((float(y) * divisor_scalar) - 0.5f) * -1.f,
				0.0f);

			phyxels_initial[count] = Vector3(
				((float(x) * divisor_scalar) - 0.5f),
				((float(y) * divisor_scalar) - 0.5f) * -1.f,
				0.0f);


			count++;
		}
	}
}

unsigned int Generator_Square_Grid::GetNumTangents()
{
	uint num_quads = (SubDivisions - 1) * (SubDivisions - 1) / 4;
	return (SubDivisions - 1) * 3 + num_quads * 9;
}

void Generator_Square_Grid::GenerateTangents(Sim_Generator_Output& out, Vector3* tangents_transformed, Vector3* tangents_initial)
{
	uint num_tangents = GetNumTangents();
	uint visual_subdivisions = (SubDivisions - 1) / 2;

	auto GenTangent = [&](uint idx, uint vert_a, uint vert_b)
	{
		tangents_transformed[idx] = out.Phyxels[vert_a] - out.Phyxels[vert_b];
		tangents_initial[idx] = out.Phyxels_Initial[vert_a] - out.Phyxels_Initial[vert_b];
	};

	//Build Tangents
	int tans[15];
	unsigned int tcount = 0;
	for (uint x = 0; x < visual_subdivisions; x++)
	{
		for (uint y = 0; y < visual_subdivisions; y++)
		{
			uint quad_idx = x * visual_subdivisions + y;
			uint quad_idx_tl = (x - 1) * visual_subdivisions + (y - 1);
			uint quad_idx_top = x * visual_subdivisions + (y - 1);
			uint quad_idx_left = (x - 1) * visual_subdivisions + y;

			FETriangle& t_top = out.Triangles[quad_idx * 2];
			FETriangle& t_btm = out.Triangles[quad_idx * 2 + 1];

			t_top.t31_mult = -1.f;
			t_top.t32_mult = -1.f;
			//t_top.t21_mult = -1.f;

			t_btm.t31_mult = -1.f;
			t_btm.t32_mult = -1.f;
			t_btm.tca_mult = -1.f;


			if (y == 0)
			{
				if (x == 0)
				{
					tans[0] = tcount; GenTangent(tcount++, t_top.v3, t_top.v1);
					tans[1] = tcount; GenTangent(tcount++, t_top.v2, t_top.v6);
					tans[2] = tcount; GenTangent(tcount++, t_top.v1, t_top.v3);

					tans[3] = tcount; GenTangent(tcount++, t_btm.v2, t_btm.v1);
					tans[4] = tcount; GenTangent(tcount++, t_btm.v3, t_btm.v4);
					tans[5] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v2);

					tans[6] = tcount; GenTangent(tcount++, t_top.v2, t_top.v1);
					tans[7] = tcount; GenTangent(tcount++, t_top.v3, t_top.v4);
					tans[8] = tcount; GenTangent(tcount++, t_top.v1, t_top.v2);

					tans[9] = tcount; GenTangent(tcount++, t_btm.v3, t_btm.v2);
					tans[10] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v5);
					tans[11] = tcount; GenTangent(tcount++, t_btm.v2, t_btm.v3);

					tans[12] = tcount; GenTangent(tcount++, t_top.v2, t_top.v3);
					tans[13] = tcount; GenTangent(tcount++, t_top.v1, t_top.v5);
					tans[14] = tcount; GenTangent(tcount++, t_top.v3, t_top.v2);
				}
				else
				{
					FETriangle& t_left_top = out.Triangles[quad_idx_left * 2];
					FETriangle& t_left_btm = out.Triangles[quad_idx_left * 2 + 1];

					t_top.t13_mult = -1.f;
					t_btm.t23_mult = -1.f;
					t_btm.tab_mult = -1.f;

					tans[0] = t_left_top.t31;
					tans[1] = tcount; GenTangent(tcount++, t_top.v2, t_top.v6);
					tans[2] = tcount; GenTangent(tcount++, t_top.v1, t_top.v3);

					tans[3] = t_left_top.t32;
					tans[4] = t_left_top.tbc;
					tans[5] = t_left_top.t23;

					tans[6] = tcount; GenTangent(tcount++, t_top.v2, t_top.v1);
					tans[7] = tcount; GenTangent(tcount++, t_top.v3, t_top.v4);
					tans[8] = tcount; GenTangent(tcount++, t_top.v1, t_top.v2);

					tans[9] = t_left_btm.t32;
					tans[10] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v5);
					tans[11] = tcount; GenTangent(tcount++, t_btm.v2, t_btm.v3);

					tans[12] = tcount; GenTangent(tcount++, t_top.v2, t_top.v3);
					tans[13] = tcount; GenTangent(tcount++, t_top.v1, t_top.v5);
					tans[14] = tcount; GenTangent(tcount++, t_top.v3, t_top.v2);
				}
			}
			else
			{
				FETriangle& t_top_top = out.Triangles[quad_idx_top * 2];
				FETriangle& t_top_btm = out.Triangles[quad_idx_top * 2 + 1];

				t_top.t32_mult = 1.f;
				t_top.tca_mult = -1.f;
				t_btm.t12_mult = -1.f;

				if (x == 0)
				{
					tans[0] = t_top_btm.t23;
					tans[1] = t_top_btm.tbc;
					tans[2] = t_top_btm.t32;

					tans[3] = t_top_btm.t21;
					tans[4] = tcount; GenTangent(tcount++, t_btm.v3, t_btm.v4);
					tans[5] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v2);

					tans[6] = tcount; GenTangent(tcount++, t_top.v2, t_top.v1);
					tans[7] = tcount; GenTangent(tcount++, t_top.v3, t_top.v4);
					tans[8] = tcount; GenTangent(tcount++, t_top.v1, t_top.v2);

					tans[9] = tcount; GenTangent(tcount++, t_btm.v3, t_btm.v2);
					tans[10] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v5);
					tans[11] = tcount; GenTangent(tcount++, t_btm.v2, t_btm.v3);

					tans[12] = t_top_top.t23;
					tans[13] = tcount; GenTangent(tcount++, t_top.v1, t_top.v5);
					tans[14] = tcount; GenTangent(tcount++, t_top.v3, t_top.v2);
				}
				else
				{
					t_btm.t23_mult = -1.f;
					t_btm.tab_mult = -1.f;
					t_btm.t13_mult = -1.f;
					t_top.t12_mult = -1.f;
					t_top.t13_mult = -1.f;

					FETriangle& t_topleft_top = out.Triangles[quad_idx_tl * 2];

					FETriangle& t_left_top = out.Triangles[quad_idx_left * 2];
					FETriangle& t_left_btm = out.Triangles[quad_idx_left * 2 + 1];

					tans[0] = t_top_btm.t23; // t_left_top.t31;
					tans[1] = t_top_btm.tbc;
					tans[2] = t_top_btm.t32;

					tans[3] = t_top_btm.t21;
					//tans[3] = t_left_top.t32;
					tans[4] = t_left_top.tbc;
					tans[5] = t_left_top.t23;

					tans[6] = t_topleft_top.t21;
					tans[7] = tcount; GenTangent(tcount++, t_top.v3, t_top.v4);
					tans[8] = tcount; GenTangent(tcount++, t_top.v1, t_top.v2);

					tans[9] = t_left_btm.t32;
					tans[10] = tcount; GenTangent(tcount++, t_btm.v1, t_btm.v5);
					tans[11] = tcount; GenTangent(tcount++, t_btm.v2, t_btm.v3);

					tans[12] = t_top_top.t23;
					tans[13] = tcount; GenTangent(tcount++, t_top.v1, t_top.v5);
					tans[14] = tcount; GenTangent(tcount++, t_top.v3, t_top.v2);
				}

			}




			//Set Tangents
			t_top.t12 = tans[6];
			t_top.t13 = tans[0];
			t_top.t21 = tans[8];
			t_top.t23 = tans[14];
			t_top.t31 = tans[2];
			t_top.t32 = tans[12];
			t_top.tab = tans[7];
			t_top.tbc = tans[13];
			t_top.tca = tans[1];

			t_btm.t12 = tans[3];
			t_btm.t13 = tans[6];
			t_btm.t21 = tans[5];
			t_btm.t23 = tans[9];
			t_btm.t31 = tans[8];
			t_btm.t32 = tans[11];
			t_btm.tab = tans[4];
			t_btm.tbc = tans[10];
			t_btm.tca = tans[7];
		}
	}

#if 0 //TEST FOR MISJOINTED TANGENTS
	for (int i = 0; i < tcount; ++i)
	{
		GenTangent(i].z = (rand() % 1000) / 500.f - 1.f;
	}
#endif
}
