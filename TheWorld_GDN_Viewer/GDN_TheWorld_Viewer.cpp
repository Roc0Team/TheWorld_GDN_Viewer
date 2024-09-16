//#include "pch.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Camera.h"
#include "GDN_TheWorld_Edit.h"

//#pragma warning(push, 1)
#include "warning_push.h"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/editor_interface.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/image_texture_layered.hpp>
#include <godot_cpp/classes/texture2D_array.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/physics_direct_space_state3d.hpp>
#include <godot_cpp/classes/physics_ray_query_parameters3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/config_file.hpp>
#include <godot_cpp/classes/performance.hpp>
#include <godot_cpp/classes/label.hpp>
#include <godot_cpp/classes/sphere_mesh.hpp>
#include <godot_cpp/classes/line2d.hpp>
#include "warning_pop.h"
//#pragma warning(pop)

#include <filesystem>
#include <limits>

#include "MeshCache.h"
#include "QuadTree.h"
#include "Profiler.h"
#include "FastNoiseLite.h"
#include "Tools.h"

#include <algorithm>

using namespace godot;
//using namespace std;

namespace fs = std::filesystem;

constexpr int32_t MAP_QUADRANT_OTPIMIZATION_LEVEL=4;

GDN_TheWorld_Map::MapTextureRectPos::MapTextureRectPos(godot::Vector2 globalPos, godot::Size2 globalSize)
{
	m_globalLowerPos.x = floor(globalPos.x / globalSize.x) * globalSize.x;
	m_globalLowerPos.y = floor(globalPos.y / globalSize.y) * globalSize.y;
	m_idx.x = (int32_t)(m_globalLowerPos.x / globalSize.x);
	m_idx.y = (int32_t)(m_globalLowerPos.y / globalSize.y);
	m_globalSize = globalSize;
}

GDN_TheWorld_Map::MapTextureRectPos::MapTextureRectPos(godot::Vector2i idx, godot::Size2 globalSize)
{
	m_idx = idx;
	m_globalLowerPos.x = m_idx.x * globalSize.x;
	m_globalLowerPos.y = m_idx.y * globalSize.y;
	m_globalSize = globalSize;
}

bool GDN_TheWorld_Map::MapTextureRectPos::operator==(const MapTextureRectPos& textureRectPos) const
{
	if (m_globalLowerPos == textureRectPos.m_globalLowerPos
		and m_globalSize == textureRectPos.m_globalSize)
		return true;
	else
		return false;
}
void GDN_TheWorld_Map::MapTextureRectPos::operator=(const MapTextureRectPos& textureRectPos)
{
	m_globalLowerPos = textureRectPos.m_globalLowerPos;
	m_globalSize = textureRectPos.m_globalSize;
	m_idx = textureRectPos.m_idx;
}
GDN_TheWorld_Map::MapTextureRectPos GDN_TheWorld_Map::MapTextureRectPos::operator+(const MapTextureRectPos& textureRectPos) const
{
	GDN_TheWorld_Map::MapTextureRectPos ret(m_idx + textureRectPos.m_idx, textureRectPos.m_globalSize);
	return ret;
}


void WorldQuadrantsMap::init(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
}

void MapQuadrantsMap::init(GDN_TheWorld_Viewer* viewer, int32_t optimizationLevel)
{
	m_viewer = viewer;
	m_optimizationLevel = optimizationLevel;
	m_imageResolution = (int32_t)m_viewer->Globals()->resolutionForOptimizationLevel((size_t)m_optimizationLevel);
}

//godot::Ref<godot::Image> MapQuadrantsMap::getMapImage(TheWorld_Utils::QuadrantPos& quadrantPos)
//{
//	std::recursive_mutex& m = m_viewer->getMainProcessingMutex();
//	std::lock_guard<std::recursive_mutex> lock(m);
//
//	if (contains(quadrantPos))
//		return (*this)[quadrantPos]->getMapImage();
//	else
//		return nullptr;
//}

void MapQuadrantsMap::addUpdatedQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	if (not m_updatedQuadrants.contains(pos))
		m_viewer->incUpdatedMapQuadrants();
	m_updatedQuadrants[pos] = true;
	if (m_updatingQuadrants.contains(pos))
		m_updatingQuadrants.erase(pos);
}

void MapQuadrantsMap::setMapData(godot::Ref<godot::Image> mapImage, TheWorld_Utils::QuadrantPos& quadrantPos)
{
	//std::recursive_mutex& m = m_viewer->getMainProcessingMutex();
	//std::lock_guard<std::recursive_mutex> lock(m);
	std::lock_guard<std::recursive_mutex> lock(m_internalMutex);

	if (mapImage != nullptr)
	{
		godot::Vector2i imageSize = mapImage->get_size();
		my_assert(imageSize.x == imageSize.y);
		my_assert(imageSize.x == getImageResolution());
	}
	
	if (not contains(quadrantPos))
		(*this)[quadrantPos] = std::make_unique<MapQuadrant>(quadrantPos, m_viewer, getOptimizationLevel());
	(*this)[quadrantPos]->setMapData(mapImage);

	addUpdatedQuadrant(quadrantPos);
}

bool MapQuadrantsMap::checkUpdatingQuadrants(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_internalMutex);

	bool statusChanged = false;

	for (auto& item : m_updatingQuadrants)
	{
		if (item.second->status() == UpdateStatus::UpdateInProgress and item.second->elapsedFromLastStatusChange() > THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE)
		{
			item.second->status(UpdateStatus::UpdateNeeded);
			statusChanged = true;
		}
	}

	return statusChanged;
}

bool MapQuadrantsMap::addUpdatingQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	bool added = false;
	std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	
	//if (pos.getIndexPosX() == 1 and pos.getIndexPosZ() == 1)	// DEBUG ONLY
	//	1;														// DEBUG ONLY

	bool updateFromServerNeeded = false;
	if (not contains(pos))
	{
		(*this)[pos] = std::make_unique<MapQuadrant>(pos, m_viewer, getOptimizationLevel());
		added = true;
		updateFromServerNeeded = true;
	}

	if (not updateFromServerNeeded and (m_updatingQuadrants.contains(pos) and m_updatingQuadrants[pos]->status() == UpdateStatus::UpdateNeeded))
		updateFromServerNeeded = true;
	
	if (updateFromServerNeeded)
	{
		if (not m_updatingQuadrants.contains(pos))
			m_updatingQuadrants[pos] = std::make_unique<UpdatingInfo>();
		m_updatingQuadrants[pos]->status(UpdateStatus::UpdateInProgress);

		TheWorld_Utils::MeshCacheBuffer cache = (*this)[pos]->getMeshCacheBuffer();
		std::string meshId = cache.getMeshIdFromDisk(false, nullptr, false);
		//std::string meshId;
		m_viewer->Globals()->Client()->MapManagerGetQuadrantVertices(m_viewer->isInEditor(), 0, 0, 0, 0, 0, 0, pos.getLowerXGridVertex(), pos.getLowerZGridVertex(),
			1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
			pos.getNumVerticesPerSize(), pos.getGridStepInWU(), pos.getLevel(), false, 0, meshId);
	}

	return added;
}

//void GDN_TheWorld_WorldViewport::_bind_methods(void)
//{
//}
//
//GDN_TheWorld_WorldViewport::GDN_TheWorld_WorldViewport(void)
//{
//}
//
//GDN_TheWorld_WorldViewport::~GDN_TheWorld_WorldViewport(void)
//{
//}
//
//void GDN_TheWorld_WorldViewport::_notification(int p_what)
//{
//	switch (p_what)
//	{
//	case NOTIFICATION_ENTER_TREE:
//	{
//	}
//	break;
//	case NOTIFICATION_WM_CLOSE_REQUEST:
//	{
//	}
//	break;
//	case NOTIFICATION_PREDELETE:
//	{
//	}
//	break;
//	}
//}
//
//void GDN_TheWorld_WorldViewport::_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}
//
//void GDN_TheWorld_WorldViewport::_unhandled_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}
//
//void GDN_TheWorld_WorldViewport::_ready(void)
//{
//}
//
//void GDN_TheWorld_WorldViewport::_process(double _delta)
//{
//}
//
//void GDN_TheWorld_WorldViewportContainer::_bind_methods(void)
//{
//}
//
//GDN_TheWorld_WorldViewportContainer::GDN_TheWorld_WorldViewportContainer(void)
//{
//}
//
//GDN_TheWorld_WorldViewportContainer::~GDN_TheWorld_WorldViewportContainer(void)
//{
//}
//
//void GDN_TheWorld_WorldViewportContainer::_notification(int p_what)
//{
//	switch (p_what)
//	{
//	case NOTIFICATION_ENTER_TREE:
//	{
//	}
//	break;
//	case NOTIFICATION_WM_CLOSE_REQUEST:
//	{
//	}
//	break;
//	case NOTIFICATION_PREDELETE:
//	{
//	}
//	break;
//	case NOTIFICATION_RESIZED:
//	{
//		godot::Vector2 newSize = get_size();
//
//		GDN_TheWorld_WorldViewport* worldViewport = getWorldViewport();
//		if (worldViewport != nullptr)
//			worldViewport->set_size(newSize);
//	}
//	break;
//	}
//}
//
//void GDN_TheWorld_WorldViewportContainer::_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}
//
//void GDN_TheWorld_WorldViewportContainer::_unhandled_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}
//
//void GDN_TheWorld_WorldViewportContainer::_ready(void)
//{
//}
//
//void GDN_TheWorld_WorldViewportContainer::_process(double _delta)
//{
//}

void GDN_TheWorld_Map::_bind_methods(void)
{
	//ClassDB::bind_method(D_METHOD("get_scale_factor"), &GDN_TheWorld_Map::getScaleFactor);
}

GDN_TheWorld_Map::GDN_TheWorld_Map(void)
{
}

GDN_TheWorld_Map::~GDN_TheWorld_Map(void)
{
}

void GDN_TheWorld_Map::init(GDN_TheWorld_MapViewer* mapViewer)
{
	m_mapViewer = mapViewer;
	//setScale(m_scaleFactor);
}


void GDN_TheWorld_Map::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
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
	//case NOTIFICATION_RESIZED:
	//{
	//	godot::Vector2 newSize = get_size();
	//}
	//break;
	}
}

void GDN_TheWorld_Map::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_Map::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_Map::_ready(void)
{
}

void GDN_TheWorld_Map::_process(double _delta)
{
	//GDN_TheWorld_Viewer* viewver = m_mapViewer->getViewer();
	//
	//if (m_zoomRequired != 0)
	//{
	//	constexpr float SCALE_INCREMENET = 0.05f;
	//	float scaleFactor = m_scaleFactor;
	//	scaleFactor += SCALE_INCREMENET * m_zoomRequired;
	//	if (scaleFactor > 0)
	//	{
	//		setScale(scaleFactor);
	//	}

	//	m_zoomRequired = 0;

	//	viewver->forceProcessNewMapQuadrants(true);
	//}
}

//void GDN_TheWorld_Map::setScale(float scaleFactor)
//{
//	if (scaleFactor > 0)
//	{
//		godot::Vector2 scale(scaleFactor, scaleFactor);
//		set_scale(scale);
//		m_scaleFactor = scaleFactor;
//	}
//}

godot::Size2 GDN_TheWorld_Map::getScreenSizeInGlobalCoord(void) const
{
	godot::Size2 screenSize;

	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	godot::Size2 windowSize = viewer->getViewportSize();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();

	screenSize = windowSize * (1 / mapCamera->getZoomFactor());

	return screenSize;
}

godot::Vector2 GDN_TheWorld_Map::getMousePosInGlobalCoord(void) const
{
	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();
	
	godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
	godot::Vector2 distanceFromUpLeftScreenBorderInGlobalCoord = mousePosInViewport * (1 / mapCamera->getZoomFactor());
	godot::Rect2 screenRectInGlobalCoord = getScreenRectInGlobalCoord();
	godot::Vector2 mousePosInGlobalCoord(screenRectInGlobalCoord.position.x + distanceFromUpLeftScreenBorderInGlobalCoord.x, screenRectInGlobalCoord.position.y + distanceFromUpLeftScreenBorderInGlobalCoord.y);
	return mousePosInGlobalCoord;
}

godot::Rect2 GDN_TheWorld_Map::getScreenRectInGlobalCoord(void) const
{
	godot::Size2 screenSizeInGlobalCoord = getScreenSizeInGlobalCoord();
	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();
	godot::Vector2 cameraScreenCenterPos = mapCamera->get_screen_center_position();

	return godot::Rect2(cameraScreenCenterPos - screenSizeInGlobalCoord / 2, screenSizeInGlobalCoord);
}

//godot::Vector2i GDN_TheWorld_Map::getTextureRectIdxFromWorldPos(float x, float z, int level, int numVerticesPerSize, float gridStepInWU) const
//{
//	float sizeInWU = (numVerticesPerSize - 1) * gridStepInWU;
//	float lowerX = floor(x / sizeInWU) * sizeInWU;
//	float lowerZ = floor(z / sizeInWU) * sizeInWU;
//	return godot::Vector2i((int)(lowerX / sizeInWU), (int)(lowerZ / sizeInWU));
//}

godot::Vector2i GDN_TheWorld_Map::getTextureRectSize(void) const
{
	godot::Vector2 imageSize = getImageSize();
	return (imageSize * godot::Vector2i(getNumImagesForTextureRectSide(), getNumImagesForTextureRectSide())) /* * getScaleFactor()*/;
}

godot::Vector2i GDN_TheWorld_Map::getImageSize(void) const
{
	return godot::Vector2i(m_mapViewer->getViewer()->getImageMapResolution(), m_mapViewer->getViewer()->getImageMapResolution());
}

godot::Vector2 GDN_TheWorld_Map::globalPosToMapPos(godot::Vector2 global)
{
	godot::Vector2 mapPos = global / getImageSize();
	mapPos = mapPos * Globals()->heightmapSizeInWUs();
	return mapPos;
}

godot::Vector2 GDN_TheWorld_Map::mapPosToGlobalPos(godot::Vector2 mapPos)
{
	godot::Vector2 globalPos = mapPos / Globals()->heightmapSizeInWUs();
	globalPos = globalPos * getImageSize();
	return globalPos;
}

int32_t GDN_TheWorld_Map::getNumTextureRects(void)
{
	int32_t num = 0;
	
	godot::Array nodes = get_children();
	for (int i = 0; i < nodes.size(); i++)
	{
		godot::Node* node = godot::Object::cast_to<godot::Node>(nodes[i]);
		if (node->is_in_group("MapTextureRect"))
			num++;
	}

	return num;
}

void GDN_TheWorld_Map::setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos, bool replace)
{
	if (mapImage == nullptr)
		return;

	constexpr size_t NUM_PADDING_ZEROES = 6;
	std::string strPosX, strPosZ;
	int posX = quadrantPos.getIndexPosX(), posZ = quadrantPos.getIndexPosZ();
	if (posX < 0)
		strPosX = "-" + TheWorld_Utils::padIntValueTo(-posX, NUM_PADDING_ZEROES);
	else
		strPosX = TheWorld_Utils::padIntValueTo(posX, NUM_PADDING_ZEROES);
	if (posZ < 0)
		strPosZ = "-" + TheWorld_Utils::padIntValueTo(-posZ, NUM_PADDING_ZEROES);
	else
		strPosZ = TheWorld_Utils::padIntValueTo(posZ, NUM_PADDING_ZEROES);

	std::string rectName = "Quadrant_" + strPosX + "_" + strPosZ;

	godot::TextureRect* textureRect = nullptr;
	godot::Node* n = find_child(rectName.c_str(), false, true);
	if (n != nullptr)
	{
		textureRect = godot::Object::cast_to<godot::TextureRect>(n);
		my_assert(textureRect != nullptr);
	}

	godot::Vector2i imageSize = mapImage->get_size();
	
	if (textureRect == nullptr)
	{
		textureRect = memnew(godot::TextureRect);
		textureRect->set_name(rectName.c_str());
		textureRect->add_to_group("MapTextureRect");
		add_child(textureRect);
		godot::Vector2i textureRectSize = getImageSize();		// occhio!
		my_assert(textureRectSize == imageSize);
		textureRect->set_size(textureRectSize);
		godot::Vector2 pos(real_t(quadrantPos.getIndexPosX() * textureRectSize.x), real_t(quadrantPos.getIndexPosZ() * textureRectSize.y));
		textureRect->set_position(pos);

		godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(mapImage);
		textureRect->set_texture(tex);
	}
	else if (replace)
	{
		godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(mapImage);
		textureRect->set_texture(tex);
	}
}

void GDN_TheWorld_Map::setWorldFlag(godot::String name, godot::Transform3D worldTransform, godot::Color c, bool setCameraDir, bool visible)
{
	//godot::MeshInstance2D* worldFlag = nullptr;
	//godot::Line2D* worldDir = nullptr;
	godot::Node2D* worldFlag = nullptr;
	if (m_worldFlagDict.has(name))
	{
		worldFlag = Object::cast_to<godot::Node2D>(m_worldFlagDict[name]);
		//godot::Dictionary worldFlagPartsDict = m_worldFlagDict[name];
		//if (worldFlagPartsDict.has("flag"))
		//	worldFlag = Object::cast_to<godot::MeshInstance2D>((*worldFlagPartsDict)["flag"]);
		//if (worldFlagPartsDict.has("dir"))
		//	worldDir = Object::cast_to<godot::Line2D>((*worldFlagPartsDict)["dir"]);
	}
	else
	{
		worldFlag = memnew(godot::Node2D);
		add_child(worldFlag);
		worldFlag->set_name("WorldFlag_" + name);
		worldFlag->set_visible(false);
		worldFlag->set_z_index(worldFlag->get_z_index() + 1);

		constexpr int32_t FLAG_RADIUS = 5;
		godot::Ref<godot::SphereMesh> sphereFlag = memnew(godot::SphereMesh);
		sphereFlag->set_radius(FLAG_RADIUS);
		sphereFlag->set_height(FLAG_RADIUS * 2);

		godot::MeshInstance2D* flag = memnew(godot::MeshInstance2D);
		worldFlag->add_child(flag);
		flag->set_name("flag");
		flag->set_mesh(sphereFlag);
		flag->set_modulate(c);
		
		if (setCameraDir)
		{
			godot::Line2D* dir = memnew(godot::Line2D);
			worldFlag->add_child(dir);
			dir->set_name("dir");
			dir->set_default_color(c);
			dir->set_width(1);
			dir->add_point(godot::Vector2(0, 0));
			dir->add_point(godot::Vector2(0, -FLAG_RADIUS * 3));
		}
		
		m_worldFlagDict[name] = worldFlag;
	}

	if (worldFlag != nullptr)
	{
		godot::Vector2 worldFlagGlobalPosition = m_mapViewer->getViewer()->mapPosToGlobalPos(godot::Vector2(worldTransform.origin.x, worldTransform.origin.z));
		worldFlag->set_global_position(worldFlagGlobalPosition);

		if (setCameraDir)
		{
			godot::Transform2D resetTransform(0, worldFlagGlobalPosition);
			worldFlag->set_transform(resetTransform);
			godot::Basis b = worldTransform.basis;
			godot::Vector3 worldCameraDir = -b.rows[b.rows->AXIS_Z];
			godot::Vector2 maCameraDir(worldCameraDir.x, worldCameraDir.z);
			float mapAngleToNorth = maCameraDir.angle_to(godot::Vector2(0, -1));
			worldFlag->rotate(mapAngleToNorth);
		}

		worldFlag->set_visible(visible);
	}
}

GDN_TheWorld_Globals* GDN_TheWorld_Map::Globals(bool useCache)
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
		m_globals = godot::Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}


void GDN_TheWorld_MapViewport::_bind_methods(void)
{
}

GDN_TheWorld_MapViewport::GDN_TheWorld_MapViewport(void)
{
}

GDN_TheWorld_MapViewport::~GDN_TheWorld_MapViewport(void)
{
}

void GDN_TheWorld_MapViewport::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
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

void GDN_TheWorld_MapViewport::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewport::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewport::_ready(void)
{
}

void GDN_TheWorld_MapViewport::_process(double _delta)
{
}

void GDN_TheWorld_MapViewportContainer::_bind_methods(void)
{
}

GDN_TheWorld_MapViewportContainer::GDN_TheWorld_MapViewportContainer(void)
{
}

GDN_TheWorld_MapViewportContainer::~GDN_TheWorld_MapViewportContainer(void)
{
}

void GDN_TheWorld_MapViewportContainer::_notification(int p_what)
{
	switch (p_what)
	{
	case NOTIFICATION_ENTER_TREE:
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
	case NOTIFICATION_RESIZED:
	{
		godot::Vector2 newSize = get_size();

		GDN_TheWorld_MapViewer* mapViewer = getMapViewer();
		if (mapViewer != nullptr)
		{
			GDN_TheWorld_MapViewport* mapViewport = getMapViewport();
			if (mapViewport != nullptr)
				mapViewport->set_size(newSize);
			//GDN_TheWorld_Map* map = mapViewer->getMap();
			//if (map != nullptr)
			//	map->set_size(newSize);
		}
	}
	break;
	}
}

void GDN_TheWorld_MapViewportContainer::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewportContainer::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewportContainer::_ready(void)
{
}

void GDN_TheWorld_MapViewportContainer::_process(double _delta)
{
}

void GDN_TheWorld_MapViewer::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("get_map"), &GDN_TheWorld_MapViewer::getMap);
}

GDN_TheWorld_MapViewer::GDN_TheWorld_MapViewer(void)
{
}

GDN_TheWorld_MapViewer::~GDN_TheWorld_MapViewer(void)
{
}

void GDN_TheWorld_MapViewer::_notification(int p_what)
{
}

void GDN_TheWorld_MapViewer::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewer::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_MapViewer::_ready(void)
{
}

void GDN_TheWorld_MapViewer::_process(double _delta)
{
}

void GDN_TheWorld_HudControls::_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

void GDN_TheWorld_HudControls::_unhandled_input(const Ref<InputEvent>& event)
{
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());

	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
	//{
	//	int a = 0;
	//}
}

int32_t GDN_TheWorld_MapModder::s_numWorking = 0;
std::recursive_mutex GDN_TheWorld_MapModder::s_mtxInternal;

// World Node Local Coordinate System is the same as MapManager coordinate system
// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

void GDN_TheWorld_MapModder::_bind_methods()
{
	godot::ClassDB::bind_method(D_METHOD("frame_post_draw"), &GDN_TheWorld_MapModder::framePostDraw);

	ADD_SIGNAL(godot::MethodInfo("tw_modder_work_done", godot::PropertyInfo(godot::Variant::INT, "map_type"), godot::PropertyInfo(godot::Variant::FLOAT, "x"), godot::PropertyInfo(godot::Variant::FLOAT, "z"), godot::PropertyInfo(godot::Variant::INT, "num_vertices_per_side"), godot::PropertyInfo(godot::Variant::INT, "level"), godot::PropertyInfo(godot::Variant::FLOAT, "grid_step")));
}

GDN_TheWorld_MapModder::GDN_TheWorld_MapModder()
{
}

GDN_TheWorld_MapModder::~GDN_TheWorld_MapModder()
{
	deinit();

	if (m_debugSprite != nullptr)
	{
		Node* parent = m_debugSprite->get_parent();
		if (parent != nullptr)
			parent->remove_child(m_debugSprite);
		m_debugSprite->queue_free();
		m_debugSprite = nullptr;
	}
}

void GDN_TheWorld_MapModder::framePostDraw(void)
{
	m_frameAvailable = true;
}

void GDN_TheWorld_MapModder::initForNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, size_t tileSize, size_t viewportBorder, bool signalRequired)
{
	my_assert(m_initialized == MapType::uninitialized);

	if (m_initialized == MapType::uninitialized)
	{
		GDN_TheWorld_MapModder::incNumWorking();
		
		m_signalRequired = signalRequired;
		m_quadTree = quadtree;
		m_float32HeigthsBuffer = float32HeigthsBuffer;
		m_numHeights = (size_t)sqrt(float(m_float32HeigthsBuffer->size() / sizeof(float)));
		my_assert(size_t(pow((float)m_numHeights, 2) * sizeof(float)) == m_float32HeigthsBuffer->size());

		m_tileSize = tileSize;
		m_viewportBorder = viewportBorder;
		deinitRequired(false);
		m_initialized = MapType::initializedForNormals;

		startProcessRequired(true);
	}
}

void GDN_TheWorld_MapModder::initForSplatmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer,
	float slopeVerticalFactor,
	float slopeFlatFactor,
	float dirtOnRocksFactor,
	float highElevationFactor,
	float lowElevationFactor,
	float minHeigth,
	float maxHeigth,
	size_t tileSize, size_t viewportBorder, bool signalRequired)
{
	my_assert(m_initialized == MapType::uninitialized);

	if (m_initialized == MapType::uninitialized)
	{
		GDN_TheWorld_MapModder::incNumWorking();
		
		m_signalRequired = signalRequired;
		m_quadTree = quadtree;
		m_float32HeigthsBuffer = float32HeigthsBuffer;
		m_numHeights = (size_t)sqrt(float(m_float32HeigthsBuffer->size() / sizeof(float)));
		my_assert(size_t(pow((float)m_numHeights, 2) * sizeof(float)) == m_float32HeigthsBuffer->size());
		m_normalsBuffer = normalsBuffer;
		m_numNormals = (size_t)sqrt(float(m_normalsBuffer->size() / sizeof(TheWorld_Utils::_RGB)));
		my_assert(size_t(pow((float)m_numNormals, 2) * sizeof(TheWorld_Utils::_RGB)) == m_normalsBuffer->size());
		my_assert(m_numNormals == 0 || m_numHeights == m_numNormals);
		m_slopeVerticalFactor = slopeVerticalFactor;
		m_slopeFlatFactor = slopeFlatFactor;
		m_dirtOnRocksFactor = dirtOnRocksFactor;
		m_highElevationFactor = highElevationFactor;
		m_lowElevationFactor = lowElevationFactor;
		m_tileSize = tileSize;
		m_viewportBorder = viewportBorder;
		m_minHeigth = minHeigth;
		m_maxHeigth = maxHeigth;
		deinitRequired(false);
		m_initialized = MapType::initializedForSplatmap;

		startProcessRequired(true);
	}
}

void GDN_TheWorld_MapModder::initForGlobalMap(QuadTree* quadtree, size_t tileSize, bool signalRequired)
{
	my_assert(m_initialized == MapType::uninitialized);

	if (m_initialized == MapType::uninitialized)
	{
		GDN_TheWorld_MapModder::incNumWorking();

		m_signalRequired = signalRequired;
		m_quadTree = quadtree;
		deinitRequired(false);
		//m_tileSize = 512;
		m_tileSize = tileSize;
		while (m_tileSize > m_viewer->Globals()->heightmapResolution())
			m_tileSize /= 2;
		//m_tileSize = m_viewer->Globals()->heightmapResolution();
		m_viewportBorder = 0;
		m_initialized = MapType::initializedForGlobalmap;
		startProcessRequired(true);
	}
}

bool GDN_TheWorld_MapModder::ready(void)
{
	if (working())
		return false;

	my_assert(m_initialized == MapType::uninitialized || deinitRequired());

	if (m_initialized != MapType::uninitialized)
		return false;

	return true;
}

bool GDN_TheWorld_MapModder::working(void)
{
	if (startProcessRequired())
		return true;

	if (m_processingTile)
		return true;

	return workToDo();
}

bool GDN_TheWorld_MapModder::workToDo(void)
{
	if (m_tilesQueue.size() > 0)
		return true;
	else
		return false;
}

TheWorld_Utils::MemoryBuffer* GDN_TheWorld_MapModder::getOutBuffer(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1 ") + __FUNCTION__, "ALL");

	if (!m_outImageValid)
		return nullptr;

	m_outBuffer.reset();
	m_outBuffer = std::make_unique< TheWorld_Utils::MemoryBuffer>();

	if (m_initialized == MapType::initializedForNormals)
	{
		godot::Image::Format outImageFormat = m_outImage->get_format();

		// DEBUG CHECK DO NOT REMOVE
		//{
		//	float originalHeight, pongHeinght;
		//	size_t numVerticesPerSize = m_viewer->Globals()->numVerticesPerQuadrantSize();
		//	int32_t w = m_outImage->get_width();
		//	int32_t h = m_outImage->get_height();
		//	if (outImageFormat == godot::Image::FORMAT_RGBA8)	// debug pong shader which reply in each pixel exactly the heigth in input to the fragment
		//	{	// for rgba8 must be set m_viewport->set_transparent_background(true);
		//		godot::Ref<godot::Image> image = godot::Image::create_from_data(w, h, false, godot::Image::FORMAT_RF, m_outImage->get_data());
		//		for (int x = 0; x < w; x++)
		//			for (int y = 0; y < h; y++)
		//			{
		//				originalHeight = m_float32HeigthsBuffer->at<float>(x, y, numVerticesPerSize);
		//				pongHeinght = image->get_pixel(x, y).r;
		//			}
		//	}
		//}
		// DEBUG CHECK DO NOT REMOVE

		if (outImageFormat != godot::Image::FORMAT_RGB8)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "convert format");
			m_outImage->convert(godot::Image::FORMAT_RGB8);
		}

		{
			// normals from shader are in format RGB8: shader calculate and pack them filling COLOR.rgb with values in the range 0.0/1.0 then the engine trasform them in sequences of three byte (first for r, second for g, third for b)
			// multiplied by 255 ==> to have normals we have to divide each byte (integer in the range 0-255) by 255 obtaining three floats and unpack the relative vector of floats

			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "copy normals out buffer");
			godot::PackedByteArray data = m_outImage->get_data();
			int64_t bufferSize = data.size();
			m_outBuffer->reserve(bufferSize);
			memcpy(m_outBuffer->ptr(), data.ptr(), bufferSize);
			m_outBuffer->adjustSize(bufferSize);
		}

		// DEBUG CHECK DO NOT REMOVE
		//{
		//	godot::Color c;
		//	float originalHeight;
		//	size_t numVerticesPerSize = m_viewer->Globals()->numVerticesPerQuadrantSize();
		//	int32_t w = m_outImage->get_width();
		//	int32_t h = m_outImage->get_height();
		//	for (int x = 0; x < w; x++)
		//		for (int y = 0; y < h; y++)
		//		{
		//			originalHeight = m_float32HeigthsBuffer->at<float>(x, y, numVerticesPerSize);
		//			c = m_outImage->get_pixel(x, y);	// packed normal 0.0-1.0
		//			float normX, normY, normZ;
		//			TheWorld_Utils::unpackNormal(c.r, c.g, c.b, normX, normY, normZ);	// unpacked normal 0.0-1.0
		//			TheWorld_Utils::_RGB rgb = m_outBuffer->at<TheWorld_Utils::_RGB>(x, y, numVerticesPerSize);	// 0-255
		//			float _normX, _normY, _normZ;
		//			TheWorld_Utils::unpackNormal(float(rgb.r) / 255, float(rgb.g) / 255, float(rgb.b) / 255, _normX, _normY, _normZ);	// unpacked normal 0.0-1.0
		//			my_assert(normX == _normX && normY == _normY && normZ == _normZ);
		//		}
		//}
		// DEBUG CHECK DO NOT REMOVE
	}
	else if (m_initialized == MapType::initializedForSplatmap)
	{
		godot::Image::Format outImageFormat = m_outImage->get_format();
		my_assert(outImageFormat == godot::Image::FORMAT_RGBA8);
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "copy splatmap out buffer");
			godot::PackedByteArray data = m_outImage->get_data();
			int64_t bufferSize = data.size();
			m_outBuffer->reserve(bufferSize);
			memcpy(m_outBuffer->ptr(), data.ptr(), bufferSize);
			m_outBuffer->adjustSize(bufferSize);
		}
	}
	else if (m_initialized == MapType::initializedForGlobalmap)
	{
		godot::Image::Format outImageFormat = m_outImage->get_format();

		if (m_debugSprite != nullptr)
		{
			std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".globalmap.png";
			m_outImage->save_png(outFileName.c_str());
		}

		if (outImageFormat != godot::Image::FORMAT_RGB8)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "convert format");
			m_outImage->convert(godot::Image::FORMAT_RGB8);
		}

		{
			// global map is an ALBEDO map so color in format RGB
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "copy globalmap out buffer");
			godot::PackedByteArray data = m_outImage->get_data();
			int64_t bufferSize = data.size();
			m_outBuffer->reserve(bufferSize);
			memcpy(m_outBuffer->ptr(), data.ptr(), bufferSize);
			m_outBuffer->adjustSize(bufferSize);
		}
	}

	return m_outBuffer.get();
}

void GDN_TheWorld_MapModder::startProcessing(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1 ") + __FUNCTION__, "ALL");

	//if (m_initialized == MapType::uninitialized)
	my_assert(!(m_initialized == MapType::uninitialized));

	set_process(false);

	// clear
	m_tiles.clear();
	std::queue<godot::Vector2i> empty;
	std::swap(m_tilesQueue, empty);
	m_processingTile = false;

	godot::Node* parent = get_parent();
	if (parent != nullptr)
		parent->remove_child(this);
	m_parent->add_child(this);
	my_assert(is_inside_tree());

	m_mapModderViewport = memnew(godot::SubViewport);
	add_child(m_mapModderViewport);
	m_mapModderViewport->set_clear_mode(godot::SubViewport::ClearMode::CLEAR_MODE_ALWAYS);
	godot::Ref<godot::World3D> w = memnew(godot::World3D);
	m_mapModderViewport->set_world_3d(w);
	m_mapModderViewport->set_use_own_world_3d(true);
	if (m_initialized == MapType::initializedForGlobalmap)
	{
		m_mapModderViewport->set_size(godot::Vector2i(int32_t(m_tileSize + 1), int32_t(m_tileSize + 1)));
		m_mapModderViewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_ALWAYS);
		m_mapModderViewport->set_debug_draw(godot::Viewport::DebugDraw::DEBUG_DRAW_UNSHADED);

		// assume square buffer
		m_imageSize = (size_t)sqrt(float(m_quadTree->getQuadrant()->getFloat32HeightsBuffer().size() / sizeof(float)));
		my_assert(size_t(pow((float)m_imageSize, 2) * sizeof(float)) == m_quadTree->getQuadrant()->getFloat32HeightsBuffer().size());	// verify buffer is square
	}
	else
	{
		m_mapModderViewport->set_size(godot::Vector2i(int32_t(m_tileSize + m_viewportBorder * 2), int32_t(m_tileSize + m_viewportBorder * 2)));
		m_mapModderViewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_DISABLED);
		m_map = memnew(godot::Sprite2D);
		m_map->set_centered(false);
		m_mapModderViewport->add_child(m_map);

		// assume square buffer
		m_imageSize = (size_t)sqrt(float(m_float32HeigthsBuffer->size() / sizeof(float)));
		my_assert(size_t(pow((float)m_imageSize, 2) * sizeof(float)) == m_float32HeigthsBuffer->size());	// verify buffer is square
		//my_assert(m_imageSize / m_tileSize * m_tileSize == m_imageSize - 1);								// verify tile can cover all the image with the exception of the left/botton border
	}

	godot::RenderingServer* vs = godot::RenderingServer::get_singleton();
	godot::Error e = vs->connect("frame_post_draw", Callable(this, "frame_post_draw"));

	godot::Vector2i viewportSize = m_mapModderViewport->get_size();
	my_assert(viewportSize.x == viewportSize.y);
	my_assert(m_imageSize >= viewportSize.x);

	size_t numFullTiles = (size_t)std::floor(float(m_imageSize) / float(m_tileSize));

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.1 ") + __FUNCTION__, "Put tiles in queue");

		for (int y = 0; y < numFullTiles; y++)
			for (int x = 0; x < numFullTiles; x++)
			{
				godot::Vector2i tilePos(x, y);
				if (m_tiles.contains(tilePos) && m_tiles[tilePos] == c_tileStatusWaiting)
					continue;
				else
				{
					m_tiles[tilePos] = c_tileStatusWaiting;
					m_tilesQueue.push(tilePos);
				}
			}
	}

	godot::Ref<godot::ImageTexture> heightmapTexture;
	if (m_initialized != MapType::initializedForGlobalmap)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.2 ") + __FUNCTION__, "Create Image/Texture from heights buffer");

		godot::PackedByteArray data;
		data.resize((int64_t)m_float32HeigthsBuffer->size());
		{
			memcpy((char*)data.ptrw(), m_float32HeigthsBuffer->ptr(), m_float32HeigthsBuffer->size());
		}
		godot::Ref<godot::Image> image = godot::Image::create_from_data((int32_t)m_imageSize, (int32_t)m_imageSize, false, Image::FORMAT_RF, data);
		// DEBUG
		//{
		//	int32_t x = 0, y = 0;
		//	Color c = image->get_pixel(x, y);
		//	size_t numVerticesPerSize = m_viewer->Globals()->numVerticesPerQuadrantSize();
		//	float height = m_float32HeigthsBuffer->at<float>(x, y, numVerticesPerSize);
		//	int i = 0;
		//}
		// DEBUG

		heightmapTexture = godot::ImageTexture::create_from_image(image);
		//godot::Ref<godot::ImageTexture> heigthTexture;
		//heigthTexture = tex;
		//m_map->set_texture(heigthTexture);
		m_map->set_texture(heightmapTexture);
		//heightmapTexture = tex;
	}

	if (m_initialized == MapType::initializedForNormals)
	{
		//m_viewport->set_transparent_background(true);		// needed to have output texture RGBA8

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.3 ") + __FUNCTION__, "Normals Material stuff");

			godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
			godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
			//godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/tools/shaders/height2normal.gdshader", "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
			godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/tools/shaders/height2normal.gdshader");
			mat->set_shader(shader);
			float gridStepInWU = m_viewer->Globals()->gridStepInWU();
			mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
			m_map->set_material(mat);
		}
	}
	else if (m_initialized == MapType::initializedForSplatmap)
	{
		m_mapModderViewport->set_transparent_background(true);		// needed to have output texture RGBA8

		godot::Ref<godot::ImageTexture> normalTexture;
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.4 ") + __FUNCTION__, "Create Image/Texture from normals buffer");
			godot::PackedByteArray data;
			data.resize((int)m_normalsBuffer->size());
			{
				memcpy((char*)data.ptrw(), m_normalsBuffer->ptr(), m_normalsBuffer->size());
			}
			godot::Ref<godot::Image> image = godot::Image::create_from_data(int32_t(m_imageSize), int32_t(m_imageSize), false, Image::FORMAT_RGB8, data);
			godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
			normalTexture = tex;
		}

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.5 ") + __FUNCTION__, "splatmap Material stuff");

			godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
			godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
									//godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/tools/shaders/height2splatmap.gdshader", "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
									//godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/tools/shaders/height2splatmap.gdshader");
			//godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/tools/shaders/height2splatmap.tres");
			godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/test/height2splatmap.tres");
			mat->set_shader(shader);
			//float gridStepInWU = m_viewer->Globals()->gridStepInWU();
			//mat->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);
			//mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
			mat->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, normalTexture);
			mat->set_shader_parameter(SHADER_PARAM_SLOPE_VERTICAL_FACTOR, m_slopeVerticalFactor);
			mat->set_shader_parameter(SHADER_PARAM_SLOPE_FLAT_FACTOR, m_slopeFlatFactor);
			mat->set_shader_parameter(SHADER_PARAM_DIRT_ON_ROCKS_FACTOR, m_dirtOnRocksFactor);
			mat->set_shader_parameter(SHADER_PARAM_HIGH_ELEVATION_FACTOR, m_highElevationFactor);
			mat->set_shader_parameter(SHADER_PARAM_LOW_ELEVATION_FACTOR, m_lowElevationFactor);
			mat->set_shader_parameter(SHADER_PARAM_MIN_HEIGHT, m_minHeigth);
			mat->set_shader_parameter(SHADER_PARAM_MAX_HEIGHT, m_maxHeigth);
			//mat->set_shader_parameter(SHADER_PARAM_HIGH_MOUNTAINS_ALTITUDE_THRESHOLD, DEFAULT_HIGH_MOUNTAINS_ALTITUDE_THRESHOLD);
			//mat->set_shader_parameter(SHADER_PARAM_MOUNTAINS_ALTITUDE_THRESHOLD, DEFAULT_MOUNTAINS_ALTITUDE_THRESHOLD);
			//mat->set_shader_parameter(SHADER_PARAM_LOW_MOUNTAINS_ALTITUDE_THRESHOLD, DEFAULT_LOW_MOUNTAINS_ALTITUDE_THRESHOLD);
			//mat->set_shader_parameter(SHADER_PARAM_HILLS_ALTITUDE_THRESHOLD, DEFAULT_HILLS_ALTITUDE_THRESHOLD);
			//mat->set_shader_parameter(SHADER_PARAM_PLAINS_ALTITUDE_THRESHOLD, DEFAULT_PLAINS_ALTITUDE_THRESHOLD);

			{
				Quadrant* quadrant = m_quadTree->getQuadrant();
				TheWorld_Utils::QuadrantPos quadrantPos = quadrant->getPos();
				
				TheWorld_Utils::QuadrantPos northPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
				QuadTree* northQuadTree = m_viewer->getQuadTree(northPos, true, false, true);
				if (northQuadTree != nullptr && northQuadTree->statusInitialized())
				{
					godot::Vector2 heights(northQuadTree->getQuadrant()->getTerrainEdit()->minHeight, northQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
					mat->set_shader_parameter(SHADER_PARAM_NORTH_HEIGHTS, heights);
					m_northQuadrantheights = heights;
				}
				else
				{
					godot::Vector2 heights(FLT_MAX, FLT_MIN);
					mat->set_shader_parameter(SHADER_PARAM_NORTH_HEIGHTS, heights);
					m_northQuadrantheights = heights;
				}
				TheWorld_Utils::QuadrantPos southPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
				QuadTree* southQuadTree = m_viewer->getQuadTree(southPos, true, false, true);
				if (southQuadTree != nullptr && southQuadTree->statusInitialized())
				{
					godot::Vector2 heights(southQuadTree->getQuadrant()->getTerrainEdit()->minHeight, southQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
					mat->set_shader_parameter(SHADER_PARAM_SOUTH_HEIGHTS, heights);
					m_southQuadrantheights = heights;
				}
				else
				{
					godot::Vector2 heights(FLT_MAX, FLT_MIN);
					mat->set_shader_parameter(SHADER_PARAM_SOUTH_HEIGHTS, heights);
					m_southQuadrantheights = heights;
				}
				TheWorld_Utils::QuadrantPos westPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
				QuadTree* westQuadTree = m_viewer->getQuadTree(westPos, true, false, true);
				if (westQuadTree != nullptr && westQuadTree->statusInitialized())
				{
					godot::Vector2 heights(westQuadTree->getQuadrant()->getTerrainEdit()->minHeight, westQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
					mat->set_shader_parameter(SHADER_PARAM_WEST_HEIGHTS, heights);
					m_westQuadrantheights = heights;
				}
				else
				{
					godot::Vector2 heights(FLT_MAX, FLT_MIN);
					mat->set_shader_parameter(SHADER_PARAM_WEST_HEIGHTS, heights);
					m_westQuadrantheights = heights;
				}
				TheWorld_Utils::QuadrantPos eastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
				QuadTree* eastQuadTree = m_viewer->getQuadTree(eastPos, true, false, true);
				if (eastQuadTree != nullptr && eastQuadTree->statusInitialized())
				{
					godot::Vector2 heights(eastQuadTree->getQuadrant()->getTerrainEdit()->minHeight, eastQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
					mat->set_shader_parameter(SHADER_PARAM_EAST_HEIGHTS, heights);
					m_eastQuadrantheights = heights;
				}
				else
				{
					godot::Vector2 heights(FLT_MAX, FLT_MIN);
					mat->set_shader_parameter(SHADER_PARAM_EAST_HEIGHTS, heights);
					m_eastQuadrantheights = heights;
				}
			}

			m_map->set_material(mat);
		}
	}
	else if (m_initialized == MapType::initializedForGlobalmap)
	{
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.6 ") + __FUNCTION__, "globalmap generating mesh");

			m_terrainPlane = memnew(godot::MeshInstance3D);
			m_mapModderViewport->add_child(m_terrainPlane);

			{
				const int32_t numVerticesPerTile = (int32_t)m_tileSize;
				const size_t numVerticesPerQuadrant = m_viewer->Globals()->heightmapResolution();
				//godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(numVerticesPerTile, 0, float(quadrantSize / numVerticesPerTile) * gridStepInWU);
				godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(numVerticesPerTile, 0, float(numVerticesPerQuadrant / numVerticesPerTile));
				//const int32_t planeResolution = 16;	// WARNING: reducing number of vertices will result very imprecise because, for optimizing normal viewport, splatmap is fetched in vertex shader rather than
														// in fragment shader (less time) but for this reason splatmap values will result from an interpolation of real splatmap values being very imprecise
														// calculating globalmap we need more precise calculation of splatmap values so, to use same shader, we have to increment the number of vertices to reduce interpolation effects
				//godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(planeResolution, 0, float(m_imageSize / planeResolution) * gridStepInWU);
				m_displacementPlane = Vector3(0.0f, 0.0f, 0.0f) * gridStepInWU;
				m_terrainPlane->set_mesh(mesh);
			}

			m_camera = memnew(godot::Camera3D);
			//m_camera->set_projection(godot::Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
			m_camera->set_projection(godot::Camera3D::ProjectionType::PROJECTION_PERSPECTIVE);
			godot::Vector2i viewportSize = m_mapModderViewport->get_size();
			m_cameraAltitude = (float)m_tileSize * 1.3f;
			m_camera->set_size(viewportSize.x);
			m_camera->set_near(0.5f);		// 0.1f
			m_camera->set_far(m_viewer->Globals()->heightmapSizeInWUs() + m_cameraAltitude);	// 20000.0f
			m_camera->set_current(true);
			m_camera->set_rotation(godot::Vector3(-TheWorld_Utils::kPi / 2, 0, 0));
			m_mapModderViewport->add_child(m_camera);
		}

		godot::Ref<godot::Material> terrainPlaneMaterial;

		//int shaderType = 0;		// StandardMaterial3D
		//int shaderType = 1;		// LookDev
		int shaderType = 2;		// Regular

		//if (shaderType == 0)
		//{
		//	godot::Ref<godot::StandardMaterial3D> material = memnew(godot::StandardMaterial3D);
		//	material->set_albedo(godot::Color(0.0f, 1.0f, 0.0f));
		//	terrainPlaneMaterial = material;
		//}
		//else if (shaderType == 1)
		//{
		//	godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getLookDevMaterial(ShaderTerrainData::LookDev::Splat);		// SHADER_PARAM_LOOKDEV_MAP
		//	godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
		//	material->set_shader(currentMaterial->get_shader());

		//	int numVerticesPerQuadrantSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();

		//	{
		//		size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
		//		godot::PackedByteArray data;
		//		size_t heightmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * uint16_t_size;
		//		data.resize((int)heightmapBufferSizeInBytes);
		//		size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
		//		assert(heightmapBufferSizeInBytes == heightsBufferSize);
		//		{
		//			memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
		//		}
		//		godot::Ref<godot::Image> image = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RH, data);
		//		// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
		//		//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, tex);
		//	}

		//	{
		//		godot::Ref<godot::Image> image;
		//		size_t normalsBufferSize = 0;
		//		normalsBufferSize = m_quadTree->getQuadrant()->getNormalsBuffer().size();
		//		if (normalsBufferSize == 0)
		//		{
		//			godot::Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::FORMAT_RGB8);
		//			image->fill(godot::Color(0, 0, 0, 1.0));
		//			image = im;
		//		}
		//		else
		//		{
		//			godot::PackedByteArray data;
		//			size_t normalmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGB);
		//			data.resize((int)normalmapBufferSizeInBytes);
		//			assert(normalmapBufferSizeInBytes == normalsBufferSize);
		//			{
		//				memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getNormalsBuffer().ptr(), normalsBufferSize);
		//			}
		//			godot::Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGB8, data);
		//			image = im;
		//		}
		//		// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
		//		//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, tex);
		//	}

		//	{
		//		Ref<godot::Image> image;
		//		size_t splatmapBufferSize = 0;
		//		splatmapBufferSize = m_quadTree->getQuadrant()->getSplatmapBuffer().size();
		//		if (splatmapBufferSize == 0)
		//		{
		//			Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::FORMAT_RGBA8);
		//			image = im;
		//			image->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));
		//		}
		//		else
		//		{
		//			godot::PackedByteArray data;
		//			size_t splatmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGBA);
		//			data.resize((int)splatmapBufferSizeInBytes);
		//			assert(splatmapBufferSizeInBytes == splatmapBufferSize);
		//			{
		//				memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getSplatmapBuffer().ptr(), splatmapBufferSizeInBytes);
		//			}
		//			Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGBA8, data);
		//			image = im;
		//			//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".splatmap.png";
		//			//image->save_png(outFileName.c_str());
		//		}
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, tex);
		//	}

		//	godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
		//	godot::Variant inverseTransform = t.affine_inverse();
		//	material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
		//	Basis normalBasis = t.basis.inverse().transposed();
		//	material->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);
		//	material->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		//	material->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, true);
		//	//material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		//	//material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
		//	//material->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());

		//	terrainPlaneMaterial = material;
		//}
		//else if (shaderType == 2)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.7 ") + __FUNCTION__, "globalmap Material stuff");

			TheWorld_Utils::TerrainEdit* terrainEdit = m_quadTree->getQuadrant()->getTerrainEdit();

			godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getRegularTerrainMaterial();
			//godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getLookDevMaterial(ShaderTerrainData::LookDev::Splat);		// SHADER_PARAM_LOOKDEV_MAP
			godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
			material->set_shader(currentMaterial->get_shader());

			//{
			//	godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
			//	//godot::TypedArray<godot:Dictionary> params = 
			//	godot::RID rid = currentMaterial->get_shader()->get_rid();
			//	godot::TypedArray<godot::Dictionary> params = rs->get_shader_parameter_list(rid);
			//	int64_t size = params.size();
			//	for (int i = 0; i < size; i++)
			//	{
			//		//std::string paramName = String(params[i].get("name")).utf8().get_data();
			//		godot::PackedByteArray array = String(params[i].get("name")).to_ascii_buffer();
			//		std::string paramName((char*)array.ptr(), array.size());
			//		godot::Variant v = currentMaterial->get_shader_parameter(paramName.c_str());
			//		material->set_shader_parameter(paramName.c_str(), v);
			//	}

			//	godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
			//	godot::Variant inverseTransform = t.affine_inverse();
			//	material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
			//	material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
			//	material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
			//}

			//Ref<godot::ImageTexture> heightmapTexture;

			material->set_shader_parameter(SHADER_PARAM_GENERATE_GLOBAL_MAP, true);
			material->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
			material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
			material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
			material->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
			material->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
			godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
			godot::Variant inverseTransform = t.affine_inverse();
			material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
			Basis normalBasis = t.basis.inverse().transposed();
			material->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);

			std::string lowElevationTexName = terrainEdit->extraValues.lowElevationTexName_r;
			if (lowElevationTexName.size() > 0)
			{
				godot::Ref<godot::ImageTexture> lowElevation_albedo_bump_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getAlbedoBumpTexture(lowElevationTexName);
				if (lowElevation_albedo_bump_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_0, lowElevation_albedo_bump_tex);

				godot::Ref<godot::ImageTexture> lowElevation_normal_roughness_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getNormalRoughnessTexture(lowElevationTexName);
				if (lowElevation_normal_roughness_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0, lowElevation_normal_roughness_tex);
			}
			std::string highElevationTexName = terrainEdit->extraValues.highElevationTexName_g;
			if (highElevationTexName.size() > 0)
			{
				godot::Ref<godot::ImageTexture> highElevation_albedo_bump_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getAlbedoBumpTexture(highElevationTexName);
				if (highElevation_albedo_bump_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_1, highElevation_albedo_bump_tex);

				godot::Ref<godot::ImageTexture> highElevation_normal_roughness_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getNormalRoughnessTexture(highElevationTexName);
				if (highElevation_normal_roughness_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1, highElevation_normal_roughness_tex);
			}
			std::string dirtTexName = terrainEdit->extraValues.dirtTexName_b;
			if (dirtTexName.size() > 0)
			{
				godot::Ref<godot::ImageTexture> dirt_albedo_bump_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getAlbedoBumpTexture(dirtTexName);
				if (dirt_albedo_bump_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_2, dirt_albedo_bump_tex);

				godot::Ref<godot::ImageTexture> dirt_normal_roughness_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getNormalRoughnessTexture(dirtTexName);
				if (dirt_normal_roughness_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2, dirt_normal_roughness_tex);
			}
			std::string rocksTexName = terrainEdit->extraValues.rocksTexName_a;
			if (rocksTexName.size() > 0)
			{
				godot::Ref<godot::ImageTexture> rocks_albedo_bump_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getAlbedoBumpTexture(rocksTexName);
				if (rocks_albedo_bump_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_3, rocks_albedo_bump_tex);

				godot::Ref<godot::ImageTexture> rocks_normal_roughness_tex = m_quadTree->getQuadrant()->getShaderTerrainData()->getNormalRoughnessTexture(rocksTexName);
				if (rocks_normal_roughness_tex != nullptr)
					material->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3, rocks_normal_roughness_tex);
			}

			int numVerticesPerQuadrantSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();

			{
				size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
				godot::PackedByteArray data;
				size_t heightmapBufferSizeInBytes = (size_t)numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * uint16_t_size;
				data.resize((int)heightmapBufferSizeInBytes);
				size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
				assert(heightmapBufferSizeInBytes == heightsBufferSize);
				{
					memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
				}
				godot::Ref<godot::Image> image = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RH, data);
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				heightmapTexture = godot::ImageTexture::create_from_image(image);
				material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);
			}

			{
				godot::Ref<godot::Image> image;
				size_t normalsBufferSize = 0;
				normalsBufferSize = m_quadTree->getQuadrant()->getNormalsBuffer().size();
				if (normalsBufferSize == 0)
				{
					godot::Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::FORMAT_RGB8);
					image = im;
					image->fill(godot::Color(0, 0, 0, 1.0));
				}
				else
				{
					godot::PackedByteArray data;
					size_t normalmapBufferSizeInBytes = (size_t)numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGB);
					data.resize((int)normalmapBufferSizeInBytes);
					assert(normalmapBufferSizeInBytes == normalsBufferSize);
					{
						memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getNormalsBuffer().ptr(), normalsBufferSize);
					}
					godot::Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGB8, data);
					image = im;
				}
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				material->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, tex);
			}

			{
				godot::Ref<godot::Image> image;

				size_t splatmapBufferSize = 0;
				splatmapBufferSize = m_quadTree->getQuadrant()->getSplatmapBuffer().size();
				if (splatmapBufferSize == 0)
				{
					Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::FORMAT_RGBA8);
					image = im;
					image->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));
				}
				else
				{
					godot::PackedByteArray data;
					size_t splatmapBufferSizeInBytes = (size_t)numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGBA);
					data.resize((int)splatmapBufferSizeInBytes);
					assert(splatmapBufferSizeInBytes == splatmapBufferSize);
					{
						memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getSplatmapBuffer().ptr(), splatmapBufferSizeInBytes);
					}
					Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGBA8, data);
					image = im;
					//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".splatmap.png";
					//image->save_png(outFileName.c_str());
				}
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				material->set_shader_parameter(SHADER_PARAM_TERRAIN_SPLATMAP, tex);
				//material->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, tex);
			}

			{
				Ref<Image> image;
				size_t colormapBufferSize = 0;
				colormapBufferSize = m_quadTree->getQuadrant()->getColormapBuffer().size();
				if (colormapBufferSize == 0)
				{
					godot::Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGBA8);
					image = im;
					image->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
				}
				else
				{
					godot::PackedByteArray data;
					size_t colormapBufferSizeInBytes = size_t(numVerticesPerQuadrantSize) * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGBA);
					data.resize((int)colormapBufferSizeInBytes);
					assert(colormapBufferSizeInBytes == colormapBufferSize);
					{
						memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getColormapBuffer().ptr(), colormapBufferSizeInBytes);
					}
					godot::Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, Image::FORMAT_RGBA8, data);
					image = im;
				}
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				material->set_shader_parameter(SHADER_PARAM_TERRAIN_COLORMAP, tex);
			}

			material->set_shader_parameter(SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE, godot::Plane(6.0f, 6.0f, 6.0f, 6.0f));
			material->set_shader_parameter(SHADER_PARAM_DEPTH_BLENDING, true);
			material->set_shader_parameter(SHADER_PARAM_TRIPLANAR, false);
			material->set_shader_parameter(SHADER_PARAM_TILE_REDUCTION_PER_TEXTURE, godot::Plane(0.0f, 0.0f, 0.0f, 0.0f));	// disabled for every texture
			material->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_START, 0);
			material->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_DISTANCE, 0);	// Disable use of globalmap on far pixels
			material->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, false);			// Disable forced use of globalmap
			material->set_shader_parameter(SHADER_PARAM_COLORMAP_OPACITY_PER_TEXTURE, godot::Plane(1.0f, 1.0f, 1.0f, 1.0f));	// Notset
			material->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, true);
			material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);

			terrainPlaneMaterial = material;
		}

		m_terrainPlane->set_material_override(terrainPlaneMaterial);

		if (m_quadTree->canContainSea() == QUADRANT_SEA_INFO_SET_TRUE)
		{
			m_seaPlane = memnew(godot::MeshInstance3D);
			m_mapModderViewport->add_child(m_seaPlane);

			{
				const int32_t numVerticesPerTile = (int32_t)m_tileSize;
				const size_t numVerticesPerQuadrant = m_viewer->Globals()->heightmapResolution();
				//godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(numVerticesPerTile, 0, float(m_tileSize / numVerticesPerTile) * gridStepInWU);
				godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(numVerticesPerTile, 0, float(numVerticesPerQuadrant / numVerticesPerTile));
				//const int32_t planeResolution = 16;	// WARNING: reducing number of vertices will result very imprecise because, for optimizing normal viewport, splatmap is fetched in vertex shader rather than
														// in fragment shader (less time) but for this reason splatmap values will result from an interpolation of real splatmap values being very imprecise
														// calculating globalmap we need more precise calculation of splatmap values so, to use same shader, we have to increment the number of vertices to reduce interpolation effects
				//godot::Ref<godot::Mesh> mesh = MeshCache::makeNewFlatChunk(planeResolution, 0, float(m_imageSize / planeResolution) * gridStepInWU);
				m_displacementPlane = Vector3(0.0f, 0.0f, 0.0f) * gridStepInWU;
				m_seaPlane->set_mesh(mesh);
			}

			godot::Ref<godot::Material> seaPlaneMaterial;

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.7 ") + __FUNCTION__, "globalmap Material stuff");

				int numVerticesPerQuadrantSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();

				godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getGlobalmapSeaMaterial();
				godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
				material->set_shader(currentMaterial->get_shader()); 
				material->set_shader_parameter(SHADER_PARAM_GENERATE_GLOBAL_MAP, true);
				material->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
				material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
				material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
				material->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
				material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)numVerticesPerQuadrantSize + 1.0f);	// useless as it is used in show chunks mode which is inhibited when generating global map
				material->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
				material->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
				godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
				godot::Variant inverseTransform = t.affine_inverse();
				material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
				material->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, true);
				material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);

				seaPlaneMaterial = material;
			}

			m_seaPlane->set_material_override(seaPlaneMaterial);
		}
	}

	if (m_debugSprite != nullptr)
	{
		m_debugSprite->set_texture(m_mapModderViewport->get_texture());
		m_debugSprite->set_position(m_debugSprite->get_texture()->get_size() / 2);
	}

	set_process(true);
	m_numProcessExecutions = 0;
	startProcessRequired(false);
}

void GDN_TheWorld_MapModder::deinit(void)
{
	if (m_initialized != MapType::uninitialized)
	{
		godot::RenderingServer* vs = godot::RenderingServer::get_singleton();
		if (vs->is_connected("frame_post_draw", Callable(this, "frame_post_draw")))
			vs->disconnect("frame_post_draw", Callable(this, "frame_post_draw"));

		set_process(false);

		m_outImageValid = false;
		m_outImage.unref();		// TO VERIFY

		// clear
		m_tiles.clear();
		std::queue<godot::Vector2i> empty;
		std::swap(m_tilesQueue, empty);
		m_processingTile = false;

		if (m_debugSprite != nullptr)
			m_debugSprite->set_texture(nullptr);

		if (m_mapModderViewport != nullptr)
		{
			Node* parent = m_mapModderViewport->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_mapModderViewport);
			m_mapModderViewport->queue_free();
			m_mapModderViewport = nullptr;
		}
		m_map = nullptr;
		m_terrainPlane = nullptr;
		m_seaPlane = nullptr;
		m_camera = nullptr;

		godot::Node* parent = get_parent();
		if (parent != nullptr)
			parent->remove_child(this);
		
		deinitRequired(false);
		startProcessRequired(false);

		GDN_TheWorld_MapModder::decNumWorking();

		m_initialized = MapType::uninitialized;
	}
}

void GDN_TheWorld_MapModder::startProcessRequired(bool b)
{
	m_startProcessingRequired = b;
}

bool GDN_TheWorld_MapModder::startProcessRequired(void) const
{
	return m_startProcessingRequired;
}

void GDN_TheWorld_MapModder::deinitRequired(bool b)
{
	m_deinitRequired = b;
}

bool GDN_TheWorld_MapModder::deinitRequired(void) const
{
	return m_deinitRequired;
}

void GDN_TheWorld_MapModder::_notification(int p_what)
{
}

void GDN_TheWorld_MapModder::_ready(void)
{
}

void GDN_TheWorld_MapModder::custom_ready(GDN_TheWorld_Viewer* viewer, Node* parent, bool debug)
{
	m_viewer = viewer;
	m_parent = parent;

	if (m_debugSprite == nullptr && debug)
	{
		m_debugSprite = memnew(godot::Sprite2D);
		//m_viewer->add_child(m_debugSprite);
		m_viewer->call_deferred("add_child", m_debugSprite);
	}

	//{
	//	const int32_t c_viewportSize = 512;

	//	godot::SubViewport*  viewport = memnew(godot::SubViewport);
	//	m_viewer->add_child(viewport);
	//	viewport->set_clear_mode(godot::SubViewport::ClearMode::CLEAR_MODE_ALWAYS);
	//	godot::Ref<godot::World3D> w = memnew(godot::World3D);
	//	viewport->set_world_3d(w);
	//	viewport->set_use_own_world_3d(true);

	//	viewport->set_size(godot::Vector2i(int32_t(c_viewportSize + 1), int32_t(c_viewportSize + 1)));
	//	viewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_ALWAYS);
	//	viewport->set_debug_draw(godot::Viewport::DebugDraw::DEBUG_DRAW_UNSHADED);

	//	godot::Ref<godot::StandardMaterial3D> stdMat = memnew(godot::StandardMaterial3D);
	//	stdMat->set_albedo(godot::Color(0.0f, 1.0f, 0.0f));

	//	godot::Ref<godot::PlaneMesh> mesh = memnew(godot::PlaneMesh);
	//	mesh->set_size(godot::Vector2((float)c_viewportSize, (float)c_viewportSize));
	//	mesh->set_subdivide_depth(3);
	//	mesh->set_subdivide_width(3);

	//	godot::MeshInstance3D* plane = memnew(godot::MeshInstance3D);
	//	plane->set_material_override(stdMat);
	//	plane->set_mesh(mesh);
	//	viewport->add_child(plane);

	//	godot::Camera3D* camera = memnew(godot::Camera3D);
	//	camera->set_projection(godot::Camera3D::ProjectionType::PROJECTION_ORTHOGONAL);
	//	godot::Vector2i viewportSize = viewport->get_size();
	//	camera->set_size(viewportSize.x);
	//	camera->set_near(0.1f);
	//	camera->set_far(10.0f);
	//	camera->set_current(true);
	//	camera->set_rotation(godot::Vector3(-TheWorld_Utils::kPi/2, 0, 0));
	//	viewport->add_child(camera);

	//	camera->set_position(godot::Vector3(0.0f, 2.0f, 0.0f));

	//	godot::Sprite2D* sprite = memnew(godot::Sprite2D);
	//	m_viewer->add_child(sprite);
	//	sprite->set_position(godot::Vector2(256 + 512 * 2, 256));
	//	sprite->set_texture(viewport->get_texture());
	//}
}

void GDN_TheWorld_MapModder::debug_process(double _delta)
{
	{
		if (m_debugSprite != nullptr)
		{
			if (m_viewer->editMode())
				m_debugSprite->set_visible(true);
			else
				m_debugSprite->set_visible(false);
		}
	}
}

void GDN_TheWorld_MapModder::_process(double _delta)
{
	if (!is_processing())
		return;

	m_numProcessExecutions++;

	if (m_processingTile && m_frameAvailable)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("MapModder _process 1 ") + __FUNCTION__, "Processing tile");

		// DEBUG
		//godot::Ref<godot::ViewportTexture> viewportTexture = m_viewport->get_texture();
		//bool b = viewportTexture->has_alpha();
		//godot::Ref<godot::Image> viewportImageTexture = viewportTexture->get_image();
		//godot::Image::Format viewportImageTextureFormat = viewportImageTexture->get_format();
		//godot::Vector2i size = viewportImageTexture->get_size();
		// DEBUG

		godot::Ref<godot::Image> viewportImage = m_mapModderViewport->get_texture()->get_image();
		godot::Image::Format viewportImageFormat = viewportImage->get_format();

		//if (m_initialized == MapType::initializedForGlobalmap)
		//{
		//	godot::Vector2i pos = m_currentTile * (int32_t)m_tileSize; 
		//	std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".splatmap.process_" + std::to_string(m_numProcessExecutions) + "_" + std::to_string(pos.x) + "_" + std::to_string(pos.y) + ".png";
		//	viewportImage->save_png(outFileName.c_str());
		//}

		// DEBUG
		//{
		//	int32_t x = (int32_t)m_viewportBorder, y = (int32_t)m_viewportBorder;
		//	Color c = viewportImage->get_pixel(x, y);
		//	size_t numVerticesPerSize = m_viewer->Globals()->numVerticesPerQuadrantSize();
		//	float height = m_float32HeigthsBuffer->at<float>(x, y, numVerticesPerSize);
		//}
		// DEBUG

		if (m_outImage == nullptr || !m_outImageValid)
		{
			//godot::Ref<godot::Image> im = godot::Image::create((int32_t)m_imageSize, (int32_t)m_imageSize, false, viewportImageFormat);
			//m_outImage = im;
			m_outImage = godot::Image::create((int32_t)m_imageSize, (int32_t)m_imageSize, false, viewportImageFormat);
			m_outImageValid = true;
		}

		godot::Vector2i pos = m_currentTile * (int32_t)m_tileSize;
		int32_t w = viewportImage->get_width();
		int32_t h = viewportImage->get_height();
		// pixels on border (border size = m_viewportBorder) are colculated but are ignored as they exceed the tile
		m_outImage->blit_rect(viewportImage, godot::Rect2i((int32_t)m_viewportBorder, (int32_t)m_viewportBorder, w - (int32_t)m_viewportBorder, h - (int32_t)m_viewportBorder), pos);

		if (m_tiles.contains(m_currentTile) && m_tiles[m_currentTile] == c_tileStatusProcessing)
			m_tiles.erase(m_currentTile);

		m_processingTile = false;
		m_frameAvailable = false;
	}

	if (!m_processingTile)
	{
		if (workToDo())
		{
			godot::Vector2i tile = m_tilesQueue.front();
			m_tilesQueue.pop();
			if (m_initialized == MapType::initializedForGlobalmap)
			{
				float gridStepInWU = m_viewer->Globals()->gridStepInWU();
				godot::Vector2 origin = tile * (int32_t)m_tileSize;
				godot::Vector2 center = origin + (0.5f * godot::Vector2((float)m_mapModderViewport->get_size().x, (float)m_mapModderViewport->get_size().y));
				origin *= gridStepInWU;
				center *= gridStepInWU;
				m_camera->set_position(godot::Vector3(center.x, m_cameraAltitude, center.y));
				m_terrainPlane->set_position(godot::Vector3(origin.x, 0.0f, origin.y) + m_displacementPlane);
				if (m_seaPlane != nullptr)
					m_seaPlane->set_position(godot::Vector3(origin.x, 0.0f, origin.y) + m_displacementPlane);
			}
			else
			{
				godot::Vector2 pos = (godot::Vector2((float)tile.x, (float)tile.y) * (float)m_tileSize * -1) + godot::Vector2((float)m_viewportBorder, (float)m_viewportBorder);
				m_map->set_position(pos);
				m_mapModderViewport->set_update_mode(godot::SubViewport::UpdateMode::UPDATE_ONCE);
			}
			m_currentTile = tile;
			m_tiles[tile] = c_tileStatusProcessing;
			m_processingTile = true;
			m_frameAvailable = false;
		}
		else
		{
			set_process(false);
			if (m_signalRequired)
				call_deferred("emit_signal", "tw_modder_work_done", (int32_t)m_initialized, m_quadTree->getQuadrant()->getPos().getLowerXGridVertex(), m_quadTree->getQuadrant()->getPos().getLowerZGridVertex(), m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize(), m_quadTree->getQuadrant()->getPos().getLevel(), m_quadTree->getQuadrant()->getPos().getGridStepInWU());
		}
	}
}

void GDN_TheWorld_MapModder::_physics_process(double _delta)
{
}

void GDN_TheWorld_MapModder::_input(const Ref<InputEvent>& event)
{
}

void GDN_TheWorld_Viewer::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("get_map_viewer"), &GDN_TheWorld_Viewer::getMapViewer);
	ClassDB::bind_method(D_METHOD("map_mode"), &GDN_TheWorld_Viewer::_mapMode);
	ClassDB::bind_method(D_METHOD("world_mode"), &GDN_TheWorld_Viewer::_worldMode);
	//ClassDB::bind_method(D_METHOD("get_map_scale_factor"), &GDN_TheWorld_Viewer::getMapScaleFactor);

	ClassDB::bind_method(D_METHOD("debug_print", "p_msg"), &GDN_TheWorld_Viewer::debugPrint);
	ClassDB::bind_method(D_METHOD("error_print", "p_msg"), &GDN_TheWorld_Viewer::errorPrint);
	ClassDB::bind_method(D_METHOD("warning_print", "p_msg"), &GDN_TheWorld_Viewer::warningPrint);
	ClassDB::bind_method(D_METHOD("info_print", "p_msg"), &GDN_TheWorld_Viewer::infoPrint);
	ClassDB::bind_method(D_METHOD("print", "p_msg"), &GDN_TheWorld_Viewer::print);

	ClassDB::bind_method(D_METHOD("set_anchor_for_origin_shift", "p_anchor"), &GDN_TheWorld_Viewer::setAnchorForOriginShift);
	ClassDB::bind_method(D_METHOD("set_editor_interface"), &GDN_TheWorld_Viewer::setEditorInterface);
	ClassDB::bind_method(D_METHOD("set_editor_3d_overlay"), &GDN_TheWorld_Viewer::setEditor3dOverlay);
	ClassDB::bind_method(D_METHOD("set_world_editor_camera"), &GDN_TheWorld_Viewer::setWorldEditorCamera);
	ClassDB::bind_method(D_METHOD("get_world_camera"), &GDN_TheWorld_Viewer::getWorldCamera);
	ClassDB::bind_method(D_METHOD("get_map_camera"), &GDN_TheWorld_Viewer::getMapCamera);
	ClassDB::bind_method(D_METHOD("get_info_camera"), &GDN_TheWorld_Viewer::getInfoCamera);
	ClassDB::bind_method(D_METHOD("get_camera_projection_mode"), &GDN_TheWorld_Viewer::getCameraProjectionMode);
	ClassDB::bind_method(D_METHOD("get_or_create_edit_mode_ui_control"), &GDN_TheWorld_Viewer::getOrCreateEditModeUIControl);
	ClassDB::bind_method(D_METHOD("toggle_track_mouse"), &GDN_TheWorld_Viewer::toggleTrackMouse);
	ClassDB::bind_method(D_METHOD("get_track_mouse_state"), &GDN_TheWorld_Viewer::getTrackMouseState);
	ClassDB::bind_method(D_METHOD("toggle_edit_mode"), &GDN_TheWorld_Viewer::toggleEditMode);
	ClassDB::bind_method(D_METHOD("set_edit_mode"), &GDN_TheWorld_Viewer::setEditMode);
	ClassDB::bind_method(D_METHOD("is_edit_mode"), &GDN_TheWorld_Viewer::isEditMode);
	ClassDB::bind_method(D_METHOD("toggle_debug_visibility"), &GDN_TheWorld_Viewer::toggleDebugVisibility);
	ClassDB::bind_method(D_METHOD("get_debug_visibility"), &GDN_TheWorld_Viewer::getDebugContentVisibility);
	ClassDB::bind_method(D_METHOD("rotate_chunk_debug_mode"), &GDN_TheWorld_Viewer::rotateChunkDebugMode);
	ClassDB::bind_method(D_METHOD("rotate_drawing_mode"), &GDN_TheWorld_Viewer::rotateDrawingMode);
	ClassDB::bind_method(D_METHOD("toggle_quadrant_selected"), &GDN_TheWorld_Viewer::toggleQuadrantSelected);
	ClassDB::bind_method(D_METHOD("set_depth_quad"), &GDN_TheWorld_Viewer::setMinDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("get_depth_quad"), &GDN_TheWorld_Viewer::getMinDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("set_cache_quad"), &GDN_TheWorld_Viewer::setCacheQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("get_cache_quad"), &GDN_TheWorld_Viewer::getCacheQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("set_max_depth_quad"), &GDN_TheWorld_Viewer::setMaxDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("get_max_depth_quad"), &GDN_TheWorld_Viewer::getMaxDepthQuadOnPerimeter);
	ClassDB::bind_method(D_METHOD("set_force_shader_elevation_0"), &GDN_TheWorld_Viewer::setForceShaderElevation0);
	ClassDB::bind_method(D_METHOD("get_force_shader_elevation_0"), &GDN_TheWorld_Viewer::getForceShaderElevation0);
	ClassDB::bind_method(D_METHOD("deploy_world"), &GDN_TheWorld_Viewer::deployWorld);
	ClassDB::bind_method(D_METHOD("undeploy_world"), &GDN_TheWorld_Viewer::undeployWorld);
	ClassDB::bind_method(D_METHOD("initial_world_viewer_pos_set"), &GDN_TheWorld_Viewer::initialWorldViewerPosSet);
	ClassDB::bind_method(D_METHOD("dump_required"), &GDN_TheWorld_Viewer::setDumpRequired);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_aabb"), &GDN_TheWorld_Viewer::getCameraTerrainChunkLocalAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_debug_aabb"), &GDN_TheWorld_Viewer::getCameraTerrainChunkLocalDebugAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_global_transform_applied"), &GDN_TheWorld_Viewer::getCameraTerrainChunkGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_debug_global_transform_applied"), &GDN_TheWorld_Viewer::getCameraTerrainChunkDebugGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_id"), &GDN_TheWorld_Viewer::getCameraTerrainChunkId);
	ClassDB::bind_method(D_METHOD("get_camera_quadrant_name"), &GDN_TheWorld_Viewer::getCameraTerrainQuadrantName);
	ClassDB::bind_method(D_METHOD("get_num_splits"), &GDN_TheWorld_Viewer::getNumSplits);
	ClassDB::bind_method(D_METHOD("get_num_joins"), &GDN_TheWorld_Viewer::getNumJoins);
	ClassDB::bind_method(D_METHOD("get_num_chunks"), &GDN_TheWorld_Viewer::getNumChunks);
	ClassDB::bind_method(D_METHOD("get_num_active_chunks"), &GDN_TheWorld_Viewer::getNumActiveChunks);
	ClassDB::bind_method(D_METHOD("get_num_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_initialized_quadrant"), &GDN_TheWorld_Viewer::getNumInitializedQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_toerase_quadrant"), &GDN_TheWorld_Viewer::getNumToEraseQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_visible_quadrant"), &GDN_TheWorld_Viewer::getNumVisibleQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_initialized_visible_quadrant"), &GDN_TheWorld_Viewer::getNumInitializedVisibleQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_need_shader_reset_quadrant"), &GDN_TheWorld_Viewer::getNumNeedShaderResetQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_empty_quadrant"), &GDN_TheWorld_Viewer::getNumEmptyQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_reloading_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrantReloadingFromCache);
	ClassDB::bind_method(D_METHOD("get_num_flushed_quadrant"), &GDN_TheWorld_Viewer::getNumFlushedQuadrant);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_enabled_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithEnabledCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_hires_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithHiResCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_lowres_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithLowResCollider);
	ClassDB::bind_method(D_METHOD("get_num_process_not_owns_lock"), &GDN_TheWorld_Viewer::getProcessNotOwnsLock);
	ClassDB::bind_method(D_METHOD("get_process_duration"), &GDN_TheWorld_Viewer::getProcessDuration);
	ClassDB::bind_method(D_METHOD("get_map_quadrants_num"), &GDN_TheWorld_Viewer::getMapQuadrantsNum);
	ClassDB::bind_method(D_METHOD("get_updating_map_quadrants_num"), &GDN_TheWorld_Viewer::getUpdatingMapQuadrantsNum);
	ClassDB::bind_method(D_METHOD("get_to_show_map_quadrants_num"), &GDN_TheWorld_Viewer::getToShowMapQuadrantsNum);
	ClassDB::bind_method(D_METHOD("get_map_quadrants_rect"), &GDN_TheWorld_Viewer::getMapQuadrantRects);
	//ClassDB::bind_method(D_METHOD("get_refresh_quads_duration"), &GDN_TheWorld_Viewer::getRefreshMapQuadDuration);
	ClassDB::bind_method(D_METHOD("get_update_quads1_duration"), &GDN_TheWorld_Viewer::getUpdateQuads1Duration);
	ClassDB::bind_method(D_METHOD("get_update_quads2_duration"), &GDN_TheWorld_Viewer::getUpdateQuads2Duration);
	ClassDB::bind_method(D_METHOD("get_update_quads3_duration"), &GDN_TheWorld_Viewer::getUpdateQuads3Duration);
	ClassDB::bind_method(D_METHOD("get_update_chunks_duration"), &GDN_TheWorld_Viewer::getUpdateChunksDuration);
	ClassDB::bind_method(D_METHOD("get_update_material_params_duration"), &GDN_TheWorld_Viewer::getUpdateMaterialParamsDuration);
	ClassDB::bind_method(D_METHOD("get_mouse_track_hit_duration"), &GDN_TheWorld_Viewer::getMouseTrackHitDuration);
	ClassDB::bind_method(D_METHOD("get_debug_draw_mode"), &GDN_TheWorld_Viewer::getDebugDrawMode);
	ClassDB::bind_method(D_METHOD("get_chunk_debug_mode"), &GDN_TheWorld_Viewer::getChunkDebugModeStr);
	ClassDB::bind_method(D_METHOD("get_mouse_hit_global"), &GDN_TheWorld_Viewer::getMouseHitGlobal);
	ClassDB::bind_method(D_METHOD("get_mouse_hit_in_map"), &GDN_TheWorld_Viewer::getMouseHitInMap);
	ClassDB::bind_method(D_METHOD("get_mouse_hit_distance_from_camera"), &GDN_TheWorld_Viewer::getMouseHitDistanceFromCamera);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_name"), &GDN_TheWorld_Viewer::_getMouseQuadrantHitName);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_tag"), &GDN_TheWorld_Viewer::_getMouseQuadrantHitTag);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_pos"), &GDN_TheWorld_Viewer::getMouseQuadrantHitPos);
	ClassDB::bind_method(D_METHOD("get_mouse_quadrant_hit_size"), &GDN_TheWorld_Viewer::getMouseQuadrantHitSize);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_name"), &GDN_TheWorld_Viewer::getMouseChunkHitId);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_pos"), &GDN_TheWorld_Viewer::getMouseChunkHitPos);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_size"), &GDN_TheWorld_Viewer::getMouseChunkHitSize);
	ClassDB::bind_method(D_METHOD("get_mouse_chunk_hit_dist_from_cam"), &GDN_TheWorld_Viewer::getMouseChunkHitDistFromCam);
	ClassDB::bind_method(D_METHOD("update_material_parmas_for_every_quadrant"), &GDN_TheWorld_Viewer::updateMaterialParamsForEveryQuadrant);
	ClassDB::bind_method(D_METHOD("set_shader_parameter"), &GDN_TheWorld_Viewer::setShaderParam);
	ClassDB::bind_method(D_METHOD("get_viewport_size"), &GDN_TheWorld_Viewer::getViewportSize);
	ClassDB::bind_method(D_METHOD("viewport_size_changed"), &GDN_TheWorld_Viewer::viewportSizeChanged);
	ClassDB::bind_method(D_METHOD("quadrant_modder_work_done", "map_type", "x", "z", "num_vertices_per_side", "level", "grid_step"), &GDN_TheWorld_Viewer::quadrantModderWorkDone);
	ClassDB::bind_method(D_METHOD("load_quadrant_data", "pos_x", "pos_z", "level"), &GDN_TheWorld_Viewer::loadQuadrantData);
	ClassDB::bind_method(D_METHOD("load_quadrant_data_index", "index_pos_x", "index_pos_z", "level"), &GDN_TheWorld_Viewer::loadQuadrantDataIndex);
	ClassDB::bind_method(D_METHOD("clear_quadrant_data"), &GDN_TheWorld_Viewer::clearQuadrantData);
	ClassDB::bind_method(D_METHOD("set_hud_message", "message", "fading_time"), &GDN_TheWorld_Viewer::setHudMessage);
	ClassDB::bind_method(D_METHOD("get_quadrant_pos"), &GDN_TheWorld_Viewer::getQuadrantPos);
	ClassDB::bind_method(D_METHOD("get_quadrant_pos_index"), &GDN_TheWorld_Viewer::getQuadrantPosIndex);
	ClassDB::bind_method(D_METHOD("get_quadrant_heightmap_image"), &GDN_TheWorld_Viewer::getQuadrantHeightmapImage);
	ClassDB::bind_method(D_METHOD("get_quadrant_normalmap_image"), &GDN_TheWorld_Viewer::getQuadrantNormalmapImage);
	ClassDB::bind_method(D_METHOD("get_quadrant_splatmap_image"), &GDN_TheWorld_Viewer::getQuadrantSplatmapImage);
	ClassDB::bind_method(D_METHOD("get_quadrant_globalmap_image"), &GDN_TheWorld_Viewer::getQuadrantGlobalmapImage);
	ClassDB::bind_method(D_METHOD("get_quadrant_colormap_image"), &GDN_TheWorld_Viewer::getQuadrantColormapImage);
	ClassDB::bind_method(D_METHOD("get_shader_param_ground_uv_scale"), &GDN_TheWorld_Viewer::getShaderParamGroundUVScale);
	ClassDB::bind_method(D_METHOD("get_shader_param_depth_blending"), &GDN_TheWorld_Viewer::getShaderParamDepthBlending);
	ClassDB::bind_method(D_METHOD("get_shader_param_triplanar"), &GDN_TheWorld_Viewer::getShaderParamTriplanar);
	ClassDB::bind_method(D_METHOD("get_shader_param_tile_reduction"), &GDN_TheWorld_Viewer::getShaderParamTileReduction);
	ClassDB::bind_method(D_METHOD("get_ground_texture_name"), &GDN_TheWorld_Viewer::getGroundTextureName);
	ClassDB::bind_method(D_METHOD("get_ground_texture_albedo_bump"), &GDN_TheWorld_Viewer::getGroundTextureAlbedoBump);
	ClassDB::bind_method(D_METHOD("get_ground_texture_normal_roughness"), &GDN_TheWorld_Viewer::getGroundTextureNormalRoughness);
	ClassDB::bind_method(D_METHOD("get_ground_texture_layers"), &GDN_TheWorld_Viewer::getGroundTextureLayers);
	ClassDB::bind_method(D_METHOD("pause_mouse_movement"), &GDN_TheWorld_Viewer::pauseMouseMovement);
	ClassDB::bind_method(D_METHOD("resume_mouse_movement"), &GDN_TheWorld_Viewer::resumeMouseMovement);

	ClassDB::bind_method(D_METHOD("global_transform_to_world_transform", "p_transform"), &GDN_TheWorld_Viewer::globalToWorld);
	ClassDB::bind_method(D_METHOD("global_pos_to_world_transform", "p_transform"), &GDN_TheWorld_Viewer::globalPosToWorldTransform);
	ClassDB::bind_method(D_METHOD("global_pos_to_world_pos", "p_transform"), &GDN_TheWorld_Viewer::globalPosToWorldPos);
	ClassDB::bind_method(D_METHOD("world_transform_to_global_transform", "p_transform"), &GDN_TheWorld_Viewer::worldToGlobal);
	ClassDB::bind_method(D_METHOD("world_pos_to_global_transform", "p_transform"), &GDN_TheWorld_Viewer::worldPosToGlobalTransform);
	ClassDB::bind_method(D_METHOD("world_pos_to_global_pos", "p_transform"), &GDN_TheWorld_Viewer::worldPosToGlobalPos);

	ADD_SIGNAL(MethodInfo("tw_quadrant_selected_for_edit", PropertyInfo(Variant::BOOL, "value")));
}

GDN_TheWorld_Viewer::GDN_TheWorld_Viewer()
{
	m_initialized = false;
	m_quitting = false;
	m_visibleInTree = false;
	m_initRequired = false;
	m_isInEditor = false;
	m_isInEditorInitialized = false;
	m_ready = false;
	m_desiderdLookDev = ShaderTerrainData::LookDev::NotSet;
	m_desideredLookDevChanged = false;
	m_stopDeploy = false;
	m_useVisualServer = true;
	m_firstProcess = true;
	m_initialWorldViewerPosSet = false;
	m_dumpRequired = false;
	lastTrackedChunk = nullptr;
	m_refreshRequired = false;
	m_worldViewerLevel = 0;
	m_customWorldCamera = nullptr;
	//m_cameraQuadTree = nullptr;
	m_globals = nullptr;
	m_editModeUIControl = nullptr;
	m_timeElapsedFromLastDump = 0;
	m_timeElapsedFromLastStatistic = 0;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numActiveChunks = 0;
	m_numProcessNotOwnsLock = 0;
	m_numQuadrant = 0;
	m_numInitializedQuadrant = 0;
	m_numVisibleQuadrant = 0;
	m_numEmptyQuadrant = 0;
	m_numFlushedQuadrant = 0;
	m_numInitializedVisibleQuadrant = 0;
	m_debugContentVisibility = false;
	m_mouseQuadrantHitSize = 0;
	//m_mouseHitQuadTree = nullptr;
	m_updateTerrainVisibilityRequired = false;
	m_currentChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
	//m_currentChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
	//m_requiredChunkDebugMode = GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
	m_updateDebugModeRequired = false;
	m_numWorldVerticesPerSize = 0;
	m_debugDraw = Viewport::DebugDraw::DEBUG_DRAW_DISABLED;
	m_altPressed = false;
	m_ctrlPressed = false;
	m_shiftPressed = false;
	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = false;
	m_editorInterface = nullptr;
	m_editor3dOverlay = nullptr;
	m_editorCamera = nullptr;
	m_shaderParam_depthBlending = false;
	m_shaderParam_triplanar = false;
	m_shaderParam_globalmapBlendStart = 0.0f;
	m_shaderParam_globalmapBlendDistance = 0.0f;

	_init();
}

GDN_TheWorld_Viewer::~GDN_TheWorld_Viewer()
{
	deinit();
}

void GDN_TheWorld_Viewer::setAnchorForOriginShift(godot::Node3D* anchorForOriginShift)
{
	m_anchorForOriginShift = anchorForOriginShift;
}

godot::Node3D* GDN_TheWorld_Viewer::getAnchorForOriginShift(void)
{
	return m_anchorForOriginShift;
}

void GDN_TheWorld_Viewer::quadrantModderWorkDone(int32_t map_type, float x, float z, int32_t num_vertices_per_side, int32_t level, float grid_step)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	TheWorld_Utils::QuadrantPos pos(x, z, level, num_vertices_per_side, grid_step);
	QuadTree* quadTree = getQuadTree(pos, true, false, true);

	if (quadTree == nullptr)
		return;

	GDN_TheWorld_MapModder* mapModder = quadTree->getQuadrant()->getMapModder();
	if (mapModder == nullptr || mapModder->working())
		return;
	
	bool save = false;

	enum class GDN_TheWorld_MapModder::MapType mapType = (enum class GDN_TheWorld_MapModder::MapType)map_type;
	if (mapType == GDN_TheWorld_MapModder::MapType::initializedForNormals)
	{
		TheWorld_Utils::MemoryBuffer* cookedNormals = mapModder->getOutBuffer();
		TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer();
		size_t numNormals = (size_t)sqrt(float(cookedNormals->size() / sizeof(TheWorld_Utils::_RGB)));
		normalsBuffer.reserve(cookedNormals->size());
		memcpy(normalsBuffer.ptr(), cookedNormals->ptr(), cookedNormals->size());
		normalsBuffer.adjustSize(cookedNormals->size());
		mapModder->deinitRequired(true);

		TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
		terrainEdit->normalsNeedRegen = false;
		terrainEdit->splatmapNeedRegen = true;

		quadTree->getQuadrant()->setNormalsUpdated(true);
		quadTree->materialParamsNeedReset(true);
		quadTree->getQuadrant()->unlockInternalData();

		save = true;
	}
	else if (mapType == GDN_TheWorld_MapModder::MapType::initializedForSplatmap)
	{
		TheWorld_Utils::MemoryBuffer* cookedSplatmap = mapModder->getOutBuffer();
		size_t numPixels = (size_t)sqrt(float(cookedSplatmap->size() / sizeof(TheWorld_Utils::_RGBA)));
		TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer();
		splatmapBuffer.reserve(cookedSplatmap->size());
		memcpy(splatmapBuffer.ptr(), cookedSplatmap->ptr(), cookedSplatmap->size());
		splatmapBuffer.adjustSize(cookedSplatmap->size());
		mapModder->deinitRequired(true);

		TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
		terrainEdit->splatmapNeedRegen = false;
		terrainEdit->eastNeighbourXPlus.minHeight(mapModder->eastQuadrantheights().x);
		terrainEdit->eastNeighbourXPlus.maxHeight(mapModder->eastQuadrantheights().y);
		terrainEdit->westNeighbourXMinus.minHeight(mapModder->westQuadrantheights().x);
		terrainEdit->westNeighbourXMinus.maxHeight(mapModder->westQuadrantheights().y);
		terrainEdit->southNeighbourZPlus.minHeight(mapModder->southQuadrantheights().x);
		terrainEdit->southNeighbourZPlus.maxHeight(mapModder->southQuadrantheights().y);
		terrainEdit->northNeighbourZMinus.minHeight(mapModder->northQuadrantheights().x);
		terrainEdit->northNeighbourZMinus.maxHeight(mapModder->northQuadrantheights().y);
		terrainEdit->globalmapNeedRegen = true;

		quadTree->getQuadrant()->setSplatmapUpdated(true);
		quadTree->materialParamsNeedReset(true);
		quadTree->getQuadrant()->unlockInternalData();

		save = true;
	}
	else if (mapType == GDN_TheWorld_MapModder::MapType::initializedForGlobalmap)
	{
		TheWorld_Utils::MemoryBuffer* cookedGlobalmap = mapModder->getOutBuffer();
		TheWorld_Utils::MemoryBuffer& globalmapBuffer = quadTree->getQuadrant()->getGlobalmapBuffer();
		size_t numPixels = (size_t)sqrt(float(cookedGlobalmap->size() / sizeof(TheWorld_Utils::_RGB)));
		globalmapBuffer.reserve(cookedGlobalmap->size());
		memcpy(globalmapBuffer.ptr(), cookedGlobalmap->ptr(), cookedGlobalmap->size());
		globalmapBuffer.adjustSize(cookedGlobalmap->size());
		mapModder->deinitRequired(true);

		TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
		terrainEdit->globalmapNeedRegen = false;
		
		quadTree->getQuadrant()->setGlobalmapUpdated(true);
		quadTree->materialParamsNeedReset(true);
		quadTree->getQuadrant()->unlockInternalData();

		save = true;
	}

	if (save)
	{
		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr)
		{
			bool manual = !quadTree->autoSaveRequired();
			editModeUIControl->addQuadToSave(pos, manual);
		}
	}
}

void GDN_TheWorld_Viewer::setShaderParam(godot::String name, godot::Variant value)
{
	if (name == "ground_uv_scale")
	{
		m_shaderParam_groundUVScale.set_normal(godot::Vector3(value, value, value));
		m_shaderParam_groundUVScale.d = value;
		m_shaderParamChanged = true;
	}
	else if (name == "depth_blending")
	{
		m_shaderParam_depthBlending = value;
		m_shaderParamChanged = true;
	}
	else if (name == "triplanar")
	{
		m_shaderParam_triplanar = value;
		m_shaderParamChanged = true;
	}
	else if (name == "tile_reduction")
	{
		bool tileReduction = value;
		if (tileReduction)
		{
			m_shaderParam_tileReduction.set_normal(godot::Vector3(1.0f, 1.0f, 1.0f));
			m_shaderParam_tileReduction.d = 1.0f;
		}
		else
		{
			m_shaderParam_tileReduction.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));
			m_shaderParam_tileReduction.d = 0.0f;
		}
		m_shaderParamChanged = true;
	}
	else if (name == "globalmap_blend_start")
	{
		m_shaderParam_globalmapBlendStart = value;
		m_shaderParamChanged = true;
	}
	else if (name == "globalmap_blend_distance")
	{
		m_shaderParam_globalmapBlendDistance = value;
		m_shaderParamChanged = true;
	}
	else if (name == "colormap_opacity")
	{
		m_shaderParam_colormapOpacity.set_normal(godot::Vector3(value, value, value));
		m_shaderParam_colormapOpacity.d = value;
		m_shaderParamChanged = true;
	}
	else if (name == "force_elevation_0")
	{
		setForceShaderElevation0(value);
		m_shaderParamChanged = true;
	}
}

//Ref<Material> GDN_TheWorld_Viewer::getRegularMaterial(void)
//{
//	if (m_regularMaterial == nullptr)
//	{
//		getMainProcessingMutex().lock();
//		Ref<ShaderMaterial> mat = ShaderMaterial::_new();
//		getMainProcessingMutex().unlock();
//		ResourceLoader* resLoader = ResourceLoader::get_singleton();
//		//String shaderPath = "res://addons/twviewer/shaders/regularChunk.shader";
//		String shaderPath = "res://addons/twviewer/shaders/simple4.shader";
//		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
//		mat->set_shader(shader);
//		m_regularMaterial = mat;
//	}
//
//	return m_regularMaterial;
//}

//Ref<Material> GDN_TheWorld_Viewer::getLookDevMaterial(void)
//{
//	if (m_lookDevMaterial == nullptr)
//	{
//		getMainProcessingMutex().lock();
//		Ref<ShaderMaterial> mat = ShaderMaterial::_new();
//		getMainProcessingMutex().unlock();
//		ResourceLoader* resLoader = ResourceLoader::get_singleton();
//		String shaderPath = "res://addons/twviewer/shaders/lookdevChunk.shader";
//		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
//		mat->set_shader(shader);
//		m_lookDevMaterial = mat;
//	}
//
//	return m_lookDevMaterial;
//}

constexpr float SATELLITE_VIEW_CONTROL_DISTANCE_FROM_BORDER = 5;
#define SATELLITE_VIEW_CONTROL_SIZE						512
#define SATELLITE_CAMERA_ALTITUDE						20000

#define BACKMIRROR_VIEW_CONTROL_DISTANCE_FROM_BORDER	5
#define BACKMIRROR_VIEW_CONTROL_SIZE_X					400
#define BACKMIRROR_VIEW_CONTROL_SIZE_Y					200

void GDN_TheWorld_Viewer::setEditor3dOverlay(godot::Object* editor3dOverlay)
{
	m_editor3dOverlay = (godot::Control*)editor3dOverlay;
	if (isInEditor() and m_editor3dOverlay != nullptr)
	{
		int64_t editor3dOverlayId = m_editor3dOverlay->get_instance_id();

		GDN_TheWorld_HudControls* hudControls = HudControls();
		if (hudControls != nullptr)
		{
			godot::Node* parent = hudControls->get_parent();
			if (parent != nullptr)
			{
				int64_t parentId = parent->get_instance_id();
				if (parentId != editor3dOverlayId)
				{
					parent->remove_child(hudControls);
					m_editor3dOverlay->add_child(hudControls);
					godot::Size2 windowSize = getViewportSize();
					setPosHud(windowSize);
				}
			}
		}

		GDN_TheWorld_MapViewer* mapViewer = MapViewer();
		if (mapViewer != nullptr)
		{
			GDN_TheWorld_Map* map = mapViewer->getMap();
			if (map != nullptr)
			{
				godot::Node* parent = map->get_parent();
				if (parent != nullptr)
				{
					int64_t parentId = parent->get_instance_id();
					if (parentId != editor3dOverlayId)
					{
						parent->remove_child(map);
						m_editor3dOverlay->add_child(map);
						godot::Size2 windowSize = getViewportSize();
						setPosMapViewer(windowSize);
					}
				}
			}
		}
	}
}

void GDN_TheWorld_Viewer::setPosWorldViewer(godot::Size2& windowSize)
{
	//GDN_TheWorld_WorldViewportContainer* worldViewportContainer = WorldViewportContainer();
	//if (worldViewportContainer != nullptr)
	//{
	//	worldViewportContainer->set_size(windowSize);
	//}
}

void GDN_TheWorld_Viewer::createMapViewer(void)
{
	SceneTree* scene = get_tree();
	Node* sceneRoot = nullptr;
	if (IS_EDITOR_HINT())
		sceneRoot = scene->get_edited_scene_root();
	else
		sceneRoot = scene->get_root();
	int64_t sceneRootId = sceneRoot->get_instance_id();

	// MapViewer
	{
		GDN_TheWorld_MapViewer* mapViewer = MapViewer(false);
		if (mapViewer == nullptr)
			mapViewer = memnew(GDN_TheWorld_MapViewer);
		mapViewer->set_name(THEWORLD_MAP_VIEWER_NODE_NAME);
		mapViewer->set_visible(false);
		mapMode(false);
		mapViewer->setViewer(this);

		Node* parent = mapViewer->get_parent();
		if (parent != nullptr)
		{
			int64_t worldMainNodeId = getWorldNode()->get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			{
				parent->remove_child(mapViewer);
				getWorldNode()->add_child(mapViewer);
			}
		}
		else
		{
			getWorldNode()->add_child(mapViewer);
		}

		{
			godot::Node* owner = mapViewer->get_owner();
			if (owner != nullptr)
			{
				int64_t ownerId = owner->get_instance_id();
				if (ownerId != sceneRootId)
					mapViewer->set_owner(sceneRoot);
			}
			else
				mapViewer->set_owner(sceneRoot);
		}
		//mapViewer->call_deferred("set_owner", sceneRoot);
	}

	// MapViewportContainer
	{
		GDN_TheWorld_MapViewer* mapViewer = MapViewer();

		GDN_TheWorld_MapViewportContainer* mapViewportContainer = MapViewportContainer(false);
		if (mapViewportContainer == nullptr)
			mapViewportContainer = memnew(GDN_TheWorld_MapViewportContainer);
		mapViewportContainer->set_name(THEWORLD_MAP_VIEWPORT_CONTAINER_NODE_NAME);
		mapViewportContainer->setMapViewer(mapViewer);
		mapViewer->setMapViewportContainer(mapViewportContainer);

		Node* parent = mapViewportContainer->get_parent();
		if (parent != nullptr)
		{
			int64_t mapViewerId = mapViewer->get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != mapViewerId)
			{
				parent->remove_child(mapViewportContainer);
				mapViewer->add_child(mapViewportContainer);
			}
		}
		else
		{
			mapViewer->add_child(mapViewportContainer);
		}

		{
			godot::Node* owner = mapViewportContainer->get_owner();
			if (owner != nullptr)
			{
				int64_t ownerId = owner->get_instance_id();
				if (ownerId != sceneRootId)
					mapViewportContainer->set_owner(sceneRoot);
			}
			else
				mapViewportContainer->set_owner(sceneRoot);
		}
		//mapViewportContainer->call_deferred("set_owner", sceneRoot);
	}

	// MapViewport
	{
		GDN_TheWorld_MapViewportContainer* mapViewportContainer = MapViewportContainer();

		GDN_TheWorld_MapViewport* mapViewport = MapViewport(false);
		if (mapViewport == nullptr)
			mapViewport = memnew(GDN_TheWorld_MapViewport);
		mapViewport->set_name(THEWORLD_MAP_VIEWPORT_NODE_NAME);
		mapViewport->setMapViewportContainer(mapViewportContainer);
		mapViewportContainer->setMapViewport(mapViewport);

		Node* parent = mapViewport->get_parent();
		if (parent != nullptr)
		{
			int64_t mapViewportContainerId = mapViewportContainer->get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != mapViewportContainerId)
			{
				parent->remove_child(mapViewport);
				mapViewportContainer->add_child(mapViewport);
			}
		}
		else
		{
			mapViewportContainer->add_child(mapViewport);
		}

		{
			godot::Node* owner = mapViewport->get_owner();
			if (owner != nullptr)
			{
				int64_t ownerId = owner->get_instance_id();
				if (ownerId != sceneRootId)
					mapViewport->set_owner(sceneRoot);
			}
			else
				mapViewport->set_owner(sceneRoot);
		}
		//mapViewport->call_deferred("set_owner", sceneRoot);
	}

	// Map
	{
		GDN_TheWorld_MapViewer* mapViewer = MapViewer();
		GDN_TheWorld_MapViewport* mapViewport = MapViewport();
		
		GDN_TheWorld_Map* map = Map(false);
		if (map == nullptr)
			map = memnew(GDN_TheWorld_Map);
		map->set_name(THEWORLD_MAP_NODE_NAME);
		//map->set_anchors_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
		map->init(mapViewer);
		mapViewport->setMap(map);
		mapViewer->setMap(map);

		Node* parent = map->get_parent();
		if (parent != nullptr)
		{
			int64_t mapViewportId = mapViewport->get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != mapViewportId)
			{
				parent->remove_child(map);
				mapViewport->add_child(map);
			}
		}
		else
		{
			mapViewport->add_child(map);
		}

		{
			godot::Node* owner = map->get_owner();
			if (owner != nullptr)
			{
				int64_t ownerId = owner->get_instance_id();
				if (ownerId != sceneRootId)
					map->set_owner(sceneRoot);
			}
			else
				map->set_owner(sceneRoot);
		}
		//map->call_deferred("set_owner", sceneRoot);
	}

	// MapCamera
	{
		GDN_TheWorld_MapViewer* mapViewer = MapViewer();
		GDN_TheWorld_MapViewport* mapViewport = MapViewport();

		GDN_TheWorld_MapCamera* mapCamera = MapCamera(false);
		if (mapCamera == nullptr)
			mapCamera = memnew(GDN_TheWorld_MapCamera);
		mapCamera->set_name(THEWORLD_MAP_CAMERA_NODE_NAME);
		mapCamera->setMapViewer(mapViewer);
		mapCamera->set_enabled(false);

		Node* parent = mapCamera->get_parent();
		if (parent != nullptr)
		{
			int64_t mapViewportId = mapViewport->get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != mapViewportId)
			{
				parent->remove_child(mapCamera);
				mapViewport->add_child(mapCamera);
			}
		}
		else
		{
			mapViewport->add_child(mapCamera);
		}

		{
			godot::Node* owner = mapCamera->get_owner();
			if (owner != nullptr)
			{
				int64_t ownerId = owner->get_instance_id();
				if (ownerId != sceneRootId)
					mapCamera->set_owner(sceneRoot);
			}
			else
				mapCamera->set_owner(sceneRoot);
		}
		//mapCamera->call_deferred("set_owner", sceneRoot);
	}
}

void GDN_TheWorld_Viewer::setPosMapViewer(godot::Size2& windowSize)
{
	GDN_TheWorld_MapViewportContainer* mapViewportContainer = MapViewportContainer();
	if (mapViewportContainer != nullptr)
	{
		mapViewportContainer->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		mapViewportContainer->set_offset(SIDE_LEFT, 0);
		mapViewportContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
		mapViewportContainer->set_offset(SIDE_TOP, 0);
		mapViewportContainer->set_size(windowSize);
	}

	//GDN_TheWorld_Map* map = Map();
	//if (map != nullptr)
	//{
	//	map->set_anchor(godot::Side::SIDE_LEFT, 0.0);
	//	map->set_offset(SIDE_LEFT, 0);
	//	mapViewportContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
	//	map->set_offset(SIDE_TOP, 0);
	//	map->set_size(windowSize);
	//}
}

void GDN_TheWorld_Viewer::createHud(void)
{
	// Hud Controls
	{
		SceneTree* scene = get_tree();
		Node* sceneRoot = nullptr;
		if (IS_EDITOR_HINT())
			sceneRoot = scene->get_edited_scene_root();
		else
			sceneRoot = scene->get_root();
		int64_t sceneRootId = sceneRoot->get_instance_id();

		GDN_TheWorld_HudControls* hudControls = HudControls(false);
		if (hudControls == nullptr)
		{
			hudControls = memnew(GDN_TheWorld_HudControls);
			m_hudControls = hudControls;	// we force not to use cache so we don't have to add this control to the scene
		}
		hudControls->set_name(THEWORLD_HUD_CONTROLS_NODE_NAME);
		Node* parent = hudControls->get_parent();
		if (parent != nullptr)
		{
			int64_t currentNodeId = get_instance_id();

			int64_t parentId = parent->get_instance_id();
			if (parentId != currentNodeId)
			{
				parent->remove_child(hudControls);
				add_child(hudControls);
			}
		}
		else
		{
			add_child(hudControls);
		}

		//{
		//	godot::Node* owner = hudControls->get_owner();
		//	if (owner != nullptr)
		//	{
		//		int64_t ownerId = owner->get_instance_id();
		//		if (ownerId != sceneRootId)
		//			hudControls->set_owner(sceneRoot);
		//	}
		//	else
		//		hudControls->set_owner(sceneRoot);
		//}
	}

	godot::Node* controlsParent = HudControls();
	
	// Message Box
	{
		m_hudMessageContainer = memnew(godot::HBoxContainer);
		controlsParent->add_child(m_hudMessageContainer);
		m_hudMessageContainer->set_name("HudMessageContainer");

		m_hudMessageLabel = memnew(godot::Label);
		m_hudMessageContainer->add_child(m_hudMessageLabel);
		m_hudMessageLabel->set_name("HudMessage");
		m_hudMessageLabel->add_theme_color_override("font_color", Color(251.0f / 255.0f, 206.0f / 255.0f, 177.0f / 255.0f));
	}

	// Streaming Info Box
	{
		m_hudStreamingInfoContainer = memnew(godot::HBoxContainer);
		controlsParent->add_child(m_hudStreamingInfoContainer);
		m_hudStreamingInfoContainer->set_name("HudStreamingInfoContainer");

		m_hudStreamingInfoLabel = memnew(godot::Label);
		m_hudStreamingInfoContainer->add_child(m_hudStreamingInfoLabel);
		m_hudStreamingInfoLabel->set_name("HudStreamingInfo");
		m_hudStreamingInfoLabel->add_theme_color_override("font_color", Color(251.0f / 255.0f, 206.0f / 255.0f, 177.0f / 255.0f));
	}
	
	// Satellite view
	{
		m_satelliteSubViewportContainer = memnew(godot::SubViewportContainer);
		controlsParent->add_child(m_satelliteSubViewportContainer);
		m_satelliteSubViewportContainer->set_name("HudSatelliteSubViewportContainer");
		m_satelliteSubViewportContainer->set_visible(false);

		{
			m_northLabel = memnew(godot::Label);
			m_satelliteSubViewportContainer->add_child(m_northLabel);
			m_northLabel->set_text("N");
			m_northLabel->set_modulate(GDN_TheWorld_Globals::g_color_red);
			m_southLabel = memnew(godot::Label);
			m_satelliteSubViewportContainer->add_child(m_southLabel);
			m_southLabel->set_text("S");
			m_southLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
			m_eastLabel = memnew(godot::Label);
			m_satelliteSubViewportContainer->add_child(m_eastLabel);
			m_eastLabel->set_text("E");
			m_eastLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
			m_westLabel = memnew(godot::Label);
			m_satelliteSubViewportContainer->add_child(m_westLabel);
			m_westLabel->set_text("W");
			m_westLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
		}

		m_satelliteSubViewport = memnew(godot::SubViewport);
		m_satelliteSubViewportContainer->add_child(m_satelliteSubViewport);
		m_satelliteSubViewport->set_name("HudSatelliteView");

		m_satelliteCamera = memnew(godot::Camera3D);
		m_satelliteSubViewport->add_child(m_satelliteCamera);
		m_satelliteCamera->set_name("HudSatelliteCamera");
		m_satelliteCamera->set_near(1);
		m_satelliteCamera->set_far(SATELLITE_CAMERA_ALTITUDE * 1.5);
	}

	// Back Mirror View
	{
		m_backMirrorSubViewportContainer = memnew(godot::SubViewportContainer);
		controlsParent->add_child(m_backMirrorSubViewportContainer);
		m_backMirrorSubViewportContainer->set_name("HudBackMirrorSubViewportContainer");
		m_backMirrorSubViewportContainer->set_visible(false);

		m_backMirrorSubViewport = memnew(godot::SubViewport);
		m_backMirrorSubViewportContainer->add_child(m_backMirrorSubViewport);
		m_backMirrorSubViewport->set_name("HudBackMirrorView");

		m_backMirrorCamera = memnew(godot::Camera3D);
		m_backMirrorSubViewport->add_child(m_backMirrorCamera);
		m_backMirrorCamera->set_name("HudBackMirrorCamera");
	}
}

void GDN_TheWorld_Viewer::setPosHud(godot::Size2& windowSize)
{
	GDN_TheWorld_HudControls* hudControls = HudControls();
	if (hudControls != nullptr)
	{
		//godot::Vector2i windowSize = get_tree()->get_root()->get_size();
		//godot::Size2 windowSize  = getViewportSize();
		hudControls->set_size(windowSize);
	}
	
	// Message Box
	if (m_hudMessageContainer != nullptr)
	{
		m_hudMessageContainer->set_anchor(godot::Side::SIDE_LEFT, 0.3);
		m_hudMessageContainer->set_offset(SIDE_LEFT, 0);
		m_hudMessageContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_hudMessageContainer->set_offset(SIDE_TOP, 0);
	}

	// Streaming Info Box
	if (m_hudStreamingInfoContainer != nullptr)
	{
		m_hudStreamingInfoContainer->set_anchor(godot::Side::SIDE_LEFT, 0.65);
		m_hudStreamingInfoContainer->set_offset(SIDE_LEFT, 0);
		m_hudStreamingInfoContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_hudStreamingInfoContainer->set_offset(SIDE_TOP, 0);
	}

	// Satellite view
	{
		if (m_satelliteSubViewportContainer != nullptr)
		{
			m_satelliteSubViewportContainer->set_size(godot::Vector2(SATELLITE_VIEW_CONTROL_SIZE, SATELLITE_VIEW_CONTROL_SIZE));
			m_satelliteSubViewportContainer->set_scale(godot::Vector2(1, 1));

			//m_satelliteSubViewportContainer->set_anchors_preset(godot::Control::LayoutPreset::PRESET_BOTTOM_RIGHT);
			m_satelliteSubViewportContainer->set_anchor(godot::Side::SIDE_LEFT, 1.0);
			m_satelliteSubViewportContainer->set_offset(godot::Side::SIDE_LEFT, -(SATELLITE_VIEW_CONTROL_SIZE + SATELLITE_VIEW_CONTROL_DISTANCE_FROM_BORDER));
			m_satelliteSubViewportContainer->set_anchor(godot::Side::SIDE_RIGHT, 1.0);
			m_satelliteSubViewportContainer->set_offset(godot::Side::SIDE_RIGHT, -(SATELLITE_VIEW_CONTROL_DISTANCE_FROM_BORDER));
			m_satelliteSubViewportContainer->set_anchor(godot::Side::SIDE_TOP, 1.0);
			m_satelliteSubViewportContainer->set_offset(godot::Side::SIDE_TOP, -(SATELLITE_VIEW_CONTROL_SIZE + SATELLITE_VIEW_CONTROL_DISTANCE_FROM_BORDER));
			m_satelliteSubViewportContainer->set_anchor(godot::Side::SIDE_BOTTOM, 1.0);
			m_satelliteSubViewportContainer->set_offset(godot::Side::SIDE_BOTTOM, -(SATELLITE_VIEW_CONTROL_DISTANCE_FROM_BORDER));
		}

		if (m_satelliteSubViewport != nullptr)
			m_satelliteSubViewport->set_size(godot::Vector2i(SATELLITE_VIEW_CONTROL_SIZE, SATELLITE_VIEW_CONTROL_SIZE));

		if (m_northLabel != nullptr)
		{
			m_northLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
			m_northLabel->set_offset(SIDE_LEFT, SATELLITE_VIEW_CONTROL_SIZE / 2);
			m_northLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
			m_northLabel->set_offset(SIDE_TOP, 0);
		}

		if (m_southLabel != nullptr)
		{
			m_southLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
			m_southLabel->set_offset(SIDE_LEFT, SATELLITE_VIEW_CONTROL_SIZE / 2);
			m_southLabel->set_anchor(godot::Side::SIDE_TOP, 1.0);
			m_southLabel->set_offset(SIDE_TOP, -20);
		}

		if (m_westLabel != nullptr)
		{
			m_westLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
			m_westLabel->set_offset(SIDE_LEFT, 0.0);
			m_westLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
			m_westLabel->set_offset(SIDE_TOP, SATELLITE_VIEW_CONTROL_SIZE / 2);
		}

		if (m_eastLabel != nullptr)
		{
			m_eastLabel->set_anchor(godot::Side::SIDE_LEFT, 1.0);
			m_eastLabel->set_offset(SIDE_LEFT, -20);
			m_eastLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
			m_eastLabel->set_offset(SIDE_TOP, SATELLITE_VIEW_CONTROL_SIZE / 2);
		}
	}

	// Back Mirror View
	{
		if (m_eastLabel != nullptr)
		{
			m_backMirrorSubViewportContainer->set_size(godot::Vector2(BACKMIRROR_VIEW_CONTROL_SIZE_X, BACKMIRROR_VIEW_CONTROL_SIZE_Y));
			m_backMirrorSubViewportContainer->set_scale(godot::Vector2(1, 1));

			//m_satelliteSubViewportContainer->set_anchors_preset(godot::Control::LayoutPreset::PRESET_BOTTOM_RIGHT);
			m_backMirrorSubViewportContainer->set_anchor(godot::Side::SIDE_LEFT, 0.0);
			m_backMirrorSubViewportContainer->set_offset(godot::Side::SIDE_LEFT, BACKMIRROR_VIEW_CONTROL_DISTANCE_FROM_BORDER);
			m_backMirrorSubViewportContainer->set_anchor(godot::Side::SIDE_RIGHT, 0.0);
			m_backMirrorSubViewportContainer->set_offset(godot::Side::SIDE_RIGHT, BACKMIRROR_VIEW_CONTROL_SIZE_X + BACKMIRROR_VIEW_CONTROL_DISTANCE_FROM_BORDER);
			m_backMirrorSubViewportContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
			m_backMirrorSubViewportContainer->set_offset(godot::Side::SIDE_TOP, BACKMIRROR_VIEW_CONTROL_DISTANCE_FROM_BORDER);
			m_backMirrorSubViewportContainer->set_anchor(godot::Side::SIDE_BOTTOM, 0.0);
			m_backMirrorSubViewportContainer->set_offset(godot::Side::SIDE_BOTTOM, BACKMIRROR_VIEW_CONTROL_SIZE_Y + BACKMIRROR_VIEW_CONTROL_DISTANCE_FROM_BORDER);
		}

		if (m_backMirrorSubViewport != nullptr)
			m_backMirrorSubViewport->set_size(godot::Vector2i(BACKMIRROR_VIEW_CONTROL_SIZE_X, BACKMIRROR_VIEW_CONTROL_SIZE_Y));

	}
}

bool GDN_TheWorld_Viewer::init(void)
{
	PLOG_INFO << "TheWorld Viewer Initializing...";

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return false;

	globals->addElementToInitialize();
	
	m_worldQuadrantsMap.init(this);
	m_mapQuadrantsMap.init(this, MAP_QUADRANT_OTPIMIZATION_LEVEL);

	// Shader Global Params
	{
		float _gridStepInWU = Globals()->gridStepInWU();
		godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
		rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GRID_STEP, _gridStepInWU);
		//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_SEA, globals->getParamShowSea());
		//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_TERRAIN, globals->getParamShowTerrain());
		rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_CHUNKS, globals->getParamShowChunks());

		godot::String waterFoamImageName("res://addons/twviewer/shaders/water_foam.png");
		godot::Ref<godot::Texture2D> waterFoamTex = ResourceLoader::get_singleton()->load(waterFoamImageName);
		rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_WATER_FOAM, waterFoamTex);

		shaderParamSeaFlatSeaHeightApplied(DEFAULT_SHADER_PARAM_SEA_FLAT_SEA_HEIGHT);
		shaderParamSeaNoiseScaleApplied(DEFAULT_SHADER_PARAM_SEA_NOISE_SCALE);
		shaderParamSeaTimeScaleApplied(DEFAULT_SHADER_PARAM_SEA_TIME_SCALE);
		shaderParamSeaHeightScaleApplied(DEFAULT_SHADER_PARAM_SEA_HEIGHT_SCALE);
		shaderParamSeaRoughFactorApplied(DEFAULT_SHADER_PARAM_SEA_ROUGH_FACTOR);
		shaderParamSeaEndCamAltForWavesApplied(DEFAULT_SHADER_PARAM_SEA_END_CAM_ALT_FOR_WAVES);
		shaderParamSeaStartCamAltForWavesAttenuationApplied(DEFAULT_SHADER_PARAM_SEA_START_CAM_ALT_FOR_WAVES_ATTENUATION);
		shaderParamSeaStartTerrDepthForWavesAttenuationApplied(DEFAULT_SHADER_PARAM_SEA_START_TERR_DEPTH_FOR_WAVES_ATTENUATION);
		shaderParamSeaScaleOffsetThresholdApplied(DEFAULT_SHADER_PARAM_SEA_SCALE_OFFSET_THRESHOLD);

		if (IS_EDITOR_HINT())
		{
			if (m_minDepthQuadOnPerimeter == -1)
				m_minDepthQuadOnPerimeter = THEWORLD_DEFAULT_DEPTH_QUAD_ON_PERIMETER_IN_EDITOR;
			if (m_cacheQuadOnPerimeter == -1)
				m_cacheQuadOnPerimeter = THEWORLD_DEFAULT_CACHE_QUAD_ON_PERIMETER_IN_EDITOR;
			if (m_maxDepthQuadOnPerimeter == -1)
				m_maxDepthQuadOnPerimeter = MAX_SETTABLE_DEPTH_ON_PERIMETER;
		}
		else
		{
			if (m_minDepthQuadOnPerimeter == -1)
				m_minDepthQuadOnPerimeter = THEWORLD_DEFAULT_DEPTH_QUAD_ON_PERIMETER;
			if (m_cacheQuadOnPerimeter == -1)
				m_cacheQuadOnPerimeter = THEWORLD_DEFAULT_CACHE_QUAD_ON_PERIMETER;
			if (m_maxDepthQuadOnPerimeter == -1)
				m_maxDepthQuadOnPerimeter = MAX_SETTABLE_DEPTH_ON_PERIMETER;
		}

		m_shaderParam_groundUVScale.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));	// if every coord is 0.0 the parameter is not passed to the shader
		m_shaderParam_groundUVScale.d = 0.0f;
		m_shaderParam_tileReduction.set_normal(godot::Vector3(0.0f, 0.0f, 0.0f));
		m_shaderParam_tileReduction.d = 0.0f;
		m_shaderParam_colormapOpacity.set_normal(godot::Vector3(1.0f, 1.0f, 1.0f));
		m_shaderParam_colormapOpacity.d = 1.0f;

		ShaderTerrainData::ShaderTerrainData::getGroundTextures(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()), this);
		std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();
		globals->addElementToInitialize((int)groundTextures.size());
	}

	// From _notification ENTER_WORLD
	printKeyboardMapping();

	string s = "Use Visual Server: "; s += (m_useVisualServer ? "True" : "False");
	if (globals != nullptr)
	{
		globals->infoPrint(s.c_str());
		globals->debugPrint("Enter world");
	}

	set_notify_transform(true);

	// Set World Camera
	{
		GDN_TheWorld_Camera* camera = WorldCamera(false);
		if (camera == nullptr)
			camera = memnew(GDN_TheWorld_Camera);
		camera->set_name(THEWORLD_CAMERA_NODE_NAME);
		setCachedCustomWorldCamera(camera);

		SceneTree* scene = get_tree();
		Node* sceneRoot = nullptr;
		if (IS_EDITOR_HINT())
			sceneRoot = scene->get_edited_scene_root();
		else
			sceneRoot = scene->get_root();

		int64_t worldMainNodeId = getWorldNode()->get_instance_id();
		//int64_t worldViewportId = WorldViewport()->get_instance_id();

		Node* parent = camera->get_parent();
		if (parent != nullptr)
		{
			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			//if (parentId != worldViewportId)
			{
				parent->remove_child(camera);
				getWorldNode()->add_child(camera);
				//WorldViewport()->add_child(camera);
			}
		}
		else
		{
			getWorldNode()->add_child(camera);
			//WorldViewport()->add_child(camera);
		}

		camera->set_owner(sceneRoot);
		//camera->call_deferred("set_owner", sceneRoot);
	}

	// Set Mesh cache
	{
		m_meshCache = make_unique<MeshCache>(this);
		m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());
	}

	// Set Streamer Thread
	{
		m_streamerThreadRequiredExit = false;
		m_streamerThreadRunning = true;
		m_streamerThread = std::thread(&GDN_TheWorld_Viewer::streamer, this);
	}
	
	// Set Editor Elements
	{
		godot::Control* control = getOrCreateEditModeUIControl();

		m_normalDrawer = memnew(GDN_TheWorld_Drawer);
		this->add_child(m_normalDrawer);
		const float radius = 1;
		m_normalLineIdx = m_normalDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::end, 15.0f, godot::Vector2i(-10, -25), GDN_TheWorld_Globals::g_color_black);
		m_normalSphereIdx = m_normalDrawer->addSphere(godot::Vector3(), radius, GDN_TheWorld_Globals::g_color_cyan);
		m_normalDrawer->set_visible(false);

		m_positionDrawer = memnew(GDN_TheWorld_Drawer);
		this->add_child(m_positionDrawer);
		m_labelPositionXIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 20), "", GDN_TheWorld_Globals::g_color_black);
		m_labelPositionZIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 40), "", GDN_TheWorld_Globals::g_color_black);
		m_labelPositionYIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 60), "", GDN_TheWorld_Globals::g_color_black);
		m_labelPositionDistIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 80), "", GDN_TheWorld_Globals::g_color_black);
		m_labelPositionDirIdx = m_positionDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-10, 100), "", GDN_TheWorld_Globals::g_color_black);
		m_linePositionIdx = m_positionDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::center, -1.0f, godot::Vector2i(-10, -10), GDN_TheWorld_Globals::g_color_black);
		m_spherePositionStartIdx = m_positionDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);
		m_spherePositionEndIdx = m_positionDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);
		m_positionDrawer->set_visible(false);

		m_rayFromCameraDrawer = memnew(GDN_TheWorld_Drawer);
		this->add_child(m_rayFromCameraDrawer);
		m_lineRayFromCameraIdx = m_rayFromCameraDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::center, -1.0f, godot::Vector2i(-10, -10), GDN_TheWorld_Globals::g_color_black);
		m_sphereRayFromCameraStartIdx = m_rayFromCameraDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);
		m_sphereRayFromCameraEndIdx = m_rayFromCameraDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_cyan);
		m_rayFromCameraDrawer->set_visible(false);

		m_gizmo = memnew(GDN_TheWorld_Gizmo3d);
		this->add_child(m_gizmo);
		m_gizmo->set_visible(false);
		m_gizmo->set_viewer(this);
	}

	globals->addNumElementInitialized();

	//m_mapModder = memnew(GDN_TheWorld_MapModder);
	//m_mapModder->custom_ready(this, this, false);

	m_tpClientWorker.Start("ClientWorker", 3, this);

	godot::Size2 windowSize = getViewportSize();
	
	// Set HUD
	{
		createHud();
		setPosHud(windowSize);
	}
	
	// Set MapViewer
	{
		createMapViewer();
		setPosMapViewer(windowSize);
	}
	
	m_initialized = true;
	m_initRequired = false;
	PLOG_INFO << "TheWorld Viewer Initialized!";

	return true;
}

void GDN_TheWorld_Viewer::preDeinit(void)
{
	enum class TheWorldStatus status = Globals()->status();
	if ((int)status >= (int)TheWorldStatus::worldDeployInProgress)
	{
		while ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
		{
			status = Globals()->status();
			if ((int)status < (int)TheWorldStatus::worldUnDeployInProgress)
				undeployWorld();

			status = Globals()->status();
			if (status == TheWorldStatus::worldDeployInProgress)
				Globals()->setStatus(TheWorldStatus::worldDeployed);

			Sleep(5);
		}
	}

	m_streamerThreadRequiredExit = true;
}

bool GDN_TheWorld_Viewer::canDeinit(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	for (auto& it : m_worldQuadrantsMap)
	{
		if (it.second->getQuadrant()->internalDataLocked())
			return false;
	}
	
	return !m_streamerThreadRunning;
}

void GDN_TheWorld_Viewer::deinit(void)
{
	if (m_initialized)
	{
		PLOG_INFO << "TheWorld Viewer Deinitializing...";

		m_tpClientWorker.Stop();
		
		// from _notification EXIT_WORLD
		GDN_TheWorld_Globals* globals = Globals();
		if (globals != nullptr)
			globals->debugPrint("Exit world");

		if (m_normalDrawer != nullptr)
		{
			Node* parent = m_normalDrawer->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_normalDrawer);
			m_normalDrawer->queue_free();
			m_normalDrawer = nullptr;
		}
		
		if (m_positionDrawer != nullptr)
		{
			Node* parent = m_positionDrawer->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_positionDrawer);
			m_positionDrawer->queue_free();
			m_positionDrawer = nullptr;
		}

		if (m_rayFromCameraDrawer != nullptr)
		{
			Node* parent = m_rayFromCameraDrawer->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_rayFromCameraDrawer);
			m_rayFromCameraDrawer->queue_free();
			m_rayFromCameraDrawer = nullptr;
		}

		if (m_gizmo != nullptr)
		{
			Node* parent = m_gizmo->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_gizmo);
			m_gizmo->queue_free();
			m_gizmo = nullptr;
		}

		//if (m_mapModder != nullptr)
		//{
		//	Node* parent = m_mapModder->get_parent();
		//	if (parent != nullptr)
		//		parent->remove_child(m_mapModder);
		//	m_mapModder->queue_free();
		//	m_mapModder = nullptr;
		//}
		
		//if (m_initialWorldViewerPosSet)
		//{
		// useless as at this point m_worldQuadrantsMap is empty because it has been cleared during undeploy reply from server
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			if (!itQuadTree->second->isValid())
				continue;

			// WARNING
			//Chunk::ExitWorldChunkAction action;
			//itQuadTree->second->ForAllChunk(action);

			itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
		}
		//}

		for (auto& it : m_worldQuadrantsMap)
		{
			while(it.second->getQuadrant()->internalDataLocked())
				Sleep(1);;
		}


		m_streamerThreadRequiredExit = true;
		if (m_streamerThread.joinable())
			m_streamerThread.join();

		//{
		//	godot::GDN_TheWorld_Globals::s_num = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed1 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed2 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed3 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed4 = 0;
		//	godot::GDN_TheWorld_Globals::s_elapsed5 = 0;
		//}
		clearQuadTreeMap();
		//{
		//	size_t el1 = godot::GDN_TheWorld_Globals::s_elapsed1 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el2 = godot::GDN_TheWorld_Globals::s_elapsed2 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el3 = godot::GDN_TheWorld_Globals::s_elapsed3 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el4 = godot::GDN_TheWorld_Globals::s_elapsed4 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el5 = godot::GDN_TheWorld_Globals::s_elapsed5 / godot::GDN_TheWorld_Globals::s_num;
		//	size_t el = godot::GDN_TheWorld_Globals::s_elapsed1 / godot::GDN_TheWorld_Globals::s_num;
		//}
		m_meshCache.reset();

		destroyModeUIControl();

		//GDN_TheWorld_Camera* camera = CameraNode(true);
		//if (camera != nullptr)
		//{
		//	Node* parent = camera->get_parent();
		//	if (parent != nullptr)									// TESTING
		//		parent->call_deferred("remove_child", camera);			// TESTING
		//	camera->queue_free();										// TESTING
		//}
		
		ShaderTerrainData::releaseGlobals();

		m_initialized = false;
		PLOG_INFO << "TheWorld Viewer Deinitialized!";
		
		Globals()->debugPrint("GDN_TheWorld_Viewer::deinit DONE!");
	}
}

void GDN_TheWorld_Viewer::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
{
	std::string method = reply.getMethod();

	if (reply.error())
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		return;
	}

	try
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES)
		{
			TheWorld_Utils::Profiler::addElapsedMs(std::string("WorldDeploy 1 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES, reply.duration());
			
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES);

			enum class TheWorldStatus status = Globals()->status();
			if (status == TheWorldStatus::worldUnDeployInProgress || status < TheWorldStatus::worldDeployInProgress)
			{
				GDN_TheWorld_Globals* globals = Globals();
				if (globals != nullptr)
				{
					std::string msg = std::string("GDN_TheWorld_Viewer::replyFromServer - ") + THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES + " - Status=" + std::to_string(int(status)) + " (" + globals->statusStr(status) + "): cannot proceed deploying quadrant";
					globals->debugPrint(msg.c_str());
				}
			}
			else
			{
				//TimerMs clock("GDN_TheWorld_Viewer::replyFromServer", "CLIENT SIDE", false, true);

				if (reply.getNumReplyParams() < 3)
				{
					std::string m = std::string("Reply MapManager::getVertices error (not enough params replied)");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}

				ClientServerVariant v = reply.getReplyParam(0);
				const auto _cameraPosX(std::get_if<float>(&v));
				if (_cameraPosX == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as first return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosX = *_cameraPosX;

				v = reply.getReplyParam(1);
				const auto _cameraPosZ(std::get_if<float>(&v));
				if (_cameraPosZ == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a float as second return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosZ = *_cameraPosZ;

				ClientServerVariant vbuffGridVertices = reply.getReplyParam(2);
				const auto _buffGridVerticesFromServer(std::get_if<std::vector<unsigned char>>(&vbuffGridVertices));
				if (_buffGridVerticesFromServer == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a string as third return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				std::string buffGridVerticesFromServer(_buffGridVerticesFromServer->begin(), _buffGridVerticesFromServer->end());

				ClientServerVariant vmeshIdFromServer = reply.getReplyParam(3);
				const auto _meshIdFromServer(std::get_if<std::vector<unsigned char>>(&vmeshIdFromServer));
				if (_meshIdFromServer == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not return a std::vector<unsigned char> as fourth return param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				std::string meshIdFromServer(_meshIdFromServer->begin(), _meshIdFromServer->end());

				v = reply.getInputParam(2);
				const auto _lowerXGridVertex(std::get_if<float>(&v));
				if (_lowerXGridVertex == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as third input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float lowerXGridVertex = *_lowerXGridVertex;

				v = reply.getInputParam(3);
				const auto _lowerZGridVertex(std::get_if<float>(&v));
				if (_lowerZGridVertex == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as forth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float lowerZGridVertex = *_lowerZGridVertex;

				v = reply.getInputParam(4);
				const auto _numVerticesPerSize(std::get_if<int>(&v));
				if (_numVerticesPerSize == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have an int as fifth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				int numVerticesPerSize = *_numVerticesPerSize;

				v = reply.getInputParam(5);
				const auto _gridStepInWU(std::get_if<float>(&v));
				if (_gridStepInWU == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as sixth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float gridStepInWU = *_gridStepInWU;

				v = reply.getInputParam(6);
				const auto _level(std::get_if<int>(&v));
				if (_level == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have an int as seventh input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				int level = *_level;

				//v = reply.getInputParam(7);
				//const auto _meshIdFromServer(std::get_if<std::vector<unsigned char>>(&v));
				//if (_meshIdFromServer == NULL)
				//{
				//	std::string m = std::string("Reply MapManager::getVertices did not have a string as eight input param");
				//	throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				//}
				//std::string meshIdFromServer(_meshIdFromServer->begin(), _meshIdFromServer->end());

				v = reply.getInputParam(8);
				const auto _setCamera(std::get_if<bool>(&v));
				if (_setCamera == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a bool as nineth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				bool setCamera = *_setCamera;

				v = reply.getInputParam(9);
				const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
				if (_cameraDistanceFromTerrainForced == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as tenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

				v = reply.getInputParam(10);
				const auto _cameraPosY(std::get_if<float>(&v));
				if (_cameraPosY == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as eleventh input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPosY = *_cameraPosY;

				v = reply.getInputParam(11);
				const auto _cameraYaw(std::get_if<float>(&v));
				if (_cameraYaw == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as twelveth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraYaw = *_cameraYaw;

				v = reply.getInputParam(12);
				const auto _cameraPitch(std::get_if<float>(&v));
				if (_cameraPitch == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as thirteenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraPitch = *_cameraPitch;

				v = reply.getInputParam(13);
				const auto _cameraRoll(std::get_if<float>(&v));
				if (_cameraRoll == NULL)
				{
					std::string m = std::string("Reply MapManager::getVertices did not have a float as fourteenth input param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				float cameraRoll = *_cameraRoll;

				TheWorld_Utils::QuadrantPos quadrantPos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);
				//if (quadrantPos.getIndexPosX() == 100 && quadrantPos.getIndexPosZ() == 100)	// DEBUG ONLY
				//	v = reply.getInputParam(13);												// DEBUG ONLY

				m_mtxQuadTreeAndMainProcessing.lock();
				if ((m_worldQuadrantsMap.contains(quadrantPos) && !m_worldQuadrantsMap[quadrantPos]->statusToErase())
					|| (m_extraQuadsMap.contains(quadrantPos) && !m_extraQuadsMap[quadrantPos]->statusToErase() && status == TheWorldStatus::worldDeployed))
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1 ") + __FUNCTION__, "Elab. quadrant");
					QuadTree* quadTree = nullptr;
					bool refreshingQuadrant = false;
					bool worldQuadrant = true;
					if (m_worldQuadrantsMap.contains(quadrantPos))
					{
						quadTree = m_worldQuadrantsMap[quadrantPos].get();
						if (quadTree->isRefreshingData())
						{
							refreshingQuadrant = true;
						}
					}
					else
					{
						quadTree = m_extraQuadsMap[quadrantPos].get();
						worldQuadrant = false;
						//if (quadrantPos.getIndexPosX() == 93 && quadrantPos.getIndexPosZ() == 99)
						//	PLOG_DEBUG << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " Reply from server";
					}
					m_mtxQuadTreeAndMainProcessing.unlock();

					//PLOG_DEBUG << "GDN_TheWorld_Viewer::replyFromServer - " << quadTree->getQuadrant()->getPos().getName();

					//if (quadTree->getQuadrant()->getPos().getIndexPosX() == -2 && quadTree->getQuadrant()->getPos().getIndexPosZ() == 3)
					//{
					//	PLOG_DEBUG << "TRACKING replyFromServer " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << quadTree->getQuadrant()->getPos().getIndexPosX() << "Z=" << quadTree->getQuadrant()->getPos().getIndexPosZ();
					//}

					if (m_stopDeploy)
					{
						//quadTree->setStatus(QuadrantStatus::initialized);
						quadTree->setStatus(QuadrantStatus::initializationStopped);
					}
					else
					{
						if (quadTree->statusGetTerrainDataInProgress() || quadTree->statusRefreshTerrainDataInProgress())
						{
							//std::recursive_mutex& quadrantMutex = quadTree->getQuadrantMutex();
							//std::lock_guard<std::recursive_mutex> lock(quadrantMutex);
							while (quadTree->getQuadrant()->lockInternalData() == false)
								Sleep(1);

							std::string outMeshIdFromBuffer;
							{
								//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);	// SUPERDEBUGRIC : to remove when the mock for altitudes is removed from cache.refreshMeshCacheFromBuffer
								TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1 ") + __FUNCTION__, "quadTree->getQuadrant()->refreshGridVertices");
								quadTree->getQuadrant()->refreshGridVerticesFromServer(buffGridVerticesFromServer, meshIdFromServer, outMeshIdFromBuffer, true);
								
								if (worldQuadrant)
								{
									godot::Ref<godot::Image> mapImage = quadTree->getQuadrant()->getMapImage();
									m_mapQuadrantsMap.setMapData(mapImage, quadrantPos);
								}
							}

							{
								quadTree->materialParamsNeedReset(true);
								quadTree->colliderNeedReset(true, QuadType::NotSet);
								if (quadTree->statusRefreshTerrainDataInProgress())
								{
									quadTree->getQuadrant()->northQuadrantData().initialized(false);
									quadTree->getQuadrant()->southQuadrantData().initialized(false);
									quadTree->getQuadrant()->westQuadrantData().initialized(false);
									quadTree->getQuadrant()->eastQuadrantData().initialized(false);
								}
							}

							if (setCamera)
							{
								TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.2 ") + __FUNCTION__, "setCamera");

								forceRefreshQuadTreeMap();

								quadTree->setVisible(true);

								size_t cameraInGridIndex = quadTree->getQuadrant()->getIndexFromMap(cameraPosX, cameraPosZ);
								if (cameraInGridIndex == -1)
								{
									quadTree->getQuadrant()->unlockInternalData();
									throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Not found WorldViewer Pos").c_str()));
								}

								float cameraHeight = quadTree->getQuadrant()->getAltitudeFromHeigthmap(cameraInGridIndex);

								Vector3 cameraPos(cameraPosX, cameraPosY, cameraPosZ);		// MapManager coordinates are local coordinates of WorldNode
								if (cameraDistanceFromTerrainForced != 0.0f)
									cameraPos.y = cameraHeight + cameraDistanceFromTerrainForced;
								float offset = cameraDistanceFromTerrainForced == 0.0f ? 300 : cameraDistanceFromTerrainForced;
								//Vector3 lookAt(cameraPosX + offset, cameraHeight, cameraPosZ + offset);
								Vector3 lookAt(cameraPosX, cameraHeight, cameraPosZ - abs(offset));		// camera look at terrain along north direction

								// Viewer stuff: set viewer position relative to world node at the first point of the bitmap and altitude 0 so that that point is at position (0,0,0) respect to the viewer
								//Transform t = get_transform();
								//t.origin = Vector3(quadTree->getQuadrant()->getGridVertices()[0].posX(), 0, quadTree->getQuadrant()->getGridVertices()[0].posZ());
								//set_transform(t);

								//WorldCamera()->initCameraInWorld(cameraPos, lookAt, cameraYaw, cameraPitch, cameraRoll);
								getCachedCustomWorldCamera()->call_deferred("init_camera_in_world", cameraPos, lookAt, cameraYaw, cameraPitch, cameraRoll);

								m_initialWorldViewerPosSet = true;
							}

							if (quadTree->statusRefreshTerrainDataInProgress())
								Globals()->debugPrint((std::string("Refreshed quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " " + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
							
							if (refreshingQuadrant)
							{
								Chunk::TerrainHeightsChangedChunkAction action;
								quadTree->ForAllChunk(action, QuadType::NotSet);
							}
							
							quadTree->getQuadrant()->mapModderWorkDone(false);

							quadTree->setStatus(QuadrantStatus::initialized);

							quadTree->getQuadrant()->unlockInternalData();
						}
					}
				}
				else
				{
					m_mtxQuadTreeAndMainProcessing.unlock();

					m_mapQuadrantsMap.lock();
					
					if (m_mapQuadrantsMap.contains(quadrantPos))
					{
						MapQuadrant* mapQuadrant = m_mapQuadrantsMap[quadrantPos].get();

						std::string outMeshIdFromBuffer;
						mapQuadrant->refreshDataFromServer(buffGridVerticesFromServer, meshIdFromServer, outMeshIdFromBuffer);
						m_mapQuadrantsMap.addUpdatedQuadrant(quadrantPos);
					}
					
#pragma warning(suppress: 26117)
					m_mapQuadrantsMap.unlock();
				}
			}
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl != nullptr && editModeUIControl->initialized())
				editModeUIControl->replyFromServer(reply);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 0.2 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL);

			ClientServerVariant v = reply.getInputParam(0);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a int as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(3);
			const auto _setCamera(std::get_if<bool>(&v));
			if (_setCamera == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a bool as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(4);
			const auto _cameraPosX(std::get_if<float>(&v));
			if (_cameraPosX == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosX = *_cameraPosX;

			v = reply.getInputParam(5);
			const auto _cameraPosY(std::get_if<float>(&v));
			if (_cameraPosY == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosY = *_cameraPosY;

			v = reply.getInputParam(6);
			const auto _cameraPosZ(std::get_if<float>(&v));
			if (_cameraPosZ == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as seventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosZ = *_cameraPosZ;

			v = reply.getInputParam(7);
			const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrainForced == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eight input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

			v = reply.getInputParam(8);
			const auto _cameraYaw(std::get_if<float>(&v));
			if (_cameraYaw == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as ninth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraYaw = *_cameraYaw;

			v = reply.getInputParam(9);
			const auto _cameraPitch(std::get_if<float>(&v));
			if (_cameraPitch == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPitch = *_cameraPitch;

			v = reply.getInputParam(10);
			const auto _cameraRoll(std::get_if<float>(&v));
			if (_cameraRoll == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eleventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraRoll = *_cameraRoll;

			TheWorld_Utils::QuadrantPos quadrantPos(cameraPosX, cameraPosZ, level, (int)numVerticesPerSize, gridStepInWU);
			quadrantPos.setTag("Camera");

			size_t maxThreads;
			while (m_tpClientWorker.getNumWorkingThreads(maxThreads) > 0)
			{
				Sleep(1);
			}

			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

			//clearQuadTreeMap();	// we almost should clear only current level quadrants
			m_worldQuadrantsMap[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
			m_worldQuadrantsMap[quadrantPos]->requestInitDelayed(cameraPosX, cameraPosY, cameraPosZ, cameraYaw, cameraPitch, cameraRoll, true, cameraDistanceFromTerrainForced);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 0.1 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD);

			ClientServerVariant v = reply.getInputParam(0);
			const auto _level(std::get_if<int>(&v));
			if (_level == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a int as first input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			int level = *_level;

			v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(3);
			const auto _setCamera(std::get_if<bool>(&v));
			if (_setCamera == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a bool as forth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			bool setCamera = *_setCamera;

			v = reply.getInputParam(4);
			const auto _cameraPosX(std::get_if<float>(&v));
			if (_cameraPosX == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as fifth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosX = *_cameraPosX;

			v = reply.getInputParam(5);
			const auto _cameraPosY(std::get_if<float>(&v));
			if (_cameraPosY == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as sixth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosY = *_cameraPosY;

			v = reply.getInputParam(6);
			const auto _cameraPosZ(std::get_if<float>(&v));
			if (_cameraPosZ == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as seventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPosZ = *_cameraPosZ;

			v = reply.getInputParam(7);
			const auto _cameraDistanceFromTerrainForced(std::get_if<float>(&v));
			if (_cameraDistanceFromTerrainForced == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eight input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraDistanceFromTerrainForced = *_cameraDistanceFromTerrainForced;

			v = reply.getInputParam(8);
			const auto _cameraYaw(std::get_if<float>(&v));
			if (_cameraYaw == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as ninth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraYaw = *_cameraYaw;

			v = reply.getInputParam(9);
			const auto _cameraPitch(std::get_if<float>(&v));
			if (_cameraPitch == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as tenth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraPitch = *_cameraPitch;

			v = reply.getInputParam(10);
			const auto _cameraRoll(std::get_if<float>(&v));
			if (_cameraRoll == NULL)
			{
				std::string m = std::string("Reply MapManager::deployWorld did not return a float as eleventh input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float cameraRoll = *_cameraRoll;

			m_worldQuadrantsMapValid = true;

			Globals()->Client()->MapManagerDeployLevel(isInEditor(), level, numVerticesPerSize, gridStepInWU, setCamera, cameraPosX, cameraPosY, cameraPosZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD)
		{
			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

				for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;

					itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
					//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
				}
			}

			enum class TheWorldStatus status = Globals()->status();

			while (Globals()->Client()->getNumCallbacksRunning() > 1)
			{
				Sleep(1);
			}
			
			size_t maxThreads;
			while (m_tpClientWorker.getNumWorkingThreads(maxThreads) > 0)
			{
				Sleep(1);
			}

			while (m_editModeUIControl != nullptr && m_editModeUIControl->getNumWorkingThread() > 0)
			{
				m_editModeUIControl->requestedQuit();
				Sleep(1);
			}

			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing); 
				clearQuadTreeMap();
			}

			if (Globals()->connectedToServer() && m_initialized)
				Globals()->setStatus(TheWorldStatus::sessionInitialized);
			else if (Globals()->connectedToServer())
				Globals()->setStatus(TheWorldStatus::connectedToServer);
			else if (Globals()->initialized())
				Globals()->setStatus(TheWorldStatus::initialized);
			else
				Globals()->setStatus(TheWorldStatus::uninitialized);

			hudMessage("", 0);
			if (m_hudStreamingInfoLabel != nullptr)
			{
				m_hudStreamingInfoLabel->set_text("");
			}
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC)
		{
			ClientServerVariant v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepinWU(std::get_if<float>(&v));
			if (_gridStepinWU == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepinWU = *_gridStepinWU;

			v = reply.getInputParam(3);
			std::vector<unsigned char>* _clientQuadrants = std::get_if<std::vector<unsigned char>>(&v);
			if (_clientQuadrants == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as fourth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string clientQuadrants((*_clientQuadrants).begin(), (*_clientQuadrants).end());

			v = reply.getReplyParam(0);
			std::vector<unsigned char>* _replyClientQuadrants = std::get_if<std::vector<unsigned char>>(&v);
			if (_replyClientQuadrants == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string replyClientQuadrants((*_replyClientQuadrants).begin(), (*_replyClientQuadrants).end());

			v = reply.getReplyParam(1);
			std::vector<unsigned char>* _replyProceGenStatus = std::get_if<std::vector<unsigned char>>(&v);
			if (_replyProceGenStatus == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as second return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string replyProceGenStatus((*_replyProceGenStatus).begin(), (*_replyProceGenStatus).end());

			v = reply.getReplyParam(2);
			std::vector<unsigned char>* _replyExtraQuadrants = std::get_if<std::vector<unsigned char>>(&v);
			if (_replyExtraQuadrants == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string replyExtraQuadrants((*_replyExtraQuadrants).begin(), (*_replyExtraQuadrants).end());

			if (clientQuadrants.length() > 0 && replyClientQuadrants.length() > 0)
			{
				size_t size = 0;

				//size_t l1 = clientQuadrants.size();	// DEBUG
				//size_t l2 = replyQuadrants.size();	// DEBUG
				size_t clientQuadrantsCursor = 1;	// bypassing first byte '0'
				size_t numClientQuadrants = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)clientQuadrants.c_str() + clientQuadrantsCursor, size);
				clientQuadrantsCursor += size;

				size_t replyCursor = 1;				// bypassing first byte '0'
				size_t numReplyQuadrants = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)replyClientQuadrants.c_str() + replyCursor, size);
				replyCursor += size;

				if (numClientQuadrants != numReplyQuadrants)
					my_assert(numClientQuadrants == numReplyQuadrants);

				if (numClientQuadrants > 0)
				{
					for (size_t idx = 0; idx < numClientQuadrants; idx++)
					{
						size_t quadrantPosSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)clientQuadrants.c_str() + clientQuadrantsCursor, size);
						clientQuadrantsCursor += size;

						TheWorld_Utils::MemoryBuffer tmpBuffer((BYTE*)clientQuadrants.c_str() + clientQuadrantsCursor, quadrantPosSize);
						clientQuadrantsCursor += quadrantPosSize;

						TheWorld_Utils::QuadrantPos pos;
						pos.deserialize(tmpBuffer);

						size_t meshIdSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)clientQuadrants.c_str() + clientQuadrantsCursor, size);
						clientQuadrantsCursor += size;

						std::string clientMeshId((char*)clientQuadrants.c_str() + clientQuadrantsCursor, meshIdSize);
						clientQuadrantsCursor += meshIdSize;

						bool quadrantRefreshNeeded = TheWorld_Utils::deserializeFromByteStream<bool>((BYTE*)replyClientQuadrants.c_str() + replyCursor, size);
						replyCursor += size;

						if (quadrantRefreshNeeded)
						{
							std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

							if (m_worldQuadrantsMap.contains(pos))
							{
								QuadTree* quadTree = m_worldQuadrantsMap[pos].get();
								if (quadTree->isValid() && quadTree->getQuadrant()->getPos().isInitialized() && quadTree->getQuadrant()->mapModderWorkDone())
									quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
							}
						}
					}
				}
			}

			if (clientQuadrants.length() > 0)
				m_quadrantSyncRunning = false;

			{
				GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
				if (replyProceGenStatus.length() > 0)
				{
					size_t size = 0;
					size_t proceGenStatusCursor = 0;
					size_t numProceGenStatus = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)replyProceGenStatus.c_str() + proceGenStatusCursor, size);
					proceGenStatusCursor += size;
					if (numProceGenStatus > 0)
					{
						for (int idx = 0; idx < numProceGenStatus; idx++)
						{
							TheWorld_Utils::MapManagerProceduralGenerationStatus procGenStatus = TheWorld_Utils::deserializeFromByteStream<TheWorld_Utils::MapManagerProceduralGenerationStatus>((BYTE*)replyProceGenStatus.c_str() + proceGenStatusCursor, size);
							proceGenStatusCursor += size;
							if (editModeUIControl != nullptr && editModeUIControl->initialized())
								editModeUIControl->setServerProcGenStatus(procGenStatus);
						}
					}
					else
						if (editModeUIControl != nullptr && editModeUIControl->initialized())
							editModeUIControl->resetServerProcGenStatus();
				}
				else
					if (editModeUIControl != nullptr && editModeUIControl->initialized())
						editModeUIControl->resetServerProcGenStatus();
			}

			{
				//size_t l = replyExtraQuadrants.size();	// DEBUG
				if (replyExtraQuadrants.length() > 0 && isEditMode())
				{
					size_t size = 0;
					size_t extraQuadrantsCursor = 0;
					size_t numExtraQuadrants = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)replyExtraQuadrants.c_str() + extraQuadrantsCursor, size);
					extraQuadrantsCursor += size;

					for (int idx = 0; idx < numExtraQuadrants; idx++)
					{
						size_t posSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)replyExtraQuadrants.c_str() + extraQuadrantsCursor, size);
						extraQuadrantsCursor += size;

						TheWorld_Utils::MemoryBuffer tmpBuffer((BYTE*)replyExtraQuadrants.c_str() + extraQuadrantsCursor, posSize);
						extraQuadrantsCursor += posSize;

						TheWorld_Utils::QuadrantPos pos;
						pos.deserialize(tmpBuffer);

						std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
						
						if (m_worldQuadrantsMap.contains(pos))
						{
							QuadTree* quadTree = m_worldQuadrantsMap[pos].get();
							if (quadTree->isValid() && quadTree->getQuadrant()->getPos().isInitialized() && quadTree->getQuadrant()->mapModderWorkDone())
							{
								quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
								{	// DEBUG
									//"TRACKING 
									//if (quadTree->getQuadrant()->getTerrainEdit()->terrainType == TheWorld_Utils::TerrainEdit::TerrainType::empty)
									//	if (quadTree->getQuadrant()->getTerrainEdit()->heightsNeedRegen)
									//		quadTree->getQuadrant()->getTerrainEdit()->heightsNeedRegen = true;
									//	else
									//		quadTree->getQuadrant()->getTerrainEdit()->heightsNeedRegen = false;
								}	// DEBUG
							}
						}
						else
						{
							if (!m_extraQuadsMap.contains(pos) && Globals()->status() == TheWorldStatus::worldDeployed)
							{
								//if (pos.getIndexPosX() == 93 && pos.getIndexPosZ() == 99)
								//	PLOG_DEBUG << "Quadrant added to m_extraQuadsMap";
								m_extraQuadsMap[pos] = make_unique<QuadTree>(this, pos);
								float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
								m_extraQuadsMap[pos]->requestInitDelayed(x, y, z, yaw, pitch, roll, false, 0.0f);
							}
						}
					}
				}
			}
		}
		else
		{
			Globals()->setStatus(TheWorldStatus::error);
			Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: unknow method ") + method).c_str());
		}
	}
	catch (GDN_TheWorld_Exception ex)
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
	}
	catch (...)
	{
		Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint("GDN_TheWorld_Globals::replyFromServer: exception caught");
	}
}

void GDN_TheWorld_Viewer::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//Godot::print("GDN_TheWorld_Viewer::_init");

	if (VISUAL_SERVER_WIREFRAME_MODE)
		godot::RenderingServer::get_singleton()->set_debug_generate_wireframes(true);

	set_name(THEWORLD_WORLD_VIEWER_NODE_NAME);
}

void GDN_TheWorld_Viewer::_ready(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("GDN_TheWorld_Viewer::_ready");

	m_ready = true;
}

bool GDN_TheWorld_Viewer::setWorldCameraWorldPos(godot::Vector2 mapPos, float altitudeFromTerrain, bool altitudeIsAbsolute, godot::Transform3D worldCameraWorldTransform)
{
	bool ret = false;
	GDN_TheWorld_Globals* globals = Globals();

	if (Globals()->status() == TheWorldStatus::worldDeployed)
	{
		// get quadrant containing requested pos
		TheWorld_Utils::QuadrantPos quadrantPos(mapPos.x, mapPos.y, m_worldViewerLevel, (int)Globals()->numVerticesPerQuadrantSize(), globals->gridStepInWU());
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		if (m_worldQuadrantsMap.contains(quadrantPos))
		{
			QuadTree* quadTree = m_worldQuadrantsMap[quadrantPos].get();
			if (quadTree->statusInitialized() and !quadTree->colliderNeedReset(QuadType::Terrain))
			{
#define MAX_ALTITUDE_DETECTED		20000

				float cameraAltitude = -999999999.9f;
				godot::Vector3 globalPosGroundZero = worldPosToGlobalPos(godot::Vector3(mapPos.x, 0, mapPos.y));

				if (altitudeIsAbsolute)
				{
					cameraAltitude = altitudeFromTerrain;
				}
				else
				{
					godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
					godot::Dictionary rayArray;
					godot::Vector3 rayOrigin = globalPosGroundZero;
					godot::Vector3 rayEnd = rayOrigin + godot::Vector3(0.0f, MAX_ALTITUDE_DETECTED, 0.0f);
					godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
					rayArray = spaceState->intersect_ray(query);
					if (rayArray.has("position"))
					{
						godot::Vector3 terrainCollision = rayArray["position"];
						cameraAltitude = terrainCollision.y + altitudeFromTerrain;
					}
					else
					{
						godot::Dictionary rayArray;
						rayEnd = rayOrigin + godot::Vector3(0.0f, -MAX_ALTITUDE_DETECTED, 0.0f);
						godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
						rayArray = spaceState->intersect_ray(query);
						if (rayArray.has("position"))
						{
							godot::Vector3 terrainCollision = rayArray["position"];
							cameraAltitude = terrainCollision.y + altitudeFromTerrain;
						}
						else
							cameraAltitude = MAX_ALTITUDE_DETECTED;
					}
				}


				godot::Camera3D* activeWorldCamera = getWorldCamera2(true);
				if (activeWorldCamera != nullptr)
				{
					activeWorldCamera->set_global_transform(worldCameraWorldTransform);
					activeWorldCamera->set_global_position(godot::Vector3(globalPosGroundZero.x, cameraAltitude, globalPosGroundZero.z));
					//GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
					//if (cam != nullptr)
					//{
					//	godot::Vector3 worldCameraMapPos(mapPosX, cameraAltitude, mapPosZ);
					//	cam->setCameraMapPos(worldCameraMapPos);
					//}
				}
			}
		}
	}

	return ret;
}

//void GDN_TheWorld_Viewer::toggleFullScreen(void)
//{
//}

void GDN_TheWorld_Viewer::toggleQuadrantSelected(void)
{
	if (trackMouse())
	{
		m_mouseQuadrantHitName = "";

		QuadTree* quadTreeSel = nullptr;
		if (m_quadrantSelPos.empty())
		{
			if (!m_quadrantHitPos.empty())
			{
				m_quadrantSelPos = m_quadrantHitPos;

				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
					WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(m_quadrantSelPos);
					if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
					{
						quadTreeSel = it->second.get();
						it->second->setEditModeQuadrantSelected(true);
						it->second->materialParamsNeedReset(true);
						call_deferred("emit_signal", "tw_quadrant_selected_for_edit", true);
					}
				}
			}
		}
		else
		{
			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
				WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(m_quadrantSelPos);
				if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
				{
					it->second->setEditModeQuadrantSelected(false);
					it->second->materialParamsNeedReset(true);
					call_deferred("emit_signal", "tw_quadrant_selected_for_edit", false);
				}
			}

			m_quadrantSelPos.reset();
		}

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl == nullptr || !editModeUIControl->initialized())
		{
			createEditModeUI();
			editModeUIControl = EditModeUIControl();
			editModeUIControl->set_visible(false);
		}
		if (editModeUIControl != nullptr /* && m_editMode*/)
		{
			if (m_quadrantSelPos.empty())
			{
				editModeUIControl->setMouseQuadSelLabelText("");
				editModeUIControl->setMouseQuadSelPosLabelText("");
				editModeUIControl->setEmptyTerrainEditValues();
				editModeUIControl->setElapsed(0, false);
			}
			else
			{
				editModeUIControl->setMouseQuadSelLabelText(m_quadrantSelPos.getName() + " " + m_quadrantSelPos.getTag());
				editModeUIControl->setMouseQuadSelPosLabelText(std::string("X=") + std::format("{:.2f}", m_quadrantSelPos.getLowerXGridVertex()) + " Z=" + std::format("{:.2f}", m_quadrantSelPos.getLowerZGridVertex()) + " " + std::format("{:.2f}", m_quadrantSelPos.getSizeInWU()));
				if (quadTreeSel != nullptr)
				{
					TheWorld_Utils::TerrainEdit* terrainEdit = quadTreeSel->getQuadrant()->getTerrainEdit();

					TheWorld_Utils::TerrainEdit* northSideTerrainEdit = nullptr;
					TheWorld_Utils::QuadrantPos pos = m_quadrantSelPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
					QuadTree* q = getQuadTree(pos);
					if (q != nullptr && !q->getQuadrant()->empty())
						northSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
					TheWorld_Utils::TerrainEdit* southSideTerrainEdit = nullptr;
					pos = m_quadrantSelPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
					q = getQuadTree(pos);
					if (q != nullptr && !q->getQuadrant()->empty())
						southSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
					TheWorld_Utils::TerrainEdit* westSideTerrainEdit = nullptr;
					pos = m_quadrantSelPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
					q = getQuadTree(pos);
					if (q != nullptr && !q->getQuadrant()->empty())
						westSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
					TheWorld_Utils::TerrainEdit* eastSideTerrainEdit = nullptr;
					pos = m_quadrantSelPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
					q = getQuadTree(pos);
					if (q != nullptr && !q->getQuadrant()->empty())
						eastSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
					terrainEdit->adjustValues(northSideTerrainEdit, southSideTerrainEdit, westSideTerrainEdit, eastSideTerrainEdit);

					editModeUIControl->setTerrainEditValues(*terrainEdit);
				}
			}
		}
	}
}

void GDN_TheWorld_Viewer::_input(const Ref<InputEvent>& event)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	bool mouseEvent = false;

	const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
	if (eventMouseButton != nullptr)
	{
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT && eventMouseButton->is_pressed())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_BUTTON_LEFT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT && eventMouseButton->is_double_click())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_LEFT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_RIGHT && eventMouseButton->is_pressed())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_BUTTON_RIGHT);
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_RIGHT && eventMouseButton->is_double_click())
			m_trackedInputEvents.push_back(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_RIGHT);
		mouseEvent = true;
	}

	/* DEBUG */
	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//if (eventMouseMotion != nullptr)
	//{
	//	mouseEvent = true;
	//}
	//if (!mouseEvent)
	//	mouseEvent = false;
	//const godot::InputEventKey* eventKey = godot::Object::cast_to<godot::InputEventKey>(event.ptr());
	//if (eventKey != nullptr)
	//{
	//	bool b = true;
	//}
	/* DEBUG */

	//if (event->is_action_pressed("ui_toggle_fullscreen"))
	//{
	//	toggleFullScreen();
	//}

	if (event->is_action_pressed("ui_select") && m_altPressed)
	{
		toggleQuadrantSelected();
	}

	if (event->is_action_pressed("ui_toggle_track_mouse"))
	{
		toggleTrackMouse();
	}

	if (event->is_action_pressed("ui_toggle_edit_mode"))
	{
		toggleEditMode();
	}

	if (event->is_action_pressed("ui_toggle_debug_visibility"))
	{
		toggleDebugVisibility();
	}
	
	if (event->is_action_pressed("ui_rotate_chunk_debug_mode"))
	{
		rotateChunkDebugMode();
	}
	if (event->is_action_pressed("ui_toggle_wireframe"))
	{
		toggleWireframe();
	}

	if (event->is_action_pressed("ui_rotate_drawing_mode"))
	{
		rotateDrawingMode();
	}

	if (event->is_action_pressed("ui_dump") && !m_ctrlPressed)
	{
		m_dumpRequired = true;
	}

	if (event->is_action_pressed("ui_refresh"))
	{
		m_refreshRequired = true;
	}

	if (event->is_action_pressed("ui_toggle_satellite_view"))
	{
		if (m_satelliteSubViewportContainer != nullptr)
		{
			m_satelliteSubViewportContainer->set_visible(!m_satelliteSubViewportContainer->is_visible());
		}
	}

	if (event->is_action_pressed("ui_toggle_back_mirror"))
	{
		if (m_backMirrorSubViewportContainer != nullptr)
		{
			m_backMirrorSubViewportContainer->set_visible(!m_backMirrorSubViewportContainer->is_visible());
		}
	}

	if (event->is_action_pressed("ui_toggle_map"))
	{
		toggleMapView();
	}
}

void GDN_TheWorld_Viewer::toggleMapView(void)
{
	GDN_TheWorld_MapViewer* mapViewer = MapViewer();
	if (mapViewer != nullptr)
	{
		if (mapMode())
		{
			Camera2D* mapCamera = MapCamera();
			if (mapCamera != nullptr)
			{
				m_mapCameraGlobalPos = mapCamera->get_global_position();
				mapCameraGlobalPositionSet = true;

				//// SWITCHING TO WORLD MODE: world camera will position on map camera position
				//{
				//	setWorldCameraWorldPos(globalPosToMapPos(mapCameraGlobalPos), m_altitudeForToggleToWorldMode, m_altitudeForToggleToWorldModeIsAbsolute, m_worldCameraWorldTransformForToggleToWorldMode);
				//}

				// SWITCHING TO WORLD MODE: world camera will not move (remaining where it was when map mode was activated)
				{
					godot::Vector2 mapPos(m_worldCameraWorldTransformForToggleToWorldMode.origin.x, m_worldCameraWorldTransformForToggleToWorldMode.origin.z);
					setWorldCameraWorldPos(mapPos, m_altitudeForToggleToWorldMode, m_altitudeForToggleToWorldModeIsAbsolute, m_worldCameraWorldTransformForToggleToWorldMode);
				}

				mapMode(false);
				mapCamera->set_enabled(false);
				mapViewer->set_visible(false);
				//mapViewer->set_visible(!mapViewer->is_visible());
			}
		}
		else
		{
			Camera3D* worldCamera = getWorldCamera();
			Camera2D* mapCamera = MapCamera();
			if (worldCamera != nullptr and mapCamera != nullptr)
			{
				GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(worldCamera);
				if (cam != nullptr)
				{
					GDN_TheWorld_Map* map = Map();
					if (map != nullptr)
					{
						if (map->getImageSize() != godot::Vector2i())
						{
							godot::Vector3 worldCameraWorldPos = cam->getWorldPos();
							
							// SWITCHING TO MAP MODE: on first switch map camera will be positioned on world camera position on next switch will stay where it was when map mode was quit
							if (not mapCameraGlobalPositionSet)
							{
								m_mapCameraGlobalPos = mapPosToGlobalPos(godot::Vector2(worldCameraWorldPos.x, worldCameraWorldPos.z));
							}
							mapCamera->set_global_position(m_mapCameraGlobalPos);
							m_worldCameraWorldTransformForToggleToWorldMode = cam->getWorldTransform();
							if (cam->cameraAltitudeFromTerrainValid())
							{
								m_altitudeForToggleToWorldMode = cam->cameraAltitudeFromTerrain();
								m_altitudeForToggleToWorldModeIsAbsolute = false;
							}
							else
							{
								m_altitudeForToggleToWorldMode = worldCameraWorldPos.y;
								m_altitudeForToggleToWorldModeIsAbsolute = true;
							}
							mapMode(true);
							mapCamera->set_enabled(true);
							mapViewer->set_visible(true);
							//mapViewer->set_visible(!mapViewer->is_visible());
						}
					}
				}
			}

			GDN_TheWorld_Map*map = Map();
			if (map != nullptr)
			{
				map->setWorldFlag("WorldCamera", m_worldCameraWorldTransformForToggleToWorldMode, GDN_TheWorld_Globals::g_color_red, true);
			}
		}
	}
}

bool GDN_TheWorld_Viewer::mapMode(void) const
{
	//bool _mapMode = false;
	//GDN_TheWorld_MapViewer* mapViewer = MapViewer();
	//if (mapViewer != nullptr and mapViewer->is_visible())
	//	_mapMode = true;
	//return _mapMode;
	return m_mapMode;
}

void GDN_TheWorld_Viewer::mapMode(bool b)
{
	m_mapMode = b;
}

bool GDN_TheWorld_Viewer::worldMode(void) const
{
	return !mapMode();
}

void GDN_TheWorld_Viewer::toggleTrackMouse(void)
{
	if (!m_editMode)
	{
		if (trackMouse())
			trackMouse(false);
		else
			trackMouse(true);
	}
	else
		m_previousTrackedMouseHitInMap = godot::Vector3();

	if (trackMouse())
		m_mouseQuadrantHitName = "";
}

bool GDN_TheWorld_Viewer::trackMouse(void) const
{
	return m_trackMouse;
}

void GDN_TheWorld_Viewer::trackMouse(bool b)
{
	m_trackMouse = b;
	if (!m_trackMouse)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		for (auto& it : m_worldQuadrantsMap)
		{
			it.second->mouseHitOnTerrainChanged(godot::Vector3(), false);
		}
		m_normalDrawer->set_visible(false);
		m_positionDrawer->set_visible(false);
		m_gizmo->set_visible(false);

		m_mouseQuadrantHitName = "";
		m_mouseQuadrantHitTag = "";
		m_mouseQuadrantHitPos = godot::Vector3();
		m_mouseQuadrantHitSize = 0.0f;
		m_quadrantHitPos = TheWorld_Utils::QuadrantPos();
		m_quadrantHit = nullptr;
		m_adjacentQuadrantsHit.clear();
		m_previousTrackedMouseHitInMap = godot::Vector3();
	}
}

void GDN_TheWorld_Viewer::toggleEditMode(void)
{
	m_previousTrackedMouseHitInMap = godot::Vector3();
	m_editMode = !m_editMode;
	if (m_editMode)
	{
		trackMouse(true);
	}
	else
	{
		if (!positionVisible() && !normalVisible() && !gizmoVisible())
			trackMouse(false);
		//gridOn(false);
	}
}

void GDN_TheWorld_Viewer::setEditMode(bool editMode)
{
	if (m_editMode != editMode)
	{
		m_previousTrackedMouseHitInMap = godot::Vector3();
		m_editMode = editMode;
		if (m_editMode)
		{
			trackMouse(true);
		}
		else
		{
			if (!positionVisible() && !normalVisible() && !gizmoVisible())
				trackMouse(false);
			//gridOn(false);
		}
	}
}

bool GDN_TheWorld_Viewer::isEditMode(void) const
{
	return m_editMode;
}

void GDN_TheWorld_Viewer::toggleDebugVisibility(void)
{
	m_debugContentVisibility = !m_debugContentVisibility;
	m_updateTerrainVisibilityRequired = true;
	forceProcessCameraPosForChunks();
}

void GDN_TheWorld_Viewer::rotateChunkDebugMode(void)
{
	m_requiredChunkDebugMode = GDN_TheWorld_Globals::rotateChunkDebugMode(m_currentChunkDebugMode);
	m_updateDebugModeRequired = true;
}

enum class GDN_TheWorld_Globals::ChunkDebugMode GDN_TheWorld_Viewer::getChunkDebugMode(void) const
{
	return m_currentChunkDebugMode;
}

void GDN_TheWorld_Viewer::rotateDrawingMode(void)
{
	godot::Viewport* vp = get_viewport();
	godot::Viewport::DebugDraw dd = vp->get_debug_draw();
	vp->set_debug_draw( (godot::Viewport::DebugDraw)((dd + 1) % 26));
	m_debugDraw = vp->get_debug_draw();
}

void GDN_TheWorld_Viewer::toggleWireframe(void) const
{
	godot::Viewport* vp = get_viewport();
	godot::Viewport::DebugDraw dd = vp->get_debug_draw();
	if (dd == godot::Viewport::DebugDraw::DEBUG_DRAW_WIREFRAME)
		vp->set_debug_draw(godot::Viewport::DebugDraw::DEBUG_DRAW_DISABLED);
	else
		vp->set_debug_draw(godot::Viewport::DebugDraw::DEBUG_DRAW_WIREFRAME);
}

void GDN_TheWorld_Viewer::printKeyboardMapping(void)
{
	Globals()->print("");
	Globals()->print("===========================================================================");
	Globals()->print("");
	Globals()->print("KEYBOARD MAPPING");
	Globals()->print("");
	Globals()->print("LEFT						==> A	MOUSE BUTTON LEFT");
	Globals()->print("RIGHT						==> D	MOUSE BUTTON LEFT");
	Globals()->print("FORWARD					==>	W	MOUSE BUTTON MID	MOUSE WHEEL UP (WITH ALT MOVE ON MAP, WITHOUT MOVE FORWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("BACKWARD					==>	S	MOUSE BUTTON MID	MOUSE WHEEL DOWN (WITH ALT MOVE ON MAP, WITHOUT MOVE BACKWARD IN THE DIRECTION OF CAMERA)");
	Globals()->print("UP						==>	Q");
	Globals()->print("DOWN						==>	E");
	Globals()->print("");
	Globals()->print("SHIFT						==>	LOW MOVEMENT");
	Globals()->print("CTRL						==> FAST MOVEMENT");
	Globals()->print("");
	Globals()->print("ROTATE CAMERA				==>	MOUSE BUTTON RIGHT");
	Globals()->print("SHIFT ORI CAMERA			==>	MOUSE BUTTON LEFT");
	Globals()->print("SHIFT VERT CAMERA			==>	MOUSE BUTTON MID");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG STATS		==>	CTRL-ALT-D");
	Globals()->print("TOGGLE EDIT				==>	ALT-E");
	Globals()->print("DUMP						==>	ALT-D");
	Globals()->print("");
	Globals()->print("TOGGLE DEBUG VISIBILITY	==>	F1");
	Globals()->print("ROTATE CHUNK DEBUG MODE	==>	F2");
	Globals()->print("ROTATE DRAWING MODE		==>	F3");
	Globals()->print("REFRESH VIEW				==>	F4");
	Globals()->print("");
	Globals()->print("EXIT						==>	ESC");
	Globals()->print("");
	Globals()->print("===========================================================================");
	Globals()->print("");
}

void GDN_TheWorld_Viewer::_notification(int p_what)
{
	//GDN_TheWorld_Globals* globals = Globals();
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
						globals->debugPrint("GDN_TheWorld_Viewer::_notification (NOTIFICATION_PREDELETE) - Destroy Viewer", false);
					else
						globals->debugPrint("GDN_TheWorld_Viewer::_notification (NOTIFICATION_PREDELETE) - Destroy Viewer");
			}
	
			deinit();
		}
		break;
		case NOTIFICATION_ENTER_WORLD:
		{
			m_worldEntered = true;

			//GDN_TheWorld_Globals* globals = Globals();
			//printKeyboardMapping();
			//string s = "Use Visual Server: "; s += (m_useVisualServer ? "True" : "False");
			//if (globals != nullptr)
			//{
			//	globals->infoPrint(s.c_str());
			//	globals->debugPrint("Enter world");
			//}
			////if (m_initialWorldViewerPosSet)
			////{
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			//for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			//{
			//	if (!itQuadTree->second->isValid())
			//		continue;

			//	Chunk::EnterWorldChunkAction action;
			//	itQuadTree->second->ForAllChunk(action);
			//	
			//	itQuadTree->second->getQuadrant()->getCollider()->enterWorld();
			//}
			////}
		}
		break;
		case NOTIFICATION_EXIT_WORLD:
		{
			//GDN_TheWorld_Globals* globals = Globals();
			//if (globals != nullptr)
			//	globals->debugPrint("Exit world");
			////if (m_initialWorldViewerPosSet)
			////{
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			//for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			//{
			//	if (!itQuadTree->second->isValid())
			//		continue;

			//	Chunk::ExitWorldChunkAction action;
			//	itQuadTree->second->ForAllChunk(action);
			//	
			//	itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
			//	//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
			//}
			////}

			m_worldEntered = false;
		}
		break;
		case NOTIFICATION_VISIBILITY_CHANGED:
		{
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				globals->debugPrint("Visibility changed");
			//if (m_initialWorldViewerPosSet)
			//{
			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
			for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			{
				if (!itQuadTree->second->isValid())
					continue;

				if (!itQuadTree->second->isVisible())
					continue;

				Chunk::VisibilityChangedChunkAction action(is_visible_in_tree(), globals);
				itQuadTree->second->ForAllChunk(action, QuadType::NotSet);
			}
			//}
		}
		break;
		case NOTIFICATION_TRANSFORM_CHANGED:
		{
			onTransformChanged();
		}
		break;
		//case NOTIFICATION_WM_SIZE_CHANGED:
		//{
		//	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		//	if (editModeUIControl != nullptr && editModeUIControl->initialized())
		//		editModeUIControl->controlNeedResize();
		//	setPosHud();
		//}
		//break;
	}
}

void GDN_TheWorld_Viewer::viewportSizeChanged(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initialized())
		editModeUIControl->controlNeedResize();

	godot::Size2 windowSize = getViewportSize();

	setPosWorldViewer(windowSize);

	setPosHud(windowSize);

	setPosMapViewer(windowSize);
}

String GDN_TheWorld_Viewer::getDebugDrawMode(void) const
{
	if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DISABLED)
		return "DEBUG_DRAW_DISABLED";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_UNSHADED)
		return "DEBUG_DRAW_UNSHADED";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_LIGHTING)
		return "DEBUG_DRAW_LIGHTING";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_OVERDRAW)
		return "DEBUG_DRAW_OVERDRAW";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_WIREFRAME)
		return "DEBUG_DRAW_WIREFRAME";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_NORMAL_BUFFER)
		return "DEBUG_DRAW_NORMAL_BUFFER";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_ALBEDO)
		return "DEBUG_DRAW_VOXEL_GI_ALBEDO";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_LIGHTING)
		return "DEBUG_DRAW_VOXEL_GI_LIGHTING";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_VOXEL_GI_EMISSION)
		return "DEBUG_DRAW_VOXEL_GI_EMISSION";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SHADOW_ATLAS)
		return "DEBUG_DRAW_SHADOW_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS)
		return "DEBUG_DRAW_DIRECTIONAL_SHADOW_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SCENE_LUMINANCE)
		return "DEBUG_DRAW_SCENE_LUMINANCE";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SSAO)
		return "DEBUG_DRAW_SSAO";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SSIL)
		return "DEBUG_DRAW_SSIL";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_PSSM_SPLITS)
		return "DEBUG_DRAW_PSSM_SPLITS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DECAL_ATLAS)
		return "DEBUG_DRAW_DECAL_ATLAS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SDFGI)
		return "DEBUG_DRAW_SDFGI";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_SDFGI_PROBES)
		return "DEBUG_DRAW_SDFGI_PROBES";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_GI_BUFFER)
		return "DEBUG_DRAW_GI_BUFFER";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_DISABLE_LOD)
		return "DEBUG_DRAW_DISABLE_LOD";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_OMNI_LIGHTS)
		return "DEBUG_DRAW_CLUSTER_OMNI_LIGHTS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_SPOT_LIGHTS)
		return "DEBUG_DRAW_CLUSTER_SPOT_LIGHTS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_DECALS)
		return "DEBUG_DRAW_CLUSTER_DECALS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_CLUSTER_REFLECTION_PROBES)
		return "DEBUG_DRAW_CLUSTER_REFLECTION_PROBES";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_OCCLUDERS)
		return "DEBUG_DRAW_OCCLUDERS";
	else if (m_debugDraw == Viewport::DebugDraw::DEBUG_DRAW_MOTION_VECTORS)
		return "DEBUG_DRAW_MOTION_VECTORS";
	else
		return std::to_string(int(m_debugDraw)).c_str();
}

String GDN_TheWorld_Viewer::getChunkDebugModeStr(void) const
{
	if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet)
		return "CHUNK_DEBUG_MODE_DO_NOT_SET";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
		return "CHUNK_DEBUG_MODE_NO_DEBUG";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
		return "CHUNK_DEBUG_MODE_WIREFRAME_AABB";
	else if (m_currentChunkDebugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		return "CHUNK_DEBUG_MODE_WIREFRAME_SQUARE";
	else
		return "";
}

void GDN_TheWorld_Viewer::setMinDepthQuadOnPerimeter(int depth)
{
	if (depth >= 0 && depth <= MAX_SETTABLE_DEPTH_ON_PERIMETER)
	{
		m_minDepthQuadOnPerimeter = depth;
		if (m_minDepthQuadOnPerimeter != -1 and m_minDepthQuadOnPerimeter > getMaxDepthQuadOnPerimeter())
			setMaxDepthQuadOnPerimeter(m_minDepthQuadOnPerimeter);
		m_lastProcessedCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			m_recalcQuadrantsInViewNeeded = true;
	}
}

int GDN_TheWorld_Viewer::getMinDepthQuadOnPerimeter(void) const
{
	return (int)m_minDepthQuadOnPerimeter;
}

void GDN_TheWorld_Viewer::setCacheQuadOnPerimeter(int cache)
{
	if (cache >= 0 && cache <= MAX_SETTABLE_CACHE_ON_PERIMETER)
	{
		m_cacheQuadOnPerimeter = cache;
		m_lastProcessedCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			m_recalcQuadrantsInViewNeeded = true;
	}
}

int GDN_TheWorld_Viewer::getCacheQuadOnPerimeter(void) const
{
	return (int)m_cacheQuadOnPerimeter;
}

void GDN_TheWorld_Viewer::setMaxDepthQuadOnPerimeter(int depth)
{
	if (depth >= 0 && depth <= MAX_SETTABLE_DEPTH_ON_PERIMETER)
	{
		m_maxDepthQuadOnPerimeter = depth;
		if (m_maxDepthQuadOnPerimeter != -1 and m_maxDepthQuadOnPerimeter < getMinDepthQuadOnPerimeter())
			setMinDepthQuadOnPerimeter(m_maxDepthQuadOnPerimeter);
		m_lastProcessedCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			m_recalcQuadrantsInViewNeeded = true;
	}
}

int GDN_TheWorld_Viewer::getMaxDepthQuadOnPerimeter(void) const
{
	if (m_maxDepthQuadOnPerimeter < m_minDepthQuadOnPerimeter)
		return m_minDepthQuadOnPerimeter;
	else
		return m_maxDepthQuadOnPerimeter;
}

int GDN_TheWorld_Viewer::maxActualDepthQuadInAllDirections(void) const
{
	int ret = m_numVisibleQuadrantOnPerimeterOnNorthSide;
	if (m_numVisibleQuadrantOnPerimeterOnSouthSide > ret)
		ret = m_numVisibleQuadrantOnPerimeterOnSouthSide;
	if (m_numVisibleQuadrantOnPerimeterOnWestSide > ret)
		ret = m_numVisibleQuadrantOnPerimeterOnWestSide;
	if (m_numVisibleQuadrantOnPerimeterOnEastSide > ret)
		ret = m_numVisibleQuadrantOnPerimeterOnEastSide;
	return ret;
}

void GDN_TheWorld_Viewer::setForceShaderElevation0(bool elevation0)
{
	m_forceShaderElevation0 = elevation0;
	//ReloadAllShadersRequired(true);
}

bool GDN_TheWorld_Viewer::getForceShaderElevation0(void) const
{
	return m_forceShaderElevation0;
}

String GDN_TheWorld_Viewer::getInfoCamera(void)
{
	godot::Camera3D* camera = getWorldCamera();

	if (camera == nullptr)
		return "";

	double z_near = camera->get_near();
	double z_far = camera->get_far();
	double size = camera->get_size();
	Vector2 offset = camera->get_frustum_offset();
	double fov = camera->get_fov();
	double h_offset = camera->get_h_offset();
	double v_offset = camera->get_v_offset();
	godot::Camera3D::KeepAspect keepAspect = camera->get_keep_aspect_mode();

	std::string info;

	info = "znear=" + std::to_string(z_near) + " zfar=" + std::to_string(z_far) + " size=" + std::to_string(size) + " offset=" + std::to_string(h_offset) + "," + std::to_string(v_offset) + " fov=" + std::to_string(fov) + " keep_aspect=" + std::to_string(int(keepAspect));

	return info.c_str();
}

int GDN_TheWorld_Viewer::getCameraProjectionMode(void)
{
	godot::Camera3D* camera = getWorldCamera();

	if (camera == nullptr)
		return -1;

	godot::Camera3D::ProjectionType projection = camera->get_projection();

	return (int)projection;
}

void GDN_TheWorld_Viewer::recalcQuadrantsInView(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

	TheWorld_Utils::QuadrantPos cameraQuadrantPos = m_computedCameraQuadrantPos;
	if (!cameraQuadrantPos.isInitialized())
		return;
	
	vector<TheWorld_Utils::QuadrantPos> quadrantPosNeeded;
	quadrantPosNeeded.push_back(cameraQuadrantPos);
	vector<enum class QuadTree::QuadrantPriority> quadrantPriority;
	quadrantPriority.push_back(QuadTree::QuadrantPriority::High);
	vector<size_t> numQuadrantsFromCameraQuadrant;
	numQuadrantsFromCameraQuadrant.push_back(0);

	m_minVisibleQuadrantOnPerimeter = getMinDepthQuadOnPerimeter();
	m_numCacheQuadrantOnPerimeter = getCacheQuadOnPerimeter();

	if (m_numVisibleQuadrantOnPerimeterOnNorthSide == -1)
		m_numVisibleQuadrantOnPerimeterOnNorthSide = m_minVisibleQuadrantOnPerimeter;
	else 
	{
		if (m_numVisibleQuadrantOnPerimeterOnNorthSide < m_minVisibleQuadrantOnPerimeter)
			m_numVisibleQuadrantOnPerimeterOnNorthSide = m_minVisibleQuadrantOnPerimeter;
		if (m_numVisibleQuadrantOnPerimeterOnNorthSide > getMaxDepthQuadOnPerimeter())
			m_numVisibleQuadrantOnPerimeterOnNorthSide = getMaxDepthQuadOnPerimeter();
	}
	if (m_numVisibleQuadrantOnPerimeterOnSouthSide == -1)
		m_numVisibleQuadrantOnPerimeterOnSouthSide = m_minVisibleQuadrantOnPerimeter;
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnSouthSide < m_minVisibleQuadrantOnPerimeter)
			m_numVisibleQuadrantOnPerimeterOnSouthSide = m_minVisibleQuadrantOnPerimeter;
		if (m_numVisibleQuadrantOnPerimeterOnSouthSide > getMaxDepthQuadOnPerimeter())
			m_numVisibleQuadrantOnPerimeterOnSouthSide = getMaxDepthQuadOnPerimeter();
	}
	if (m_numVisibleQuadrantOnPerimeterOnWestSide == -1)
		m_numVisibleQuadrantOnPerimeterOnWestSide = m_minVisibleQuadrantOnPerimeter;
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnWestSide < m_minVisibleQuadrantOnPerimeter)
			m_numVisibleQuadrantOnPerimeterOnWestSide = m_minVisibleQuadrantOnPerimeter;
		if (m_numVisibleQuadrantOnPerimeterOnWestSide > getMaxDepthQuadOnPerimeter())
			m_numVisibleQuadrantOnPerimeterOnWestSide = getMaxDepthQuadOnPerimeter();
	}
	if (m_numVisibleQuadrantOnPerimeterOnEastSide == -1)
		m_numVisibleQuadrantOnPerimeterOnEastSide = m_minVisibleQuadrantOnPerimeter;
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnEastSide < m_minVisibleQuadrantOnPerimeter)
			m_numVisibleQuadrantOnPerimeterOnEastSide = m_minVisibleQuadrantOnPerimeter;
		if (m_numVisibleQuadrantOnPerimeterOnEastSide > getMaxDepthQuadOnPerimeter())
			m_numVisibleQuadrantOnPerimeterOnEastSide = getMaxDepthQuadOnPerimeter();
	}

	// add horizontal (X axis) quadrants on the left
	for (int i = 0; i < m_numVisibleQuadrantOnPerimeterOnWestSide + m_numCacheQuadrantOnPerimeter; i++)
	{
		TheWorld_Utils::QuadrantPos q = cameraQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus, 1 + i);
		q.setTag(cameraQuadrantPos.getTag() + " X-" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
		if (i < m_numVisibleQuadrantOnPerimeterOnWestSide)
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::High);
		}
		else if (i == m_numVisibleQuadrantOnPerimeterOnWestSide)
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::Mid);
		}
		else
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::Low);
		}
		numQuadrantsFromCameraQuadrant.push_back(i + 1);
		//break;	// SUPERDEBUGRIC only X-1 quadrant
	}

	// add horizontal (X axis) quadrants on the right
	for (int i = 0; i < m_numVisibleQuadrantOnPerimeterOnEastSide + m_numCacheQuadrantOnPerimeter; i++)
	{
		TheWorld_Utils::QuadrantPos q = cameraQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus, 1 + i);
		q.setTag(cameraQuadrantPos.getTag() + " X+" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
		if (i < m_numVisibleQuadrantOnPerimeterOnEastSide)
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::High);
		}
		else if (i == m_numVisibleQuadrantOnPerimeterOnEastSide)
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::Mid);
		}
		else
		{
			quadrantPriority.push_back(QuadTree::QuadrantPriority::Low);
		}
		numQuadrantsFromCameraQuadrant.push_back(i + 1);
		//break;	// SUPERDEBUGRIC only X+1 quadrant
	}

	// for each horizontal quadrant ...
	size_t size = quadrantPosNeeded.size();
	for (size_t idx = 0; idx < size; idx++)
	{
		// ... add vertical (Z axis) quadrants up
		for (size_t i = 0; i < m_numVisibleQuadrantOnPerimeterOnNorthSide + m_numCacheQuadrantOnPerimeter; i++)
		{
			TheWorld_Utils::QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z-" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
			if (i < m_numVisibleQuadrantOnPerimeterOnNorthSide)
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::High);
			}
			else if (i == m_numVisibleQuadrantOnPerimeterOnNorthSide)
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::Mid);
			}
			else
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::Low);
			}
			numQuadrantsFromCameraQuadrant.push_back(i + 1);
			//break;	// SUPERDEBUGRIC only Z-1 quadrant
		}

		// ... add vertical (Z axis) quadrants down
		for (size_t i = 0; i < m_numVisibleQuadrantOnPerimeterOnSouthSide + m_numCacheQuadrantOnPerimeter; i++)
		{
			TheWorld_Utils::QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z+" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
			if (i < m_numVisibleQuadrantOnPerimeterOnSouthSide)
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::High);
			}
			else if (i == m_numVisibleQuadrantOnPerimeterOnSouthSide)
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::Mid);
			}
			else
			{
				quadrantPriority.push_back(QuadTree::QuadrantPriority::Low);
			}
			numQuadrantsFromCameraQuadrant.push_back(i + 1);
			//break;	// SUPERDEBUGRIC only Z+1 quadrant
		}
	}

	vector<TheWorld_Utils::QuadrantPos> quadrantToDelete;

	bool addedQuandrantsForInititalization = false;
	bool counterStartedNow = false;
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (map stuff)");
		// insert needed quadrants not present in map and refresh the ones which are already in it
		//vector<QuadrantPos> quadrantToAdd;
		std::timespec refreshTime;

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.0 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (get time)");

			int ret = timespec_get(&refreshTime, TIME_UTC);
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create/refresh)");

			size_t size = quadrantPosNeeded.size();
			for (int idx = 0; idx < size; idx++)
			{
				WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(quadrantPosNeeded[idx]);
				if (it == m_worldQuadrantsMap.end())
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create new quads)");

					//if (it != m_worldQuadrantsMap.end() /* && it->second->statusToErase() // useless for the previous if */)
					//{
					//	// extract element from map without destroying it than remember to destory later (next _process)
					//	std::_Node_handle h = m_worldQuadrantsMap.extract(it->first);
					//	TheWorld_Utils::QuadrantPos pos = h.key();
					//	m_toErase[pos] = true;
					//}

					m_worldQuadrantsMap[quadrantPosNeeded[idx]] = make_unique<QuadTree>(this, quadrantPosNeeded[idx]);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->refreshTime(refreshTime);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->quadrantPriority(quadrantPriority[idx]);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->numQuadrantsFromCameraQuadrant(numQuadrantsFromCameraQuadrant[idx]);
					addedQuandrantsForInititalization = true;

					if (!m_streamingTime.counterStarted())
					{
						counterStartedNow = true;
						writeStatisticsNewQuadrants();
						if (m_notStreamingTime.counterStarted())
							m_notStreamingTime.tock();
						PLOG_DEBUG /* << "\n" */  << "Start initializing of a stream of quadrant";
						m_streamingTime.tick();

						startInitializingStreamOfQuadrants();
					}
				}
				else
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (refresh old quads)");

					if (it->second->statusToErase())
					{
						// recycle of quadrant flagged to erase
						it->second->setPrevStatus();
						if (m_toErase.contains(it->first))
							m_toErase.erase(it->first);
					}

					it->second->refreshTime(refreshTime);
					it->second->setTag(quadrantPosNeeded[idx].getTag());
					it->second->quadrantPriority(quadrantPriority[idx]);
					it->second->numQuadrantsFromCameraQuadrant(numQuadrantsFromCameraQuadrant[idx]);
				}
			}
		}

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (second loop)");

			// look for the quadrant to delete (the ones not refreshed)
			for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			{
				//size_t distanceInPerimeterFromCameraQuadrant = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_computedCameraQuadrantPos);

				if (itQuadTree->second->getRefreshTime().tv_nsec == refreshTime.tv_nsec && itQuadTree->second->getRefreshTime().tv_sec == refreshTime.tv_sec)
				{
					bool visibilityChanged = false;
					int distanceInPerimeterFromCameraQuadOnEstXPlus = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(cameraQuadrantPos, TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
					int distanceInPerimeterFromCameraQuadOnSouthZPlus = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(cameraQuadrantPos, TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
					bool outOfView = false;
					if (distanceInPerimeterFromCameraQuadOnEstXPlus > 0 && distanceInPerimeterFromCameraQuadOnEstXPlus > m_numVisibleQuadrantOnPerimeterOnEastSide)
						outOfView = true;
					if (distanceInPerimeterFromCameraQuadOnEstXPlus < 0 && -distanceInPerimeterFromCameraQuadOnEstXPlus > m_numVisibleQuadrantOnPerimeterOnWestSide)
						outOfView = true;
					if (distanceInPerimeterFromCameraQuadOnSouthZPlus > 0 && distanceInPerimeterFromCameraQuadOnSouthZPlus > m_numVisibleQuadrantOnPerimeterOnSouthSide)
						outOfView = true;
					if (distanceInPerimeterFromCameraQuadOnSouthZPlus < 0 && -distanceInPerimeterFromCameraQuadOnSouthZPlus > m_numVisibleQuadrantOnPerimeterOnNorthSide)
						outOfView = true;
					if (outOfView)
					{
						if (itQuadTree->second->isVisible())
						{
							visibilityChanged = true;
							itQuadTree->second->setVisible(false);
						}
					}
					else
					{
						if (!itQuadTree->second->isVisible())
						{
							visibilityChanged = true;
							itQuadTree->second->setVisible(true);
						}
					}

					if (visibilityChanged)
					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (ForAllChunk)");
						Chunk::VisibilityChangedChunkAction action(m_visibleInTree, Globals());
						itQuadTree->second->ForAllChunk(action, QuadType::NotSet);
					}
				}
				else
				{
					// Quadrant too far from camera: need to be deleted
					if (itQuadTree->second->statusInitialized() || itQuadTree->second->statusUninitialized() || itQuadTree->second->statusRefreshTerrainDataNeeded() || itQuadTree->second->statusLoadTerrainDataFromCacheNeeded())
					{
						itQuadTree->second->setVisible(false);
						quadrantToDelete.push_back(itQuadTree->first);
					}
					//if (m_cameraQuadTree != nullptr && itQuadTree->first == m_cameraQuadTree->getQuadrant()->getPos())
					//{
					//	// if we delete the ones containing old camera chunk we invalidate it
					//	m_cameraChunk = nullptr;
					//	m_cameraQuadTree = nullptr;
					//}
				}
				if (itQuadTree->second->getQuadrant()->needReloadTerrainDataFromCache())
					itQuadTree->second->setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);
			}
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase all quads)");

			for (int idx = 0; idx < quadrantToDelete.size(); idx++)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("recalcQuadrantsInView 1.1.3.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase quad)");
				m_worldQuadrantsMap[quadrantToDelete[idx]]->setStatus(QuadrantStatus::toErase);
				m_toErase[quadrantToDelete[idx]] = true;
				//m_worldQuadrantsMap.erase(quadrantToDelete[idx]);
			}
		}
	}
	
	if (addedQuandrantsForInititalization && !counterStartedNow)
	{
		PLOG_DEBUG << "STREAMER WARNING: streaming process too slow to satisfy camera movement (added new quadrant to list but older quadrants were still intializing)";
	}
	
	quadrantToDelete.clear();

	m_numVisibleQuadrantOnPerimeterOnNorthSideApplied = m_numVisibleQuadrantOnPerimeterOnNorthSide;
	m_numVisibleQuadrantOnPerimeterOnSouthSideApplied = m_numVisibleQuadrantOnPerimeterOnSouthSide;
	m_numVisibleQuadrantOnPerimeterOnWestSideApplied = m_numVisibleQuadrantOnPerimeterOnWestSide;
	m_numVisibleQuadrantOnPerimeterOnEastSideApplied = m_numVisibleQuadrantOnPerimeterOnEastSide;
}

void GDN_TheWorld_Viewer::startInitializingStreamOfQuadrants(void)
{
	m_streamingInProgress = true;
}

void GDN_TheWorld_Viewer::endInitializingStreamOfQuadrants(void)
{
	m_streamingInProgress = false;
	finishedResizingGrid(false);
}

void GDN_TheWorld_Viewer::_process(double _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

//#define MAX_PROCESS_DURATION	100000
//#define MAX_PROCESS_DURATION	200000
#define MAX_PROCESS_DURATION	300000
	m_maxProcessDuration = MAX_PROCESS_DURATION;
	if (globals->status() == TheWorldStatus::worldDeployed)
		m_processCanBeInterrupted = true;
	else
		m_processCanBeInterrupted = false;
	m_processCanBeInterrupted = false;	// DEBUG

	m_lastProcessDuration = 0;
	m_lastProcessImplDuration = 0;
	//m_lastRefreshMapQuadDuration = 0;
	m_lastUpdateQuads1Duration = 0;
	m_lastUpdateQuads2Duration = 0;
	m_lastUpdateQuads3Duration = 0;
	m_lastUpdateChunksDuration = 0;
	m_lastUpdateMaterialParamsDuration = 0;
	m_lastMouseTrackHitDuration = 0;;
	m_lastEraseQuads = 0;
	m_lastProcessMapModder = 0;
	m_lastUpdateCameraQuad = 0;
	m_lastStreamingQuadrantStuff = 0;
	m_lastProcessExecutionInterrupted = false;
	//m_processSteps.clear();
	//m_lastProcessedStep = "";

	//m_processClock.tick();
	m_processSteps.init();

	{
		//TimerMcs clock;
		//clock.tick();
		//auto save_duration = TheWorld_Viewer_Utils::finally([&clock, this] {
		//	clock.tock();
		//	this->m_lastProcessDuration = clock.duration().count();
		//	this->m_numProcessExecution++;
		//	this->m_sumProcessDuration += this->m_lastProcessDuration;
		//	});

		TheWorld_Utils::GuardProfiler profiler(std::string("_process 1 ") + __FUNCTION__, "ALL");

		m_visibleInTree = is_visible_in_tree();

		compressTrackedInputEvents();

		if (initRequired())
		{
			init();
			if (!ShaderTerrainData::groundTexturesNeedProcessing())
				globals->setStatus(TheWorldStatus::sessionInitialized);
		}

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);		// WARNING
		std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
		if (!lock.owns_lock())
		{
			m_numProcessNotOwnsLock++;
			return;
		}

		if (ShaderTerrainData::groundTexturesNeedProcessing())
		{
			std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>>& groundTextures = ShaderTerrainData::getGroundTextures();

			bool resetgroundTexturesProcessing = true;

			for (const auto& groundTexture : groundTextures)
			{
				if (!groundTexture.second->initialized)
				{
					std::string s(groundTexture.first);
					ShaderTerrainData::getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + s.c_str(), groundTexture.second.get(), this);
					globals->addNumElementInitialized();
					resetgroundTexturesProcessing = false;
					break;
				}
			}

			if (resetgroundTexturesProcessing)
			{
				godot::Ref<godot::Texture2DArray> albedoBumpTexture2DArray = memnew(godot::Texture2DArray);
				godot::TypedArray<godot::Image> albedoBumpImageArray;
				godot::Ref<godot::Texture2DArray> normalRoughnessTexture2DArray = memnew(godot::Texture2DArray);
				godot::TypedArray<godot::Image> normalRoughnessImageArray;
				std::map<std::string, int32_t>& groundTextureLayers = ShaderTerrainData::getGroundTextureLayers();
				groundTextureLayers.clear();
				int layer = 0;
				for (const auto& groundTexture : groundTextures)
				{
					albedoBumpImageArray.push_back(groundTexture.second->m_albedo_bump_tex->get_image());
					normalRoughnessImageArray.push_back(groundTexture.second->m_normal_roughness_tex->get_image());
					groundTextureLayers[groundTexture.first] = layer;
					globals->debugPrint((std::string("Ground_texture [") + groundTexture.first + "] ==> layer = " + std::to_string(layer)).c_str());
					layer++;
				}
				godot::Error e = albedoBumpTexture2DArray->create_from_images(albedoBumpImageArray);
				e = normalRoughnessTexture2DArray->create_from_images(normalRoughnessImageArray);
				godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
				rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_ALBEDO_BUMP_ARRAY, albedoBumpTexture2DArray);
				rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_NORMAL_ROUGHNESS_ARRAY, normalRoughnessTexture2DArray);

				ShaderTerrainData::groundTexturesNeedProcessing(false);
				globals->setStatus(TheWorldStatus::sessionInitialized);
			}
		}

		if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
			return;

		if (!getCachedCustomWorldCamera() || !m_initialWorldViewerPosSet)
		{
			for (auto& item : m_worldQuadrantsMap)
			{
				item.second->initDelayed();

				if (item.second->statusUninitialized())
				{
					bool forced = true;
					item.second->initDelayed(forced);
				}
			}
			return;
		}

		//godot::Camera3D* activeCamera = nullptr;
		//if (IS_EDITOR_HINT())
		//{
		//	activeCamera = getWorldCamera();
		//}
		//else
		//{
		//	activeCamera = getCachedCustomWorldCamera()->getActiveCamera();
		//}
		godot::Camera3D* activeWorldCamera = getWorldCamera2(true);
		if (activeWorldCamera == nullptr)
			return;

		godot::Camera2D* activeMapCamera = getMapCamera();
		if (activeMapCamera == nullptr)
			return;

		if (m_gizmo != nullptr)
			m_gizmo->set_camera(activeWorldCamera);

		if (m_normalDrawer != nullptr)
			m_normalDrawer->set_camera(activeWorldCamera, this);

		if (m_positionDrawer != nullptr)
			m_positionDrawer->set_camera(activeWorldCamera, this);

		if (m_rayFromCameraDrawer != nullptr)
			m_rayFromCameraDrawer->set_camera(activeWorldCamera, this);

		//// DEBUG
		//{
		//	godot::Camera3D::ProjectionType type = activeWorldCamera->get_projection();
		//	int i = activeWorldCamera->get_cull_mask();
		//	bool b = activeWorldCamera->is_current();
		//	godot::Camera3D::DopplerTracking d = activeWorldCamera->get_doppler_tracking();
		//	double far1 = activeWorldCamera->get_far();
		//	double fov = activeWorldCamera->get_fov();
		//	godot::Vector2 frustumOffset = activeWorldCamera->get_frustum_offset();
		//	double hOffset = activeWorldCamera->get_h_offset();
		//	double vOffset = activeWorldCamera->get_v_offset();
		//	double near1 = activeWorldCamera->get_near();
		//	double size1 = activeWorldCamera->get_size();
		//	godot::Camera3D::KeepAspect k = activeWorldCamera->get_keep_aspect_mode();
		//	type = (godot::Camera3D::ProjectionType)0;
		//}
		//// DEBUG

		{
			TimerMcs clock;
			clock.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock, this] {
				clock.tock();
				this->m_lastProcessImplDuration = clock.duration().count();
				this->m_numProcessImplExecution++;
				this->m_sumProcessImplDuration += this->m_lastProcessImplDuration;
				});

			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.1 ") + __FUNCTION__, "process_impl");
			process_impl(_delta, activeMapCamera, activeWorldCamera);
		}

		resetTrackedInputEvents();
	
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2 ") + __FUNCTION__, "Process Dump/Statistics");
			process_dump(globals);
			process_statistics(globals);
		}
	}

	m_lastProcessDuration = m_processSteps.elapsed();
	m_numProcessExecution++;
	m_sumProcessDuration += this->m_lastProcessDuration;

	if ((m_lastProcessExecutionInterrupted || m_lastProcessDuration > m_maxProcessDuration) && globals->status() == TheWorldStatus::worldDeployed)
	{
		std::map<enum class ProcessStep::ProcessStepIdx, ProcessStep>& steps = m_processSteps.steps();
		std::string lastExecutedProcessStep = (steps.size() > 0 ? m_processSteps.lastAddedStepName() : "");
		//size_t extra = m_lastProcessDuration - (/*m_lastRefreshMapQuadDuration + */m_lastUpdateQuads1Duration + m_lastUpdateQuads2Duration + m_lastUpdateQuads3Duration + m_lastUpdateChunksDuration + m_lastUpdateMaterialParamsDuration + m_lastMouseTrackHitDuration + m_lastEraseQuads + m_lastProcessMapModder + m_lastUpdateCameraQuad + m_lastStreamingQuadrantStuff);
		//std::string msg = "PROCESS DURATION WARNING: "
		//	+ std::to_string(m_lastProcessDuration / 1000) + (m_lastProcessExecutionInterrupted ? " EXECUTION INTERRUPTED after " + lastExecutedProcessStep + " " : " ")
		//	+ "Process Impl = " + std::to_string(m_lastProcessImplDuration / 1000) + " "
		//	//+ ", Refresh Map Quad: " + std::to_string(m_lastRefreshMapQuadDuration / 1000) + " "
		//	+ "Update Quads Stage 1 = " + std::to_string(m_lastUpdateQuads1Duration / 1000) + " "
		//	+ "Update Quads Stage 2 = " + std::to_string(m_lastUpdateQuads2Duration / 1000) + " "
		//	+ "Update Quads Stage 3 = " + std::to_string(m_lastUpdateQuads3Duration / 1000) + " "
		//	+ "Update Chunks = " + std::to_string(m_lastUpdateChunksDuration / 1000) + " "
		//	+ "Update Shader = " + std::to_string(m_lastUpdateMaterialParamsDuration / 1000) + " "
		//	+ "Mouse Track Collisions = " + std::to_string(m_lastMouseTrackHitDuration / 1000) + " "
		//	+ "Erase Quads = " + std::to_string(m_lastEraseQuads / 1000) + " "
		//	+ "Process Mapmodder = " + std::to_string(m_lastProcessMapModder / 1000) + " "
		//	+ "Update Camera Quad = " + std::to_string(m_lastUpdateCameraQuad / 1000) + " "
		//	+ "Init New Quads = " + std::to_string(m_lastStreamingQuadrantStuff / 1000) + " "
		//	+ "Extra = " + std::to_string(extra / 1000);
		//globals->debugPrint(msg.c_str());

		if (m_lastProcessExecutionInterrupted)
		{
			std::string msg1 = "PROCESS DURATION WARNING: " + std::to_string(m_lastProcessDuration / 1000) + (m_lastProcessExecutionInterrupted ? " EXECUTION INTERRUPTED after " + lastExecutedProcessStep + " " : " ");
			for (auto& step : steps)
				msg1 += step.second.msg() + " ";
			globals->debugPrint(msg1.c_str());
		}
	}
}

godot::Control* GDN_TheWorld_Viewer::getOrCreateEditModeUIControl(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl == nullptr || !editModeUIControl->initialized())
	{
		createEditModeUI();
		editModeUIControl = EditModeUIControl();

		if (m_editMode)
			editModeUIControl->set_visible(true);
		else
			editModeUIControl->set_visible(false);
	}

	return editModeUIControl;
}

void GDN_TheWorld_Viewer::process_impl(double _delta, godot::Camera2D* activeMapCamera, godot::Camera3D* activeWorldCamera)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1 ") + __FUNCTION__, "Inside lock");

	if (m_firstProcess)
	{
		m_debugDraw = get_viewport()->get_debug_draw();

		// view frustum
		//Ref<Mesh> _mesh = activeCamera->DrawViewFrustum(GDN_TheWorld_Globals::g_color_green);
		//SpatialMaterial* mat = SpatialMaterial::_new();
		//mat->set_flag(SpatialMaterial::Flags::FLAG_UNSHADED, true);
		//mat->set_flag(SpatialMaterial::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
		//mat->set_albedo(GDN_TheWorld_Globals::g_color_green);
		//_mesh->surface_set_material(0, mat);
		//VisualServer* vs = VisualServer::get_singleton();
		//m_viewFrustumMeshInstance = vs->instance_create();
		//vs->instance_set_visible(m_viewFrustumMeshInstance, true);
		//RID meshRID = _mesh->get_rid();
		//VisualServer::get_singleton()->instance_set_base(m_viewFrustumMeshInstance, meshRID);
		//m_viewFrustumMeshRID = meshRID;
		//m_viewFrustumMesh = _mesh;

		m_firstProcess = false;
	}

	static size_t s_num = 0, s_loadingWorldRefresh = 0;
	if (globals->status() == TheWorldStatus::worldDeployInProgress && s_loadingWorldRefresh < 10)
		if (s_num > 10)
		{
			m_refreshRequired = true;
			s_num = 0;
			s_loadingWorldRefresh++;
		}
		else
			s_num++;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.01 ") + __FUNCTION__, "Modder work");
		process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution(_delta);
	}

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initialized())
	{
		if (m_editMode)
		{
			if (!editModeUIControl->is_visible())
				editModeUIControl->set_visible(true);
		}
		else
		{
			if (editModeUIControl->is_visible())
				editModeUIControl->set_visible(false);
		}
	}
	
	// erase out-of-view quadrants
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.02 ") + __FUNCTION__, "Erase out-of-view quadrants");
		process_eraseOutOfViewQuads(editModeUIControl);
	}
	
	godot::Vector3 cameraWorldPos;
	if (worldMode())
	{
		if (IS_EDITOR_HINT())
			cameraWorldPos = activeWorldCamera->get_global_transform().get_origin();
		else
			cameraWorldPos = ((GDN_TheWorld_Camera*)activeWorldCamera)->getWorldPos();
	}
	else
	{
		godot::Vector2 mapCameraWorldPos;
		if (IS_EDITOR_HINT())
			mapCameraWorldPos = activeMapCamera->get_global_transform().get_origin();
		else
			mapCameraWorldPos = ((GDN_TheWorld_MapCamera*)activeMapCamera)->getCameraMapPos();
		cameraWorldPos = godot::Vector3(mapCameraWorldPos.x, m_altitudeForToggleToWorldMode, mapCameraWorldPos.y);
	}

	bool processCameraPosForQuadrants = true;
	bool processCameraPosForChunks = true;
	if (globals->status() == TheWorldStatus::worldDeployed)
	{
		processCameraPosForQuadrants = false;
		processCameraPosForChunks = false;
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (m_cameraTerrainChunk == nullptr || m_lastCameraTerrainChunk == nullptr || (TIME_INTERVAL_BETWEEN_ELAB_CAMERA != -1 && timeElapsed - m_timeElapsedFromLastProcessCameraPosForQuadrants > TIME_INTERVAL_BETWEEN_ELAB_CAMERA))
		{
			m_timeElapsedFromLastProcessCameraPosForQuadrants = timeElapsed;
			processCameraPosForQuadrants = true;
			processCameraPosForChunks = true;
			if (m_lastCameraTerrainChunk != m_cameraTerrainChunk)
				m_lastCameraTerrainChunk = m_cameraTerrainChunk;
		}
		else
		{
			float distance = (float)cameraWorldPos.distance_to(m_lastProcessedCameraPosForQuadrants);
			float threshold = globals->getRefreshQuadrantTreeThreshold();
			if (m_lastProcessedCameraPosForQuadrants == godot::Vector3() || distance > threshold)
			{
				m_lastProcessedCameraPosForQuadrants = cameraWorldPos;
				processCameraPosForQuadrants = true;
			}
			distance = (float)cameraWorldPos.distance_to(m_lastProcessedCameraPosForChunks);
			threshold = globals->getRefreshChunkLodThreshold();
			if (m_lastProcessedCameraPosForChunks == godot::Vector3() || distance > threshold)
			{
				m_lastProcessedCameraPosForChunks = cameraWorldPos;
				processCameraPosForChunks = true;
			}
			else
			{
				if (m_lastCameraTerrainChunk != m_cameraTerrainChunk)
				{
					processCameraPosForChunks = true;
					m_lastCameraTerrainChunk = m_cameraTerrainChunk;
				}
			}
		}
	}

	//Transform globalTransform = internalTransformGlobalCoord();
	//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.03 ") + __FUNCTION__, "Adjust Quadtrees");
		if (processCameraPosForQuadrants)
		{
			process_updateCameraQuad(globals, cameraWorldPos);
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateCameraQuad))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}
	}

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.04 ") + __FUNCTION__, "Init new quadrants");
		process_initNewQuads(processCameraPosForQuadrants);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_initNewQuads))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	//if (worldMode())	// TODO in mapMode: process_trackMouse
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.05 ") + __FUNCTION__, "Track mouse");
		process_trackMouse(activeWorldCamera);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_trackMouse))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	if (worldMode()) 
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.06 ") + __FUNCTION__, "Cast Ray From Camera");
		process_castRayFromCamera(activeWorldCamera);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_castRayFromCamera))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.07 ") + __FUNCTION__, "Update camera in quadrants");
		process_updateCameraPosInShader(activeWorldCamera);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateCameraInQuadrants))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	if (worldMode()) 
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.08 ") + __FUNCTION__, "Update origin shift");
		process_updateOriginShift(activeMapCamera, activeWorldCamera);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateOriginShift))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09 ") + __FUNCTION__, "Update quadrants");
		if (processCameraPosForChunks || !finishedResizingGrid())
		{
			process_updateLodInQuads(cameraWorldPos);
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateLodInQuads))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}
		else
		{
			for (auto& itQuadTree : m_worldQuadrantsMap)
			{
				itQuadTree.second->checkMouseHitOnTerrain();
			}
		}
	}

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.10 ") + __FUNCTION__, "Update chunks");
		//if (processCameraPosForChunks)
		{
			process_updateMeshForUpdatedChunks();
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateMeshForUpdatedChunks))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}
	}

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.11 ") + __FUNCTION__, "Update Grid Dimension");
		if (processCameraPosForChunks || !finishedResizingGrid())
		{
			process_updateGridDimension(globals, cameraWorldPos, activeWorldCamera);
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateGridDimension))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}
	}

	if (worldMode()) 
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.12 ") + __FUNCTION__, "Other stuff");

		if (m_updateTerrainVisibilityRequired)
		{
			if (m_initialWorldViewerPosSet)
			{
				for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;
					Chunk::DebugVisibilityChangedChunkAction action(m_debugContentVisibility);
					itQuadTree->second->ForAllChunk(action, QuadType::NotSet);
				}
			}
			m_updateTerrainVisibilityRequired = false;
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::UpdateTerrainVisibility))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}

		if (m_updateDebugModeRequired)
		{
			if (m_initialWorldViewerPosSet)
			{
				for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
				{
					Chunk::SwitchDebugModeAction action(m_requiredChunkDebugMode);
					itQuadTree->second->ForAllChunk(action, QuadType::NotSet);
				}
				m_currentChunkDebugMode = m_requiredChunkDebugMode;
			}
			m_updateDebugModeRequired = false;
			if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::UpdateDebugMode))
			{
				m_lastProcessExecutionInterrupted = true;
				return;
			}
		}
	}

	if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.13 ") + __FUNCTION__, "Process Lookdev");
		process_lookDev();
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_lookDev))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}
	
	//if (worldMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.14 ") + __FUNCTION__, "Process Material Params");
		process_materialParam(cameraWorldPos, processCameraPosForQuadrants, processCameraPosForChunks);
		if (m_lastProcessExecutionInterrupted)
			return;
	}

	if (mapMode())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.15 ") + __FUNCTION__, "Process Map Quadrants");
		process_mapQuadrants(cameraWorldPos);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_mapQuadrants))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.16 ") + __FUNCTION__, "Update HUD");
		process_updateHud(activeWorldCamera);
		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateHud))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	size_t elapsed = addProcessStep(ProcessStep::ProcessStepIdx::EndOfProcessExecution);
	//m_desideredLookDevChanged = false;
}

void GDN_TheWorld_Viewer::process_eraseOutOfViewQuads(GDN_TheWorld_Edit* editModeUIControl)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Erase Out-Of-View Quadrants", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastEraseQuads = clock1.duration().count();
		this->m_numEraseQuads++;
		this->m_sumEraseQuads += this->m_lastEraseQuads;
		});

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	std::vector<TheWorld_Utils::QuadrantPos> toErase;

	for (auto& item : m_toErase)
	{
		size_t numErased = 0;

		TheWorld_Utils::QuadrantPos pos = item.first;

		if (m_worldQuadrantsMap.contains(pos))
		{
			bool locked = m_worldQuadrantsMap[pos]->getQuadrant()->lockInternalData();
			if (locked)
			{
				if (editModeUIControl != nullptr)
				{
					bool manualSave;
					QuadTree* quadTreeToErase = m_worldQuadrantsMap[pos].get();
					if (!editModeUIControl->isQuadToSave(pos, manualSave) && !quadTreeToErase->getQuadrant()->needUploadToServer())
					{
						m_adjacentQuadrantsHit.remove(quadTreeToErase);
						if (m_quadrantHit == quadTreeToErase)
							m_quadrantHit = nullptr;
						if (m_editMode && editModeUIControl && editModeUIControl->initialized())
							if (editModeUIControl->getQuadrantHit() == quadTreeToErase)
								editModeUIControl->setQuadrantHit(nullptr);
						numErased = m_worldQuadrantsMap.erase(pos);
						toErase.push_back(pos);
						break;
					}
					else
						m_worldQuadrantsMap[pos]->getQuadrant()->unlockInternalData();
				}
				else
				{
					numErased = m_worldQuadrantsMap.erase(pos);
					toErase.push_back(pos);
					break;
				}
			}
		}
	}

	//m_toErase.clear();
	for (auto& pos : toErase)
	{
		m_toErase.erase(pos);
	}

	toErase.clear();
	for (auto& item : m_extraQuadsMap)
		if (item.second->getQuadrant()->mapModderWorkDone())
			toErase.push_back(item.first);
	for (auto& pos : toErase)
	{
		m_extraQuadsMap.erase(pos);
	}
}

void GDN_TheWorld_Viewer::process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution(double _delta)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Process Map Modder", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastProcessMapModder = clock1.duration().count();
		this->m_numProcessMapModder++;
		this->m_sumProcessMapModder += this->m_lastProcessMapModder;
		});

	std::lock_guard<std::recursive_mutex> lock1(m_mtxQuadTreeAndMainProcessing);

	//if (m_mapModder != nullptr)
	//{
	//	if (m_mapModder->startProcessRequired())
	//	{
	//		m_mapModder->startProcessing();
	//		m_mapModder->startProcessRequired(false);
	//	}

	//	if (m_mapModder->deinitRequired())
	//	{
	//		m_mapModder->deinit();
	//		m_mapModder->deinitRequired(false);
	//	}

	//	m_mapModder->debug_process(_delta);
	//}
	
	GDN_TheWorld_Globals* globals = Globals();

	for (auto& item : m_worldQuadrantsMap)
	{
		if (globals->getParamShowSeaChanged())
		{
			Chunk::VisibilityChangedChunkAction action(globals->getParamShowSea(), globals);
			item.second->ForAllChunk(action, QuadType::Sea);
		}

		if (globals->getParamShowTerrainChanged())
		{
			Chunk::VisibilityChangedChunkAction action(globals->getParamShowTerrain(), globals);
			item.second->ForAllChunk(action, QuadType::Terrain);
		}

		if (item.second->initDelayedRequested())
			item.second->initDelayed();

		item.second->getQuadrant()->checkInternalData(_delta);
		
		item.second->getQuadrant()->lockInternalDataUntilCompleteProcessExecution(false);

		if (canProcessDataUpdate())
			item.second->getQuadrant()->process_mapModder(_delta);

		if (item.second->statusLoadTerrainDataFromCacheNeeded())
		{
			item.second->setStatus(QuadrantStatus::loadTerrainDataFromCacheInProgress);
			std::function<void(void)> f = std::bind(&Quadrant::loadTerrainDataFromCache, item.second->getQuadrant());
			m_tpClientWorker.QueueJob(f);
		}

		item.second->checkSeaChunksToDelete();
	}

	if (globals->getParamShowSeaChanged())
		globals->setParamShowSeaChanged(false);

	if (globals->getParamShowTerrainChanged())
		globals->setParamShowTerrainChanged(false);

	if (Globals()->status() == TheWorldStatus::worldDeployed)
	{
		for (auto& item : m_extraQuadsMap)
		{
			if (canProcessDataUpdate())
			{
				if (item.second->initDelayedRequested())
					item.second->initDelayed();

				if (item.second->statusUninitialized())
				{
					bool forced = true;
					item.second->initDelayed(forced);
				}

				item.second->getQuadrant()->process_mapModder(_delta);
			}
		}
	}
}

void GDN_TheWorld_Viewer::process_updateGridDimension(GDN_TheWorld_Globals* globals, godot::Vector3& cameraPosInMap, godot::Camera3D* activeWorldCamera)
{
	int64_t elapsedFromLastRecalc = TIME()->get_ticks_msec() - m_timeElapsedFromLastRecalc;
	if (Globals()->status() == TheWorldStatus::worldDeployed
		&& allCollidersReady()
		&& !m_streamingInProgress
		//&& elapsedFromLastRecalc > TIME_INTERVAL_BETWEEN_RECALC_QUADRANT
		&& m_computedCameraQuadrantPos.isInitialized()
		&& Globals()->getStreamWorld())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.11.01 ") + __FUNCTION__, "Update Grid Dimension");

		finishedResizingGrid(true);

		godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
		godot::Vector3 rayOrigin = activeWorldCamera->get_global_position();

		std::vector<bool> _hit;
		std::vector<godot::Vector3> _hitPosition;
		std::vector<float> _angle;
		std::vector<godot::Vector3> _rayEnd;
		godot::Dictionary rayArray;
		float rotationAngle = (18 * TheWorld_Utils::kPi) / 180;		// degree to radians
		int numRaysPerDirection = 5;
		
		bool debug = getDebugContentVisibility();

		if (!debug)
			m_debugRayToUpdateGridDimension.clear();
		
		// North direction
		_hit.clear();
		_hitPosition.clear();
		_angle.clear();
		_rayEnd.clear();
		rayArray.clear();
		{
			godot::Vector3 castDirection = godot::Vector3(0, 0, -(m_numVisibleQuadrantOnPerimeterOnNorthSide + 1) * m_computedCameraQuadrantPos.getSizeInWU());

			for (int i = 0; i <= (numRaysPerDirection - 1) / 2; i++)
			{
				float angle = rotationAngle * i;
				godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
				_angle.push_back(angle);
				_rayEnd.push_back(rayEnd);
				//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
				godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
				rayArray = spaceState->intersect_ray(query);
				if (rayArray.has("position"))
				{
					_hit.push_back(true);
					_hitPosition.push_back(rayArray["position"]);
				}
				else
				{
					_hit.push_back(false);
					_hitPosition.push_back(godot::Vector3());
				}

				//if (debug)
				//{
				//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::North;
				//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
				//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//	else
				//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//}

				if (i > 0)
				{
					float angle = -rotationAngle * i;
					godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
					_angle.push_back(angle);
					_rayEnd.push_back(rayEnd);
					//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
					godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
					rayArray = spaceState->intersect_ray(query);
					if (rayArray.has("position"))
					{
						_hit.push_back(true);
						_hitPosition.push_back(rayArray["position"]);
					}
					else
					{
						_hit.push_back(false);
						_hitPosition.push_back(godot::Vector3());
					}

					//if (debug)
					//{
					//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::North;
					//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
					//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//	else
					//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//}
				}
			}
			
			bool allHit = true;
			float maxDistanceFromCamera = 0;
			int idxMaxDistanceFromCamera = -1;
			for (int i = 0; i < _hit.size(); i++)
			{
				if (_hit[i] == true)
				{
					float distanceFromcamera = activeWorldCamera->get_global_position().distance_to(_hitPosition[i]);
					if (distanceFromcamera > maxDistanceFromCamera)
					{
						maxDistanceFromCamera = distanceFromcamera;
						idxMaxDistanceFromCamera = i;
					}
				}
				else
				{
					allHit = false;
					break;
				}
			}

			if (debug)
			{
				for (int i = 0; i < _hit.size(); i++)
				{
					bool maxDistanceFromCameraForSide = false;
					if (allHit && i == idxMaxDistanceFromCamera)
						maxDistanceFromCameraForSide = true;
					DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::North;
					if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(_angle[i]))
						m_debugRayToUpdateGridDimension[side][_angle[i]]->setup(godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
					else
						m_debugRayToUpdateGridDimension[side][_angle[i]] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeWorldCamera, side, _angle[i], godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
				}
			}

			if (allHit)
			{
				if (m_numVisibleQuadrantOnPerimeterOnNorthSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnNorthSide - 1) * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnNorthSide--;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnNorthSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnNorthSide * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnNorthSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnNorthSide < getMaxDepthQuadOnPerimeter())
				{
					m_numVisibleQuadrantOnPerimeterOnNorthSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
			
		
		}

		// South direction
		_hit.clear();
		_hitPosition.clear();
		_angle.clear();
		_rayEnd.clear();
		rayArray.clear();
		{
			godot::Vector3 castDirection = godot::Vector3(0, 0, (m_numVisibleQuadrantOnPerimeterOnSouthSide + 1) * m_computedCameraQuadrantPos.getSizeInWU());

			for (int i = 0; i <= (numRaysPerDirection - 1) / 2; i++)
			{
				float angle = rotationAngle * i;
				godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
				_angle.push_back(angle);
				_rayEnd.push_back(rayEnd);
				//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
				godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
				rayArray = spaceState->intersect_ray(query);
				if (rayArray.has("position"))
				{
					_hit.push_back(true);
					_hitPosition.push_back(rayArray["position"]);
				}
				else
				{
					_hit.push_back(false);
					_hitPosition.push_back(godot::Vector3());
				}

				//if (debug)
				//{
				//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::South;
				//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
				//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//	else
				//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//}

				if (i > 0)
				{
					float angle = -rotationAngle * i; 
					godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
					_angle.push_back(angle);
					_rayEnd.push_back(rayEnd);
					//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
					godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
					rayArray = spaceState->intersect_ray(query);
					if (rayArray.has("position"))
					{
						_hit.push_back(true);
						_hitPosition.push_back(rayArray["position"]);
					}
					else
					{
						_hit.push_back(false);
						_hitPosition.push_back(godot::Vector3());
					}

					//if (debug)
					//{
					//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::South;
					//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
					//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//	else
					//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//}
				}
			}

			bool allHit = true;
			float maxDistanceFromCamera = 0;
			int idxMaxDistanceFromCamera = -1;
			for (int i = 0; i < _hit.size(); i++)
			{
				if (_hit[i] == true)
				{
					float distanceFromcamera = activeWorldCamera->get_global_position().distance_to(_hitPosition[i]);
					if (distanceFromcamera > maxDistanceFromCamera)
					{
						maxDistanceFromCamera = distanceFromcamera;
						idxMaxDistanceFromCamera = i;
					}
				}
				else
				{
					allHit = false;
					break;
				}
			}

			if (debug)
			{
				for (int i = 0; i < _hit.size(); i++)
				{
					bool maxDistanceFromCameraForSide = false;
					if (allHit && i == idxMaxDistanceFromCamera)
						maxDistanceFromCameraForSide = true;
					DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::South;
					if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(_angle[i]))
						m_debugRayToUpdateGridDimension[side][_angle[i]]->setup(godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
					else
						m_debugRayToUpdateGridDimension[side][_angle[i]] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeWorldCamera, side, _angle[i], godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
				}
			}

			if (allHit)
			{
				if (m_numVisibleQuadrantOnPerimeterOnSouthSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnSouthSide - 1) * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnSouthSide--;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnSouthSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnSouthSide * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnSouthSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnSouthSide < getMaxDepthQuadOnPerimeter())
				{
					m_numVisibleQuadrantOnPerimeterOnSouthSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
		}

		// West direction
		_hit.clear();
		_hitPosition.clear();
		_angle.clear();
		_rayEnd.clear();
		rayArray.clear();
		{
			godot::Vector3 castDirection = godot::Vector3(-(m_numVisibleQuadrantOnPerimeterOnWestSide + 1) * m_computedCameraQuadrantPos.getSizeInWU(), 0, 0);

			for (int i = 0; i <= (numRaysPerDirection - 1) / 2; i++)
			{
				float angle = rotationAngle * i;
				godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
				_angle.push_back(angle);
				_rayEnd.push_back(rayEnd);
				//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
				godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
				rayArray = spaceState->intersect_ray(query);
				if (rayArray.has("position"))
				{
					_hit.push_back(true);
					_hitPosition.push_back(rayArray["position"]);
				}
				else
				{
					_hit.push_back(false);
					_hitPosition.push_back(godot::Vector3());
				}

				//if (debug)
				//{
				//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::West;
				//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
				//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//	else
				//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//}

				if (i > 0)
				{
					float angle = -rotationAngle * i;
					godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
					_angle.push_back(angle);
					_rayEnd.push_back(rayEnd);
					//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
					godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
					rayArray = spaceState->intersect_ray(query);
					if (rayArray.has("position"))
					{
						_hit.push_back(true);
						_hitPosition.push_back(rayArray["position"]);
					}
					else
					{
						_hit.push_back(false);
						_hitPosition.push_back(godot::Vector3());
					}

					//if (debug)
					//{
					//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::West;
					//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
					//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//	else
					//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//}
				}
			}

			bool allHit = true;
			float maxDistanceFromCamera = 0;
			int idxMaxDistanceFromCamera = -1;
			for (int i = 0; i < _hit.size(); i++)
			{
				if (_hit[i] == true)
				{
					float distanceFromcamera = activeWorldCamera->get_global_position().distance_to(_hitPosition[i]);
					if (distanceFromcamera > maxDistanceFromCamera)
					{
						maxDistanceFromCamera = distanceFromcamera;
						idxMaxDistanceFromCamera = i;
					}
				}
				else
				{
					allHit = false;
					break;
				}
			}

			if (debug)
			{
				for (int i = 0; i < _hit.size(); i++)
				{
					bool maxDistanceFromCameraForSide = false;
					if (allHit && i == idxMaxDistanceFromCamera)
						maxDistanceFromCameraForSide = true;
					DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::West;
					if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(_angle[i]))
						m_debugRayToUpdateGridDimension[side][_angle[i]]->setup(godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
					else
						m_debugRayToUpdateGridDimension[side][_angle[i]] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeWorldCamera, side, _angle[i], godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
				}
			}

			if (allHit)
			{
				if (m_numVisibleQuadrantOnPerimeterOnWestSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnWestSide - 1) * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnWestSide--;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnWestSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnWestSide * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnWestSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnWestSide < getMaxDepthQuadOnPerimeter())
				{
					m_numVisibleQuadrantOnPerimeterOnWestSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
		}

		// East direction
		_hit.clear();
		_hitPosition.clear();
		_angle.clear();
		_rayEnd.clear();
		rayArray.clear();
		{
			godot::Vector3 castDirection = godot::Vector3((m_numVisibleQuadrantOnPerimeterOnEastSide + 1) * m_computedCameraQuadrantPos.getSizeInWU(), 0, 0);

			for (int i = 0; i <= (numRaysPerDirection - 1) / 2; i++)
			{
				float angle = rotationAngle * i;
				godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
				_angle.push_back(angle);
				_rayEnd.push_back(rayEnd);
				//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
				godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
				rayArray = spaceState->intersect_ray(query);
				if (rayArray.has("position"))
				{
					_hit.push_back(true);
					_hitPosition.push_back(rayArray["position"]);
				}
				else
				{
					_hit.push_back(false);
					_hitPosition.push_back(godot::Vector3());
				}

				//if (debug)
				//{
				//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::East;
				//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
				//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//	else
				//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
				//}

				if (i > 0)
				{
					float angle = -rotationAngle * i;
					godot::Vector3 rayEnd = rayOrigin + castDirection.rotated(Vector3Up, angle);
					_angle.push_back(angle);
					_rayEnd.push_back(rayEnd);
					//float d = rayOrigin.distance_to(rayEnd);	// DEBUG
					godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd, THEWORLD_TERRAIN_COLLISION_LAYER);
					rayArray = spaceState->intersect_ray(query);
					if (rayArray.has("position"))
					{
						_hit.push_back(true);
						_hitPosition.push_back(rayArray["position"]);
					}
					else
					{
						_hit.push_back(false);
						_hitPosition.push_back(godot::Vector3());
					}

					//if (debug)
					//{
					//	DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::East;
					//	if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(angle))
					//		m_debugRayToUpdateGridDimension[side][angle]->setup(godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//	else
					//		m_debugRayToUpdateGridDimension[side][angle] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeCamera, side, angle, godot::Vector3(), globalPosToWorldPos(rayEnd), _hit[_hit.size() - 1], globalPosToWorldPos(_hitPosition[_hitPosition.size() - 1]));
					//}
				}
			}

			bool allHit = true;
			float maxDistanceFromCamera = 0;
			int idxMaxDistanceFromCamera = -1;
			for (int i = 0; i < _hit.size(); i++)
			{
				if (_hit[i] == true)
				{
					float distanceFromcamera = activeWorldCamera->get_global_position().distance_to(_hitPosition[i]);
					if (distanceFromcamera > maxDistanceFromCamera)
					{
						maxDistanceFromCamera = distanceFromcamera;
						idxMaxDistanceFromCamera = i;
					}
				}
				else
				{
					allHit = false;
					break;
				}
			}

			if (debug)
			{
				for (int i = 0; i < _hit.size(); i++)
				{
					bool maxDistanceFromCameraForSide = false;
					if (allHit && i == idxMaxDistanceFromCamera)
						maxDistanceFromCameraForSide = true;
					DebugRayToUpdateGridDimensionDrawer::Side side = DebugRayToUpdateGridDimensionDrawer::Side::East;
					if (m_debugRayToUpdateGridDimension.contains(side) && m_debugRayToUpdateGridDimension[side].contains(_angle[i]))
						m_debugRayToUpdateGridDimension[side][_angle[i]]->setup(godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
					else
						m_debugRayToUpdateGridDimension[side][_angle[i]] = make_unique<DebugRayToUpdateGridDimensionDrawer>(this, activeWorldCamera, side, _angle[i], godot::Vector3(), globalPosToWorldPos(_rayEnd[i]), _hit[i], globalPosToWorldPos(_hitPosition[i]), maxDistanceFromCameraForSide);
				}
			}

			if (allHit)
			{
				if (m_numVisibleQuadrantOnPerimeterOnEastSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnEastSide - 1) * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnEastSide--;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnEastSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnEastSide * m_computedCameraQuadrantPos.getSizeInWU())
				{
					m_numVisibleQuadrantOnPerimeterOnEastSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnEastSide < getMaxDepthQuadOnPerimeter())
				{
					m_numVisibleQuadrantOnPerimeterOnEastSide++;
					m_recalcQuadrantsInViewNeeded = true;
					finishedResizingGrid(false);
				}
			}
		}

		if (m_recalcQuadrantsInViewNeeded)
		{
			GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
			if (cam != nullptr)
				cam->forceRecalcCameraFar();
		}

		m_timeElapsedFromLastRecalc = TIME()->get_ticks_msec();
	}
}

void GDN_TheWorld_Viewer::process_updateCameraQuad(GDN_TheWorld_Globals* globals, godot::Vector3& cameraPosInMap)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Camera Quad", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastUpdateCameraQuad = clock1.duration().count();
		this->m_numUpdateCameraQuad++;
		this->m_sumUpdateCameraQuad += this->m_lastUpdateCameraQuad;
		});

	TheWorld_Utils::QuadrantPos cameraQuadrantPos;

	// Look for Camera QuadTree: it will be used as first element of quadrantPosNeeded
	for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	{
		if (itQuadTree->second->statusToErase())
			continue;

		float quadrantSizeInWU = itQuadTree->second->getQuadrant()->getPos().getSizeInWU();
		if (cameraPosInMap.x >= itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() && cameraPosInMap.x < (itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() + quadrantSizeInWU)
			&& cameraPosInMap.z >= itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() && cameraPosInMap.z < (itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() + quadrantSizeInWU))
		{
			cameraQuadrantPos = itQuadTree->second->getQuadrant()->getPos();
			cameraQuadrantPos.setTag("Camera");
			break;
		}
	}

	if (cameraQuadrantPos.empty())	// boh not found camera-quad-tree (it seems impossible) ...
	{
		// ... btw we create new quad tree containing camera
		float x = (float)cameraPosInMap.x, z = (float)cameraPosInMap.z;
		float _gridStepInWU = globals->gridStepInWU();
		TheWorld_Utils::QuadrantPos q(x, z, m_worldViewerLevel, m_numWorldVerticesPerSize, _gridStepInWU);
		cameraQuadrantPos = q;
		cameraQuadrantPos.setTag("Camera");
	}

	//
	// now quadrantPosNeeded has 1 element and quadrantPosNeeded[0] is the camera 
	//

	// if forced or the camera has changed quadrant the cache is repopulated
	//m_computedCameraQuadrantPos = quadrantPosNeeded[0];	// DEBUG
	if ((m_refreshQuadTreeMap || m_computedCameraQuadrantPos != cameraQuadrantPos) && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

		//TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Recalc in memory map of quadrants", false, false);
		//clock1.tick();
		//auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		//	clock1.tock();
		//	this->m_lastRefreshMapQuadDuration = clock1.duration().count();
		//	this->m_numRefreshMapQuad++;
		//	this->m_sumRefreshMapQuadDuration += this->m_lastRefreshMapQuadDuration;
		//	});

		m_refreshQuadTreeMap = false;
		m_computedCameraQuadrantPos = cameraQuadrantPos;

		if (Globals()->status() != TheWorldStatus::worldDeployed || Globals()->getStreamWorld())
			m_recalcQuadrantsInViewNeeded = true;
	}

	for (auto& itQuadTree : m_worldQuadrantsMap)
	{
		itQuadTree.second->setQuadrantOptimizations(cameraPosInMap);
	}
}

void GDN_TheWorld_Viewer::process_initNewQuads(bool processCameraPosForQuadrants)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Init new quads", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastStreamingQuadrantStuff = clock1.duration().count();
		this->m_numStreamingQuadrantStuff++;
		this->m_sumStreamingQuadrantStuff += this->m_lastStreamingQuadrantStuff;
		});

	bool allQuadrantInitialized = true;

	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
	if (lock.owns_lock())
	{
		for (size_t numQuadrantsFromCameraQuadrant = 0; numQuadrantsFromCameraQuadrant <= getMaxDepthQuadOnPerimeter() + m_numCacheQuadrantOnPerimeter; numQuadrantsFromCameraQuadrant++)
		{
			if (initNewQuadsAtPriority(numQuadrantsFromCameraQuadrant, allQuadrantInitialized))
				break;
		}

		lock.unlock();

		if (allQuadrantInitialized)
		{
			// m_minVisibleQuadrantOnPerimeter == 1
			if (Globals()->status() == TheWorldStatus::worldDeployInProgress && ((m_minVisibleQuadrantOnPerimeter == 0 && m_worldQuadrantsMap.size() == 1) || m_worldQuadrantsMap.size() > 1))
			{
				Globals()->setStatus(TheWorldStatus::worldDeployed);
				m_refreshRequired = true;
			}

			if (m_streamingTime.counterStarted())
			{
				// Initialization of last stream of quadrant COMPLETED
				m_lastCameraAltitudeSetToShader = -1.0f;
				m_lastCameraPosSetToShaderInitialized = false;
				writeStatisticsNewQuadrants();
				m_streamingTime.tock();
				m_notStreamingTime.tick();
				
				endInitializingStreamOfQuadrants();
			}
		}
	}
}

bool GDN_TheWorld_Viewer::initNewQuadsAtPriority(size_t numQuadrantsFromCameraQuadrant, bool& allQuadrantInitialized)
{
	bool exitForNow = false;
	
	for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	{
		itQuadTree->second->checkAndInitSeaQuadrant();

		//if (itQuadTree->second->status() != QuadrantStatus::initialized)
		//if (itQuadTree->second->status() != QuadrantStatus::initialized && itQuadTree->second->status() != QuadrantStatus::toErase)
		if (itQuadTree->second->status() < QuadrantStatus::initialized)
			allQuadrantInitialized = false;

		Quadrant* quadrant = itQuadTree->second->getQuadrant();

		if (quadrant->internalDataLocked())
			continue;

		if (itQuadTree->second->statusUninitialized() || itQuadTree->second->statusRefreshTerrainDataNeeded())
		{
			//size_t distanceInPerimeterFromCameraQuadrant = quadrant->getPos().distanceInPerimeter(m_computedCameraQuadrantPos);
			if (itQuadTree->second->numQuadrantsFromCameraQuadrant() == numQuadrantsFromCameraQuadrant)
			{
				bool proceed = true;
				if (itQuadTree->second->quadrantPriority() == QuadTree::QuadrantPriority::Low && finishedResizingGrid())
				{
					int64_t elapsedFromLastQuadInitAtLowPriority = TIME()->get_ticks_msec() - m_timeElapsedFromLastQuadrandtInit;
					if (elapsedFromLastQuadInitAtLowPriority < TIME_INTERVAL_BETWEEN_QUADRANT_INIT_AT_LOW_PRIORITY)
						proceed = false;
				}

				if (proceed)
				{
					m_timeElapsedFromLastQuadrandtInit = TIME()->get_ticks_msec();

					float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
					if (itQuadTree->second->statusUninitialized())
						itQuadTree->second->init(x, y, z, yaw, pitch, roll, false, 0.0f);
					else	// itQuadTree->second->statusRefreshTerrainDataNeeded()
					{
						itQuadTree->second->refreshTerrainData(x, y, z, yaw, pitch, roll, false, 0.0f);
						Globals()->debugPrint((std::string("Refreshing quadrant ") + itQuadTree->second->getQuadrant()->getPos().getIdStr() + " " + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(itQuadTree->second->getQuadrant()->getMeshId())).c_str());
					}
					exitForNow = true;
					break;
				}
			}
		}

		//if (itQuadTree->second->statusLoadTerrainDataFromCacheNeeded())
		//{
		//	itQuadTree->second->setStatus(QuadrantStatus::loadTerrainDataFromCacheInProgress);
		//	std::function<void(void)> f = std::bind(&Quadrant::loadTerrainDataFromCache, itQuadTree->second->getQuadrant());
		//	m_tpClientWorker.QueueJob(f);
		//}
	}

	return exitForNow;
}

void GDN_TheWorld_Viewer::allCollidersReady(bool b)
{
	//if (b)
	//{
	//	bool b1 = b;
	//}
	//else
	//{
	//	bool b1 = b;
	//}
	if (b && !m_allCollidersReady)
	{
		PLOG_DEBUG << "All colliders are now ready!";
	}
	m_allCollidersReady = b;

}

void GDN_TheWorld_Viewer::writeStatisticsNewQuadrants(void)
{
	size_t num = m_numVisibleQuadrantsStreamed + m_numInvisibleQuadrantsStreamed;
	
	if (!m_streamingTime.counterStarted() && !m_notStreamingTime.counterStarted())
		return;
	
	m_mtxQuadTreeAndMainProcessing.lock();
	size_t numQuadrants = m_worldQuadrantsMap.size();
	m_mtxQuadTreeAndMainProcessing.unlock();

	GDN_TheWorld_Map* map = Map();
	std::string mapQuadrantsMessage = "MAP QUADRANTS\n"
		+ std::string("Num Map Quadrants = ") + std::to_string(m_mapQuadrantsMap.size())
		+ std::string(" Num Updated Map Quadrants = ") + std::to_string(fetchUpdatedMapQuadrants())
		+ std::string( "Num Map Texture Rects = ") + std::to_string(map->getNumTextureRects());

	std::string s;
	if (m_streamingTime.counterStarted())
	{
		s = "\nWORLD QUADRANTS\nInitialization of last stream of quadrant COMPLETED:" + std::string("\n")
			+ "  num streamed = " + std::to_string(num) + " total quadrants = " + std::to_string(numQuadrants) + " elapsed = " + std::to_string(m_streamingTime.partialDuration().count()) + " average = " + std::to_string(m_streamingTime.partialDuration().count() / (num == 0 ? 1 : num)) + "\n"
			+ "  visible = " + std::to_string(m_numVisibleQuadrantsStreamed) + "\n"
			+ "  invisible = " + std::to_string(m_numInvisibleQuadrantsStreamed) + "\n";
	}
	else
	{
		s = "\nWORLD QUADRANTS\nNo initialization of quadrants during last time lapse (" + std::to_string(m_notStreamingTime.partialDuration().count()) + ")" + std::string("\n");
	}
	
	PLOG_DEBUG << s
		<< "  opt. level change = " << std::to_string(m_numOptimizationLevelChange)
		<< "  texture resize = " << m_numTextureResizing << "  elapsed = " << std::to_string(m_elapsedTextureResizing) << " average = " << std::to_string(m_elapsedTextureResizing / (m_numTextureResizing == 0 ? 1 : m_numTextureResizing)) << "\n"
		<< "  load from cache = " << m_numLoadTerrainData << "  elapsed = " << std::to_string(m_elapsedLoadTerrainData) << " average = " << std::to_string(m_elapsedLoadTerrainData / (m_numLoadTerrainData == 0 ? 1 : m_numLoadTerrainData)) << "\n"
		<< "  reset shader params = " << m_numResetShaderParam << "  elapsed = " << std::to_string(m_elapsedResetShaderParam) << " average = " << std::to_string(m_elapsedResetShaderParam / (m_numResetShaderParam == 0 ? 1 : m_numResetShaderParam)) << "\n"
		<< "  update shader params = " << m_numUpdateShaderParam << "  elapsed = " << std::to_string(m_elapsedUpdateShaderParam) << " average = " << std::to_string(m_elapsedUpdateShaderParam / (m_numUpdateShaderParam == 0 ? 1 : m_numUpdateShaderParam)) << "\n"
		<< "  set hires collisions = " << m_numSetHiResCollisionData << "  elapsed = " << std::to_string(m_elapsedSetHiResCollisionData) << " average = " << std::to_string(m_elapsedSetHiResCollisionData / (m_numSetHiResCollisionData == 0 ? 1 : m_numSetHiResCollisionData)) << "\n"
		<< "  set lowres collisions = " << m_numSetLowResCollisionData << "  elapsed = " << std::to_string(m_elapsedSetLowResCollisionData) << " average = " << std::to_string(m_elapsedSetLowResCollisionData / (m_numSetLowResCollisionData == 0 ? 1 : m_numSetLowResCollisionData)) << "\n"
		<< mapQuadrantsMessage;

	m_numVisibleQuadrantsStreamed = 0;
	m_numInvisibleQuadrantsStreamed = 0;
	m_numTextureResizing = 0;
	m_elapsedTextureResizing = 0;
	m_numOptimizationLevelChange = 0;
	m_numLoadTerrainData = 0;
	m_elapsedLoadTerrainData = 0;
	m_numResetShaderParam = 0;
	m_elapsedResetShaderParam = 0;
	m_numUpdateShaderParam = 0;
	m_elapsedUpdateShaderParam = 0;
	m_numSetHiResCollisionData = 0;
	m_elapsedSetHiResCollisionData = 0;
	m_numSetLowResCollisionData = 0;
	m_elapsedSetLowResCollisionData = 0;
}

godot::Transform3D GDN_TheWorld_Viewer::globalToWorld(godot::Transform3D global) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	return viewerT * global;
}

godot::Transform2D GDN_TheWorld_Viewer::globalToMap(godot::Transform2D global)
{
	//godot::Transform3D viewerT3d = getInternalGlobalTransform().affine_inverse();
	//godot::Transform2D viewerT2d;
	//viewerT2d.set_origin(godot::Vector2(viewerT3d.get_origin().x, viewerT3d.get_origin().z));
	//return viewerT2d * global;
	GDN_TheWorld_Map* map = Map();
	godot::Transform2D viewerT2d = map->get_transform();
	viewerT2d.set_origin(globalPosToMapPos(global.get_origin()));
	return viewerT2d;
}

godot::Transform3D GDN_TheWorld_Viewer::globalPosToWorldTransform(godot::Vector3 global) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	return viewerT * godot::Transform3D(godot::Basis(), global);
}

godot::Vector3 GDN_TheWorld_Viewer::globalPosToWorldPos(godot::Vector3 global) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	godot::Transform3D t = viewerT * godot::Transform3D(godot::Basis(), global);
	return t.origin;
}

godot::Vector2 GDN_TheWorld_Viewer::globalPosToMapPos(godot::Vector2 global)
{
	GDN_TheWorld_Map* map = Map();
	return map->globalPosToMapPos(global);
	//godot::Vector2 mapPos = global / map->getTextureRectSize();
	//mapPos = mapPos * Globals()->heightmapSizeInWUs();
	//return mapPos;
}

godot::Transform3D GDN_TheWorld_Viewer::worldToGlobal(godot::Transform3D map) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	return viewerT * map;
}

godot::Transform3D GDN_TheWorld_Viewer::worldPosToGlobalTransform(godot::Vector3 map) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	return viewerT * godot::Transform3D(godot::Basis(), map);
}

godot::Vector3 GDN_TheWorld_Viewer::worldPosToGlobalPos(godot::Vector3 map) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	godot::Transform3D t = viewerT * godot::Transform3D(godot::Basis(), map);
	return t.origin;
}

godot::Vector2 GDN_TheWorld_Viewer::mapPosToGlobalPos(godot::Vector2 mapPos)
{
	GDN_TheWorld_Map* map = Map();
	return map->mapPosToGlobalPos(mapPos);
	//godot::Vector2 globalPos = mapPos / Globals()->heightmapSizeInWUs();
	//globalPos = globalPos * map->getTextureRectSize();
	//return globalPos;
}

void GDN_TheWorld_Viewer::process_updateOriginShift(godot::Camera2D* activeMapCamera, godot::Camera3D* activeWorldCamera)
{
	//return;
	GDN_TheWorld_Globals* globals = Globals();

	int64_t paramOriginShiftFactor = globals->getParamOriginShiftFactor();
	if (paramOriginShiftFactor == 0)
		return;

	if (IS_EDITOR_HINT())
		return;
	
	godot::Node3D* anchorForOriginShift = getAnchorForOriginShift();
	if (anchorForOriginShift == nullptr)
		return;

	if (globals->status() != TheWorldStatus::worldDeployed)
		return;
	
	if (m_streamingInProgress)
		return;

	float distance;
	godot::Transform3D cameraWorldTransform;
	godot::Vector2 cameraMapPos;
	if (worldMode())
	{
		cameraWorldTransform = globalPosToWorldTransform(activeWorldCamera->get_global_position() * godot::Vector3(1, 0, 1));
		distance = cameraWorldTransform.origin.distance_to(m_lastOriginShiftCameraPosInMap);
	}
	else
	{
		godot::Vector2 lastOriginShiftCameraPosInMap(m_lastOriginShiftCameraPosInMap.x, m_lastOriginShiftCameraPosInMap.z);
		cameraMapPos = globalPosToMapPos(activeMapCamera->get_global_position());
		distance = cameraMapPos.distance_to(lastOriginShiftCameraPosInMap);
	}

	//if (m_originShiftedQuadrantPos != m_computedCameraQuadrantPos)
	if (distance > globals->heightmapSizeInWUs() * paramOriginShiftFactor)
	{
		godot::Vector3 originShift;
		originShift.x = -m_computedCameraQuadrantPos.getLowerXGridVertex();
		originShift.y = 0;
		originShift.z = -m_computedCameraQuadrantPos.getLowerZGridVertex();

		//godot::Vector3 pos1 = activeCamera->get_global_position();	// DEBUG ONLY

		// shift anchor
		//godot::Vector3 pos = anchorForOriginShift->get_global_position();	// DEBUG ONLY
		//pos += originShift;
		//anchorForOriginShift->set_global_position(pos);
		anchorForOriginShift->set_global_position(originShift);

		//godot::Vector3 pos2 = activeCamera->get_global_position();	// DEBUG ONLY

		// shift camera
		//pos = activeCamera->get_global_position();
		//pos += originShift;
		//activeCamera->set_global_position(pos);

		// shift all chunks of all quadrants
		//Chunk::TransformChangedChunkAction action;
		//for (auto& itQuadTree : m_worldQuadrantsMap)
		//	itQuadTree.second->ForAllChunk(action);

		m_originShiftedQuadrantPos = m_computedCameraQuadrantPos;
		if (worldMode())
			m_lastOriginShiftCameraPosInMap = cameraWorldTransform.origin;
		else
			m_lastOriginShiftCameraPosInMap = godot::Vector3(cameraMapPos.x, 0, cameraMapPos.y);

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (m_editMode && editModeUIControl && editModeUIControl->initialized())
		{
			editModeUIControl->setQuadrantHit(nullptr);
		}

	}
}

void GDN_TheWorld_Viewer::pauseMouseMovement(void)
{
	GDN_TheWorld_MapCamera* mapCamera = MapCamera();
	if (mapCamera != nullptr)
		mapCamera->pauseMouseMovement();
	GDN_TheWorld_Camera* worldCamera = WorldCamera();
	if (worldCamera != nullptr)
		worldCamera->pauseMouseMovement();
}
void GDN_TheWorld_Viewer::resumeMouseMovement(void)
{
	GDN_TheWorld_MapCamera* mapCamera = MapCamera();
	if (mapCamera != nullptr)
		mapCamera->resumeMouseMovement();
	GDN_TheWorld_Camera* worldCamera = WorldCamera();
	if (worldCamera != nullptr)
		worldCamera->resumeMouseMovement();
}

void GDN_TheWorld_Viewer::setHudMessage(godot::String _hudMessage, int _fadingTime)
{
	hudMessage(_hudMessage.utf8().get_data(), _fadingTime);
}

void GDN_TheWorld_Viewer::hudMessage(std::string hudMessage, int64_t fadingTime)
{
	m_hudMessage = hudMessage;
	m_fadingTime = fadingTime;
	m_savedTimeToFadeMessage = 0;
	if (m_fadingTime > 0)
		m_savedTimeToFadeMessage = TIME()->get_ticks_msec();
}

void GDN_TheWorld_Viewer::process_updateHud(godot::Camera3D* activeWorldCamera)
{
	//GDN_TheWorld_Globals* globals = Globals();

	if (m_hudMessageLabel != nullptr)
	{
		//std::string currentHudMessage = m_hudMessageLabel->get_text().utf8().get_data();

		if (m_savedTimeToFadeMessage > 0)
		{
			if (TIME()->get_ticks_msec() - m_savedTimeToFadeMessage > m_fadingTime)
			{
				m_hudMessage = "";
				m_savedTimeToFadeMessage = 0;
			}
		}

		m_hudMessageLabel->set_text(m_hudMessage.c_str());
	}

	if (m_hudStreamingInfoLabel != nullptr)
	{
		std::string hudStreaming;
		int numInitializedQuadrant = getNumInitializedQuadrant();
		int numQuadrant = getNumQuadrant();
		if (numInitializedQuadrant == numQuadrant)
			hudStreaming = "";
		else
		{
			int progress = numInitializedQuadrant * 100 / numQuadrant;
			hudStreaming = "Streaming World " + std::to_string(progress) + "%";
		}
		m_hudStreamingInfoLabel->set_text(hudStreaming.c_str());
	}
	
	if (m_backMirrorSubViewportContainer != nullptr and m_backMirrorCamera != nullptr)
	{
		if (m_backMirrorSubViewportContainer->is_visible())
		{
			m_backMirrorCamera->set_global_transform(activeWorldCamera->get_global_transform());
			m_backMirrorCamera->set_near(activeWorldCamera->get_near());
			m_backMirrorCamera->set_far(activeWorldCamera->get_far());
			GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
			if (cam != nullptr)
			{
				if (cam->trackingPlayer())
				{
					m_backMirrorCamera->rotate_y(TheWorld_Utils::kPi);
				}
				else
				{
					//godot::Vector3 activeCameraDirection = -activeCamera->get_global_transform().basis.rows[godot::Vector3::Axis::AXIS_Z];
					//m_backMirrorCamera->look_at(m_backMirrorCamera->get_position() - activeCameraDirection);
					godot::Vector3 activeCameraDirection = -activeWorldCamera->get_global_transform().basis.rows[godot::Vector3::Axis::AXIS_Z];
					m_backMirrorCamera->look_at(m_backMirrorCamera->get_position() + activeCameraDirection);
				}
			}
		}
	}

	if (m_satelliteSubViewportContainer != nullptr and m_satelliteCamera != nullptr)
	{
		if (m_satelliteSubViewportContainer->is_visible())
		{
			godot::Transform3D t = m_satelliteCamera->get_global_transform();
			godot::Basis b;
			t.origin = (activeWorldCamera->get_global_position() * godot::Vector3(1, 0, 1)) + godot::Vector3(0, SATELLITE_CAMERA_ALTITUDE, 0);
																											//t.basis = b.rotated(godot::Vector3(1, 0, 0), -TheWorld_Utils::kPi2);			// fixed pointing on north direction
																											//t.basis = b;
																											//t.basis.rotate(godot::Vector3(1, 0, 0), -TheWorld_Utils::kPi2);
			// camera pointing vertically on terrain upper border on north direction
			t.basis.rows[godot::Vector3::Axis::AXIS_X] = godot::Vector3(1, 0, 0);
			t.basis.rows[godot::Vector3::Axis::AXIS_Y] = godot::Vector3(0, 0, 1);
			t.basis.rows[godot::Vector3::Axis::AXIS_Z] = godot::Vector3(0, -1, 0);
			// moving around as the cam rotate
			//GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeCamera);
			//if (cam != nullptr)
			//	t.basis = t.basis.rotated(godot::Vector3(0, 1, 0), (real_t)TheWorld_Utils::deg_to_rad(-cam->getAngleFromNorthDeg()));	// pointing on same direction on plane XZ as activeCamera
			m_satelliteCamera->set_global_transform(t);

			int lowerNumVisibleQuadrantOnEachSide = m_numVisibleQuadrantOnPerimeterOnNorthSide;
			if (m_numVisibleQuadrantOnPerimeterOnSouthSide < lowerNumVisibleQuadrantOnEachSide)
				lowerNumVisibleQuadrantOnEachSide = m_numVisibleQuadrantOnPerimeterOnSouthSide;
			if (m_numVisibleQuadrantOnPerimeterOnEastSide < lowerNumVisibleQuadrantOnEachSide)
				lowerNumVisibleQuadrantOnEachSide = m_numVisibleQuadrantOnPerimeterOnEastSide;
			if (m_numVisibleQuadrantOnPerimeterOnWestSide < lowerNumVisibleQuadrantOnEachSide)
				lowerNumVisibleQuadrantOnEachSide = m_numVisibleQuadrantOnPerimeterOnWestSide;
			
			double fov = 40.5 + (lowerNumVisibleQuadrantOnEachSide * 5.75);
			m_satelliteCamera->set_fov(fov);
		}
	}
}

void GDN_TheWorld_Viewer::process_updateCameraPosInShader(godot::Camera3D* activeWorldCamera)
{
	godot::RenderingServer* rs = godot::RenderingServer::get_singleton();

	GDN_TheWorld_Globals* globals = Globals();
	
	if (globals->status() != TheWorldStatus::worldDeployed)
		return;

	GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
	if (cam != nullptr)
	{
		float camAltitude = cam->cameraAltitudeFromTerrain();
		
		if (m_lastCameraAltitudeSetToShader == -1.0f)
			cam->forceRecalcCameraAltitude();

		if (camAltitude != m_lastCameraAltitudeSetToShader)
		{
			rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_ALTITUDE, camAltitude);
			m_lastCameraAltitudeSetToShader = camAltitude;
		}

		godot::Vector3 cameraPos = cam->get_global_position();
		if (!m_lastCameraPosSetToShaderInitialized || cameraPos != m_lastCameraPosSetToShader)
		{
			rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_POS, cameraPos);
			m_lastCameraPosSetToShader = cameraPos;
			m_lastCameraPosSetToShaderInitialized = true;
		}

		bool cameraTerrainChunkContainsSea = false;
		if (m_cameraTerrainChunk != nullptr)
			cameraTerrainChunkContainsSea = m_cameraTerrainChunk->containsSea();
		if (m_cameraVerticalOnSea != cameraTerrainChunkContainsSea)
		{
			m_cameraVerticalOnSea = cameraTerrainChunkContainsSea;
			rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_ON_SEA, m_cameraVerticalOnSea);
		}

		//shaderParamCameraUnderWaterApplied(cam->cameraUnderWater());
	}
}

void GDN_TheWorld_Viewer::process_castRayFromCamera(godot::Camera3D* activeWorldCamera)
{
	if (m_rayFromCameraDrawer == nullptr || m_lineRayFromCameraIdx == -1 || m_sphereRayFromCameraStartIdx == -1 || m_sphereRayFromCameraEndIdx == -1)
		return;
	
	if (castRayFromCamera())
	{
		godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
		godot::Dictionary rayArray;
		godot::Vector2 mousePosInViewport = getMousePosInViewport();
		godot::Vector3 rayOrigin = activeWorldCamera->get_global_position();
		godot::Vector3 rayEnd = rayOrigin + activeWorldCamera->project_ray_normal(mousePosInViewport) * (float)activeWorldCamera->get_far() * 1.5;
		godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd);
		//query->set_collide_with_bodies(true);	// DEFAULT
		rayArray = spaceState->intersect_ray(query);

		godot::Vector3 hitPosition;
		godot::Vector3 hitPositionInMap;
		bool hit = false;
		if (rayArray.has("position"))
		{
			hitPosition = rayArray["position"];
			hitPositionInMap = globalPosToWorldPos(hitPosition);
			hit = true;
		}

		if (hit)
		{
			float hitDistance = (float)rayOrigin.distance_to(hitPosition);
			godot::Vector3 hitDirection = (hitPosition - activeWorldCamera->get_global_position()).normalized();
			
			godot::Vector3 northDirection(0, 0, -1);
			float degreeWithNorthDirection = (northDirection.angle_to(godot::Vector3(hitDirection.x, 0, hitDirection.z)) * 180) / TheWorld_Utils::kPi;
			if (hitDirection.x < 0)
				degreeWithNorthDirection = 360 - degreeWithNorthDirection;

			godot::Vector3 upDirection(0, 1, 0);
			float pitch = 90 - (upDirection.angle_to(hitDirection) * 180) / TheWorld_Utils::kPi;

			float distanceFromCameraInQuads = hitDistance / Globals()->heightmapSizeInWUs();

			std::string text = "";
			if (!positionVisible())
				text = "   Dist=" + std::format("{:.2f}", hitDistance) + " quads=" + std::format("{:.2f}", distanceFromCameraInQuads) + " Deg=" + std::format("{:.2f}", degreeWithNorthDirection) + "° pitch=" + std::format("{:.2f}", pitch) + "°";
			m_rayFromCameraDrawer->updateLine(m_lineRayFromCameraIdx, globalPosToWorldPos(activeWorldCamera->get_global_position()) + godot::Vector3(0, -1.0f, 0), hitPositionInMap, text);
			
			//m_rayFromCameraDrawer->updateSphere(m_sphereRayFromCameraStartIdx, globalPosToWorldPos(rayOrigin), 5);
			m_rayFromCameraDrawer->updateSphere(m_sphereRayFromCameraEndIdx, hitPositionInMap, 5);
			m_rayFromCameraDrawer->set_visible(true);
		}
		else
			m_rayFromCameraDrawer->set_visible(false);
	}
	else
		m_rayFromCameraDrawer->set_visible(false);
}

void GDN_TheWorld_Viewer::process_trackMouse(godot::Camera3D* activeWorldCamera)
{
	if (mapMode())
	{
		m_mouseTrackedOnSomething = false;

		m_mouseQuadrantHitTag = "";

		GDN_TheWorld_Map* map = Map();
		if (map != nullptr)
		{
			godot::Vector2 mousePosGlobal = map->getMousePosInGlobalCoord();
			m_trackedMouseHitGlobal = godot::Vector3(mousePosGlobal.x, 0, mousePosGlobal.y);

			godot::Vector2 mousePosInMap = map->globalPosToMapPos(mousePosGlobal);
			m_trackedMouseHitInMap = godot::Vector3(mousePosInMap.x, 0, mousePosInMap.y);

			int numVerticesPerQuadrantSize = (int)Globals()->numVerticesPerQuadrantSize();
			float gridStepInWU = Globals()->gridStepInWU();
			TheWorld_Utils::QuadrantPos pos(mousePosInMap.x, mousePosInMap.y, m_worldViewerLevel, numVerticesPerQuadrantSize, gridStepInWU);
			m_mouseQuadrantHitName = pos.getName();
			m_mouseQuadrantHitPos = godot::Vector3(pos.getLowerXGridVertex(), 0, pos.getLowerZGridVertex());
		}
	}
	else
	{
		godot::Node* collider = nullptr;
		if (trackMouse())
		{
			compressTrackedInputEvents(&m_trackedInputEventsForTrackMouse);

			int64_t timeElapsed = TIME()->get_ticks_msec();
			if (timeElapsed - m_timeElapsedFromLastMouseTrack > TIME_INTERVAL_BETWEEN_MOUSE_TRACK)
			{
				TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Track Mouse Chunk", false, false);
				clock1.tick();
				auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
					clock1.tock();
					this->m_lastMouseTrackHitDuration = clock1.duration().count();
					this->m_numMouseTrackHit++;
					this->m_sumMouseTrackHitDuration += this->m_lastMouseTrackHitDuration;
					});

				godot::PhysicsDirectSpaceState3D* spaceState = get_world_3d()->get_direct_space_state();
				godot::Dictionary rayArray;
				godot::Vector2 mousePosInViewport = getMousePosInViewport();
				godot::Vector3 rayOrigin = activeWorldCamera->project_ray_origin(mousePosInViewport);
				godot::Vector3 rayEnd = rayOrigin + activeWorldCamera->project_ray_normal(mousePosInViewport) * (float)activeWorldCamera->get_far() * 1.5;
				godot::Ref<godot::PhysicsRayQueryParameters3D> query = PhysicsRayQueryParameters3D::create(rayOrigin, rayEnd);
				//query->set_collide_with_bodies(true);	// DEFAULT
				rayArray = spaceState->intersect_ray(query);

				GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

				m_mouseTrackedOnTerrain = false;
				if (rayArray.has("position"))
				{
					m_trackedMouseHitGlobal = rayArray["position"];
					godot::Transform3D t(godot::Basis(), m_trackedMouseHitGlobal);
					m_trackedMouseHitInMap = globalToWorld(t).origin;
					m_mouseTrackedOnSomething = true;
				}
				else
					m_mouseTrackedOnSomething = false;

				if (m_mouseTrackedOnSomething)
				{
					if (m_editMode && editModeUIControl && editModeUIControl->initialized())
						editModeUIControl->setMouseHitLabelText(std::string("X=") + std::format("{:.2f}", m_trackedMouseHitInMap.x) + " Y=" + std::format("{:.2f}", m_trackedMouseHitInMap.y) + " Z=" + std::format("{:.2f}", m_trackedMouseHitInMap.z));

					godot::Vector3 normalHit = Vector3Zero;
					if (rayArray.has("normal"))
						normalHit = rayArray["normal"];

					if (rayArray.has("collider"))
					{
						collider = godot::Object::cast_to<godot::Node>(rayArray["collider"]);
						if (collider->has_meta("QuadrantName"))
						{
							m_mouseTrackedOnTerrain = true;
							godot::String s = collider->get_meta("QuadrantName", "");
							godot::PackedByteArray array = s.to_ascii_buffer();
							std::string mouseQuadrantHitName((char*)array.ptr(), array.size());
							//const char* str = s.utf8().get_data();
							//std::string mouseQuadrantHitName = str;
							if (mouseQuadrantHitName != m_mouseQuadrantHitName)
							{
								/*s = collider->get_meta("QuadrantTag", "");
								str = s.alloc_c_string();
								std::string mouseQuadrantHitTag = str;
								godot::api->godot_free(str);*/
								godot::Vector3 mouseQuadrantHitPos = collider->get_meta("QuadrantOrig", Vector3());
								float mouseQuadrantHitSize = collider->get_meta("QuadrantSize", 0.0);
								float gridStepInWu = collider->get_meta("QuadrantStep");
								int level = collider->get_meta("QuadrantLevel");
								int numVerticesPerSize = collider->get_meta("QuadrantNumVert");
								TheWorld_Utils::QuadrantPos quadrantHitPos((float)mouseQuadrantHitPos.x, (float)mouseQuadrantHitPos.z, level, numVerticesPerSize, gridStepInWu);
								//quadrantHitPos.setTag(mouseQuadrantHitTag);

								trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, false, godot::Vector3());

								// Get current quadrant
								std::string mouseQuadrantHitTag;
								QuadTree* quadrantHit = nullptr;
								std::list<QuadTree*> adjacentQuadrantsHit;
								WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(quadrantHitPos);
								if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
								{
									mouseQuadrantHitTag = it->second->getTag();

									quadrantHit = it->second.get();
									quadrantHit->getAdjacentQuadrants(adjacentQuadrantsHit);
									trackedMouseHitChanged(activeWorldCamera, quadrantHit, adjacentQuadrantsHit, true, m_trackedMouseHitInMap, normalHit);
								}

								if (m_editMode && editModeUIControl && editModeUIControl->initialized())
								{
									editModeUIControl->setQuadrantHit(quadrantHit);
									editModeUIControl->setMouseQuadHitLabelText(mouseQuadrantHitName + " " + mouseQuadrantHitTag);
									editModeUIControl->setMouseQuadHitPosLabelText(std::string("X=") + std::format("{:.2f}", mouseQuadrantHitPos.x) + " Z=" + std::format("{:.2f}", mouseQuadrantHitPos.z) + " " + std::format("{:.2f}", mouseQuadrantHitSize));

									//// Get TerrainEdit Data from hit quadrant
									//WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(quadrantHitPos);
									//if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
									//{
									//	TerrainEdit* terrainEdit = it->second->getQuadrant()->getTerrainEdit();
									//	editModeUIControl->setSeed(terrainEdit->noiseSeed);
									//	editModeUIControl->setFrequency(terrainEdit->frequency);
									//	editModeUIControl->setOctaves(terrainEdit->fractalOctaves);
									//	editModeUIControl->setLacunarity(terrainEdit->fractalLacunarity);
									//	editModeUIControl->setGain(terrainEdit->fractalGain);
									//	editModeUIControl->setWeightedStrength(terrainEdit->fractalWeightedStrength);
									//	editModeUIControl->setPingPongStrength(terrainEdit->fractalPingPongStrength);
									//	editModeUIControl->setAmplitude(terrainEdit->amplitude);
									//	editModeUIControl->setMinHeight(terrainEdit->minHeight);
									//	editModeUIControl->setMaxHeight(terrainEdit->maxHeight);
									//	//editModeUIControl->setElapsed(0);
									//}
								}

								m_mouseQuadrantHitName = mouseQuadrantHitName;
								m_mouseQuadrantHitTag = mouseQuadrantHitTag;
								m_mouseQuadrantHitPos = mouseQuadrantHitPos;
								m_mouseQuadrantHitSize = mouseQuadrantHitSize;
								m_quadrantHitPos = quadrantHitPos;
								m_quadrantHit = quadrantHit;
								m_adjacentQuadrantsHit.clear();
								for (auto& item : adjacentQuadrantsHit)
									m_adjacentQuadrantsHit.push_back(item);
							}
							else
							{
								trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, true, m_trackedMouseHitInMap, normalHit);
							}
						}
						else
						{
							trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, true, m_trackedMouseHitInMap, normalHit);
						}
					}
				}
				else
				{
					trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, false, godot::Vector3());

					m_mouseQuadrantHitName = "";
					m_mouseQuadrantHitTag = "";
					m_mouseQuadrantHitPos = godot::Vector3();
					m_mouseQuadrantHitSize = 0.0f;
					m_quadrantHitPos = TheWorld_Utils::QuadrantPos();
					m_quadrantHit = nullptr;
					m_adjacentQuadrantsHit.clear();
					m_previousTrackedMouseHitInMap = godot::Vector3();
				}

				resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse);

				m_timeElapsedFromLastMouseTrack = timeElapsed;
			}
		}
		else
		{
			resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse);

			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (m_editMode && editModeUIControl && editModeUIControl->initialized())
			{
				editModeUIControl->setQuadrantHit(nullptr);
			}
		}
	}
}

void GDN_TheWorld_Viewer::trackedMouseHitChanged(godot::Camera3D* activeWorldCamera, QuadTree* quadrantHit, std::list<QuadTree*>& adjacentQuadrantsHit, bool hit, godot::Vector3 mouseHitInMap, godot::Vector3 normalHit)
{

	//if (quadrantHit != nullptr)
	//{
		if (hit)
		{
			if (positionVisible())
			{
				if (isInputEventTracked(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_LEFT, &m_trackedInputEventsForTrackMouse))
					trackingDistance(true, mouseHitInMap);
				if (isInputEventTracked(TrackedInputEvent::MOUSE_DOUBLE_BUTTON_RIGHT, &m_trackedInputEventsForTrackMouse))
					trackingDistance(false);
			}

			if (m_previousTrackedMouseHitInMap != mouseHitInMap)
			{
				if (quadrantHit != nullptr)
					quadrantHit->mouseHitOnTerrainChanged(mouseHitInMap, true);
				for (auto& item : adjacentQuadrantsHit)
					item->mouseHitOnTerrainChanged(mouseHitInMap, true);

				bool _normalVisible = false;
				if (normalVisible())
				{
					godot::Vector3 normal = normalHit;
					if (normal == Vector3Zero && quadrantHit != nullptr)
					{
						size_t index = quadrantHit->getQuadrant()->getIndexFromMap((float)mouseHitInMap.x, (float)mouseHitInMap.z);
						if (index != -1)
						{
							bool ok;
							normal = quadrantHit->getQuadrant()->getNormalFromNormalmap(index, ok);
							if (!ok)
								normal = Vector3Zero;
						}
					}
					if (normal != Vector3Zero)
					{
						const float lineSize = 50;
						godot::Vector3 end = mouseHitInMap + normal * lineSize;
						float radAngleWithVertical = (float)acos(normal.dot(Vector3Up));
						float slope = (radAngleWithVertical * 180) / TheWorld_Utils::kPi;

						if (m_normalLineIdx != -1 && m_normalSphereIdx != -1)
						{
							_normalVisible = true;
							m_normalDrawer->updateLine(m_normalLineIdx, mouseHitInMap, end, std::to_string(int32_t(slope)) + "°");
							m_normalDrawer->updateSphere(m_normalSphereIdx, end);
						}
					}
					
					//size_t index = quadrantHit->getQuadrant()->getIndexFromMap(mouseHitInMap.x, mouseHitInMap.z);
					//if (index != -1)
					//{
					//	bool ok;
					//	godot::Vector3 normal = quadrantHit->getQuadrant()->getNormalFromNormalmap(index, ok);
					//	if (ok)
					//	{
					//		const float lineSize = 50;
					//		godot::Vector3 end = mouseHitInMap + normal * lineSize;
					//		float radAngleWithVertical = acos(normal.dot(Vector3Up));
					//		float slope = (radAngleWithVertical * 180) / TheWorld_Utils::kPi;
					//		
					//		if (m_normalLineIdx != -1 && m_normalSphereIdx != -1)
					//		{
					//			_normalVisible = true;
					//			m_normalDrawer->updateLine(m_normalLineIdx, mouseHitInMap, end, std::to_string(int32_t(slope)) + "°");
					//			m_normalDrawer->updateSphere(m_normalSphereIdx, end);
					//		}
					//	}
					//}
				}

				if (_normalVisible)
					m_normalDrawer->set_visible(true);
				else
					m_normalDrawer->set_visible(false);

				bool _positionVisible = false;
				if (positionVisible())
				{
					if (m_labelPositionXIdx != -1 && m_labelPositionYIdx != -1 && m_labelPositionZIdx != -1 && m_labelPositionDistIdx != -1 && m_labelPositionDirIdx != -1)
					{
						_positionVisible = true;

						m_positionDrawer->updateLabel2d(m_labelPositionXIdx, mouseHitInMap, std::string("X=") + std::format("{:.2f}", mouseHitInMap.x));
						m_positionDrawer->updateLabel2d(m_labelPositionZIdx, mouseHitInMap, std::string("Z=") + std::format("{:.2f}", mouseHitInMap.z));
						m_positionDrawer->updateLabel2d(m_labelPositionYIdx, mouseHitInMap, std::string("H=") + std::format("{:.2f}", mouseHitInMap.y));

						godot::Vector3 cameraPosInMap = globalPosToWorldPos(activeWorldCamera->get_global_position());
						float distanceFromCamera = (float)cameraPosInMap.distance_to(mouseHitInMap);
						float distanceFromCameraInQuads = distanceFromCamera / Globals()->heightmapSizeInWUs();
						m_positionDrawer->updateLabel2d(m_labelPositionDistIdx, mouseHitInMap, std::string("Dist=") + std::format("{:.2f}", distanceFromCamera) + " quads=" + std::format("{:.2f}", distanceFromCameraInQuads));

						godot::Vector3 hitDirection = (mouseHitInMap - cameraPosInMap).normalized();
						godot::Vector3 northDirection(0, 0, -1);
						float degreeWithNorthDirection = (northDirection.angle_to(godot::Vector3(hitDirection.x, 0, hitDirection.z)) * 180) / TheWorld_Utils::kPi;
						if (mouseHitInMap.x < 0)
							degreeWithNorthDirection = 360 - degreeWithNorthDirection;

						godot::Vector3 upDirection(0, 1, 0);
						float pitch = 90 - (upDirection.angle_to(hitDirection) * 180) / TheWorld_Utils::kPi;

						std::string text = "Deg=" + std::format("{:.2f}", degreeWithNorthDirection) + "° pitch=" + std::format("{:.2f}", pitch) + "°";
						m_positionDrawer->updateLabel2d(m_labelPositionDirIdx, mouseHitInMap, text);
					}

					if (trackingDistance())
					{
						float distance = (float)mouseHitInMap.distance_to(m_savedPosForTrackingDistance);
						float distanceInQuads = distance / Globals()->heightmapSizeInWUs();
						std::string text = "Dist=" + std::format("{:.2f}", distance) + " quads=" + std::format("{:.2f}", distanceInQuads);
						m_positionDrawer->updateLine(m_linePositionIdx, m_savedPosForTrackingDistance, mouseHitInMap, text);
						m_positionDrawer->updateSphere(m_spherePositionStartIdx, m_savedPosForTrackingDistance, 5);
						m_positionDrawer->updateSphere(m_spherePositionEndIdx, mouseHitInMap, 5);
					}
				}

				if (_positionVisible)
					m_positionDrawer->set_visible(true);
				else
					m_positionDrawer->set_visible(false);

				if (gizmoVisible())
				{
					m_gizmo->set_visible(true);
					m_gizmo->set_global_position(worldPosToGlobalPos(mouseHitInMap));
				}
				else
					m_gizmo->set_visible(false);

				m_previousTrackedMouseHitInMap = mouseHitInMap;
			}
		}
		else
		{
			if (quadrantHit != nullptr)
				quadrantHit->mouseHitOnTerrainChanged(godot::Vector3(), false);
			for (auto& item : adjacentQuadrantsHit)
				item->mouseHitOnTerrainChanged(godot::Vector3(), false);
			m_normalDrawer->set_visible(false);
			m_positionDrawer->set_visible(false);
			m_gizmo->set_visible(false);
		}
	//}
}

bool GDN_TheWorld_Viewer::mouseInsideMainEditPanel(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->initialized())
		return editModeUIControl->mouseInsideEditPanel();
	else
		return false;
}

void GDN_TheWorld_Viewer::process_updateLodInQuads(Vector3& cameraPosInMap)
{
	for (int i = 0; i < (int)QuadType::NotUsed; i++)
	{
		process_updateLodInQuads(cameraPosInMap, (QuadType)i);
	}
}

void GDN_TheWorld_Viewer::process_updateLodInQuads(Vector3& cameraPosInMap, enum class QuadType type)
{
	// UPDATE QUADS - Stage 1
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09.1 ") + __FUNCTION__, "Update quadrants Stage 1");

		TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 1", false, false);
		clock1.tick();
		auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
			clock1.tock();
			this->m_lastUpdateQuads1Duration = clock1.duration().count();
			this->m_numUpdateQuads1++;
			this->m_sumUpdateQuads1Duration += this->m_lastUpdateQuads1Duration;
			});

		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			if (!itQuadTree->second->containsQuadOfType(type))
				continue;

			int numSplitRequired = 0;
			itQuadTree->second->update(cameraPosInMap, QuadTree::UpdateStage::Stage1, numSplitRequired, type);
		}
	}

	while (true)
	{
		int numSplitRequired = 0;

		// UPDATE QUADS - Stage 2
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09.2 ") + __FUNCTION__, "Update quadrants Stage 2");

			TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 2", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
				this->m_lastUpdateQuads2Duration = clock1.duration().count();
				this->m_numUpdateQuads2++;
				this->m_sumUpdateQuads2Duration += this->m_lastUpdateQuads2Duration;
				});


			for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			{
				if (!itQuadTree->second->containsQuadOfType(type))
					continue;

				itQuadTree->second->update(Vector3(), QuadTree::UpdateStage::Stage2, numSplitRequired, type);
			}
		}

		if (numSplitRequired == 0)
			break;
		else
		{
			// UPDATE QUADS - Stage 3
			TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09.3 ") + __FUNCTION__, "Update quadrants Stage 3");

			TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Quads Stage 3", false, false);
			clock1.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
				clock1.tock();
				this->m_lastUpdateQuads3Duration = clock1.duration().count();
				this->m_numUpdateQuads3++;
				this->m_sumUpdateQuads3Duration += this->m_lastUpdateQuads3Duration;
				});

			for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			{
				if (!itQuadTree->second->containsQuadOfType(type))
					continue;

				int numSplitRequired = 0;
				itQuadTree->second->update(cameraPosInMap, QuadTree::UpdateStage::Stage3, numSplitRequired, type);
			}
		}
	}

	if (m_refreshRequired)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09.4 ") + __FUNCTION__, "Refresh required");

		Chunk::RefreshChunkAction action(m_visibleInTree);
		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			if (!itQuadTree->second->containsQuadOfType(type))
				continue;

			if (itQuadTree->second->getQuadrant()->internalDataLocked())
				continue;

			itQuadTree->second->ForAllChunk(action, type);
		}
		m_refreshRequired = false;
	}
}
void GDN_TheWorld_Viewer::process_updateMeshForUpdatedChunks(void)
{
	for (int i = 0; i < (int)QuadType::NotUsed; i++)
	{
		process_updateMeshForUpdatedChunks((QuadType)i);
	}
}

void GDN_TheWorld_Viewer::process_updateMeshForUpdatedChunks(enum class QuadType type)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.10.1 ") + __FUNCTION__, "Update Chunks");

	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Chunks", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastUpdateChunksDuration = clock1.duration().count();
		this->m_numUpdateChunks++;
		this->m_sumUpdateChunksDuration += this->m_lastUpdateChunksDuration;
		});

	for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	{
		if (!itQuadTree->second->containsQuadOfType(type))
			continue;

		if (!itQuadTree->second->isValid())
			continue;

		if (!itQuadTree->second->isVisible())
			continue;

		if (itQuadTree->second->getQuadrant()->internalDataLocked())
			continue;

		// All chunk that need an update (those are chunk that got split or joined)
		std::vector<Chunk*> vectChunkUpdate = itQuadTree->second->getUpdatedChunks(type);

		// Forcing update to all neighbouring chunks to readjust the seams (by populating vectAdditionalUpdateChunk)
		// chunks affected are neighbouring ones with same lod or less (more definded) ==> neighbouring chunks with greater lod (less defined) are not selected
		// as the seams are calculated on chunks with lesser lod (more defined chunks) if the neighbourings have different lod
		std::vector<Chunk*> vectAdditionalUpdateChunk;
		for (std::vector<Chunk*>::iterator itChunk = vectChunkUpdate.begin(); itChunk != vectChunkUpdate.end(); itChunk++)
		{
			if (!(*itChunk)->isActive())
				continue;

			Chunk::ChunkPos pos = (*itChunk)->getPos();
			int lod = pos.getLod();
			int numChunksPerSideAtLowerLod = 0;
			Chunk::ChunkPos posFirstInternalChunkAtLowerLod(0, 0, 0);
			if (lod > 0)
			{
				posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
				numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);
			}

			{
				// In case the chunk got split/joined forcing update to left, right, bottom, top chunks with same lod
				Chunk* chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk, type);	// x lessening
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					// - o
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk, type);			// x growing
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					//   o -
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk, type);			// z lessening
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//   -
					//   o
					//
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				chunk = itQuadTree->second->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk, type);			// z growing
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//
					//   o
					//   -
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
			}

			// In case the chunk got joined: we have to look for adjacent chunks with lower lod (more defined)
			if (lod > 0 && (*itChunk)->gotJustJoined())
			{
				//(*itChunk)->setJustJoined(false);

				//	o =
				//	= =
				//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

				//	o =
				//	= =
				Chunk* chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusQuadSecondChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//  
					//	- o =
					//	  = =
					//  
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusQuadForthChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//  
					//	  o =
					//	- = =
					//  
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusQuadFirstChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//  
					//	  o = -
					//	  = =
					//  
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusQuadThirdChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//  
					//	  o = 
					//	  = = -
					//  
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusQuadThirdChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//      
					//    -
					//	  o =
					//	  = =
					//    
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusQuadForthChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//      
					//      -
					//	  o =
					//	  = =
					//    
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusQuadFirstChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//    
					//	  o =
					//	  = =
					//    -
					//      
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
				//	o =
				//	= =
				chunk = itQuadTree->second->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusQuadSecondChunk, type);
				if (chunk != nullptr && chunk->isActive() && !chunk->isPendingUpdate())
				{
					//      
					//	  o =
					//	  = =
					//      -
					//      
					chunk->setPendingUpdate(true);
					vectAdditionalUpdateChunk.push_back(chunk);
				}
			}

			// if the current chunk is on a border of the quadrant we need to look for adjacent chunks (with same lod or lesser if the chunk was just joined) in Adjacent quadrant
			TheWorld_Utils::QuadrantPos XMinusQuadrantPos, XPlusQuadrantPos, ZMinusQuadrantPos, ZPlusQuadrantPos;
			if (itQuadTree->second->isChunkOnBorderOfQuadrant(pos, XMinusQuadrantPos, XPlusQuadrantPos, ZMinusQuadrantPos, ZPlusQuadrantPos))
			{
				int numChunksPerSide = Globals()->numChunksPerHeightmapSide(pos.getLod());

				// if the chunks is adjacent to another quadrant
				if (XMinusQuadrantPos.isInitialized())
				{
					// take the adjacent quadrant if it exists
					auto iter = m_worldQuadrantsMap.find(XMinusQuadrantPos);
					if (iter != m_worldQuadrantsMap.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
					{
						QuadTree* quadTreeXMinus = iter->second.get();
						if (quadTreeXMinus->isValid())
						{
							Chunk::ChunkPos posSameLodOnAdjacentQuadrant(numChunksPerSide - 1, pos.getSlotPosZ(), pos.getLod());
							Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posSameLodOnAdjacentQuadrant, type);
							if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
							{
								//
								// - | o
								//
								chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
								vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
							}

							if (lod > 0 && ((*itChunk)->gotJustJoined() || lod == Globals()->lodMaxDepth()))
							{
								//	o =
								//	= =
								//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

								//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getSlotPosZ(), posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	- | o =
										//	  | = =
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() + 1, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXMinus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	  | o =
										//	- | = =
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
							}
						}
					}
				}

				if (XPlusQuadrantPos.isInitialized())
				{
					// take the adjacent quadrant if it exists
					auto iter = m_worldQuadrantsMap.find(XPlusQuadrantPos);
					if (iter != m_worldQuadrantsMap.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
					{
						QuadTree* quadTreeXPlus = iter->second.get();
						if (quadTreeXPlus->isValid())
						{
							Chunk::ChunkPos posSameLodOnAdjacentQuadrant(0, pos.getSlotPosZ(), pos.getLod());
							Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posSameLodOnAdjacentQuadrant, type);
							if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
							{
								//
								//   o | -
								//
								chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
								vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
							}

							if (lod > 0 && (*itChunk)->gotJustJoined())
							{
								//	o =
								//	= =
								//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

								//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunkAtLowerLod.getSlotPosZ(), posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	o = | -
										//	= = |
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(0, posFirstInternalChunkAtLowerLod.getSlotPosZ() + 1, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeXPlus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	o = |
										//	= = | -
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
							}
						}
					}
				}

				if (ZMinusQuadrantPos.isInitialized())
				{
					// take the adjacent quadrant if it exists
					auto iter = m_worldQuadrantsMap.find(ZMinusQuadrantPos);
					if (iter != m_worldQuadrantsMap.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
					{
						QuadTree* quadTreeZMinus = iter->second.get();
						if (quadTreeZMinus->isValid())
						{
							Chunk::ChunkPos posSameLodOnAdjacentQuadrant(pos.getSlotPosX(), numChunksPerSide - 1, pos.getLod());
							Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posSameLodOnAdjacentQuadrant, type);
							if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
							{
								//   -
								//   =
								//   o
								//
								chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
								vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
							}

							if (lod > 0 && (*itChunk)->gotJustJoined())
							{
								//	o =
								//	= =
								//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

								//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX(), numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//  -
										//  = =
										//	o =
										//	= =
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX() + 1, numChunksPerSideAtLowerLod - 1, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZMinus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//    -
										//  = =
										//	o =
										//	= =
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
							}
						}
					}
				}

				if (ZPlusQuadrantPos.isInitialized())
				{
					// take the adjacent quadrant if it exists
					auto iter = m_worldQuadrantsMap.find(ZPlusQuadrantPos);
					if (iter != m_worldQuadrantsMap.end() && !iter->second->statusToErase() && !iter->second->getQuadrant()->internalDataLocked())
					{
						QuadTree* quadTreeZPlus = iter->second.get();
						if (quadTreeZPlus->isValid())
						{
							//
							//   o
							//   =
							//   -
							Chunk::ChunkPos posSameLodOnAdjacentQuadrant(pos.getSlotPosX(), 0, pos.getLod());
							Chunk* chunkSameLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posSameLodOnAdjacentQuadrant, type);
							if (chunkSameLodOnAdjacentQuadrant != nullptr && chunkSameLodOnAdjacentQuadrant->isActive() && !chunkSameLodOnAdjacentQuadrant->isPendingUpdate())
							{
								chunkSameLodOnAdjacentQuadrant->setPendingUpdate(true);
								vectAdditionalUpdateChunk.push_back(chunkSameLodOnAdjacentQuadrant);
							}

							if (lod > 0 && (*itChunk)->gotJustJoined())
							{
								//	o =
								//	= =
								//Chunk::ChunkPos posFirstInternalChunk(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);

								//int numChunksPerSideAtLowerLod = Globals()->numChunksPerHeightmapSide(pos.getLod() - 1);

								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX(), 0, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	o =
										//	= =
										//  = =
										//  -
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
								{
									Chunk::ChunkPos posLesserLodOnAdjacentQuadrant(posFirstInternalChunkAtLowerLod.getSlotPosX() + 1, 0, posFirstInternalChunkAtLowerLod.getLod());
									Chunk* chunkLesserLodOnAdjacentQuadrant = quadTreeZPlus->getChunkAt(posLesserLodOnAdjacentQuadrant, type);
									if (chunkLesserLodOnAdjacentQuadrant != nullptr && chunkLesserLodOnAdjacentQuadrant->isActive() && !chunkLesserLodOnAdjacentQuadrant->isPendingUpdate())
									{
										//  
										//	o =
										//	= =
										//  = =
										//    -
										//  
										chunkLesserLodOnAdjacentQuadrant->setPendingUpdate(true);
										vectAdditionalUpdateChunk.push_back(chunkLesserLodOnAdjacentQuadrant);
									}
								}
							}
						}
					}
				}
			}

			// Anycase clear flag just joined
			if ((*itChunk)->gotJustJoined())
				(*itChunk)->setJustJoined(false);
		}

		for (std::vector<Chunk*>::iterator it = vectChunkUpdate.begin(); it != vectChunkUpdate.end(); it++)
			(*it)->update(m_visibleInTree);

		for (std::vector<Chunk*>::iterator it = vectAdditionalUpdateChunk.begin(); it != vectAdditionalUpdateChunk.end(); it++)
			(*it)->update(m_visibleInTree);

		itQuadTree->second->clearChunkUpdate(type);
		vectAdditionalUpdateChunk.clear();

		if (itQuadTree->second->statusInitialized() && itQuadTree->second->updateForAllChunksForced(type))
		{
			itQuadTree->second->forceUpdateForAllChunks(type);
			itQuadTree->second->updateForAllChunksForced(type, false);
		}
	}
}

void GDN_TheWorld_Viewer::process_lookDev(void)
{
	if (m_desideredLookDevChanged)
	{
		ShaderTerrainData::freeLookDevSubViewport();

		enum class ShaderTerrainData::LookDev desideredLookDev = getDesideredLookDev();

		for (auto& it : m_worldQuadrantsMap)
		{
			if (desideredLookDev == ShaderTerrainData::LookDev::NotSet)
				it.second->setRegularMaterial(QuadType::NotSet);
			else
				it.second->setLookDevMaterial(desideredLookDev, QuadType::NotSet);
		}
	}

	m_desideredLookDevChanged = false;
}

void GDN_TheWorld_Viewer::process_mapQuadrants(godot::Vector3 cameraWorldPos)
{
	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
	{
		m_mapQuadrantsMap.lock();

		// Render updated quadrants
		while (m_mapQuadrantsMap.updated())
		{
			TheWorld_Utils::QuadrantPos pos = m_mapQuadrantsMap.fetchUpdatedQuadrant();
			if (not pos.empty() and m_mapQuadrantsMap.contains(pos))
			{
				MapQuadrant* mapQuadrant = m_mapQuadrantsMap.at(pos).get();
				if (mapQuadrant->updated())
				{
					map->setMapQuadrantData(mapQuadrant->getMapImage(), pos, true);
					mapQuadrant->updated(false);
				}
			}
		}

		// Check quadrant update request timed out
		constexpr int64_t  secondsBetweenCheck = 1;
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastUpdatingQuadrantsCheck > secondsBetweenCheck * 1000)
		{
			bool statusChanged = m_mapQuadrantsMap.checkUpdatingQuadrants();
			if (statusChanged)
				forceProcessNewMapQuadrants(true);

			m_timeElapsedFromLastUpdatingQuadrantsCheck = timeElapsed;
		}

		// Request update for new quadrants (when map camera moves / scales)
		if (forceProcessNewMapQuadrants() 
			or m_lastProcessedCameraForMapQuadrants == godot::Vector3()
			or (m_lastProcessedCameraForMapQuadrants.x != cameraWorldPos.x or m_lastProcessedCameraForMapQuadrants.z != cameraWorldPos.z))
		{
			GDN_TheWorld_Map* map = Map();
			GDN_TheWorld_MapCamera* mapCamera = MapCamera();
			if (map != nullptr and mapCamera != nullptr)
			{
				// W A R N I N G: all calcs are made in global coords not in pixels because camera zoom alter distances
				godot::Rect2 screenRectInGlobalCoord = map->getScreenRectInGlobalCoord();
				godot::Vector2i textureRectSize = map->getTextureRectSize();
				GDN_TheWorld_Map::MapTextureRectPos cameraTextureRectGlobalPos(mapCamera->get_global_position(), textureRectSize);
				int32_t numTextureRectsUpToScreenBorder = (int32_t)ceil((cameraTextureRectGlobalPos.getGlobalLowerPosY() - screenRectInGlobalCoord.position.y) / textureRectSize.y);
				int32_t numTextureRectsDownToScreenBorder = (int32_t)ceil((screenRectInGlobalCoord.get_end().y - cameraTextureRectGlobalPos.getGlobalHigherPosY()) / textureRectSize.y);
				int32_t numTextureRectsLeftToScreenBorder = (int32_t)ceil((cameraTextureRectGlobalPos.getGlobalLowerPosX() - screenRectInGlobalCoord.position.x) / textureRectSize.x);
				int32_t numTextureRectsRightToScreenBorder = (int32_t)ceil((screenRectInGlobalCoord.get_end().x - cameraTextureRectGlobalPos.getGlobalHigherPosX()) / textureRectSize.x);

				int lowerIndexY = cameraTextureRectGlobalPos.getIdxY() - numTextureRectsUpToScreenBorder;
				int higherIndexY = cameraTextureRectGlobalPos.getIdxY() + numTextureRectsDownToScreenBorder;
				int lowerIndexX = cameraTextureRectGlobalPos.getIdxX() - numTextureRectsLeftToScreenBorder;
				int higherIndexX = cameraTextureRectGlobalPos.getIdxX() + numTextureRectsRightToScreenBorder;

				int numVerticesPerQuadrantSize = (int)Globals()->numVerticesPerQuadrantSize();
				float gridStepInWU = Globals()->gridStepInWU();

				// from upper left texture rect moving horizontally and vertically to the edge of the screen
				for (int y = lowerIndexY; y <= higherIndexY; y++)
				{
					for (int x = lowerIndexX; x <= higherIndexX; x++)
					{
						GDN_TheWorld_Map::MapTextureRectPos textureRectGlobalPos(godot::Vector2i(x, y), textureRectSize);

						for (int imageIdxY = 0; imageIdxY < map->getNumImagesForTextureRectSide(); imageIdxY++)
						{
							for (int imageIdxX = 0; imageIdxX < map->getNumImagesForTextureRectSide(); imageIdxX++)
							{
								godot::Vector2 globalPos(textureRectGlobalPos.getGlobalLowerPosX() + imageIdxX * map->getImageSize().x, textureRectGlobalPos.getGlobalLowerPosY() + imageIdxY * map->getImageSize().y);
								godot::Vector2 mapPos = map->globalPosToMapPos(globalPos);
								TheWorld_Utils::QuadrantPos pos(mapPos.x, mapPos.y, m_worldViewerLevel, numVerticesPerQuadrantSize, gridStepInWU);
								//if (not m_mapQuadrantsMap.contains(pos))
								//{
									bool added = m_mapQuadrantsMap.addUpdatingQuadrant(pos);
								//}
							}
						}
					}
				}
			}

			m_lastProcessedCameraForMapQuadrants = cameraWorldPos;
			forceProcessNewMapQuadrants(false);
		}

		m_mapQuadrantsMap.unlock();

	}
}

void GDN_TheWorld_Viewer::process_materialParam(godot::Vector3& cameraPosInMap, bool processCameraPosForQuadrants, bool processCameraPosForChunks)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Material Params", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastUpdateMaterialParamsDuration = clock1.duration().count();
		this->m_numUpdateMaterialParams++;
		this->m_sumUpdateMaterialParamsDuration += this->m_lastUpdateMaterialParamsDuration;
		});

	GDN_TheWorld_Globals* globals = Globals();

	if (getWorldCamera() == nullptr)
		return;
	
	if (ReloadSelectedQuadrantShaderRequired())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 1 ") + __FUNCTION__, "ReloadSelectedQuadrantShaderRequired");

		QuadTree* quadTreeSel = nullptr;
		TheWorld_Utils::QuadrantPos quadrantSelPos = getQuadrantSelForEdit(&quadTreeSel);

		if (quadTreeSel != nullptr)
		{
			quadTreeSel->reloadCurrentMaterial(QuadType::NotSet);
		}
		
		ReloadSelectedQuadrantShaderRequired(false);

		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::ReloadSelectedQuadrantShader))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	if (ReloadAllShadersRequired())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 2 ") + __FUNCTION__, "ReloadAllShadersRequired");

		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			itQuadTree->second->reloadCurrentMaterial(QuadType::NotSet);
		}

		ReloadAllShadersRequired(false);

		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::ReloadAllShaders))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}
	
	if (m_shaderParamChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 3 ") + __FUNCTION__, "shaderParamChanged");

		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			itQuadTree->second->getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(true);
		}

		m_shaderParamChanged = false;

		if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::ShaderParamChanged))
		{
			m_lastProcessExecutionInterrupted = true;
			return;
		}
	}

	//if ((int)globals->status() >= (int)TheWorldStatus::worldDeployed)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4 ") + __FUNCTION__, "Loop quadrant");

		std::string lastProcessStepExecuted;
		size_t elapsed = 0;
		
		bool loopCompleted = true;
		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			if (processCameraPosForChunks || itQuadTree->second->forceRecalcOptimizationLevel() || !finishedResizingGrid())	// occhio!!!
				itQuadTree->second->setQuadrantOptimizations(cameraPosInMap);
			
			bool materialParamsReset = false;
			bool materialParamsUpdated = false;
			bool colliderUpdated = false;

			bool locked = itQuadTree->second->getQuadrant()->lockInternalData();
			if (locked)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1 ") + __FUNCTION__, "Quadrant locked");

				process_neighbourQuadrantData(itQuadTree->second.get());

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.1 ") + __FUNCTION__, "resetMaterialParams");
					materialParamsReset = itQuadTree->second->resetMaterialParams(m_lastProcessExecutionInterrupted);
					if (m_lastProcessExecutionInterrupted)
					{
						itQuadTree->second->getQuadrant()->unlockInternalData();
						loopCompleted = false;
						break;
					}
					//if (materialParamsReset)			// DEBUG
					//	materialParamsReset = true;		// DEBUG
				}

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.2 ") + __FUNCTION__, "updateMaterialParams");
					materialParamsUpdated = itQuadTree->second->updateMaterialParams();
					if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_updateMaterialParams))
					{
						m_lastProcessExecutionInterrupted = true;
						itQuadTree->second->getQuadrant()->unlockInternalData();
						loopCompleted = false;
						break;
					}
					//if (materialParamsUpdated)		// DEBUG
					//	materialParamsUpdated = true;	// DEBUG
				}

				bool colliderCanHaveHiResCollisionInfo = itQuadTree->second->getQuadrant()->canHaveHiResCollisionInfo();
				if (!itQuadTree->second->getQuadrant()->getCollider()->hasData() || 
					(colliderCanHaveHiResCollisionInfo && !itQuadTree->second->getQuadrant()->getCollider()->hasHiResData()) || (!colliderCanHaveHiResCollisionInfo && itQuadTree->second->getQuadrant()->getCollider()->hasHiResData()))
					processCameraPosForQuadrants = true;
				
				if (itQuadTree->second->colliderNeedReset(QuadType::NotSet) || processCameraPosForQuadrants)
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.3 ") + __FUNCTION__, "updateCollider");
					colliderUpdated = itQuadTree->second->updateCollider(QuadType::NotSet);
					if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_updateCollider))
					{
						m_lastProcessExecutionInterrupted = true;
						itQuadTree->second->getQuadrant()->unlockInternalData();
						loopCompleted = false;
						break;
					}
					//if (colliderUpdated)		// DEBUG
					//	colliderUpdated = true;	// DEBUG
				}

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.4 ") + __FUNCTION__, "releaseTerrainValuesMemory");
					itQuadTree->second->getQuadrant()->releaseTerrainValuesMemory(itQuadTree->second->isVisible());
				}

				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.5 ") + __FUNCTION__, "unlockInternalData");
					itQuadTree->second->getQuadrant()->unlockInternalData();
				}
			}
			else 
				loopCompleted = false;

			if (materialParamsUpdated)
				m_previousTrackedMouseHitInMap = godot::Vector3();

			if ((materialParamsReset || materialParamsUpdated || colliderUpdated) && !m_desideredLookDevChanged)
			{
				loopCompleted = false;
				break;
			}
		}

		if (loopCompleted)
			allCollidersReady(true);
	}
}

void GDN_TheWorld_Viewer::process_neighbourQuadrantData(QuadTree* quadTree)
{
	//return;
	if (quadTree->statusInitialized())
	{
		Quadrant* quadrant = quadTree->getQuadrant();
		TheWorld_Utils::QuadrantPos quadrantPos = quadrant->getPos();

		Quadrant::NeighbourQuadrantData& northQuadrantData = quadrant->northQuadrantData();
		if (!northQuadrantData.initialized())
		{

			TheWorld_Utils::QuadrantPos northPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
			if (m_worldQuadrantsMap.contains(northPos))
			{
				QuadTree* northQuadTree = m_worldQuadrantsMap[northPos].get();
				if (!northQuadTree->statusToErase() && northQuadTree->statusInitialized())
				{
					northQuadrantData.init(northQuadTree->getQuadrant()->quadrantTextureLayers());
					northQuadTree->getQuadrant()->southQuadrantData().init(quadrant->quadrantTextureLayers());
				}
			}
		}

		Quadrant::NeighbourQuadrantData& southQuadrantData = quadrant->southQuadrantData();
		if (!southQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos southPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
			if (m_worldQuadrantsMap.contains(southPos))
			{
				QuadTree* southQuadTree = m_worldQuadrantsMap[southPos].get();
				if (!southQuadTree->statusToErase() && southQuadTree->statusInitialized())
				{
					southQuadrantData.init(southQuadTree->getQuadrant()->quadrantTextureLayers());
					southQuadTree->getQuadrant()->northQuadrantData().init(quadrant->quadrantTextureLayers());
				}
			}
		}

		Quadrant::NeighbourQuadrantData& westQuadrantData = quadrant->westQuadrantData();
		if (!westQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos westPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
			if (m_worldQuadrantsMap.contains(westPos))
			{
				QuadTree* westQuadTree = m_worldQuadrantsMap[westPos].get();
				if (!westQuadTree->statusToErase() && westQuadTree->statusInitialized())
				{
					westQuadrantData.init(westQuadTree->getQuadrant()->quadrantTextureLayers());
					westQuadTree->getQuadrant()->eastQuadrantData().init(quadrant->quadrantTextureLayers());
				}
			}
		}
		
		Quadrant::NeighbourQuadrantData& eastQuadrantData = quadrant->eastQuadrantData();
		if (!eastQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos eastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
			if (m_worldQuadrantsMap.contains(eastPos))
			{
				QuadTree* eastQuadTree = m_worldQuadrantsMap[eastPos].get();
				if (!eastQuadTree->statusToErase() && eastQuadTree->statusInitialized())
				{
					eastQuadrantData.init(eastQuadTree->getQuadrant()->quadrantTextureLayers());
					eastQuadTree->getQuadrant()->westQuadrantData().init(quadrant->quadrantTextureLayers());
				}
			}
		}

		if (is_inside_tree() && quadTree->getLookDev() == ShaderTerrainData::LookDev::NotSet)
		{
			if (northQuadrantData.initialized() && !northQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_NORTH_GROUND_LAYERS, northQuadrantData.groundTextureLayers());
				northQuadrantData.shaderUpdated(true);
			}

			if (southQuadrantData.initialized() && !southQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_SOUTH_GROUND_LAYERS, southQuadrantData.groundTextureLayers());
				southQuadrantData.shaderUpdated(true);
			}

			if (westQuadrantData.initialized() && !westQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_WEST_GROUND_LAYERS, westQuadrantData.groundTextureLayers());
				westQuadrantData.shaderUpdated(true);
			}

			if (eastQuadrantData.initialized() && !eastQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_EAST_GROUND_LAYERS, eastQuadrantData.groundTextureLayers());
				eastQuadrantData.shaderUpdated(true);
			}
		}
	}
}

void GDN_TheWorld_Viewer::process_dump(GDN_TheWorld_Globals* globals)
{
//#pragma warning(push, 1)
//#pragma warning( disable : 6237 )
	// Check for Dump
	if (TIME_INTERVAL_BETWEEN_DUMP != 0 && globals->isDebugEnabled())
	{
//#pragma warning(pop)
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastDump > TIME_INTERVAL_BETWEEN_DUMP * 1000)
		{
			m_dumpRequired = true;
			m_timeElapsedFromLastDump = timeElapsed;
		}
	}
	//if (m_dumpRequired)
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.12.1 ") + __FUNCTION__, "Dump stuff");

	//	m_dumpRequired = false;
	//	dump();
	//}
}

void GDN_TheWorld_Viewer::process_statistics(GDN_TheWorld_Globals* globals)
{
	// Check for Statistics
	if (TIME_INTERVAL_BETWEEN_STATISTICS != 0)
	{
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastStatistic > TIME_INTERVAL_BETWEEN_STATISTICS)
		{
			//m_numProcessNotOwnsLock = 0;

			if (m_notStreamingTime.counterStarted() && m_notStreamingTime.partialDuration().count() > 60000)
			{
				writeStatisticsNewQuadrants();
				m_notStreamingTime.tock();
				m_notStreamingTime.tick();
			}

			refreshQuadrantStatistics();

			if (m_numProcessExecution > 0)
			{
				m_averageProcessDuration = int(m_sumProcessDuration / m_numProcessExecution);
				//globals->debugPrint(String("m_numProcessExecution=") + std::to_string(m_numProcessExecution).c_str() + ", m_processDuration=" + std::to_string(m_processDuration).c_str() + ", m_averageProcessDuration=" + std::to_string(m_averageProcessDuration).c_str());
				m_sumProcessDuration = 0;
				m_numProcessExecution = 0;
			}
			else
				m_averageProcessDuration = 0;

			if (m_numProcessImplExecution > 0)
			{
				m_averageProcessImplDuration = int(m_sumProcessImplDuration / m_numProcessImplExecution);
				//globals->debugPrint(String("m_numProcessImplExecution=") + std::to_string(m_numProcessImplExecution).c_str() + ", m_processImplDuration=" + std::to_string(m_processImplDuration).c_str() + ", m_averageProcessImplDuration=" + std::to_string(m_averageProcessImplDuration).c_str());
				m_sumProcessImplDuration = 0;
				m_numProcessImplExecution = 0;
			}
			else
				m_averageProcessImplDuration = 0;

			//if (m_numRefreshMapQuad > 10)
			//{
			//	m_averageRefreshMapQuadDuration = int(m_sumRefreshMapQuadDuration / m_numRefreshMapQuad);
			//	m_sumRefreshMapQuadDuration = 0;
			//	m_numRefreshMapQuad = 0;
			//}

			if (m_numMouseTrackHit > 10)
			{
				m_averageMouseTrackHitDuration = int(m_sumMouseTrackHitDuration / m_numMouseTrackHit);
				m_sumMouseTrackHitDuration = 0;
				m_numMouseTrackHit = 0;
			}

			if (m_numUpdateQuads1 > 0)
			{
				m_averageUpdateQuads1Duration = int(m_sumUpdateQuads1Duration / m_numUpdateQuads1);
				m_sumUpdateQuads1Duration = 0;
				m_numUpdateQuads1 = 0;
			}
			else
				m_averageUpdateQuads1Duration = 0;

			if (m_numUpdateQuads2 > 0)
			{
				m_averageUpdateQuads2Duration = int(m_sumUpdateQuads2Duration / m_numUpdateQuads2);
				m_sumUpdateQuads2Duration = 0;
				m_numUpdateQuads2 = 0;
			}
			else
				m_averageUpdateQuads2Duration = 0;

			if (m_numUpdateQuads3 > 0)
			{
				m_averageUpdateQuads3Duration = int(m_sumUpdateQuads3Duration / m_numUpdateQuads3);
				m_sumUpdateQuads3Duration = 0;
				m_numUpdateQuads3 = 0;
			}
			else
				m_averageUpdateQuads3Duration = 0;

			if (m_numUpdateChunks > 0)
			{
				m_averageUpdateChunksDuration = int(m_sumUpdateChunksDuration / m_numUpdateChunks);
				m_sumUpdateChunksDuration = 0;
				m_numUpdateChunks = 0;
			}
			else
				m_averageUpdateChunksDuration = 0;

			if (m_numUpdateMaterialParams > 0)
			{
				m_averageUpdateMaterialParamsDuration = int(m_sumUpdateMaterialParamsDuration / m_numUpdateMaterialParams);
				m_sumUpdateMaterialParamsDuration = 0;
				m_numUpdateMaterialParams = 0;
			}
			else
				m_averageUpdateMaterialParamsDuration = 0;

			if (m_numEraseQuads > 0)
			{
				m_averageEraseQuads = int(m_sumEraseQuads / m_numEraseQuads);
				m_sumEraseQuads = 0;
				m_numEraseQuads = 0;
			}
			else
				m_averageEraseQuads = 0;

			if (m_numProcessMapModder > 0)
			{
				m_averageProcessMapModder = int(m_sumProcessMapModder / m_numProcessMapModder);
				m_sumProcessMapModder = 0;
				m_numProcessMapModder = 0;
			}
			else
				m_averageProcessMapModder = 0;

			if (m_numUpdateCameraQuad > 0)
			{
				m_averageUpdateCameraQuad = int(m_sumUpdateCameraQuad / m_numUpdateCameraQuad);
				m_sumUpdateCameraQuad = 0;
				m_numUpdateCameraQuad = 0;
			}
			else
				m_averageUpdateCameraQuad = 0;

			if (m_numStreamingQuadrantStuff > 0)
			{
				m_averageStreamingQuadrantStuff = int(m_sumStreamingQuadrantStuff / m_numStreamingQuadrantStuff);
				m_sumStreamingQuadrantStuff = 0;
				m_numStreamingQuadrantStuff = 0;
			}
			else
				m_averageStreamingQuadrantStuff = 0;
			
			m_timeElapsedFromLastStatistic = timeElapsed;
		}
	}
}

void GDN_TheWorld_Viewer::_physics_process(double _delta)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("_physics_process 1 ") + __FUNCTION__, "ALL");

	//if (IS_EDITOR_HINT())
	//	return;

	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

	if ((int)globals->status() < (int)TheWorldStatus::sessionInitialized)
		return;

	Input* input = Input::get_singleton();

	if (IS_EDITOR_HINT())
	{

	}
	else
	{
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
	}
}

void GDN_TheWorld_Viewer::clearQuadTreeMap(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	m_worldQuadrantsMapValid = false;
	m_worldQuadrantsMap.clear();
	m_extraQuadsMap.clear();
	//m_cameraQuadTree = nullptr;
	m_quadrantHit = nullptr;
	m_adjacentQuadrantsHit.clear();
	m_previousTrackedMouseHitInMap = godot::Vector3();
	m_quadrantSelPos.reset();
}

size_t GDN_TheWorld_Viewer::getExtraQuadsMapSize(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
	return m_extraQuadsMap.size();
}

void GDN_TheWorld_Viewer::getAllQuadrantPos(std::vector<TheWorld_Utils::QuadrantPos>& allQuandrantPos, bool withExtraQuads)
{
	allQuandrantPos.clear();

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	size_t size = m_worldQuadrantsMap.size();
	if (withExtraQuads)
	{
		size += m_extraQuadsMap.size();
	}
	allQuandrantPos.reserve(size);
	for (auto const& item : m_worldQuadrantsMap)
		allQuandrantPos.push_back(item.first);
	for (auto const& item : m_extraQuadsMap)
		allQuandrantPos.push_back(item.first);
}

QuadTree* GDN_TheWorld_Viewer::getQuadTree(TheWorld_Utils::QuadrantPos pos, bool excludeToErase, bool lockQuadrant, bool extraQuadAllowed)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	auto item = m_worldQuadrantsMap.find(pos);
	if (item == m_worldQuadrantsMap.end())
	{
		if (extraQuadAllowed)
		{
			item = m_extraQuadsMap.find(pos);
			if (item == m_extraQuadsMap.end())
				return nullptr;
		}
		else
			return nullptr;
	}

	QuadTree* quadTree = item->second.get();
	if (quadTree->statusToErase() && excludeToErase)
		return nullptr;

	if (lockQuadrant)
	{
		bool locked = quadTree->getQuadrant()->lockInternalData();
		if (!locked)
			return nullptr;
	}

	return quadTree;
}

Chunk* GDN_TheWorld_Viewer::getActiveChunkAt(TheWorld_Utils::QuadrantPos pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter, enum class QuadType type)
{
	QuadTree* quadTree = getQuadTree(pos);
	if (quadTree == nullptr)
		return nullptr;

	Chunk* chunk = quadTree->getChunkAt(chunkPos, type);
	if (chunk == nullptr)
		return nullptr;

	return getActiveChunkAt(chunk, dir, filter, type);
}

Chunk* GDN_TheWorld_Viewer::getActiveChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter, enum class QuadType type)
{
	// A T T E N Z I O N E : DA TESTARE
	
	if (dir == Chunk::DirectionSlot::Center)
		return chunk;

	enum PosInQuad posInQuad = chunk->getPosInQuad();
	if (posInQuad == PosInQuad::NotSet)
		return nullptr;

	Chunk::ChunkPos pos = chunk->getPos();
	QuadTree* quadTree = chunk->getQuadTree();
		
	Chunk* retChunk = nullptr;

	//int idx = 0;
	switch (dir)
	{
	case Chunk::DirectionSlot::XMinusChunk:
	{
		if (posInQuad == PosInQuad::Second || posInQuad == PosInQuad::Forth)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk, type);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosX() == 0)
			{
				QuadTree* quadTreeXMinus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus));
				if (quadTreeXMinus != nullptr && quadTreeXMinus->isValid() && quadTreeXMinus->isVisible())
				{
					Chunk::ChunkPos posLastChunkAtSameLod(numChunksPerHeightmapSide - 1, pos.getSlotPosZ(), lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeXMinus->getChunkAt(posLastChunkAtSameLod, type);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() * 2 + 1, posLastChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() / 2, posLastChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posSecondInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posSecondInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeXMinus->getChunkAt(posSecondInternalChunkAtLowerLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeXMinus->getChunkAt(posChunkAtHigherLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posSecondInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posSecondInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posSecondInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XMinusChunk, type);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::XMinusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::XMinusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2, posFirstInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::XPlusChunk:
	{
		if (posInQuad == PosInQuad::First || posInQuad == PosInQuad::Third)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk, type);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosX() == numChunksPerHeightmapSide - 1)
			{
				QuadTree* quadTreeXPlus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus));
				if (quadTreeXPlus != nullptr && quadTreeXPlus->isValid() && quadTreeXPlus->isVisible())
				{
					Chunk::ChunkPos posFirstChunkAtSameLod(0, pos.getSlotPosZ(), lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeXPlus->getChunkAt(posFirstChunkAtSameLod, type);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() * 2, posFirstChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() / 2, posFirstChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeXPlus->getChunkAt(posFirstInternalChunkAtLowerLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeXPlus->getChunkAt(posChunkAtHigherLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posFirstInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::XPlusChunk, type);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2 + 1, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posSecondInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posSecondInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posSecondInternalChunkAtLowerLod, Chunk::DirectionSlot::XPlusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::XPlusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posSecondInternalChunkAtLowerLod = Chunk::ChunkPos(posSecondInternalChunkAtLowerLod.getSlotPosX() * 2, posSecondInternalChunkAtLowerLod.getSlotPosZ() * 2, posSecondInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::ZMinusChunk:
	{
		if (posInQuad == PosInQuad::Third || posInQuad == PosInQuad::Forth)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk, type);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosZ() == 0)
			{
				QuadTree* quadTreeZMinus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus));
				if (quadTreeZMinus != nullptr && quadTreeZMinus->isValid() && quadTreeZMinus->isVisible())
				{
					Chunk::ChunkPos posLastChunkAtSameLod(pos.getSlotPosX(), numChunksPerHeightmapSide - 1, lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeZMinus->getChunkAt(posLastChunkAtSameLod, type);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() * 2, posLastChunkAtSameLod.getSlotPosZ() * 2 + 1, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posLastChunkAtSameLod.getSlotPosX() / 2, posLastChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posThirdInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posThirdInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeZMinus->getChunkAt(posThirdInternalChunkAtLowerLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeZMinus->getChunkAt(posChunkAtHigherLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posThirdInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posThirdInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posThirdInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZMinusChunk, type);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posFirstInternalChunkAtLowerLod, Chunk::DirectionSlot::ZMinusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::ZMinusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2, posFirstInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	case Chunk::DirectionSlot::ZPlusChunk:
	{
		if (posInQuad == PosInQuad::First || posInQuad == PosInQuad::Second)
		{
			if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk, type);
			if (retChunk != nullptr && !retChunk->isActive())
				retChunk = nullptr;
		}
		else
		{
			int lod = pos.getLod();
			int maxLod = Globals()->lodMaxDepth();
			int numChunksPerHeightmapSide = Globals()->numChunksPerHeightmapSide(lod);

			if (chunk->getSlotPosZ() == numChunksPerHeightmapSide - 1)
			{
				QuadTree* quadTreeZPlus = getQuadTree(quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus));
				if (quadTreeZPlus != nullptr && quadTreeZPlus->isValid() && quadTreeZPlus->isVisible())
				{
					Chunk::ChunkPos posFirstChunkAtSameLod(pos.getSlotPosX(), 0, lod);
					if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
					{
						retChunk = quadTreeZPlus->getChunkAt(posFirstChunkAtSameLod, type);
						if (retChunk != nullptr && !retChunk->isActive())
							retChunk = nullptr;
					}
					if (retChunk == nullptr)
					{
						Chunk::ChunkPos posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() * 2, posFirstChunkAtSameLod.getSlotPosZ() * 2, lod - 1);
						Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(posFirstChunkAtSameLod.getSlotPosX() / 2, posFirstChunkAtSameLod.getSlotPosZ() / 2, lod + 1);
						while (true)
						{
							if (retChunk != nullptr || (posFirstInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
								break;

							if (retChunk == nullptr && posFirstInternalChunkAtLowerLod.getLod() >= 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
							{
								// we have not found a chunk on the X minus quad at the same lod: looking for a chunk at lower lod (most defined)
								retChunk = quadTreeZPlus->getChunkAt(posFirstInternalChunkAtLowerLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}
							if (retChunk == nullptr && posChunkAtHigherLod.getLod() <= maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
							{
								// we have not found a chunk on the X minus quad nor at the same nor at lower lod: looking for a chunk at higher lod (less defined)
								retChunk = quadTreeZPlus->getChunkAt(posChunkAtHigherLod, type);
								if (retChunk != nullptr && !retChunk->isActive())
									retChunk = nullptr;
							}

							posFirstInternalChunkAtLowerLod = Chunk::ChunkPos(posFirstInternalChunkAtLowerLod.getSlotPosX() * 2 + 1, posFirstInternalChunkAtLowerLod.getSlotPosZ() * 2,
								posFirstInternalChunkAtLowerLod.getLod() - 1);
							posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
							//idx++;
							//if (idx >= 2)
							//	Godot::print("msg");
						}
					}
				}
			}
			else
			{
				if (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::SameLod)
				{
					retChunk = quadTree->getChunkAt(pos, Chunk::DirectionSlot::ZPlusChunk, type);
					if (retChunk != nullptr && !retChunk->isActive())
						retChunk = nullptr;
				}
				if (retChunk == nullptr)
				{
					Chunk::ChunkPos posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(pos.getSlotPosX() * 2, pos.getSlotPosZ() * 2 + 1, lod - 1);
					Chunk::ChunkPos posChunkAtHigherLod = Chunk::ChunkPos(pos.getSlotPosX() / 2, pos.getSlotPosZ() / 2, lod + 1);
					while (true)
					{
						if (retChunk != nullptr || (posThirdInternalChunkAtLowerLod.getLod() < 0 && posChunkAtHigherLod.getLod() > maxLod))
							break;

						if (retChunk == nullptr && posThirdInternalChunkAtLowerLod.getLod() > 0 && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::LowerLod))
						{
							// we have not found a chunk on the X minus direction at the same lod: looking for a chunk at lower lod (most defined)
							retChunk = quadTree->getChunkAt(posThirdInternalChunkAtLowerLod, Chunk::DirectionSlot::ZPlusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}
						if (retChunk == nullptr && posChunkAtHigherLod.getLod() < maxLod && (filter == Chunk::LookForChunk::All || filter == Chunk::LookForChunk::HigherLod))
						{
							// we have not found a chunk on the X minus direction nor at same nor at lower lod: looking for a chunk at higher lod (less defined)
							retChunk = quadTree->getChunkAt(posChunkAtHigherLod, Chunk::DirectionSlot::ZPlusChunk, type);
							if (retChunk != nullptr && !retChunk->isActive())
								retChunk = nullptr;
						}

						posThirdInternalChunkAtLowerLod = Chunk::ChunkPos(posThirdInternalChunkAtLowerLod.getSlotPosX() * 2, posThirdInternalChunkAtLowerLod.getSlotPosZ() * 2, posThirdInternalChunkAtLowerLod.getLod() - 1);
						posChunkAtHigherLod = Chunk::ChunkPos(posChunkAtHigherLod.getSlotPosX() / 2, posChunkAtHigherLod.getSlotPosZ() / 2, posChunkAtHigherLod.getLod() + 1);
						//idx++;
						//if (idx >= 2)
						//	Godot::print("msg");
					}
				}
			}
		}
	}
	break;
	}

	return retChunk;
}

void GDN_TheWorld_Viewer::createEditModeUI(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl == nullptr)
	{
		m_mtxQuadTreeAndMainProcessing.lock();
		editModeUIControl = memnew(GDN_TheWorld_Edit);
		m_mtxQuadTreeAndMainProcessing.unlock();
		if (editModeUIControl == nullptr)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Create Control error!").c_str()));
	}

	if (!editModeUIControl->initialized())
	{
		editModeUIControl->init(this);
		//SceneTree* scene = get_tree();
		//Node* sceneRoot = nullptr;
		//if (scene != nullptr)
		//	sceneRoot = scene->get_edited_scene_root();
		//if (sceneRoot != nullptr)
		//	editModeUIControl->set_owner(sceneRoot);
	}
}

bool GDN_TheWorld_Viewer::isQuadrantSelectedForEdit(void)
{
	if (m_quadrantSelPos.empty())
		return false;
	else
		return true;
}

TheWorld_Utils::QuadrantPos GDN_TheWorld_Viewer::getQuadrantSelForEdit(QuadTree** quadTreeSel)
{
	*quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos ret = m_quadrantSelPos;

	if (ret.empty())
		return ret;

	{
		*quadTreeSel = getQuadTree(ret);
		if (*quadTreeSel == nullptr)
			return TheWorld_Utils::QuadrantPos();
		else
			return ret;

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		//WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(ret);
		//if (it == m_worldQuadrantsMap.end() || it->second->statusToErase())
		//	return QuadrantPos();
		//else
		//{
		//	*quadTreeSel = it->second.get();
		//	return ret;
		//}
	}
}

godot::GDN_TheWorld_Edit* GDN_TheWorld_Viewer::EditModeUIControl(bool useCache)
{
	if (m_editModeUIControl == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return NULL;
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_editModeUIControl = godot::Object::cast_to<GDN_TheWorld_Edit>(root->find_child(THEWORLD_EDIT_MODE_UI_CONTROL_NAME, true, false));
	}

	return m_editModeUIControl;
}

GDN_TheWorld_Globals* GDN_TheWorld_Viewer::Globals(bool useCache)
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
		m_globals = godot::Object::cast_to<GDN_TheWorld_Globals>(root->find_child(THEWORLD_GLOBALS_NODE_NAME, true, false));
	}

	return m_globals;
}

//GDN_TheWorld_WorldViewportContainer* GDN_TheWorld_Viewer::WorldViewportContainer(bool useCache)
//{
//	if (m_worldViewportContainer == nullptr || !useCache)
//	{
//		if (not is_inside_tree())
//			return nullptr;
//		SceneTree* scene = get_tree();
//		if (!scene)
//			return nullptr;
//		Viewport* root = scene->get_root();
//		if (!root)
//			return nullptr;
//		m_worldViewportContainer = godot::Object::cast_to<GDN_TheWorld_WorldViewportContainer>(root->find_child(THEWORLD_WORLD_VIEWPORT_CONTAINER_NODE_NAME, true, false));
//	}
//
//	return m_worldViewportContainer;
//}

//GDN_TheWorld_WorldViewport* GDN_TheWorld_Viewer::WorldViewport(bool useCache)
//{
//	if (m_worldViewport == nullptr || !useCache)
//	{
//		if (not is_inside_tree())
//			return nullptr;
//		SceneTree* scene = get_tree();
//		if (!scene)
//			return nullptr;
//		Viewport* root = scene->get_root();
//		if (!root)
//			return nullptr;
//		m_worldViewport = godot::Object::cast_to<GDN_TheWorld_WorldViewport>(root->find_child(THEWORLD_WORLD_VIEWPORT_NODE_NAME, true, false));
//	}
//
//	return m_worldViewport;
//}

GDN_TheWorld_Camera* GDN_TheWorld_Viewer::WorldCamera(bool useCache)
{
	if (m_customWorldCamera == NULL || !useCache)
	{
		if (not is_inside_tree())
			return NULL;
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_customWorldCamera = godot::Object::cast_to<GDN_TheWorld_Camera>(root->find_child(THEWORLD_CAMERA_NODE_NAME, true, false));
	}

	return m_customWorldCamera;
}

GDN_TheWorld_HudControls* GDN_TheWorld_Viewer::HudControls(bool useCache)
{
	if (m_hudControls == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		m_hudControls = godot::Object::cast_to<GDN_TheWorld_HudControls>(root->find_child(THEWORLD_HUD_CONTROLS_NODE_NAME, true, false));
	}

	return m_hudControls;
}

//float GDN_TheWorld_Viewer::getMapScaleFactor(void)
//{
//	float mapScaleFactor = 0;
//
//	GDN_TheWorld_MapViewer* mapViewer = getMapViewer();
//	if (mapViewer != nullptr)
//	{
//		GDN_TheWorld_Map* map = mapViewer->getMap();
//		if (map != nullptr)
//		{
//			mapScaleFactor = map->getScaleFactor();
//		}
//	}
//
//	return mapScaleFactor;
//}


GDN_TheWorld_MapViewer* GDN_TheWorld_Viewer::MapViewer(bool useCache)
{
	if (m_mapViewer == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		m_mapViewer = godot::Object::cast_to<GDN_TheWorld_MapViewer>(root->find_child(THEWORLD_MAP_VIEWER_NODE_NAME, true, false));
	}

	return m_mapViewer;
}

GDN_TheWorld_MapViewportContainer* GDN_TheWorld_Viewer::MapViewportContainer(bool useCache)
{
	if (m_mapViewportContainer == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		m_mapViewportContainer = godot::Object::cast_to<GDN_TheWorld_MapViewportContainer>(root->find_child(THEWORLD_MAP_VIEWPORT_CONTAINER_NODE_NAME, true, false));
	}

	return m_mapViewportContainer;
}

GDN_TheWorld_MapViewport* GDN_TheWorld_Viewer::MapViewport(bool useCache)
{
	if (m_mapViewport == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		m_mapViewport = godot::Object::cast_to<GDN_TheWorld_MapViewport>(root->find_child(THEWORLD_MAP_VIEWPORT_NODE_NAME, true, false));
	}

	return m_mapViewport;
}

GDN_TheWorld_Map* GDN_TheWorld_Viewer::Map(bool useCache)
{
	if (m_map == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		m_map = godot::Object::cast_to<GDN_TheWorld_Map>(root->find_child(THEWORLD_MAP_NODE_NAME, true, false));
	}

	return m_map;
}

GDN_TheWorld_MapCamera* GDN_TheWorld_Viewer::MapCamera(bool useCache)
{
	if (m_mapCamera == nullptr || !useCache)
	{
		if (not is_inside_tree())
			return nullptr;
		SceneTree* scene = get_tree();
		if (!scene)
			return nullptr;
		Viewport* root = scene->get_root();
		if (!root)
			return nullptr;
		godot::Node* n = root->find_child(THEWORLD_MAP_CAMERA_NODE_NAME, true, false);
		m_mapCamera = godot::Object::cast_to<GDN_TheWorld_MapCamera>(root->find_child(THEWORLD_MAP_CAMERA_NODE_NAME, true, false));
	}

	return m_mapCamera;
}

bool GDN_TheWorld_Viewer::terrainShiftPermitted(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

	if (editModeUIControl == nullptr || !editModeUIControl->initialized())
		return true;
	else
	{
		if (editModeUIControl->UIAcceptingFocus())
			return false;
		else
			return true;
		//if (!editMode() && !editModeUIControl->actionInProgress())
		//	return true;
		//else
		//	return false;
	}

	return true;
	return m_numInitializedQuadrant >= m_numQuadrant;
}

godot::Camera3D* GDN_TheWorld_Viewer::getWorldCamera(void)
{
	return getWorldCamera2();
}

godot::Camera3D* GDN_TheWorld_Viewer::getWorldCamera2(bool force)
{
	if (!force and mapMode())
		return nullptr;

	if (IS_EDITOR_HINT())
		return getWorldCameraInEditor();
	else
	{
		//return get_tree()->get_root()->get_camera_3d();
		return get_viewport()->get_camera_3d();
	}
}

godot::Camera2D* GDN_TheWorld_Viewer::getMapCamera(void)
{
	//if (!mapMode())
	//	return nullptr; 
	//
	//if (IS_EDITOR_HINT())
	//	throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("GDN_TheWorld_Viewer::getMapCamera not implemented in EDITOR").c_str()));
	//else
	//{
	//	//return get_tree()->get_root()->get_camera_3d();
	//	return get_viewport()->get_camera_2d();
	//}
	return MapCamera();
}

void GDN_TheWorld_Viewer::setWorldEditorCamera(godot::Camera3D* editorCamera)
{
	if (m_editorCamera == nullptr)
		onTransformChanged();

	m_editorCamera = editorCamera;
}

godot::Camera3D* GDN_TheWorld_Viewer::getWorldCameraInEditor(void)
{
	if (!IS_EDITOR_HINT())
		return nullptr;

	if (m_editorCamera != nullptr)
		return m_editorCamera;
	
	godot::Camera3D* editorCamera = nullptr;

	if (m_editorInterface != nullptr)
	{
		Node* editorSceneRootViewport = m_editorInterface->get_edited_scene_root();

		godot::Array foundNodes;
		//godot::Array children = get_tree()->get_edited_scene_root()->get_children();
		godot::Array children = editorSceneRootViewport->get_children();
		int64_t ___size = children.size();	// DEBUG
		if (!children.is_empty())
			_findChildNodes(foundNodes, children, "GDN_TheWorld_Camera");

		___size = foundNodes.size();	// DEBUG
		if (!foundNodes.is_empty())
			editorCamera = godot::Object::cast_to<godot::Camera3D>(foundNodes[0]);
	}

	return editorCamera;
}

Chunk* GDN_TheWorld_Viewer::cameraTerrainChunk(void)
{
	return m_cameraTerrainChunk;
}

void GDN_TheWorld_Viewer::cameraTerrainChunk(Chunk* c)
{
	m_cameraTerrainChunk = c;
}

Chunk* GDN_TheWorld_Viewer::mouseHitTerrainChunk(void)
{
	return m_mouseHitTerrainChunk;
}

void GDN_TheWorld_Viewer::mouseHitTerrainChunk(Chunk* c)
{
	m_mouseHitTerrainChunk = c;
}

void GDN_TheWorld_Viewer::_findChildNodes(godot::Array& foundNodes, godot::Array& searchNodes, String searchClass)
{
	for (int i = 0; i < searchNodes.size(); i++)
	{
		String className = godot::Object::cast_to<godot::Node>(searchNodes[i])->get_class();
		std::string ___classsName = to_string(className);		// DEBUG
		std::string ___nodeName = to_string(godot::Object::cast_to<godot::Node>(searchNodes[i])->get_name());	// DEBUG
		if (className == searchClass)
			foundNodes.push_back(searchNodes[i]);
		godot::Array children = godot::Object::cast_to<godot::Node>(searchNodes[i])->get_children();
		int64_t ___size = children.size();	// DEBUG
		if (!children.is_empty())
			_findChildNodes(foundNodes, children, searchClass);
	}
}

void GDN_TheWorld_Viewer::deployWorld(float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll, int level, int chunkSizeShift, int heightmapResolutionShift)
{
	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System

	enum class TheWorldStatus status = Globals()->status();
	if (status < TheWorldStatus::sessionInitialized)
		DebugBreak();

	assert(status >= TheWorldStatus::sessionInitialized);
	if (status < TheWorldStatus::sessionInitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Incorrect Status for resetInitialWorldViewerPos").c_str()));

	m_initialWorldViewerPosSet = false;
	
	m_worldViewerLevel = level;

	try
	{
		bool changed = Globals()->resize(chunkSizeShift, heightmapResolutionShift);
		if (changed)
		{
			if (m_meshCache)
				m_meshCache.reset();
			m_meshCache = make_unique<MeshCache>(this);
			m_meshCache->initCache(Globals()->numVerticesPerChuckSide(), Globals()->numLods());
		}

		m_numWorldVerticesPerSize = Globals()->heightmapResolution() + 1;
		
		Globals()->setStatus(TheWorldStatus::worldDeployInProgress);

		float _gridStepInWU = Globals()->gridStepInWU();

		Globals()->Client()->MapManagerDeployWorld(isInEditor(), level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		
		//Globals()->Client()->MapManagerDeployLevel(level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);

		//QuadrantPos quadrantPos(cameraX, cameraZ, level, m_numWorldVerticesPerSize, _gridStepInWU);
		//quadrantPos.setTag("Camera");

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

		//clearQuadTreeMap();
		//m_worldQuadrantsMap[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
		//m_worldQuadrantsMap[quadrantPos]->init(cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll, true, cameraDistanceFromTerrainForced);
	}
	catch (TheWorld_MapManager::MapManagerException& e)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, e.exceptionName() + string(" caught - ") + e.what());
		return;
	}
	catch (std::exception& e)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("std::exception caught - ") + e.what());
		return;
	}
	catch (...)
	{
		Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "std::exception caught");
		return;
	}
}

void GDN_TheWorld_Viewer::undeployWorld(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	m_stopDeploy = true;
	
	enum class TheWorldStatus status = Globals()->status();

	if ((int)status <= (int)TheWorldStatus::worldDeployInProgress)
		return;
	
	if ((int)status >= (int)TheWorldStatus::worldUnDeployInProgress)
		return;

	Globals()->setStatus(TheWorldStatus::worldUnDeployInProgress);

	Globals()->Client()->MapManagerUndeployWorld(isInEditor());

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr)
	{
		editModeUIControl->setQuadrantHit(nullptr);
		editModeUIControl->set_visible(false);
	}

	m_stopDeploy = false;
}

void GDN_TheWorld_Viewer::destroyModeUIControl(void)
{
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr)
	{
		editModeUIControl->deinit();

		//Node* parent = editModeUIControl->get_parent();		// Useless it is freed in deinit
		//if (parent != nullptr)
		//	//parent->call_deferred("remove_child", editModeUIControl);	// Mah
		//	parent->remove_child(editModeUIControl);
		//editModeUIControl->queue_free();
		
		m_editModeUIControl = nullptr;
	}
}

Node3D* GDN_TheWorld_Viewer::getWorldNode(void) const
{
	// MapManager coordinates are relative to WorldNode
	return (Node3D*)get_parent();
}

//void GDN_TheWorld_Viewer::getPartialAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
//{
//	int idxFirstColFirstRowWorldVert = m_numWorldVerticesPerSize * firstWorldVertRow + firstWorldVertCol;
//	int idxLastColFirstRowWorldVert = m_numWorldVerticesPerSize * firstWorldVertRow + lastWorldVertCol;
//	int idxFirstColLastRowWorldVert = m_numWorldVerticesPerSize * lastWorldVertRow + firstWorldVertCol;
//	int idxLastColLastRowWorldVert = m_numWorldVerticesPerSize * lastWorldVertRow + lastWorldVertCol;
//
//	// altitudes
//	float minHeigth = 0, maxHeigth = 0;
//	bool firstTime = true;
//	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
//	{
//		for (int idxVert = idxFirstColFirstRowWorldVert + idxRow * m_numWorldVerticesPerSize; idxVert < idxLastColFirstRowWorldVert + idxRow * m_numWorldVerticesPerSize + 1; idxVert += step)
//		{
//			if (firstTime)
//			{
//				minHeigth = maxHeigth = m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude();
//				firstTime = false;
//			}
//			else
//			{
//				minHeigth = Utils::min2(minHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude());
//				maxHeigth = Utils::max2(maxHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxVert].altitude());
//			}
//		}
//	}
//	
//	Vector3 startPosition(m_quadTree->getQuadrant()->getGridVertices()[idxFirstColFirstRowWorldVert].posX(), minHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxFirstColFirstRowWorldVert].posZ());
//	//Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX() - m_worldVertices[idxFirstColFirstRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ() - m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
//	Vector3 endPosition(m_quadTree->getQuadrant()->getGridVertices()[idxLastColLastRowWorldVert].posX(), maxHeigth, m_quadTree->getQuadrant()->getGridVertices()[idxLastColLastRowWorldVert].posZ());
//	Vector3 size = endPosition - startPosition;
//
//	aabb.set_position(startPosition);
//	aabb.set_size(size);
//}

void GDN_TheWorld_Viewer::onTransformChanged(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("Transform changed");
	
	//The transformand other properties can be set by the scene loader, before we enter the tree
	if (!is_inside_tree())
		return;

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	//godot::Camera3D* activeCamera = nullptr;
	//if (IS_EDITOR_HINT())
	//{
	//	activeCamera = getWorldCamera();
	//}
	//else
	//{
	//	activeCamera = getCachedCustomWorldCamera()->getActiveCamera();
	//}
	//godot::Camera3D* activeWorldCamera = getWorldCamera();

	// WARNING: distanceFromCamera must be unique to avoid to lose quads for duplicate distance
	//std::map<float, QuadTree*> mapByDistancefromCamera;
	//for (auto& item : m_worldQuadrantsMap)
	//{
	//	float distanceFromCamera = 0;
	//	if (activeCamera != nullptr)
	//	{
	//		//godot::Vector3 quadrantCenter(item.first.getCenterX(), 0, item.first.getCenterZ());	// DEBUG ONLY
	//		godot::Transform3D t(godot::Basis(), item.second->getQuadrant()->getCenter());
	//		godot::Vector3 quadrantCenterGlobal = worldToGlobal(t).origin;
	//		distanceFromCamera = quadrantCenterGlobal.distance_to(activeCamera->get_global_position());
	//	}

	//	mapByDistancefromCamera[distanceFromCamera] = item.second.get();
	//}

	//if (mapByDistancefromCamera.size() != m_worldQuadrantsMap.size())																		// DEBUG ONLY
	//	m_lastCameraAltitudeSetToShader = -1.0f;																					// DEBUG ONLY

	m_lastCameraAltitudeSetToShader = -1.0f;
	m_lastCameraPosSetToShaderInitialized = false;

	for (auto& item : m_worldQuadrantsMap)
	{
		//if (item.second->getQuadrant()->getPos().getIndexPosX() == 0 && item.second->getQuadrant()->getPos().getIndexPosZ() == 12)	// DEBUG ONLY
		//	m_lastCameraAltitudeSetToShader = -1.0f;																				// DEBUG ONLY
		
		if (item.second->isValid() || item.second->isRefreshingData())
		{
			item.second->onGlobalTransformChanged();

			Chunk::TransformChangedChunkAction action;
			item.second->ForAllChunk(action, QuadType::NotSet);

			item.second->materialParamsNeedUpdate(true, true);
			//item.second->getQuadrant()->setHeightsUpdated(true);
			//item.second->getQuadrant()->setNormalsUpdated(true);
			//item.second->getQuadrant()->setSplatmapUpdated(true);
			//item.second->getQuadrant()->setColorsUpdated(true);
			//item.second->getQuadrant()->setGlobalmapUpdated(true);
			bool updateTerrainData = false;
			item.second->updateMaterialParams(updateTerrainData);

			item.second->getQuadrant()->getCollider()->onGlobalTransformChanged();
		}
	}
}

void GDN_TheWorld_Viewer::resetMaterialParamsForEveryQuadrant(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("resetMaterialParamsForEveryQuadrant");

	if (!is_inside_tree())
		return;

	for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;

		if (itQuadTree->second->status() != QuadrantStatus::uninitialized && itQuadTree->second->status() != QuadrantStatus::toErase)
			itQuadTree->second->materialParamsNeedReset(true);
	}
}

void GDN_TheWorld_Viewer::updateMaterialParamsForEveryQuadrant(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("updateMaterialParamsForEveryQuadrant");

	if (!is_inside_tree())
		return;

	for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	{
		if (!itQuadTree->second->isValid())
			continue;

		if (itQuadTree->second->status() != QuadrantStatus::uninitialized && itQuadTree->second->status() != QuadrantStatus::toErase)
			itQuadTree->second->materialParamsNeedUpdate(true);
	}
}

void GDN_TheWorld_Viewer::setColliderNeedUpdateForEveryQuadrant(void)
{
	for (auto& itQuadTree : m_worldQuadrantsMap)
		if (itQuadTree.second->isValid())
			itQuadTree.second->colliderNeedReset(true, QuadType::NotSet);
}

//void GDN_TheWorld_Viewer::setMapScale(Vector3 mapScaleVector)
//{
//	if (m_mapScaleVector == mapScaleVector)
//		return;
//
//	mapScaleVector.x = Utils::max2(mapScaleVector.x, MIN_MAP_SCALE);
//	mapScaleVector.y = Utils::max2(mapScaleVector.y, MIN_MAP_SCALE);
//	mapScaleVector.z = Utils::max2(mapScaleVector.z, MIN_MAP_SCALE);
//
//	m_mapScaleVector = mapScaleVector;
//
//	onTransformChanged();
//}

AABB GDN_TheWorld_Viewer::getCameraTerrainChunkLocalAABB(void)
{
	if (m_cameraTerrainChunk && !m_cameraTerrainChunk->isMeshNull())
		return m_cameraTerrainChunk->getAABB();
	else
		return AABB();
}

AABB GDN_TheWorld_Viewer::getCameraTerrainChunkLocalDebugAABB(void)
{
	if (m_cameraTerrainChunk && !m_cameraTerrainChunk->isDebugMeshNull())
		return m_cameraTerrainChunk->getDebugMeshAABB();
	else
		return AABB();
}

Transform3D GDN_TheWorld_Viewer::getCameraTerrainChunkGlobalTransformApplied(void)
{
	if (m_cameraTerrainChunk && !m_cameraTerrainChunk->isMeshNull())
		return m_cameraTerrainChunk->getGlobalTransformApplied();
	else
		return Transform3D();
}

Transform3D GDN_TheWorld_Viewer::getCameraTerrainChunkDebugGlobalTransformApplied(void)
{
	if (m_cameraTerrainChunk && !m_cameraTerrainChunk->isDebugMeshNull())
		return m_cameraTerrainChunk->getDebugGlobalTransformApplied();
	else
		return Transform3D();
}


String GDN_TheWorld_Viewer::getCameraTerrainChunkId(void)
{
	if (m_cameraTerrainChunk)
		return m_cameraTerrainChunk->getPos().getIdStr().c_str();
	else
		return "";
}

String GDN_TheWorld_Viewer::getCameraTerrainQuadrantName(void)
{
	if (m_cameraTerrainChunk)
		return m_cameraTerrainChunk->getQuadTree()->getQuadrant()->getPos().getName().c_str();
	else
		return "";
}

void GDN_TheWorld_Viewer::refreshQuadrantStatistics()
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);

	int numSplits = 0;
	int numJoins = 0;
	int numVisibleChunks = 0;
	map<int, size_t> mapNumActiveChunksPerLod;
	int numActiveChunks = 0;
	int numVisibleTerrainChunks = 0;
	map<int, size_t> mapNumActiveTerrainChunksPerLod;
	int numActiveTerrainChunks = 0;
	int numVisibleSeaChunks = 0;
	map<int, size_t> mapNumActiveSeaChunksPerLod;
	int numActiveSeaChunks = 0;
	int numQuadrant = (int)m_worldQuadrantsMap.size();
	int numinitializedQuadrant = 0;
	int numToEraseQuadrants = 0;
	int numVisibleQuadrant = 0;
	int numEmptyQuadrant = 0;
	int numQuadrantReloadingFromCache = 0;
	int numFlushedQuadrant = 0;
	int numQuadrantWithCollider = 0;
	int numQuadrantWithEnabledCollider = 0;
	int numQuadrantWithHiResCollider = 0;
	int numQuadrantWithLowResCollider = 0;
	int numinitializedVisibleQuadrant = 0;
	int numQuadrantNeedResetMaterialParams = 0;
	m_mapNumActiveChunksPerLod.clear();
	m_mapNumActiveTerrainChunksPerLod.clear();
	m_mapNumActiveSeaChunksPerLod.clear();

	//for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	for (auto& itQuadTree : m_worldQuadrantsMap)
	{
		if (itQuadTree.second->isVisible())
		{
			numVisibleQuadrant++;
			if (itQuadTree.second->statusInitialized())
				numinitializedVisibleQuadrant++;
			if (itQuadTree.second->materialParamsNeedReset())
				numQuadrantNeedResetMaterialParams++;
		}
		
		if (itQuadTree.second->statusInitialized())
		{
			numSplits += itQuadTree.second->getNumSplits();
			numJoins += itQuadTree.second->getNumJoins();
			numVisibleChunks += itQuadTree.second->getNumChunks(true, QuadType::NotSet);
			mapNumActiveChunksPerLod.clear();
			numActiveChunks += itQuadTree.second->getNumActiveChunks(mapNumActiveChunksPerLod, QuadType::NotSet);
			numVisibleTerrainChunks += itQuadTree.second->getNumChunks(true, QuadType::Terrain);
			mapNumActiveTerrainChunksPerLod.clear();
			numActiveTerrainChunks += itQuadTree.second->getNumActiveChunks(mapNumActiveTerrainChunksPerLod, QuadType::Terrain);
			numVisibleSeaChunks += itQuadTree.second->getNumChunks(true, QuadType::Sea);
			mapNumActiveSeaChunksPerLod.clear();
			numActiveSeaChunks += itQuadTree.second->getNumActiveChunks(mapNumActiveSeaChunksPerLod, QuadType::Sea);
			numinitializedQuadrant++;
		}

		if (itQuadTree.second->statusToErase())
			numToEraseQuadrants++;

		if (itQuadTree.second->getQuadrant()->empty())
			numEmptyQuadrant++;

		if (itQuadTree.second->statusLoadTerrainDataFromCacheNeeded() || itQuadTree.second->statusLoadTerrainDataFromCacheInProgress())
			numQuadrantReloadingFromCache++;

		if (itQuadTree.second->getQuadrant()->isFlushed())
			numFlushedQuadrant++;
		
		if (itQuadTree.second->getQuadrant()->getCollider() != nullptr && itQuadTree.second->getQuadrant()->getCollider()->hasData())
		{
			numQuadrantWithCollider++;
			if (itQuadTree.second->getQuadrant()->getCollider()->colliderEnabled())
				numQuadrantWithEnabledCollider++;
			if (itQuadTree.second->getQuadrant()->getCollider()->hasHiResData())
				numQuadrantWithHiResCollider++;
			else
				numQuadrantWithLowResCollider++;
		}
		
		for (auto& item : mapNumActiveChunksPerLod)
		{
			if (m_mapNumActiveChunksPerLod.contains(item.first))
				m_mapNumActiveChunksPerLod[item.first] += item.second;
			else
				m_mapNumActiveChunksPerLod[item.first] = item.second;
		}
		for (auto& item : mapNumActiveTerrainChunksPerLod)
		{
			if (m_mapNumActiveTerrainChunksPerLod.contains(item.first))
				m_mapNumActiveTerrainChunksPerLod[item.first] += item.second;
			else
				m_mapNumActiveTerrainChunksPerLod[item.first] = item.second;
		}
		for (auto& item : mapNumActiveSeaChunksPerLod)
		{
			if (m_mapNumActiveSeaChunksPerLod.contains(item.first))
				m_mapNumActiveSeaChunksPerLod[item.first] += item.second;
			else
				m_mapNumActiveSeaChunksPerLod[item.first] = item.second;
		}
	}

	GDN_TheWorld_Map* map = Map();
	m_mapQuadrantsNum = m_mapQuadrantsMap.size();
	m_updatingMapQuadrantsNum = m_mapQuadrantsMap.getNumUpdatingQuadrants();
	m_toShowMapQuadrantsNum = m_mapQuadrantsMap.getNumQuadrantsToShow();
	m_mapQuadrantRects = map->getNumTextureRects();

	m_numSplits = numSplits;
	m_numJoins = numJoins;
	m_numChunks = numVisibleChunks;
	m_numActiveChunks = numActiveChunks;
	m_numTerrainChunks = numVisibleTerrainChunks;
	m_numActiveTerrainChunks = numActiveTerrainChunks;
	m_numSeaChunks = numVisibleSeaChunks;
	m_numActiveSeaChunks = numActiveSeaChunks;
	m_numQuadrant = numQuadrant;
	m_numInitializedQuadrant = numinitializedQuadrant;
	m_numToEraseQuadrant = numToEraseQuadrants;
	m_numVisibleQuadrant = numVisibleQuadrant;
	m_numEmptyQuadrant = numEmptyQuadrant;
	m_numQuadrantReloadingFromCache = numQuadrantReloadingFromCache;
	m_numFlushedQuadrant = numFlushedQuadrant;
	m_numQuadrantWithCollider = numQuadrantWithCollider;
	m_numQuadrantWithEnabledCollider = numQuadrantWithEnabledCollider;
	m_numQuadrantWithHiResCollider = numQuadrantWithHiResCollider;
	m_numQuadrantWithLowResCollider = numQuadrantWithLowResCollider;
	m_numInitializedVisibleQuadrant = numinitializedVisibleQuadrant;
	m_numQuadrantNeedResetMaterialParam = numQuadrantNeedResetMaterialParams;
}

int GDN_TheWorld_Viewer::getProcessNotOwnsLock(void) const
{
	return m_numProcessNotOwnsLock;
}

int GDN_TheWorld_Viewer::getNumChunks(void) const
{
	return m_numChunks;
}

int GDN_TheWorld_Viewer::getNumActiveChunks(void) const
{
	return m_numActiveChunks;
}

int GDN_TheWorld_Viewer::getNumTerrainChunks(void) const
{
	return m_numTerrainChunks;
}

int GDN_TheWorld_Viewer::getNumActiveTerrainChunks(void) const
{
	return m_numActiveTerrainChunks;
}

int GDN_TheWorld_Viewer::getNumSeaChunks(void) const
{
	return m_numSeaChunks;
}

int GDN_TheWorld_Viewer::getNumActiveSeaChunks(void) const
{
	return m_numActiveSeaChunks;
}

int GDN_TheWorld_Viewer::getNumSplits(void) const
{
	return m_numSplits;
}

int GDN_TheWorld_Viewer::getNumJoins(void) const
{
	return m_numJoins;
}

int GDN_TheWorld_Viewer::getNumQuadrant(void) const
{
	return m_numQuadrant;
}

int GDN_TheWorld_Viewer::getNumInitializedQuadrant(void) const
{
	return m_numInitializedQuadrant;
}

int GDN_TheWorld_Viewer::getNumToEraseQuadrant(void) const
{
	return m_numToEraseQuadrant;
}

int GDN_TheWorld_Viewer::getNumVisibleQuadrant(void) const
{
	return m_numVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getNumEmptyQuadrant(void) const
{
	return m_numEmptyQuadrant;
}

int GDN_TheWorld_Viewer::getNumQuadrantReloadingFromCache(void) const
{
	return m_numQuadrantReloadingFromCache;
}

int GDN_TheWorld_Viewer::getNumFlushedQuadrant(void) const
{
	return m_numFlushedQuadrant;
}

int GDN_TheWorld_Viewer::getQuadrantNumWithCollider(void) const
{
	return m_numQuadrantWithCollider;
}
int GDN_TheWorld_Viewer::getQuadrantNumWithEnabledCollider(void) const
{
	return m_numQuadrantWithEnabledCollider;
}
int GDN_TheWorld_Viewer::getQuadrantNumWithHiResCollider(void) const
{
	return m_numQuadrantWithHiResCollider;
}
int GDN_TheWorld_Viewer::getQuadrantNumWithLowResCollider(void) const
{
	return m_numQuadrantWithLowResCollider;
}
int GDN_TheWorld_Viewer::getNumInitializedVisibleQuadrant(void) const
{
	return m_numInitializedVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getNumNeedShaderResetQuadrant(void) const
{
	return m_numQuadrantNeedResetMaterialParam;
}

int GDN_TheWorld_Viewer::getProcessDuration(void) const
{
	return (int)m_averageProcessDuration;
}

//int GDN_TheWorld_Viewer::getRefreshMapQuadDuration(void)
//{
//	return (int)m_averageRefreshMapQuadDuration;
//}

int GDN_TheWorld_Viewer::getUpdateQuads1Duration(void) const
{
	return (int)m_averageUpdateQuads1Duration;
}

int GDN_TheWorld_Viewer::getUpdateQuads2Duration(void) const
{
	return (int)m_averageUpdateQuads2Duration;
}

int GDN_TheWorld_Viewer::getUpdateQuads3Duration(void) const
{
	return (int)m_averageUpdateQuads3Duration;
}

int GDN_TheWorld_Viewer::getUpdateChunksDuration(void) const
{
	return (int)m_averageUpdateChunksDuration;
}

int GDN_TheWorld_Viewer::getUpdateMaterialParamsDuration(void) const
{
	return (int)m_averageUpdateMaterialParamsDuration;
}

int GDN_TheWorld_Viewer::getMouseTrackHitDuration(void) const
{
	return (int)m_averageMouseTrackHitDuration;
}

void GDN_TheWorld_Viewer::dump()
{
	Globals()->debugPrint("*************");
	Globals()->debugPrint("STARTING DUMP");

	double f = Engine::get_singleton()->get_frames_per_second();
	Globals()->debugPrint("FPS: " + String(std::to_string(f).c_str()));

	{
		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr && !editModeUIControl->initialized())
			editModeUIControl = nullptr;

		size_t numQuadrantsWithSea = 0;
		std::map<size_t, size_t> mapOptimizationLevels;
		std::map<int, size_t> mapQuadrantsToSaveTerrainType;
		std::map<int, size_t> mapQuadrantsToUploadTerrainType;
		size_t mapMemoryOccupation = 0, heightmap16MemoryOccupation = 0, heightmap32MemoryOccupation = 0, normalmapMemoryOccupation = 0, splatmapMemoryOccupation = 0, globalmapMemoryOccupation = 0, colormapMemoryOccupation = 0, terrainEditMemoryOccupation = 0, colliderHeightsMemoryOccupation = 0;
		size_t texMemoryOccupation = 0;
		size_t resizedTexMemoryOccupation = 0;
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		for (auto& quadTree : m_worldQuadrantsMap)
		{
			if (quadTree.second->containsSeaChunks())
				numQuadrantsWithSea++;

			mapMemoryOccupation += quadTree.second->getQuadrant()->calcMapMemoryOccupation();
			heightmap16MemoryOccupation += quadTree.second->getQuadrant()->calcHeightmap16MemoryOccupation();
			heightmap32MemoryOccupation += quadTree.second->getQuadrant()->calcHeightmap32MemoryOccupation();
			normalmapMemoryOccupation += quadTree.second->getQuadrant()->calcNormalmapMemoryOccupation();
			splatmapMemoryOccupation += quadTree.second->getQuadrant()->calcSplatmapMemoryOccupation();
			globalmapMemoryOccupation += quadTree.second->getQuadrant()->calcGlobalmapMemoryOccupation();
			colormapMemoryOccupation += quadTree.second->getQuadrant()->calcColormapMemoryOccupation();
			terrainEditMemoryOccupation += quadTree.second->getQuadrant()->calcTerrainEditMemoryOccupation();
			colliderHeightsMemoryOccupation += quadTree.second->getQuadrant()->calcColliderHeightsMemoryOccupation();

			texMemoryOccupation += quadTree.second->getQuadrant()->calcTexMemoryOccupation();
			resizedTexMemoryOccupation += quadTree.second->getQuadrant()->calcResizedTexMemoryOccupation();
			
			size_t optimizationLevel = quadTree.second->getQuadrant()->optimizationLevel();
			if (mapOptimizationLevels.contains(optimizationLevel))
				mapOptimizationLevels[optimizationLevel]++;
			else
				mapOptimizationLevels[optimizationLevel] = 1;

			int terrainType = (int)quadTree.second->getQuadrant()->getTerrainEdit()->terrainType;
			TheWorld_Utils::QuadrantPos pos = quadTree.second->getQuadrant()->getPos();
			bool manual;
			if (editModeUIControl != nullptr && editModeUIControl->isQuadToSave(pos, manual))
			{
				if (mapQuadrantsToSaveTerrainType.contains(terrainType))
					mapQuadrantsToSaveTerrainType[terrainType]++;
				else
					mapQuadrantsToSaveTerrainType[terrainType] = 1;
			}
			
			if (quadTree.second->getQuadrant()->getTerrainEdit()->needUploadToServer)
			{
				if (mapQuadrantsToUploadTerrainType.contains(terrainType))
					mapQuadrantsToUploadTerrainType[terrainType]++;
				else
					mapQuadrantsToUploadTerrainType[terrainType] = 1;
			}
		}

		godot::Performance* perf = godot::Performance::get_singleton();

		float thresholdCacheHeights = Globals()->getParamThresholdCacheHeights();
		char _thresholdCacheHeights[24]; // dummy size, you should take care of the size!
		sprintf_s(_thresholdCacheHeights, "%.2f", thresholdCacheHeights);
		Globals()->debugPrint(String("Threshold value to cache maps and images : ") + std::string(_thresholdCacheHeights).c_str() + " m");
		Globals()->debugPrint(String("Quadrants : ") + String(std::to_string(m_worldQuadrantsMap.size()).c_str()) + ", cached map memory : " + String(std::to_string((float)mapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB, cached image memory : " + String(std::to_string((float)texMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB (resized cached image memory : " + String(std::to_string((float)resizedTexMemoryOccupation / (1024 * 1024 * 1024)).c_str())  + " GB)");
		Globals()->debugPrint(String("   Heigthmap16 : ") + String(std::to_string((float)heightmap16MemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Heigthmap32 : ") + String(std::to_string((float)heightmap32MemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Normalmap : ") + String(std::to_string((float)normalmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Splatmap : ") + String(std::to_string((float)splatmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Globalmap : ") + String(std::to_string((float)globalmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Colormap : ") + String(std::to_string((float)colormapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   TerrainEdit : ") + String(std::to_string((float)terrainEditMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("   Collider Heights : ") + String(std::to_string((float)colliderHeightsMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		Globals()->debugPrint(String("Engine monitor : "));
		Globals()->debugPrint(String("   Static memory in use : ") + std::to_string(perf->get_monitor(godot::Performance::Monitor::MEMORY_STATIC) / (1024 * 1024 * 1024)).c_str() + " GB");
		Globals()->debugPrint(String("   Render video memory in use : ") + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_VIDEO_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB, for texture " + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_TEXTURE_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB, for buffers " + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_BUFFER_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB");

		Globals()->debugPrint(String("Num. quadrants with sea = ") + std::to_string(numQuadrantsWithSea).c_str());

		Globals()->debugPrint(String("Min. num. visible quadrants on perimeter : ") + std::to_string(m_minVisibleQuadrantOnPerimeter).c_str());
		Globals()->debugPrint(String("Max. num. visible quadrants on perimeter : ") + std::to_string(getMaxDepthQuadOnPerimeter()).c_str());
		Globals()->debugPrint(String("Num. cache quadrants on perimeter : ") + std::to_string(m_numCacheQuadrantOnPerimeter).c_str());
		Globals()->debugPrint(String("Actual num quadrants on perimeter on north side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnNorthSide).c_str());
		Globals()->debugPrint(String("Actual num quadrants on perimeter on east side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnEastSide).c_str());
		Globals()->debugPrint(String("Actual num quadrants on perimeter on south side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnSouthSide).c_str());
		Globals()->debugPrint(String("Actual num quadrants on perimeter on west side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnWestSide).c_str());

		for (auto& item : mapOptimizationLevels)
		{
			//size_t resizedMapSize = (Globals()->heightmapResolution() / (size_t)pow(2, item.first)) + 1;
			size_t resizedMapSize = Globals()->resolutionForOptimizationLevel(item.first);
			std::string msg = "Num quadrants with optimization level " + std::to_string(item.first) + " : " + std::to_string(item.second) + " - map size : " + std::to_string(resizedMapSize);
			Globals()->debugPrint(msg.c_str());
		}

		for (auto& item : mapQuadrantsToSaveTerrainType)
		{
			std::string msg = "Num quadrants to save with terrain type <" + TheWorld_Utils::TerrainEdit::terrainTypeString((TheWorld_Utils::TerrainEdit::TerrainType)item.first) + "> : " + std::to_string(item.second);
			Globals()->debugPrint(msg.c_str());
		}
		for (auto& item : mapQuadrantsToUploadTerrainType)
		{
			std::string msg = "Num quadrants to upload with terrain type <" + TheWorld_Utils::TerrainEdit::terrainTypeString((TheWorld_Utils::TerrainEdit::TerrainType)item.first) + "> : " + std::to_string(item.second);
			Globals()->debugPrint(msg.c_str());
		}

		for (auto& item : m_mapNumActiveChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = Globals()->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = Globals()->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(Globals()->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * Globals()->gridStepInHeightmapWUs(lod));

				Globals()->debugPrint(msg.c_str());
		}
		Globals()->debugPrint("Terrain Chunks");
		for (auto& item : m_mapNumActiveTerrainChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = Globals()->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = Globals()->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(Globals()->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * Globals()->gridStepInHeightmapWUs(lod));

			Globals()->debugPrint(msg.c_str());
		}
		Globals()->debugPrint("Sea Chunks");
		for (auto& item : m_mapNumActiveSeaChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = Globals()->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = Globals()->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(Globals()->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * Globals()->gridStepInHeightmapWUs(lod));

			Globals()->debugPrint(msg.c_str());
		}
	}

	//Node* node = get_node(NodePath("/root"));
	//if (node != nullptr)
	//{
	//	Globals()->debugPrint("=== NODES ===========================================");
	//	Globals()->debugPrint("");
	//	Globals()->debugPrint("@@2 = res://native/GDN_TheWorld_Viewer.gdns");
	//	Globals()->debugPrint("");
	//	Globals()->debugPrint(node->get_name());
	//	Array nodes = node->get_children();
	//	dumpRecurseIntoChildrenNodes(nodes, 1);
	//	Globals()->debugPrint("=== NODES ===========================================");
	//}
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			itQuadTree->second->dump(true);
		}
	}

	std::vector<std::string> names = TheWorld_Utils::Profiler::names("");
	if (names.size() > 0)
	{
		Globals()->debugPrint("== PROFILE DATA ==========================================");
		for (auto& name : names)
		{
			size_t num = 0;
			size_t elapsed = TheWorld_Utils::Profiler::elapsedMs("", name, num);
			size_t avgElapsed = TheWorld_Utils::Profiler::averageElapsedMs("", name, num);
			size_t minElapsed = TheWorld_Utils::Profiler::minElapsedMs("", name);
			size_t maxElapsed = TheWorld_Utils::Profiler::maxElapsedMs("", name);
			if (num > 0)
			{
				Globals()->debugPrint(String(name.c_str()) + " num=" + std::to_string(num).c_str() + " Elapsed=" + std::to_string(elapsed).c_str() + " MinElapsed=" + std::to_string(minElapsed).c_str() + " MaxElapsed=" + std::to_string(maxElapsed).c_str() + " AvgElapsed=" + std::to_string(avgElapsed).c_str());
				TheWorld_Utils::Profiler::reset("", name);
			}
		}
		Globals()->debugPrint("== PROFILE DATA ==========================================");
	}

	Globals()->debugPrint("DUMP COMPLETE");
	Globals()->debugPrint("*************");

	//godot::Viewport* vp = get_viewport();
	//godot::Ref<godot::ViewportTexture> vpt = vp->get_texture();
	//godot::Vector2 size = vpt->get_size();
	//godot::Ref<godot::Image> image = vpt->get_image();
	//std::string outFileName = GDN_TheWorld_Globals::getClientDataDir() + "\\screen.png";
	//image->save_png(outFileName.c_str());
}

void GDN_TheWorld_Viewer::dumpRecurseIntoChildrenNodes(godot::Array nodes, int level)
{
	for (int i = 0; i < nodes.size(); i++)
	{
		std::string header(level, '\t');
		godot::Node* node = godot::Object::cast_to<godot::Node>(nodes[i]);
		Globals()->debugPrint(godot::String(header.c_str()) + node->get_name());
		godot::Array nodes = node->get_children();
		dumpRecurseIntoChildrenNodes(nodes, level + 1);
	}
}

void GDN_TheWorld_Viewer::setCameraTerrainChunk(Chunk* chunk, QuadTree* quadTree)
{
	if (m_cameraTerrainChunk != nullptr && m_cameraTerrainChunk != chunk)
	{
		m_cameraTerrainChunk->resetCameraVerticalOnChunk();
		quadTree->addChunkUpdate(m_cameraTerrainChunk, chunk->getOwnQuadType());
	}

	m_cameraTerrainChunk = chunk;
	//m_cameraQuadTree = quadTree;
}

void GDN_TheWorld_Viewer::streamer(void)
{
	m_streamerThreadRunning = true;

	Globals()->debugPrint("streamer thread STARTED");

	TheWorld_Viewer_Utils::MsTimePoint lastSyncTime = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	while (!m_streamerThreadRequiredExit)
	{
		try
		{
			try
			{
				{
					//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing);
					std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
					if (lock.owns_lock())
					{
						for (WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.begin(); it != m_worldQuadrantsMap.end(); it++)
						{
							bool reset = false;

							if (it->second->statusToErase() && !it->second->getQuadrant()->internalDataLocked())
							{
								m_toErase[it->first] = true;
							}
						}

						lock.unlock();
					}
				}

				// Optimization: calc here getHeightsForCollider, getFloat16HeightsBuffer, getFloat32HeightsBuffer, getNormalsBuffer if they are empty and if we know they will be loaded in next _process
				// with this optimization we want to do the work outside _process to have more fluidity
				
				size_t numNeededQuads = static_cast<size_t>(1 + m_numVisibleQuadrantOnPerimeterOnNorthSide + m_numVisibleQuadrantOnPerimeterOnSouthSide + 2 * getCacheQuadOnPerimeter())
					* static_cast<size_t>(1 + m_numVisibleQuadrantOnPerimeterOnWestSide + m_numVisibleQuadrantOnPerimeterOnEastSide + 2 * getCacheQuadOnPerimeter());
				if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployed && m_worldQuadrantsMap.size() < numNeededQuads)
					m_recalcQuadrantsInViewNeeded = true;

				if (m_numVisibleQuadrantOnPerimeterOnNorthSideApplied != m_numVisibleQuadrantOnPerimeterOnNorthSide
					|| m_numVisibleQuadrantOnPerimeterOnSouthSideApplied != m_numVisibleQuadrantOnPerimeterOnSouthSide
					|| m_numVisibleQuadrantOnPerimeterOnWestSideApplied != m_numVisibleQuadrantOnPerimeterOnWestSide
					|| m_numVisibleQuadrantOnPerimeterOnEastSideApplied != m_numVisibleQuadrantOnPerimeterOnEastSide
					)
					m_recalcQuadrantsInViewNeeded = true;
				
				if (m_recalcQuadrantsInViewNeeded && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
				{
					recalcQuadrantsInView();
					m_recalcQuadrantsInViewNeeded = false;
				}
				
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				if ((now - lastSyncTime).count() > STREAMER_SYNC_PERIOD_TIME)
				{
					std::string quadrants;
					if (!m_quadrantSyncRunning && Globals()->status() == TheWorldStatus::worldDeployed)
					{
						std::unique_lock<std::recursive_mutex> lock(m_mtxQuadTreeAndMainProcessing, std::try_to_lock);
						if (lock.owns_lock())
						{
							m_quadrantSyncRunning = true;

							TheWorld_Utils::MemoryBuffer buffer;
							buffer.reserve(100000);
							
							*buffer.ptr() = '0';
							size_t cursor = 1;

							size_t size;
							TheWorld_Utils::serializeToByteStream<size_t>(m_worldQuadrantsMap.size(), buffer.ptr() + cursor, size);
							cursor += size;
							
							for (auto& worldQuadrantsMapItem : m_worldQuadrantsMap)
							{
								TheWorld_Utils::MemoryBuffer quadrantPosBuffer;
								worldQuadrantsMapItem.first.serialize(quadrantPosBuffer);

								buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
								TheWorld_Utils::serializeToByteStream<size_t>(quadrantPosBuffer.size(), buffer.ptr() + cursor, size);
								cursor += size;

								buffer.reserve(cursor + quadrantPosBuffer.size());		// to avoid memory overflow
								memcpy(buffer.ptr() + cursor, quadrantPosBuffer.ptr(), quadrantPosBuffer.size());
								cursor += quadrantPosBuffer.size();

								std::string meshId = worldQuadrantsMapItem.second->getQuadrant()->getMeshId();

								buffer.reserve(cursor + 8);		// to avoid memory overflow
								TheWorld_Utils::serializeToByteStream<size_t>(meshId.size(), buffer.ptr() + cursor, size);
								cursor += size;

								buffer.reserve(cursor + meshId.size());		// to avoid memory overflow
								memcpy(buffer.ptr() + cursor, meshId.c_str(), meshId.size());
								cursor += meshId.size();
							}
							
							buffer.adjustSize(cursor);
							buffer.toString(quadrants);
						}
					}
					Globals()->Client()->Sync(isInEditor(), isEditMode(), Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU(), quadrants);
					lastSyncTime = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				}

				if (m_dumpRequired)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("streamer 1 ") + __FUNCTION__, "Dump stuff");

					m_dumpRequired = false;
					dump();
				}
			}
			catch (std::exception& e)
			{
				Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::streamer - std::exception caught - ") + e.what()).c_str());
				throw(e);
			}
			catch (...)
			{
				Globals()->errorPrint("GDN_TheWorld_Viewer::streamer - Exception caught");
				throw(new exception("exception caught"));
			}
		}
		catch (std::exception& e)
		{
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, string("GDN_TheWorld_Viewer::streamer - std::exception caught - ") + e.what());
		}
		catch (...)
		{
			Globals()->_setAppInError(THEWORLD_VIEWER_GENERIC_ERROR, "GDN_TheWorld_Viewer::streamer - Exception caught");
		}

		Sleep(STREAMER_SLEEP_TIME);
	}

	Globals()->debugPrint("streamer thread STOPPED");

	m_streamerThreadRunning = false;
}

Transform3D GDN_TheWorld_Viewer::getInternalGlobalTransform(void) const
{
	Transform3D gt = get_global_transform();
	return gt;
}

bool GDN_TheWorld_Viewer::loadQuadrantDataIndex(int posX, int posZ, int level)
{
	size_t numVerticesPerSize = Globals()->numVerticesPerQuadrantSize();
	float gridStepInWU = Globals()->gridStepInWU();
	TheWorld_Utils::QuadrantPos pos(posX, posZ, level, (int)numVerticesPerSize, gridStepInWU);
	
	return loadQuadrantData(pos.getLowerXGridVertex(), pos.getLowerZGridVertex(), level);
}

bool GDN_TheWorld_Viewer::loadQuadrantData(float posX, float posZ, int level)
{
	size_t numVerticesPerSize = Globals()->numVerticesPerQuadrantSize();
	float gridStepInWU = Globals()->gridStepInWU();
	TheWorld_Utils::QuadrantPos pos(posX, posZ, level, (int)numVerticesPerSize, gridStepInWU);

	std::string dir = GDN_TheWorld_Globals::getClientDataDir();
	std::string mapName = Globals()->getMapName();
	TheWorld_Utils::MeshCacheBuffer loadedCache = TheWorld_Utils::MeshCacheBuffer(dir, mapName, pos.getGridStepInWU(), pos.getNumVerticesPerSize(), pos.getLevel(), pos.getLowerXGridVertex(), pos.getLowerZGridVertex());

	TheWorld_Utils::MemoryBuffer buffer;
	loadedCache.readBufferFromDisk("", buffer);
	if (buffer.empty())
		return false;

	TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
	TheWorld_Utils::MemoryBuffer terrainEditValues;
	TheWorld_Utils::MemoryBuffer heights16Buffer;
	TheWorld_Utils::MemoryBuffer heights32Buffer;
	TheWorld_Utils::MemoryBuffer normalsBuffer;
	TheWorld_Utils::MemoryBuffer splatmapBuffer;
	TheWorld_Utils::MemoryBuffer globalmapBuffer;
	TheWorld_Utils::MemoryBuffer colormapBuffer;
	//cacheQuadrantData.terrainEdit = &m_tempTerrainEdit;
	cacheQuadrantData.terrainEditValues = &terrainEditValues;
	cacheQuadrantData.heights16Buffer = &heights16Buffer;
	cacheQuadrantData.heights32Buffer = &heights32Buffer;
	cacheQuadrantData.normalsBuffer = &normalsBuffer;
	cacheQuadrantData.splatmapBuffer = &splatmapBuffer;
	cacheQuadrantData.globalmapBuffer = &globalmapBuffer;
	cacheQuadrantData.colormapBuffer = &colormapBuffer;
	loadedCache.refreshCacheQuadrantDataFromBuffer(buffer, cacheQuadrantData, false);
	
	m_tempPos = pos;

	if (terrainEditValues.size() > 0)
		m_tempTerrainEdit.deserialize(terrainEditValues);

	m_tempHeightmapImage = ShaderTerrainData::createHeightmapImage(heights16Buffer, (int)numVerticesPerSize);
	m_tempNormalmapImage = ShaderTerrainData::createNormalmapImage(normalsBuffer, (int)numVerticesPerSize);
	m_tempSplatmapImage = ShaderTerrainData::createSplatlmapImage(splatmapBuffer, (int)numVerticesPerSize);
	m_tempGlobalmapImage = ShaderTerrainData::createGloablmapImage(globalmapBuffer, (int)numVerticesPerSize);
	m_tempColormapImage = ShaderTerrainData::createColormapImage(colormapBuffer, (int)numVerticesPerSize);
	
	return true;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getQuadrantHeightmapImage()
{
	return m_tempHeightmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getQuadrantNormalmapImage()
{
	return m_tempNormalmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getQuadrantSplatmapImage()
{
	return m_tempSplatmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getQuadrantGlobalmapImage()
{
	return m_tempGlobalmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getQuadrantColormapImage()
{
	return m_tempColormapImage;
}

godot::Vector2 GDN_TheWorld_Viewer::getQuadrantPos(void)
{
	return godot::Vector2(m_tempPos.getLowerXGridVertex(), m_tempPos.getLowerZGridVertex());
}

godot::Vector2 GDN_TheWorld_Viewer::getQuadrantPosIndex(void)
{
	return godot::Vector2((float)m_tempPos.getIndexPosX(), (float)m_tempPos.getIndexPosZ());
}

godot::String GDN_TheWorld_Viewer::getGroundTextureName(int type)
{
	if (type == 0)
	{
		return m_tempTerrainEdit.extraValues.lowElevationTexName_r;
	}
	else if (type == 1)
	{
		return m_tempTerrainEdit.extraValues.highElevationTexName_g;
	}
	else if (type == 2)
	{
		return m_tempTerrainEdit.extraValues.dirtTexName_b;
	}
	else if (type == 3)
	{
		return m_tempTerrainEdit.extraValues.rocksTexName_a;
	}

	return "";
}

godot::Ref<godot::ImageTexture> GDN_TheWorld_Viewer::getGroundTextureAlbedoBump(godot::String texName)
{
	ShaderTerrainData::GroundTexture groundTexture;
	ShaderTerrainData::getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + texName, &groundTexture);
	return groundTexture.m_albedo_bump_tex;
}

godot::Ref<godot::ImageTexture> GDN_TheWorld_Viewer::getGroundTextureNormalRoughness(godot::String texName)
{
	ShaderTerrainData::GroundTexture groundTexture;
	ShaderTerrainData::getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + texName, &groundTexture);
	return groundTexture.m_normal_roughness_tex;
}

int GDN_TheWorld_Viewer::getGroundTextureLayer(godot::String texName)
{
	std::string _texName = texName.utf8().get_data();
	std::map<std::string, int32_t>& groundTextureLayers = ShaderTerrainData::getGroundTextureLayers();
	if (groundTextureLayers.contains(_texName))
		return groundTextureLayers[_texName];
	else
		return -1;
}

godot::Vector4 GDN_TheWorld_Viewer::getGroundTextureLayers(void)
{
	godot::Vector4 v(
		(float)getGroundTextureLayer(m_tempTerrainEdit.extraValues.lowElevationTexName_r),
		(float)getGroundTextureLayer(m_tempTerrainEdit.extraValues.highElevationTexName_g),
		(float)getGroundTextureLayer(m_tempTerrainEdit.extraValues.dirtTexName_b),
		(float)getGroundTextureLayer(m_tempTerrainEdit.extraValues.rocksTexName_a)
		);
	return v;
}

void GDN_TheWorld_Viewer::clearQuadrantData(void)
{
	m_tempHeightmapImage.unref();
	m_tempNormalmapImage.unref();
	m_tempSplatmapImage.unref();
	m_tempGlobalmapImage.unref();
	m_tempColormapImage.unref();
}

GDN_TheWorld_Viewer::DebugRayToUpdateGridDimensionDrawer::DebugRayToUpdateGridDimensionDrawer(GDN_TheWorld_Viewer* _viewer, godot::Camera3D* _camera, enum class Side _side, float _angle, godot::Vector3 _rayStart, godot::Vector3 _rayEnd, bool _hit, godot::Vector3 _hitPosition, bool _maxDistanceFromCameraForSide)
{
	viewer = _viewer;
	camera = _camera;
	side = _side;
	angle = _angle;
	m_rayDrawer = memnew(GDN_TheWorld_Drawer);
	viewer->add_child(m_rayDrawer);
	//rayIdx = m_rayDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::center, -1.0f, godot::Vector2i(-10, -10), GDN_TheWorld_Globals::g_color_black);
	sphere = m_rayDrawer->addSphere(godot::Vector3(), 5, GDN_TheWorld_Globals::g_color_black);
	label = m_rayDrawer->addLabel2d(godot::Vector3(), godot::Vector2i(-50, 20), "", GDN_TheWorld_Globals::g_color_black);
	m_rayDrawer->set_camera(camera, viewer);
	setup(_rayStart, _rayEnd, _hit, _hitPosition, _maxDistanceFromCameraForSide);
	m_rayDrawer->set_visible(true);
}

GDN_TheWorld_Viewer::DebugRayToUpdateGridDimensionDrawer::~DebugRayToUpdateGridDimensionDrawer(void)
{
	if (m_rayDrawer != nullptr)
	{
		Node* parent = m_rayDrawer->get_parent();
		if (parent != nullptr)
			parent->call_deferred("remove_child", m_rayDrawer);	// Mah
		m_rayDrawer->queue_free();
		m_rayDrawer = nullptr;
	}
}

void GDN_TheWorld_Viewer::DebugRayToUpdateGridDimensionDrawer::setup(godot::Vector3 _rayStart, godot::Vector3 _rayEnd, bool _hit, godot::Vector3 _hitPosition, bool _maxDistanceFromCameraForSide)
{
	rayStart = _rayStart;
	if (rayStart == godot::Vector3() && camera != nullptr && viewer != nullptr)
		rayStart = viewer->globalPosToWorldPos(camera->get_global_position());
	rayEnd = _rayEnd;
	hit = _hit;
	hitPosition = _hitPosition;
	maxDistanceFromCameraForSide = _maxDistanceFromCameraForSide;
	//godot::Vector3 start = _rayStart /* + godot::Vector3(0, -1.0f, 0)*/;
	if (m_rayDrawer != nullptr)
	{
		if (hit)
		{
			godot::Color c = GDN_TheWorld_Globals::g_color_red;
			c.a = 1.0f;
			//m_rayDrawer->updateLine(rayIdx, rayStart, hitPosition, "", GDN_TheWorld_Drawer::Drawing::LabelPos::not_set, -1.0f, godot::Vector2i(0, 0), c);
			float hitDistance = rayStart.distance_to(hitPosition);
			float distanceFromCameraInQuads = hitDistance / viewer->Globals()->heightmapSizeInWUs();
			std::string text = sideToString(side) + " " + std::format("{:.2f}", hitDistance) + "/" + std::format("{:.2f}", distanceFromCameraInQuads) + (maxDistanceFromCameraForSide ? " (*)" : "");
			m_rayDrawer->updateLabel2d(label, hitPosition, text, c);
			m_rayDrawer->updateSphere(sphere, hitPosition, hitDistance / 100, c);
		}
		else
		{
			godot::Color c = GDN_TheWorld_Globals::g_color_black;
			c.a = 1.0f;
			//m_rayDrawer->updateLine(rayIdx, rayStart, rayEnd, "", GDN_TheWorld_Drawer::Drawing::LabelPos::not_set, -1.0f, godot::Vector2i(0, 0), c);
			float distance = rayStart.distance_to(rayEnd);
			float distanceFromCameraInQuads = distance / viewer->Globals()->heightmapSizeInWUs();
			std::string text = sideToString(side) + " " + std::format("{:.2f}", distance) + "/" + std::format("{:.2f}", distanceFromCameraInQuads);
			m_rayDrawer->updateLabel2d(label, rayEnd, text, c);
			m_rayDrawer->updateSphere(sphere, rayEnd, distance / 100, c);
		}
	}
}

std::string GDN_TheWorld_Viewer::DebugRayToUpdateGridDimensionDrawer::sideToString(enum class Side side)
{
	switch (side)
	{
	case Side::North:
		return "N";
	case Side::South:
		return "S";
	case Side::East:
		return "E";
	case Side::West:
		return "W";
	default:
		return "";
	}
}

void GDN_TheWorld_Viewer::terrainHeightsChangedEvent(void)
{
	for (auto& item : m_worldQuadrantsMap)
	{
		Chunk::TerrainHeightsChangedChunkAction action;
		item.second->ForAllChunk(action, QuadType::NotSet);
		item.second->colliderNeedReset(true, QuadType::NotSet);
	}
}

