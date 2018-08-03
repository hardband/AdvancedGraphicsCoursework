#include "ShadowRenderer.h"

#define OFFSET_X 9
#define OFFSET_Y 9

ShadowRenderer::ShadowRenderer(Window & parent) : OGLRenderer(parent)
{

	camera = new Camera(-15.0f, 0.0f, Vector3(-600.0f, 3000.0f, 4000.0f));
	light = new Light(Vector3(200.0f, 2000.0f, 100.0f), Vector4(1, 1, 1, 1), 55000.0f);
	secondLight = new Light(Vector3(500.0f, 2000.0f, 1500.0f), Vector4(1, 1, 1, 1), 55000.0f);

	currentLight = light;

	hellData = new MD5FileData(MESHDIR"hellknight.md5mesh");
	hellNode = new MD5Node(*hellData);

	hellData->AddAnim(MESHDIR"idle2.md5anim");
	hellNode->PlayAnim(MESHDIR"idle2.md5anim");

	//Load in and set up the cube mesh to be used in the scene
	OBJMesh* m = new OBJMesh();
	m->LoadOBJMesh(MESHDIR"cube.obj");
	m->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"purple_swirl.JPG", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	cube = m;

	//Set up the rain particle emitter
	emitter = new PlaneEmitter(8000,8000);
	emitter->SetTexture(TEXTUREDIR"water.JPG");
	emitter->SetParticleSize(5.0f);
	emitter->SetParticleVariance(0.001f);
	emitter->SetLaunchParticles(30.0f);
	emitter->SetParticleLifetime(8000.0f);
	emitter->SetParticleSpeed(0.6f);
	emitter->SetDirection(Vector3(0, -1, 0));
	emitter->setPosition(Vector3(0,4000,0));

	//Load in the shaders
	sceneShader = new Shader(SHADERDIR"shadowSceneVert.glsl", SHADERDIR"shadowSceneFrag.glsl");
	shadowShader = new Shader(SHADERDIR"shadowVert.glsl", SHADERDIR"shadowFrag.glsl");
	fontShader = new Shader(SHADERDIR"fontVertex.glsl", SHADERDIR"TexturedFragment.glsl");
	particleShader = new Shader(SHADERDIR"particleVertex.glsl",SHADERDIR"particleFragment.glsl",SHADERDIR"particleGeometry.glsl");


	//Link the shaders
	if (!sceneShader->LinkProgram() || !shadowShader->LinkProgram()|| !fontShader->LinkProgram() || !particleShader->LinkProgram()) {
		return;
	}

	glGenTextures(1, &shadowTex);
	glBindTexture(GL_TEXTURE_2D, shadowTex);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOWSIZE, SHADOWSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);

	glBindTexture(GL_TEXTURE_2D, 0);

	glGenFramebuffers(1, &shadowFBO);

	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
	glDrawBuffer(GL_NONE);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	floor = Mesh::GenerateQuad();
	floor->SetTexture(SOIL_load_OGL_texture(TEXTUREDIR"brick.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));
	floor->SetBumpMap(SOIL_load_OGL_texture(TEXTUREDIR"brickDOT3.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_MIPMAPS));

	glEnable(GL_DEPTH_TEST);

	SetTextureRepeating(cube->GetTexture(), true);

	basicFont = new Font(SOIL_load_OGL_texture(TEXTUREDIR"tahoma.tga", SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_COMPRESS_TO_DXT), 16, 16);

	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);

	init = true;
}


ShadowRenderer::~ShadowRenderer()
{
	glDeleteTextures(1, &shadowTex);
	glDeleteFramebuffers(1, &shadowFBO);
	delete camera;
	delete light;

	delete hellData;
	delete hellNode;

	delete floor;

	//Delete shaders
	delete sceneShader;
	delete shadowShader;
	delete fontShader;
	delete particleShader;

	delete cube;

	delete emitter;

	delete basicFont;

	currentShader = NULL;
}

void ShadowRenderer::UpdateScene(float msec) {
	camera->UpdateCamera(msec);
	hellNode->Update(msec);

	//Update framerate
	frameRate = setFramerate(msec);

	//Update rain particles
	emitter->Update(msec);

	//Updates the position of the light to move it back and forth
	updateLightPos(msec);

	if (Window::GetKeyboard()->KeyDown(KEYBOARD_UP) && inputBuffer > 500) {
		if (currentLight == light) {
			currentLight = secondLight;
		}
		else {
			currentLight = light;
		}
		inputBuffer = 0;
	}
	inputBuffer = inputBuffer + msec;
}

void ShadowRenderer::RenderScene() {
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	DrawShadowScene(currentLight);
	DrawCombinedScene(currentLight);
	DrawParticles();
	DrawText(frameRate, Vector3(0, 0, 0), 16.0f);

	SwapBuffers();
}

void ShadowRenderer::DrawShadowScene(Light* source) {
	glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);

	glClear(GL_DEPTH_BUFFER_BIT);

	glViewport(0, 0, SHADOWSIZE, SHADOWSIZE);

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

	SetCurrentShader(shadowShader);

	viewMatrix = Matrix4::BuildViewMatrix(source->GetPosition(), Vector3(0, -10000, 0));
	textureMatrix = biasMatrix *(projMatrix * viewMatrix);

	UpdateShaderMatrices();

	//Draws shadows of each object
	DrawFloor();
	DrawMesh();
	DrawPillars();
	DrawLightCube();

	glUseProgram(0);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glViewport(0, 0, width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

}

void ShadowRenderer::DrawCombinedScene(Light* source) {
	SetCurrentShader(sceneShader);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "bumpTex"), 1);
	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "shadowTex"), 2);

	glUniform3fv(glGetUniformLocation(currentShader->GetProgram(), "cameraPos"), 1, (float *)& camera->GetPosition());

	SetShaderLight(*source);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, shadowTex);

	viewMatrix = camera->BuildViewMatrix();
	UpdateShaderMatrices();

	//Darws combined scene of each object
	DrawFloor();
	DrawMesh();
	DrawPillars();
	DrawLightCube();

	glUseProgram(0);

}

