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
#include <godot_cpp/classes/fast_noise_lite.hpp>
#pragma warning (pop)

#include <math.h> 

using namespace godot;

constexpr auto DISTANCE_THRESHOLD_TO_UPDATE_ALTITUDE = 5.0f;
constexpr auto DISTANCE_THRESHOLD_TO_UPDATE_CAMERA_FAR = 50.0f;
constexpr auto MAX_ALTITUDE_DETECTED = 6000.0f;

constexpr auto POST_PROCESSING_UNDER_WATER_NODE = "PostProcessingUnderWater";
constexpr auto POST_PROCESSING_UNDER_WATER_OVERRIDE_NODE = "PostProcessingUnderWaterOverride";

void GDN_TheWorld_MapCamera::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("get_camera_world_transform"), &GDN_TheWorld_MapCamera::getCameraWorldTransform);
	ClassDB::bind_method(D_METHOD("get_zoom_factor"), &GDN_TheWorld_MapCamera::getZoomFactor);
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

	if (mouseMovementSuspended())
		m_mouseShiftCameraOn = false;
	else
	{
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
	//setZoom(m_zoomFactor);
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

	godot::Input* input = godot::Input::get_singleton();

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

#define MAPCAMERA_MOVE_INCREMENT	20.0
	if (input->is_action_pressed("ui_left") && !m_altPressed)
	{
		m_mouseRelativePosToShift += godot::Vector2(MAPCAMERA_MOVE_INCREMENT, 0);
		m_updateCameraRequired = true;
	}
	if (input->is_action_pressed("ui_right") && !m_altPressed)
	{
		m_mouseRelativePosToShift += godot::Vector2(-MAPCAMERA_MOVE_INCREMENT, 0);
		m_updateCameraRequired = true;
	}
	if (input->is_action_pressed("ui_forward") && !m_altPressed)
	{
		m_mouseRelativePosToShift += godot::Vector2(0, MAPCAMERA_MOVE_INCREMENT);
		m_updateCameraRequired = true;
	}
	if (input->is_action_pressed("ui_backward") && !m_altPressed)
	{
		m_mouseRelativePosToShift += godot::Vector2(0, -MAPCAMERA_MOVE_INCREMENT);
		m_updateCameraRequired = true;
	}

	if (m_updateCameraRequired)
	{
		m_updateCameraRequired = false;

		if (m_zoomRequired != 0)
		{
			//map->zoom(m_zoomRequired);
			constexpr float ZOOM_INCREMENET = 0.02f;
			float zoomFactor = getZoomFactor();
			zoomFactor += ZOOM_INCREMENET * m_zoomRequired;
			setZoom(zoomFactor);

			m_zoomRequired = 0;
		}

		if (m_mouseRelativePosToShift != godot::Vector2(0, 0))
		{
			float sensitivity = m_sensitivity;
			if (m_ctrlPressed)
				sensitivity *= 2;
			float zoomModifier = (1 - getZoomFactor()) * 2;
			if (zoomModifier < 1)
				zoomModifier = 1;
			m_mouseRelativePosToShift *= sensitivity * zoomModifier;

			//translate(godot::Vector2(-m_mouseRelativePosToShift.x, -m_mouseRelativePosToShift.y));
			translate(-m_mouseRelativePosToShift);

			m_mouseRelativePosToShift = godot::Vector2(0, 0);
		}
	}
}

void GDN_TheWorld_MapCamera::setZoom(float zoomFactor)
{
	if (zoomFactor >= 0.09 /*zoomFactor > 0.03*/ and zoomFactor <= 1)
	{
		godot::Vector2 zoom(zoomFactor, zoomFactor);
		set_zoom(zoom);
		m_zoomFactor = zoomFactor;

		if (m_mapViewer != nullptr)
		{
			GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
			if (viewer != nullptr)
				viewer->forceProcessNewMapQuadrants(true);

			GDN_TheWorld_Map* map = m_mapViewer->getMap();
			if (map != nullptr)
			{
				map->setMapGridLineThickness(zoomFactor);
				map->resizeRuler();
			}
		}
	}
}

void GDN_TheWorld_MapCamera::_process(double _delta)
{
	m_savedGlobalPos = get_global_position();
}

godot::Vector2 GDN_TheWorld_MapCamera::getSavedGlobalPosition(void)
{
	return m_savedGlobalPos;
}

godot::Transform2D GDN_TheWorld_MapCamera::getCameraWorldTransform()
{
	godot::Transform2D map;

	if (m_mapViewer == nullptr)
		return map;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return map;

	godot::Transform2D cameraT = get_global_transform();
	map = viewer->globalToWorld(cameraT);
	
	return map;
}

