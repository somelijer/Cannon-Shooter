#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include "physics.hpp"
#include <functional>
#include <iostream>


void rk4Step(glm::vec3& position, glm::vec3& velocity,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationX,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationY,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationZ,
    float dt) {
    glm::vec3 k1, k2, k3, k4;

    k1 = dt * velocity;

    k2 = dt * glm::vec3(
        accelerationX(position + 0.5f * k1, velocity),
        accelerationY(position + 0.5f * k1, velocity),
        accelerationZ(position + 0.5f * k1, velocity)
    );

    k3 = dt * glm::vec3(
        accelerationX(position + 0.5f * k2, velocity),
        accelerationY(position + 0.5f * k2, velocity),
        accelerationZ(position + 0.5f * k2, velocity)
    );

    k4 = dt * glm::vec3(
        accelerationX(position + k3, velocity),
        accelerationY(position + k3, velocity),
        accelerationZ(position + k3, velocity)
    );

    velocity += (k1 + 2.0f * k2 + 2.0f * k3 + k4) / 6.0f;
    position += dt * velocity;
}


const double GRAVITY_ACC = 9.81;
const double AIR_RESIS = 0.1;

glm::vec3 calculateGravityForce(float mass) {
    return glm::vec3(0.0,-(float)(mass * GRAVITY_ACC), 0.0);
}

glm::vec3 calculateDragForce(const glm::vec3& speed, double dragConst) {
    double speedNorm = glm::length(speed);
    glm::vec3 ret = glm::vec3(0.0f, 0.0f, 0.0f);
    ret.x = -(1.0 / 2.0) * speed.x * float(speedNorm) * AIR_RESIS * float(dragConst);
    ret.y= -(1.0 / 2.0) * speed.y * float(speedNorm) * AIR_RESIS * float(dragConst);
    ret.z = -(1.0 / 2.0) * speed.z * float(speedNorm) * AIR_RESIS * float(dragConst);
    return ret;
}

bool areSpheresTouching(const Sphere& sphere1, const Sphere& sphere2) {
    float distance = glm::distance(sphere1.Position, sphere2.Position);
    float sumRadii = sphere1.Radius + sphere2.Radius;
    return distance < sumRadii;
}

void handleSphereCollision(Sphere* sphere1, Sphere* sphere2) {
    glm::vec3 collisionNormal = glm::normalize(sphere2->Position - sphere1->Position);
    glm::vec3 relativeVelocity = sphere2->Velocity - sphere1->Velocity;
    float impactSpeed = glm::dot(relativeVelocity, collisionNormal);
    float penetrationDepth = sphere1->Radius + sphere2->Radius - glm::distance(sphere1->Position, sphere2->Position);

    if (impactSpeed > 0)return;
    std::cout << "Collision detected! ==================" << std::endl;
    std::cout << "Before Collision - Sphere 1: Position(" << sphere1->Position.x << ", " << sphere1->Position.y << ", " << sphere1->Position.z
        << ") Velocity(" << sphere1->Velocity.x << ", " << sphere1->Velocity.y << ", " << sphere1->Velocity.z << ")" << std::endl;
    std::cout << "Before Collision - Sphere 2: Position(" << sphere2->Position.x << ", " << sphere2->Position.y << ", " << sphere2->Position.z
        << ") Velocity(" << sphere2->Velocity.x << ", " << sphere2->Velocity.y << ", " << sphere2->Velocity.z << ")" << std::endl;
    std::cout << "Penetration: "<< penetrationDepth << std::endl;
    std::cout << "ImpactSpeed: "<< impactSpeed << std::endl;
    std::cout << "ColisionNormal: " << collisionNormal.x << ", " << collisionNormal.y << ", " << collisionNormal.z << std::endl;

    float totalMass = sphere1->Mass + sphere2->Mass;
    glm::vec3 impulse = (1.0f + elasticity) * impactSpeed * collisionNormal / totalMass;

    sphere1->Velocity += impulse * sphere2->Mass;
    sphere2->Velocity -= impulse * sphere1->Mass;

    glm::vec3 separationVector = 0.5f * penetrationDepth * collisionNormal;
    sphere1->Position += separationVector;
    sphere2->Position -= separationVector;

    std::cout << "After Collision - Sphere 1: Position(" << sphere1->Position.x << ", " << sphere1->Position.y << ", " << sphere1->Position.z
        << ") Velocity(" << sphere1->Velocity.x << ", " << sphere1->Velocity.y << ", " << sphere1->Velocity.z << ")" << std::endl;
    std::cout << "After Collision - Sphere 2: Position(" << sphere2->Position.x << ", " << sphere2->Position.y << ", " << sphere2->Position.z
        << ") Velocity(" << sphere2->Velocity.x << ", " << sphere2->Velocity.y << ", " << sphere2->Velocity.z << ")" << std::endl;
    std::cout << " ===================================" << std::endl << std::endl;

}


