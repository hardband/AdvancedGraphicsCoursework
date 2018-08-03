#include "HellRenderer.h"

#define MAP_SCALE 2

//Initialise class
HellRenderer::HellRenderer(Window & parent) : OGLRenderer(parent)
{
	camera = new Camera(-30.0f, -50.0f, Vector3(2100.0f, 2000.0f, RAW_HEIGHT * HEIGHTMAP_Z));

	heightMap = new HeightMap(TEXTUREDIR"terrain.raw");

	//Make correct shaders
	terrainShader = new Shader(SHADERDIR"BumpVertex.glsl", SHADERDIR"BumpFragment.glsl");
	skyboxShader = new Shader(SHADERDIR"skyboxVertex.glsl", SHADERDIR"skyboxFragment.glsl");
	lavaShader = new Shader(SHADERDIR"texturedVertex.glsl", SHADERDIR"texturedFragment.glsl");
	knightShader = new Shader(SHADERDIR"TexturedVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	fontShader = new Shader(SHADERDIR"fontVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	particleShader = new Shader(SHADERDIR"particleVertex.glsl", SHADERDIR"particleFragment.glsl", SHADERDIR"particleGeometry.glsl");
	reflectShader = new Shader(SHADERDIR"PerPixelVertex.glsl", SHADERDIR "reflectFragment.glsl");

	heightMap->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"Barren Reds.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	heightMap->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"Barren RedsDOT3.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	//Hell knight
	hellData = new MD5FileData(MESHDIR"hellknight.md5mesh");
	hellNode = new MD5Node(*hellData);

	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellNode->PlayAnim(MESHDIR"idle2.md5anim");

	//Generate quads
	skyboxQuad = Mesh::GenerateQuad();
	lavaQuad = Mesh::GenerateQuad();
	reflectQuad = Mesh::GenerateQuad();

	//Set up particle emitter
	smokeEmitter = new PlaneEmitter(20000, 20000);
	//Smoke texture from: https://opengameart.org/content/smoke-particle-assets
	smokeEmitter->SetTexture(TEXTUREDIR"smoke.PNG");
	smokeEmitter->SetParticleSize(50.0f);
	smokeEmitter->SetParticleVariance(0.001f);
	smokeEmitter->SetLaunchParticles(10.0f);
	smokeEmitter->SetParticleLifetime(8000.0f);
	smokeEmitter->SetParticleSpeed(0.5f);
	smokeEmitter->SetDirection(Vector3(0, 1, 0));
	smokeEmitter->setColour(Vector4(0.8f,0.8f,1.0f,1.0f));

	//Set up font for framerate display
	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	//Link shaders and check nothing breaks
	if (!terrainShader->LinkProgram() || !skyboxShader->LinkProgram() || !lavaShader->LinkProgram() || !knightShader->LinkProgram() || !fontShader->LinkProgram()
		|| !particleShader->LinkProgram() || !reflectShader->LinkProgram()||!heightMap->GetTexture() || !heightMap->GetBumpMap()) {
		return;
	}

	//Skybox texture from: http://www.custommapmakers.org/skyboxes.php
	cubeMap = SOIL_load_OGL_cubemap(TEXTUREDIR"hell_rt.tga",
		TEXTUREDIR"hell_lf.tga",
		TEXTUREDIR"hell_up.tga",
		TEXTUREDIR"hell_dn.tga",
		TEXTUREDIR"hell_bk.tga",
		TEXTUREDIR"hell_ft.tga",
		SOIL_LOAD_RGB,
		SOIL_CREATE_NEW_ID, 0
	);

	//Lava texture from: https://opengameart.org/node/18855
	lavaQuad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"lava.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	reflectQuad->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"shiny.PNG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	if (!cubeMap || !lavaQuad->GetTexture()) {
		return;
	}

	//Set some textures to repeat
	SetTextureRepeating(heightMap->GetTexture(), true);
	SetTextureRepeating(heightMap->GetBumpMap(), true);
	SetTextureRepeating(lavaQuad->GetTexture(), true);

	light = new Light(Vector3((RAW_HEIGHT * HEIGHTMAP_X / 2.0f), 500.0f, (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f)), Vector4(1, 1, 1, 1), ((RAW_WIDTH * HEIGHTMAP_X) / 2.0f) * 50);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	//Enable correct stuff
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
	init = true;
}

HellRenderer::~HellRenderer()
{
	delete camera;
	delete heightMap;
	delete light;

	//Delete shaders
	delete skyboxShader;
	delete terrainShader;
	delete lavaShader;
	delete knightShader;
	delete fontShader;
	delete particleShader;
	delete reflectShader;

	//Delete quads
	delete skyboxQuad;
	delete lavaQuad;
	delete reflectQuad;

	delete hellData;
	delete hellNode;

	delete basicFont;

	currentShader = 0;
}

void HellRenderer::RenderScene()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	//Draw everything in order
	drawSkyBox();
	drawHeightMap();
	drawKnight();
	drawReflect();
	drawLava();
	drawSmoke();
	DrawText(frameRate, Vector3(0, 0, 0), 16.0f);

	SwapBuffers();
}

