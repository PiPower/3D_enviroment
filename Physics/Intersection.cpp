#include "Intersection.hpp"

bool GjkIntersectionTest(Body* bodyA, Body* bodyB, Contact* contact)
{
	return false;
}

bool CheckIntersection(Body* bodyA, Body* bodyB, Contact* contact, float dt)
{
	// divide time into sections and iterate body
	constexpr uint8_t ITERS = 10;
	float stepSize = dt / (float)ITERS;
	for (size_t i = 0; i < ITERS; i++)
	{
		if (GjkIntersectionTest(bodyA, bodyB, contact))
		{
			return true;
		}

		UpdateBody(bodyA, stepSize);
		UpdateBody(bodyB, stepSize);
	}

	UpdateBody(bodyA, -dt);
	UpdateBody(bodyB, -dt);
	return false;
}
