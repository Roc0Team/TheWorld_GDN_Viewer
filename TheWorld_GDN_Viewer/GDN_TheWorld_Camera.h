#pragma once

#pragma warning (push, 0)
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/noise_texture2d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#pragma warning (pop)

#include "Utils.h"

#define GD_ACTIVE_CAMERA_GROUP	"TheWorldViewerActiveCamera"
#define GD_CUSTOM_CAMERAS_GROUP	"TheWorldViewerCustomCameras"
#define INVALID_CAMERA_ALTITUDE	-999999999.9f

constexpr float CAMERA_INITIAL_SCALE_FACTOR = 0.5;		// 1.0;

namespace godot {

	class GDN_TheWorld_Globals;
	class GDN_TheWorld_MapViewer;

	class GDN_TheWorld_MapCamera : public godot::Camera2D
	{
		GDCLASS(GDN_TheWorld_MapCamera, Camera2D)

	protected:
		static void _bind_methods(void);
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapCamera(void);
		~GDN_TheWorld_MapCamera(void);

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		virtual void _unhandled_input(const Ref<InputEvent>& event) override;
		virtual void _physics_process(double _delta) override;

	public:
		godot::Vector2 getSavedGlobalPosition(void);
		godot::Transform2D getCameraWorldTransform();
		godot::Vector2 getCameraWorldPos();

		void init(GDN_TheWorld_MapViewer* mapViewer)
		{
			m_mapViewer = mapViewer;
			setZoom(CAMERA_INITIAL_SCALE_FACTOR);
			//setZoom(0.1f);
		}

		void pauseMouseMovement(void)
		{
			m_mouseMovementPaused = true;
		}
		void resumeMouseMovement(void)
		{
			m_mouseMovementPaused = false;
		}
		bool mouseMovementSuspended(void) const
		{
			return m_mouseMovementPaused;
		}

		void setZoom(float zoomFactor);
		float getZoomFactor(void) const
		{
			return m_zoomFactor;
		}
		void resetInputEvents(void)
		{
			m_mouseShiftCameraOn = false;
		}

	private:
		GDN_TheWorld_MapViewer* m_mapViewer = nullptr;
		int m_zoomRequired = 0;
		bool m_updateCameraRequired = false;
		godot::Vector2 m_savedGlobalPos;

		bool m_mouseMovementPaused = false;

		bool m_shiftPressed = false;
		bool m_ctrlPressed = false;
		bool m_altPressed = false;
	
		// Camera Movement/Rotation
		bool m_mouseShiftCameraOn = false;
		godot::Vector2 m_mouseRelativePosToShift;
		// Camera Movement/Rotation

		float m_sensitivity = 1.0;	// 0.0 - 1.0

		float m_zoomFactor = -1;
	};

	class GDN_TheWorld_Camera : public Camera3D
	{
		GDCLASS(GDN_TheWorld_Camera, Camera3D)

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		GDN_TheWorld_Camera();
		~GDN_TheWorld_Camera();

		virtual void _ready() override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const godot::Ref<godot::InputEvent>& event) override;