//Draws the cube pillars in the scene
void ShadowRenderer::DrawPillars()
{
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 10; j++) {
			modelMatrix = Matrix4::Scale(Vector3(100.0f, 500.0f, 100.0f)) * Matrix4::Translation(Vector3(OFFSET_X * i - OFFSET_X * 5, 0, OFFSET_Y * j - OFFSET_Y * 5));

			Matrix4 tempMatrix = textureMatrix * modelMatrix;

			glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *& tempMatrix.values);

			glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *& modelMatrix.values);

			cube->Draw();
		}
	}
}

//Draw the rain particles
void ShadowRenderer::DrawParticles()
{

	SetCurrentShader(particleShader);

	modelMatrix.ToIdentity();

	glUniform1i(glGetUniformLocation(currentShader->GetProgram(), "diffuseTex"), 0);
	glUniform1f(glGetUniformLocation(currentShader->GetProgram(), "particleSize"), emitter->GetParticleSize());

	UpdateShaderMatrices();

	emitter->Draw();

	glDisable(GL_BLEND);

	glUseProgram(0);
}

//Draw cube moving back and forth in time with light
void ShadowRenderer::DrawLightCube()
{
	//modelMatrix = Matrix4::Scale(Vector3(50.0f, 50.0f, 50.0f)) * Matrix4::Translation(Vector3(light->GetPosition().x / 50 ,40,light->GetPosition().z / 50));
	modelMatrix =  Matrix4::Translation(Vector3(light->GetPosition().x, light->GetPosition().y + 10, light->GetPosition().z)) * Matrix4::Scale(Vector3(50.0f, 50.0f, 50.0f));

	Matrix4 tempMatrix = textureMatrix * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *& tempMatrix.values);

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *& modelMatrix.values);

	cube->Draw();
}

//Draws text to screen
void ShadowRenderer::DrawText(const std::string & text, const Vector3 & position, const float size, const bool perspective)
{
	SetCurrentShader(fontShader);

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
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

	glDisable(GL_BLEND);
	glEnable(GL_DEPTH_TEST);
	
	projMatrix = Matrix4::Perspective(1.0f, 15000.0f, (float)width / (float)height, 45.0f);


	delete mesh; //Once it's drawn, we don't need it anymore!

	glUseProgram(0);
}

//Moves the light back and forth 
void ShadowRenderer::updateLightPos(float msec)
{
	Vector3 lightPos = light->GetPosition();

	if (forward) {
		lightPos.x = lightPos.x + LIGHT_SPEED * (1000.0f / msec);
		
		if (lightPos.x > MOVE_DISTANCE) {
			forward = false;
		}
	}
	else
	{
		lightPos.x = lightPos.x - LIGHT_SPEED * (1000.0f / msec);

		if (lightPos.x < -MOVE_DISTANCE) {
			forward = true;
		}
	}

	light->SetPosition(lightPos);
}

string ShadowRenderer::setFramerate(float msec)
{
	float frameRate = 1000 / msec;

	int intFrame = (int)frameRate;

	if ((int)(frameRate + 0.5) > intFrame) {
		intFrame++;
	}

	return to_string(intFrame);
}

//Draws the hell knight
void ShadowRenderer::DrawMesh() {

	modelMatrix = Matrix4::Translation(Vector3(0.0f, 1000.0f, 0.0f));

	Matrix4 tempMatrix = textureMatrix * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *& tempMatrix.values);

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *& modelMatrix.values);

	hellNode->Draw(*this);

}

//Draws the quad which acts as the floor
void ShadowRenderer::DrawFloor() {
	modelMatrix = Matrix4::Rotation(90, Vector3(1, 0, 0)) * Matrix4::Scale(Vector3(4800, 4800, 1));
	Matrix4 tempMatrix = textureMatrix * modelMatrix;

	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "textureMatrix"), 1, false, *& tempMatrix.values);
	glUniformMatrix4fv(glGetUniformLocation(currentShader->GetProgram(), "modelMatrix"), 1, false, *& modelMatrix.values);

	floor->Draw();
}
