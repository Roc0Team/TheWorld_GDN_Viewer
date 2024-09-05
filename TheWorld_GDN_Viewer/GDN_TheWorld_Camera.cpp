//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Viewer.h"

#pragma warning (push, 0)
//#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#pragma warning (pop)

#include <math.h> 

using namespace godot;

#define DISTANCE_THRESHOLD_TO_UPDATE_ALTITUDE	5.0f
#define DISTANCE_THRESHOLD_TO_UPDATE_CAMERA_FAR	50.0f
#define MAX_ALTITUDE_DETECTED					5000.0f

#define	POST_PROCESSING_UNDER_WATER_NODE			"PostProcessingUnderWater"
#define	POST_PROCESSING_UNDER_WATER_OVERRIDE_NODE	"PostProcessingUnderWaterOverride"

void GDN_TheWorld_MapCamera::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("get_camera_map_transform"), &GDN_TheWorld_MapCamera::getCameraMapTransform);
}

GDN_TheWorld_MapCamera::GDN_TheWorld_MapCamera(void)
{
}

GDN_TheWorld_MapCamera::~GDN_TheWorld_MapCamera(void)
{
}

void GDN_TheWorld_MapCamera::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
	{
	}
	break;
	case NOTIFICATION_EXIT_TREE:
	{
	}
	break;
	case NOTIFICATION_WM_CLOSE_REQUEST:
	{
	}
	break;
	case NOTIFICATION_PREDELETE:
	{
	}
	break;
	}
}

void GDN_TheWorld_MapCamera::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
	
	if (m_mapViewer == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return;
	
	if (!viewer->mapMode())
		return;

	if (event->is_action_pressed("ui_mouse_wheel_up"))
	{
		m_zoomRequired++;
		m_updateCameraRequired = true;
	}

	if (event->is_action_pressed("ui_mouse_wheel_down"))
	{
		m_zoomRequired--;
		m_updateCameraRequired = true;
	}

	if (event->is_action_pressed("ui_mouse_button_left"))
		m_mouseShiftCameraOn = true;
	
	if (event->is_action_released("ui_mouse_button_left"))
		m_mouseShiftCameraOn = false;

	const InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<InputEventMouseMotion>(event.ptr());
	if (eventMouseMotion != nullptr)
	{
		//Globals()->debugPrint("GDN_TheWorld_Camera::_input: " + event->as_text());
		if (m_mouseShiftCameraOn)
		{
			m_mouseRelativePosToShift = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
	}

}

void GDN_TheWorld_MapCamera::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapCamera::_ready(void)
{
}

void GDN_TheWorld_MapCamera::_physics_process(double _delta)
{
	if (m_mapViewer == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return;

	GDN_TheWorld_Map* map = m_mapViewer->getMap();
	if (map == nullptr)
		return;

	if (!viewer->mapMode())
		return;

	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_shift"))
		m_shiftPressed = true;
	else
		m_shiftPressed = false;

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;

	if (input->is_action_pressed("ui_alt"))
		m_altPressed = true;
	else
		m_altPressed = false;

	//if (input->is_action_pressed("ui_mouse_button_left"))
	//	m_mouseShiftCameraOn = true;
	//else
	//	m_mouseShiftCameraOn = false;

	if (m_updateCameraRequired)
	{
		m_updateCameraRequired = false;

		if (m_zoomRequired != 0)
		{
			map->zoom(m_zoomRequired);
			m_zoomRequired = 0;
		}

		if (m_mouseRelativePosToShift != godot::Vector2(0, 0))
		{
			float sensitivity = m_sensitivity;
			if (m_ctrlPressed)
				sensitivity *= 2;
			m_mouseRelativePosToShift *= sensitivity;

			//translate(godot::Vector2(-m_mouseRelativePosToShift.x, -m_mouseRelativePosToShift.y));
			translate(-m_mouseRelativePosToShift);

			m_mouseRelativePosToShift = godot::Vector2(0, 0);
		}
	}
}

void GDN_TheWorld_MapCamera::_process(double _delta)
{
}

godot::Transform2D GDN_TheWorld_MapCamera::getCameraMapTransform()
{
	godot::Transform2D map;

	if (m_mapViewer == nullptr)
		return map;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return map;

	godot::Transform2D cameraT = get_global_transform();
	map = viewer->globalToMap(cameraT);
	
	return map;
}

godot::Vector2 GDN_TheWorld_MapCamera::getCameraMapPos()
{
	return getCameraMapTransform().get_origin();
}

void GDN_TheWorld_Camera::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("is_active_camera"), &GDN_TheWorld_Camera::isActiveCamera);
	ClassDB::bind_method(D_METHOD("get_active_camera"), &GDN_TheWorld_Camera::getActiveCamera);
	ClassDB::bind_method(D_METHOD("get_angle_from_north_degree"), &GDN_TheWorld_Camera::getAngleFromNorthDeg);
	ClassDB::bind_method(D_METHOD("get_yaw"), &GDN_TheWorld_Camera::getYaw);
	ClassDB::bind_method(D_METHOD("set_yaw"), &GDN_TheWorld_Camera::setYaw);
	ClassDB::bind_method(D_METHOD("get_pitch"), &GDN_TheWorld_Camera::getPitch);
	ClassDB::bind_method(D_METHOD("set_pitch"), &GDN_TheWorld_Camera::setPitch);
	ClassDB::bind_method(D_METHOD("get_roll"), &GDN_TheWorld_Camera::getRoll);
	ClassDB::bind_method(D_METHOD("set_roll"), &GDN_TheWorld_Camera::setRoll);
	ClassDB::bind_method(D_METHOD("init_camera_in_world"), &GDN_TheWorld_Camera::initCameraInWorld);
	ClassDB::bind_method(D_METHOD("track_player"), &GDN_TheWorld_Camera::trackPlayer);
	ClassDB::bind_method(D_METHOD("untrack_player"), &GDN_TheWorld_Camera::untrackPlayer);
	ClassDB::bind_method(D_METHOD("set_camera_tracking_speed", "p_value"), &GDN_TheWorld_Camera::setCameraTrackingSpeed);
	ClassDB::bind_method(D_METHOD("get_camera_tracking_speed"), &GDN_TheWorld_Camera::getCameraTrackingSpeed);
	ClassDB::bind_method(D_METHOD("set_camera_tracking_offset", "p_value"), &GDN_TheWorld_Camera::setCameraTrackingOffeset);
	ClassDB::bind_method(D_METHOD("get_camera_tracking_offset"), &GDN_TheWorld_Camera::getCameraTrackingOffeset);
	ClassDB::bind_method(D_METHOD("get_camera_altitude_from_terrain"), &GDN_TheWorld_Camera::cameraAltitudeFromTerrain);
	ClassDB::bind_method(D_METHOD("set_camera_global_transform"), &GDN_TheWorld_Camera::setGlobalTransform);
	ClassDB::bind_method(D_METHOD("set_camera_world_transform", "p_transform"), &GDN_TheWorld_Camera::setWorldTransform);
	ClassDB::bind_method(D_METHOD("get_camera_world_transform"), &GDN_TheWorld_Camera::getWorldTransform);

	ClassDB::add_property_group("GDN_TheWorld_Camera", "Camera", "camera_");
	ClassDB::add_property("GDN_TheWorld_Camera", PropertyInfo(Variant::FLOAT, "camera_tracking_speed", PROPERTY_HINT_RANGE, "0.0, 10.0, 0.1, or_greater", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), "set_camera_tracking_speed", "get_camera_tracking_speed");
	ClassDB::add_property("GDN_TheWorld_Camera", PropertyInfo(Variant::VECTOR3, "camera_tracking_offset"), "set_camera_tracking_offset", "get_camera_tracking_offset");
}