godot::Vector2 GDN_TheWorld_MapCamera::getCameraWorldPos()
{
	return getCameraWorldTransform().get_origin();
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
	ClassDB::bind_method(D_METHOD("get_camera_altitude_from_sea"), &GDN_TheWorld_Camera::cameraAltitudeFromSea);
	ClassDB::bind_method(D_METHOD("set_camera_global_transform"), &GDN_TheWorld_Camera::setGlobalTransform);
	ClassDB::bind_method(D_METHOD("set_camera_world_transform", "p_transform"), &GDN_TheWorld_Camera::setWorldTransform);
	ClassDB::bind_method(D_METHOD("get_camera_world_transform"), &GDN_TheWorld_Camera::getWorldTransform);
	ClassDB::bind_method(D_METHOD("set_camera_world_pos", "p_pos"), &GDN_TheWorld_Camera::setWorldPos);
	ClassDB::bind_method(D_METHOD("get_camera_world_pos"), &GDN_TheWorld_Camera::getWorldPos);
	ClassDB::bind_method(D_METHOD("get_velocity"), &GDN_TheWorld_Camera::velocity);
	ClassDB::bind_method(D_METHOD("is_camera_under_water"), &GDN_TheWorld_Camera::cameraUnderWater);

	ClassDB::add_property_group("GDN_TheWorld_Camera", "Camera", "camera_");
	ClassDB::add_property("GDN_TheWorld_Camera", PropertyInfo(Variant::FLOAT, "camera_tracking_speed", PROPERTY_HINT_RANGE, "0.0, 10.0, 0.1, or_greater", PROPERTY_USAGE_EDITOR /* | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED*/), "set_camera_tracking_speed", "get_camera_tracking_speed");
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

	// Create, Load and pass to the GPU Sea Wave Map
	{
		//String texName = "res://addons/twviewer/shaders/tex_sea_wave.tres";
		//m_seaWaveMapTexture = godot::ResourceLoader::get_singleton()->load(texName);

		godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
		noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		noise->set_seed(0);
		noise->set_frequency(0.01f);
		noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_FBM);
		noise->set_fractal_octaves(5);
		noise->set_fractal_lacunarity(2.0f);
		noise->set_fractal_gain(0.5f);
		noise->set_fractal_weighted_strength(0.0f);
		godot::Ref<godot::NoiseTexture2D> tex = memnew(godot::NoiseTexture2D);
		tex->set_noise(noise);
		tex->set_width(512);
		tex->set_height(512);
		tex->set_generate_mipmaps(true);
		tex->set_seamless(true);
		tex->set_normalize(true);
		tex->set_seamless_blend_skirt(0.1f);
		m_seaWaveMapTexture = tex;
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_WAVE, m_seaWaveMapTexture);
	}

	// Create and Pass to the GPU Water Foam Map
	{
		godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
		noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		noise->set_seed(0);
		noise->set_frequency(0.01f);
		noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_FBM);
		noise->set_fractal_octaves(5);
		noise->set_fractal_lacunarity(2.0f);
		noise->set_fractal_gain(0.5f);
		noise->set_fractal_weighted_strength(0.0f);
		godot::Ref<godot::NoiseTexture2D> tex = memnew(godot::NoiseTexture2D);
		tex->set_noise(noise);
		tex->set_width(512);
		tex->set_height(512);
		tex->set_generate_mipmaps(true);
		tex->set_seamless(true);
		tex->set_normalize(true);
		tex->set_seamless_blend_skirt(0.1f);
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_WATER_FOAM, tex);
	}

	// Create and Pass to the GPU Sea Texture Normal
	{
		godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
		noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_PERLIN);
		noise->set_seed(2);
		noise->set_frequency(0.011f);
		noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_RIDGED);
		noise->set_fractal_octaves(5);
		noise->set_fractal_lacunarity(2.0f);
		noise->set_fractal_gain(0.5f);
		noise->set_fractal_weighted_strength(0.0f);
		godot::Ref<godot::NoiseTexture2D> tex = memnew(godot::NoiseTexture2D);
		tex->set_noise(noise);
		tex->set_width(512);
		tex->set_height(512);
		tex->set_generate_mipmaps(true);
		tex->set_seamless(true);
		tex->set_as_normal_map(true);
		tex->set_normalize(true);
		tex->set_seamless_blend_skirt(0.1f);
		tex->set_bump_strength(50.0f);
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_TEXTURE_NORMAL, tex);
	}

	// Create and Pass to the GPU Sea Texture Normal 2
	{
		godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
		noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_PERLIN);
		noise->set_seed(0);
		noise->set_frequency(0.011f);
		noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_RIDGED);
		noise->set_fractal_octaves(5);
		noise->set_fractal_lacunarity(2.0f);
		noise->set_fractal_gain(0.5f);
		noise->set_fractal_weighted_strength(0.0f);
		godot::Ref<godot::NoiseTexture2D> tex = memnew(godot::NoiseTexture2D);
		tex->set_noise(noise);
		tex->set_width(512);
		tex->set_height(512);
		tex->set_generate_mipmaps(true);
		tex->set_seamless(true);
		tex->set_as_normal_map(true);
		tex->set_normalize(true);
		tex->set_seamless_blend_skirt(0.1f);
		tex->set_bump_strength(50.0f);
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SEA_TEXTURE_NORMAL2, tex);
	}

	// Create and Pass to the GPU Terrain Quad Border Distance Map
	{
		godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
		noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_SIMPLEX_SMOOTH);
		noise->set_seed(0);
		noise->set_frequency(0.01f);
		noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_FBM);
		noise->set_fractal_octaves(5);
		noise->set_fractal_lacunarity(2.0f);
		noise->set_fractal_gain(0.5f);
		noise->set_fractal_weighted_strength(0.0f);
		godot::Ref<godot::NoiseTexture2D> tex = memnew(godot::NoiseTexture2D);
		tex->set_noise(noise);
		tex->set_width(512);
		tex->set_height(512);
		tex->set_generate_mipmaps(true);
		//tex->set_seamless(true);
		tex->set_normalize(true);
		//tex->set_seamless_blend_skirt(0.1f);
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_TERRAIN_QUAD_BORDER_DISTANCE, tex);
	}

	// Under water post processing effect
	//if (not IS_EDITOR_HINT())
	//{
	//	godot::TypedArray<godot::Node> children = get_children();
	//	while (children.size())
	//	{
	//		godot::MeshInstance3D* child = Object::cast_to<godot::MeshInstance3D>(children.pop_back());
	//		if (child == nullptr)
	//			continue;
	//		if (child->get_name() == godot::String(POST_PROCESSING_UNDER_WATER_NODE))
	//			m_postProcessingUnderWaterNode = child;
	//		if (child->get_name() == godot::String(POST_PROCESSING_UNDER_WATER_OVERRIDE_NODE))
	//			m_postProcessingUnderWaterOverrideNode = child;
	//	}

	//	if (m_postProcessingUnderWaterNode == nullptr)
	//	{
	//		String shaderPath;
	//																														//shaderPath  = "res://addons/twviewer/shaders/CameraUnderWater.gdshader";
	//		shaderPath = "res://addons/twviewer/shaders/CameraUnderWaterNew.gdshader";
	//		godot::Ref<godot::Shader> shader = godot::ResourceLoader::get_singleton()->load(shaderPath);

	//		godot::ShaderMaterial* mat = memnew(godot::ShaderMaterial);
	//		mat->set_shader(shader);
	//		//mat->set_render_priority(0);

	//		godot::QuadMesh* quadMesh = memnew(godot::QuadMesh);
	//		quadMesh->set_size(godot::Vector2(3, 2));
	//																														//quadMesh->set_size(godot::Vector2(2, 2));
	//		quadMesh->set_orientation(godot::PlaneMesh::Orientation::FACE_Z);
	//		quadMesh->set_flip_faces(false);
	//																														//quadMesh->set_flip_faces(true);
	//		quadMesh->set_material(mat);

	//		m_postProcessingUnderWaterNode = memnew(godot::MeshInstance3D);
	//		m_postProcessingUnderWaterNode->set_name(POST_PROCESSING_UNDER_WATER_NODE);
	//		add_child(m_postProcessingUnderWaterNode);
	//		m_postProcessingUnderWaterNode->set_visible(true);
	//		m_postProcessingUnderWaterNode->set_position(godot::Vector3(0, 0, -1));
	//																														//m_postProcessingUnderWaterNode->set_position(godot::Vector3(0, 0, -11.381f));
	//		m_postProcessingUnderWaterNode->set_rotation_degrees(godot::Vector3(0, 0, 0));
	//																														//m_postProcessingUnderWaterNode->set_rotation_degrees(godot::Vector3(12.5f, -56.4f, -18.1f));
	//																														//m_postProcessingUnderWaterNode->set_extra_cull_margin(20000);
	//		m_postProcessingUnderWaterNode->set_mesh(quadMesh);
	//	}

	//	if (m_postProcessingUnderWaterOverrideNode == nullptr or !m_postProcessingUnderWaterOverrideNode->is_visible())
	//	{
	//		if (m_postProcessingUnderWaterNode != nullptr)
	//			m_postProcessingUnderWaterNode->set_visible(true);
	//	}
	//	else
	//	{
	//		if (m_postProcessingUnderWaterNode != nullptr)
	//			m_postProcessingUnderWaterNode->set_visible(false);
	//	}
	//}

	bool cameraUnderWater = false;
	float f = cameraUnderWater ? 1.0f : 0.0f;
	//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_UNDER_WATER, f);
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
	case NOTIFICATION_TRANSFORM_CHANGED:
	{
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

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return map;

	godot::Vector3 v = camera->get_global_position();				// DEBUG
	godot::Transform3D cameraT = camera->get_global_transform();
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

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	//godot::Transform3D cameraT = camera->get_global_transform();	// DEBUG ONLY
	godot::Transform3D newCameraT = viewer->worldToGlobal(map);
	//godot::Transform3D viewerT = viewer->getInternalGlobalTransform().affine_inverse();
	//godot::Transform3D newCameraT = viewerT * t;
	godot::Transform3D oldCameraT = camera->get_global_transform();
	camera->set_global_transform(newCameraT);
	
	viewer->checkNewVisibleQuadrantsRectSize(newCameraT, oldCameraT);

	godot::Vector3 anchorPos = Globals()->Viewer()->getAnchorForOriginShift()->get_global_position();
	PLOG_DEBUG << "Set camera transform - World pos=" << map.origin.x << "," << map.origin.y << "," << map.origin.z << " - Global pos=" << newCameraT.origin.x << "," << newCameraT.origin.y << "," << newCameraT.origin.z << " - Anchor=" << anchorPos.x << "," << anchorPos.z;
}