		bool initCameraInWorld(Vector3 cameraPos, Vector3 lookAt, float cameraYaw, float cameraPitch, float cameraRoll);
		void set_cameraFar(void);
		godot::Transform3D getWorldTransform(void);
		void setWorldTransform(godot::Transform3D map);
		godot::Vector3 getWorldPos(void);
		void setWorldPos(godot::Vector3 worldPos);
		//bool initPlayerCamera(void);
		//bool initOtherEntityCamera(void);
		void calcYawPitchRoll(void);
		bool updateCamera(double _delta);
		bool updateFreeLookCamera(double _delta);
		bool updatePlayerCamera(double _delta);
		void updateCameraUnderWater(void);
		void activateCamera(void);
		void deactivateCamera(void);
		GDN_TheWorld_Camera* getActiveCamera(void);
		bool isActiveCamera(void) const;
		bool isWorldCamera() const
		{
			return m_WorldCamera;
		}
		void notifyActiveCameraFlag(bool active);
		GDN_TheWorld_Globals* Globals(bool useCache = true);
		float getAngleFromNorthDeg(void) const
		{
			return m_angleFromNorthDeg;
		}
		float getYaw(bool radiant = true) const
		{
			if (radiant)
				return m_yawEuler;
			else
				return (m_yawEuler * 180) / TheWorld_Utils::kPi;
		}
		void setYaw(float yaw, bool radiant = true);
		float getPitch(bool radiant = true) const
		{
			if (radiant)
				return m_pitchEuler;
			else
				return (m_pitchEuler * 180) / TheWorld_Utils::kPi;
		}
		void setPitch(float pitch, bool radiant = true);
		float getRoll(bool radiant = true) const
		{
			if (radiant)
				return m_rollEuler;
			else
				return (m_rollEuler * 180) / TheWorld_Utils::kPi;
		}
		void setRoll(float roll, bool radiant = true);
		void trackPlayer(godot::Node3D* player)
		{
			m_trackedPlayer = player;
			m_trackingPlayer = true;
		}
		void untrackPlayer(void)
		{
			m_trackedPlayer = nullptr;
			m_trackingPlayer = false;
		}
		bool trackingPlayer(void) const
		{
			return m_trackingPlayer;
		}
		float getCameraTrackingSpeed(void) const
		{
			return m_trackingLerpSpeed;
		}
		void setCameraTrackingSpeed(float f)
		{
			m_trackingLerpSpeed = f;
		}
		godot::Vector3 getCameraTrackingOffeset(void) const
		{
			return m_trackingOffset;
		}
		void setCameraTrackingOffeset(godot::Vector3 v)
		{
			m_trackingOffset = v;
		}

		void updateAltitudeFromTerrain(bool force);

		float cameraMaxAltitudeFromTerrainDetected(void) const;
		float cameraAltitudeFromTerrain(void) const
		{
			return m_lastCameraAltitudeFromTerrain;
		}
		float cameraAltitudeFromTerrainValid(void) const
		{
			return m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE;
		}
		void forceRecalcCameraAltitude(void)
		{
			m_forceRecalcCameraAltitude = true;
		}
		void forceRecalcCameraFar(void)
		{
			m_forceRecalcCameraFar = true;
		}

		void setGlobalTransform(godot::Transform3D t)
		{
			set_global_transform(t);
		}

		bool cameraUnderWater(void) const
		{
			return m_cameraUnderWater;
		}

		void pauseMouseMovement(void)
		{
			m_mouseMovementPaused = true;
		}
		void resumeMouseMovement(void)
		{
			m_mouseMovementPaused = false;
		}
		bool mouseMovementSuspended(void) const
		{
			return m_mouseMovementPaused;
		}

		void updateCameraRequired(bool b)
		{
			m_updateCameraRequired = b;
		}

		void adjustPositionOnTerrain(void);
		void adjustPositionOnTerrainRequired(bool b, bool overSea = false)
		{
			m_adjustPositionOnTerrainRequired = b;
			m_overSea = overSea;
		}
		bool adjustPositionOnTerrainRequired(void)
		{
			return m_adjustPositionOnTerrainRequired;
		}
		float velocity(void) const
		{
			return m_velocity;
		}

	private:

	private:
		bool m_ready;
		bool m_quitting;
		godot::Transform3D m_lastCameraPosInWorld;
		bool m_WorldCamera;
		bool m_updateCameraRequired;
		bool m_adjustPositionOnTerrainRequired = false;
		bool m_overSea = false;
		bool m_isActive;
		int64_t m_instanceId;
		godot::Node3D* m_trackedPlayer = nullptr;
		//godot::Transform3D m_posToTrack;
		//bool m_pos_toTrackInitialized = false;
		bool m_trackingPlayer = false;
		float m_trackingLerpSpeed = 10.0f;
		godot::Vector3 m_trackingOffset = godot::Vector3(0.0f, 7.0f, 5.0f);