void handleSphereCollisionWithPlane(Sphere* sphere, const glm::vec3& planeNormal, float planeConstant) {
    float distanceToPlane = glm::dot(planeNormal, sphere->Position) - planeConstant;

    if (distanceToPlane < sphere->Radius) {
        sphere->Position -= (distanceToPlane - sphere->Radius) * planeNormal;
        sphere->Velocity = glm::reflect(sphere->Velocity, planeNormal) * elasticity;
    }
}

void handleSphereCollisionWithCylinder(Sphere* sphere, Cylinder* cylinder) {

    glm::vec3 AB = cylinder->PointB - cylinder->PointA;
    glm::vec3 AC = sphere->Position - cylinder->PointA;
    glm::vec3 BC = sphere->Position - cylinder->PointB;


    float scalarProjection = glm::dot(AC, AB) / glm::dot(AB, AB);

    glm::vec3 closestPointOnLine = cylinder->PointA + scalarProjection * AB;

    if (scalarProjection > 1 || scalarProjection < 0) return;

    glm::vec3 normal = AC - scalarProjection * AB;

    float distance = glm::length(normal);
    normal = glm::normalize(normal);


    if (distance < sphere->Radius + cylinder->Radius) {
        sphere->Position -= (distance - sphere->Radius) * -normal;
        sphere->Velocity = glm::reflect(sphere->Velocity, -normal) * elasticity;

        std::cout << "Collision detected! ==================" << std::endl;
        std::cout << "Cylinder: PointA(" << cylinder->PointA.x << ", " << cylinder->PointA.y << ", " << cylinder->PointA.z << ")";
        std::cout << " PointB(" << cylinder->PointB.x << ", " << cylinder->PointB.y << ", " << cylinder->PointB.z << ")";
        std::cout << " Radius: " << cylinder->Radius << std::endl;
        std::cout << "Sphere: Position(" << sphere->Position.x << ", " << sphere->Position.y << ", " << sphere->Position.z << ")";
        std::cout << " Radius: " << sphere->Radius << std::endl;
        std::cout << "Closest Point on Line: (" << closestPointOnLine.x << ", " << closestPointOnLine.y << ", " << closestPointOnLine.z << ")" << std::endl;
        std::cout << "Normal: (" << normal.x << ", " << normal.y << ", " << normal.z << ")" << std::endl;
        std::cout << "Scalar projection: " << scalarProjection << std::endl;
        std::cout << "Distance: " << distance << std::endl << std::endl;
    }


}

void checkConstraints(std::list<Sphere*>& sphereList, std::list<Plane*>& planeList, std::list<Cylinder*>& cylinderList)
{
    for (auto outerSphereIt = sphereList.begin(); outerSphereIt != sphereList.end(); ++outerSphereIt) {
        Sphere* sphere = *outerSphereIt;

        for (Plane* plane : planeList) {
            handleSphereCollisionWithPlane(sphere, plane->planeNormal, plane->planeConstant);
        }
        
        for (Cylinder* cylinder : cylinderList) {
            handleSphereCollisionWithCylinder(sphere,cylinder);
        }

        for (auto innerSphereIt = outerSphereIt; innerSphereIt != sphereList.end(); ++innerSphereIt) {
            Sphere* otherSphere = *innerSphereIt;

            if (sphere != otherSphere && areSpheresTouching(*sphere, *otherSphere)) {
                handleSphereCollision(sphere, otherSphere);
            }
        }
    }

    
}





void updateSphere(Sphere* sphere,float dt) {

    float mass = sphere->Mass;
    glm::vec3 addedForce = glm::vec3(0.0f, 0.0f, 0.0f);

    auto accelerationX = [=](const glm::vec3& position, const glm::vec3& velocity) {
        glm::vec3 fg = calculateGravityForce(mass);
        glm::vec3 f_drag = calculateDragForce(velocity, dragConst);
        glm::vec3 f_rez = fg + addedForce + f_drag;
        return f_rez.x / float(mass);
        };

    auto accelerationY = [=](const glm::vec3& position, const glm::vec3& velocity) {
        glm::vec3 fg = calculateGravityForce(mass);
        glm::vec3 f_drag = calculateDragForce(velocity, dragConst);
        glm::vec3 f_rez = fg + addedForce + f_drag;
        return f_rez.y / float(mass);
        };

    auto accelerationZ = [=](const glm::vec3& position, const glm::vec3& velocity) {
        glm::vec3 fg = calculateGravityForce(mass);
        glm::vec3 f_drag = calculateDragForce(velocity, dragConst);
        glm::vec3 f_rez = fg + addedForce + f_drag;
        return f_rez.z / float(mass);
        };


    rk4Step(sphere->Position, sphere->Velocity, accelerationX, accelerationY, accelerationZ, dt);
}


