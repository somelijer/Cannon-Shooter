#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>
#include "physics.hpp"
#include <functional>


void rk4Step(glm::vec3& position, glm::vec3& velocity,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationX,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationY,
    const std::function<double(const glm::vec3&, const glm::vec3&)>& accelerationZ,
    float dt) {
    glm::vec3 k1, k2, k3, k4;

    // Evaluate k1
    k1 = dt * velocity;

    // Evaluate k2
    k2 = dt * glm::vec3(
        accelerationX(position + 0.5f * k1, velocity),
        accelerationY(position + 0.5f * k1, velocity),
        accelerationZ(position + 0.5f * k1, velocity)
    );

    // Evaluate k3
    k3 = dt * glm::vec3(
        accelerationX(position + 0.5f * k2, velocity),
        accelerationY(position + 0.5f * k2, velocity),
        accelerationZ(position + 0.5f * k2, velocity)
    );

    // Evaluate k4
    k4 = dt * glm::vec3(
        accelerationX(position + k3, velocity),
        accelerationY(position + k3, velocity),
        accelerationZ(position + k3, velocity)
    );

    // Update velocity and position using RK4 formula
    velocity += (k1 + 2.0f * k2 + 2.0f * k3 + k4) / 6.0f;
    position += dt * velocity;
}


const double GRAVITY_ACC = 9.81;
const double AIR_RESIS = 0.1;

// Function to calculate gravitational force
glm::vec3 calculateGravityForce(float mass) {
    return glm::vec3(0.0,-(float)(mass * GRAVITY_ACC), 0.0);
}

// Function to calculate drag force
glm::vec3 calculateDragForce(const glm::vec3& speed, double dragConst) {
    double speedNorm = glm::length(speed);
    glm::vec3 ret = glm::vec3(0.0f, 0.0f, 0.0f);
    ret.x = -(1.0 / 2.0) * speed.x * float(speedNorm) * AIR_RESIS * float(dragConst);
    ret.y= -(1.0 / 2.0) * speed.y * float(speedNorm) * AIR_RESIS * float(dragConst);
    ret.z = -(1.0 / 2.0) * speed.z * float(speedNorm) * AIR_RESIS * float(dragConst);
    return ret;
}


void CheckConstraints(Sphere* sphere)
{
    float floorHeight = 0.1f;
    if (sphere->Position.y - sphere->Radius < floorHeight) {
        sphere->Position.y = floorHeight + sphere->Radius;
        sphere->Velocity.y *= -1;
        sphere->Velocity *= 0.8;
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

    CheckConstraints(sphere);

    rk4Step(sphere->Position, sphere->Velocity, accelerationX, accelerationY, accelerationZ, dt);
}


