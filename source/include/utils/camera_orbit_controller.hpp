
#pragma once

#include "mrender/mrender.hpp"
#include "mapp/app.hpp"
#include "mcore/math.hpp"

class CameraOrbitController
{
public:
	CameraOrbitController(mrender::GfxContext* context, mrender::CameraHandle camera);

	void onEvent(mapp::Event& event);
	void onUpdate(const float& dt);

	[[nodiscard]] mrender::CameraHandle getCamera() { return mCamera; }
private:
	mrender::GfxContext* mContext;
	mrender::CameraHandle mCamera;

	bool mMousePressed;
	float mDistanceFromTarget, mYaw, mPitch;
	float mTargetYaw, mTargetPitch;  // Interpolated target rotation values
	mcore::Vector<float, 3> mTargetPosition;
	mcore::Vector<float, 3> mPosition;
	
	float mInterpolationSpeed = 5.0f;  // Speed at which rotation values are interpolated
};