GDN_TheWorld_Camera::GDN_TheWorld_Camera()
{
	m_quitting = false;
	m_ready = false;
	//std::string m = TheWorld_Utils::MeshCacheBuffer::generateNewMeshId();
	m_isActive = false;
	m_instanceId = -1;
	m_WorldCamera = false;
	m_updateCameraRequired = false;
	m_angleFromNorthDeg = 0;
	m_yawEuler = 0;
	m_pitchEuler = 0;
	m_rollEuler = 0;

	// Camera Movement
	// Camera Movement
	m_numMoveStepForward = 0;
	m_numMoveStepLeft = 0;
	m_numMoveStepUp = 0;
	m_wheelVeryFastVelocity = 500.0f;	// 40.0;	// 10.0;
	m_wheelFastVelocity = 20.0f;	// 40.0;	// 10.0;
	m_wheelNormalVelocity = 5.0f;	// 10.0;
	m_wheelSlowVelocity = 0.3F;		// 10.0;
	//m_forwardMovementOn = false;
	//m_backwardMovementOn = false;
	//m_leftMovementOn = false;
	//m_rightMovementOn = false;
	//m_upwardMovementOn = false;
	//m_downwardMovementOn = false;
	// Camera Movement

	// Camera Rotation
	m_mouseShiftCameraOn = false;
	//m_shiftVertCameraOn = false;
	m_shiftPressed = false;
	m_ctrlPressed = false;
	m_altPressed = false;
	m_rotateCameraOn = false;
	m_mouseRelativePosToShift = godot::Vector2(0, 0);
	//m_mouseRelativePosToShiftVert = godot::Vector2(0, 0);
	m_mouseRelativePosToRotate = godot::Vector2(0, 0);
	m_sensitivity = 1.0;	// 0.5;
	m_smoothness = 0.5;
	m_yaw = 0.0;
	m_lastYaw = m_yaw;
	m_totalYaw = 0.0;
	m_yawLimit = 360;
	m_pitch = 0.0;
	m_lastPitch = m_pitch;
	m_totalPitch = 0.0;
	m_pitchLimit = 360;
	// Camera Rotation

	m_globals = NULL;

	_init();
}

GDN_TheWorld_Camera::~GDN_TheWorld_Camera()
{
	remove_from_group(GD_CUSTOM_CAMERAS_GROUP);
}

void GDN_TheWorld_Camera::_init()
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_Camera::_init");

	m_instanceId = get_instance_id();
	set_name(THEWORLD_CAMERA_NODE_NAME);
}

void GDN_TheWorld_Camera::_ready()
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
	{
		globals->debugPrint("GDN_TheWorld_Camera::_ready");
		//Node* parent = get_parent();
		//String parentName = parent->get_name();
		//globals->debugPrint("GDN_TheWorld_Camera::_ready - Parent name: " + parentName);
	}

	m_ready = true;

	add_to_group(GD_CUSTOM_CAMERAS_GROUP);

	// Load Sea Wave Map
	{
		String texName = "res://addons/twviewer/shaders/tex_sea_wave.tres";
		m_seaWaveMapTexture = godot::ResourceLoader::get_singleton()->load(texName);
	}

	// Under water post processing effect
	if (not IS_EDITOR_HINT())
	{
		godot::TypedArray<godot::Node> children = get_children();
		while (children.size())
		{
			godot::MeshInstance3D* child = Object::cast_to<godot::MeshInstance3D>(children.pop_back());
			if (child == nullptr)
				continue;
			if (child->get_name() == godot::String(POST_PROCESSING_UNDER_WATER_NODE))
				m_postProcessingUnderWaterNode = child;
			if (child->get_name() == godot::String(POST_PROCESSING_UNDER_WATER_OVERRIDE_NODE))
				m_postProcessingUnderWaterOverrideNode = child;
		}

		if (m_postProcessingUnderWaterNode == nullptr)
		{
			String shaderPath;
																															//shaderPath  = "res://addons/twviewer/shaders/CameraUnderWater.gdshader";
			shaderPath = "res://addons/twviewer/shaders/CameraUnderWaterNew.gdshader";
			godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(shaderPath);

			godot::ShaderMaterial* mat = memnew(godot::ShaderMaterial);
			mat->set_shader(shader);
			//mat->set_render_priority(0);

			godot::QuadMesh* quadMesh = memnew(godot::QuadMesh);
			quadMesh->set_size(godot::Vector2(3, 2));
																															//quadMesh->set_size(godot::Vector2(2, 2));
			quadMesh->set_orientation(godot::PlaneMesh::Orientation::FACE_Z);
			quadMesh->set_flip_faces(false);
																															//quadMesh->set_flip_faces(true);
			quadMesh->set_material(mat);

			m_postProcessingUnderWaterNode = memnew(godot::MeshInstance3D);
			m_postProcessingUnderWaterNode->set_name(POST_PROCESSING_UNDER_WATER_NODE);
			add_child(m_postProcessingUnderWaterNode);
			m_postProcessingUnderWaterNode->set_visible(true);
			m_postProcessingUnderWaterNode->set_position(godot::Vector3(0, 0, -1));
																															//m_postProcessingUnderWaterNode->set_position(godot::Vector3(0, 0, -11.381f));
			m_postProcessingUnderWaterNode->set_rotation_degrees(godot::Vector3(0, 0, 0));
																															//m_postProcessingUnderWaterNode->set_rotation_degrees(godot::Vector3(12.5f, -56.4f, -18.1f));
																															//m_postProcessingUnderWaterNode->set_extra_cull_margin(20000);
			m_postProcessingUnderWaterNode->set_mesh(quadMesh);
		}

		if (m_postProcessingUnderWaterOverrideNode == nullptr or !m_postProcessingUnderWaterOverrideNode->is_visible())
		{
			if (m_postProcessingUnderWaterNode != nullptr)
				m_postProcessingUnderWaterNode->set_visible(true);
		}
		else
		{
			if (m_postProcessingUnderWaterNode != nullptr)
				m_postProcessingUnderWaterNode->set_visible(false);
		}
	}
}

