#include "CommonUtils.h"

#include "ObjectMesh.h"
#include "CommonMeshes.h"

Vector4 CommonUtils::GenColour(float scalar, float alpha)
{
	Vector4 c;
	c.w = alpha;

	float t;
	c.x = abs(modf(scalar + 1.0f, &t) * 6.0f - 3.0f) - 1.0f;
	c.y = abs(modf(scalar + 2.0f / 3.0f, &t) * 6.0f - 3.0f) - 1.0f;
	c.z = abs(modf(scalar + 1.0f / 3.0f, &t) * 6.0f - 3.0f) - 1.0f;

	c.x = min(max(c.x, 0.0f), 1.0f);
	c.y = min(max(c.y, 0.0f), 1.0f);
	c.z = min(max(c.z, 0.0f), 1.0f);

	return c;
}

Object* CommonUtils::BuildSphereObject(
	const std::string& name,
	const Vector3& pos,
	float radius,
	bool physics_enabled,
	float inverse_mass,
	bool collidable,
	bool dragable,
	const Vector4& color)
{
	ObjectMesh* sphere = new ObjectMesh(name);

	sphere->SetMesh(CommonMeshes::Sphere(), false);
	sphere->SetTexture(CommonMeshes::CheckerboardTex(), false);
	sphere->SetLocalTransform(Matrix4::Scale(Vector3(radius, radius, radius)));
	sphere->SetColour(color);
	sphere->SetBoundingRadius(radius);

	sphere->SetLocalTransform(Matrix4::Translation(pos) * sphere->GetLocalTransform());
	
	return sphere;
}

Object* CommonUtils::BuildCuboidObject(
	const std::string& name,
	const Vector3& pos,
	const Vector3& halfdims,
	bool physics_enabled,
	float inverse_mass,
	bool collidable,
	bool dragable,
	const Vector4& color)
{
	ObjectMesh* cuboid = new ObjectMesh(name);

	cuboid->SetMesh(CommonMeshes::Cube(), false);
	cuboid->SetTexture(CommonMeshes::CheckerboardTex(), false);
	cuboid->SetLocalTransform(Matrix4::Scale(halfdims));
	cuboid->SetColour(color);
	cuboid->SetBoundingRadius(halfdims.Length());

	cuboid->SetLocalTransform(Matrix4::Translation(pos) * cuboid->GetLocalTransform());
	
	return cuboid;
}