godot::Vector3 GDN_TheWorld_Camera::getWorldPos(void)
{
	return getWorldTransform().get_origin();
}

void GDN_TheWorld_Camera::setWorldPos(godot::Vector3 worldPos)
{
	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	godot::Transform3D worldTransform(camera->get_global_transform().basis, worldPos);
	setWorldTransform(worldTransform);
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

	adjustPositionOnTerrain();
	
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

	godot::Input* input = godot::Input::get_singleton();

	if (mouseMovementSuspended())
	{
		m_mouseShiftCameraOn = false;
		m_rotateCameraOn = false;
		if (input->is_action_pressed("ui_mouse_button_right"))
			m_mouseButtonRightPressedWhenSuspended = true;
		else
			m_mouseButtonRightPressedWhenSuspended = false;
		if (input->is_action_pressed("ui_mouse_button_left"))
			m_mouseButtonLeftPressedWhenSuspended = true;
		else
			m_mouseButtonLeftPressedWhenSuspended = false;
	}
	else
	{
		if (input->is_action_pressed("ui_mouse_button_right") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		{
			if (not m_mouseButtonRightPressedWhenSuspended)
			{
				//if (!m_rotateCameraOn)
				//	Globals()->debugPrint("Rotate ON");
				m_rotateCameraOn = true;
			}
		}
		else
		{
			//if (m_rotateCameraOn)
			//	Globals()->debugPrint("Rotate OFF");
			m_rotateCameraOn = false;
			m_mouseButtonRightPressedWhenSuspended = false;
		}

		if (input->is_action_pressed("ui_mouse_button_left") /* && viewer->terrainShiftPermitted()*/ /* && !viewer->editMode()*/)
		{
			if (not m_mouseButtonLeftPressedWhenSuspended)
				m_mouseShiftCameraOn = true;
		}
		else
		{
			m_mouseShiftCameraOn = false;
			m_mouseButtonLeftPressedWhenSuspended = false;
		}
	}

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
	
	//debugCheckCollisions();
}

void GDN_TheWorld_Camera::calcYawPitchRoll(void)
{
	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;
	godot::Vector3 euler = camera->get_global_transform().basis.get_euler();;
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

	if (not mouseMovementSuspended())
	{
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

godot::Camera3D* GDN_TheWorld_Camera::getRealSceneCamera(void)
{
	if (IS_EDITOR_HINT())
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
		if (viewer == nullptr)
			return nullptr;
		godot::Camera3D* camera = viewer->getWorldCamera();
		if (camera == nullptr)
			return nullptr;
		return camera;
	}
	else
		return this;
}

void GDN_TheWorld_Camera::set_cameraFar(void)
{
	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	float quadrantSize = Globals()->heightmapSizeInWUs();
	float thresholdUpdateCameraFar = DISTANCE_THRESHOLD_TO_UPDATE_CAMERA_FAR;
	godot::Vector3 cameraGlobalPosition = camera->get_global_position();
	if (m_forceRecalcCameraFar or cameraGlobalPosition.distance_to(m_savedCameraPosAfterSetFar) > thresholdUpdateCameraFar)
	{
		float _far = quadrantSize * (Globals()->Viewer()->maxActualDepthQuadInAllDirections() + 1) * 1.5f;	// zfar at 0 altitude
		_far = sqrtf(powf(_far, 2.0) + powf(cameraGlobalPosition.y, 2.0));
		camera->set_far(_far);

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
		camera->set_near(_near);

		//PLOG_DEBUG << "Camera znear=" << _near;
		//PLOG_DEBUG << "Camera zfar=" << _far;

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
	{
		godot::Camera3D* camera = getRealSceneCamera();
		if (camera == nullptr)
			return false;
		camera->look_at_from_position(cameraPosGlobalCoord, lookAtGlobalCoord, godot::Vector3(0, 1, 0));
	}

	godot::Vector3 anchorPos = Globals()->Viewer()->getAnchorForOriginShift()->get_global_position();
	PLOG_DEBUG << "Init camera in world - Camera world pos=" << cameraPos.x << "," << cameraPos.y << "," << cameraPos.z << " - Global pos=" << cameraPosGlobalCoord.x << "," << cameraPosGlobalCoord.y << "," << cameraPosGlobalCoord.z << " - Anchor=" << anchorPos.x << "," << anchorPos.z;

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

	{
		godot::Ref<godot::SphereShape3D> sphereShape = memnew(godot::SphereShape3D);
		sphereShape->set_radius(MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA);
		sphereShape->set_margin(0.1f);
		m_shapeCast = memnew(godot::ShapeCast3D);
		add_child(m_shapeCast);
		m_shapeCast->set_shape(sphereShape);
		m_shapeCast->set_target_position(godot::Vector3(0, 0, 0));
		m_shapeCast->set_collision_mask(4294967295);
	}
	
	return true;
}

bool GDN_TheWorld_Camera::updateCamera(double _delta)
{
	bool ret;

	m_prevCameraPos = get_global_position();

	if (trackingPlayer())
		ret = updatePlayerCamera(_delta);
	else
		ret = updateFreeLookCamera(_delta);

	double movement = (get_global_position() - m_prevCameraPos).length();
	if (_delta > 0)
		m_velocity = float(movement / _delta);
	else
		m_velocity = 0;

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

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	//if (m_postProcessingUnderWaterNode != nullptr)
	//{
	//	if (m_postProcessingUnderWaterOverrideNode == nullptr or !m_postProcessingUnderWaterOverrideNode->is_visible())
	//		m_postProcessingUnderWaterNode->set_visible(true);
	//	else
	//		m_postProcessingUnderWaterNode->set_visible(false);
	//}

	QuadTree* quadTree = viewer->getCameraQuadrant(camera);
	if (not globals->getParamShowSea() or (quadTree != nullptr and quadTree->canContainSea() == QUADRANT_SEA_INFO_FALSE))
	{
		viewer->shaderParamCameraUnderWaterApplied(false);
		return;
	}

	if (m_seaWaveMapTexture != nullptr)
	{
		if (m_seaWaveMapImage == nullptr)
			m_seaWaveMapImage = m_seaWaveMapTexture->get_image();
		if (m_seaWaveMapImage != nullptr)
		{
			// WARNING: Noise texture must be SEAMLESS and Normalized

			//GDN_TheWorld_Globals* globals = Globals();
			//GDN_TheWorld_Viewer* viewer = globals->Viewer();

			godot::Vector3 cameraGlobalPos = camera->get_global_position();

			float shaderParamSeaFlatSeaHeightApplied = viewer->shaderParamSeaFlatSeaHeightApplied();
			if (cameraGlobalPos.y > shaderParamSeaFlatSeaHeightApplied - THEWORLD_SEA_MAX_WAVE_HEIGHT and cameraGlobalPos.y < shaderParamSeaFlatSeaHeightApplied + THEWORLD_SEA_MAX_WAVE_HEIGHT)
			{
				// sea height in range 0.0 / 1.0 from noise texture moving uv with vertex global position and time 
				float uv_factor = float(globals->shaderParamGlobalTimeApplied() * viewer->shaderParamSeaTimeScaleApplied() * viewer->shaderParamSeaRoughFactorApplied());
				godot::Vector2 uv = godot::Vector2((cameraGlobalPos.x / viewer->shaderParamSeaNoiseScaleApplied()) + uv_factor, (cameraGlobalPos.z / viewer->shaderParamSeaNoiseScaleApplied()) + uv_factor);
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
				if (cameraGlobalPos.y > shaderParamSeaFlatSeaHeightApplied + shaderParamSeaStartCamAltForWavesAttenuationApplied)
					scaleFactor = scaleFactor - (scaleFactor * (cameraGlobalPos.y - shaderParamSeaStartCamAltForWavesAttenuationApplied) / (viewer->shaderParamSeaEndCamAltForWavesApplied() - shaderParamSeaStartCamAltForWavesAttenuationApplied));

				if (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE)
				{
					float terrainHeigth = cameraGlobalPos.y - m_lastCameraAltitudeFromTerrain;
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

				if (seaHeight > cameraGlobalPos.y)
					m_cameraUnderWater = true;
				else
					m_cameraUnderWater = false;
			}
			else
			{
				if (cameraGlobalPos.y > shaderParamSeaFlatSeaHeightApplied + THEWORLD_SEA_MAX_WAVE_HEIGHT)
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
	godot::Vector3 preMoveCameraPos = get_global_position();
	float preMoveCameraAltitudeFromTerrain = m_lastCameraAltitudeFromTerrain;

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

	godot::Vector3 resultantCameraMovement;

	if (m_mouseRelativePosToShift != godot::Vector2(0, 0))
	{
		float sensitivity = m_sensitivity;
		if (m_ctrlPressed)
			sensitivity *= 2;
		m_mouseRelativePosToShift *= sensitivity;
		godot::Vector3 cameraMovement = godot::Vector3(-m_mouseRelativePosToShift.x, 0, -m_mouseRelativePosToShift.y);
		m_mouseRelativePosToShift = godot::Vector2(0, 0);
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	//if (m_mouseRelativePosToShiftVert != godot::Vector2(0, 0))
	//{
	//	m_mouseRelativePosToShiftVert *= m_sensitivity;

	//	translate_object_local(godot::Vector3(0, 0, -m_mouseRelativePosToShiftVert.y));

	//	m_mouseRelativePosToShiftVert = godot::Vector2(0, 0);
	//}

	if (m_numLongMoveStepForward)
	{
		godot::Vector3 cameraMovement = godot::Vector3(0, 0, m_numLongMoveStepForward * m_wheelVeryFastVelocity);
		m_numLongMoveStepForward = 0;
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	if (m_numLongMoveStepLeft)
	{
		godot::Vector3 cameraMovement = godot::Vector3(-m_numLongMoveStepLeft * m_wheelVeryFastVelocity, 0, 0);
		m_numLongMoveStepLeft = 0;
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	float delayDegree = Globals()->Viewer()->getDelayDegree();
	delayDegree = delayDegree < 1 ? 1 : delayDegree;	
	float delayFactor = 1 / delayDegree;
	//delayFactor = 1;	// DEBUG - Disable delay factor for camera movement for now as it is not really useful and make camera movement more difficult to control (especially with mouse wheel)	

	if (m_numMoveStepForward)
	{
		godot::Vector3 cameraMovement;
		if (m_altPressed)
		{
			if (m_shiftPressed)
			{
				cameraMovement = godot::Vector3(0, -m_numMoveStepForward * m_wheelSlowVelocity, 0);
			}
			else if (m_ctrlPressed)
			{
				cameraMovement = godot::Vector3(0, -m_numMoveStepForward * m_wheelFastVelocity * delayFactor, 0);
			}
			else
			{
				cameraMovement = godot::Vector3(0, -m_numMoveStepForward * m_wheelNormalVelocity, 0);
			}
		}
		else
		{
			if (m_shiftPressed)
			{
				cameraMovement = godot::Vector3(0, 0, m_numMoveStepForward * m_wheelSlowVelocity);
			}
			else if (m_ctrlPressed)
			{
				cameraMovement = godot::Vector3(0, 0, m_numMoveStepForward * m_wheelFastVelocity * delayFactor);
			}
			else
			{
				cameraMovement = godot::Vector3(0, 0, m_numMoveStepForward * m_wheelNormalVelocity);
			}
		}
		m_numMoveStepForward = 0;
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	if (m_numMoveStepLeft)
	{
		godot::Vector3 cameraMovement;
		if (m_shiftPressed)
		{
			cameraMovement = godot::Vector3(-m_numMoveStepLeft * m_wheelSlowVelocity, 0, 0);
		}
		else if (m_ctrlPressed)
		{
			cameraMovement = godot::Vector3(-m_numMoveStepLeft * m_wheelFastVelocity * delayFactor, 0, 0);
		}
		else
		{
			cameraMovement = godot::Vector3(-m_numMoveStepLeft * m_wheelNormalVelocity, 0, 0);
		}
		m_numMoveStepLeft = 0;
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	if (m_numMoveStepUp)
	{
		godot::Vector3 cameraMovement;
		if (m_shiftPressed)
		{
			cameraMovement = godot::Vector3(0, m_numMoveStepUp * m_wheelSlowVelocity, 0);
		}
		else if (m_ctrlPressed)
		{
			cameraMovement = godot::Vector3(0, m_numMoveStepUp * m_wheelFastVelocity * delayFactor, 0);
		}
		else
		{
			cameraMovement = godot::Vector3(0, m_numMoveStepUp * m_wheelNormalVelocity, 0);
		}
		m_numMoveStepUp = 0;
		//translate_object_local(cameraMovement);
		resultantCameraMovement += cameraMovement;
	}

	if (resultantCameraMovement != godot::Vector3())
	{
		translate_object_local(manageCollisionsPreMovement(resultantCameraMovement));
	}

	manageCollisionsPostMovement(preMoveCameraAltitudeFromTerrain, preMoveCameraPos);
	
	return true;
}

bool GDN_TheWorld_Camera::checkCollisionWithTerrain(godot::Vector3& cameraMovementLocalCoord)
{
	bool collisionDetected = false;

	if (IS_EDITOR_HINT())
		return collisionDetected;

	godot::Vector3 newCameraMovement = get_global_basis().xform(cameraMovementLocalCoord);	// transform movement from local camera coordinates to global coordinates
	
	godot::Vector3 cameraGlobalPosition = get_global_position();

	static size_t maxNumIterations = 0;
	static size_t maxNumInternalIterations = 0;

	const size_t maxIterationsBeforeGivingUp = 2;	// to prevent infinite loop in case of unexpected collision configuration (ex: camera stuck in a corner between 2 or more terrain triangles)
	size_t numIterations = 0;
	while (numIterations < maxIterationsBeforeGivingUp)
	{
		numIterations++;

		godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
		godot::Dictionary rayArray;
		godot::Vector3 rayOrigin = cameraGlobalPosition;
		godot::Vector3 rayEnd = rayOrigin + newCameraMovement;
		godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
		rayArray = spaceState->intersect_ray(query);
		if (not rayArray.has("position"))
			break;	// no collision, we can move camera as desidered

		// we have to manage collision with terrain, we will try to move camera as close as possible to collision point without crossing it and then check if we are still colliding (ex: in case of collision with a slope, we may need to move camera up to not be colliding anymore)
		collisionDetected = true;

		godot::Vector3 collisionPos = rayArray["position"];
		godot::Vector3 collisionNormal = rayArray["normal"];
		//collisionNormal.normalize();	// DEBUG - Check that collision normal is normalized as it should be, if not it means there is a problem with the collision detection in terrain collider
		
		godot::Vector3 movementDir = newCameraMovement.normalized();
		godot::Vector3 fromCameraToCollision = collisionPos - rayOrigin;
		float dot = movementDir.dot(collisionNormal);

		//godot::Vector3 movementDirCheck = fromCameraToCollision.normalized();	// DEBUG - Check that movement direction is the same as direction from ray origin to collision position
		//assert(1.0f - abs(movementDirCheck.dot(movementDir)) < 0.01f);			// DEBUG - Check that movement direction is the same as direction from ray origin to collision position

		//{
		//	m_shapeCast->set_global_position(cameraGlobalPosition);
		//	if (m_shapeCast->is_colliding())
		//	{
		//		int numColliders = m_shapeCast->get_collision_count();
		//		godot::Vector3 collisionPos1 = m_shapeCast->get_collision_point(0);
		//		godot::Vector3 collisionNormal1 = m_shapeCast->get_collision_normal(0);
		//		godot::Object* collider = m_shapeCast->get_collider(0);

		//		collisionPos = collisionPos1;
		//		collisionNormal = collisionNormal1;
		//	}
		//}

		if (1.0f - abs(dot) < 0.01f)
		{
			// movement is nearly in the same direction as collision normal, we can just stop camera movement near the collision point
			newCameraMovement = fromCameraToCollision - movementDir * MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA;
			break;
		}

		bool isFacingCollisionNormal = dot < 0;
		if (not isFacingCollisionNormal)
		{
			// movement is going away from terrain (dot > 0) or is parallel to terrain (dot == 0), no collision to manage
			break;
		}

		
		float penetrationDepth = fromCameraToCollision.dot(collisionNormal);
		if (penetrationDepth < 0)
		{
			// move camera out of collision
			//godot::Vector3 correctionVector = collisionNormal * (-penetrationDepth + 0.1f);	// add a small offset to prevent sticking to terrain
			godot::Vector3 correctionVector = collisionNormal;
			newCameraMovement = fromCameraToCollision + correctionVector;

			//m_shapeCast->set_global_transform(get_global_transform());	// same position and orientation as camera
			//const size_t maxIterationsBeforeGivingUp = 10;	// to prevent infinite loop in case of unexpected collision configuration (ex: camera stuck in a corner between 2 or more terrain triangles)
			//size_t numIterations = 0;
			//while (numIterations < maxIterationsBeforeGivingUp)
			//{
			//	numIterations++;

			//	m_shapeCast->set_target_position(m_shapeCast->get_global_basis().xform_inv(newCameraMovement));
			//	if (not m_shapeCast->is_colliding())
			//		break;

			//	newCameraMovement = fromCameraToCollision + correctionVector * float(numIterations + 1);
			//}
			//if (numIterations > maxNumInternalIterations)
			//	maxNumInternalIterations = numIterations;

			break;
		}
		else
		{
			my_assert(false);
		}
	}

	if (numIterations > maxNumIterations)
		maxNumIterations = numIterations;

	if (numIterations >= maxIterationsBeforeGivingUp)
	{
		//	assert(numIterations < maxIterationsBeforeGivingUp);
		newCameraMovement = godot::Vector3();	// give up, we will not move camera this frame to prevent possible infinite loop in case of unexpected collision configuration (ex: camera stuck in a corner between 2 or more terrain triangles)
	}
	
	cameraMovementLocalCoord = get_global_basis().xform_inv(newCameraMovement);

	return collisionDetected;
}

//void GDN_TheWorld_Camera::debugCheckCollisions(void)
//{
//	bool testCollisions = false;
//
//	if (!testCollisions)
//		return;
//	
//	godot::Camera3D* camera = getRealSceneCamera();
//	if (camera == nullptr)
//		return;
//
//	godot::Vector3 cameraGlobalPosition = camera->get_global_position();
//	godot::Vector3 cameraGlobalDirection = -camera->get_global_transform().basis.rows[godot::Vector3::Axis::AXIS_Z].normalized();
//	godot::Vector3 camdirCollisionOnTerrainPos, camdirCollisionOnTerrainNormal;
//	float distanceFromCamdirCollisionOnTerrainPos;
//
//	//godot::Vector3 downCollisionOnTerrainPos, downCollisionOnTerrainNormal;
//	//float distanceFromDownCollisionOnTerrainPos;
//	//{
//	//	godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
//	//	godot::Dictionary rayArray;
//	//	godot::Vector3 rayOrigin = cameraGlobalPosition;
//	//	godot::Vector3 rayEnd = rayOrigin + godot::Vector3(0.0f, -MAX_ALTITUDE_DETECTED, 0.0f);
//	//	godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
//	//	rayArray = spaceState->intersect_ray(query);
//	//	if (rayArray.has("position"))
//	//	{
//	//		downCollisionOnTerrainPos = rayArray["position"];
//	//		distanceFromDownCollisionOnTerrainPos = (downCollisionOnTerrainPos - cameraGlobalPosition).length();
//	//	}
//	//	if (rayArray.has("normal"))
//	//		downCollisionOnTerrainNormal = rayArray["normal"];
//	//}
//
//	{
//		godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
//		godot::Dictionary rayArray;
//		godot::Vector3 rayOrigin = cameraGlobalPosition;
//		godot::Vector3 rayEnd = rayOrigin + cameraGlobalDirection * MAX_ALTITUDE_DETECTED * 3;
//		godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
//		rayArray = spaceState->intersect_ray(query);
//		if (rayArray.has("position"))
//		{
//			camdirCollisionOnTerrainPos = rayArray["position"];
//			distanceFromCamdirCollisionOnTerrainPos = (camdirCollisionOnTerrainPos - cameraGlobalPosition).length();
//		}
//		if (rayArray.has("normal"))
//			camdirCollisionOnTerrainNormal = rayArray["normal"];
//		if (camdirCollisionOnTerrainPos != godot::Vector3() and camdirCollisionOnTerrainNormal != godot::Vector3())
//		{
//			//godot::UtilityFunctions::print((std::string("Camera collision in cam dir - Distance: ") + std::to_string(distanceFromCamdirCollisionOnTerrainPos) + " - Normal: " + std::to_string(camdirCollisionOnTerrainNormal.x) + "," + std::to_string(camdirCollisionOnTerrainNormal.y) + "," + std::to_string(camdirCollisionOnTerrainNormal.z)).c_str());
//			float penetrationDepth = (camdirCollisionOnTerrainPos - rayOrigin).dot(camdirCollisionOnTerrainNormal);
//			if (penetrationDepth < 0)
//			{
//				//godot::UtilityFunctions::print((std::string("Camera penetration depth: ") + std::to_string(penetrationDepth)).c_str());
//				bool isFacingCollisionNormal = cameraGlobalDirection.dot(camdirCollisionOnTerrainNormal) < 0;
//				bool no_penetration = true;
//				//godot::UtilityFunctions::print((std::string("Camera is penetrating terrain with depth: ") + std::to_string(penetrationDepth)).c_str());
//			}
//			else
//			{
//				//godot::UtilityFunctions::print((std::string("Camera penetration depth: ") + std::to_string(penetrationDepth)).c_str());
//				bool isFacingCollisionNormal = cameraGlobalDirection.dot(camdirCollisionOnTerrainNormal) < 0;
//				bool no_penetration = false;
//			}
//		}
//	}
//}

godot::Vector3 GDN_TheWorld_Camera::manageCollisionsPreMovement(godot::Vector3 cameraMovementLocalCoord)
{
	if (IS_EDITOR_HINT())
		return cameraMovementLocalCoord;
	
	if (!Globals()->Viewer()->isCameraQuadrantColliderSet())
		return cameraMovementLocalCoord;

	godot::Vector3 newCameraMovement = cameraMovementLocalCoord;
	bool b = checkCollisionWithTerrain(newCameraMovement);
	return newCameraMovement;
}

void GDN_TheWorld_Camera::manageCollisionsPostMovement(float preMoveCameraAltitudeFromTerrain, godot::Vector3 preMoveCameraPos)
{
	updateAltitudeFromTerrain(true);

	if (m_lastCameraAltitudeFromTerrain == INVALID_CAMERA_ALTITUDE)
	{
		if (preMoveCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE)
		{
			if (preMoveCameraAltitudeFromTerrain < MAX_ALTITUDE_DETECTED - (MAX_ALTITUDE_DETECTED * 0.1f))	// Far above terrain we leave camera move else we force camera still
			{
				set_global_position(preMoveCameraPos);
				m_lastCameraAltitudeFromTerrain = preMoveCameraAltitudeFromTerrain;
			}
		}
	}
	else
	{
		godot::Vector3 postMoveCameraPos = get_global_position();
		if (postMoveCameraPos.y < preMoveCameraPos.y - (m_lastCameraAltitudeFromTerrain - MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA))
		{
			set_global_position(godot::Vector3(postMoveCameraPos.x, preMoveCameraPos.y - (m_lastCameraAltitudeFromTerrain - MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA), postMoveCameraPos.z));
			m_lastCameraAltitudeFromTerrain = MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA;
		}
	}
}

void GDN_TheWorld_Camera::adjustPositionOnTerrain(void)
{
	if (IS_EDITOR_HINT())
		return;
	
	if (adjustPositionOnTerrainRequired() or (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE and m_lastCameraAltitudeFromTerrain < 0))
	{
		//if (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE and m_lastCameraAltitudeFromTerrain < 0)		// DEBUG
		//	assert(m_lastCameraAltitudeFromTerrain < 0);																// DEBUG
		
		if (!Globals()->Viewer()->recalcGridOfQuadrantsInViewNeeded() and Globals()->Viewer()->isCameraQuadrantColliderSet())
		{
			updateAltitudeFromTerrain(true);

			if (m_lastCameraAltitudeFromTerrain != INVALID_CAMERA_ALTITUDE)
			{
				float altitudeFromTerrain = MIN_DISTANCE_FROM_TERRAIN_ALLOWED_FOR_CAMERA;
				altitudeFromTerrain = 50;
				float cameraAltitudeComputed = m_lastComputedCameraVerticalOnTerrain.y + altitudeFromTerrain;
				if (m_overSea)
					cameraAltitudeComputed = THEWORLD_SEA_LEVEL + THEWORLD_SEA_MAX_WAVE_HEIGHT * 2;
				godot::Vector3 cameraPos = get_global_position();
				set_global_position(godot::Vector3(cameraPos.x, cameraAltitudeComputed, cameraPos.z));
				m_lastCameraAltitudeFromTerrain = altitudeFromTerrain;
			}
			else
				assert(m_lastCameraAltitudeFromTerrain == INVALID_CAMERA_ALTITUDE);

			adjustPositionOnTerrainRequired(false);
		}
	}
}

float GDN_TheWorld_Camera::cameraMaxAltitudeFromTerrainDetected(void) const
{
	return MAX_ALTITUDE_DETECTED;
}

void GDN_TheWorld_Camera::updateAltitudeFromTerrain(bool force)
{
	//if (Globals()->status() != TheWorldStatus::worldDeployed)
	//	return;
	if (!Globals()->Viewer()->isCameraQuadrantColliderSet())
		return;
	
	//return;
	if (force)
		m_forceRecalcCameraAltitude = true;
	
	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	godot::Vector3 cameraGlobalPosition = camera->get_global_position();
	
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
			m_lastComputedCameraVerticalOnTerrain = rayArray["position"];
			m_lastCameraAltitudeFromTerrain = (float)abs(cameraGlobalPosition.y - m_lastComputedCameraVerticalOnTerrain.y);
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
				m_lastComputedCameraVerticalOnTerrain = rayArray["position"];
				m_lastCameraAltitudeFromTerrain = (float)(cameraGlobalPosition.y - m_lastComputedCameraVerticalOnTerrain.y);
			}
			else
			{
				m_lastComputedCameraVerticalOnTerrain = godot::Vector3();
				m_lastCameraAltitudeFromTerrain = INVALID_CAMERA_ALTITUDE;
			}
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

void GDN_TheWorld_Camera::setYaw(float yaw, bool radiant)
{
	float radiantYaw = radiant ? yaw : (yaw * TheWorld_Utils::kPi) / 180;

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	Transform3D t = camera->get_global_transform();
	godot::Vector3 v = t.basis.get_euler();
	v.y = radiantYaw;
	t.basis.set_euler(v);
	camera->set_global_transform(t);

	//if (IS_EDITOR_HINT())
	//{
	//	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	//	if (viewer == nullptr)
	//		return;
	//	godot::Camera3D* camera = viewer->getWorldCamera();
	//	if (camera != nullptr)
	//	{
	//		Transform3D t = camera->get_global_transform();
	//		godot::Vector3 v = t.basis.get_euler();
	//		v.y = radiantYaw;
	//		t.basis.set_euler(v);
	//		camera->set_global_transform(t);
	//	}
	//}
	//else
	//{
	//	Transform3D t = get_global_transform();
	//	godot::Vector3 v = t.basis.get_euler();
	//	v.y = radiantYaw;
	//	t.basis.set_euler(v);
	//	set_global_transform(t);
	//}
}
void GDN_TheWorld_Camera::setPitch(float pitch, bool radiant)
{
	float radiantPitch = radiant ? pitch : (pitch * TheWorld_Utils::kPi) / 180;

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	Transform3D t = camera->get_global_transform();
	godot::Vector3 v = t.basis.get_euler();
	v.x = radiantPitch;
	t.basis.set_euler(v);
	camera->set_global_transform(t);

	//if (IS_EDITOR_HINT())
	//{
	//	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	//	if (viewer == nullptr)
	//		return;
	//	godot::Camera3D* camera = viewer->getWorldCamera();
	//	if (camera != nullptr)
	//	{
	//		Transform3D t = camera->get_global_transform();
	//		godot::Vector3 v = t.basis.get_euler();
	//		v.x = radiantPitch;
	//		t.basis.set_euler(v);
	//		camera->set_global_transform(t);
	//	}
	//}
	//else
	//{
	//	Transform3D t = get_global_transform();
	//	godot::Vector3 v = t.basis.get_euler();
	//	v.x = radiantPitch;
	//	t.basis.set_euler(v);
	//	set_global_transform(t);
	//}
}
void GDN_TheWorld_Camera::setRoll(float roll, bool radiant)
{
	float radiantRoll = radiant ? roll : (roll * TheWorld_Utils::kPi) / 180;

	godot::Camera3D* camera = getRealSceneCamera();
	if (camera == nullptr)
		return;

	Transform3D t = camera->get_global_transform();
	godot::Vector3 v = t.basis.get_euler();
	v.z = radiantRoll;
	t.basis.set_euler(v);
	camera->set_global_transform(t);

	//if (IS_EDITOR_HINT())
	//{
	//	GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
	//	if (viewer == nullptr)
	//		return;
	//	godot::Camera3D* camera = viewer->getWorldCamera();
	//	if (camera != nullptr)
	//	{
	//		Transform3D t = camera->get_global_transform();
	//		godot::Vector3 v = t.basis.get_euler();
	//		v.z = radiantRoll;
	//		t.basis.set_euler(v);
	//		camera->set_global_transform(t);
	//	}
	//}
	//else
	//{
	//	Transform3D t = get_global_transform();
	//	godot::Vector3 v = t.basis.get_euler();
	//	v.z = radiantRoll;
	//	t.basis.set_euler(v);
	//	set_global_transform(t);
	//}
}