void GDN_TheWorld_Camera::_notification(int p_what)
{
	//if (globals->status() != TheWorldStatus::sessionInitialized)
	//	return;

	switch (p_what)
	{
	case NOTIFICATION_WM_CLOSE_REQUEST:
	{
		m_quitting = true;
	}
	break;
	case NOTIFICATION_PREDELETE:
	{
		if (m_ready)
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				if (m_quitting)
					globals->debugPrint("GDN_TheWorld_Camera::_notification (NOTIFICATION_PREDELETE) - Destroy Camera", false);
				else
					globals->debugPrint("GDN_TheWorld_Camera::_notification (NOTIFICATION_PREDELETE) - Destroy Camera");
		}
	}
	break;
	}
}

void GDN_TheWorld_Camera::_process(double _delta)
{
	//GDN_TheWorld_Globals* globals = Globals();
	//if (globals == nullptr)
	//	return;

	//if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
	//	return;

	//if (!isActiveCamera())
	//{
	//	return;
	//}

	//if (!is_current())
	//{
	//	return;
	//}
}

godot::Transform3D GDN_TheWorld_Camera::getWorldTransform(void)
{
	godot::Transform3D map;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return map;

	godot::Transform3D cameraT = get_global_transform();
	map = viewer->globalToWorld(cameraT);
	//godot::Transform3D viewerT = viewer->getInternalGlobalTransform().affine_inverse();
	//ret = viewerT * cameraT;

	//godot::Transform3D cameraT1 = viewer->worldToGlobal(map);	// DEBUG ONLY

	return map;
}

void GDN_TheWorld_Camera::setWorldTransform(godot::Transform3D map)
{
	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	//godot::Transform3D cameraT = get_global_transform();	// DEBUG ONLY
	godot::Transform3D newCameraT = viewer->worldToGlobal(map);
	//godot::Transform3D viewerT = viewer->getInternalGlobalTransform().affine_inverse();
	//godot::Transform3D newCameraT = viewerT * t;
	set_global_transform(newCameraT);
}

godot::Vector3 GDN_TheWorld_Camera::getWorldPos(void)
{
	return getWorldTransform().get_origin();
}

void GDN_TheWorld_Camera::setWorldPos(godot::Vector3 mapPos)
{
	godot::Transform3D mapTransform(get_global_transform().basis, mapPos);
	setWorldTransform(mapTransform);
}

void GDN_TheWorld_Camera::_physics_process(double _delta)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("_physics_process 1 ") + __FUNCTION__, "ALL");

	// To activate _process method add this Node to a Godot Scene
	//Globals()->debugPrint("GDN_TheWorld_Camera::_physics_process");

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	if (!viewer->worldMode())
		return;
	
	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (IS_EDITOR_HINT())
	{
		calcYawPitchRoll();
		updateAltitudeFromTerrain(false);
		return;
	}

	if (!isActiveCamera())
	{
		calcYawPitchRoll();
		updateAltitudeFromTerrain(false);
		return;
	}

	if (!is_current())
	{
		calcYawPitchRoll();
		updateAltitudeFromTerrain(false);
		return;
	}

	Input* input = Input::get_singleton();

	if (input->is_action_pressed("ui_mouse_button_right") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
	{
		//if (!m_rotateCameraOn)
		//	Globals()->debugPrint("Rotate ON");
		m_rotateCameraOn = true;
	}
	else
	{
		//if (m_rotateCameraOn)
		//	Globals()->debugPrint("Rotate OFF");
		m_rotateCameraOn = false;
	}

	//input = Input::get_singleton();
	if (input->is_action_pressed("ui_mouse_button_left") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		m_mouseShiftCameraOn = true;
	else
		m_mouseShiftCameraOn = false;

	////input = Input::get_singleton();
	//if (input->is_action_pressed("ui_mouse_button_mid") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
	//	m_shiftVertCameraOn = true;
	//else
	//	m_shiftVertCameraOn = false;

	if (input->is_action_pressed("ui_shift"))
		m_shiftPressed = true;
	else
		m_shiftPressed = false;

	if (input->is_action_pressed("ui_ctrl"))
		m_ctrlPressed = true;
	else
		m_ctrlPressed = false;

	if (input->is_action_pressed("ui_alt"))
		m_altPressed = true;
	else
		m_altPressed = false;

	if (!trackingPlayer())
	{
		if (input->is_action_pressed("ui_forward") /* && !m_altPressed*/)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				m_numMoveStepForward--;
				m_updateCameraRequired = true;
				//m_forwardMovementOn = true;
			}
		}
		//else
		//	m_forwardMovementOn = false;

		if (input->is_action_pressed("ui_backward") /* && !m_altPressed*/)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				m_numMoveStepForward++;
				m_updateCameraRequired = true;
				//m_backwardMovementOn = true;
			}
		}
		//else
		//	m_backwardMovementOn = false;

		if (input->is_action_pressed("ui_left") && !m_altPressed)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				if (input->is_key_pressed(godot::KEY_LEFT) && m_ctrlPressed)
					m_numLongMoveStepLeft++;
				else
					m_numMoveStepLeft++;
				m_updateCameraRequired = true;
				//m_leftMovementOn = true;
			}
		}
		//else
		//	m_leftMovementOn = false;

		if (input->is_action_pressed("ui_right") && !m_altPressed)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				if (input->is_key_pressed(godot::KEY_RIGHT) && m_ctrlPressed)
					m_numLongMoveStepLeft--;
				else
					m_numMoveStepLeft--;
				m_updateCameraRequired = true;
				//m_rightMovementOn = true;
			}
		}
		//else
		//	m_rightMovementOn = false;

		if (input->is_action_pressed("ui_up") && !m_altPressed)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				if (input->is_key_pressed(godot::KEY_UP) && m_ctrlPressed)
					m_numLongMoveStepForward--;
				else
					m_numMoveStepUp++;
				m_updateCameraRequired = true;
				//m_upwardMovementOn = true;
			}
		}
		//else
		//	m_upwardMovementOn = false;

		if (input->is_action_pressed("ui_down") && !m_altPressed)
		{
			if (viewer->terrainShiftPermitted() /* && !viewer->editMode()*/)
			{
				if (input->is_key_pressed(godot::KEY_DOWN) && m_ctrlPressed)
					m_numLongMoveStepForward++;
				else
					m_numMoveStepUp--;
				m_updateCameraRequired = true;
				//m_downwardMovementOn = true;
			}
		}
		//else
		//	m_downwardMovementOn = false;
		// 

		//if (input->is_action_pressed("ui_mouse_wheel_up"))
		//{
		//	if (viewer->terrainShiftPermitted())
		//	{
		//		m_numMoveStepForward--;
		//		m_updateCameraRequired = true;
		//	}
		//}

		//if (input->is_action_pressed("ui_mouse_wheel_down"))
		//{
		//	if (viewer->terrainShiftPermitted())
		//	{
		//		m_numMoveStepForward++;
		//		m_updateCameraRequired = true;
		//	}
		//}
	}


	{
		bool b = updateCamera(_delta);
		calcYawPitchRoll();
	}
}

