#include <glm/ext/vector_float3.hpp>
#include <list>
#ifndef PHYSICS_HPP
#define PHYSICS_HPP

const double deltaTime = 0.1;
const double dragConst = 6.5;

struct Sphere {
    float Mass;
    float Radius;
    glm::vec3 Position;
    glm::vec3 Velocity;
};

void checkConstraints(std::list<Sphere*> sphereList);

void updateSphere(Sphere* sphere, float dt);


#endif // PHYSICS_HPP