#include <glm/ext/vector_float3.hpp>
#include <list>
#ifndef PHYSICS_HPP
#define PHYSICS_HPP

const double deltaTime = 0.1;
const double dragConst = 6.5;
const float floorHeight = 0.1f;
const float elasticity = 0.9f;

struct Sphere {
    float Mass;
    float Radius;
    glm::vec3 Position;
    glm::vec3 Velocity;
};

struct Cylinder {
    float Radius;
    glm::vec3 PointA;
    glm::vec3 PointB;
};

struct Plane {
    glm::vec3 planeNormal;
    float planeConstant;
};

void checkConstraints(std::list<Sphere*>& sphereList, std::list<Plane*>& planeList, std::list<Cylinder*>& cylinderList);

void updateSphere(Sphere* sphere, float dt);


#endif // PHYSICS_HPP