void GDN_TheWorld_Camera::calcYawPitchRoll(void)
{
	godot::Vector3 euler = get_global_transform().basis.get_euler();
	m_pitchEuler = (float)euler.x;
	m_yawEuler = (float)euler.y;		// 0 = camera aligned with z axis pointing to decresing z (north), - PI/2 = camera aligned with x axis pointing to incrasing x (east), 
	// - PI or PI = camera aligned with z axis pointing to increasing z (south), PI/2 = camera aligned with x axis pointing to decreasing X (west)
	m_rollEuler = (float)euler.z;
	float angleFromNorthRad = (float)(m_yawEuler < 0 ? -m_yawEuler : (m_yawEuler == 0 ? 0 : TheWorld_Utils::kPi * 2 - m_yawEuler));
	m_angleFromNorthDeg = (angleFromNorthRad * 180) / TheWorld_Utils::kPi;		// transform angle from radiant to degree
}

void GDN_TheWorld_Camera::_input(const Ref<godot::InputEvent>& event)
{
	//Globals()->debugPrint("GDN_TheWorld_Camera::_input: " + event->as_text());

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	if (!viewer->worldMode())
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	if (!isActiveCamera())
	{
		return;
	}

	if (!is_current())
	{
		return;
	}

	if (viewer->mouseInsideMainEditPanel())
		return;

	const InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<InputEventMouseMotion>(event.ptr());
	if (eventMouseMotion != nullptr)
	{
		//Globals()->debugPrint("GDN_TheWorld_Camera::_input: " + event->as_text());
		if (m_rotateCameraOn)
		{
			m_mouseRelativePosToRotate = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
		if (m_mouseShiftCameraOn)
		{
			m_mouseRelativePosToShift = eventMouseMotion->get_relative();
			m_updateCameraRequired = true;
		}
		//if (m_shiftVertCameraOn)
		//{
		//	m_mouseRelativePosToShiftVert = eventMouseMotion->get_relative();
		//	m_updateCameraRequired = true;
		//}
	}

	if (!trackingPlayer())
	{
		if (event->is_action_pressed("ui_mouse_wheel_up"))
		{
			if (viewer->terrainShiftPermitted())
			{
				m_numMoveStepForward--;
				m_updateCameraRequired = true;
			}
		}

		if (event->is_action_pressed("ui_mouse_wheel_down"))
		{
			if (viewer->terrainShiftPermitted())
			{
				m_numMoveStepForward++;
				m_updateCameraRequired = true;
			}
		}
	}
}

void GDN_TheWorld_Camera::activateCamera(void)
{
	if (IS_EDITOR_HINT())
		return;

	if (!isActiveCamera())
	{
		GDN_TheWorld_Camera* activeCamera = (GDN_TheWorld_Camera*)getActiveCamera();
		if (activeCamera)
		{
			activeCamera->set_process_input(false);
			activeCamera->set_process(false);
			activeCamera->set_physics_process(false);
			activeCamera->remove_from_group(GD_ACTIVE_CAMERA_GROUP);
			activeCamera->notifyActiveCameraFlag(false);
		}

		make_current();
		//godot::Camera3D* cam = get_viewport()->get_camera_3d();
		set_process_input(true);
		set_process(true);
		set_physics_process(true);
		add_to_group(GD_ACTIVE_CAMERA_GROUP);
		notifyActiveCameraFlag(true);
	}
}

void GDN_TheWorld_Camera::deactivateCamera(void)
{
	if (IS_EDITOR_HINT())
		return;

	if (isActiveCamera())
	{
		clear_current(true);
		set_process_input(false);
		set_process(false);
		set_physics_process(false);
		remove_from_group(GD_ACTIVE_CAMERA_GROUP);
		notifyActiveCameraFlag(false);
	}
}

bool GDN_TheWorld_Camera::isActiveCamera(void) const
{
	bool b = is_current();
	return m_isActive;
	//return is_in_group(GD_ACTIVE_CAMERA_GROUP);
}

void GDN_TheWorld_Camera::notifyActiveCameraFlag(bool active)
{
	m_isActive = active;
}


GDN_TheWorld_Camera* GDN_TheWorld_Camera::getActiveCamera(void)
{
	if (not is_inside_tree())
		return NULL;
	SceneTree* scene = get_tree();
	if (scene == NULL)
		return NULL;

	godot::Array camArray = scene->get_nodes_in_group(GD_ACTIVE_CAMERA_GROUP);

	
	if (camArray.size() > 1)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "Too many active cameras");
		return NULL;
	}

	if (camArray.size() == 1)
		return godot::Object::cast_to<GDN_TheWorld_Camera>(camArray[0]);
	else
		return NULL;
}

//bool GDN_TheWorld_Camera::initPlayerCamera(void)
//{
//	set_name("Camera");
//
//	m_PlayerCamera = true;
//
//	return true;
//}
//
//bool GDN_TheWorld_Camera::initOtherEntityCamera(void)
//{
//	set_name("Camera");
//
//	m_OtherEntityCamera = true;
//
//	return true;
//}

void GDN_TheWorld_Camera::set_cameraFar(void)
{
	float quadrantSize = Globals()->heightmapSizeInWUs();
	float thresholdUpdateCameraFar = DISTANCE_THRESHOLD_TO_UPDATE_CAMERA_FAR;
	godot::Vector3 cameraGlobalPosition = get_global_position();
	if (m_forceRecalcCameraFar or cameraGlobalPosition.distance_to(m_savedCameraPosAfterSetFar) > thresholdUpdateCameraFar)
	{
		float _far = quadrantSize * (Globals()->Viewer()->maxActualDepthQuadInAllDirections() + 1) * 1.5f;	// zfar at 0 altitude
		_far = sqrtf(powf(_far, 2.0) + powf(cameraGlobalPosition.y, 2.0));
		Camera3D::set_far(_far);

#define MIN_CAMERA_NEAR	0.5f
#define MAX_CAMERA_NEAR	100.0f
		// at great distance from sea we need to increment near plane in camera frustum to prevent z-figthing
		float _near = MIN_CAMERA_NEAR;
		if (m_lastCameraAltitudeFromTerrain == INVALID_CAMERA_ALTITUDE or m_lastCameraAltitudeFromTerrain > 10000.0f)
			_near = MAX_CAMERA_NEAR;
		else if (m_lastCameraAltitudeFromTerrain > 1000.0f and m_lastCameraAltitudeFromTerrain <= 10000.0f)
		{
			_near = lerp(MIN_CAMERA_NEAR, MAX_CAMERA_NEAR, m_lastCameraAltitudeFromTerrain / 10000.0f);
		}
		Camera3D::set_near(_near);

		PLOG_DEBUG << "Camera znear=" << _near;
		PLOG_DEBUG << "Camera zfar=" << _far;

		m_forceRecalcCameraFar = false;
		m_savedCameraPosAfterSetFar = cameraGlobalPosition;
	}

	//Camera3D::set_far(30000.0);
	//Camera3D::set_far(20000.0);
	//Camera3D::set_far(10000.0);
}

