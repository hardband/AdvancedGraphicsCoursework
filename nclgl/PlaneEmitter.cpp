#include "PlaneEmitter.h"



PlaneEmitter::PlaneEmitter(float xLength, float yLength) : ParticleEmitter()
{
	this->xLength = xLength;
	this->yLength = yLength;
	colour = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	emitterPos = Vector3(0, 0, 0);
}

PlaneEmitter::~PlaneEmitter()
{
}

void PlaneEmitter::setXLength(float x)
{
	xLength = x;
}

void PlaneEmitter::setYLength(float y)
{
	yLength = y;
}

void PlaneEmitter::setColour(Vector4 colour)
{
	this->colour = colour;
}

Particle * PlaneEmitter::GetFreeParticle()
{
	Particle * p = NULL;

	//If we have a spare particle on the freelist, pop it off!
	if (freeList.size() > 0) {
		p = freeList.back();
		freeList.pop_back();
	}
	else {
		//no spare particles, so we need a new one
		p = new Particle();
	}

	//Now we have to reset its values - if it was popped off the
	//free list, it'll still have the values of its 'previous life'

	p->colour = colour;
	p->direction = initialDirection;
	p->direction.x += ((RAND() - RAND()) * particleVariance);
	p->direction.y += ((RAND() - RAND()) * particleVariance);
	p->direction.z += ((RAND() - RAND()) * particleVariance);

	p->direction.Normalise();	//Keep its direction normalised!
	p->position = emitterPos;
	p->position.x = p->position.x + ((xLength * RAND()) - (xLength/2.0f));
	p->position.z = p->position.z + ((yLength * RAND()) - (yLength / 2.0f));

	return p;	//return the new particle :-)
}
