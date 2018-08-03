#pragma once

#include "../../nclgl/OGLRenderer.h"
#include "../../nclgl/Camera.h"
#include "../../nclgl/OBJMesh.h"
#include "../../nclgl/HeightMap.h"

#define LIGHTNUM 8
class DiscoRenderer : public OGLRenderer
{
public:
	DiscoRenderer(Window & parent);
	virtual ~DiscoRenderer();

	virtual void RenderScene();
	virtual void UpdateScene(float msec);

protected:
	void FillBuffers(); //G- Buffer Fill Render Pass
	void DrawPointLights(); // Lighting Render Pass
	void CombineBuffers(); // Combination Render Pass
	void drawSkyBox();

	void GenerateScreenTexture(GLuint & into, bool depth = false);

	Shader * sceneShader; // Shader to fill our GBuffers
	Shader * pointlightShader; // Shader to calculate lighting
	Shader * combineShader; // shader to stick it all together
	Shader* skyboxShader;

	Light * pointLights; // Array of lighting data
	Mesh * heightMap; // Terrain !
	OBJMesh * sphere; // Light volume
	Mesh * quad; // To draw a full - screen quad
	Camera * camera; // Our usual camera
	Mesh* skyboxQuad;
	OBJMesh* discoBall;

	float rotation; // How much to increase rotation by

	GLuint bufferFBO; // FBO for our G- Buffer pass
	GLuint bufferColourTex; // Albedo goes here
	GLuint bufferNormalTex; // Normals go here
	GLuint bufferDepthTex; // Depth goes here

	GLuint pointLightFBO; // FBO for our lighting pass
	GLuint lightEmissiveTex; // Store emissive lighting
	GLuint lightSpecularTex; // Store specular lighting

	GLuint cubeMap;
};