bool GDN_TheWorld_Camera::initCameraInWorld(godot::Vector3 cameraPos, godot::Vector3 lookAt, float cameraYaw, float cameraPitch, float cameraRoll)
{
	//set_name("WorldCamera");

	activateCamera();

	// cameraPos and lookAt are in WorldNode local coordinates and must be transformed in godot global coordinates
	//godot::Vector3 worldNodePosGlobalCoord = Globals()->Viewer()->getWorldNode()->get_global_transform().origin;
	//godot::Vector3 cameraPosGlobalCoord = cameraPos + worldNodePosGlobalCoord;
	//godot::Vector3 lookAtGlobalCoord = lookAt + worldNodePosGlobalCoord;
	godot::Vector3 cameraPosGlobalCoord = Globals()->Viewer()->worldToGlobal(godot::Transform3D(godot::Basis(), cameraPos)).origin;
	godot::Vector3 lookAtGlobalCoord = Globals()->Viewer()->worldToGlobal(godot::Transform3D(godot::Basis(), lookAt)).origin;;
	if (cameraPosGlobalCoord != lookAtGlobalCoord)
		Camera3D::look_at_from_position(cameraPosGlobalCoord, lookAtGlobalCoord, godot::Vector3(0, 1, 0));

	setYaw(cameraYaw, false);
	setPitch(cameraPitch, false);
	setRoll(cameraRoll, false);

	//float z_near = Camera::get_znear();
	//float z_far = Camera::get_zfar();
	//float size = Camera::get_size();
	//godot::Vector2 offset = Camera::get_frustum_offset();

	//set_cameraFar();
	Camera3D::set_far(30000.0);

	m_WorldCamera = true;

	return true;
}

bool GDN_TheWorld_Camera::updateCamera(double _delta)
{
	bool ret;

	if (trackingPlayer())
		ret = updatePlayerCamera(_delta);
	else
		ret = updateFreeLookCamera(_delta);

	//updateCameraUnderWater();

	return ret;
}

void GDN_TheWorld_Camera::updateCameraUnderWater(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	if (viewer == nullptr)
		return;

	if (!viewer->worldMode())
		return;

	if (m_postProcessingUnderWaterNode != nullptr)
	{
		if (m_postProcessingUnderWaterOverrideNode == nullptr or !m_postProcessingUnderWaterOverrideNode->is_visible())
			m_postProcessingUnderWaterNode->set_visible(true);
		else
			m_postProcessingUnderWaterNode->set_visible(false);
	}

	if (m_seaWaveMapTexture != nullptr)
	{
		if (m_seaWaveMapImage == nullptr)
			m_seaWaveMapImage = m_seaWaveMapTexture->get_image();
		if (m_seaWaveMapImage != nullptr)
		{
			// WARNING: Noise texture must be SEAMLESS and Normalized

			GDN_TheWorld_Globals* globals = Globals();
			GDN_TheWorld_Viewer* viewer = globals->Viewer();
			
			godot::Vector3 pos = get_global_position();
			float shaderParamSeaFlatSeaHeightApplied = viewer->shaderParamSeaFlatSeaHeightApplied();
			if (pos.y > shaderParamSeaFlatSeaHeightApplied - THEWORLD_SEA_MAX_WAVE_HEIGHT and pos.y < shaderParamSeaFlatSeaHeightApplied + THEWORLD_SEA_MAX_WAVE_HEIGHT)
			{
				// sea height in range 0.0 / 1.0 from noise texture moving uv with vertex global position and time 
				float uv_factor = float(globals->shaderParamGlobalTimeApplied() * viewer->shaderParamSeaTimeScaleApplied() * viewer->shaderParamSeaRoughFactorApplied());
				godot::Vector2 uv = godot::Vector2((pos.x / viewer->shaderParamSeaNoiseScaleApplied()) + uv_factor, (pos.z / viewer->shaderParamSeaNoiseScaleApplied()) + uv_factor);
				// force UV in 0.0/1.0 range
				godot::Vector2 uv_fractional_part = uv - godot::Vector2(float(int(uv.x)), float(int(uv.y)));
				if (uv_fractional_part.x < 0)
					uv_fractional_part.x += 1.0;
				if (uv_fractional_part.y < 0)
					uv_fractional_part.y += 1.0;
				//int32_t w = m_seaWaveMapImage->get_width();
				//int32_t h = m_seaWaveMapImage->get_height();
				godot::Vector2i pixelCoord(int(m_seaWaveMapImage->get_width() * uv_fractional_part.x), int(m_seaWaveMapImage->get_height() * uv_fractional_part.y));
				godot::Color c = m_seaWaveMapImage->get_pixel(pixelCoord.x, pixelCoord.y);
				float seaHeight = c.r;

				float scaleFactor = viewer->shaderParamSeaHeightScaleApplied() * viewer->shaderParamSeaRoughFactorApplied();

				// Attenuation of waves height when camera is between start_camera_altitude_for_waves_attenuation (max waves height) and end_camera_altitude_for_waves (null waves height)
				float shaderParamSeaStartCamAltForWavesAttenuationApplied = viewer->shaderParamSeaStartCamAltForWavesAttenuationApplied();
				if (pos.y > shaderParamSeaFlatSeaHeightApplied + shaderParamSeaStartCamAltForWavesAttenuationApplied)
					scaleFactor = scaleFactor - (scaleFactor * (pos.y - shaderParamSeaStartCamAltForWavesAttenuationApplied) / (viewer->shaderParamSeaEndCamAltForWavesApplied() - shaderParamSeaStartCamAltForWavesAttenuationApplied));

				if (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE)
				{
					float terrainHeigth = pos.y - m_lastCameraAltitudeFromTerrain;
					float shaderParamSeaStartTerrDepthForWavesAttenuationApplied = viewer->shaderParamSeaStartTerrDepthForWavesAttenuationApplied();
					// Attenuation of height waves at lower depth (near the coast)
					if (terrainHeigth > shaderParamSeaFlatSeaHeightApplied - shaderParamSeaStartTerrDepthForWavesAttenuationApplied)
						scaleFactor *= -TheWorld_Viewer_Utils::Utils::clamp<float>(terrainHeigth, -shaderParamSeaStartTerrDepthForWavesAttenuationApplied, 0) / shaderParamSeaStartTerrDepthForWavesAttenuationApplied;

					float shaderParamSeaScaleOffsetThresholdApplied = viewer->shaderParamSeaScaleOffsetThresholdApplied();
					if (terrainHeigth >= shaderParamSeaFlatSeaHeightApplied - shaderParamSeaScaleOffsetThresholdApplied) {
						float scale_offset = TheWorld_Viewer_Utils::Utils::clamp<float>(shaderParamSeaScaleOffsetThresholdApplied + terrainHeigth, 0, shaderParamSeaScaleOffsetThresholdApplied);
						scaleFactor += scale_offset;
					}
				}

				seaHeight = shaderParamSeaFlatSeaHeightApplied + (seaHeight * scaleFactor) - (scaleFactor / 3);

				if (seaHeight > pos.y)
					m_cameraUnderWater = true;
				else
					m_cameraUnderWater = false;
			}
			else
			{
				if (pos.y > shaderParamSeaFlatSeaHeightApplied + THEWORLD_SEA_MAX_WAVE_HEIGHT)
					m_cameraUnderWater = false;
				else
					m_cameraUnderWater = true;
			}

			viewer->shaderParamCameraUnderWaterApplied(m_cameraUnderWater);
		}
	}
}

