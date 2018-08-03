#pragma comment(lib, "nclgl.lib")

#include "../../nclgl/Window.h"
#include "HellRenderer.h"
#include "DiscoRenderer.h"
#include "CubeRenderer.h"
#include "ShadowRenderer.h"

#define MAX_SELECT 3
#define SCENE_TIME 30000

int main() {
	Window w("Graphics Coursework", 1920, 1080, true);

	if (!w.HasInitialised()) {
		return -1;
	}

	int selection = 0;

	OGLRenderer* renderer = NULL;

	w.LockMouseToWindow(true);
	w.ShowOSPointer(false);

	bool timedSwitch = true;
	bool newScene = true;
	float sceneTime = 0;
	float inputBuffer = 0;

	while (w.UpdateWindow() && !Window::GetKeyboard()->KeyDown(KEYBOARD_ESCAPE)) {
		if (inputBuffer > 500.0f){
			if (Window::GetKeyboard()->KeyDown(KEYBOARD_LEFT)) {
				if (selection == 0) {
					selection = MAX_SELECT - 1;
				}
				else {
					selection--;
				}
				newScene = true;
				inputBuffer = 0;
			}
		
			if (Window::GetKeyboard()->KeyDown(KEYBOARD_RIGHT)) {
				if (selection == MAX_SELECT - 1) {
					selection = 0;
				}
				else {
					selection++;
				}
				newScene = true;
				inputBuffer = 0;
			}


			if (Window::GetKeyboard()->KeyDown(KEYBOARD_P)) {
				timedSwitch = !timedSwitch;
				inputBuffer = 0;
			}
		}

		if (newScene) {
			newScene = false;
			sceneTime = 0;

			if (renderer != NULL) {
				delete renderer;
			}

			if (selection == 0) {
				renderer = new CubeRenderer(w);
			}
			else if (selection == 1) {
				renderer = new HellRenderer(w);
			}
			else if (selection == 2) {
				renderer = new ShadowRenderer(w);
			}
			//BROKEN DO NOT ACCESS 
			else if (selection == 3) {
				renderer = new DiscoRenderer(w);
			}

		}

		float msec = w.GetTimer()->GetTimedMS();

		if (timedSwitch) {
			sceneTime = sceneTime + msec;
		}

		if (sceneTime > SCENE_TIME) {
			if (selection == MAX_SELECT - 1) {
				selection = 0;
			}
			else {
				selection++;
			}
			newScene = true;
		}
		else {
			renderer->UpdateScene(msec);
			renderer->RenderScene();
		}
		
		inputBuffer = inputBuffer + msec;

	}

	return 0;
}