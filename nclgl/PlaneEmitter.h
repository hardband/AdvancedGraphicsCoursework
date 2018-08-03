#pragma once
#include "ParticleEmitter.h"

class PlaneEmitter :
	public ParticleEmitter
{
public:
	PlaneEmitter(float xLength = 10.0f, float yLength = 10.0f);
	virtual ~PlaneEmitter();

	void setXLength(float x);
	void setYLength(float y);
	void setColour(Vector4 colour);
	

protected:
	virtual Particle* GetFreeParticle();

	float xLength;
	float yLength;

	Vector4 colour;
	

};