bool GDN_TheWorld_Camera::updatePlayerCamera(double _delta)
{
	if (m_rotateCameraOn)
	{
		if (m_mouseRelativePosToRotate != godot::Vector2(0, 0))
		{
			float sensitivity = m_sensitivity;
			if (m_shiftPressed)
				sensitivity /= 4;
			if (m_ctrlPressed)
				sensitivity *= 2;
			m_mouseRelativePosToRotate *= sensitivity;
			m_yaw = m_yaw * m_smoothness + (float)m_mouseRelativePosToRotate.x * (1.0F - m_smoothness);
			m_pitch = m_pitch * m_smoothness + (float)m_mouseRelativePosToRotate.y * (1.0F - m_smoothness);
			m_mouseRelativePosToRotate = godot::Vector2(0, 0);

			if (m_yawLimit < 360)
				m_yaw = TheWorld_Viewer_Utils::Utils::clamp(m_yaw, -m_yawLimit - m_totalYaw, m_yawLimit - m_totalYaw);
			if (m_pitchLimit < 360)
				m_pitch = TheWorld_Viewer_Utils::Utils::clamp(m_pitch, -m_pitchLimit - m_totalPitch, m_pitchLimit - m_totalPitch);
			m_totalYaw *= m_yaw;
			m_totalPitch *= m_pitch;

			float pitch = Math::deg_to_rad(-m_pitch);
			if ((pitch + get_rotation().x) > kPi / 2)
				pitch = (kPi / 2) - (float)get_rotation().x;
			if ((pitch + get_rotation().x) < -(kPi / 2))
				pitch = (-(kPi / 2)) - (float)get_rotation().x;

			rotate_y(Math::deg_to_rad(-m_yaw));
			rotate_object_local(godot::Vector3(1, 0, 0), pitch);

			/*if (m_lastYaw != m_yaw)
				Godot::print("Camera Yaw: " + String(std::to_string(m_yaw).c_str()));
			if (m_lastPitch != m_pitch)
				Godot::print("Camera Pitch: " + String(std::to_string(m_pitch).c_str()));*/

			m_lastYaw = m_yaw;
			m_lastPitch = m_pitch;
		}

		godot::Transform3D cameraTransform = get_global_transform();
		godot::Transform3D target = m_trackedPlayer->get_global_transform();
		godot::Vector3 TargetToCameraDir = (cameraTransform.origin - target.origin).normalized();
		set_global_position(target.origin + TargetToCameraDir * m_trackingOffset.length());
	}
	else
	{
#define STOP_ANGLE_TO_CAMERA_MOVE		0.05f
#define STOP_DISTANCE_TO_CAMERA_MOVE	1.0f

		//if (!m_pos_toTrackInitialized)
		//{
		//	m_posToTrack = target;
		//	m_pos_toTrackInitialized = true;
		//}
		//else
		//{
		//	if (target.origin.distance_to(m_posToTrack.origin) > OFFSET_DISTANCE_TO_CHANGE_TRACKED_TARGET)
		//	{
		//		m_posToTrack = target;
		//	}
		//}

		godot::Transform3D target = m_trackedPlayer->get_global_transform();
		godot::Transform3D camera = get_global_transform();
		godot::Vector3 trackingOffset = target.basis.xform(m_trackingOffset);
		godot::Transform3D desideredCameraTarget = target.translated(trackingOffset);
		float cameraDistanceFromNewCameraTarget = (float)camera.origin.distance_to(desideredCameraTarget.origin);
		godot::Vector3 rayToTarget = target.origin - camera.origin;
		godot::Vector3 desideredRayToTarget = target.origin - desideredCameraTarget.origin;
		float angle = (float)rayToTarget.angle_to(desideredRayToTarget);
		if (cameraDistanceFromNewCameraTarget > STOP_DISTANCE_TO_CAMERA_MOVE || angle > STOP_ANGLE_TO_CAMERA_MOVE)
		{
			//godot::UtilityFunctions::print((std::string("Distance=") + std::to_string(cameraDistanceFromNewCameraTarget) + " angle=" + std::to_string(angle)).c_str());
			godot::Transform3D targetCameraTransform = get_global_transform().interpolate_with(desideredCameraTarget, float(getCameraTrackingSpeed() * _delta));
			set_global_position(targetCameraTransform.origin);
			look_at(target.origin, Vector3Up);
		}
	}

	updateAltitudeFromTerrain(false);

	return true;
}


