// The script side of the particle containers (particles.cpp): the global `particleSystem`.
#pragma once

class ParticleContainer;

/** Takes the container of the userdata that a script left on the stack (the result of `particleSystem:new{...}`): the
 *  userdata no longer owns it, so the caller does. Null (and nothing taken off the stack) when the top is no container. */
ParticleContainer *takeParticleContainer();