void HellRenderer::UpdateScene(float msec)
{
	camera->UpdateCamera(msec);
	viewMatrix = camera->BuildViewMatrix();

	//Update framerate
	frameRate = setFramerate(msec);

	//Update particle emitter
	smokeEmitter->Update(msec);

	//Update hell knight animation
	hellNode->Update(msec);
}

//Draws the terrain height map
void HellRenderer::drawHeightMap()
{

	SetCurrentShader(terrainShader);
	SetShaderLight(*light);

	glUseProgram(currentShader->GetProgram());
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);

	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float *)& camera->GetPosition());

	textureMatrix.ToIdentity();

	float scale = MAP_SCALE;

	modelMatrix = Matrix4::Scale(Vector3(MAP_SCALE ,MAP_SCALE , MAP_SCALE));

	UpdateShaderMatrices();

	heightMap->Draw();

	glUseProgram(0);
}

//Draws the skybox
void HellRenderer::drawSkyBox()
{
	glDepthMask(GL_FALSE);
	SetCurrentShader(skyboxShader);

	UpdateShaderMatrices();
	skyboxQuad->Draw();

	glUseProgram(0);
	glDepthMask(GL_TRUE);

	
}

//Draws the plane of lava
void HellRenderer::drawLava()
{
	SetCurrentShader(lavaShader);

	float heightX = (RAW_WIDTH * HEIGHTMAP_X / 2.0f) * MAP_SCALE;

	float heightY = 256 * HEIGHTMAP_Y / 3.0f * MAP_SCALE;

	float heightZ = (RAW_HEIGHT * HEIGHTMAP_Z / 2.0f) * MAP_SCALE;

	modelMatrix =
		Matrix4::Translation(Vector3(heightX, heightY, heightZ)) *
		Matrix4::Scale(Vector3(heightX, 1, heightZ)) *
		Matrix4::Rotation(90, Vector3(1.0f, 0.0f, 0.0f));

	textureMatrix = Matrix4::Scale(Vector3(10.0f, 10.0f, 10.0f)) * Matrix4::Rotation(0.0f, Vector3(0.0f, 0.0f, 1.0f));

	UpdateShaderMatrices();

	lavaQuad->Draw();

	glUseProgram(0);
}

//Draws the hell knight
void HellRenderer::drawKnight()
{;

	SetCurrentShader(knightShader);
	SetShaderLight(*light);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	modelMatrix =Matrix4::Translation(Vector3(2000, 270, 500) * MAP_SCALE) *  (Matrix4::Scale(Vector3(2.0f, 2.0f, 2.0f) * MAP_SCALE));

	UpdateShaderMatrices();

	hellNode->Draw(*this);

	glUseProgram(0);
}

//Draw new smoke particles
void HellRenderer::drawSmoke()
{
	SetCurrentShader(particleShader);

	modelMatrix.ToIdentity();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "particleSize"), smokeEmitter->GetParticleSize());

	UpdateShaderMatrices();

	smokeEmitter->Draw();

	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(0);
}

//Draw the reflection pool
void HellRenderer::drawReflect()
{
	SetCurrentShader(reflectShader);
	SetShaderLight(*light);
	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float *)& camera->GetPosition());

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "cubeTex"), 2);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMap);

	float heightY = 256 * HEIGHTMAP_Y / 3.0f * MAP_SCALE + 20;

	modelMatrix = Matrix4::Scale(Vector3(1000.0f, 1.0f, 1200.0f)) * Matrix4::Translation(Vector3(6,350,2.4)) * Matrix4::Rotation(90,Vector3(1,0,0));

	UpdateShaderMatrices();

	reflectQuad->Draw();

	glUseProgram(0);
}

//Draws text. Used to display the framerate
void HellRenderer::DrawText(const std::string & text, const Vector3 & position, const float size, const bool perspective)
{
	SetCurrentShader(fontShader);

	glDisable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	//Create a new temporary TextMesh, using our line of text and our font
	TextMesh* mesh = new TextMesh(text, *basicFont);

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);

	if (perspective) {
		modelMatrix = Matrix4::Translation(position) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix = camera->BuildViewMatrix();
		projMatrix = Matrix4::Perspective(1.0f, 10000.0f, (float)width / (float)height, 45.0f);
	}
	else {
		modelMatrix = Matrix4::Translation(Vector3(position.x, height - position.y, position.z)) * Matrix4::Scale(Vector3(size, size, 1));
		viewMatrix.ToIdentity();
		projMatrix = Matrix4::Orthographic(-1.0f, 1.0f, (float)width, 0.0f, (float)height, 0.0f);
	}
	//Update the matrices, and draw the mesh
	UpdateShaderMatrices();

	mesh->Draw();

	glEnable(GL_DEPTH_TEST);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);


	delete mesh; //Once it's drawn, we don't need it anymore!

	glUseProgram(0);

}

//Returns a string of the current framerate formatted to be rounded to the nearest integer
string HellRenderer::setFramerate(float msec)
{

	float frameRate = 1000 / msec;

	int intFrame = (int)frameRate;

	if ((int)(frameRate + 0.5) > intFrame) {
		intFrame++;
	}

	return to_string(intFrame);
}