bool GDN_TheWorld_Camera::updateFreeLookCamera(double _delta)
{
	if (!m_updateCameraRequired)
	{
		updateAltitudeFromTerrain(false);
		return true;
	}

	m_updateCameraRequired = false;

	//updateAltitude(true);
	godot::Vector3 savedCameraPos = get_global_position();
	float savedLastCameraAltitudeComputed = m_lastCameraAltitudeFromTerrain;

	if (m_mouseRelativePosToRotate != godot::Vector2(0, 0))
	{
		float sensitivity = m_sensitivity;
		if (m_shiftPressed)
			sensitivity /= 4;
		if (m_ctrlPressed)
			sensitivity *= 2;
		m_mouseRelativePosToRotate *= sensitivity;
		m_yaw = m_yaw * m_smoothness + (float)m_mouseRelativePosToRotate.x * (1.0F - m_smoothness);
		m_pitch = m_pitch * m_smoothness + (float)m_mouseRelativePosToRotate.y * (1.0F - m_smoothness);
		m_mouseRelativePosToRotate = godot::Vector2(0, 0);

		if (m_yawLimit < 360)
			m_yaw = TheWorld_Viewer_Utils::Utils::clamp(m_yaw, -m_yawLimit - m_totalYaw, m_yawLimit - m_totalYaw);
		if (m_pitchLimit < 360)
			m_pitch = TheWorld_Viewer_Utils::Utils::clamp(m_pitch, -m_pitchLimit - m_totalPitch, m_pitchLimit - m_totalPitch);
		m_totalYaw *= m_yaw;
		m_totalPitch *= m_pitch;

		float pitch = Math::deg_to_rad(-m_pitch);
		if ((pitch + get_rotation().x) > kPi / 2)
			pitch = (kPi / 2) - (float)get_rotation().x;
		if ((pitch + get_rotation().x) < -(kPi / 2))
			pitch = (-(kPi / 2)) - (float)get_rotation().x;

		rotate_y(Math::deg_to_rad(-m_yaw));
		rotate_object_local(godot::Vector3(1, 0, 0), pitch);

		/*if (m_lastYaw != m_yaw)
			Godot::print("Camera Yaw: " + String(std::to_string(m_yaw).c_str()));
		if (m_lastPitch != m_pitch)
			Godot::print("Camera Pitch: " + String(std::to_string(m_pitch).c_str()));*/

		m_lastYaw = m_yaw;
		m_lastPitch = m_pitch;
	}

	if (m_mouseRelativePosToShift != godot::Vector2(0, 0))
	{
		float sensitivity = m_sensitivity;
		if (m_ctrlPressed)
			sensitivity *= 2;
		m_mouseRelativePosToShift *= sensitivity;

		translate_object_local(godot::Vector3(-m_mouseRelativePosToShift.x, 0, -m_mouseRelativePosToShift.y));

		m_mouseRelativePosToShift = godot::Vector2(0, 0);
	}

	//if (m_mouseRelativePosToShiftVert != godot::Vector2(0, 0))
	//{
	//	m_mouseRelativePosToShiftVert *= m_sensitivity;

	//	translate_object_local(godot::Vector3(0, 0, -m_mouseRelativePosToShiftVert.y));

	//	m_mouseRelativePosToShiftVert = godot::Vector2(0, 0);
	//}

	if (m_numLongMoveStepForward)
	{
		translate_object_local(godot::Vector3(0, 0, m_numLongMoveStepForward * m_wheelVeryFastVelocity));
		m_numLongMoveStepForward = 0;
	}

	if (m_numLongMoveStepLeft)
	{
		translate_object_local(godot::Vector3(-m_numLongMoveStepLeft * m_wheelVeryFastVelocity, 0, 0));
		m_numLongMoveStepLeft = 0;
	}

	if (m_numMoveStepForward)
	{
		if (m_altPressed)
		{
			if (m_shiftPressed)
			{
				translate_object_local(godot::Vector3(0, -m_numMoveStepForward * m_wheelSlowVelocity, 0));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(godot::Vector3(0, -m_numMoveStepForward * m_wheelFastVelocity, 0));
			}
			else
			{
				translate_object_local(godot::Vector3(0, -m_numMoveStepForward * m_wheelNormalVelocity, 0));
			}
		}
		else
		{
			if (m_shiftPressed)
			{
				translate_object_local(godot::Vector3(0, 0, m_numMoveStepForward * m_wheelSlowVelocity));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(godot::Vector3(0, 0, m_numMoveStepForward * m_wheelFastVelocity));
			}
			else
			{
				translate_object_local(godot::Vector3(0, 0, m_numMoveStepForward * m_wheelNormalVelocity));
			}
		}
		m_numMoveStepForward = 0;
	}

	if (m_numMoveStepLeft)
	{
		//if (m_altPressed)
		//{
		//	Transform t = get_global_transform();
		//	if (m_shiftPressed)
		//	{
		//		t.origin += godot::Vector3(-m_numMoveStepLeft * m_wheelSlowVelocity, 0, 0);
		//	}
		//	else if (m_ctrlPressed)
		//	{
		//		t.origin += godot::Vector3(-m_numMoveStepLeft * m_wheelFastVelocity, 0, 0);
		//	}
		//	else
		//	{
		//		t.origin += godot::Vector3(-m_numMoveStepLeft * m_wheelNormalVelocity, 0, 0);
		//	}
		//	set_global_transform(t);
		//}
		//else
		//{
			if (m_shiftPressed)
			{
				translate_object_local(godot::Vector3(-m_numMoveStepLeft * m_wheelSlowVelocity, 0, 0));
			}
			else if (m_ctrlPressed)
			{
				translate_object_local(godot::Vector3(-m_numMoveStepLeft * m_wheelFastVelocity, 0, 0));
			}
			else
			{
				translate_object_local(godot::Vector3(-m_numMoveStepLeft * m_wheelNormalVelocity, 0, 0));
			}
		//}
		m_numMoveStepLeft = 0;
	}

	if (m_numMoveStepUp)
	{
		Transform3D t = get_global_transform();
		if (m_shiftPressed)
		{
			t.origin += godot::Vector3(0, m_numMoveStepUp * m_wheelSlowVelocity, 0);
		}
		else if (m_ctrlPressed)
		{
			t.origin += godot::Vector3(0, m_numMoveStepUp * m_wheelFastVelocity, 0);
		}
		else
		{
			t.origin += godot::Vector3(0, m_numMoveStepUp * m_wheelNormalVelocity, 0);
		}
		set_global_transform(t);
		m_numMoveStepUp = 0;
	}

	updateAltitudeFromTerrain(true);

	if (m_lastCameraAltitudeFromTerrain == INVALID_CAMERA_ALTITUDE)
	{
		if (savedLastCameraAltitudeComputed != INVALID_CAMERA_ALTITUDE)
		{
			if (savedLastCameraAltitudeComputed < MAX_ALTITUDE_DETECTED - (MAX_ALTITUDE_DETECTED * 0.1f))	// Far above terrain we leave camera move else we force camera still
			{
				set_global_position(savedCameraPos);
				m_lastCameraAltitudeFromTerrain = savedLastCameraAltitudeComputed;
			}
		}
	}
	else
	{
#define MIN_ALTITUDE_ALLOWED_FOR_CAMERA	3.0f
		godot::Vector3 postMoveCameraPos = get_global_position();
		if (postMoveCameraPos.y < savedCameraPos.y - (m_lastCameraAltitudeFromTerrain - MIN_ALTITUDE_ALLOWED_FOR_CAMERA))
		{
			set_global_position(godot::Vector3(postMoveCameraPos.x, savedCameraPos.y - (m_lastCameraAltitudeFromTerrain - MIN_ALTITUDE_ALLOWED_FOR_CAMERA), postMoveCameraPos.z));
			m_lastCameraAltitudeFromTerrain = MIN_ALTITUDE_ALLOWED_FOR_CAMERA;
		}
	}

	return true;
}

float GDN_TheWorld_Camera::cameraMaxAltitudeFromTerrainDetected(void) const
{
	return MAX_ALTITUDE_DETECTED;
}

