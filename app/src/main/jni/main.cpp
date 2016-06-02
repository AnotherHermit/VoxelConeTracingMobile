#include "Engine.h"

Engine g_engine;

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {
	app_dummy();

	g_engine.SetState(state);

	// Init helper functions
	//ndk_helper::JNIHelper::GetInstance()->Init(state->activity,
	//																					 HELPER_CLASS_NAME);

	state->userData = &g_engine;
	state->onAppCmd = Engine::HandleCmd;
	state->onInputEvent = Engine::HandleInput;

#ifdef USE_NDK_PROFILER
	monstartup("libvoxels.so");
#endif

	// Prepare to monitor accelerometer
	g_engine.InitSensors();

	// loop waiting for stuff to do.
	while (1) {
		// Read all pending events.
		int id;
		int events;
		android_poll_source *source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, NULL, &events,
																 (void **) &source)) >= 0) {
			// Process this event.
			if (source != NULL) source->process(state, source);

			g_engine.ProcessSensors(id);

			// Check if we are exiting.
			if (state->destroyRequested != 0) {
				g_engine.TermDisplay();
				return;
			}
		}

		if (g_engine.IsReady()) {
			// Drawing is throttled to the screen update rate, so there
			// is no need to do timing here.
			g_engine.DrawFrame();
		}
	}
}