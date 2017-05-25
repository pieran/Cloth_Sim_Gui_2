#pragma once
#include "Vector3.h"

struct Ray
{
	Vector3 GetPointOnRay(float distance) const
	{
		return pos + dir * distance;
	}

	bool intersectsSphere(
		const Vector3& sphere_pos, float sphere_radius,
		float* nearest_dist = NULL, float* furtherst_dist = NULL) const
	{
		const float radius2 = sphere_radius * sphere_radius;

		// geometric solution
		Vector3 L = sphere_pos - pos;
		float tca = Vector3::Dot(L, dir);
		
		if (tca < 0) 
			return false;
		
		float d2 = Vector3::Dot(L, L) - tca * tca;
		
		if (d2 > radius2) 
			return false;
		
		float thc = sqrt(radius2 - d2);
		float t0 = tca - thc;
		float t1 = tca + thc;

		if (t0 > t1) std::swap(t0, t1); //Set t0 as the nearest contact distance

		if (t0 < 0) {
			t0 = t1; // if t0 is negative, let's use t1 instead 
			
			if (t0 < 0) 
				return false; // both t0 and t1 are negative 
		}

		if (nearest_dist)
			*nearest_dist = t0;

		if (furtherst_dist)
			*furtherst_dist = t1; //may be negative

		return true;
	}

	Vector3 pos;
	Vector3 dir;
};