void GDN_TheWorld_Camera::updateAltitudeFromTerrain(bool force)
{
	if (Globals()->status() != TheWorldStatus::worldDeployed)
		return;
	
	//return;
	if (force)
		m_forceRecalcCameraAltitude = true;
	
	godot::Vector3 cameraGlobalPosition = get_global_position();
	
	float thresholdToUpdateAltitude = DISTANCE_THRESHOLD_TO_UPDATE_ALTITUDE;
	//if (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE && m_lastCameraAltitudeFromTerrain <= thresholdToUpdateAltitude)
	//	thresholdToUpdateAltitude = 1.0f;

	if (m_lastCameraAltitudeFromTerrain == INVALID_CAMERA_ALTITUDE || m_forceRecalcCameraAltitude || cameraGlobalPosition.distance_to(m_savedCameraPosAfterCalcAltitude) > thresholdToUpdateAltitude)
	{
		m_forceRecalcCameraAltitude = false;

		godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
		godot::Dictionary rayArray;
		godot::Vector3 rayOrigin = cameraGlobalPosition;
		godot::Vector3 rayEnd = rayOrigin + godot::Vector3(0.0f, -MAX_ALTITUDE_DETECTED, 0.0f);
		godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
		rayArray = spaceState->intersect_ray(query);
		if (rayArray.has("position"))
		{
			godot::Vector3 lastComputedCameraVerticalOnTerrain = rayArray["position"];
			m_lastCameraAltitudeFromTerrain = (float)abs(cameraGlobalPosition.y - lastComputedCameraVerticalOnTerrain.y);
		}
		else
		{
			godot::Dictionary rayArray;
			rayOrigin = cameraGlobalPosition;
			rayEnd = rayOrigin + godot::Vector3(0.0f, MAX_ALTITUDE_DETECTED, 0.0f);
			godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
			rayArray = spaceState->intersect_ray(query);
			if (rayArray.has("position"))
			{
				godot::Vector3 lastComputedCameraVerticalOnTerrain = rayArray["position"];
				m_lastCameraAltitudeFromTerrain = (float)(cameraGlobalPosition.y - lastComputedCameraVerticalOnTerrain.y);
			}
			else
				m_lastCameraAltitudeFromTerrain = INVALID_CAMERA_ALTITUDE;
		}

		m_savedCameraPosAfterCalcAltitude = cameraGlobalPosition;
	}

	set_cameraFar();
	//Camera3D::set_far(30000.0);
}

GDN_TheWorld_Globals* GDN_TheWorld_Camera::Globals(bool useCache)
{
	if (m_globals == NULL || !useCache)
	{
		if (not is_inside_tree())
			return NULL;
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_globals = Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}

godot::Ref<godot::ArrayMesh> GDN_TheWorld_Camera::DrawViewFrustum(Color c)
{
	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();

	float m_FarOffset = (float)godot::Camera3D::get_far();
	float m_NearOffset = (float)godot::Camera3D::get_near();
	float m_ScreenWidth = (float)godot::Camera3D::get_viewport()->get_visible_rect().get_size().x;
	float m_ScreenHeight = (float)godot::Camera3D::get_viewport()->get_visible_rect().get_size().y;

	float m_AspectRatio = (m_ScreenWidth > m_ScreenHeight) ? (m_ScreenWidth / m_ScreenHeight) : (m_ScreenHeight / m_ScreenWidth);

	float m_FarHeight = 2.0f * ((float)tan(godot::Camera3D::get_fov() / 2.0f)) * m_FarOffset;
	float m_FarWidth = m_FarHeight * m_AspectRatio;

	m_FarHeight = m_FarHeight + ((m_FarHeight / 7) * 3);
	m_FarWidth = m_FarWidth + ((m_FarWidth / 7) * 3);

	godot::Vector3 m_Forward = godot::Camera3D::get_global_transform().origin + Vector3Forward * m_FarOffset;

	//--> Near World
	godot::Vector3 m_NearTopLeft = godot::Camera3D::project_position(godot::Vector2(0, 0), godot::Camera3D::get_near());
	godot::Vector3 m_NearTopRight = godot::Camera3D::project_position(godot::Vector2(m_ScreenWidth, 0), godot::Camera3D::get_near());
	godot::Vector3 m_NearBottomLeft = godot::Camera3D::project_position(godot::Vector2(0, m_ScreenHeight), godot::Camera3D::get_near());
	godot::Vector3 m_NearBottomRight = godot::Camera3D::project_position(godot::Vector2(m_ScreenWidth, m_ScreenHeight), godot::Camera3D::get_near());
	
	//--> Far LOCAL
	godot::Vector3 m_FarTopLeft = m_Forward + (Vector3Up * m_FarHeight / 2.0f) - (Vector3Right * m_FarWidth / 2.0f);
	godot::Vector3 m_FarTopRight = m_Forward + (Vector3Up * m_FarHeight / 2.0f) + (Vector3Right * m_FarWidth / 2.0f);
	godot::Vector3 m_FarBottomLeft = m_Forward - (Vector3Up * m_FarHeight / 2.0f) - (Vector3Right * m_FarWidth / 2.0f);
	godot::Vector3 m_FarBottomRight = m_Forward - (Vector3Up * m_FarHeight / 2.0f) + (Vector3Right * m_FarWidth / 2.0f);

	//--> Far WORLD
	m_FarTopLeft = godot::Camera3D::get_transform().xform(m_FarTopLeft);
	m_FarTopRight = godot::Camera3D::get_transform().xform(m_FarTopRight);
	m_FarBottomRight = godot::Camera3D::get_transform().xform(m_FarBottomRight);
	m_FarBottomLeft = godot::Camera3D::get_transform().xform(m_FarBottomLeft);
	
	godot::PackedVector3Array positions;
	godot::PackedColorArray colors;
	godot::PackedInt32Array indices;

	//--> Draw lines
	positions.append(m_FarTopLeft);			colors.append(c);	// index 0
	positions.append(m_FarTopRight);		colors.append(c);	// index 1
	positions.append(m_FarBottomRight);		colors.append(c);	// index 2
	positions.append(m_FarBottomLeft);		colors.append(c);	// index 3
	positions.append(m_NearTopLeft);		colors.append(c);	// index 4
	positions.append(m_NearTopRight);		colors.append(c);	// index 5
	positions.append(m_NearBottomRight);	colors.append(c);	// index 6
	positions.append(m_NearBottomLeft);		colors.append(c);	// index 7

	// Far rect lines
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// Near rect lines
	indices.append(4); indices.append(5);
	indices.append(5); indices.append(6);
	indices.append(6); indices.append(7);
	indices.append(7); indices.append(4);
	// Left lines        
	indices.append(4); indices.append(0);
	indices.append(7); indices.append(3);
	// Right lines        
	indices.append(5); indices.append(1);
	indices.append(6); indices.append(2);

	godot::Array arrays;
	arrays.resize(godot::ArrayMesh::ARRAY_MAX);
	arrays[godot::ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[godot::ArrayMesh::ARRAY_COLOR] = colors;
	arrays[godot::ArrayMesh::ARRAY_INDEX] = indices;

	if (viewer != nullptr)
		viewer->getMainProcessingMutex().lock();
	godot::Ref <godot::ArrayMesh> mesh = memnew(godot::ArrayMesh);
	if (viewer != nullptr)
		viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}