		bool m_mouseMovementPaused = false;

		
		godot::Vector3 m_savedCameraPosAfterSetFar = godot::Vector3(0.0f, 0.0f, 0.0f);
		bool m_forceRecalcCameraFar = false;
		godot::Vector3 m_savedCameraPosAfterCalcAltitude = godot::Vector3(0.0f, 0.0f, 0.0f);
		godot::Vector3 m_lastComputedCameraVerticalOnTerrain;
		float m_lastCameraAltitudeFromTerrain = INVALID_CAMERA_ALTITUDE;
		bool m_forceRecalcCameraAltitude = false;
		//godot::Vector3 m_lastComputedCameraVerticalOnTerrain = godot::Vector3(0.0f, 0.0f, 0.0f);

		float m_angleFromNorthDeg;	// angle expressed in degrees of the projection of the camera on XZ plane: 0 ==> camera points to north, 90 camera points to east, 180 camera points to south, 270 camera points to west

		float m_yawEuler;	// radiant angle of the projection of the camera on XZ plane and Z axis (-Z direction)
							// 0 ==> projection of camera is aligned with Z axis and points to decreasing Z direction (north)
							// PI/2 ==> projection of camera is aligned with X axis and points to increasing X direction (east)
							// PI(/-PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (south)
							// -PI/2 ==> projection of camera is aligned with X axis and points to decreasing X direction (west)
							// -PI(/PI) ==> projection of camera is aligned with Z axis and points to decreasing Z direction (south)

		float m_pitchEuler;	// radiant angle of the projection of the camera on YZ plane and Z axis (-Z direction)
							// 0 ==> projection of camera is aligned with Z axis and points to decreasing Z direction (face forward to horizonth aligned with terrain)
							// PI/2 ==> projection of camera is aligned with Y axis and points to increasing Y direction (face up perpendicularly to terrain)
							// PI(/-PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (face backward to horizonth aligned with terrain)
							// -PI/2 ==> projection of camera is aligned with Y axis and points to decreasing Y direction (face down perpendicularly to terrain)
							// -PI(/PI) ==> projection of camera is aligned with Z axis and points to increasing Z direction (face backward to horizonth aligned with terrain)

		float m_rollEuler;

		// Camera Movement
		int m_numMoveStepForward = 0;
		int m_numLongMoveStepForward = 0;
		int m_numMoveStepLeft = 0;
		int m_numLongMoveStepLeft = 0;
		int m_numMoveStepUp;
		float m_wheelVeryFastVelocity;
		float m_wheelFastVelocity;
		float m_wheelNormalVelocity;
		float m_wheelSlowVelocity;
		// Camera Movement

		bool m_mouseButtonRightPressedWhenSuspended = false;
		bool m_mouseButtonLeftPressedWhenSuspended = false;

		godot::Vector3 m_prevCameraPos;
		float m_velocity = 0;	// m/s
		
		// Camera Movement/Rotation
		bool m_mouseShiftCameraOn;
		//bool m_shiftVertCameraOn;
		bool m_rotateCameraOn;
		bool m_shiftPressed;
		bool m_ctrlPressed;
		bool m_altPressed;
		//bool m_forwardMovementOn;
		//bool m_backwardMovementOn;
		//bool m_leftMovementOn;
		//bool m_rightMovementOn;
		//bool m_upwardMovementOn;
		//bool m_downwardMovementOn;
		godot::Vector2 m_mouseRelativePosToRotate;
		godot::Vector2 m_mouseRelativePosToShift;
		//godot::Vector2 m_mouseRelativePosToShiftVert;
		float m_sensitivity;			// 0.0 - 1.0
		float m_smoothness;				// 0.0 - 0.999 tick 0.001
		float m_yaw;
		float m_lastYaw;
		float m_totalYaw;
		int m_yawLimit;
		float m_pitch;
		float m_lastPitch;
		float m_totalPitch;
		int m_pitchLimit;
		// Camera Movement/Rotation
		
		godot::Ref<godot::Image> m_seaWaveMapImage;
		godot::Ref<godot::NoiseTexture2D> m_seaWaveMapTexture;
		bool m_cameraUnderWater = false;
		//godot::MeshInstance3D* m_postProcessingUnderWaterNode = nullptr;
		//godot::MeshInstance3D* m_postProcessingUnderWaterOverrideNode = nullptr;

		// Node cache
		GDN_TheWorld_Globals* m_globals;
	};

}

