/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef VOXELCONETRACINGMOBILE_ENGINE_H
#define VOXELCONETRACINGMOBILE_ENGINE_H

//--------------------------------------------------------------------------------
// Include files
//--------------------------------------------------------------------------------
#include <jni.h>

#include <EGL/egl.h>
#include <GLES/gl.h>

#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include "NDKHelper.h"

//-------------------------------------------------------------------------
// Preprocessor
//-------------------------------------------------------------------------
#define HELPER_CLASS_NAME \
  "com/sample/helper/NDKHelper"  // Class name of helper function

//-------------------------------------------------------------------------
// Constants
//-------------------------------------------------------------------------
const int32_t NUM_TEAPOTS_X = 8;

const int32_t NUM_TEAPOTS_Y = 8;

const int32_t NUM_TEAPOTS_Z = 8;

//-------------------------------------------------------------------------
// Shared state for our app.
//-------------------------------------------------------------------------
struct android_app;

class Engine {
	//MoreTeapotsRenderer renderer_;

	ndk_helper::GLContext *gl_context_;

	bool initialized_resources_;
	bool has_focus_;

	ndk_helper::DoubletapDetector doubletap_detector_;
	ndk_helper::PinchDetector pinch_detector_;
	ndk_helper::DragDetector drag_detector_;
	ndk_helper::PerfMonitor monitor_;

	ndk_helper::TapCamera tap_camera_;

	android_app *app_;

	ASensorManager *sensor_manager_;
	const ASensor *accelerometer_sensor_;
	ASensorEventQueue *sensor_event_queue_;

	void UpdateFPS(float fps);
	void ShowUI();
	void TransformPosition(ndk_helper::Vec2 &vec);

public:
	static void HandleCmd(struct android_app *app, int32_t cmd);
	static int32_t HandleInput(android_app *app, AInputEvent *event);

	Engine();
	~Engine();

	void SetState(android_app *state);
	int InitDisplay();
	void LoadResources();
	void UnloadResources();
	void DrawFrame();
	void TermDisplay();
	void TrimMemory();
	bool IsReady();
	void UpdatePosition(AInputEvent *event, int32_t index, float &x, float &y);
	void InitSensors();
	void ProcessSensors(int32_t id);
	void SuspendSensors();
	void ResumeSensors();
};



#endif //VOXELCONETRACINGMOBILE_ENGINE_H
