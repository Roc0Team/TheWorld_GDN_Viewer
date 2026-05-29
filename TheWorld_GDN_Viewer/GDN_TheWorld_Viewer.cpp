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
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/center_container.hpp>
#include <godot_cpp/classes/world_environment.hpp>
#include <godot_cpp/classes/environment.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/quad_mesh.hpp>
#include <godot_cpp/classes/fast_noise_lite.hpp>
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

void WorldQuadrantsMap::init(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
}

void MapQuadrantImagesMap::init(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;
	m_imageNumPixelsPerSize = (int32_t)m_viewer->Globals()->resolutionForOptimizationLevel((size_t)MapQuadrantImagesMap::getMapQuadrantOptimizationLevel());
}

//godot::Ref<godot::Image> MapQuadrantImagesMap::getMapImage(TheWorld_Utils::QuadrantPos& quadrantPos)
//{
//	std::recursive_mutex& m = m_viewer->getMainProcessingMutex();
//	std::lock_guard<std::recursive_mutex> lock(m);
//
//	if (contains(quadrantPos))
//		return (*this)[quadrantPos]->getMapImage();
//	else
//		return nullptr;
//}

void MapQuadrantImagesMap::addUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	if (not m_updatedQuadrants.contains(pos))
		m_viewer->incUpdatedMapQuadrants();
	m_updatedQuadrants[pos] = true;
	//PLOG_DEBUG << "#################### Added updated quadrant info - Pos:" << pos.getIndexPosX() << "," << pos.getIndexPosZ();
	if (m_updatingQuadrants.contains(pos))
		m_updatingQuadrants.erase(pos);
}

//TheWorld_Utils::QuadrantPos MapQuadrantImagesMap::fetchUpdatedQuadrant(void)
//{
//	TheWorld_Utils::QuadrantPos pos;
//	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
//	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
//	if (m_updatedQuadrants.size() > 0)
//	{
//		pos = m_updatedQuadrants.begin()->first;
//		m_updatedQuadrants.erase(m_updatedQuadrants.begin());
//	}
//	return pos;
//}
void MapQuadrantImagesMap::eraseUpdatedQuadrant(TheWorld_Utils::QuadrantPos& pos)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	m_updatedQuadrants.erase(pos);
	//PLOG_DEBUG << "#################### Erased updated quadrant info - Pos:" << pos.getIndexPosX() << "," << pos.getIndexPosZ();
}
void MapQuadrantImagesMap::eraseUpdatingQuadrant(TheWorld_Utils::QuadrantPos& pos)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	m_updatingQuadrants.erase(pos);
}

void MapQuadrantImagesMap::eraseByRef(TheWorld_Utils::QuadrantPos& pos)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantImage>>::erase(pos);
	eraseUpdatedQuadrant(pos);
	eraseUpdatingQuadrant(pos);
}

void MapQuadrantImagesMap::lock(void)
{
	//m_internalMutex.lock();
	m_viewer->getMainProcessingMutex().lock();
}
void MapQuadrantImagesMap::unlock(void)
{
	//#pragma warning(suppress: 26110)
				//m_internalMutex.unlock();
	m_viewer->getMainProcessingMutex().unlock();
}

//void MapQuadrantImagesMap::setMapData(godot::Ref<godot::Image> mapImage, TheWorld_Utils::QuadrantPos& quadrantPos, std::string& meshId)
//{
//	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
//	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
//
//	if (mapImage != nullptr)
//	{
//		godot::Vector2i imageSize = mapImage->get_size();
//		my_assert(imageSize.x == imageSize.y);
//		my_assert(imageSize.x == getImageNumPixelsPerSize());
//	}
//	
//	if (not contains(quadrantPos))
//		(*this)[quadrantPos] = std::make_unique<MapQuadrantImage>(quadrantPos, m_viewer, MapQuadrantImagesMap::getMapQuadrantOptimizationLevel());
//	(*this)[quadrantPos]->setMapData(mapImage);
//	(*this)[quadrantPos]->setMeshId(meshId);
//
//	addUpdatedQuadrant(quadrantPos);
//}

void MapQuadrantImagesMap::clearThrash(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	std::vector<TheWorld_Utils::QuadrantPos> toDelete;
	for (auto& item : m_updatedQuadrants)
	{
		if (not std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<MapQuadrantImage>>::contains(item.first))
		{
			toDelete.push_back(item.first);
		}
	}
	for (auto& item : toDelete)
	{
		m_updatedQuadrants.erase(item);
		//PLOG_DEBUG << "#################### Erased updated quadrant info - Pos:" << item.getIndexPosX() << "," << item.getIndexPosZ();
	}
}

bool MapQuadrantImagesMap::updated(void)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());
	return (m_updatedQuadrants.size() > 0);
}

bool MapQuadrantImagesMap::checkUpdatingQuadrants(void)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

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

bool MapQuadrantImagesMap::updateNeeded(TheWorld_Utils::QuadrantPos& pos)
{
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	if (m_updatingQuadrants.contains(pos) and m_updatingQuadrants[pos]->status() == UpdateStatus::UpdateNeeded)
		return true;
	else
		return false;
}

bool MapQuadrantImagesMap::scheduleQuadrantForUpdate(TheWorld_Utils::QuadrantPos& pos)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.3.1. MapQuadrantImagesMap::addUpdatingQuadrant") + __FUNCTION__, "ALL");

	bool scheduled = false;
	//std::lock_guard<std::recursive_mutex> lock(m_internalMutex);
	std::lock_guard<std::recursive_mutex> lock(m_viewer->getMainProcessingMutex());

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.3.1.1. MapQuadrantImagesMap::addUpdatingQuadrant") + __FUNCTION__, "Inside mutex");

		//if (pos.getIndexPosX() == 1 and pos.getIndexPosZ() == 1)	// DEBUG ONLY
		//	1;														// DEBUG ONLY

		bool updateFromServerNeeded = false;
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.3.1.1.1. MapQuadrantImagesMap::addUpdatingQuadrant") + __FUNCTION__, "Check and create MapQuadrant");

			if (not contains(pos))
			{
				(*this)[pos] = std::make_unique<MapQuadrantImage>(pos, m_viewer, MapQuadrantImagesMap::getMapQuadrantOptimizationLevel());
				updateFromServerNeeded = true;
			}
		}

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.3.1.1.2. MapQuadrantImagesMap::addUpdatingQuadrant") + __FUNCTION__, "Check and change status");

			if (not updateFromServerNeeded and (m_updatingQuadrants.contains(pos) and m_updatingQuadrants[pos]->status() == UpdateStatus::UpdateNeeded))
			{
				updateFromServerNeeded = true;
			}
		}

		if (updateFromServerNeeded)
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.3.1.1.3. MapQuadrantImagesMap::addUpdatingQuadrant") + __FUNCTION__, "m_viewer->Globals()->Client()->MapManagerGetQuadrantVertices");

			if (not m_updatingQuadrants.contains(pos))
				m_updatingQuadrants[pos] = std::make_unique<UpdatingInfo>();
			m_updatingQuadrants[pos]->status(UpdateStatus::UpdateInProgress);

			TheWorld_Utils::MeshCacheBuffer cache = (*this)[pos]->getMeshCacheBuffer();
			std::string meshId = cache.getMeshIdFromDisk(false, nullptr, false);
			//std::string meshId;
			m_viewer->Globals()->Client()->MapManagerGetQuadrantBuffer(m_viewer->isInEditor(), 0, 0, 0, 0, 0, 0, pos.getLowerXGridVertex(), pos.getLowerZGridVertex(),
				1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
				pos.getNumVerticesPerSize(), pos.getGridStepInWU(), pos.getLevel(), false, 0, meshId);

			scheduled = true;
		}
	}
	
	return scheduled;
}

#define GLOBAL_SHADER_PARAM_MAP_GRID_ON						"gu_map_grid_on"
#define SHADER_PARAM_LINE_THICKNESS							"u_line_thickness"
#define SHADER_PARAM_NUM_QUADRANTS							"u_num_quadrants"
//#define SHADER_PARAM_QUADRANT_SIZE							"u_quadrant_size"
#define SHADER_PARAM_TEXTURE_BORDER_COLOR					"u_texture_border_color"
#define SHADER_PARAM_HIGHLIGHTED							"u_highlighted"

GDN_TheWorld_Map::MapTextureRect::MapTextureRect(GDN_TheWorld_Map* map, TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	m_map = map;
	m_mapTextureRectPos = mapTextureRectPos;
	setStatus(Status::CheckFromServerNotDone);
	if (m_map != nullptr)
	{
		m_map->incMapTextureRectNum();
	}
}

GDN_TheWorld_Map::MapTextureRect::~MapTextureRect(void)
{
	if (m_map != nullptr)
		m_map->decMapTextureRectNum();
	//destroyTextureRect();
}

void GDN_TheWorld_Map::MapTextureRect::init(GDN_TheWorld_Map* map)
{
	if (map != nullptr)
		m_map = map;
	
	if (m_map != nullptr)
	{
		m_internalColorRect = memnew(MapTextureColorOverlay);
		m_map->add_child(m_internalColorRect);
		m_internalColorRect->init(this);
	}
}

godot::Color GDN_TheWorld_Map::MapTextureRect::colorFromStatus(enum class GDN_TheWorld_Map::MapTextureRect::Status status, bool& highlighted)
{
	godot::Color c;
	
	switch (status)
	{
	case GDN_TheWorld_Map::MapTextureRect::Status::CheckFromServerNotDone:
		c = GDN_TheWorld_Globals::g_color_aquamarine_green;
		highlighted = false;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::CheckFromServerRequired:
		c = GDN_TheWorld_Globals::g_color_yellow_apricot;
		highlighted = true;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::CheckFromServerInProgress:
		c = GDN_TheWorld_Globals::g_color_yellow_apricot;
		highlighted = true;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::CheckFromServerDone:
		c = GDN_TheWorld_Globals::g_color_blue;
		highlighted = true;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsNeeded:
		c = GDN_TheWorld_Globals::g_color_dark_red;
		highlighted = true;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsInProgress:
		c = GDN_TheWorld_Globals::g_color_dark_red;
		highlighted = true;
		break;
	case GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsDone:
		c = GDN_TheWorld_Globals::g_color_black;
		highlighted = true;
		break;
	default:
		c = GDN_TheWorld_Globals::g_color_white;
		highlighted = false;
		break;
	}

	return c;
}

void GDN_TheWorld_Map::MapTextureRect::detachTextureResource(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	if (m_quadrantTextureOverrideContainer.contains(quadrantPos) and m_quadrantTextureOverrideContainer[quadrantPos] != nullptr)
	{
		m_quadrantTextureOverrideContainer[quadrantPos].release();
	}
}

void GDN_TheWorld_Map::MapTextureRect::detachColorResource(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	if (m_quadrantColorOverrideContainer.contains(quadrantPos))
	{
		m_quadrantColorOverrideContainer[quadrantPos].release();
	}
}

void GDN_TheWorld_Map::MapTextureRect::saveRectImage(godot::Ref<godot::Image> rectImage, std::chrono::system_clock::time_point moreRecentQuadrantTime)
{
	godot::String imagePath = getImageCachePath();
	rectImage->save_png(imagePath);
	std::string _imagePath = imagePath.utf8().get_data();
	//if (not TheWorld_Utils::is_time_point_null(moreRecentQuadrantTime))
		TheWorld_Utils::last_write_time_point_utc(_imagePath, moreRecentQuadrantTime);
	fileTimeStrFromFile(_imagePath);
}

bool GDN_TheWorld_Map::MapTextureRect::rectFull(std::chrono::system_clock::time_point& moreRecentQuadrantTime)
{
	if (m_quadrantImages.empty())
		return false;

	moreRecentQuadrantTime = std::chrono::system_clock::time_point();

	bool rectFull = true;

	for (int y = 0; y < m_mapTextureRectPos.getNumQuadrantsPerSize(); y++)
	{
		for (int x = 0; x < m_mapTextureRectPos.getNumQuadrantsPerSize(); x++)
			if (m_quadrantImages[y][x].image() == nullptr)
			{
				rectFull = false;
				break;
			}
			else
			{
				if (m_quadrantImages[y][x].quadrantTime() > moreRecentQuadrantTime)
					moreRecentQuadrantTime = m_quadrantImages[y][x].quadrantTime();
			}
		if (not rectFull)
			break;
	}

	return rectFull;
}

bool GDN_TheWorld_Map::MapTextureRect::destroyInternalTextureRect(void)
{
	if (m_internalTextureRect != nullptr)
	{
		for (auto& item : m_quadrantColorOverrideContainer)
		{
			if (item.second != nullptr)
				item.second->freeResource();
			item.second.release();
		}
		m_quadrantColorOverrideContainer.clear();

		for (auto& item : m_quadrantTextureOverrideContainer)
		{
			if (item.second != nullptr)
				item.second->freeResource();
			item.second.release();
		}
		m_quadrantTextureOverrideContainer.clear();

		godot::Node* parent = m_internalTextureRect->get_parent();
		if (parent != nullptr)
			parent->call_deferred("remove_child", m_internalTextureRect);
		m_internalTextureRect->queue_free();
		m_internalTextureRect = nullptr;
	}

	return true;
}

bool GDN_TheWorld_Map::MapTextureRect::isTextureRectRendered(void)
{
	return getInternalTextureRect() != nullptr;
}

godot::TextureRect* GDN_TheWorld_Map::MapTextureRect::getInternalTextureRect(void)
{
	return m_internalTextureRect;
}

void GDN_TheWorld_Map::MapTextureRect::renderImage(godot::Ref<godot::Image> image)
{
	destroyInternalTextureRect();

	godot::Vector2i rectImageSize = image->get_size();

	m_internalTextureRect = memnew(godot::TextureRect);
	m_map->add_child(m_internalTextureRect);
	m_internalTextureRect->set_name(m_mapTextureRectPos.getName().c_str());
	m_internalTextureRect->add_to_group("MapTextureRect");
	m_internalTextureRect->set_size(rectImageSize);
	godot::Vector2 pos(m_mapTextureRectPos.getGlobalLowerPosX(), m_mapTextureRectPos.getGlobalLowerPosY());
	m_internalTextureRect->set_position(pos);

	godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
	m_internalTextureRect->set_texture(tex);

	std::vector<TheWorld_Utils::QuadrantPos> quadrantsPos;
	m_mapTextureRectPos.getAllContainedQuadrantPos(quadrantsPos, m_map->getMapViewer()->getViewer()->getLevel(), (int)m_map->getMapViewer()->getViewer()->Globals()->numVerticesPerQuadrantSize(), m_map->getMapViewer()->getViewer()->Globals()->gridStepInWU());
	for (auto& item : quadrantsPos)
	{
		MapQuadrantOverride* mapQuadrantOverride = m_map->getMapQuadrantOverride(item);
		if (mapQuadrantOverride != nullptr)
		{
			if (mapQuadrantOverride->quadrantActuallyInWorld())
			{
				MapQuadrantColorOverride* mapQuadrantColorOverride = getQuadrantColorOverride(item, true);
				if (mapQuadrantColorOverride != nullptr)
					mapQuadrantColorOverride->apply(mapQuadrantOverride);
			}
			if (mapQuadrantOverride->overrideTerrainTypeSet())
			{
				MapQuadrantTextureOverride* mapQuadrantTextureOverride = getQuadrantTextureOverride(item, true);
				if (mapQuadrantTextureOverride != nullptr)
					mapQuadrantTextureOverride->apply(mapQuadrantOverride);
			}
		}
	}
}

GDN_TheWorld_Map::MapTextureRect::Status GDN_TheWorld_Map::MapTextureRect::getStatus(void)
{
	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
	long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
	if (m_status == Status::CheckFromServerInProgress and elapsedFromStatusChange > 60000)
		//m_status = Status::CheckFromServerRequired;
		setStatus(Status::CheckFromServerRequired);
	return m_status;
}
void GDN_TheWorld_Map::MapTextureRect::setStatus(GDN_TheWorld_Map::MapTextureRect::Status status)
{
	if (m_status != status)
	{
		TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
		m_lastStatusChange = now;
		m_status = status;
		if (m_internalColorRect != nullptr)
		{
			m_internalColorRect->call_deferred("rect_status_changed");
		}
	}
}

void GDN_TheWorld_Map::MapTextureRect::setInternalImageData(TheWorld_Utils::MemoryBuffer& imageBuffer, std::chrono::system_clock::time_point imageTime)
{
	m_internalImageData.resize((int64_t)imageBuffer.size());
	{
		memcpy((char*)m_internalImageData.ptrw(), imageBuffer .ptr(), imageBuffer.size());
	}
	m_imageTime = imageTime;
	m_map->getMapViewer()->getViewer()->forceProcessNewMapQuadrants(true);
}

//#define SHADER_PARAM_MAP_RECT_COLOR		"u_color"
//#define SHADER_PARAM_MAP_RECT_ALPHA		"u_alpha"
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_NOPE			-1;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CAMERA		0;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_INITIAL		1;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_GROWING		2;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CACHE		3;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_BLEND_NEEDED	4;
#define SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT		"u_world_quadrant"
#define SHADER_PARAM_QUADRANT_COLOR_QUADRANT_READY		"u_quadrant_ready"
#define SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT		"u_far_quadrant"
#define SHADER_PARAM_QUADRANT_COLOR_EDIT_MODE			"u_edit_mode"
#define SHADER_PARAM_QUADRANT_HIGHLIGHTED				"u_highlighted"

GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride* GDN_TheWorld_Map::MapTextureRect::getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing)
{
	std::lock_guard<std::recursive_mutex> lockM(m_map->getMapViewer()->getViewer()->getMainProcessingMutex());
	
	MapQuadrantColorOverride* rect = nullptr;

	if (m_internalTextureRect == nullptr)
		return rect;

	
	if (not m_quadrantColorOverrideContainer.contains(quadrantPos) or m_quadrantColorOverrideContainer[quadrantPos] == nullptr)
	{
		if (createIfMissing)
		{
			godot::Vector2 rectSize(m_map->getQuadrantGlobalSize(), m_map->getQuadrantGlobalSize());
			godot::Vector2 globalQuadrantPos = m_map->worldPosToGlobalPos(godot::Vector2(quadrantPos.getLowerXGridVertex(), quadrantPos.getLowerZGridVertex()));

			godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
			godot::String shaderPath = "res://addons/twviewer/shaders/MapQuadrantColor.gdshader";
			godot::Ref<godot::Shader> shader = resLoader->load(shaderPath);
			godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
			mat->set_shader(shader);
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_NOPE;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);

			MapQuadrantColorOverride* colorRect = memnew(MapQuadrantColorOverride);
			m_internalTextureRect->add_child(colorRect);
			colorRect->init(this, quadrantPos);
			colorRect->set_name(("Color_" + m_mapTextureRectPos.getName()).c_str());
			colorRect->add_to_group("MapQuadrantColorRect");
			colorRect->set_material(mat);
			colorRect->set_size(rectSize);
			colorRect->set_global_position(globalQuadrantPos);
			colorRect->set_z_index(GDN_TheWorld_Map::c_map_z_index_quadrant_color_override);
			colorRect->set_visible(m_map->mapGridOn());

			if (not m_quadrantColorOverrideContainer.contains(quadrantPos))
				m_quadrantColorOverrideContainer.insert({ quadrantPos , std::unique_ptr<MapQuadrantColorOverride>(colorRect) });
			else
				m_quadrantColorOverrideContainer[quadrantPos] = std::unique_ptr<MapQuadrantColorOverride>(colorRect);

			rect = m_quadrantColorOverrideContainer[quadrantPos].get();
		}
	}
	else
		rect = m_quadrantColorOverrideContainer[quadrantPos].get();

	return rect;
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride* GDN_TheWorld_Map::MapTextureRect::getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing)
{
	std::lock_guard<std::recursive_mutex> lockM(m_map->getMapViewer()->getViewer()->getMainProcessingMutex());

	GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride* rect = nullptr;

	if (m_internalTextureRect == nullptr)
		return rect;

	if (not m_quadrantTextureOverrideContainer.contains(quadrantPos) or m_quadrantTextureOverrideContainer[quadrantPos] == nullptr)
	{
		if (createIfMissing)
		{
			godot::Vector2 rectSize(m_map->getQuadrantGlobalSize(), m_map->getQuadrantGlobalSize());
			godot::Vector2 globalQuadrantPos = m_map->worldPosToGlobalPos(godot::Vector2(quadrantPos.getLowerXGridVertex(), quadrantPos.getLowerZGridVertex()));

			MapQuadrantTextureOverride* textureRect = memnew(MapQuadrantTextureOverride);
			m_internalTextureRect->add_child(textureRect);
			textureRect->init(this, quadrantPos);
			textureRect->set_name(("Texture_" + m_mapTextureRectPos.getName()).c_str());
			textureRect->add_to_group("MapQuadrantTextureRect");
			textureRect->set_size(rectSize);
			textureRect->set_global_position(globalQuadrantPos);
			textureRect->set_z_index(GDN_TheWorld_Map::c_map_z_index_quadrant_texture_override);
			textureRect->set_visible(m_map->mapGridOn());
			textureRect->set_expand_mode(godot::TextureRect::EXPAND_IGNORE_SIZE);
			textureRect->set_stretch_mode(godot::TextureRect::StretchMode::STRETCH_SCALE);

			if (m_quadrantTextureOverrideContainer.contains(quadrantPos))	// is nullptr
				m_quadrantTextureOverrideContainer[quadrantPos] = std::unique_ptr<MapQuadrantTextureOverride>(textureRect);
			else
				m_quadrantTextureOverrideContainer[quadrantPos] = std::unique_ptr<MapQuadrantTextureOverride>(textureRect);
				//m_quadrantTextureOverrideContainer.insert({ quadrantPos , std::unique_ptr<MapQuadrantTextureOverride>(textureRect) });

			rect = m_quadrantTextureOverrideContainer[quadrantPos].get();
		}
	}
	else
		rect = m_quadrantTextureOverrideContainer[quadrantPos].get();
	
	return rect;
}

void GDN_TheWorld_Map::MapTextureRect::releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_map->getMapViewer()->getViewer()->getMainProcessingMutex());

	if (m_quadrantColorOverrideContainer.contains(quadrantPos) and m_quadrantColorOverrideContainer[quadrantPos] != nullptr)
	{
		m_quadrantColorOverrideContainer[quadrantPos]->freeResource();
		m_quadrantColorOverrideContainer[quadrantPos]->queue_free();
		m_quadrantColorOverrideContainer[quadrantPos].release();
		m_quadrantColorOverrideContainer.erase(quadrantPos);
	}
}

void GDN_TheWorld_Map::MapTextureRect::releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_map->getMapViewer()->getViewer()->getMainProcessingMutex());

	if (m_quadrantTextureOverrideContainer.contains(quadrantPos) and m_quadrantTextureOverrideContainer[quadrantPos] != nullptr)
	{
		m_quadrantTextureOverrideContainer[quadrantPos]->freeResource();
		m_quadrantTextureOverrideContainer[quadrantPos]->queue_free();
		m_quadrantTextureOverrideContainer[quadrantPos].release();
		m_quadrantTextureOverrideContainer.erase(quadrantPos);
	}
}

godot::String GDN_TheWorld_Map::MapTextureRect::getImageCachePath(void)
{
	godot::String cacheDir = m_map->getCacheDir() + "\\MapRects\\";
	godot::String imagePath = cacheDir + m_mapTextureRectPos.getName().c_str() + ".png";
	std::string _cacheDir = cacheDir.utf8().get_data();
	if (!fs::exists(_cacheDir))
	{
		fs::create_directories(_cacheDir);
	}
	return imagePath;
}

bool GDN_TheWorld_Map::MapTextureRect::eraseCachedTexture(void)
{
	godot::String imagePath = getImageCachePath();
	std::string _imagePath = imagePath.utf8().get_data();
	if (!fs::exists(_imagePath))
		return false;

	fs::remove(_imagePath);
	return true;
}

bool GDN_TheWorld_Map::MapTextureRect::renderCachedTexture(void)
{
	godot::String imagePath = getImageCachePath();
	std::string _imagePath = imagePath.utf8().get_data();
	if (!fs::exists(_imagePath))
		return false;
	godot::Ref<godot::Image> rectImage = godot::Image::load_from_file(imagePath);
	if (rectImage == nullptr)
		return false;
	else
	{
		renderImage(rectImage);
		fileTimeStrFromFile(_imagePath);
		return true;
	}
}

bool GDN_TheWorld_Map::MapTextureRect::renderInternalTexture(void)
{
	bool rendered = false;
	
	if (not internalImageDataValid())
		return rendered;
	
	godot::Ref<godot::Image> rectImage = godot::Image::create_from_data(int32_t(m_mapTextureRectPos.getGlobalSize()), int32_t(m_mapTextureRectPos.getGlobalSize()), false, godot::Image::Format::FORMAT_RGBA8, m_internalImageData);
	saveRectImage(rectImage, m_imageTime);
	renderImage(rectImage);

	m_internalImageData.clear();
	m_imageTime = std::chrono::system_clock::time_point();
	assert(!internalImageDataValid());	// DEBUG

	rendered = true;

	return rendered;
}

bool GDN_TheWorld_Map::MapTextureRect::setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos, std::chrono::system_clock::time_point quadrantTime)
{
	bool fullRectRendered = false;
	
	if (mapImage == nullptr)
		return fullRectRendered;

	if (not m_mapTextureRectPos.initialized())
		return fullRectRendered;

	if (m_quadrantImages.empty())
	{
		QuadrantImage emptyQuadrantimage(nullptr, std::chrono::system_clock::time_point());
		
		// pre init of images vector
		for (int y = 0; y < m_mapTextureRectPos.getNumQuadrantsPerSize(); y++)
		{
			std::vector<QuadrantImage> v;
			m_quadrantImages.push_back(v);
			for (int x = 0; x < m_mapTextureRectPos.getNumQuadrantsPerSize(); x++)
				m_quadrantImages[y].push_back(emptyQuadrantimage);
		}
	}

	QuadrantImage quadrantImage(mapImage, quadrantTime);
	
	int32_t idxX, idxY;
	bool quadrantIsContainedInRect = m_mapTextureRectPos.getQuadrantRelativeIndices(quadrantPos, idxX, idxY);
	if (quadrantIsContainedInRect)
	{
		m_quadrantImages[idxY][idxX] = quadrantImage;
	}

	std::chrono::system_clock::time_point moreRecentQuadrantTime;
	if (rectFull(moreRecentQuadrantTime))
	{
		godot::Ref<godot::Image> rectImage = godot::Image::create_empty(int32_t(m_mapTextureRectPos.getNumPixelsPerSize()), int32_t(m_mapTextureRectPos.getNumPixelsPerSize()), false, godot::Image::Format::FORMAT_RGB8);

		for (int32_t y = 0; y < m_mapTextureRectPos.getNumQuadrantsPerSize(); y++)
			for (int32_t x = 0; x < m_mapTextureRectPos.getNumQuadrantsPerSize(); x++)
			{
				QuadrantImage quadrantImage = m_quadrantImages[y][x];
				godot::Rect2i quadrantImageRect(0, 0, quadrantImage.image()->get_size().x, quadrantImage.image()->get_size().y);
				godot::Vector2i pos(x * int32_t(m_mapTextureRectPos.getQuadrantGlobalSize()), y * int32_t(m_mapTextureRectPos.getQuadrantGlobalSize()));
				rectImage->blit_rect(quadrantImage.image(), quadrantImageRect, pos);
			}

		m_quadrantImages.clear();

		saveRectImage(rectImage, moreRecentQuadrantTime);
		
		std::string _fileTimeStr = fileTimeStr();
		m_map->selectTextureRectForUpload(m_mapTextureRectPos, _fileTimeStr, rectImage);

		renderImage(rectImage);

		fullRectRendered = true;
	}

	return fullRectRendered;
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::MapQuadrantTextureOverride(void)
{
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::~MapQuadrantTextureOverride(void)
{
	if (m_mapTextureRect != nullptr and m_quadrantPos.isInitialized())
	{
		m_mapTextureRect->detachTextureResource(m_quadrantPos);
	}
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->decMapQuadrantTextureOverrideNum();
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::init(MapTextureRect* mapTextureRect, TheWorld_Utils::QuadrantPos& quadrantPos)
{
	m_mapTextureRect = mapTextureRect;
	m_quadrantPos = quadrantPos;
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->incMapQuadrantTextureOverrideNum();
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::freeResource(void)
{
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->decMapQuadrantTextureOverrideNum();
	m_mapTextureRect = nullptr;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::apply(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride)
{
	godot::Ref<godot::Texture2D> tex = nullptr;
	
	if (not mapQuadrantOverride->overrideTerrainTypeNull() or mapQuadrantOverride->overrideTerrainTypeUnknown())
	{
		GDN_TheWorld_Edit* editModeUIControl = m_mapTextureRect->getMap()->getMapViewer()->getViewer()->EditModeUIControl();
		if (editModeUIControl != nullptr and editModeUIControl->initialized())
		{
			tex = editModeUIControl->getTextureByTerrainType(mapQuadrantOverride->getTerrainType());
			//tex = godot::ResourceLoader::get_singleton()->load("res://icon.svg");
		}
	}

	set_texture(tex);

	if (canBeErased(mapQuadrantOverride))
		queue_free();
}

bool GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::canBeErased(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride)
{
	if (get_texture() == nullptr)
		return true;
	else
		return false;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::applyOverride()
{
	if (m_mapTextureRect == nullptr)
		return;

	GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride = m_mapTextureRect->getMap()->getMapQuadrantOverride(m_quadrantPos);
	if (mapQuadrantOverride != nullptr)
		apply(mapQuadrantOverride);;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("apply_override"), &GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::applyOverride);
}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_notification(int p_what)
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

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_ready(void)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_process(double _delta)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_input(const Ref<InputEvent>& event)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride::_unhandled_input(const Ref<InputEvent>& event)
//{
//}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::MapQuadrantColorOverride(void)
{
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::~MapQuadrantColorOverride(void)
{
	if (m_mapTextureRect != nullptr and m_quadrantPos.isInitialized())
	{
		m_mapTextureRect->detachColorResource(m_quadrantPos);
	}
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->decMapQuadrantColorOverrideNum();
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::init(MapTextureRect* mapTextureRect, TheWorld_Utils::QuadrantPos& quadrantPos)
{
	m_mapTextureRect = mapTextureRect;
	m_quadrantPos = quadrantPos;
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->incMapQuadrantColorOverrideNum();
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::freeResource(void)
{
	if (m_mapTextureRect != nullptr)
		m_mapTextureRect->getMap()->decMapQuadrantColorOverrideNum();
	m_mapTextureRect = nullptr;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::apply(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride)
{
	godot::Ref<godot::ShaderMaterial> mat = get_material();
	if (mat != nullptr)
	{
		enum class QuadTree::QuadrantDepth quadrantDepth = mapQuadrantOverride->quadrantDepth();
		int numQuadrantsFromCameraQuadrant = mapQuadrantOverride->numQuadrantsFromCameraQuadrant();
		bool blendNeeded = mapQuadrantOverride->blendNeeded();
		if (quadrantDepth == QuadTree::QuadrantDepth::NotSet)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_NOPE;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, -1);
		}
		else if (blendNeeded)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_BLEND_NEEDED;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
		}
		else if (numQuadrantsFromCameraQuadrant == 0)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CAMERA;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
		}
		else if (quadrantDepth == QuadTree::QuadrantDepth::Initial)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_INITIAL;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
		}
		else if (quadrantDepth == QuadTree::QuadrantDepth::Growing)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_GROWING;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
		}
		else if (quadrantDepth == QuadTree::QuadrantDepth::Cache)
		{
			const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CACHE;
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
			mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
		}

		bool quadrantReady = mapQuadrantOverride->isQuadrantReady();
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_QUADRANT_READY, quadrantReady);
		bool editMode = mapQuadrantOverride->editMode();
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_EDIT_MODE, editMode);

		bool refreshInProgress = mapQuadrantOverride->refreshInProgress();
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_HIGHLIGHTED, refreshInProgress);
	}

	if (canBeErased(mapQuadrantOverride))
		queue_free();
}

bool GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::canBeErased(GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride)
{
	if (not mapQuadrantOverride->quadrantActuallyInWorld() and not mapQuadrantOverride->editMode() and not mapQuadrantOverride->refreshInProgress())
		return true;
	else
		return false;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::applyOverride()
{
	if (m_mapTextureRect == nullptr)
		return;

	GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride = m_mapTextureRect->getMap()->getMapQuadrantOverride(m_quadrantPos);
	if (mapQuadrantOverride != nullptr)
		apply(mapQuadrantOverride);;
}

void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("apply_override"), &GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::applyOverride);
}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_notification(int p_what)
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

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_ready(void)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_process(double _delta)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_input(const Ref<InputEvent>& event)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride::_unhandled_input(const Ref<InputEvent>& event)
//{
//}

GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::MapTextureColorOverlay(void)
{
}

GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::~MapTextureColorOverlay(void)
{
}

void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::init(MapTextureRect* mapTextureRect)
{
	m_mapTextureRect = mapTextureRect;

	godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
	godot::String shaderPath = "res://addons/twviewer/shaders/MapTextureRect.gdshader";
	godot::Ref<godot::Shader> shader = resLoader->load(shaderPath);
	godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
	mat->set_shader(shader);
	mat->set_shader_parameter(SHADER_PARAM_NUM_QUADRANTS, GDN_TheWorld_Map::numImagesForTextureRectSide());
	mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_mapTextureRect->getMap()->getQuadrantGlobalSize());

	godot::Vector2i rectSize = m_mapTextureRect->getMap()->getTextureRectSize();
	set_name((std::string("Color_") + m_mapTextureRect->pos().getName()).c_str());
	add_to_group("MapColorRect");
	set_z_index(GDN_TheWorld_Map::c_map_z_index_rect_color_overlay);
	set_size(rectSize);
	godot::Vector2 pos(m_mapTextureRect->pos().getGlobalLowerPosX(), m_mapTextureRect->pos().getGlobalLowerPosY());
	set_position(pos);
	set_material(mat);
	call_deferred("rect_status_changed");
}

void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::rectStatusChanged(void)
{
	godot::Ref<godot::ShaderMaterial> mat = get_material();
	bool highlighted = false;
	mat->set_shader_parameter(SHADER_PARAM_TEXTURE_BORDER_COLOR, GDN_TheWorld_Map::MapTextureRect::colorFromStatus(m_mapTextureRect->getStatus(), highlighted));
	mat->set_shader_parameter(SHADER_PARAM_HIGHLIGHTED, highlighted);
}

void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("rect_status_changed"), &GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::rectStatusChanged);
}

//void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_notification(int p_what)
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

//void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_ready(void)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_process(double _delta)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_input(const Ref<InputEvent>& event)
//{
//}

//void GDN_TheWorld_Map::MapTextureRect::MapTextureColorOverlay::_unhandled_input(const Ref<InputEvent>& event)
//{
//}

GDN_TheWorld_Map::MapQuadrantOverride::MapQuadrantOverride(GDN_TheWorld_Map* map, TheWorld_Utils::QuadrantPos& quadrantPos)
{
	m_map = map;
	m_quadrantPos = quadrantPos;
	internalSetName();
	m_terrainEdit = TheWorld_Utils::TerrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);
}

GDN_TheWorld_Map::MapQuadrantOverride::~MapQuadrantOverride(void)
{
	m_map->releaseQuadrantColorOverride(m_quadrantPos);
	m_map->releaseQuadrantTextureOverride(m_quadrantPos);
}

void GDN_TheWorld_Map::MapQuadrantOverride::setVisible(bool b)
{
	godot::TextureRect* textureOverride = m_map->getQuadrantTextureOverride(m_quadrantPos);
	if (textureOverride != nullptr)
		textureOverride->set_visible(b);
	godot::ColorRect* colorOverride = m_map->getQuadrantColorOverride(m_quadrantPos);
	if (colorOverride != nullptr)
		colorOverride->set_visible(b);
}

void GDN_TheWorld_Map::MapQuadrantOverride::quadrantDepth(enum class QuadTree::QuadrantDepth quadrantDepth, int numQuadrantsFromCameraQuadrant)
{
	m_quadrantDepth = quadrantDepth;
	m_numQuadrantsFromCameraQuadrant = numQuadrantsFromCameraQuadrant;
	
	//godot::ColorRect* colorOverride = m_map->getQuadrantColorOverride(m_quadrantPos, true);
	//if (colorOverride == nullptr)
	//	return;

	//godot::Ref<godot::ShaderMaterial> mat = colorOverride->get_material();

	//if (mat != nullptr)
	//{
	//	if (quadrantDepth == QuadTree::QuadrantDepth::NotSet)
	//	{
	//		const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_NOPE;
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, -1);
	//	}
	//	else if (numQuadrantsFromCameraQuadrant == 0)
	//	{
	//		const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CAMERA;
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
	//	}
	//	else if (quadrantDepth == QuadTree::QuadrantDepth::Initial)
	//	{
	//		const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_INITIAL;
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
	//	}
	//	else if (quadrantDepth == QuadTree::QuadrantDepth::Growing)
	//	{
	//		const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_GROWING;
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
	//	}
	//	else if (quadrantDepth == QuadTree::QuadrantDepth::Cache)
	//	{
	//		const godot::Variant v = SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT_CACHE;
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_WORLD_QUADRANT, v);
	//		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_FAR_QUADRANT, numQuadrantsFromCameraQuadrant);
	//	}
	//}
}

void GDN_TheWorld_Map::MapQuadrantOverride::quadrantReady(void)
{
	m_quadrantReady = true;
	
	//godot::ColorRect* colorOverride = m_map->getQuadrantColorOverride(m_quadrantPos, true);
	//if (colorOverride == nullptr)
	//	return;

	//godot::Ref<godot::ShaderMaterial> mat = colorOverride->get_material();

	//if (mat != nullptr)
	//{
	//	mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_QUADRANT_READY, true);
	//}
}

void GDN_TheWorld_Map::MapQuadrantOverride::quadrantWorkInProgress(void)
{
	m_quadrantReady = false;

	//godot::ColorRect* colorOverride = m_map->getQuadrantColorOverride(m_quadrantPos, true);
	//if (colorOverride == nullptr)
	//	return;

	//godot::Ref<godot::ShaderMaterial> mat = colorOverride->get_material();

	//if (mat != nullptr)
	//{
	//	mat->set_shader_parameter(SHADER_PARAM_QUADRANT_COLOR_QUADRANT_READY, false);
	//}
}

void GDN_TheWorld_Map::MapQuadrantOverride::applyColor(void)
{
	GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride* colorOverride = m_map->getQuadrantColorOverride(m_quadrantPos, true);
	if (colorOverride == nullptr)
		return;
	//colorOverride->apply(this);
	colorOverride->call_deferred("apply_override");
}

void GDN_TheWorld_Map::MapQuadrantOverride::applyTexture(void)
{
	GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride* textureOverride = m_map->getQuadrantTextureOverride(m_quadrantPos, true);
	if (textureOverride == nullptr)
		return;
	//textureOverride->apply(this);
	textureOverride->call_deferred("apply_override");
}

void GDN_TheWorld_Map::MapQuadrantOverride::internalSetName(void)
{
	constexpr size_t NUM_PADDING_ZEROES = 12;
	std::string strPosX, strPosZ, strLevel;
	int32_t posX = int32_t(m_quadrantPos.getLowerXGridVertex()), posZ = int32_t(m_quadrantPos.getLowerZGridVertex()), level = int32_t(m_quadrantPos.getLevel());
	if (posX < 0)
		strPosX = "-" + TheWorld_Utils::padIntValueTo(-posX, NUM_PADDING_ZEROES);
	else
		strPosX = TheWorld_Utils::padIntValueTo(posX, NUM_PADDING_ZEROES);
	if (posZ < 0)
		strPosZ = "-" + TheWorld_Utils::padIntValueTo(-posZ, NUM_PADDING_ZEROES);
	else
		strPosZ = TheWorld_Utils::padIntValueTo(posZ, NUM_PADDING_ZEROES);
	if (level < 0)
		strLevel = "-" + TheWorld_Utils::padIntValueTo(-level, 2);
	else
		strLevel = TheWorld_Utils::padIntValueTo(level, 2);
	m_name = "MapQuadrant_" + strLevel + "_" + strPosX + "_" + strPosZ;
}


//void GDN_TheWorld_Map::MapQuadrantOverride::render(void)
//{
//
//}

void GDN_TheWorld_Map::_bind_methods(void)
{
	ClassDB::bind_method(D_METHOD("viewport_size_changed"), &GDN_TheWorld_Map::viewportSizeChanged);
}

GDN_TheWorld_Map::GDN_TheWorld_Map(void)
{
}

GDN_TheWorld_Map::~GDN_TheWorld_Map(void)
{
	m_destroying = true;

	if (m_mapViewport != nullptr)
	{
		if (m_mapViewport->is_connected("size_changed", Callable(this, "viewport_size_changed")))
			m_mapViewport->disconnect("size_changed", Callable(this, "viewport_size_changed"));
	}
}

int32_t GDN_TheWorld_Map::numImagesForTexture(void)
{
	return s_numImagesForTexture;
}

void GDN_TheWorld_Map::init(GDN_TheWorld_MapViewer* mapViewer, godot::Vector2 containerSize)
{
	m_mapViewer = mapViewer;

	GDN_TheWorld_MapViewportContainer* mapViewportContainer = m_mapViewer->getMapViewportContainer();
	m_mapViewport = mapViewportContainer->getMapViewport();

	godot::Error e = godot::Error::OK;
	if (not m_mapViewport->is_connected("size_changed", Callable(this, "viewport_size_changed")))
	{
		e = m_mapViewport->connect("size_changed", Callable(this, "viewport_size_changed"));
	}

	constexpr int FONT_SIZE_MULTIPLIER = 2;
	
	SAFE_CREATE_NODE(m_northLabel, godot::Label);
	//m_northLabel = memnew(godot::Label);
	mapViewportContainer->add_child(m_northLabel);
	m_northLabel->set_name("MapNorthLabel");
	m_northLabel->set_text("N");
	m_northLabel->set_modulate(GDN_TheWorld_Globals::g_color_red);
	int32_t sz = m_northLabel->get_theme_font_size("font_size");
	m_northLabel->add_theme_font_size_override("font_size", sz * FONT_SIZE_MULTIPLIER);
	
	SAFE_CREATE_NODE(m_southLabel, godot::Label);
	//m_southLabel = memnew(godot::Label);
	mapViewportContainer->add_child(m_southLabel);
	m_southLabel->set_name("MapSouthLabel");
	m_southLabel->set_text("S");
	m_southLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
	sz = m_southLabel->get_theme_font_size("font_size");
	m_southLabel->add_theme_font_size_override("font_size", sz * FONT_SIZE_MULTIPLIER);
	
	SAFE_CREATE_NODE(m_eastLabel, godot::Label);
	//m_eastLabel = memnew(godot::Label);
	mapViewportContainer->add_child(m_eastLabel);
	m_eastLabel->set_name("MapEastLabel");
	m_eastLabel->set_text("E");
	m_eastLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
	sz = m_eastLabel->get_theme_font_size("font_size");
	m_eastLabel->add_theme_font_size_override("font_size", sz * FONT_SIZE_MULTIPLIER);
	
	SAFE_CREATE_NODE(m_westLabel, godot::Label);
	//m_westLabel = memnew(godot::Label);
	mapViewportContainer->add_child(m_westLabel);
	m_westLabel->set_name("MapWestLabel");
	m_westLabel->set_text("W");
	m_westLabel->set_modulate(GDN_TheWorld_Globals::g_color_black);
	sz = m_westLabel->get_theme_font_size("font_size");
	m_westLabel->add_theme_font_size_override("font_size", sz * FONT_SIZE_MULTIPLIER);

	SAFE_CREATE_NODE(m_compassImage, godot::TextureRect);
	//m_compassImage = memnew(godot::TextureRect);
	mapViewportContainer->add_child(m_compassImage);
	m_compassImage->set_name("MapCompassImage");
	godot::Ref<godot::Texture2D> compassImage = godot::ResourceLoader::get_singleton()->load("res://addons/twviewer/assets/compass_image.png");
	//godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(compassImage);

	m_compassImage->set_texture(compassImage);

	const float MapRulerHLineSize = getQuadrantGlobalSize();
	const float MapRulerVLineSize = 10;
	const double MapRulerLineWidth = 3;
	const godot::Color MapRulerColor(GDN_TheWorld_Globals::g_color_red);
	SAFE_CREATE_NODE(m_rulerGroup, godot::VBoxContainer);
	//m_rulerGroup = memnew(godot::VBoxContainer);
	mapViewportContainer->add_child(m_rulerGroup);
	m_rulerGroup->set_name("MapRulerGroup");

	godot::Label* rulerLabel = memnew(godot::Label);
	m_rulerGroup->add_child(rulerLabel);
	rulerLabel->set_name("MapRulerLabel");
	rulerLabel->set_modulate(MapRulerColor);
	//rulerLabel->set_horizontal_alignment(godot::HorizontalAlignment::HORIZONTAL_ALIGNMENT_CENTER);
	
	godot::Container* ruler = memnew(godot::Container);
	m_rulerGroup->add_child(ruler);
	ruler->set_name("MapRuler");
	godot::Line2D* rulerHorizontalLine = memnew(godot::Line2D);
	ruler->add_child(rulerHorizontalLine);
	rulerHorizontalLine->set_name("MapRulerHLine");
	rulerHorizontalLine->set_default_color(MapRulerColor);
	rulerHorizontalLine->set_width(MapRulerLineWidth);
	godot::Line2D* rulerVerticalLine1 = memnew(godot::Line2D);
	ruler->add_child(rulerVerticalLine1);
	rulerVerticalLine1->set_name("MapRulerVLine1");
	rulerVerticalLine1->set_default_color(MapRulerColor);
	rulerVerticalLine1->set_width(MapRulerLineWidth);
	godot::Line2D* rulerVerticalLine2 = memnew(godot::Line2D);
	ruler->add_child(rulerVerticalLine2);
	rulerVerticalLine2->set_name("MapRulerVLine2");
	rulerVerticalLine2->set_default_color(MapRulerColor);
	rulerVerticalLine2->set_width(MapRulerLineWidth);

	mapGridOn(false);
}

void GDN_TheWorld_Map::enterMapMode(void)
{
	resetDragInputEvents();
	
	for (auto& textureRectItem : m_mapTextureRectContainer)
	{
		if (textureRectItem.second->getStatus() == MapTextureRect::Status::AcqQuadrantsDone)
		{
			textureRectItem.second->setStatus(MapTextureRect::Status::CheckFromServerNotDone);
		}
	}
}

void GDN_TheWorld_Map::exitMapMode(void)
{
	resetDragInputEvents();
}

void GDN_TheWorld_Map::setMapGridLineThickness(float multiplier)
{
}

void GDN_TheWorld_Map::mapGridOn(bool b)
{
	m_mapGridOn = b;

	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_MAP_GRID_ON, b);
	
	for (auto& item : m_mapQuadrantOverrideContainer)
	{
		item.second->setVisible(m_mapGridOn);
	}

	resetDragInputEvents();
}

void GDN_TheWorld_Map::viewportSizeChanged(void)
{
	if (m_mapViewport != nullptr)
	{
		godot::Vector2 newSize = m_mapViewport->get_size();
		viewportResized(newSize);
	}
}

void GDN_TheWorld_Map::viewportResized(godot::Vector2 newSize)
{
	if (m_northLabel != nullptr)
	{
		m_northLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		m_northLabel->set_offset(SIDE_LEFT, newSize.x / 2);
		m_northLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_northLabel->set_offset(SIDE_TOP, 0);
	}
	if (m_southLabel != nullptr)
	{
		godot::Rect2 rect = m_southLabel->get_rect();
		godot::Size2 size = rect.size;
		m_southLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		m_southLabel->set_offset(SIDE_LEFT, newSize.x / 2);
		m_southLabel->set_anchor(godot::Side::SIDE_TOP, 1.0);
		m_southLabel->set_offset(SIDE_TOP, -size.y);
	}
	if (m_eastLabel != nullptr)
	{
		godot::Rect2 rect = m_eastLabel->get_rect();
		godot::Size2 size = rect.size;
		m_eastLabel->set_anchor(godot::Side::SIDE_LEFT, 1.0);
		m_eastLabel->set_offset(SIDE_LEFT, -size.x);
		m_eastLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_eastLabel->set_offset(SIDE_TOP, newSize.y / 2);
	}
	if (m_westLabel != nullptr)
	{
		m_westLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		m_westLabel->set_offset(SIDE_LEFT, 0.0);
		m_westLabel->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_westLabel->set_offset(SIDE_TOP, newSize.y / 2);
	}

	if (m_compassImage != nullptr)
	{
		//m_compassImage->set_anchor(godot::Side::SIDE_LEFT, 0.01);
		//m_compassImage->set_offset(SIDE_LEFT, 0.0);
		//m_compassImage->set_anchor(godot::Side::SIDE_TOP, 0.03);
		//m_compassImage->set_offset(SIDE_TOP, 0.0);
		m_compassImage->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		m_compassImage->set_offset(SIDE_LEFT, 25.0);
		m_compassImage->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_compassImage->set_offset(SIDE_TOP, 25.0);
	}

	resizeRuler(newSize);
	
	getMapViewer()->getViewer()->forceProcessNewMapQuadrants(true);
}

void GDN_TheWorld_Map::resizeRuler(void)
{
	if (m_mapViewport != nullptr)
	{
		godot::Vector2 newSize = m_mapViewport->get_size();
		resizeRuler(newSize);
	}
}

void GDN_TheWorld_Map::resizeRuler(godot::Vector2 newSize)
{
	if (m_rulerGroup != nullptr)
	{
		godot::Node* n = nullptr;

		Camera2D* mapCamera = getMapViewer()->getViewer()->MapCamera();
		GDN_TheWorld_MapCamera* cam = godot::Object::cast_to<GDN_TheWorld_MapCamera>(mapCamera);

		float zoomFactor = cam->getZoomFactor();

		//const float MapRulerHLineSize = getQuadrantGlobalSize() * zoomFactor;
		//std::string labelText = std::to_string(int(getQuadrantWorldSize())) + " m";
		const float MapRulerHLineSize = getQuadrantGlobalSize();
		std::string labelText = std::format("{:.2f}", getQuadrantWorldSize() / zoomFactor) + " m";

		const float MapRulerVLineSize = 10;
		const double MapRulerLineWidth = 3;

		n = m_rulerGroup->find_child("MapRulerLabel", true, false);
		godot::Label* rulerLabel = godot::Object::cast_to<godot::Label>(n);
		rulerLabel->set_text(labelText.c_str());
		int32_t fontSize = rulerLabel->get_theme_font_size("font_size");
		godot::Vector2 minimumRulerLabelSize(real_t(labelText.size() * fontSize), real_t(fontSize));
		//rulerLabel->set_size(minimumRulerLabelSize);

		n = m_rulerGroup->find_child("MapRulerHLine", true, false);
		godot::Line2D* rulerHorizontalLine = godot::Object::cast_to<godot::Line2D>(n);
		rulerHorizontalLine->clear_points();
		rulerHorizontalLine->add_point(godot::Vector2(0, 0));
		rulerHorizontalLine->add_point(godot::Vector2(MapRulerHLineSize, 0));

		n = m_rulerGroup->find_child("MapRulerVLine1", true, false);
		godot::Line2D* rulerVerticalLine1 = godot::Object::cast_to<godot::Line2D>(n);
		rulerVerticalLine1->clear_points();
		rulerVerticalLine1->add_point(godot::Vector2(0, -MapRulerVLineSize / 2));
		rulerVerticalLine1->add_point(godot::Vector2(0, MapRulerVLineSize / 2));

		n = m_rulerGroup->find_child("MapRulerVLine2", true, false);
		godot::Line2D* rulerVerticalLine2 = godot::Object::cast_to<godot::Line2D>(n);
		rulerVerticalLine2->clear_points();
		rulerVerticalLine2->add_point(godot::Vector2(MapRulerHLineSize, -MapRulerVLineSize / 2));
		rulerVerticalLine2->add_point(godot::Vector2(MapRulerHLineSize, MapRulerVLineSize / 2));

		godot::Vector2 minimumRulerSize(MapRulerHLineSize + float(MapRulerLineWidth), MapRulerVLineSize + fontSize);
		m_rulerGroupCustomMinimumSize = godot::Vector2(TheWorld_Utils::Utils::max2(minimumRulerLabelSize.x, minimumRulerSize.x), TheWorld_Utils::Utils::max2(minimumRulerLabelSize.y, minimumRulerSize.y));

		godot::Vector2 rulerGroupSize = m_rulerGroupCustomMinimumSize;
		m_rulerGroup->set_anchor(godot::Side::SIDE_LEFT, 0.8f);
		m_rulerGroup->set_offset(SIDE_LEFT, 0.0);
		m_rulerGroup->set_anchor(godot::Side::SIDE_TOP, 1.0);
		m_rulerGroup->set_offset(SIDE_TOP, -(rulerGroupSize.y + 20));
		m_rulerGroup->set_size(rulerGroupSize);
	}
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
	}
}

void GDN_TheWorld_Map::_physics_process(double _delta)
{
	if (m_mapViewer == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
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

	if (not m_shiftPressed)
	{
		resetDragInputEvents();
	}
	
	bool startDragging = false;
	bool stopDragging = false;
	if (m_shiftPressed and input->is_action_pressed("ui_mouse_button_left"))
	{
		if (not m_dragBoxOn)
		{
			startDragging = true;
			m_dragBoxOn = true;
			m_dragBoxType = DragBoxType::Select;
		}
	}

	if (m_shiftPressed and input->is_action_just_released("ui_mouse_button_left"))
	{
		if (m_dragBoxOn)
		{
			if (m_dragBoxType != DragBoxType::Select and m_dragBox != nullptr)
				m_dragBox->hide();	// cancel dragging operation if release button is different from press button
			stopDragging = true;
			m_dragBoxOn = false;
		}
	}

	if (m_shiftPressed and input->is_action_pressed("ui_mouse_button_right"))
	{
		if (not m_dragBoxOn)
		{
			startDragging = true;
			m_dragBoxOn = true;
			m_dragBoxType = DragBoxType::Unselect;
		}
	}

	if (m_shiftPressed and input->is_action_just_released("ui_mouse_button_right"))
	{
		if (m_dragBoxOn)
		{
			if (m_dragBoxType != DragBoxType::Unselect and m_dragBox != nullptr)
				m_dragBox->hide();	// cancel dragging operation if release button is different from press button
			stopDragging = true;
			m_dragBoxOn = false;
		}
	}

	if (startDragging)
	{
		godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
		godot::Vector2 mouseGlobalPos = getMousePosInGlobalCoord(mousePosInViewport);
		godot::Vector2 mouseWorldPos = globalPosToWorldPos(mouseGlobalPos);
		m_startDragBoxQuadrantPos = TheWorld_Utils::QuadrantPos(mouseWorldPos.x, mouseWorldPos.y, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
	}

	if (stopDragging)
	{
		godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
		godot::Vector2 mouseGlobalPos = getMousePosInGlobalCoord(mousePosInViewport);
		godot::Vector2 mouseWorldPos = globalPosToWorldPos(mouseGlobalPos);
		m_endDragBoxQuadrantPos = TheWorld_Utils::QuadrantPos(mouseWorldPos.x, mouseWorldPos.y, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
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

	if (m_mapViewer == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return;

	if (!viewer->mapMode())
		return;

	bool startDragging = false;
	bool stopDragging = false;
	if (m_shiftPressed and event->is_action_pressed("ui_mouse_button_left"))
	{
		if (not m_dragBoxOn)
		{
			startDragging = true;
			m_dragBoxOn = true;
			m_dragBoxType = DragBoxType::Select;
		}
	}

	if (m_shiftPressed and event->is_action_released("ui_mouse_button_left"))
	{
		if (m_dragBoxOn)
		{
			if (m_dragBoxType != DragBoxType::Select and m_dragBox != nullptr)
				m_dragBox->hide();	// cancel dragging operation if release button is different from press button
			stopDragging = true;
			m_dragBoxOn = false;
		}
	}

	if (m_shiftPressed and event->is_action_pressed("ui_mouse_button_right"))
	{
		if (not m_dragBoxOn)
		{
			startDragging = true;
			m_dragBoxOn = true;
			m_dragBoxType = DragBoxType::Unselect;
		}
	}

	if (m_shiftPressed and event->is_action_released("ui_mouse_button_right"))
	{
		if (m_dragBoxOn)
		{
			if (m_dragBoxType != DragBoxType::Unselect and m_dragBox != nullptr)
				m_dragBox->hide();	// cancel dragging operation if release button is different from press button
			stopDragging = true;
			m_dragBoxOn = false;
		}
	}

	if (startDragging)
	{
		godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
		godot::Vector2 mouseGlobalPos = getMousePosInGlobalCoord(mousePosInViewport);
		godot::Vector2 mouseWorldPos = globalPosToWorldPos(mouseGlobalPos);
		m_startDragBoxQuadrantPos = TheWorld_Utils::QuadrantPos(mouseWorldPos.x, mouseWorldPos.y, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
	}

	if (stopDragging)
	{
		godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
		godot::Vector2 mouseGlobalPos = getMousePosInGlobalCoord(mousePosInViewport);
		godot::Vector2 mouseWorldPos = globalPosToWorldPos(mouseGlobalPos);
		m_endDragBoxQuadrantPos = TheWorld_Utils::QuadrantPos(mouseWorldPos.x, mouseWorldPos.y, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
	}
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

	if (m_mapViewer == nullptr)
		return;

	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
	if (viewer == nullptr)
		return;

	std::lock_guard<std::recursive_mutex> lockM (viewer->getMainProcessingMutex());

	for (auto& item : m_mapTextureRectRefreshScheduled)
	{
		TheWorld_Utils::MapTextureRectPos pos = item.first;

		setStatus(pos, GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsNeeded);
		//bool success = destroyTextureRect(pos);
		//success = eraseCachedTexture(pos);
		//success = eraseMapTextureRect(pos);

		m_mapViewer->getViewer()->forceProcessNewMapQuadrants(true);
	}
	m_mapTextureRectRefreshScheduled.clear();

	std::vector<TheWorld_Utils::MapTextureRectPos> toErase;
	for (auto& item : m_uploadingMapTextureRectContainer)
	{
		if (item.second->uploaded())
		{
			toErase.push_back(item.first);
		}
	}

	for (auto& item : toErase)
	{
		m_uploadingMapTextureRectContainer.erase(item);
	}

	if (viewer->isEditMode())
	{
		if (m_dragBoxOn)
		{
			if (m_dragBox == nullptr)
			{
				m_dragBox = memnew(godot::ColorRect);
				add_child(m_dragBox);
				m_dragBox->set_name("MapDragBox");
				//m_dragBox->set_color(GDN_TheWorld_Globals::g_color_yellow_transparent);
				m_dragBox->set_color(GDN_TheWorld_Globals::g_color_yellow_transparent);
				m_dragBox->set_z_index(GDN_TheWorld_Map::c_map_z_index_drag_box);
			}
			m_dragBox->show();
			godot::Vector2 rectPos = worldPosToGlobalPos(godot::Vector2(m_startDragBoxQuadrantPos.getLowerXGridVertex(), m_startDragBoxQuadrantPos.getLowerZGridVertex()));
			godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
			godot::Vector2 mouseGlobalPos = getMousePosInGlobalCoord(mousePosInViewport);
			godot::Vector2 mouseWorldPos = globalPosToWorldPos(mouseGlobalPos);
			TheWorld_Utils::QuadrantPos currentDragBoxQuadrantPos = TheWorld_Utils::QuadrantPos(mouseWorldPos.x, mouseWorldPos.y, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
			godot::Vector2 currentEndDragBoxGlobalPos = worldPosToGlobalPos(godot::Vector2(currentDragBoxQuadrantPos.getLowerXGridVertex() + currentDragBoxQuadrantPos.getSizeInWU(), currentDragBoxQuadrantPos.getLowerZGridVertex() + currentDragBoxQuadrantPos.getSizeInWU()));
			godot::Vector2 rectSize = currentEndDragBoxGlobalPos - rectPos;
			if (rectSize.x <= 0)
			{
				rectPos.x += rectSize.x;
				rectPos.x -= worldSizeToGlobalSize(currentDragBoxQuadrantPos.getSizeInWU());
				rectSize.x = -rectSize.x + 2 * worldSizeToGlobalSize(currentDragBoxQuadrantPos.getSizeInWU());
			}
			if (rectSize.y <= 0)
			{
				rectPos.y += rectSize.y;
				rectPos.y -= worldSizeToGlobalSize(currentDragBoxQuadrantPos.getSizeInWU());
				rectSize.y = -rectSize.y + 2 * worldSizeToGlobalSize(currentDragBoxQuadrantPos.getSizeInWU());
			}
			m_dragBox->set_global_position(rectPos);
			m_dragBox->set_size(rectSize);
		}
		else
		{
			if (m_dragBox != nullptr and m_dragBox->is_visible())
			{
				//m_dragBox->hide();
				//m_dragBox->queue_free();
				//m_dragBox = nullptr;

				{
					// Select/unselect quadrants in rect
					viewer->deselectWorldQuadrant();	// Eventually deselect previously selected quadrant if any

					int startQuadrantIndexX = m_startDragBoxQuadrantPos.getIndexPosX();
					int endQuadrantIndexX = m_endDragBoxQuadrantPos.getIndexPosX();
					if (m_endDragBoxQuadrantPos.getIndexPosX() < m_startDragBoxQuadrantPos.getIndexPosX())
					{
						startQuadrantIndexX = m_endDragBoxQuadrantPos.getIndexPosX();
						endQuadrantIndexX = m_startDragBoxQuadrantPos.getIndexPosX();
					}
					int startQuadrantIndexZ = m_startDragBoxQuadrantPos.getIndexPosZ();
					int endQuadrantIndexZ = m_endDragBoxQuadrantPos.getIndexPosZ();
					if (m_endDragBoxQuadrantPos.getIndexPosZ() < m_startDragBoxQuadrantPos.getIndexPosZ())
					{
						startQuadrantIndexZ = m_endDragBoxQuadrantPos.getIndexPosZ();
						endQuadrantIndexZ = m_startDragBoxQuadrantPos.getIndexPosZ();
					}
					for (int x = startQuadrantIndexX; x <= endQuadrantIndexX; ++x)
					{
						for (int z = startQuadrantIndexZ; z <= endQuadrantIndexZ; ++z)
						{
							// Select/unselect quadrant at (x, z)
							TheWorld_Utils::QuadrantPos pos = TheWorld_Utils::QuadrantPos(x, z, viewer->getLevel(), (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
							if (m_dragBoxType == DragBoxType::Select)
							{
								viewer->selectMapQuadrantRef(pos);
								viewer->deselectMapQuadrant();
							}
							if (m_dragBoxType == DragBoxType::Unselect)
							{
								viewer->clearOverrideMapQuadrantRef(pos);
							}
						}
					}

					m_dragBoxType = DragBoxType::Unknown;
					m_startDragBoxQuadrantPos.reset();
					m_endDragBoxQuadrantPos.reset();
				}

				resetDragInputEvents();
			}
		}
	}
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
	uint64_t mainTid = godot::OS::get_singleton()->get_main_thread_id();
	uint64_t tid = godot::OS::get_singleton()->get_thread_caller_id();
	godot::Size2 windowSize;
	if (mainTid == tid)
		windowSize = viewer->getViewportSize();
	else
		windowSize = viewer->getSavedViewportSize();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();

	screenSize = windowSize * (1 / mapCamera->getZoomFactor());

	return screenSize;
}

godot::Vector2 GDN_TheWorld_Map::getMousePosInGlobalCoord(void) const
{
	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	
	godot::Vector2 mousePosInViewport = viewer->getMousePosInViewport();
	return getMousePosInGlobalCoord(mousePosInViewport);
}

godot::Vector2 GDN_TheWorld_Map::getMousePosInGlobalCoord(godot::Vector2 mousePosInViewport) const
{
	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();

	godot::Vector2 distanceFromUpLeftScreenBorderInGlobalCoord = mousePosInViewport * (1 / mapCamera->getZoomFactor());
	godot::Rect2 screenRectInGlobalCoord = getScreenRectInGlobalCoord();
	godot::Vector2 mousePosInGlobalCoord(screenRectInGlobalCoord.position.x + distanceFromUpLeftScreenBorderInGlobalCoord.x, screenRectInGlobalCoord.position.y + distanceFromUpLeftScreenBorderInGlobalCoord.y);
	return mousePosInGlobalCoord;
}

bool GDN_TheWorld_Map::eraseNotInScreenTextureRects(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

	bool allTextureRendered = true;
	
	std::vector<TheWorld_Utils::MapTextureRectPos> toDeleteRectsPos;
	std::vector<TheWorld_Utils::MapTextureRectPos> rectsPos;
	getScreenTextureRects(rectsPos);
	
	std::map<TheWorld_Utils::MapTextureRectPos, bool> rectsPosMap;
	for (auto& item : rectsPos)
		rectsPosMap[item] = true;

	for (auto& item : m_mapTextureRectContainer)
	{
		if (not rectsPosMap.contains(item.first))
			toDeleteRectsPos.push_back(item.first);
		if (not item.second->isTextureRectRendered() or item.second->AcqQuadrantsNeeded() or item.second->AcqQuadrantsInProgress())
			allTextureRendered = false;
	}

	for (auto& item : toDeleteRectsPos)
	{
		m_mapTextureRectContainer[item]->destroyInternalTextureRect();
		m_mapTextureRectContainer.erase(item);
	}

	return allTextureRendered;
}

void GDN_TheWorld_Map::getScreenTextureRects(std::vector<TheWorld_Utils::MapTextureRectPos>& rectsPos)
{
	rectsPos.clear();
	
	GDN_TheWorld_Viewer* viewer = getMapViewer()->getViewer();
	GDN_TheWorld_MapCamera* mapCamera = viewer->MapCamera();

	// W A R N I N G: all calcs are made in global coords not in pixels because camera zoom alter distances
	godot::Rect2 screenRectInGlobalCoord = getScreenRectInGlobalCoord();
	godot::Vector2i textureRectSize = getTextureRectSize();
	godot::Vector2 mapCameraGlobalPos;
	uint64_t mainTid = godot::OS::get_singleton()->get_main_thread_id();
	uint64_t tid = godot::OS::get_singleton()->get_thread_caller_id();
	if (mainTid == tid)
		mapCameraGlobalPos = mapCamera->get_global_position();
	else
		mapCameraGlobalPos = mapCamera->getSavedGlobalPosition();
	TheWorld_Utils::MapTextureRectPos cameraTextureRectGlobalPos(mapCameraGlobalPos.x, mapCameraGlobalPos.y, viewer->getLevel(), getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());
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
	//higherIndexY = lowerIndexY;		higherIndexX = lowerIndexX;		// DEBUG ONLY
	for (int32_t y = lowerIndexY; y <= higherIndexY; y++)
	{
		for (int32_t x = lowerIndexX; x <= higherIndexX; x++)
		{
			TheWorld_Utils::MapTextureRectPos textureRectGlobalPos(x, y, viewer->getLevel(), getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());
			rectsPos.push_back(textureRectGlobalPos);
		}
	}
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

float GDN_TheWorld_Map::getQuadrantWorldSize(void)
{
	return Globals()->heightmapSizeInWUs();
}

float GDN_TheWorld_Map::getQuadrantGlobalSize(void) const
{
	//return float(m_mapViewer->getViewer()->getImageNumPixelsPerSize());
	return float(m_mapViewer->getViewer()->getImageNumPixelsPerSize() - 1);
}

int32_t GDN_TheWorld_Map::getNumQuadrantsPerSize(void) const
{
	return GDN_TheWorld_Map::numImagesForTextureRectSide();
}

godot::Vector2i GDN_TheWorld_Map::getTextureRectSize(void) const
{
	godot::Vector2 imageSize = getImageSize();
	return (imageSize * godot::Vector2i(GDN_TheWorld_Map::numImagesForTextureRectSide(), GDN_TheWorld_Map::numImagesForTextureRectSide())) /* * getScaleFactor()*/;
}

godot::Vector2i GDN_TheWorld_Map::getImageSize(void) const
{
	//return godot::Vector2i(m_mapViewer->getViewer()->getImageMapResolution(), m_mapViewer->getViewer()->getImageMapResolution());
	return godot::Vector2i((int32_t)getQuadrantGlobalSize(), (int32_t)getQuadrantGlobalSize());
}

godot::Vector2 GDN_TheWorld_Map::globalPosToWorldPos(godot::Vector2 globalPos)
{
	//godot::Vector2 worldPos = global / (getImageSize() - godot::Vector2(1, 1));
	godot::Vector2 worldPos = globalPos / getImageSize();
	worldPos = worldPos * Globals()->heightmapSizeInWUs();
	return worldPos;
}

godot::Vector2 GDN_TheWorld_Map::worldPosToGlobalPos(godot::Vector2 worldPos)
{
	godot::Vector2 globalPos = worldPos / Globals()->heightmapSizeInWUs();
	//globalPos = globalPos * (getImageSize() - godot::Vector2(1, 1));
	globalPos = globalPos * getImageSize();
	return globalPos;
}

float GDN_TheWorld_Map::globalSizeToWorldSize(float globalSize)
{
	float worldSize = globalSize / getImageSize().x;
	worldSize = worldSize * Globals()->heightmapSizeInWUs();
	return worldSize;
}

float GDN_TheWorld_Map::worldSizeToGlobalSize(float worldSize)
{
	float globalSize = worldSize / Globals()->heightmapSizeInWUs();
	globalSize = globalSize * getImageSize().x;
	return globalSize;
}

int32_t GDN_TheWorld_Map::getNumTextureRects(void)
{
	int32_t num = 0;
	
	//godot::Array nodes = get_children();
	//int32_t size = int32_t(nodes.size());
	//for (int i = 0; i < size; i++)
	//{
	//	godot::Node* node = godot::Object::cast_to<godot::Node>(nodes[i]);
	//	if (node->is_in_group("MapTextureRect"))
	//		num++;
	//}

	num = m_mapTextureRectNum;
	num = get_tree()->get_node_count_in_group("MapTextureRect");

	return num;
}

int32_t GDN_TheWorld_Map::getMapQuadrantColorOverrideNum(void)
{
	int32_t num = 0;
	
	num = m_mapQuadrantColorOverrideNum;
	num = get_tree()->get_node_count_in_group("MapQuadrantColorRect");
	return num;
}

int32_t GDN_TheWorld_Map::getMapQuadrantTextureOverrideNum(void)
{
	int32_t num = 0;
	
	num = m_mapQuadrantTextureOverrideNum;
	num = get_tree()->get_node_count_in_group("MapQuadrantTextureRect");
	return num;
}

void GDN_TheWorld_Map::addMapTextureRectRefreshScheduled(TheWorld_Utils::MapTextureRectPos& pos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	m_mapTextureRectRefreshScheduled[pos] = true;
	if (m_mapTextureRectContainer.contains(pos))
		m_mapTextureRectContainer[pos]->checkForRefreshDisabled();
}

void GDN_TheWorld_Map::quadrantReady(TheWorld_Utils::QuadrantPos& pos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	if (m_mapQuadrantOverrideContainer.contains(pos))
	{
		m_mapQuadrantOverrideContainer[pos]->quadrantReady();
		m_mapQuadrantOverrideContainer[pos]->applyColor();
	}
}

void GDN_TheWorld_Map::quadrantWorkInProgress(TheWorld_Utils::QuadrantPos& pos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	if (m_mapQuadrantOverrideContainer.contains(pos))
	{
		m_mapQuadrantOverrideContainer[pos]->quadrantWorkInProgress();
		m_mapQuadrantOverrideContainer[pos]->applyColor();
	}
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride* GDN_TheWorld_Map::getQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	GDN_TheWorld_Map::MapTextureRect::MapQuadrantColorOverride* rect = nullptr;

	TheWorld_Utils::MapTextureRectPos pos = TheWorld_Utils::MapTextureRectPos::getFromQuadrant(quadrantPos, GDN_TheWorld_Map::getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());

	if (m_mapTextureRectContainer.contains(pos))
	{
		rect = m_mapTextureRectContainer[pos]->getQuadrantColorOverride(quadrantPos, createIfMissing);
	}

	return rect;
}

GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride* GDN_TheWorld_Map::getQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos, bool createIfMissing)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	GDN_TheWorld_Map::MapTextureRect::MapQuadrantTextureOverride* rect = nullptr;

	TheWorld_Utils::MapTextureRectPos pos = TheWorld_Utils::MapTextureRectPos::getFromQuadrant(quadrantPos, GDN_TheWorld_Map::getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());

	if (m_mapTextureRectContainer.contains(pos))
	{
		rect = m_mapTextureRectContainer[pos]->getQuadrantTextureOverride(quadrantPos, createIfMissing);
	}

	return rect;
}

void GDN_TheWorld_Map::releaseQuadrantColorOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	if (m_destroying)	// If map is destroying Globals and Viewer are already done
		return;
	
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	TheWorld_Utils::MapTextureRectPos pos = TheWorld_Utils::MapTextureRectPos::getFromQuadrant(quadrantPos, GDN_TheWorld_Map::getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());

	if (m_mapTextureRectContainer.contains(pos))
	{
		m_mapTextureRectContainer[pos]->releaseQuadrantColorOverride(quadrantPos);
	}
}

void GDN_TheWorld_Map::releaseQuadrantTextureOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	if (m_destroying)	// If map is destroying Globals and Viewer are already done
		return;

	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	TheWorld_Utils::MapTextureRectPos pos = TheWorld_Utils::MapTextureRectPos::getFromQuadrant(quadrantPos, GDN_TheWorld_Map::getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());

	if (m_mapTextureRectContainer.contains(pos))
	{
		m_mapTextureRectContainer[pos]->releaseQuadrantTextureOverride(quadrantPos);
	}
}

void GDN_TheWorld_Map::getAllMapQuadrantOverridePos(std::list<TheWorld_Utils::QuadrantPos>& list, bool onlyEdited)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	list.clear();
	for (auto& item : m_mapQuadrantOverrideContainer)
	{
		if (onlyEdited)
		{
			if (item.second->edited())
				list.push_back(item.first);
		}
		else
			list.push_back(item.first);
	}
}

size_t GDN_TheWorld_Map::getAllMapQuadrantOverrideNum(bool onlyEdited)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	
	if (onlyEdited)
	{
		size_t count = 0;
		for (auto& item : m_mapQuadrantOverrideContainer)
		{
			if (item.second->edited())
				count++;
		}
		return count;
	}
	else
		return m_mapQuadrantOverrideContainer.size();
}

GDN_TheWorld_Map::MapQuadrantOverride* GDN_TheWorld_Map::getMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
		return m_mapQuadrantOverrideContainer[quadrantPos].get();
	else
		return nullptr;
}

GDN_TheWorld_Map::MapQuadrantOverride* GDN_TheWorld_Map::addEditedMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos, TheWorld_Utils::TerrainEdit& terrainEdit)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
	{
		m_mapQuadrantOverrideContainer[quadrantPos]->setTerrainEdit(terrainEdit);
	}
	else
	{
		m_mapQuadrantOverrideContainer[quadrantPos] = std::make_unique<MapQuadrantOverride>(this, quadrantPos);
		m_mapQuadrantOverrideContainer[quadrantPos]->setTerrainEdit(terrainEdit);
	}
	m_mapQuadrantOverrideContainer[quadrantPos]->applyTexture();
	return m_mapQuadrantOverrideContainer[quadrantPos].get();
}

void GDN_TheWorld_Map::removeEditedMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
	{
		m_mapQuadrantOverrideContainer[quadrantPos]->resetEditedStatus();
		if (m_mapQuadrantOverrideContainer[quadrantPos]->canBeErased())
			m_mapQuadrantOverrideContainer.erase(quadrantPos);
	}
}

GDN_TheWorld_Map::MapQuadrantOverride* GDN_TheWorld_Map::addRefreshingMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
	{
		m_mapQuadrantOverrideContainer[quadrantPos]->refreshInProgress(true);
	}
	else
	{
		m_mapQuadrantOverrideContainer[quadrantPos] = std::make_unique<MapQuadrantOverride>(this, quadrantPos);
		m_mapQuadrantOverrideContainer[quadrantPos]->refreshInProgress(true);
	}
	m_mapQuadrantOverrideContainer[quadrantPos]->applyColor();
	return m_mapQuadrantOverrideContainer[quadrantPos].get();
}

void GDN_TheWorld_Map::removeRefreshingMapQuadrantOverride(TheWorld_Utils::QuadrantPos& quadrantPos)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
	{
		m_mapQuadrantOverrideContainer[quadrantPos]->refreshInProgress(false);
		if (m_mapQuadrantOverrideContainer[quadrantPos]->canBeErased())
			m_mapQuadrantOverrideContainer.erase(quadrantPos);
		else
			m_mapQuadrantOverrideContainer[quadrantPos]->applyColor();
	}
}

void GDN_TheWorld_Map::editMode(TheWorld_Utils::QuadrantPos& quadrantPos, bool b)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());
	if (m_mapQuadrantOverrideContainer.contains(quadrantPos))
	{
		m_mapQuadrantOverrideContainer[quadrantPos]->editMode(b);
		m_mapQuadrantOverrideContainer[quadrantPos]->applyColor();
	}
}

void GDN_TheWorld_Map::resetDragInputEvents(void)
{
	m_dragBoxOn = false;
	m_dragBoxType = DragBoxType::Unknown;
	//m_shiftPressed = false;
	//m_ctrlPressed = false;
	//m_altPressed = false;
	if (m_dragBox != nullptr)
	{
		m_dragBox->hide();
		//m_dragBox->queue_free();
		//m_dragBox = nullptr;
	}
	//m_startDragBoxQuadrantPos.reset();
	//m_endDragBoxQuadrantPos.reset();
}

void GDN_TheWorld_Map::worldQuadrantsMapReset(void)
{
	std::lock_guard<std::recursive_mutex> lockM(m_mapViewer->getViewer()->getMainProcessingMutex());

	WorldQuadrantsMap& worldQuadrantsMap = m_mapViewer->getViewer()->worldQuadrantsMapReference();

	std::map<TheWorld_Utils::QuadrantPos, bool> checkQuadrantsActuallyInWorld;
	for (auto& item : m_mapQuadrantOverrideContainer)
		checkQuadrantsActuallyInWorld[item.first] = false;
	for (auto& worldQuadrantItem : worldQuadrantsMap)
		if (not worldQuadrantItem.second->statusToErase())
			checkQuadrantsActuallyInWorld[worldQuadrantItem.first] = true;

	size_t numAddedInWorld = 0, numErasedInWorld = 0, numMainteinedInWorld = 0;
	for (auto& item : checkQuadrantsActuallyInWorld)
	{
		if (item.second)
		{
			// quadrant actually in world
			if (not m_mapQuadrantOverrideContainer.contains(item.first))
			{
				TheWorld_Utils::QuadrantPos quadrantPos = item.first;
				m_mapQuadrantOverrideContainer[item.first] = std::make_unique<MapQuadrantOverride>(this, quadrantPos);
				//m_mapQuadrantImageOverrideContainer[item.first]->render();
				numAddedInWorld++;
			}
			else
			{
				numMainteinedInWorld++;
			}
			m_mapQuadrantOverrideContainer[item.first]->quadrantActuallyInWorld(true);
		}
		else
		{
			// quadrant not actually in world
			m_mapQuadrantOverrideContainer[item.first]->quadrantActuallyInWorld(false);
			numErasedInWorld++;
		}
	}

	//assert(numAddedInWorld >= 0);
	//assert(numMainteinedInWorld >= 0);
	//assert(numErasedInWorld >= 0);

	std::vector<TheWorld_Utils::QuadrantPos> canBeErased;

	for (auto& quadrantOverrideItem : m_mapQuadrantOverrideContainer)
	{
		MapQuadrantOverride* mapQuadrantOverride = quadrantOverrideItem.second.get();
		if (quadrantOverrideItem.second->quadrantActuallyInWorld())
		{
			QuadTree* quadTree = worldQuadrantsMap[quadrantOverrideItem.first].get();
			mapQuadrantOverride->quadrantDepth(quadTree->quadrantDepth(), int(quadTree->numQuadrantsFromCameraQuadrant()));
			if (quadTree->quadrantReady())
				mapQuadrantOverride->quadrantReady();
			else
				mapQuadrantOverride->quadrantWorkInProgress();
		}
		else
		{
			mapQuadrantOverride->quadrantDepth(QuadTree::QuadrantDepth::NotSet, -1);
		}
		mapQuadrantOverride->applyColor();

		if (mapQuadrantOverride->canBeErased())
			canBeErased.push_back(quadrantOverrideItem.first);
	}

	for (auto& item : canBeErased)
	{
		m_mapQuadrantOverrideContainer.erase(item);
	}
}

godot::String GDN_TheWorld_Map::getCacheDir(void)
{
	return m_mapViewer->getViewer()->Globals()->getCacheDir(m_mapViewer->getViewer()->getLevel());
}

void GDN_TheWorld_Map::selectTextureRectForUpload(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, std::string& utcFileTimeStr, godot::Ref<godot::Image> rectImage)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	m_uploadingMapTextureRectContainer[mapTextureRectPos] = make_unique<UploadingMapTextureRect>(this, mapTextureRectPos, utcFileTimeStr, rectImage);
}

bool GDN_TheWorld_Map::uploadSelectedTextureRect(void)
{
	bool uploadInProgress = false;
	
	if (m_mapViewer == nullptr)
		return uploadInProgress;

	if (m_mapViewer->getViewer() == nullptr)
		return uploadInProgress;

	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

	if (m_uploadingMapTextureRectContainer.size() == 0)
		return uploadInProgress;

	size_t size = 0;
	size_t cursor = 0;
	TheWorld_Utils::MemoryBuffer buffer;

	buffer.reserve(100000);		// preallocation for optimization purpose
	constexpr int32_t maxRectsInBuffer = 10;
	size_t numSent = 0;
	for (auto& item : m_uploadingMapTextureRectContainer)
	{
		if (item.second->uploadRequired())
		{
			// textureRectPosBuffer
			TheWorld_Utils::MemoryBuffer textureRectPosBuffer;
			item.second->pos().serialize(textureRectPosBuffer);
			buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
			TheWorld_Utils::serializeToByteStream<size_t>(textureRectPosBuffer.size(), buffer.ptr() + cursor, size);
			buffer.adjustCursorAndSize(cursor, size);
			buffer.reserve(cursor + textureRectPosBuffer.size());		// to avoid memory overflow
			memcpy(buffer.ptr() + cursor, textureRectPosBuffer.ptr(), textureRectPosBuffer.size());
			buffer.adjustCursorAndSize(cursor, textureRectPosBuffer.size());

			// utcFileTimeStr
			std::string fileTime = item.second->utcFileTimeStr();
			buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
			TheWorld_Utils::serializeToByteStream<size_t>(fileTime.size(), buffer.ptr() + cursor, size);
			buffer.adjustCursorAndSize(cursor, size);
			buffer.reserve(cursor + fileTime.size());		// to avoid memory overflow
			memcpy(buffer.ptr() + cursor, fileTime.c_str(), fileTime.size());
			buffer.adjustCursorAndSize(cursor, fileTime.size());

			// rectImage stream
			godot::Ref<godot::Image> image = item.second->rectImage();
			int32_t _imageSize = image->get_size().x * image->get_size().y;
			size_t imageSize = image->get_data_size();
			godot::PackedByteArray imageData = image->get_data();
			buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
			TheWorld_Utils::serializeToByteStream<size_t>(imageSize, buffer.ptr() + cursor, size);
			buffer.adjustCursorAndSize(cursor, size);
			buffer.reserve(cursor + imageSize);		// to avoid memory overflow
			memcpy(buffer.ptr() + cursor, (char*)imageData.ptr(), imageSize);
			buffer.adjustCursorAndSize(cursor, imageSize);

			numSent++;
			item.second->setStatus(UploadingMapTextureRect::Status::UploadInProgress);

			if (numSent >= maxRectsInBuffer)
				break;
		}
	}
	std::string mapTextureRects;
	buffer.toString(mapTextureRects);

	if (numSent == 0)
		return uploadInProgress;

	//buffer.clear();
	buffer.reserve(100000);		// preallocation for optimization purpose
	cursor = 0;
	*buffer.ptr() = '0';
	buffer.adjustCursorAndSize(cursor, 1);

	// numSent
	buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
	TheWorld_Utils::serializeToByteStream<size_t>(numSent, buffer.ptr() + cursor, size);
	buffer.adjustCursorAndSize(cursor, size);

	// mapTextureRects as string
	buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
	TheWorld_Utils::serializeToByteStream<size_t>(mapTextureRects.size(), buffer.ptr() + cursor, size);
	buffer.adjustCursorAndSize(cursor, size);
	buffer.reserve(cursor + mapTextureRects.size());		// to avoid memory overflow
	memcpy(buffer.ptr() + cursor, mapTextureRects.c_str(), mapTextureRects.size());
	buffer.adjustCursorAndSize(cursor, mapTextureRects.size());

	buffer.toString(mapTextureRects);
	Globals()->Client()->MapManagerUploadTextureRects(m_mapViewer->getViewer()->isInEditor(), m_mapViewer->getViewer()->isEditMode(), Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU(), mapTextureRects);

	uploadInProgress = true;
	return uploadInProgress;
}

bool GDN_TheWorld_Map::checkRequiredTextureRect(void)
{
	bool checkInProgress = false;

	if (m_mapViewer == nullptr or m_mapViewer->getViewer() == nullptr)
		return checkInProgress;

	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

	size_t size = 0;
	size_t cursor = 0;
	TheWorld_Utils::MemoryBuffer buffer;

	buffer.reserve(100000);		// preallocation for optimization purpose
	constexpr int32_t maxRectsInBuffer = 10;
	size_t numSent = 0;
	for (auto& item : m_mapTextureRectContainer)
	{
		if (item.second->CheckFromServerRequired())
		{
			// textureRectPosBuffer
			TheWorld_Utils::MemoryBuffer textureRectPosBuffer;
			item.second->pos().serialize(textureRectPosBuffer);
			buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
			TheWorld_Utils::serializeToByteStream<size_t>(textureRectPosBuffer.size(), buffer.ptr() + cursor, size);
			buffer.adjustCursorAndSize(cursor, size);
			buffer.reserve(cursor + textureRectPosBuffer.size());		// to avoid memory overflow
			memcpy(buffer.ptr() + cursor, textureRectPosBuffer.ptr(), textureRectPosBuffer.size());
			buffer.adjustCursorAndSize(cursor, textureRectPosBuffer.size());

			numSent++;
			item.second->setStatus(MapTextureRect::Status::CheckFromServerInProgress);

			if (numSent >= maxRectsInBuffer)
				break;
		}
	}
	std::string mapTextureRects;
	buffer.toString(mapTextureRects);

	if (numSent == 0)
		return checkInProgress;

	//buffer.clear();
	buffer.reserve(100000);		// preallocation for optimization purpose
	cursor = 0;
	*buffer.ptr() = '0';
	buffer.adjustCursorAndSize(cursor, 1);

	// numSent
	buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
	TheWorld_Utils::serializeToByteStream<size_t>(numSent, buffer.ptr() + cursor, size);
	buffer.adjustCursorAndSize(cursor, size);

	// mapTextureRects as string
	buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
	TheWorld_Utils::serializeToByteStream<size_t>(mapTextureRects.size(), buffer.ptr() + cursor, size);
	buffer.adjustCursorAndSize(cursor, size);
	buffer.reserve(cursor + mapTextureRects.size());		// to avoid memory overflow
	memcpy(buffer.ptr() + cursor, mapTextureRects.c_str(), mapTextureRects.size());
	buffer.adjustCursorAndSize(cursor, mapTextureRects.size());

	buffer.toString(mapTextureRects);
	Globals()->Client()->MapManagerCheckTextureRects(m_mapViewer->getViewer()->isInEditor(), m_mapViewer->getViewer()->isEditMode(), Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU(), mapTextureRects);

	checkInProgress = true;
	return checkInProgress;
}

void GDN_TheWorld_Map::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
{
	std::string method = reply.getMethod();

	if (reply.error())
	{
		//Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint((std::string("GDN_TheWorld_Map::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADMAPTEXTURERECTS)
			m_mapViewer->getViewer()->uploadTextureRectInProgress(false);
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_CHECKMAPTEXTURERECTS)
			m_mapViewer->getViewer()->checkServerTextureRectInProgress(false);
		return;
	}

	try
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADMAPTEXTURERECTS)
		{
			ClientServerVariant v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getReplyParam(0);
			std::vector<unsigned char>* _repliedTextureRects = std::get_if<std::vector<unsigned char>>(&v);
			if (_repliedTextureRects == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a std::vector<unsigned char> as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string repliedTextureRects((*_repliedTextureRects).begin(), (*_repliedTextureRects).end());

			if (repliedTextureRects.size() > 0)
			{
				size_t size = 0;
				size_t replyCursor = 1;				// bypassing first byte '0'
				size_t numRepliedTextureRects = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)repliedTextureRects.c_str() + replyCursor, size);
				replyCursor += size;

				if (numRepliedTextureRects > 0)
				{
					size_t repliedMapTextureRectsBufferSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)repliedTextureRects.c_str() + replyCursor, size);
					replyCursor += size;

					TheWorld_Utils::MemoryBuffer repliedMapTextureRectsBuffer((BYTE*)repliedTextureRects.c_str() + replyCursor, repliedMapTextureRectsBufferSize);
					replyCursor += repliedMapTextureRectsBufferSize;

					std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

					size_t repliedMapTextureRectsBufferCursor = 0;
					for (int32_t idx = 0; idx < numRepliedTextureRects; idx++)
					{
						size_t mapTextureRectPosBufferSize = TheWorld_Utils::deserializeFromByteStream<size_t>(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, size);
						repliedMapTextureRectsBufferCursor += size;

						TheWorld_Utils::MemoryBuffer mapTextureRectPosBuffer(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, mapTextureRectPosBufferSize);
						repliedMapTextureRectsBufferCursor += mapTextureRectPosBufferSize;
						TheWorld_Utils::MapTextureRectPos mapTextureRectPos;
						mapTextureRectPos.deserialize(mapTextureRectPosBuffer);

						if (m_uploadingMapTextureRectContainer.contains(mapTextureRectPos) and m_uploadingMapTextureRectContainer[mapTextureRectPos]->uploadInProgress())
						{
							m_uploadingMapTextureRectContainer[mapTextureRectPos]->setStatus(UploadingMapTextureRect::Status::Uploaded);
						}
					}
				}
			}

			m_mapViewer->getViewer()->uploadTextureRectInProgress(false);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_CHECKMAPTEXTURERECTS)
		{
			ClientServerVariant v = reply.getInputParam(1);
			const auto _numVerticesPerSize(std::get_if<size_t>(&v));
			if (_numVerticesPerSize == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a size_t as second input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			size_t numVerticesPerSize = *_numVerticesPerSize;

			v = reply.getInputParam(2);
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getReplyParam(0);
			std::vector<unsigned char>* _repliedTextureRects = std::get_if<std::vector<unsigned char>>(&v);
			if (_repliedTextureRects == NULL)
			{
				std::string m = std::string("Reply MapManager::uploadTextureRects did not return a std::vector<unsigned char> as first return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string repliedTextureRects((*_repliedTextureRects).begin(), (*_repliedTextureRects).end());

			if (repliedTextureRects.size() > 0)
			{
				size_t size = 0;
				size_t replyCursor = 1;				// bypassing first byte '0'
				size_t numRepliedTextureRects = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)repliedTextureRects.c_str() + replyCursor, size);
				replyCursor += size;

				if (numRepliedTextureRects > 0)
				{
					size_t repliedMapTextureRectsBufferSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)repliedTextureRects.c_str() + replyCursor, size);
					replyCursor += size;

					TheWorld_Utils::MemoryBuffer repliedMapTextureRectsBuffer((BYTE*)repliedTextureRects.c_str() + replyCursor, repliedMapTextureRectsBufferSize);
					replyCursor += repliedMapTextureRectsBufferSize;

					std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

					size_t repliedMapTextureRectsBufferCursor = 0;
					for (int32_t idx = 0; idx < numRepliedTextureRects; idx++)
					{
						// TheWorld_Utils::MapTextureRectPos
						size_t mapTextureRectPosBufferSize = TheWorld_Utils::deserializeFromByteStream<size_t>(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, size);
						repliedMapTextureRectsBufferCursor += size;
						TheWorld_Utils::MemoryBuffer mapTextureRectPosBuffer(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, mapTextureRectPosBufferSize);
						repliedMapTextureRectsBufferCursor += mapTextureRectPosBufferSize;
						TheWorld_Utils::MapTextureRectPos mapTextureRectPos;
						mapTextureRectPos.deserialize(mapTextureRectPosBuffer);

						// Image Buffer
						size_t imageBufferSize = TheWorld_Utils::deserializeFromByteStream<size_t>(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, size);
						repliedMapTextureRectsBufferCursor += size;
						TheWorld_Utils::MemoryBuffer imageBuffer;
						if (imageBufferSize > 0)
						{
							imageBuffer.set(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, imageBufferSize);
							repliedMapTextureRectsBufferCursor += imageBufferSize;
						}

						// Image Time
						std::chrono::system_clock::time_point imageTime = TheWorld_Utils::deserializeFromByteStream<std::chrono::system_clock::time_point>(repliedMapTextureRectsBuffer.ptr() + repliedMapTextureRectsBufferCursor, size);
						repliedMapTextureRectsBufferCursor += size;

						if (m_mapTextureRectContainer.contains(mapTextureRectPos) and m_mapTextureRectContainer[mapTextureRectPos]->CheckFromServerInProgress())
						{
							if (imageBuffer.size() > 0)
							{
								m_mapTextureRectContainer[mapTextureRectPos]->setInternalImageData(imageBuffer, imageTime);
							}
							m_mapTextureRectContainer[mapTextureRectPos]->setStatus(MapTextureRect::Status::CheckFromServerDone);
						}
					}
				}
			}

			m_mapViewer->getViewer()->checkServerTextureRectInProgress(false);
		}
		else
		{
			Globals()->setStatus(TheWorldStatus::error);
			Globals()->errorPrint((std::string("GDN_TheWorld_Map::replyFromServer: unknow method ") + method).c_str());
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
		Globals()->errorPrint("GDN_TheWorld_Map::replyFromServer: exception caught");
	}
}

bool GDN_TheWorld_Map::destroyTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->destroyInternalTextureRect();
}

bool GDN_TheWorld_Map::isTextureRectRendered(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->isTextureRectRendered();
}

bool GDN_TheWorld_Map::eraseCachedTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->eraseCachedTexture();
}

bool GDN_TheWorld_Map::renderCachedTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->renderCachedTexture();
}

bool GDN_TheWorld_Map::renderInternalTexture(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->renderInternalTexture();
}

bool GDN_TheWorld_Map::CheckFromServerNotDone(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->CheckFromServerNotDone();
}

bool GDN_TheWorld_Map::CheckFromServerRequired(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->CheckFromServerRequired();
}

bool GDN_TheWorld_Map::CheckFromServerInProgress(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->CheckFromServerInProgress();
}

bool GDN_TheWorld_Map::CheckFromServerDone(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->CheckFromServerDone();
}

bool GDN_TheWorld_Map::AcqQuadrantsNeeded(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->AcqQuadrantsNeeded();
}

bool GDN_TheWorld_Map::AcqQuadrantsInProgress(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(mapTextureRectPos)->AcqQuadrantsInProgress();
}

void GDN_TheWorld_Map::setStatus(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, MapTextureRect::Status status)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	getMapTextureRect(mapTextureRectPos)->setStatus(status);
}

GDN_TheWorld_Map::MapTextureRect* GDN_TheWorld_Map::getMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos, bool add)
{
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());

	if (!add and not m_mapTextureRectContainer.contains(mapTextureRectPos))
		return nullptr;

	if (not m_mapTextureRectContainer.contains(mapTextureRectPos))
	{
		m_mapTextureRectContainer[mapTextureRectPos] = std::make_unique<MapTextureRect>(this, mapTextureRectPos);
		m_mapTextureRectContainer[mapTextureRectPos]->init();
	}
	return m_mapTextureRectContainer[mapTextureRectPos].get();
}

//bool GDN_TheWorld_Map::eraseMapTextureRect(TheWorld_Utils::MapTextureRectPos& mapTextureRectPos)
//{
//	GDN_TheWorld_Viewer* viewer = m_mapViewer->getViewer();
//	std::lock_guard<std::recursive_mutex> lock(viewer->getMainProcessingMutex());
//
//	GDN_TheWorld_Globals* globals = viewer->Globals();
//
//	std::vector<TheWorld_Utils::QuadrantPos> quadrantsPos;
//	mapTextureRectPos.getAllContainedQuadrantPos(quadrantsPos, viewer->getLevel(), (int)globals->numVerticesPerQuadrantSize(), globals->gridStepInWU());
//	for (auto& pos : quadrantsPos)
//		viewer->eraseMapQuadrant(pos);
//
//	if (m_mapTextureRectContainer.contains(mapTextureRectPos))
//	{
//		m_mapTextureRectContainer.erase(mapTextureRectPos);
//		return true;
//	}
//	else
//		return false;
//}

GDN_TheWorld_Map::MapTextureRect* GDN_TheWorld_Map::getMapTextureRect(const TheWorld_Utils::QuadrantPos& quadrantPos, bool add)
{
	godot::Vector2 quadrantGlobalPos = worldPosToGlobalPos(godot::Vector2(quadrantPos.getLowerXGridVertex(), quadrantPos.getLowerZGridVertex()));
	TheWorld_Utils::MapTextureRectPos mapTextureRectPos(quadrantGlobalPos.x, quadrantGlobalPos.y, quadrantPos.getLevel(), getNumQuadrantsPerSize(), getQuadrantGlobalSize(), getQuadrantWorldSize());
	return getMapTextureRect(mapTextureRectPos, add);
}

bool GDN_TheWorld_Map::setMapQuadrantData(const godot::Ref<godot::Image> mapImage, const TheWorld_Utils::QuadrantPos& quadrantPos, std::chrono::system_clock::time_point quadrantTime)
{
	if (mapImage == nullptr)
		return false;
	
	std::lock_guard<std::recursive_mutex> lock(m_mapViewer->getViewer()->getMainProcessingMutex());
	return getMapTextureRect(quadrantPos)->setMapQuadrantData(mapImage, quadrantPos, quadrantTime);
}

void GDN_TheWorld_Map::setWorldFlag(godot::String name, godot::Color c, bool setCameraDir, bool visible)
{
	godot::Node2D* worldFlag = nullptr;
	if (m_worldFlagDict.has(name))
	{
		worldFlag = Object::cast_to<godot::Node2D>(m_worldFlagDict[name]);
	}
	else
	{
		worldFlag = memnew(godot::Node2D);
		add_child(worldFlag);
		worldFlag->set_name("WorldFlag_" + name);
		worldFlag->set_visible(false);
		worldFlag->set_z_index(c_map_z_index_rect_color_overlay);

		constexpr int32_t FLAG_RADIUS = 7;
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
			dir->set_width(3);
			dir->add_point(godot::Vector2(0, 0));
			dir->add_point(godot::Vector2(0, -FLAG_RADIUS * 3));
		}
		
		m_worldFlagDict[name] = worldFlag;
	}

	if (worldFlag != nullptr)
	{
		godot::Camera3D* worldCamera = getMapViewer()->getViewer()->getWorldCamera2(true);
		GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(worldCamera);
		if (cam != nullptr)
		{
			godot::Transform3D worldTransform = cam->getWorldTransform();

			godot::Vector2 worldFlagGlobalPosition = m_mapViewer->getViewer()->worldPosToGlobalPos(godot::Vector2(worldTransform.origin.x, worldTransform.origin.z));
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

//void GDN_TheWorld_MapViewport::_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}

//void GDN_TheWorld_MapViewport::_unhandled_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}

//void GDN_TheWorld_MapViewport::_ready(void)
//{
//}

//void GDN_TheWorld_MapViewport::_process(double _delta)
//{
//}

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
	//if (p_what != NOTIFICATION_PROCESS
	//	and p_what != NOTIFICATION_WM_MOUSE_ENTER and p_what != NOTIFICATION_WM_MOUSE_EXIT
	//	and p_what != 41 and p_what != 42 and p_what != 61 and p_what != 62 and p_what != 30 and p_what != 31 and p_what != 50 and p_what != 51 and p_what != 1004 and p_what != 1005 and p_what != 2016 and p_what != 2017)
	//{
	//	godot::Vector2 newSize = get_size();
	//}
	
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
		sizeChanged(newSize);
	}
	break;
	}
}

void GDN_TheWorld_MapViewportContainer::sizeChanged(godot::Vector2 newSize)
{
	GDN_TheWorld_MapViewer* mapViewer = getMapViewer();
	if (mapViewer != nullptr)
	{
		GDN_TheWorld_MapViewport* mapViewport = getMapViewport();
		if (mapViewport != nullptr)
			mapViewport->set_size(newSize);
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

//void GDN_TheWorld_MapViewer::_notification(int p_what)
//{
//}

//void GDN_TheWorld_MapViewer::_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}

//void GDN_TheWorld_MapViewer::_unhandled_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}

//void GDN_TheWorld_MapViewer::_ready(void)
//{
//}

//void GDN_TheWorld_MapViewer::_process(double _delta)
//{
//}

//void GDN_TheWorld_HudControls::_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//
//	//if (event->is_action_pressed("ui_mouse_button_left"))
//	//{
//	//	PLOG_INFO << "ui_mouse_button_left is_action_pressed...";
//	//}
//
//	//if (event->is_action_released("ui_mouse_button_left"))
//	//{
//	//	PLOG_INFO << "ui_mouse_button_left is_action_released...";
//	//}
//}

//void GDN_TheWorld_HudControls::_unhandled_input(const Ref<InputEvent>& event)
//{
//	//const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
//	//const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
//
//	//if (eventMouseButton != nullptr and eventMouseButton->is_pressed())
//	//{
//	//	int a = 0;
//	//}
//}

void GDN_TheWorld_HudControls::init(GDN_TheWorld_Viewer* viewer)
{
	m_viewer = viewer;

	godot::Control::MouseFilter mouseFilter = godot::Control::MouseFilter::MOUSE_FILTER_IGNORE;
	set_mouse_filter(mouseFilter);

	m_loadingVBox = memnew(godot::VBoxContainer);
	add_child(m_loadingVBox);
	m_loadingVBox->set_name("HudLoadingVBox");
	m_loadingVBox->set_custom_minimum_size(godot::Vector2(500, 0));
	m_loadingVBox->set_visible(false);
	m_loadingVBox->set_mouse_filter(mouseFilter);

	m_loadingLabel = memnew(godot::Label);
	m_loadingVBox->add_child(m_loadingLabel);
	m_loadingLabel->set_name("HudLoadingText");
	m_loadingLabel->add_theme_color_override("font_color", Color(0.5, 0, 0));
	m_loadingLabel->add_theme_font_size_override("font_size", 20);
	m_loadingLabel->set_text("Loading...");
	m_loadingLabel->set_mouse_filter(mouseFilter);

	m_loadingProgressBar = memnew(godot::ProgressBar);
	m_loadingVBox->add_child(m_loadingProgressBar);
	m_loadingProgressBar->set_name("HudLoadingProgressBar");
	m_loadingProgressBar->set_mouse_filter(mouseFilter);
}

void GDN_TheWorld_HudControls::setPos(godot::Size2& windowSize)
{
	set_size(windowSize);

	if (m_loadingVBox != nullptr)
	{
		godot::Vector2 loadingContainerSize = m_loadingVBox->get_size();
		
		m_loadingVBox->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_loadingVBox->set_offset(SIDE_TOP, (windowSize.y / 2) - (loadingContainerSize.y / 2));
		m_loadingVBox->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		m_loadingVBox->set_offset(SIDE_LEFT, (windowSize.x / 2) - (loadingContainerSize.x / 2));
	}
}

void GDN_TheWorld_HudControls::resetLoadingProgress(void)
{
	m_loadingProgress = 0;
}

void GDN_TheWorld_HudControls::setLoadingProgress(float loadingProgress)
{
	if (m_loadingProgressBar != nullptr)
	{
		//if (m_loadingProgress == 100)
		//	assert(m_loadingProgress == 100);
		if (loadingProgress == 0)
			m_loadingProgress = 0;
		if (loadingProgress < m_loadingProgress)
		{
			//loadingProgress = m_loadingProgress;
		}
		else
			m_loadingProgress = loadingProgress;
		//if (loadingProgress == 100)
		//	assert(loadingProgress == 100);
		m_loadingProgressBar->set_value(loadingProgress);
	}
}

void GDN_TheWorld_HudControls::showLoadingBar(bool reset)
{
	if (m_loadingVBox != nullptr)
	{
		if (reset)
			setLoadingProgress(0);
		m_loadingVBox->set_visible(true);
	}
}

void GDN_TheWorld_HudControls::hideLoadingBar(void)
{
	if (m_loadingVBox != nullptr)
		m_loadingVBox->set_visible(false);
}

bool GDN_TheWorld_HudControls::isLoadingBarVisibile(void)
{
	if (m_loadingVBox != nullptr)
		return m_loadingVBox->is_visible();
	else
		return false;
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
		PLOG_DEBUG << "MapModder Init For Normals - Quadrant " << quadtree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadtree->getQuadrant()->getMeshId());

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
		PLOG_DEBUG << "MapModder Init For Splatmap - Quadrant " << quadtree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadtree->getQuadrant()->getMeshId());

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
		PLOG_DEBUG << "MapModder Init For Globalmap - Quadrant " << quadtree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadtree->getQuadrant()->getMeshId());

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

		if (outImageFormat != godot::Image::Format::FORMAT_RGB8)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "convert format");
			m_outImage->convert(godot::Image::Format::FORMAT_RGB8);
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
		my_assert(outImageFormat == godot::Image::Format::FORMAT_RGBA8);
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

		if (outImageFormat != godot::Image::Format::FORMAT_RGB8)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder getOutBuffer 1.2 ") + __FUNCTION__, "convert format");
			m_outImage->convert(godot::Image::Format::FORMAT_RGB8);
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
		godot::Ref<godot::Image> image = godot::Image::create_from_data((int32_t)m_imageSize, (int32_t)m_imageSize, false, godot::Image::Format::FORMAT_RF, data);
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
			mat->set_shader_parameter(SHADER_PARAM_GRID_STEP_OPTLVL_ZERO, gridStepInWU);
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
			godot::Ref<godot::Image> image = godot::Image::create_from_data(int32_t(m_imageSize), int32_t(m_imageSize), false, godot::Image::Format::FORMAT_RGB8, data);
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
			//godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/test/height2splatmap.tres");
			godot::Ref<godot::Shader> shader = resLoader->load("res://addons/twviewer/shaders/height2splatmap.tres");
			mat->set_shader(shader);
			//float gridStepInWU = m_viewer->Globals()->gridStepInWU();
			//mat->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);
			//mat->set_shader_parameter(SHADER_PARAM_GRID_STEP_OPTLVL_ZERO, gridStepInWU);
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

			//{
			//	Quadrant* quadrant = m_quadTree->getQuadrant();
			//	TheWorld_Utils::QuadrantPos quadrantPos = quadrant->getPos();
			//	
			//	TheWorld_Utils::QuadrantPos northPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
			//	QuadTree* northQuadTree = m_viewer->getQuadTree(northPos, true, false, true);
			//	if (northQuadTree != nullptr && northQuadTree->statusInitialized())
			//	{
			//		godot::Vector2 heights(northQuadTree->getQuadrant()->getTerrainEdit()->minHeight, northQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
			//		mat->set_shader_parameter(SHADER_PARAM_NORTH_HEIGHTS, heights);
			//		m_northQuadrantheights = heights;
			//	}
			//	else
			//	{
			//		godot::Vector2 heights(FLT_MAX, FLT_MIN);
			//		mat->set_shader_parameter(SHADER_PARAM_NORTH_HEIGHTS, heights);
			//		m_northQuadrantheights = heights;
			//	}
			//	TheWorld_Utils::QuadrantPos southPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
			//	QuadTree* southQuadTree = m_viewer->getQuadTree(southPos, true, false, true);
			//	if (southQuadTree != nullptr && southQuadTree->statusInitialized())
			//	{
			//		godot::Vector2 heights(southQuadTree->getQuadrant()->getTerrainEdit()->minHeight, southQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
			//		mat->set_shader_parameter(SHADER_PARAM_SOUTH_HEIGHTS, heights);
			//		m_southQuadrantheights = heights;
			//	}
			//	else
			//	{
			//		godot::Vector2 heights(FLT_MAX, FLT_MIN);
			//		mat->set_shader_parameter(SHADER_PARAM_SOUTH_HEIGHTS, heights);
			//		m_southQuadrantheights = heights;
			//	}
			//	TheWorld_Utils::QuadrantPos westPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
			//	QuadTree* westQuadTree = m_viewer->getQuadTree(westPos, true, false, true);
			//	if (westQuadTree != nullptr && westQuadTree->statusInitialized())
			//	{
			//		godot::Vector2 heights(westQuadTree->getQuadrant()->getTerrainEdit()->minHeight, westQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
			//		mat->set_shader_parameter(SHADER_PARAM_WEST_HEIGHTS, heights);
			//		m_westQuadrantheights = heights;
			//	}
			//	else
			//	{
			//		godot::Vector2 heights(FLT_MAX, FLT_MIN);
			//		mat->set_shader_parameter(SHADER_PARAM_WEST_HEIGHTS, heights);
			//		m_westQuadrantheights = heights;
			//	}
			//	TheWorld_Utils::QuadrantPos eastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
			//	QuadTree* eastQuadTree = m_viewer->getQuadTree(eastPos, true, false, true);
			//	if (eastQuadTree != nullptr && eastQuadTree->statusInitialized())
			//	{
			//		godot::Vector2 heights(eastQuadTree->getQuadrant()->getTerrainEdit()->minHeight, eastQuadTree->getQuadrant()->getTerrainEdit()->maxHeight);
			//		mat->set_shader_parameter(SHADER_PARAM_EAST_HEIGHTS, heights);
			//		m_eastQuadrantheights = heights;
			//	}
			//	else
			//	{
			//		godot::Vector2 heights(FLT_MAX, FLT_MIN);
			//		mat->set_shader_parameter(SHADER_PARAM_EAST_HEIGHTS, heights);
			//		m_eastQuadrantheights = heights;
			//	}
			//}

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
			m_camera->set_size(float(viewportSize.x));
			m_camera->set_near(0.5f);		// 0.1f
			m_camera->set_far(m_viewer->Globals()->heightmapSizeInWUs() + m_cameraAltitude);	// 20000.0f
			m_camera->set_current(true);
			m_camera->set_rotation(godot::Vector3(-TheWorld_Utils::kPi / 2, 0, 0));
			m_mapModderViewport->add_child(m_camera);
		}

		godot::Ref<godot::Material> terrainPlaneMaterial;

		int shaderType = 2;		// Regular

		//else if (shaderType == 2)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("MapModder startProcessing 1.7 ") + __FUNCTION__, "globalmap Material stuff");

			TheWorld_Utils::TerrainEdit* terrainEdit = m_quadTree->getQuadrant()->getTerrainEdit();

			godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getRegularTerrainMaterial();
				material->set_shader(currentMaterial->get_shader());
			}
			//{
			//	String shaderPath = "res://addons/twviewer/test/terrain_globalmap.gdshader";
			//	godot::Ref<godot::Shader> shader;
			//	shader = godot::ResourceLoader::get_singleton()->load(shaderPath);
			//	material->set_shader(shader);
			//}

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
			//	material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
			//	material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
			//}

			//material->set_shader_parameter(SHADER_PARAM_GENERATE_GLOBAL_MAP, true);
			material->set_shader_parameter(SHADER_PARAM_GRID_STEP_OPTLVL_ZERO, gridStepInWU);
			material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
			material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
			material->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
			material->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
			godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
			godot::Variant inverseTransform = t.affine_inverse();
			material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
			//Basis normalBasis = t.basis.inverse().transposed();
			//material->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);
			//material->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());

			godot::Vector4 v = m_quadTree->getQuadrant()->quadrantTextureLayers();
			material->set_shader_parameter(SHADER_PARAM_GROUND_LAYERS, v);

			{
				bool proceedWithBorderOverflow = true;
				//godot::Ref<godot::Texture2D> northQuadrantSplatmapBorderRowTexture;
				//if (proceedWithBorderOverflow)
				//{
				//	northQuadrantSplatmapBorderRowTexture = m_quadTree->getQuadrant()->getSplatmapBorderRowTexture(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
				//	if (northQuadrantSplatmapBorderRowTexture == nullptr)
				//		proceedWithBorderOverflow = false;
				//}
				//godot::Ref<godot::Texture2D> westQuadrantSplatmapBorderRowTexture;
				//if (proceedWithBorderOverflow)
				//{
				//	westQuadrantSplatmapBorderRowTexture = m_quadTree->getQuadrant()->getSplatmapBorderRowTexture(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
				//	if (westQuadrantSplatmapBorderRowTexture == nullptr)
				//		proceedWithBorderOverflow = false;
				//}
				//godot::Color northWestQuadrantCornerSplatmapTexel;
				//if (proceedWithBorderOverflow)
				//{
				//	northWestQuadrantCornerSplatmapTexel = m_quadTree->getQuadrant()->getSplatmapBorderRowTexel(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus);
				//	if (northWestQuadrantCornerSplatmapTexel == godot::Color(0, 0, 0, 0))
				//		proceedWithBorderOverflow = false;
				//}

				godot::Vector4 northQuadrantTextureLayers;
				if (proceedWithBorderOverflow)
				{
					northQuadrantTextureLayers = m_quadTree->getQuadrant()->northQuadrantTextureLayersFromTerrainData();
					if (northQuadrantTextureLayers.x < 0 or northQuadrantTextureLayers.y < 0 or northQuadrantTextureLayers.z < 0 or northQuadrantTextureLayers.w < 0)
						proceedWithBorderOverflow = false;
				}
				godot::Vector4 westQuadrantTextureLayers;
				if (proceedWithBorderOverflow)
				{
					westQuadrantTextureLayers = m_quadTree->getQuadrant()->westQuadrantTextureLayersFromTerrainData();
					if (westQuadrantTextureLayers.x < 0 or westQuadrantTextureLayers.y < 0 or westQuadrantTextureLayers.z < 0 or westQuadrantTextureLayers.w < 0)
						proceedWithBorderOverflow = false;
				}
				godot::Vector4 northwestQuadrantTextureLayers;
				if (proceedWithBorderOverflow)
				{
					northwestQuadrantTextureLayers = m_quadTree->getQuadrant()->northwestQuadrantTextureLayersFromTerrainData();
					if (northwestQuadrantTextureLayers.x < 0 or northwestQuadrantTextureLayers.y < 0 or northwestQuadrantTextureLayers.z < 0 or northwestQuadrantTextureLayers.w < 0)
						proceedWithBorderOverflow = false;
				}

				if (proceedWithBorderOverflow)
				{
					material->set_shader_parameter(SHADER_PARAM_NORTH_GROUND_LAYERS, northQuadrantTextureLayers);
					material->set_shader_parameter(SHADER_PARAM_WEST_GROUND_LAYERS, westQuadrantTextureLayers);
					material->set_shader_parameter(SHADER_PARAM_NORTH_WEST_GROUND_LAYERS, northwestQuadrantTextureLayers);

					//material->set_shader_parameter(SHADER_PARAM_NORTH_BORDER_SPLATMAP, northQuadrantSplatmapBorderRowTexture);
					//material->set_shader_parameter(SHADER_PARAM_WEST_BORDER_SPLATMAP, westQuadrantSplatmapBorderRowTexture);
					//material->set_shader_parameter(SHADER_PARAM_NORTH_WEST_CORNER_SPLATMAP_TEXEL, northWestQuadrantCornerSplatmapTexel);

					float f = m_viewer->Globals()->getTextureOverflowSize();
					material->set_shader_parameter(SHADER_PARAM_TEXTURE_OVERFLOW_SIZE_OVERRIDE, f);
				}
				else
					material->set_shader_parameter(SHADER_PARAM_TEXTURE_OVERFLOW_SIZE_OVERRIDE, 0.0f);		// Disable texture override for global map generation
			}

			material->set_shader_parameter(SHADER_PARAM_OVERRIDE_CAM_ALTITUDE, -1);		// we force, as override for this quadrant, camera altitude from terrain to -1 so the shader will not consider the real camera altitude which is expressed as a global shader param valid for every quadrant
						
			material->set_shader_parameter(SHADER_PARAM_SPLAT_SLOPE_VERTICAL_FACTOR, terrainEdit->extraValues.slopeVerticalFactor);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_SLOPE_FLAT_FACTOR, terrainEdit->extraValues.slopeFlatFactor);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_DIRT_ON_ROCKS_FACTOR, terrainEdit->extraValues.dirtOnRocksFactor);
			//material->set_shader_parameter(SHADER_PARAM_SPLAT_HIGH_ELEVATION_FACTOR, terrainEdit->extraValues.highElevationFactor);
			//material->set_shader_parameter(SHADER_PARAM_SPLAT_LOW_ELEVATION_FACTOR, terrainEdit->extraValues.lowElevationFactor);
			//material->set_shader_parameter(SHADER_PARAM_SPLAT_MIN_HEIGHT, terrainEdit->minHeight);
			//material->set_shader_parameter(SHADER_PARAM_SPLAT_MAX_HEIGHT, terrainEdit->maxHeight);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_HIGH_ELEVATION_FACTOR, TheWorld_Utils::TerrainEdit::THEWORLD_SPLATMAP_HIGH_ELEVATION_FACTOR);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_LOW_ELEVATION_FACTOR, TheWorld_Utils::TerrainEdit::THEWORLD_SPLATMAP_LOW_ELEVATION_FACTOR);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_MIN_HEIGHT, TheWorld_Utils::TerrainEdit::THEWORLD_SPLATMAP_MIN_HEIGHT);
			material->set_shader_parameter(SHADER_PARAM_SPLAT_MAX_HEIGHT, TheWorld_Utils::TerrainEdit::THEWORLD_SPLATMAP_MAX_HEIGHT);

			int numVerticesPerQuadrantSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();

			{
				std::string profileHeader(" ");
				size_t optimizationLevel = 0;
				size_t resolution = m_viewer->Globals()->resolutionForOptimizationLevel(optimizationLevel);
				godot::Ref<godot::Image> image = m_quadTree->getQuadrant()->getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, optimizationLevel, resolution);

				heightmapTexture = godot::ImageTexture::create_from_image(image);
				//if (ShaderTerrainData::s_extraNeighbourBorderInHeightmap)
				//{
				//	image = m_quadTree->getQuadrant()->getShaderTerrainData()->addExtraBordersToHeightmap(image, optimizationLevel);
				//	godot::Ref<godot::ImageTexture> heightmapTexture = godot::ImageTexture::create_from_image(image);
				//	material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);
				//	material->set_shader_parameter(SHADER_EXTRA_VERTICES_ON_HEIGHTMAP, size_t(1));
				//}
				//else
				{
					material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, heightmapTexture);
					material->set_shader_parameter(SHADER_EXTRA_VERTICES_ON_HEIGHTMAP, size_t(0));
				}
			}

			material->set_shader_parameter(SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE, godot::Plane(6.0f, 6.0f, 6.0f, 6.0f));
			material->set_shader_parameter(SHADER_PARAM_DEPTH_BLENDING, true);
			material->set_shader_parameter(SHADER_PARAM_TRIPLANAR, false);
			material->set_shader_parameter(SHADER_PARAM_TILE_REDUCTION_PER_TEXTURE, godot::Plane(0.0f, 0.0f, 0.0f, 0.0f));	// disabled for every texture
			material->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_START, 0);
			material->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_DISTANCE, 0);	// Disable use of globalmap on far pixels
			//material->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, false);			// Disable forced use of globalmap
			material->set_shader_parameter(SHADER_PARAM_COLORMAP_OPACITY_PER_TEXTURE, godot::Plane(1.0f, 1.0f, 1.0f, 1.0f));	// Notset
			material->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, true);
			material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
			//material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_QUADRANT, (float)m_viewer->Globals()->numVerticesPerQuadrantSize());

			terrainPlaneMaterial = material;
		}

		m_terrainPlane->set_material_override(terrainPlaneMaterial);

		if (m_quadTree->canContainSea() == QUADRANT_SEA_INFO_TRUE)
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
				material->set_shader_parameter(SHADER_PARAM_GRID_STEP_OPTLVL_ZERO, gridStepInWU);
				material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
				material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
				material->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
				material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)numVerticesPerQuadrantSize);	// useless as it is used in show chunks mode which is inhibited when generating global map
				//material->set_shader_parameter(SHADER_PARAM_VERTICES_PER_QUADRANT, (float)numVerticesPerQuadrantSize);
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

void GDN_TheWorld_Viewer::ProcessSteps::finalizeCurrentExecution(void)
{
	m_processClock.tock();

	std::vector<ProcessStep> stepsInLastExecutionSortedByPartialElapsed;
	for (auto& processStep : m_steps)		// local copy of step so to modify order sorting by by last partial elapsed
		if (processStep.getNumInCurrentExecution() > 0)
			stepsInLastExecutionSortedByPartialElapsed.push_back(processStep);
	std::sort(stepsInLastExecutionSortedByPartialElapsed.begin(), stepsInLastExecutionSortedByPartialElapsed.end());	// sort by last partial elapsed (operator<) of current process execution

	size_t previousStepElapsedFromProcessStart = 0;
	for (auto& processStep : stepsInLastExecutionSortedByPartialElapsed)
	{
		m_steps[int(processStep.getIdx())].finalizeCurrentExecution(previousStepElapsedFromProcessStart);
	}

	if (m_stepsOrder.size() == 0)
	{
		for (auto& processStep : stepsInLastExecutionSortedByPartialElapsed)
		{
			m_stepsOrder.push_back(processStep.getIdx());
			m_steps[int(processStep.getIdx())].setInOrdering(true);
		}
	}
	else
	{
		for (size_t idx = 0; idx < stepsInLastExecutionSortedByPartialElapsed.size(); idx++)
		{
			if (not stepsInLastExecutionSortedByPartialElapsed[idx].setInOrdering())
			{
				if (stepsInLastExecutionSortedByPartialElapsed.size() > idx + 1)
				{ 
					std::list<ProcessStep::ProcessStepIdx>::iterator itNext = m_stepsOrder.end();
					for (std::list<ProcessStep::ProcessStepIdx>::iterator it = m_stepsOrder.begin(); it != m_stepsOrder.end(); it++)
					{
						if (*it == stepsInLastExecutionSortedByPartialElapsed[idx + 1].getIdx())
						{
							itNext = it;
							break;
						}
					}
					if (itNext != m_stepsOrder.end())
					{
						m_stepsOrder.insert(itNext, stepsInLastExecutionSortedByPartialElapsed[idx].getIdx());
						m_steps[int(stepsInLastExecutionSortedByPartialElapsed[idx].getIdx())].setInOrdering(true);
					}
				}
			}
		}
	}
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
	ClassDB::bind_method(D_METHOD("get_anchor_for_origin_shift"), &GDN_TheWorld_Viewer::getAnchorForOriginShift);
	ClassDB::bind_method(D_METHOD("set_editor_interface"), &GDN_TheWorld_Viewer::setEditorInterface);
	ClassDB::bind_method(D_METHOD("set_editor_3d_overlay"), &GDN_TheWorld_Viewer::setEditor3dOverlay);
	ClassDB::bind_method(D_METHOD("set_world_editor_camera"), &GDN_TheWorld_Viewer::setWorldEditorCamera);
	ClassDB::bind_method(D_METHOD("get_custom_camera"), &GDN_TheWorld_Viewer::getCustomCamera);
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
	ClassDB::bind_method(D_METHOD("set_info_panel_visible", "value"), &GDN_TheWorld_Viewer::setInfoPanelVisible);
	ClassDB::bind_method(D_METHOD("toggle_debug_visibility"), &GDN_TheWorld_Viewer::toggleDebugVisibility);
	ClassDB::bind_method(D_METHOD("get_debug_visibility"), &GDN_TheWorld_Viewer::getDebugContentVisibility);
	ClassDB::bind_method(D_METHOD("rotate_chunk_debug_mode"), &GDN_TheWorld_Viewer::rotateChunkDebugMode);
	ClassDB::bind_method(D_METHOD("rotate_drawing_mode"), &GDN_TheWorld_Viewer::rotateDrawingMode);
	ClassDB::bind_method(D_METHOD("toggle_quadrant_selected"), &GDN_TheWorld_Viewer::toggleWorldQuadrantSelected);
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
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_aabb"), &GDN_TheWorld_Viewer::getWorldCameraTerrainChunkLocalAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_local_debug_aabb"), &GDN_TheWorld_Viewer::getWorldCameraTerrainChunkLocalDebugAABB);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_global_transform_applied"), &GDN_TheWorld_Viewer::getWorldCameraTerrainChunkGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_debug_global_transform_applied"), &GDN_TheWorld_Viewer::getWorldCameraTerrainChunkDebugGlobalTransformApplied);
	ClassDB::bind_method(D_METHOD("get_camera_chunk_id"), &GDN_TheWorld_Viewer::getWorldCameraTerrainChunkId);
	ClassDB::bind_method(D_METHOD("get_camera_quadrant_name"), &GDN_TheWorld_Viewer::getWorldCameraTerrainQuadrantName);
	ClassDB::bind_method(D_METHOD("get_num_splits"), &GDN_TheWorld_Viewer::getNumSplits);
	ClassDB::bind_method(D_METHOD("get_num_joins"), &GDN_TheWorld_Viewer::getNumJoins);
	ClassDB::bind_method(D_METHOD("get_num_chunks"), &GDN_TheWorld_Viewer::getNumChunks);
	ClassDB::bind_method(D_METHOD("get_num_active_chunks"), &GDN_TheWorld_Viewer::getNumActiveChunks);
	//ClassDB::bind_method(D_METHOD("get_loading_progress"), &GDN_TheWorld_Viewer::getLoadingProgress);
	ClassDB::bind_method(D_METHOD("get_max_num_quadrant"), &GDN_TheWorld_Viewer::getMaxNumQuadrants);
	ClassDB::bind_method(D_METHOD("get_min_num_quadrant"), &GDN_TheWorld_Viewer::getMinNumQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_initialized_quadrant"), &GDN_TheWorld_Viewer::getNumInitializedQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_toerase_quadrant"), &GDN_TheWorld_Viewer::getNumToEraseQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_visible_quadrant"), &GDN_TheWorld_Viewer::getNumVisibleQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_initialized_visible_quadrant"), &GDN_TheWorld_Viewer::getNumInitializedVisibleQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_need_shader_reset_quadrant"), &GDN_TheWorld_Viewer::getNumNeedShaderResetQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_need_shader_update_quadrant"), &GDN_TheWorld_Viewer::getNumNeedShaderUpdateQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_empty_quadrant"), &GDN_TheWorld_Viewer::getNumEmptyQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_loading_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrantsLoadingFromServer);
	ClassDB::bind_method(D_METHOD("get_num_reloading_quadrant"), &GDN_TheWorld_Viewer::getNumQuadrantsLoadingFromLocalCache);
	ClassDB::bind_method(D_METHOD("get_num_flushed_quadrant"), &GDN_TheWorld_Viewer::getNumFlushedQuadrants);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_enabled_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithEnabledCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_hires_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithHiResCollider);
	ClassDB::bind_method(D_METHOD("get_num_quadrant_with_lowres_collider"), &GDN_TheWorld_Viewer::getQuadrantNumWithLowResCollider);
	ClassDB::bind_method(D_METHOD("get_delay_degree"), &GDN_TheWorld_Viewer::getDelayDegree);
	ClassDB::bind_method(D_METHOD("get_num_process_not_owns_lock"), &GDN_TheWorld_Viewer::getProcessNotOwnsLock);
	ClassDB::bind_method(D_METHOD("get_process_duration"), &GDN_TheWorld_Viewer::getProcessDuration);
	ClassDB::bind_method(D_METHOD("get_map_quadrant_num"), &GDN_TheWorld_Viewer::getMapQuadrantNum);
	ClassDB::bind_method(D_METHOD("get_updating_map_quadrant_num"), &GDN_TheWorld_Viewer::getUpdatingMapQuadrantNum);
	ClassDB::bind_method(D_METHOD("get_to_show_map_quadrant_num"), &GDN_TheWorld_Viewer::getToShowMapQuadrantNum);
	ClassDB::bind_method(D_METHOD("get_map_quadrant_override_num"), &GDN_TheWorld_Viewer::getMapQuadrantOverrideNum);
	ClassDB::bind_method(D_METHOD("get_map_quadrant_color_override_num"), &GDN_TheWorld_Viewer::getMapQuadrantColorOverrideNum);
	ClassDB::bind_method(D_METHOD("get_map_quadrant_texture_override_num"), &GDN_TheWorld_Viewer::getMapQuadrantTextureOverrideNum);
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
	ClassDB::bind_method(D_METHOD("get_mouse_hit_global"), &GDN_TheWorld_Viewer::getMouseHitGlobalPos);
	ClassDB::bind_method(D_METHOD("get_mouse_hit_in_map"), &GDN_TheWorld_Viewer::getMouseHitWorldPos);
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
	ClassDB::bind_method(D_METHOD("clear_test_quadrant_data"), &GDN_TheWorld_Viewer::clearTestQuadrantData);
	ClassDB::bind_method(D_METHOD("set_hud_message", "message", "fading_time"), &GDN_TheWorld_Viewer::setHudMessage);
	ClassDB::bind_method(D_METHOD("get_quadrant_pos"), &GDN_TheWorld_Viewer::getQuadrantPos);
	ClassDB::bind_method(D_METHOD("get_quadrant_pos_index"), &GDN_TheWorld_Viewer::getQuadrantPosIndex);
	ClassDB::bind_method(D_METHOD("get_test_quadrant_heightmap_image"), &GDN_TheWorld_Viewer::getTestQuadrantHeightmapImage);
	ClassDB::bind_method(D_METHOD("get_test_quadrant_normalmap_image"), &GDN_TheWorld_Viewer::getTestQuadrantNormalmapImage);
	ClassDB::bind_method(D_METHOD("get_test_quadrant_splatmap_image"), &GDN_TheWorld_Viewer::getTestQuadrantSplatmapImage);
	ClassDB::bind_method(D_METHOD("get_test_quadrant_globalmap_image"), &GDN_TheWorld_Viewer::getTestQuadrantGlobalmapImage);
	ClassDB::bind_method(D_METHOD("get_test_quadrant_colormap_image"), &GDN_TheWorld_Viewer::getTestQuadrantColormapImage);
	ClassDB::bind_method(D_METHOD("get_shader_param_ground_uv_scale"), &GDN_TheWorld_Viewer::getShaderParamGroundUVScale);
	ClassDB::bind_method(D_METHOD("get_shader_param_depth_blending"), &GDN_TheWorld_Viewer::getShaderParamDepthBlending);
	ClassDB::bind_method(D_METHOD("get_shader_param_triplanar"), &GDN_TheWorld_Viewer::getShaderParamTriplanar);
	ClassDB::bind_method(D_METHOD("get_shader_param_tile_reduction"), &GDN_TheWorld_Viewer::getShaderParamTileReduction);
	ClassDB::bind_method(D_METHOD("get_ground_texture_name"), &GDN_TheWorld_Viewer::getGroundTextureName);
	ClassDB::bind_method(D_METHOD("get_ground_texture_layers", "p_layer_name"), &GDN_TheWorld_Viewer::getGroundTextureIdxs);
	ClassDB::bind_method(D_METHOD("pause_mouse_movement"), &GDN_TheWorld_Viewer::pauseMouseMovement);
	ClassDB::bind_method(D_METHOD("resume_mouse_movement"), &GDN_TheWorld_Viewer::resumeMouseMovement);

	ClassDB::bind_method(D_METHOD("get_loading_progress"), &GDN_TheWorld_Viewer::getloadingProgressScript);

	ClassDB::bind_method(D_METHOD("global_transform_to_world_transform", "p_transform"), &GDN_TheWorld_Viewer::globalToWorld3d);
	ClassDB::bind_method(D_METHOD("global_pos_to_world_transform", "p_transform"), &GDN_TheWorld_Viewer::globalPosToWorldTransform);
	ClassDB::bind_method(D_METHOD("global_pos_to_world_pos", "p_transform"), &GDN_TheWorld_Viewer::globalPosToWorldPos3d);
	ClassDB::bind_method(D_METHOD("world_transform_to_global_transform", "p_transform"), &GDN_TheWorld_Viewer::worldToGlobal);
	ClassDB::bind_method(D_METHOD("world_pos_to_global_transform", "p_transform"), &GDN_TheWorld_Viewer::worldPosToGlobalTransform);
	ClassDB::bind_method(D_METHOD("world_pos_to_global_pos", "p_transform"), &GDN_TheWorld_Viewer::worldPosToGlobalPos3d);

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
	m_streamerThreadRequiredExit = false;
	m_streamerThreadRunning = false;
	m_editorInterface = nullptr;
	m_editor3dOverlay = nullptr;
	m_editorCamera = nullptr;
	m_shaderParam_depthBlending = false;
	m_shaderParam_triplanar = false;
	m_shaderParam_globalmapBlendStart = 0.0f;
	m_shaderParam_globalmapBlendDistance = 0.0f;
	m_anchorForOriginShift = this;				// origin shift anchor atm must be a node3d parent of the viewer and the camera (because when anchor is relocated both viewer and camera must relocate accordingly), atm the viewer is not parent of the camera so to anable origin shift an external node must be set using setAnchorForOriginShift

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

void GDN_TheWorld_Viewer::ctrlPressed(bool b)
{
	m_ctrlPressed = b;

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr and editModeUIControl->initialized())
		editModeUIControl->holdBlending(m_ctrlPressed);
}

//void GDN_TheWorld_Viewer::resetDataRefreshToShaderFlags(void)
//{
//	m_mapQuadrantToShader.clear();
//	m_vecQuadrantToShader.clear();
//}

//void GDN_TheWorld_Viewer::requestDataRefreshToShader(QuadTree* quadTree, bool request)
//{
//	TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
//	
//	if (request)
//	{
//		if (m_mapQuadrantToShader.contains(pos))
//			return;
//
//		m_mapQuadrantToShader[pos] = true;
//		m_vecQuadrantToShader.push_back(quadTree);
//	}
//	else
//	{
//		if (m_mapQuadrantToShader.contains(pos))
//		{
//			m_mapQuadrantToShader.erase(pos);
//			auto it = std::find(m_vecQuadrantToShader.begin(), m_vecQuadrantToShader.end(), quadTree);
//			if (it != m_vecQuadrantToShader.end())
//				m_vecQuadrantToShader.erase(it);
//		}
//	}
//}

//size_t GDN_TheWorld_Viewer::getNumQuadrantsToRefreshToShader(void) const
//{
//	return (size_t)m_vecQuadrantToShader.size();
//}
//
//void GDN_TheWorld_Viewer::getQuadrantsToRefreshToShader(std::vector<QuadTree*>& quadrantsToRefresh) const
//{
//	quadrantsToRefresh = m_vecQuadrantToShader;
//}

void GDN_TheWorld_Viewer::worldQuadrantReady(TheWorld_Utils::QuadrantPos& pos)
{
	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
	{
		map->quadrantReady(pos);
	}
}

void GDN_TheWorld_Viewer::worldQuadrantWorkInProgress(TheWorld_Utils::QuadrantPos& pos)
{
	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
	{
		map->quadrantWorkInProgress(pos);
	}
}

void GDN_TheWorld_Viewer::quadrantModderWorkDone(int32_t map_type, float x, float z, int32_t num_vertices_per_side, int32_t level, float grid_step)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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
		terrainEdit->splatmapNeedRegen(true);

		quadTree->getQuadrant()->sendNormalsToShader(true);
		quadTree->materialParamsNeedReset(true);
		
		//quadTree->getQuadrant()->northQuadrantData().initialized(false);
		//quadTree->getQuadrant()->southQuadrantData().initialized(false);
		//quadTree->getQuadrant()->westQuadrantData().initialized(false);
		//quadTree->getQuadrant()->eastQuadrantData().initialized(false);
		
		quadTree->getQuadrant()->unlockInternalData();

		save = true;

		PLOG_DEBUG << "MapModder Init For Normals - Quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Work Done";
	}
	else if (mapType == GDN_TheWorld_MapModder::MapType::initializedForSplatmap)
	{
		TheWorld_Utils::MemoryBuffer* cookedSplatmap = mapModder->getOutBuffer();
		size_t numPixels = cookedSplatmap->size() / sizeof(TheWorld_Utils::_RGBA);
		size_t verticesPerBorder = Globals()->numVerticesPerQuadrantSize();
		my_assert(numPixels == verticesPerBorder * verticesPerBorder);
		TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer();
		{
			//splatmapBuffer.reserve(cookedSplatmap->size());
			//splatmapBuffer.adjustSize(cookedSplatmap->size());
			//memcpy(splatmapBuffer.ptr(), cookedSplatmap->ptr(), cookedSplatmap->size());
			cookedSplatmap->getBorders<struct TheWorld_Utils::_RGBA>(verticesPerBorder, splatmapBuffer);	// get 4 borders (N,S,W,E)
		}
		mapModder->deinitRequired(true);

		TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
		terrainEdit->splatmapNeedRegen(false);
		//terrainEdit->eastNeighbourXPlus.minHeight(mapModder->eastQuadrantheights().x);
		//terrainEdit->eastNeighbourXPlus.maxHeight(mapModder->eastQuadrantheights().y);
		//terrainEdit->westNeighbourXMinus.minHeight(mapModder->westQuadrantheights().x);
		//terrainEdit->westNeighbourXMinus.maxHeight(mapModder->westQuadrantheights().y);
		//terrainEdit->southNeighbourZPlus.minHeight(mapModder->southQuadrantheights().x);
		//terrainEdit->southNeighbourZPlus.maxHeight(mapModder->southQuadrantheights().y);
		//terrainEdit->northNeighbourZMinus.minHeight(mapModder->northQuadrantheights().x);
		//terrainEdit->northNeighbourZMinus.maxHeight(mapModder->northQuadrantheights().y);
		terrainEdit->globalmapNeedRegen = true;

		quadTree->getQuadrant()->sendSplatmapToShader(true);
		quadTree->materialParamsNeedReset(true);

		// CHECK these lines were remarks
		quadTree->getQuadrant()->northQuadrantData().initialized(false);
		quadTree->getQuadrant()->southQuadrantData().initialized(false);
		quadTree->getQuadrant()->westQuadrantData().initialized(false);
		quadTree->getQuadrant()->eastQuadrantData().initialized(false);
		quadTree->getQuadrant()->northWestQuadrantData().initialized(false);
		quadTree->getQuadrant()->southEastQuadrantData().initialized(false);

		quadTree->getQuadrant()->splatmapChanged();

		quadTree->getQuadrant()->unlockInternalData();

		save = true;

		PLOG_DEBUG << "MapModder Init For Splatmap - Quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Work Done";

	}
	else if (mapType == GDN_TheWorld_MapModder::MapType::initializedForGlobalmap)
	{
		TheWorld_Utils::MemoryBuffer* cookedGlobalmap = mapModder->getOutBuffer();
		TheWorld_Utils::MemoryBuffer& globalmapBuffer = quadTree->getQuadrant()->getGlobalmapBuffer();
		
		////size_t numPixels = (size_t)sqrt(float(cookedGlobalmap->size() / sizeof(TheWorld_Utils::_RGB)));
		//globalmapBuffer.reserve(cookedGlobalmap->size());
		//memcpy(globalmapBuffer.ptr(), cookedGlobalmap->ptr(), cookedGlobalmap->size());
		//globalmapBuffer.adjustSize(cookedGlobalmap->size());

		size_t imageResolution = Globals()->resolutionForOptimizationLevel((size_t)MapQuadrantImagesMap::getMapQuadrantOptimizationLevel());
		godot::Ref<godot::Image> quadrantMapImage = MapQuadrantImage::quadrantMapBufferToQuadrantMapImage(*cookedGlobalmap, Globals()->numVerticesPerQuadrantSize(), imageResolution);
		godot::PackedByteArray quadrantMapImageData = quadrantMapImage->get_data();
		//std::string outFileName = quadTree->getQuadrant()->getCacheFilePath() + ".map2.png";	// DEBUG
		//quadrantMapImage->save_png(outFileName.c_str());										// DEBUG
		//globalmapBuffer.reserve(quadrantMapImageData.size() + sizeof(int32_t) + sizeof(int32_t) + sizeof(int64_t));
		size_t offset = 0;
		globalmapBuffer.serializeToBufferAndTruncate<int32_t>(quadrantMapImage->get_width(), offset);
		globalmapBuffer.serializeToBufferAndTruncate<int32_t>(quadrantMapImage->get_format(), offset);
		globalmapBuffer.serializeToBufferAndTruncate<int64_t>(quadrantMapImageData.size(), offset);
		globalmapBuffer.serializeToBufferAndTruncate((BYTE*)quadrantMapImageData.ptr(), quadrantMapImageData.size(), offset);

		mapModder->deinitRequired(true);

		TheWorld_Utils::TerrainEdit* terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
		terrainEdit->globalmapNeedRegen = false;
		
		quadTree->getQuadrant()->sendGlobalmapToShader(true);
		quadTree->materialParamsNeedReset(true);
		
		//quadTree->getQuadrant()->northQuadrantData().initialized(false);
		//quadTree->getQuadrant()->southQuadrantData().initialized(false);
		//quadTree->getQuadrant()->westQuadrantData().initialized(false);
		//quadTree->getQuadrant()->eastQuadrantData().initialized(false);

		quadTree->getQuadrant()->unlockInternalData();

		save = true;

		PLOG_DEBUG << "MapModder Init For Globalmap - Quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Work Done";
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
	
	// putting hudControls child of editor3dOverlay makes overlay gui hang
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

		mapViewportContainer->set_anchors_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
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
		GDN_TheWorld_MapViewportContainer* mapViewportContainer = MapViewportContainer();
		GDN_TheWorld_MapViewport* mapViewport = MapViewport();
		
		GDN_TheWorld_Map* map = Map(false);
		if (map == nullptr)
			map = memnew(GDN_TheWorld_Map);
		map->set_name(THEWORLD_MAP_NODE_NAME);
		//map->set_anchors_preset(godot::Control::LayoutPreset::PRESET_FULL_RECT);
		map->init(mapViewer, mapViewportContainer->get_size());
		mapViewport->setMap(map);
		mapViewportContainer->setMap(map);
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
		mapCamera->init(mapViewer);
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
		//mapViewportContainer->set_anchor(godot::Side::SIDE_LEFT, 0.0);
		//mapViewportContainer->set_offset(SIDE_LEFT, 0);
		//mapViewportContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
		//mapViewportContainer->set_offset(SIDE_TOP, 0);
		////mapViewportContainer->set_position(godot::Vector2(0, 0));
		//mapViewportContainer->set_size(windowSize);

		GDN_TheWorld_MapViewport* mapViewport = mapViewportContainer->getMapViewport();
		if (mapViewport != nullptr)
		{
			mapViewport->set_size(windowSize);
		}
	}
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

		SAFE_DELETE_NODE(m_hudControls);	// safe delete of controls because we want to recreate them from scratch when we call createHud, this is because we want to be sure that all the controls are created and added to the scene in the right order, and also because some controls are created as child of others so it's easier to manage them if we delete them all and recreate them from scratch instead of trying to manage the deletion and creation of each control separately
		//m_hudMessageContainer = nullptr;
		//m_hudMessageLabel = nullptr;
		//m_hudStreamingInfoContainer = nullptr;
		//m_hudStreamingInfoLabel = nullptr;
		//m_satelliteSubViewportContainer = nullptr;
		//m_northLabel = nullptr;
		//m_southLabel = nullptr;
		//m_eastLabel = nullptr;
		//m_westLabel = nullptr;
		//m_satelliteSubViewport = nullptr;
		//m_satelliteCamera = nullptr;
		//m_backMirrorSubViewportContainer = nullptr;
		//m_backMirrorSubViewport = nullptr;
		//m_backMirrorCamera = nullptr;

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
		hudControls->init(this);
	}

	godot::Node* controlsParent = HudControls();
	
	// controls can be created as new because safe deleting the HUD all of them are or will be destroyed

	// Message Box
	{
		m_hudMessageContainer = memnew(godot::HBoxContainer);
		controlsParent->add_child(m_hudMessageContainer);
		m_hudMessageContainer->set_name("HudMessageContainer");

		m_hudMessageLabel = memnew(godot::Label);
		m_hudMessageContainer->add_child(m_hudMessageLabel);
		m_hudMessageLabel->set_name("HudMessage");
		//m_hudMessageLabel->add_theme_color_override("font_color", Color(251.0f / 255.0f, 206.0f / 255.0f, 177.0f / 255.0f));
		m_hudMessageLabel->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_red);
	}

	// Streaming Info Box
	{
		m_hudStreamingInfoContainer = memnew(godot::HBoxContainer);
		controlsParent->add_child(m_hudStreamingInfoContainer);
		m_hudStreamingInfoContainer->set_name("HudStreamingInfoContainer");

		m_hudStreamingInfoLabel = memnew(godot::Label);
		m_hudStreamingInfoContainer->add_child(m_hudStreamingInfoLabel);
		m_hudStreamingInfoLabel->set_name("HudStreamingInfo");
		//m_hudStreamingInfoLabel->add_theme_color_override("font_color", Color(251.0f / 255.0f, 206.0f / 255.0f, 177.0f / 255.0f));
		m_hudStreamingInfoLabel->add_theme_color_override("font_color", GDN_TheWorld_Globals::g_color_green);
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
		hudControls->setPos(windowSize);
	
	// Message Box
	if (m_hudMessageContainer != nullptr)
	{
		m_hudMessageContainer->set_anchor(godot::Side::SIDE_LEFT, 0.3f);
		m_hudMessageContainer->set_offset(SIDE_LEFT, 0);
		m_hudMessageContainer->set_anchor(godot::Side::SIDE_TOP, 0.0);
		m_hudMessageContainer->set_offset(SIDE_TOP, 0);
	}

	// Streaming Info Box
	if (m_hudStreamingInfoContainer != nullptr)
	{
		m_hudStreamingInfoContainer->set_anchor(godot::Side::SIDE_LEFT, 0.65f);
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
			godot::Rect2 rect = m_southLabel->get_rect();
			godot::Size2 size = rect.size;
			m_southLabel->set_anchor(godot::Side::SIDE_LEFT, 0.0);
			m_southLabel->set_offset(SIDE_LEFT, SATELLITE_VIEW_CONTROL_SIZE / 2);
			m_southLabel->set_anchor(godot::Side::SIDE_TOP, 1.0);
			m_southLabel->set_offset(SIDE_TOP, -size.y);
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
			godot::Rect2 rect = m_eastLabel->get_rect();
			godot::Size2 size = rect.size;
			m_eastLabel->set_anchor(godot::Side::SIDE_LEFT, 1.0);
			m_eastLabel->set_offset(SIDE_LEFT, -size.x);
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
	
	SceneTree* scene = get_tree();
	if (scene != nullptr)
	{
		Viewport* root = scene->get_root();
		if (root != nullptr)
		{
			//godot::Node* n = root->find_child(THEWORLD_POST_PROCESSING_UNDER_WATER_EFFECT_NODE, true, false);
			//m_postProcessingUnderWaterEffectNode = godot::Object::cast_to<godot::CanvasItem>(n);
			godot::Camera3D* cam = getWorldCamera();
			if (cam != nullptr)
			{
				godot::Ref<godot::FastNoiseLite> noise = memnew(godot::FastNoiseLite);
				noise->set_noise_type(godot::FastNoiseLite::NoiseType::TYPE_CELLULAR);
				noise->set_seed(0);
				noise->set_frequency(0.0018f);
				noise->set_fractal_type(godot::FastNoiseLite::FractalType::FRACTAL_RIDGED);
				noise->set_fractal_octaves(5);
				noise->set_fractal_lacunarity(2.0f);
				noise->set_fractal_gain(0.6f);
				noise->set_fractal_weighted_strength(0.85f);

				godot::Ref<godot::NoiseTexture2D> seaUnderWaterDistorsion = memnew(godot::NoiseTexture2D);
				seaUnderWaterDistorsion->set_noise(noise);
				seaUnderWaterDistorsion->set_width(512);
				seaUnderWaterDistorsion->set_height(512);
				seaUnderWaterDistorsion->set_generate_mipmaps(true);
				seaUnderWaterDistorsion->set_seamless(true);
				seaUnderWaterDistorsion->set_normalize(true);
				seaUnderWaterDistorsion->set_seamless_blend_skirt(0.648f);

				// 1. Crea la MeshInstance
				m_postProcessingUnderWaterEffectMesh = memnew(godot::MeshInstance3D);
				cam->add_child(m_postProcessingUnderWaterEffectMesh);
				// 2. Crea e assegna la QuadMesh
				godot::Ref<godot::QuadMesh> quadMesh = memnew(godot::QuadMesh);
				quadMesh->set_size(godot::Vector2(2, 2));
				m_postProcessingUnderWaterEffectMesh->set_mesh(quadMesh);
				// 3. Posizionamento
				// Lo spostiamo leggermente avanti rispetto alla camera(0.2 metri)
				m_postProcessingUnderWaterEffectMesh->set_position(godot::Vector3(0, 0, -0.7f));
				// 4. Configurazione Materiale(Necessario per l'effetto subacqueo)
				godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
				material->set_shader(ResourceLoader::get_singleton()->load("res://addons/twviewer/test/CameraUnderWater.gdshader"));
				material->set_shader_parameter("u_noise_distorsion", seaUnderWaterDistorsion);
				m_postProcessingUnderWaterEffectMesh->set_surface_override_material(0, material);
				// 5. Rendi la mesh "Full Screen" ignorando la prospettiva
				// Questo trucco assicura che il Quad copra sempre tutto il frame
				m_postProcessingUnderWaterEffectMesh->set_extra_cull_margin(16384.0); // Evita che sparisca ai bordi
			}
		}
	}
	
	m_worldQuadrantsMap.init(this);
	m_mapQuadrantImagesMap.init(this);

	// Shader Global Params
	{
		float _gridStepInWU = Globals()->gridStepInWU();
		godot::RenderingServer* rs = godot::RenderingServer::get_singleton();
		//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GRID_STEP, _gridStepInWU);
		//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_SEA, globals->getParamShowSea());
		//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_TERRAIN, globals->getParamShowTerrain());
		rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_CHUNKS, globals->getParamShowChunks());

		highlightHitQuadrant(false, true);

		rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_HOLD_BLENDING, false);

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

		//ShaderTerrainData::getGroundTextures(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()), this);
		ShaderTerrainData::loadAllGroundTextures("", this);
		ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
		//globals->addElementToInitialize((int)groundTextures.size());
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

	// Adjust environment
	adjustEnvironment();
	
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
		m_normalLineIdx = m_normalDrawer->addLine(godot::Vector3(), godot::Vector3(), "", false, true, GDN_TheWorld_Drawer::Drawing::LabelPos::end, 3.0f, godot::Vector2i(-10, -25), GDN_TheWorld_Globals::g_color_black);
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

void GDN_TheWorld_Viewer::adjustEnvironment(void)
{
	//if (!IS_EDITOR_HINT())
	{
		godot::Array worldEnvironmentNodes;
		getChildrenOfType<godot::WorldEnvironment>(worldEnvironmentNodes);
		if (worldEnvironmentNodes.size() > 0)
		{
			godot::WorldEnvironment* worldEnv = godot::Object::cast_to<godot::WorldEnvironment>(worldEnvironmentNodes[0]);
			if (worldEnv != nullptr)
			{
				godot::Ref<godot::Environment> env = worldEnv->get_environment();
				if (env != nullptr)
				{
					float fogDistanceFactor = Globals()->getParamFogDistanceFactor();
					float fogDepthBegin = float(50000 / 12) * getMaxDepthQuadOnPerimeter() * fogDistanceFactor;	// 53000
					float fogDepthEnd = float(60000 / 12) * getMaxDepthQuadOnPerimeter() * fogDistanceFactor;
					env->set_fog_enabled(Globals()->getParamShowFog());
					env->set_fog_mode(godot::Environment::FogMode::FOG_MODE_DEPTH);
					env->set_fog_density(1.0);
					env->set_fog_aerial_perspective(1.0);
					env->set_fog_depth_begin(fogDepthBegin);
					env->set_fog_depth_end(fogDepthEnd);
				}
			}
		}
	}
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
			if (status == TheWorldStatus::worldDeployInProgress || status == TheWorldStatus::minimalWorldDeployed)
				Globals()->setStatus(TheWorldStatus::worldDeployed);

			Sleep(5);
		}
	}

	m_streamerThreadRequiredExit = true;
}

bool GDN_TheWorld_Viewer::canDeinit(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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

		if (m_postProcessingUnderWaterEffectMesh != nullptr)
		{
			Node* parent = m_postProcessingUnderWaterEffectMesh->get_parent();
			if (parent != nullptr)
				parent->remove_child(m_postProcessingUnderWaterEffectMesh);
			m_postProcessingUnderWaterEffectMesh->queue_free();
			m_postProcessingUnderWaterEffectMesh = nullptr;
		}

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
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
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
		clearQuadrantsMaps();
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
		//Globals()->setStatus(TheWorldStatus::error);
		Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC)
		{
			ClientServerVariant v = reply.getInputParam(3);
			std::vector<unsigned char>* _clientQuadrants = std::get_if<std::vector<unsigned char>>(&v);
			if (_clientQuadrants != NULL)
			{
				std::string clientQuadrants((*_clientQuadrants).begin(), (*_clientQuadrants).end());
				if (clientQuadrants.length() > 0)
					m_quadrantSyncInProgress = false;
			}
		}
		return;
	}

	//try	
	{
		if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER)
		{
			std::string profilerFunction = std::string("WorldDeploy 1 ") + __FUNCTION__;
			std::string profilerName = THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER;
			TheWorld_Utils::Profiler::addElapsedMs(profilerFunction, profilerName, reply.duration());
			
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2 ") + __FUNCTION__, THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER);

			enum class TheWorldStatus status = Globals()->status();
			if (status == TheWorldStatus::worldUnDeployInProgress || status < TheWorldStatus::worldDeployInProgress)
			{
				GDN_TheWorld_Globals* globals = Globals();
				if (globals != nullptr)
				{
					std::string msg = std::string("GDN_TheWorld_Viewer::replyFromServer - ") + THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER + " - Status=" + std::to_string(int(status)) + " (" + globals->statusStr(status) + "): cannot proceed deploying quadrant";
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

				TheWorld_Utils::QuadrantPos quadrantPos(lowerXGridVertex, lowerZGridVertex, level, numVerticesPerSize, gridStepInWU);\
				//if (quadrantPos.getIndexPosX() == 100 && quadrantPos.getIndexPosZ() == 100)	// DEBUG ONLY
				//	v = reply.getInputParam(13);												// DEBUG ONLY

				bool updateMapQuadrantMap = true;
				
				m_mtxQuadrantsAndMainProcessing.lock();
				
				if ((m_worldQuadrantsMap.contains(quadrantPos) && !m_worldQuadrantsMap[quadrantPos]->statusToErase())
					|| (m_extraQuadsMap.contains(quadrantPos) && !m_extraQuadsMap[quadrantPos]->statusToErase() && (status == TheWorldStatus::worldDeployed or status == TheWorldStatus::minimalWorldDeployed)))
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1 ") + __FUNCTION__, "Elab. quadrant");
					
					QuadTree* quadTree = nullptr;
					bool refreshingQuadrant = false;
					//bool worldQuadrant = true;
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
					}
					
					m_mtxQuadrantsAndMainProcessing.unlock();

					if (m_stopDeploy)
					{
						//quadTree->setStatus(QuadrantStatus::initialized);
						quadTree->setStatus(QuadrantStatus::initializationStopped);
						updateMapQuadrantMap = false;
					}
					else
					{
						if (quadTree->statusGetTerrainDataInProgress() || quadTree->statusRefreshTerrainDataInProgress())
						{
							if (quadTree->statusRefreshTerrainDataInProgress())
							{
								//Globals()->debugPrint((std::string("Refreshing quadrant: received terrain data for quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(meshIdFromServer) + " ").c_str());
								PLOG_VERBOSE << "Refreshing quadrant: received terrain data for quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(meshIdFromServer);
								quadrantPos.trackQuadrantForDebug("", "Refreshing quadrant: received terrain data for quadrant");
							}
							else
							{
								//Globals()->debugPrint((std::string("Initializing quadrant: received terrain data for quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
								PLOG_VERBOSE << "Initializing quadrant: received terrain data for quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId());
								quadrantPos.trackQuadrantForDebug("", "Initializing quadrant: received terrain data for quadrant");
							}

							//std::recursive_mutex& quadrantMutex = quadTree->getQuadrantMutex();
							//std::lock_guard<std::recursive_mutex> lock(quadrantMutex);
							while (quadTree->getQuadrant()->lockInternalData() == false)
								Sleep(1);

							std::string outMeshIdFromBuffer;
							{
								//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);	// SUPERDEBUGRIC : to remove when the mock for altitudes is removed from cache.refreshMeshCacheFromBuffer
								TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1 ") + __FUNCTION__, "quadTree->getQuadrant()->refreshGridVertices");
								quadTree->getQuadrant()->refreshGridVerticesFromServer(buffGridVerticesFromServer, meshIdFromServer, outMeshIdFromBuffer, true);

								//if (worldQuadrant)
								//{
								//	godot::Ref<godot::Image> mapImage = quadTree->getQuadrant()->getMapImage();
								//	m_mapQuadrantImagesMap.setMapData(mapImage, quadrantPos, meshIdFromServer);
								//	updateMapQuadrantMap = false;
								//}
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
							{
								//Globals()->debugPrint((std::string("Refreshed quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
								PLOG_VERBOSE << "Refreshed quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId());
							}
							else
							{
								//Globals()->debugPrint((std::string("Initialized quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
								PLOG_VERBOSE << "Initialized quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId());
							}

							if (refreshingQuadrant)
							{
								Chunk::TerrainHeightsChangedChunkAction action;
								quadTree->ForAllChunk(action, QuadType::NotSet);
							}

							{
								m_mtxQuadrantsAndMainProcessing.lock(); 
								
								quadTree->materialParamsNeedReset(true);
								quadTree->colliderNeedReset(true, QuadType::NotSet);
								if (quadTree->statusRefreshTerrainDataInProgress())
								{
									quadTree->getQuadrant()->northQuadrantData().initialized(false);
									quadTree->getQuadrant()->southQuadrantData().initialized(false);
									quadTree->getQuadrant()->westQuadrantData().initialized(false);
									quadTree->getQuadrant()->eastQuadrantData().initialized(false);
									quadTree->getQuadrant()->northWestQuadrantData().initialized(false);
									quadTree->getQuadrant()->southEastQuadrantData().initialized(false);
								}

								quadTree->getQuadrant()->mapModderWorkDone(false);

								GDN_TheWorld_Map* map = Map(true, true);
								if (map != nullptr)
									map->removeEditedMapQuadrantOverride(quadrantPos);

								quadTree->setStatus(QuadrantStatus::initialized);

								m_mtxQuadrantsAndMainProcessing.unlock();
							}
							
							quadTree->getQuadrant()->unlockInternalData();
						}
					}
				}
				else
				{
					m_mtxQuadrantsAndMainProcessing.unlock();
				}

				if (updateMapQuadrantMap)
				{
					QuadTree* quadTree = nullptr;
					m_mtxQuadrantsAndMainProcessing.lock();
					if(m_worldQuadrantsMap.contains(quadrantPos))
						quadTree = m_worldQuadrantsMap[quadrantPos].get();
					m_mtxQuadrantsAndMainProcessing.unlock();

					if (quadTree != nullptr)	// lock quadrant if is a world quadrant: to avoid concurrent acces to disk cache
						while (quadTree->getQuadrant()->lockInternalData() == false)
							Sleep(1);

					MapQuadrantImage* mapQuadrant = nullptr;

					m_mapQuadrantImagesMap.lock();
					if (m_mapQuadrantImagesMap.contains(quadrantPos))
					{
						mapQuadrant = m_mapQuadrantImagesMap[quadrantPos].get();
						while (mapQuadrant->lockInternalData() == false)
							Sleep(1);
					}
					else
					{
						// clear everything
						m_mapQuadrantImagesToDeleteMap[quadrantPos] = true;
						//m_mapQuadrantImagesMap.eraseByRef(quadrantPos);
					}
					m_mapQuadrantImagesMap.unlock();

					if (mapQuadrant != nullptr)
					{
						std::string outMeshIdFromBuffer;
						mapQuadrant->refreshDataFromServer(buffGridVerticesFromServer, meshIdFromServer, outMeshIdFromBuffer, true);
						mapQuadrant->unlockInternalData();
						m_mapQuadrantImagesMap.lock();
						m_mapQuadrantImagesMap.addUpdatedQuadrant(quadrantPos);
						m_mapQuadrantImagesMap.unlock();
					}

#pragma warning(suppress: 26117)
					if (quadTree != nullptr)
						quadTree->getQuadrant()->unlockInternalData();
				}
			}
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADQUADRANTBUFFER)
		{
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl != nullptr && editModeUIControl->initialized())
				editModeUIControl->replyFromServer(reply);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADGENERATIONQUADOVERRIDES)
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

			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

			//clearQuadrantsMaps();	// we almost should clear only current level quadrants
			m_worldQuadrantsMap[quadrantPos] = make_unique<QuadTree>(this, quadrantPos);
			m_worldQuadrantsMap[quadrantPos]->numQuadrantsFromCameraQuadrant(0);
			m_worldQuadrantsMap[quadrantPos]->quadrantDepth(QuadTree::QuadrantDepth::Initial);
			m_worldQuadrantsMap[quadrantPos]->requestDelayedInit(cameraPosX, cameraPosY, cameraPosZ, cameraYaw, cameraPitch, cameraRoll, true, cameraDistanceFromTerrainForced);
			PLOG_INFO << "Requested delayed init for start-up quadrant " << quadrantPos.getIdStr();
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
			m_cameraVerticalOnSeaSet = false;

			Globals()->Client()->MapManagerDeployLevel(isInEditor(), level, numVerticesPerSize, gridStepInWU, setCamera, cameraPosX, cameraPosY, cameraPosZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		}
		else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD)
		{
			GDN_TheWorld_Globals* globals = Globals();

			{
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

				for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
				{
					if (!itQuadTree->second->isValid())
						continue;

					itQuadTree->second->getQuadrant()->getCollider()->exitWorld();
					//itQuadTree->second->getQuadrant()->getCollider()->onGlobalTransformChanged();
				}
			}

			enum class TheWorldStatus status = globals->status();

			while (globals->Client()->getNumCallbacksRunning() > 1)
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
				std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
				clearQuadrantsMaps();
			}

			if (globals->connectedToServer() && m_initialized)
				globals->setStatus(TheWorldStatus::sessionInitialized);
			else if (globals->connectedToServer())
				globals->setStatus(TheWorldStatus::connectedToServer);
			else if (globals->initialized())
				globals->setStatus(TheWorldStatus::initialized);
			else
				globals->setStatus(TheWorldStatus::uninitialized);

			hudMessage("", 0);
			if (m_hudStreamingInfoLabel != nullptr /* and (globals->statusWorldDeployInProgress() or globals->statusMinimalWorldDeployed() or globals->statusWorldDeployed())*/)
			{
				//m_hudStreamingInfoLabel->set_text("");
				m_hudStreamingInfoLabel->call_deferred("set_text", "");
				m_hudMessageLabel->call_deferred("set_text", "");
			}

			shaderParamCameraUnderWaterApplied(false);
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
			const auto _gridStepInWU(std::get_if<float>(&v));
			if (_gridStepInWU == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a float as third input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			float gridStepInWU = *_gridStepInWU;

			v = reply.getInputParam(3);
			std::vector<unsigned char>* _clientQuadrants = std::get_if<std::vector<unsigned char>>(&v);
			if (_clientQuadrants == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as fourth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string clientQuadrants((*_clientQuadrants).begin(), (*_clientQuadrants).end());

			v = reply.getInputParam(5);
			std::vector<unsigned char>* _mapTextureRects = std::get_if<std::vector<unsigned char>>(&v);
			if (_mapTextureRects == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as sisxth input param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string mapTextureRects((*_mapTextureRects).begin(), (*_mapTextureRects).end());

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
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as third return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string replyExtraQuadrants((*_replyExtraQuadrants).begin(), (*_replyExtraQuadrants).end());

			v = reply.getReplyParam(3);
			std::vector<unsigned char>* _replyMapTextureRects = std::get_if<std::vector<unsigned char>>(&v);
			if (_replyMapTextureRects == NULL)
			{
				std::string m = std::string("Reply MapManager::sync did not return a std::vector<unsigned char> as fourth return param");
				throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
			}
			std::string replyMapTextureRects((*_replyMapTextureRects).begin(), (*_replyMapTextureRects).end());

			// *
			// We manage actual world quadrants that need syncronization with server as the server has a newer version
			// *
			{
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
								std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

								if (m_worldQuadrantsMap.contains(pos))
								{
									QuadTree* quadTree = m_worldQuadrantsMap[pos].get();
									if (quadTree->isValid() && quadTree->getQuadrant()->getPos().isInitialized() && quadTree->getQuadrant()->mapModderWorkDone())
									{
										pos.trackQuadrantForDebug("", "Requesting refresh for quadrant");
										quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
										PLOG_VERBOSE << "Requested refresh for quadrant in world quads map " << pos.getIdStr();
									}
								}
							}
						}
					}
				}
			}

			// *
			// We manage actual map texture rects that need syncronization with server as the server has a newer version
			// *
			{
				GDN_TheWorld_Map* map = Map(true, true);
				if (map != nullptr and mapTextureRects.length() > 0 and replyMapTextureRects.length() > 0)
				{
					size_t size = 0;

					size_t mapTextureRectsCursor = 1;	// bypassing first byte '0'
					size_t numMapTextureRects = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)mapTextureRects.c_str() + mapTextureRectsCursor, size);
					mapTextureRectsCursor += size;

					size_t replyMapTextureRectsCursor = 1;	// bypassing first byte '0'
					size_t numMapTextureRectsReplied = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)replyMapTextureRects.c_str() + replyMapTextureRectsCursor, size);
					replyMapTextureRectsCursor += size;

					for (int idx = 0; idx < numMapTextureRectsReplied; idx++)
					{
						size_t mapTextureRectPosSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)mapTextureRects.c_str() + mapTextureRectsCursor, size);
						mapTextureRectsCursor += size;

						TheWorld_Utils::MemoryBuffer tmpBuffer((BYTE*)mapTextureRects.c_str() + mapTextureRectsCursor, mapTextureRectPosSize);
						mapTextureRectsCursor += mapTextureRectPosSize;

						TheWorld_Utils::MapTextureRectPos pos;
						pos.deserialize(tmpBuffer);

						size_t fileTimeSize = TheWorld_Utils::deserializeFromByteStream<size_t>((BYTE*)mapTextureRects.c_str() + mapTextureRectsCursor, size);
						mapTextureRectsCursor += size;

						std::string fileTime(mapTextureRects.c_str() + mapTextureRectsCursor, fileTimeSize);
						mapTextureRectsCursor += fileTimeSize;

						bool refreshRequired = TheWorld_Utils::deserializeFromByteStream<bool>((BYTE*)replyMapTextureRects.c_str() + replyMapTextureRectsCursor, size);
						replyMapTextureRectsCursor += size;

						if (refreshRequired)
						{
							map->addMapTextureRectRefreshScheduled(pos);

							//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
							//bool success = map->destroyTextureRect(pos);
							//success = map->eraseCachedTexture(pos);
							//success = map->eraseMapTextureRect(pos);
						}
					}
				}
			}

			// *
			// We manage server procedural generation status to show it in the edit mode UI
			// *
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
						bool _serverGenerationInProgress = false;
						for (int idx = 0; idx < numProceGenStatus; idx++)
						{
							TheWorld_Utils::MapManagerProceduralGenerationStatus procGenStatus = TheWorld_Utils::deserializeFromByteStream<TheWorld_Utils::MapManagerProceduralGenerationStatus>((BYTE*)replyProceGenStatus.c_str() + proceGenStatusCursor, size);
							proceGenStatusCursor += size;
							if (procGenStatus.m_proceduralProcessingInProgress or procGenStatus.m_numQuadrantsToElabClientSide > 0)
								_serverGenerationInProgress = true;
							if (editModeUIControl != nullptr && editModeUIControl->initialized())
								editModeUIControl->setServerProcGenStatus(procGenStatus);
						}
						serverGenerationInProgress(_serverGenerationInProgress);
					}
					else
					{
						serverGenerationInProgress(false);
						if (editModeUIControl != nullptr && editModeUIControl->initialized())
							editModeUIControl->resetServerProcGenStatus();
					}
				}
				else
				{
					serverGenerationInProgress(false);
					if (editModeUIControl != nullptr && editModeUIControl->initialized())
						editModeUIControl->resetServerProcGenStatus();
				}
			}

			// *
			// We manage extra world quadrants that need syncronization with server as the server has a newer version (this quadrants are modified after inital server load)
			// *
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

						std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
						
						if (m_worldQuadrantsMap.contains(pos))
						{
							// Quadrants needing client work but despite being in world quads map they are up to date (itherwise they would be in replyClientQuadrants list): we force mapModderWork (QuadTree::process_mapModder) /*we need to refresh them*/
							QuadTree* quadTree = m_worldQuadrantsMap[pos].get();
							if (quadTree->isValid() && quadTree->getQuadrant()->getPos().isInitialized() && quadTree->getQuadrant()->mapModderWorkDone())
							{
								//pos.trackQuadrantForDebug("",  "Requesting refresh for quadrant up to date as client work is needed");
								//quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded);
								//PLOG_VERBOSE << "Requested refresh for quadrant in extra quads map and world quads map " << pos.getIdStr();
								pos.trackQuadrantForDebug("", "Requested map modder work for quadrant up to date");	// XXXXX
								quadTree->getQuadrant()->mapModderWorkDone(false);
								PLOG_VERBOSE << "Requested map modder work for quadrant up to date " << pos.getIdStr();
							}
						}
						else
						{
							// Quadrants needing client work but not being in world quads map: we need to create a new copy of them in extra quads map to initialize them let the client work begin (QuadTree::process_mapModder)
							if (!m_extraQuadsMap.contains(pos) && (Globals()->status() == TheWorldStatus::worldDeployed or Globals()->status() == TheWorldStatus::minimalWorldDeployed))
							{
								pos.trackQuadrantForDebug("", "Adding newely created quadrant to extra quads map and requesting delayed init");
								m_extraQuadsMap[pos] = make_unique<QuadTree>(this, pos);	// Quadrant is uninitialized now
								float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
								m_extraQuadsMap[pos]->requestDelayedInit(x, y, z, yaw, pitch, roll, false, 0.0f);	// To initialize in process procedure
								PLOG_VERBOSE << "Requested delayed init for quadrant just added to extra quads map " << pos.getIdStr();
							}
						}
					}
				}
			}

			if (clientQuadrants.length() > 0)
				m_quadrantSyncInProgress = false;
		}
		else
		{
			Globals()->setStatus(TheWorldStatus::error);
			Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::replyFromServer: unknow method ") + method).c_str());
		}
	}
	//catch (GDN_TheWorld_Exception ex)
	//{
	//	Globals()->setStatus(TheWorldStatus::error);
	//	Globals()->errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
	//}
	//catch (...)
	//{
	//	Globals()->setStatus(TheWorldStatus::error);
	//	Globals()->errorPrint("GDN_TheWorld_Viewer::replyFromServer: exception caught");
	//}
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

bool GDN_TheWorld_Viewer::worldStreamingTerminated(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	GDN_TheWorld_Globals* globals = Globals();

	if (globals->loadingScreenExitOnFullWorldDeploy())
	{
		if ((globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed)
			and finishedResizingGrid()
			//and !worldStreamingInProgress()	// *** WARNING ***
			and allVisibleCollidersReady()
			and recalcGridOfQuadrantsInViewNeeded() == false
			and isCameraQuadrantColliderSet()
			and recalcAllVisibleQuadrantsInitialized()	//and getNumInitializedQuadrant2(true) == getNumQuadrant2(true)		// *** WARNING ***
			)
		{
			//godot::Camera3D* activeWorldCamera = getWorldCamera2(true);
			//if (process_updateGridDimension(globals, activeWorldCamera))
			//{
			//	if (recalcGridOfQuadrantsInViewNeeded() == false)
			//		return true;
			//	else
			//		return false;
			//}
			//else
			//	return false;
			return true;
		}
		else
			return false;
	}
	else
	{
		int numEvaluated;
		float numReady;
		if ((globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed)
			//and finishedResizingGrid()
			//and !worldStreamingInProgress()
			//and allCollidersReady()
			and recalcGridOfQuadrantsInViewNeeded() == false
			and isCameraQuadrantColliderSet()
			and (getNumReadyQuadrantsAtDepth(numEvaluated, numReady, QuadTree::QuadrantDepth::Initial) >= getMinNumVisibleQuadrants())	//and (getNumReadyQuadrantsAtDepth(QuadTree::QuadrantDepth::Initial, QuadTree::QuadrantDepth::Cache)) >= getMinNumQuadrants()
			//and getNumInitializedQuadrant2(true) >= getMinNumQuadrant())
			//and getNumInitializedQuadrant2(true) == getNumQuadrant2(true))
			)
		{
			return true;
		}
		else
			return false;
	}

}

bool GDN_TheWorld_Viewer::worldStreamingTerminated(float& loadingProgress)
{
	bool b = worldStreamingTerminated();
	if (b)
		loadingProgress = 100;
	else
		loadingProgress = getLoadingProgress();
	//PLOG_DEBUG << "Terminated: " << (b ? "true" : "false") << " - Loading Progress: " << loadingProgress;
	return b;
}

float GDN_TheWorld_Viewer::getloadingProgressScript(void)
{
	float loadingProgress;
	worldStreamingTerminated(loadingProgress);
	return loadingProgress;
}

QuadTree* GDN_TheWorld_Viewer::getCameraQuadrant(godot::Camera3D* camera)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (camera == nullptr)
		camera = getWorldCamera2(true);
	godot::Vector3 cameraGlobalPos = camera->get_global_position();
	godot::Vector3 cameraWorldPos = globalPosToWorldPos(cameraGlobalPos);
	TheWorld_Utils::QuadrantPos quadrantPos(cameraWorldPos.x, cameraWorldPos.z, m_worldViewerLevel, (int)globals->numVerticesPerQuadrantSize(), globals->gridStepInWU());
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
	if (m_worldQuadrantsMap.contains(quadrantPos))
	{
		return m_worldQuadrantsMap[quadrantPos].get();
	}
	else
		return nullptr;
}

bool GDN_TheWorld_Viewer::isCameraQuadrantColliderSet(void)
{
	bool ret = false;

	//GDN_TheWorld_Globals* globals = Globals();
	//godot::Camera3D* camera = getWorldCamera2(true);
	//godot::Vector3 cameraGlobalPos = camera->get_global_position();
	//godot::Vector3 cameraWorldPos = globalPosToWorldPos(cameraGlobalPos);
	//TheWorld_Utils::QuadrantPos quadrantPos(cameraWorldPos.x, cameraWorldPos.z, m_worldViewerLevel, (int)globals->numVerticesPerQuadrantSize(), globals->gridStepInWU());
	//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
	//if (m_worldQuadrantsMap.contains(quadrantPos))
	//{
	//	QuadTree* quadTree = m_worldQuadrantsMap[quadrantPos].get();
	//	if (quadTree->statusInitialized() and !quadTree->colliderNeedReset(QuadType::NotSet))
	//	{
	//		ret = true;
	//	}
	//}
	
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
	QuadTree* quadTree = getCameraQuadrant();
	if (quadTree!= nullptr and quadTree->statusInitialized() and !quadTree->colliderNeedReset(QuadType::NotSet))
		ret = true;

	return ret;
}

bool GDN_TheWorld_Viewer::setWorldCameraWorldPos(godot::Vector2 worldPos, float altitudeFromTerrain, bool altitudeIsAbsolute, godot::Transform3D worldCameraWorldTransform, bool overSea, bool adjustPositionOnTerrain)
{
	bool ret = false;
	GDN_TheWorld_Globals* globals = Globals();

	if (globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed)
	{
		godot::Vector3 globalPosGroundZero = worldPosToGlobalPos(godot::Vector3(worldPos.x, 0, worldPos.y));
		float cameraAltitude = altitudeFromTerrain;

		// get quadrant containing requested pos
		TheWorld_Utils::QuadrantPos quadrantPos(worldPos.x, worldPos.y, m_worldViewerLevel, (int)globals->numVerticesPerQuadrantSize(), globals->gridStepInWU());
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		if (m_worldQuadrantsMap.contains(quadrantPos))
		{
			QuadTree* quadTree = m_worldQuadrantsMap[quadrantPos].get();
			if (quadTree->statusInitialized() and !quadTree->colliderNeedReset(QuadType::NotSet))
			{
#define MAX_ALTITUDE_DETECTED		20000

				cameraAltitude = -999999999.9f;

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
			}
		}
		else
		{
			cameraAltitude = 0;
		}

		//if (overSea and cameraAltitude <= 0)
		//	cameraAltitude = 30;

		godot::Camera3D* activeWorldCamera = getWorldCamera2(true);
		if (activeWorldCamera != nullptr)
		{
			godot::Transform3D oldCameraT = activeWorldCamera->get_global_transform();
			activeWorldCamera->set_global_transform(worldCameraWorldTransform);
			activeWorldCamera->set_global_position(godot::Vector3(globalPosGroundZero.x, cameraAltitude, globalPosGroundZero.z));
			godot::Transform3D newCameraT = activeWorldCamera->get_global_transform();
			checkNewVisibleQuadrantsRectSize(newCameraT, oldCameraT);
			if (adjustPositionOnTerrain)
			{
				GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
				if (cam != nullptr)
				{
					cam->adjustPositionOnTerrainRequired(true, overSea);
				}
			}
		}
	}

	return ret;
}

//void GDN_TheWorld_Viewer::toggleFullScreen(void)
//{
//}

void GDN_TheWorld_Viewer::selectWorldQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	selectWorldQuadrantRef(pos);
}

void GDN_TheWorld_Viewer::selectWorldQuadrantRef(TheWorld_Utils::QuadrantPos& pos)
{
	doSelectWorldQuadrant(pos);
}

void GDN_TheWorld_Viewer::doSelectWorldQuadrant(TheWorld_Utils::QuadrantPos& pos)
{
	bool sameQuadrant = false;
	if (isWorldQuadrantSelectedForEdit())
	{
		if (m_selectedWorldQuadrantPos == pos)
			sameQuadrant = true;
	}

	if (isWorldQuadrantSelectedForEdit())
		deselectWorldQuadrant();

	if (not sameQuadrant)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(pos);
		if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
		{
			_doSelectWorldQuadrant(pos, it->second.get());
		}
	}
}

void GDN_TheWorld_Viewer::_doSelectWorldQuadrant(TheWorld_Utils::QuadrantPos& pos, QuadTree* quadTreeSel)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	m_selectedWorldQuadrantPos = pos;

	quadTreeSel->setEditModeQuadrantSelected(true);
	quadTreeSel->materialParamsNeedReset(true);
	call_deferred("emit_signal", "tw_quadrant_selected_for_edit", true);

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	//if (editModeUIControl == nullptr || !editModeUIControl->initialized())
	//{
	//	createEditModeUI();
	//	editModeUIControl = EditModeUIControl();
	//	editModeUIControl->set_visible(false);
	//}
	if (editModeUIControl != nullptr and editModeUIControl->initialized())
	{
		editModeUIControl->setMouseQuadSelLabelText(m_selectedWorldQuadrantPos.getName() + " " + m_selectedWorldQuadrantPos.getTag());
		editModeUIControl->setMouseQuadSelPosLabelText(std::string("X=") + std::format("{:.2f}", m_selectedWorldQuadrantPos.getLowerXGridVertex()) + " Z=" + std::format("{:.2f}", m_selectedWorldQuadrantPos.getLowerZGridVertex()) + " " + std::format("{:.2f}", m_selectedWorldQuadrantPos.getSizeInWU()));
		
		TheWorld_Utils::TerrainEdit* terrainEdit = nullptr;
		TheWorld_Utils::TerrainEdit _terrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);

		if (worldMode())
		{
			// in world mode we use current world terrain edit values
			quadTreeSel->getQuadrant()->getTerrainEdit();
		}
		else
		{
			// in map mode we get override terrain values if an override exists
			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				GDN_TheWorld_Map::MapQuadrantOverride* MapQuadrantOverride = map->getMapQuadrantOverride(pos);
				if (MapQuadrantOverride != nullptr and MapQuadrantOverride->getTerrainType() != TheWorld_Utils::TerrainEdit::TerrainType::notset)
				{
					_terrainEdit = MapQuadrantOverride->getTerrainEdit();
					terrainEdit = &_terrainEdit;
				}
				else
				{
					// in map mode we use current world terrain edit values if an override does not exist
					quadTreeSel->getQuadrant()->getTerrainEdit();
				}
			}
			else
			{
				// in map mode we use current world terrain edit values if map does not exist
				quadTreeSel->getQuadrant()->getTerrainEdit();
			}
		}

		if (terrainEdit != nullptr)
		{
			TheWorld_Utils::TerrainEdit* northSideTerrainEdit = nullptr;
			TheWorld_Utils::QuadrantPos pos = m_selectedWorldQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
			QuadTree* q = getQuadTree(pos);
			if (q != nullptr /*&& !q->getQuadrant()->empty()*/)
				northSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
			
			TheWorld_Utils::TerrainEdit* southSideTerrainEdit = nullptr;
			pos = m_selectedWorldQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
			q = getQuadTree(pos);
			if (q != nullptr /* && !q->getQuadrant()->empty()*/)
				southSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
			
			TheWorld_Utils::TerrainEdit* westSideTerrainEdit = nullptr;
			pos = m_selectedWorldQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
			q = getQuadTree(pos);
			if (q != nullptr /* && !q->getQuadrant()->empty()*/)
				westSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
			
			TheWorld_Utils::TerrainEdit* eastSideTerrainEdit = nullptr;
			pos = m_selectedWorldQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
			q = getQuadTree(pos);
			if (q != nullptr /*&& !q->getQuadrant()->empty()*/)
				eastSideTerrainEdit = q->getQuadrant()->getTerrainEdit();
			
			terrainEdit->adjustValues(northSideTerrainEdit, southSideTerrainEdit, westSideTerrainEdit, eastSideTerrainEdit);

			editModeUIControl->setTerrainEditValues(*terrainEdit, quadTreeSel->getQuadrant()->getPos());

			//editModeUIControl->setAllChecked(false);
		}
	}
}

void GDN_TheWorld_Viewer::deselectWorldQuadrant(void)
{
	doDeselectWorldQuadrant();
}

void GDN_TheWorld_Viewer::doDeselectWorldQuadrant(void)
{
	if (m_selectedWorldQuadrantPos.uninitialized())
		return;
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(m_selectedWorldQuadrantPos);
		if (it != m_worldQuadrantsMap.end() && !it->second->statusToErase())
		{
			it->second->setEditModeQuadrantSelected(false);
			it->second->materialParamsNeedReset(true);
			call_deferred("emit_signal", "tw_quadrant_selected_for_edit", false);
		}
	}

	m_selectedWorldQuadrantPos.reset();
	
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl == nullptr || !editModeUIControl->initialized())
	{
		createEditModeUI();
		editModeUIControl = EditModeUIControl();
		editModeUIControl->set_visible(false);
	}
	if (editModeUIControl != nullptr /* && m_editMode*/)
	{
		editModeUIControl->setMouseQuadSelLabelText("");
		editModeUIControl->setMouseQuadSelPosLabelText("");
		//editModeUIControl->setEmptyTerrainEditValues();
		//editModeUIControl->setElapsed(0, false);
	}
}

void GDN_TheWorld_Viewer::toggleWorldQuadrantSelected(void)
{
	if (trackMouse() and editMode())
	{
		m_mouseQuadrantHitName = "";

		if (isWorldQuadrantSelectedForEdit())
		{
			deselectWorldQuadrant();
			deselectMapQuadrant();
		}
		else
		{
			if (mapMode())
			{
				GDN_TheWorld_Map* map = Map();
				godot::Vector2 mousePosInViewport = getMousePosInViewport();;
				godot::Vector2 mouseGlobalPos = map->getMousePosInGlobalCoord(mousePosInViewport);
				godot::Vector2 mouseWorldPos = map->globalPosToWorldPos(mouseGlobalPos);
				TheWorld_Utils::QuadrantPos pos(mouseWorldPos.x, mouseWorldPos.y, m_worldViewerLevel, (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
				selectMapQuadrantRef(pos);
				selectWorldQuadrantRef(pos);
			}
			else
			{
				if (!m_quadrantHitPos.empty())
				{
					selectWorldQuadrantRef(m_quadrantHitPos);
					selectMapQuadrantRef(m_quadrantHitPos);
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

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
	if (eventMouseButton != nullptr)
	{
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_LEFT && eventMouseButton->is_pressed())
		{
			TrackedInputEvent event(this, TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT);
			m_trackedInputEventsQueueForProcess.push_back(event);
			m_trackedInputEventsQueueForTrackMouse.push_back(event);
		}
		if (eventMouseButton->get_button_index() == godot::MouseButton::MOUSE_BUTTON_RIGHT && eventMouseButton->is_pressed())
		{
			TrackedInputEvent event(this, TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT);
			m_trackedInputEventsQueueForProcess.push_back(event);
			m_trackedInputEventsQueueForTrackMouse.push_back(event);
		}
	}

	/* DEBUG */
	//{
	//	bool mouseEvent = false;
	//	const godot::InputEventMouseButton* eventMouseButton = godot::Object::cast_to<godot::InputEventMouseButton>(event.ptr());
	//	if (eventMouseButton != nullptr)
	//	{
	//		mouseEvent = true;
	//	}
	//	const godot::InputEventMouseMotion* eventMouseMotion = godot::Object::cast_to<godot::InputEventMouseMotion>(event.ptr());
	//	if (eventMouseMotion != nullptr)
	//	{
	//		mouseEvent = true;
	//	}
	//	if (!mouseEvent)
	//		mouseEvent = false;
	//	const godot::InputEventKey* eventKey = godot::Object::cast_to<godot::InputEventKey>(event.ptr());
	//	if (eventKey != nullptr)
	//	{
	//		bool b = true;
	//	}
	//}
	/* DEBUG */

	if (worldMode())
	{
		if (event->is_action_pressed("ui_toggle_wireframe"))
		{
			toggleWireframe();
		}

		if (event->is_action_pressed("ui_toggle_debug_visibility"))
		{
			toggleDebugVisibility();
		}

		if (event->is_action_pressed("ui_rotate_chunk_debug_mode"))
		{
			rotateChunkDebugMode();
		}

		if (event->is_action_pressed("ui_refresh"))
		{
			m_refreshRequired = true;
		}
	}
	else
	{
		// mapMode()
	}

	//if (event->is_action_pressed("ui_toggle_fullscreen"))
	//{
	//	toggleFullScreen();
	//}

	if (event->is_action_pressed("ui_select") && m_altPressed)
	{
		toggleWorldQuadrantSelected();
	}

	if (event->is_action_pressed("ui_toggle_track_mouse"))
	{
		toggleTrackMouse();
	}

	if (event->is_action_pressed("ui_toggle_map_grid"))
	{
		toggleMapGrid();
	}

	if (event->is_action_pressed("ui_toggle_edit_mode"))
	{
		toggleEditMode();
	}

	if (event->is_action_pressed("ui_rotate_drawing_mode"))
	{
		rotateDrawingMode();
	}

	if (event->is_action_pressed("ui_dump") && !ctrlPressed())
	{
		m_dumpRequired = true;
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

void GDN_TheWorld_Viewer::adjustWorldCamera(godot::Vector2 desideredCameraWorldPos)
{
	if (not mapMode())
		return;

	GDN_TheWorld_MapViewer* mapViewer = MapViewer();
	if (mapViewer != nullptr)
	{
		Camera2D* mapCamera = MapCamera();
		if (mapCamera != nullptr)
		{
			setWorldCameraWorldPos(desideredCameraWorldPos, 0, true, m_worldCameraWorldTransformForToggleToWorldMode, true);
			delayedWorldMode(true);
			//mapMode(false);
			//mapCamera->set_enabled(false);
			//mapViewer->set_visible(false);
		}
	}
}

void GDN_TheWorld_Viewer::toggleMapView(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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
				//	setWorldCameraWorldPos(globalPosToWorldPos(mapCameraGlobalPos), m_altitudeForToggleToWorldMode, m_altitudeForToggleToWorldModeIsAbsolute, m_worldCameraWorldTransformForToggleToWorldMode);
				//}

				// SWITCHING TO WORLD MODE: world camera will not move (remaining where it was when map mode was activated)
				{
					godot::Vector2 worldPos(m_worldCameraWorldTransformForToggleToWorldMode.origin.x, m_worldCameraWorldTransformForToggleToWorldMode.origin.z);
					setWorldCameraWorldPos(worldPos, m_altitudeForToggleToWorldMode, m_altitudeForToggleToWorldModeIsAbsolute, m_worldCameraWorldTransformForToggleToWorldMode, true, false);
				}

				mapMode(false);
				//mapCamera->set_enabled(false);
				//mapViewer->set_visible(false);
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
							
							mapCameraGlobalPositionSet = false;
							// SWITCHING TO MAP MODE: on first switch map camera will be positioned on world camera position on next switch will stay where it was when map mode was quit
							if (not mapCameraGlobalPositionSet)
							{
								m_mapCameraGlobalPos = worldPosToGlobalPos(godot::Vector2(worldCameraWorldPos.x, worldCameraWorldPos.z));
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
							//mapCamera->set_enabled(true);
							//GDN_TheWorld_MapCamera* map = Object::cast_to<GDN_TheWorld_MapCamera>(mapCamera);
							//if (map != nullptr)
							//	map->resetInputEvents();
							//mapViewer->set_visible(true);
						}
					}
				}
			}

			GDN_TheWorld_Map*map = Map();
			if (map != nullptr)
				map->setWorldFlag("WorldCamera", GDN_TheWorld_Globals::g_color_red, true);
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

bool GDN_TheWorld_Viewer::adjustWorldCameraRequired(void) const
{
	return m_adjustWorldCameraRequired;
}

void GDN_TheWorld_Viewer::adjustWorldCameraRequired(bool b, godot::Vector2 desideredCameraWorldPos)
{
	m_adjustWorldCameraRequired = b;
	m_desideredCameraWorldPos = desideredCameraWorldPos;
}

bool GDN_TheWorld_Viewer::delayedWorldMode(void) const
{
	return m_delayedWorldMode;
}

void GDN_TheWorld_Viewer::delayedWorldMode(bool b)
{
	if (b)
	{
		if (mapMode())
		{
			m_delayedWorldMode = b;
			finishedResizingGrid(false);
			//recalcGridOfQuadrantsInView();
			recalcGridOfQuadrantsInViewNeeded(true);
		}
	}
	else
		m_delayedWorldMode = b;
}

void GDN_TheWorld_Viewer::mapMode(bool b)
{
	//if (m_mapMode != b)
	{
		m_mapMode = b;
		if (b)
		{
			Camera2D* mapCamera = MapCamera();
			if (mapCamera != nullptr)
			{
				mapCamera->set_enabled(true);
				GDN_TheWorld_MapCamera* mapCamera1 = Object::cast_to<GDN_TheWorld_MapCamera>(mapCamera);
				if (mapCamera1 != nullptr)
					mapCamera1->resetInputEvents();
			}
			
			GDN_TheWorld_MapViewer* mapViewer = MapViewer();
			if (mapViewer != nullptr)
			{
				mapViewer->set_visible(true);
			}

			GDN_TheWorld_MapViewportContainer* mapViewportContainer = MapViewportContainer();
			if (mapViewportContainer != nullptr)
			{
				GDN_TheWorld_HudControls* hudControls = HudControls();
				if (hudControls != nullptr)
				{
					Node* parent = hudControls->get_parent();
					if (parent != nullptr)
					{
						parent->remove_child(hudControls);
						mapViewportContainer->add_child(hudControls);
					}
					else
					{
						mapViewportContainer->add_child(hudControls);
					}
					hudControls->hideLoadingBar();
					//godot::Size2 windowSize = map->get_viewport()->get_visible_rect().size;
					//setPosHud(windowSize);
				}
			}

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
				map->enterMapMode();
			
			//if (m_postProcessingUnderWaterEffectNode != nullptr)
			//	m_postProcessingUnderWaterEffectNode->set_visible(false);
			if (m_postProcessingUnderWaterEffectMesh != nullptr)
				m_postProcessingUnderWaterEffectMesh->set_visible(false);
		}
		else
		{
			GDN_TheWorld_MapViewer* mapViewer = MapViewer();
			if (mapViewer != nullptr)
				mapViewer->set_visible(false);
			
			Camera2D* mapCamera = MapCamera();
			if (mapCamera != nullptr)
				mapCamera->set_enabled(false);

			GDN_TheWorld_HudControls* hudControls = HudControls();
			if (hudControls != nullptr)
			{
				Node* parent = hudControls->get_parent();
				if (parent != nullptr)
				{
					parent->remove_child(hudControls);
					add_child(hudControls);
				}
				else
				{
					add_child(hudControls);
				}
				hudControls->hideLoadingBar();
				//godot::Size2 windowSize = getViewportSize();
				//setPosHud(windowSize);
			}

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
				map->exitMapMode();

			//if (m_postProcessingUnderWaterEffectNode != nullptr)
			//	m_postProcessingUnderWaterEffectNode->set_visible(true);
			if (m_postProcessingUnderWaterEffectMesh != nullptr)
				m_postProcessingUnderWaterEffectMesh->set_visible(true);

		}

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr && editModeUIControl->initialized())
			editModeUIControl->mapMode(b);
	}
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
		m_previousTrackedMouseHitWorldPos = godot::Vector3();

	if (trackMouse())
		m_mouseQuadrantHitName = "";
}

void GDN_TheWorld_Viewer::toggleMapGrid(void)
{
	if (!m_editMode)
	{
		if (mapGridOn())
			mapGridOn(false);
		else
			mapGridOn(true);
	}
}

bool GDN_TheWorld_Viewer::mapGridOn(void)
{
	//return m_mapGridOn;

	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
		return map->mapGridOn();
	else
		return false;
}

void GDN_TheWorld_Viewer::mapGridOn(bool b)
{
	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
		map->mapGridOn(b);

	//m_mapGridOn = b;
	//if (m_mapGridOn)
	//{
	//	GDN_TheWorld_Map* map = Map();
	//	if (map != nullptr)
	//		map->mapGridOn(true);
	//}
	//else
	//{
	//	GDN_TheWorld_Map* map = Map();
	//	if (map != nullptr)
	//		map->mapGridOn(false);
	//}
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
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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
		m_previousTrackedMouseHitWorldPos = godot::Vector3();
	}
}

void GDN_TheWorld_Viewer::toggleEditMode(void)
{
	m_previousTrackedMouseHitWorldPos = godot::Vector3();
	m_editMode = !m_editMode;
	if (m_editMode)
	{
		trackMouse(true);
		mapGridOn(true);
	}
	else
	{
		if (!showPositionOnCursorHit() && !normalVisible() && !gizmoVisible() && !highlightHitQuadrant())
			trackMouse(false);
		mapGridOn(false);
		//gridOn(false);
	}
}

void GDN_TheWorld_Viewer::setEditMode(bool editMode)
{
	if (m_editMode != editMode)
	{
		m_previousTrackedMouseHitWorldPos = godot::Vector3();
		m_editMode = editMode;
		if (m_editMode)
		{
			trackMouse(true);
			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				map->mapGridOn(true);
			}
		}
		else
		{
			if (!showPositionOnCursorHit() && !normalVisible() && !gizmoVisible() && !highlightHitQuadrant())
				trackMouse(false);
			//gridOn(false);
			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				map->mapGridOn(false);
			}
		}
	}
}

bool GDN_TheWorld_Viewer::isEditMode(void) const
{
	return m_editMode;
}

void GDN_TheWorld_Viewer::setInfoPanelVisible(bool b)
{
	m_infoPanelVisible = b;
}

void GDN_TheWorld_Viewer::toggleDebugVisibility(void)
{
	m_debugContentVisibility = !m_debugContentVisibility;
	m_updateTerrainVisibilityRequired = true;
	forceProcessWorldCameraPosForChunks();
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
	Globals()->print("", false);
	Globals()->print("===========================================================================", false);
	Globals()->print("", false);
	Globals()->print("KEYBOARD MAPPING", false);
	Globals()->print("", false);
	Globals()->print("      TOGGLE MAP VIEW                 ==> M", false);
	Globals()->print("      DROP PLAYER                     ==> P", false);
	Globals()->print("      CTRL-1/0                        ==> SAVE VIEWPOINT 1/0", false);
	Globals()->print("      ALT-1/0                         ==> LOAD VIEWPOINT 1/0", false);
	Globals()->print("      TOGGLE DEBUG STATS              ==> CTRL-ALT-D", false);
	Globals()->print("      TOGGLE EDIT                     ==> ALT-E", false);
	Globals()->print("      TOGGLE TRACK MOUSE              ==> ALT-T", false);
	Globals()->print("      DUMP                            ==> ALT-D", false);
	Globals()->print("      TOGGLE FULL SCREEN              ==> ALT-ENTER", false);
	Globals()->print("      PRINT SCREEN                    ==> ALT-C", false);
	Globals()->print("", false);
	Globals()->print("      TOGGLE DEBUG VISIBILITY         ==> F1", false);
	Globals()->print("      ROTATE CHUNK DEBUG MODE         ==> F2", false);
	Globals()->print("      ROTATE DRAWING MODE             ==> F3", false);
	Globals()->print("      REFRESH VIEW                    ==> F4", false);
	Globals()->print("      TOGGLE WIREFRAME                ==> F5", false);
	Globals()->print("", false);
	Globals()->print("      EXIT                            ==> ESC", false);
	Globals()->print("", false);
	Globals()->print("   WORLD MODE", false);
	Globals()->print("", false);
	Globals()->print("      LEFT                            ==> A   MOUSE BUTTON LEFT", false);
	Globals()->print("      RIGHT                           ==> D   MOUSE BUTTON LEFT", false);
	Globals()->print("      FORWARD                         ==> W   MOUSE BUTTON MID / MOUSE WHEEL UP (WITH ALT MOVE ON MAP, WITHOUT MOVE FORWARD IN THE DIRECTION OF CAMERA)", false);
	Globals()->print("      BACKWARD                        ==> S   MOUSE BUTTON MID / MOUSE WHEEL DOWN (WITH ALT MOVE ON MAP, WITHOUT MOVE BACKWARD IN THE DIRECTION OF CAMERA)", false);
	Globals()->print("      UP                              ==> Q", false);
	Globals()->print("      DOWN                            ==> E", false);
	Globals()->print("", false);
	Globals()->print("      SHIFT                           ==> LOW MOVEMENT", false);
	Globals()->print("      CTRL                            ==> FAST MOVEMENT", false);
	Globals()->print("", false);
	Globals()->print("      ROTATE CAMERA                   ==> MOUSE BUTTON RIGHT", false);
	Globals()->print("      SHIFT ORI CAMERA                ==> MOUSE BUTTON LEFT", false);
	Globals()->print("      SHIFT VERT CAMERA               ==> MOUSE BUTTON MID", false);
	Globals()->print("", false);
	Globals()->print("      TOGGLE TRACK PLAYER             ==> SHIFT-F", false);
	Globals()->print("      TOGGLE BACK MIRROR              ==> B", false);
	Globals()->print("      TOGGLE SATELLITE VIEW           ==> Y", false);
	Globals()->print("", false);
	Globals()->print("      EDIT MODE ENABLED", false);
	Globals()->print("", false);
	Globals()->print("         SAVE                         ==> PERSIST LOCALLY ALL EDITED QUADRANTS", false);
	Globals()->print("         UPLOAD                       ==> UPLOAD (UPLOAD IMPLIES AUTOMATIC SAVE) ALL EDITED QUADRANTS", false);
	Globals()->print("                                          UPLOAD OPERATION START AN AUTOMATIC REFRESH OF THE MAP", false);
	Globals()->print("         RELOAD SHADER                ==> RELOAD THE SHADER OF THE SELECTED QUADRANT", false);
	Globals()->print("         RESET NORM/TEX               ==> RESET THE BORMALS AND THE TEXTURE OF THE SELECTED QUADRANT FORCING A REGENERATION AND A REPAINT", false);
	Globals()->print("", false);
	Globals()->print("", false);
	Globals()->print("         SELECT/DESELECT QUADRANT            ==> ALT-ENTER (WARNING: IT IS MIRRORED IN MAP MODE AS FOR \"MOUSE DOUBLE LEFT CLICK\")", false);
	Globals()->print("", false);
	Globals()->print("         SHOW POSITION SELECTED", false);
	Globals()->print("", false);
	Globals()->print("            SET INITIAL POS FOR DISTANCE CALC   ==> MOUSE DOUBLE BUTTON LEFT", false);
	Globals()->print("            CANCEL DISTANCE CALC                ==> MOUSE DOUBLE BUTTON RIGHT", false);
	Globals()->print("", false);
	Globals()->print("         HIGHLIGHT HIT QUAD                     ==> CHECK BOX IN EDIT MODE: HIGHLIGHT THE QUAD HIT BY THE MOUSE", false);
	Globals()->print("", false);
	Globals()->print("         SHOW CHUNKS                            ==> CHECK BOX IN EDIT MODE: SHOW CHUNK COLORED ACCORDING TO LOD: FROM MAXIMUM DETAIL OF THE MESH TO MINIMUM", false);
	Globals()->print("                                                        YELLOW APPRICOT", false);
	Globals()->print("                                                        AQUAMARINE GREEN", false);
	Globals()->print("                                                        GREEN", false);
	Globals()->print("                                                        CYAN", false);
	Globals()->print("                                                        RED SALMON", false);
	Globals()->print("                                                        BLUE", false);
	Globals()->print("                                                        PINK AMARANTH", false);
	Globals()->print("                                                        DARKSEA GREEN", false);
	Globals()->print("                                                        RED", false);
	Globals()->print("", false);
	Globals()->print("         SHOW WIREFRAME                         ==> CHECK BOX IN EDIT MODE: QUADRANT BORDERS ARE BLACK BOLDED, LINES CROSS POINTS OF THE REAL GRID AT CURRENT LEVEL OF OPTIMIZATION", false);
	Globals()->print("", false);
	Globals()->print("         GRID ON                                ==> CHECK BOX IN EDIT MODE: DRAW A GRID FOR EVERY VIRTUAL POINT OF HE GRID NOT TAKING INTO ACCOUNT OPTIMIZATION LEVEL", false);
	Globals()->print("", false);
	Globals()->print("         CAST RAY                               ==> CHECK BOX IN EDIT MODE: CAST A RAY FROM THE CAMERA TO THE POINT IN WHICH THE MOUSE COLLIDES WITH THE TERRAIN", false);
	Globals()->print("", false);
	Globals()->print("         TERRAIN GENERATION:", false);
	Globals()->print("            1) select a terrain quadrant moving the mouse over the terrain anc click \"Alt-Enter\"", false);
	Globals()->print("            2) in the section \"TERRAIN GENERATION\" select the terrain type: the generation start instantly and the blending is automatic for all the generated quadrants. If the ctrl is pressed when the selection", false);
	Globals()->print("               is made the blending is suspended and can be made successively usign the \"BLEND\" button in the \"TERRAIN BLENDING\" section. Eventually the parameters in both sections \"TERRAIN GENERATION\" and \"TERRAIN BLENDING\"", false);
	Globals()->print("               can be modified from the current values (last used from previous generation of the quadrant or default values if new/empty quadrant) to generate/blend accordingly", false);
	Globals()->print("", false);
	Globals()->print("         BLENDING:", false);
	Globals()->print("            - you can do a check to see which quadrants need blending using the relative check box in the \"TERRAIN BLENDING\" section: the red lines show the unblended sides", false);
	Globals()->print("            - you can blen all the quadrants which have been newly generated plus the quadrants needing blending (if you highlight them with the check box \"Check Blend\") using the \"BLEND\" button", false);
	Globals()->print("", false);
	Globals()->print("   MAP MODE", false);
	Globals()->print("", false);
	Globals()->print("      WARNING: THE MAP IS AUTOMATICALLY REFRESHED WHEN ALL EDITED QUADRANTS ARE UPLOADED", false);
	Globals()->print("", false);
	Globals()->print("      LEFT                            ==> A", false);
	Globals()->print("      RIGHT                           ==> D", false);
	Globals()->print("      FORWARD                         ==> W", false);
	Globals()->print("      BACKWARD                        ==> S", false);
	Globals()->print("", false);
	Globals()->print("      CTRL                            ==> FAST MOVEMENT", false);
	Globals()->print("", false);
	Globals()->print("      MOUSE BUTTON LEFT               ==> DRAG MAP", false);
	Globals()->print("", false);
	Globals()->print("      TOGGLE MAP GRID                 ==> ALT-G", false);
	Globals()->print("", false);
	Globals()->print("      EDIT MODE DISABLED ", false);
	Globals()->print("         MOUSE DOUBLE LEFT CLICK      ==> move camera on selected point (same direction) and switch to world mode", false);
	Globals()->print("", false);
	Globals()->print("      EDIT MODE ENABLED", false);
	Globals()->print("", false);
	Globals()->print("         MOUSE DOUBLE LEFT CLICK      ==> if the pointed quadrant is already selected deselect it saving parameters from editor panel otherwise select pointed quadrant and activate the editing ", false);
	Globals()->print("                                          of that quadrant setting editor panel parameter with its actual values, if it is not actually in world map editor panel parameters are used", false);
	Globals()->print("         MOUSE DOUBLE RIGHT CLICK     ==> if a quadrant is actually selected deselect it without modifying its parameters regardless which quadrant is pointed", false);
	Globals()->print("         MOUSE LEFT CLICK             ==> if exist an override for pointed quadrant its parameters are displayed in editor panel", false);
	Globals()->print("         MOUSE RIGHT CLICK            ==> if \"quadrant editing\" is active and no quadrant is selected do next things:", false);
	Globals()->print("                                          if the pointed quadrant has an override remove it", false);
	Globals()->print("                                          if the pointed quadrant does not have an override insert one using editor panel terrain type selected", false);
	Globals()->print("         \"UPLOAD BUTTON\"              ==> upload edited quadrant and start server generation of terrain data then generated datas are streamed to the client\"", false);
	Globals()->print("", false);
	Globals()->print("===========================================================================", false);
	Globals()->print("", false);
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
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
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
			//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
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
			std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
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
		m_lastProcessedWorldCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedWorldCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			recalcGridOfQuadrantsInViewNeeded(true);
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
		m_forceUpdateTerrainMaterial = true;

		m_cacheQuadOnPerimeter = cache;
		m_lastProcessedWorldCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedWorldCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			recalcGridOfQuadrantsInViewNeeded(true);
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
		m_forceUpdateTerrainMaterial = true;

		m_maxDepthQuadOnPerimeter = depth;
		if (m_maxDepthQuadOnPerimeter != -1 and m_maxDepthQuadOnPerimeter < getMinDepthQuadOnPerimeter())
			setMinDepthQuadOnPerimeter(m_maxDepthQuadOnPerimeter);
		m_lastProcessedWorldCameraPosForQuadrants = godot::Vector3();
		m_lastProcessedWorldCameraPosForChunks = godot::Vector3();
		if ((int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
			recalcGridOfQuadrantsInViewNeeded(true);
		if(m_maxDepthQuadOnPerimeter >= 0)
			adjustEnvironmentRequired(true);
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

godot::Rect2 GDN_TheWorld_Viewer::getVisibleQuadrantsRect(TheWorld_Utils::QuadrantPos& cameraQuadrantPos, bool& valid)
{
	godot::Rect2 visibleQuadrantsRect;
	valid = false;

	float quadrantsSizeInWUs = Globals()->heightmapSizeInWUs();

	if (!cameraQuadrantPos.isInitialized())
		return visibleQuadrantsRect;

	if (m_numVisibleQuadrantOnPerimeterOnWestSide == -1 or m_numVisibleQuadrantOnPerimeterOnEastSide == -1 or m_numVisibleQuadrantOnPerimeterOnNorthSide == -1 or m_numVisibleQuadrantOnPerimeterOnSouthSide == -1)
		return visibleQuadrantsRect;

	valid = true;

	visibleQuadrantsRect = godot::Rect2(
		cameraQuadrantPos.getLowerXGridVertex() - m_numVisibleQuadrantOnPerimeterOnWestSide * quadrantsSizeInWUs,
		cameraQuadrantPos.getLowerZGridVertex() - m_numVisibleQuadrantOnPerimeterOnNorthSide * quadrantsSizeInWUs,
		(m_numVisibleQuadrantOnPerimeterOnWestSide + m_numVisibleQuadrantOnPerimeterOnEastSide + 1) * quadrantsSizeInWUs,
		(m_numVisibleQuadrantOnPerimeterOnNorthSide + m_numVisibleQuadrantOnPerimeterOnSouthSide + 1) * quadrantsSizeInWUs
	);
	
	return visibleQuadrantsRect;
}

void GDN_TheWorld_Viewer::checkNewVisibleQuadrantsRectSize(godot::Transform3D newCameraT, godot::Transform3D oldCameraT)
{
	godot::Vector3 newPos = newCameraT.origin;
	godot::Vector3 oldPos = oldCameraT.origin;
	float quadrantsSizeInWUs = Globals()->heightmapSizeInWUs();
	TheWorld_Utils::QuadrantPos oldCameraQuadrantPos(oldPos.x, oldPos.z, m_worldViewerLevel, m_numWorldVerticesPerSize, Globals()->gridStepInWU());
	TheWorld_Utils::QuadrantPos newCameraQuadrantPos(newPos.x, newPos.z, m_worldViewerLevel, m_numWorldVerticesPerSize, Globals()->gridStepInWU());

	if (newCameraQuadrantPos != oldCameraQuadrantPos)	// if oldPos and newPos are in the same quadrant I don't need to modify visible quadrant size ...
	{
		// ... else
		bool visibleQuadrantsRectValid = false;
		godot::Rect2 oldPosVisibleQuadrantsRect = getVisibleQuadrantsRect(oldCameraQuadrantPos, visibleQuadrantsRectValid);
		if (visibleQuadrantsRectValid)
		{
			godot::Rect2 newPosVisibleQuadrantsRect = getVisibleQuadrantsRect(newCameraQuadrantPos, visibleQuadrantsRectValid);
			if (visibleQuadrantsRectValid)
			{
				if (newPosVisibleQuadrantsRect.intersects(oldPosVisibleQuadrantsRect))
				{
					godot::Rect2 intersection = newPosVisibleQuadrantsRect.intersection(oldPosVisibleQuadrantsRect);
					if (not intersection.has_point(godot::Vector2(newPos.x, newPos.z)))	// if new and old visible quadrant intersect but newPos is contained in intersection better to not modify new visible quadrant size
																						// because we surely have many shared quadrants and we don't want to re-acquire them from server ...
					{
						// else ...
						// Check East Side
						if (newPos.x > intersection.get_end().x)
							m_numVisibleQuadrantOnPerimeterOnEastSide = -1;		// we force recalculation of visible quadrants in East direction
						// Check North Side
						if (newPos.z < intersection.get_position().y)
							m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;	// we force recalculation of visible quadrants in North direction
						// Check West Side
						if (newPos.x < intersection.get_position().x)
							m_numVisibleQuadrantOnPerimeterOnWestSide = -1;		// we force recalculation of visible quadrants in West direction
						// Check South Side
						if (newPos.z > intersection.get_end().y)
							m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;	// we force recalculation of visible quadrants in South direction
					}
				}
				else
				{
					// if new and old visible quadrant don't intersect we can't have shared quadrants so it is better to re-calculate visible quadrant size and re-acquire from server all quadrants
					m_numVisibleQuadrantOnPerimeterOnWestSide = -1;		// we force recalculation of visible quadrants in West direction
					m_numVisibleQuadrantOnPerimeterOnEastSide = -1;		// we force recalculation of visible quadrants in East direction
					m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;	// we force recalculation of visible quadrants in North direction
					m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;	// we force recalculation of visible quadrants in South direction
				}
			}
		}
		
		
		
		//float roundedNumQuadrantsBetweenPositionsOnWestEastLine = floor(abs(newPos.x - oldPos.x) / quadrantsSizeInWUs) + 2;
		//float roundedNumQuadrantsBetweenPositionsOnNorthSouthLine = floor(abs(newPos.z - oldPos.z) / quadrantsSizeInWUs) + 2;
		//bool visibleQuadrantsRectValid = false;
		//godot::Rect2 visibleQuadrantsRect = getVisibleQuadrantsRect(oldCameraQuadrantPos, visibleQuadrantsRectValid);
		//if (visibleQuadrantsRectValid)
		//{
		//	if (newPos.x >= oldPos.x)		// new position on east side of old position
		//	{
		//		if (newPos.z >= oldPos.z)
		//		{
		//			// new position on south-east quadrant respect to old position

		//			if (m_numVisibleQuadrantOnPerimeterOnWestSide < roundedNumQuadrantsBetweenPositionsOnWestEastLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnWestSide = -1;
		//			if (m_numVisibleQuadrantOnPerimeterOnNorthSide < roundedNumQuadrantsBetweenPositionsOnNorthSouthLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;
		//			if (newPos.x > (oldCameraQuadrantPos.getIndexPosX() + m_numVisibleQuadrantOnPerimeterOnEastSide * quadrantsSizeInWUs))		// if 
		//				m_numVisibleQuadrantOnPerimeterOnEastSide = -1;
		//			if (newPos.z > (oldCameraQuadrantPos.getIndexPosZ() + m_numVisibleQuadrantOnPerimeterOnSouthSide * quadrantsSizeInWUs))
		//				m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;
		//		}
		//		else
		//		{
		//			// new position on north-east quadrant respect to old position

		//			if (m_numVisibleQuadrantOnPerimeterOnWestSide < roundedNumQuadrantsBetweenPositionsOnWestEastLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnWestSide = -1;
		//			if (m_numVisibleQuadrantOnPerimeterOnSouthSide < roundedNumQuadrantsBetweenPositionsOnNorthSouthLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnEastSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;
		//		}
		//	}
		//	else	// new position on west side of old position
		//	{
		//		if (newPos.z >= oldPos.z)
		//		{
		//			// new position on south-west quadrant respect to old position

		//			if (m_numVisibleQuadrantOnPerimeterOnEastSide < roundedNumQuadrantsBetweenPositionsOnWestEastLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnEastSide = -1;
		//			if (m_numVisibleQuadrantOnPerimeterOnNorthSide < roundedNumQuadrantsBetweenPositionsOnNorthSouthLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnWestSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;
		//		}
		//		else
		//		{
		//			// new position on north-west quadrant respect to old position

		//			if (m_numVisibleQuadrantOnPerimeterOnEastSide < roundedNumQuadrantsBetweenPositionsOnWestEastLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnEastSide = -1;
		//			if (m_numVisibleQuadrantOnPerimeterOnSouthSide < roundedNumQuadrantsBetweenPositionsOnNorthSouthLine / 2)
		//				m_numVisibleQuadrantOnPerimeterOnSouthSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnWestSide = -1;
		//			m_numVisibleQuadrantOnPerimeterOnNorthSide = -1;
		//		}
		//	}
		//}
	}
	
	recalcGridOfQuadrantsInViewNeeded(true);
	finishedResizingGrid(false);
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

size_t GDN_TheWorld_Viewer::numQuadrantsPerGridSize(void) const
{
	return (getMaxDepthQuadOnPerimeter() + getCacheQuadOnPerimeter()) * 2 + 1;	// +1 because we count also the central quadrant where the camera is
}

float GDN_TheWorld_Viewer::getMaxGridSizeInWUs(void)
{
	return numQuadrantsPerGridSize() * Globals()->heightmapSizeInWUs();
}

godot::Vector2 GDN_TheWorld_Viewer::getGridPosInWUs()
{
	size_t numQuadrantsToBorderOfGrid = (numQuadrantsPerGridSize() - 1) / 2;	// -1 because we don't count the central quadrant where the camera is
	return godot::Vector2(
		m_gridWorldCameraQuadrantPos.getLowerXGridVertex() - numQuadrantsToBorderOfGrid * Globals()->heightmapSizeInWUs(),
		m_gridWorldCameraQuadrantPos.getLowerZGridVertex() - numQuadrantsToBorderOfGrid * Globals()->heightmapSizeInWUs()
	);
}

bool GDN_TheWorld_Viewer::recalcGridOfQuadrantsInView(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants");

	bool newQuadAdded = false;
	
	PLOG_DEBUG << "Recalc quadrants in view - World camera quadrant pos: x=" << m_computedWorldCameraQuadrantPos.getLowerXGridVertex() << " z=" << m_computedWorldCameraQuadrantPos.getLowerZGridVertex() << " (" << m_computedWorldCameraQuadrantPos.getIndexPosX()  << "," << m_computedWorldCameraQuadrantPos.getIndexPosZ() << ")";
	
	m_gridWorldCameraQuadrantPos = m_computedWorldCameraQuadrantPos;
	if (!m_gridWorldCameraQuadrantPos.isInitialized())
		return newQuadAdded;
	
	vector<TheWorld_Utils::QuadrantPos> quadrantPosNeeded;
	quadrantPosNeeded.push_back(m_gridWorldCameraQuadrantPos);
	vector<enum class QuadTree::QuadrantDepth> quadrantDepth;
	quadrantDepth.push_back(QuadTree::QuadrantDepth::Initial);
	vector<size_t> numQuadrantsFromCameraQuadrant;
	numQuadrantsFromCameraQuadrant.push_back(0);

	m_minVisibleQuadrantOnPerimeter = getMinDepthQuadOnPerimeter();

	if (m_numCacheQuadrantOnPerimeter != getCacheQuadOnPerimeter())
	{
		m_numCacheQuadrantOnPerimeter = getCacheQuadOnPerimeter();
		worldGridDimensionChanged(true);
	}

	if (m_numVisibleQuadrantOnPerimeterOnNorthSide == -1)
	{
		m_numVisibleQuadrantOnPerimeterOnNorthSide = m_minVisibleQuadrantOnPerimeter;
		worldGridDimensionChanged(true);
	}
	else 
	{
		if (m_numVisibleQuadrantOnPerimeterOnNorthSide < m_minVisibleQuadrantOnPerimeter)
		{
			m_numVisibleQuadrantOnPerimeterOnNorthSide = m_minVisibleQuadrantOnPerimeter;
			worldGridDimensionChanged(true);
		}
		if (m_numVisibleQuadrantOnPerimeterOnNorthSide > getMaxDepthQuadOnPerimeter())
		{
			m_numVisibleQuadrantOnPerimeterOnNorthSide = getMaxDepthQuadOnPerimeter();
			worldGridDimensionChanged(true);
		}
	}
	if (m_numVisibleQuadrantOnPerimeterOnSouthSide == -1)
	{
		m_numVisibleQuadrantOnPerimeterOnSouthSide = m_minVisibleQuadrantOnPerimeter;
		worldGridDimensionChanged(true);
	}
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnSouthSide < m_minVisibleQuadrantOnPerimeter)
		{
			m_numVisibleQuadrantOnPerimeterOnSouthSide = m_minVisibleQuadrantOnPerimeter;
			worldGridDimensionChanged(true);
		}
		if (m_numVisibleQuadrantOnPerimeterOnSouthSide > getMaxDepthQuadOnPerimeter())
		{
			m_numVisibleQuadrantOnPerimeterOnSouthSide = getMaxDepthQuadOnPerimeter();
			worldGridDimensionChanged(true);
		}
	}
	if (m_numVisibleQuadrantOnPerimeterOnWestSide == -1)
	{
		m_numVisibleQuadrantOnPerimeterOnWestSide = m_minVisibleQuadrantOnPerimeter;
		worldGridDimensionChanged(true);
	}
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnWestSide < m_minVisibleQuadrantOnPerimeter)
		{
			m_numVisibleQuadrantOnPerimeterOnWestSide = m_minVisibleQuadrantOnPerimeter;
			worldGridDimensionChanged(true);
		}
		if (m_numVisibleQuadrantOnPerimeterOnWestSide > getMaxDepthQuadOnPerimeter())
		{
			m_numVisibleQuadrantOnPerimeterOnWestSide = getMaxDepthQuadOnPerimeter();
			worldGridDimensionChanged(true);
		}
	}
	if (m_numVisibleQuadrantOnPerimeterOnEastSide == -1)
	{
		m_numVisibleQuadrantOnPerimeterOnEastSide = m_minVisibleQuadrantOnPerimeter;
		worldGridDimensionChanged(true);
	}
	else
	{
		if (m_numVisibleQuadrantOnPerimeterOnEastSide < m_minVisibleQuadrantOnPerimeter)
		{
			m_numVisibleQuadrantOnPerimeterOnEastSide = m_minVisibleQuadrantOnPerimeter;
			worldGridDimensionChanged(true);
		}
		if (m_numVisibleQuadrantOnPerimeterOnEastSide > getMaxDepthQuadOnPerimeter())
		{
			m_numVisibleQuadrantOnPerimeterOnEastSide = getMaxDepthQuadOnPerimeter();
			worldGridDimensionChanged(true);
		}
	}

	// add horizontal (X axis) quadrants on the left
	for (int i = 0; i < m_numVisibleQuadrantOnPerimeterOnWestSide + m_numCacheQuadrantOnPerimeter; i++)
	{
		TheWorld_Utils::QuadrantPos q = m_gridWorldCameraQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus, 1 + i);
		q.setTag(m_gridWorldCameraQuadrantPos.getTag() + " X-" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
		if (i < m_minVisibleQuadrantOnPerimeter)
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Initial);
		}
		else if (i < m_numVisibleQuadrantOnPerimeterOnWestSide)
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Growing);
		}
		else
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Cache);
		}
		numQuadrantsFromCameraQuadrant.push_back(static_cast<size_t>(i) + 1);
		//break;	// SUPERDEBUGRIC only X-1 quadrant
	}

	// add horizontal (X axis) quadrants on the right
	for (int i = 0; i < m_numVisibleQuadrantOnPerimeterOnEastSide + m_numCacheQuadrantOnPerimeter; i++)
	{
		TheWorld_Utils::QuadrantPos q = m_gridWorldCameraQuadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus, 1 + i);
		q.setTag(m_gridWorldCameraQuadrantPos.getTag() + " X+" + std::to_string(1 + i));
		quadrantPosNeeded.push_back(q);
		if (i < m_minVisibleQuadrantOnPerimeter)
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Initial);
		}
		else if (i < m_numVisibleQuadrantOnPerimeterOnEastSide)
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Growing);
		}
		else
		{
			quadrantDepth.push_back(QuadTree::QuadrantDepth::Cache);
		}
		numQuadrantsFromCameraQuadrant.push_back(static_cast<size_t>(i) + 1);
		//break;	// SUPERDEBUGRIC only X+1 quadrant
	}

	// for each horizontal quadrant ...
	size_t size = quadrantPosNeeded.size();
	for (size_t idx = 0; idx < size; idx++)
	{
		// first is central quadrant
		// following there are (((size - 1) / 2) - m_numCacheQuadrantOnPerimeter) visible quadrant on left side
		// following there are m_numCacheQuadrantOnPerimeter cache quadrant on left size
		// following there are (((size - 1) / 2) - m_numCacheQuadrantOnPerimeter) visible quadrant on right side
		// finally there are m_numCacheQuadrantOnPerimeter cache quadrant on right size
		
		int horizontalDelimiter;
		if (idx <= m_numVisibleQuadrantOnPerimeterOnWestSide + m_numCacheQuadrantOnPerimeter)
			horizontalDelimiter = m_numVisibleQuadrantOnPerimeterOnWestSide;	// because after camera idx (=0) there are quadrants on left (west) side
		else
			horizontalDelimiter = m_numVisibleQuadrantOnPerimeterOnEastSide;	// then there are quadrants on right (east) side

		// ... add vertical (Z axis) quadrants up
		for (size_t i = 0; i < m_numVisibleQuadrantOnPerimeterOnNorthSide + m_numCacheQuadrantOnPerimeter; i++)
		{
			TheWorld_Utils::QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z-" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
			if (numQuadrantsFromCameraQuadrant[idx] <= m_minVisibleQuadrantOnPerimeter and i < m_minVisibleQuadrantOnPerimeter)
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Initial);
			}
			else if (numQuadrantsFromCameraQuadrant[idx] <= horizontalDelimiter and i < m_numVisibleQuadrantOnPerimeterOnNorthSide)
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Growing);
			}
			else
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Cache);
			}
			numQuadrantsFromCameraQuadrant.push_back(TheWorld_Utils::Utils::max2(numQuadrantsFromCameraQuadrant[idx], i + 1));
			//break;	// SUPERDEBUGRIC only Z-1 quadrant
		}

		// ... add vertical (Z axis) quadrants down
		for (size_t i = 0; i < m_numVisibleQuadrantOnPerimeterOnSouthSide + m_numCacheQuadrantOnPerimeter; i++)
		{
			TheWorld_Utils::QuadrantPos q = quadrantPosNeeded[idx].getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus, 1 + int(i));
			q.setTag(quadrantPosNeeded[idx].getTag() + " Z+" + std::to_string(1 + i));
			quadrantPosNeeded.push_back(q);
			if (numQuadrantsFromCameraQuadrant[idx] <= m_minVisibleQuadrantOnPerimeter and i < m_minVisibleQuadrantOnPerimeter)
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Initial);
			}
			else if (numQuadrantsFromCameraQuadrant[idx] <= horizontalDelimiter and i < m_numVisibleQuadrantOnPerimeterOnSouthSide)
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Growing);
			}
			else
			{
				quadrantDepth.push_back(QuadTree::QuadrantDepth::Cache);
			}
			numQuadrantsFromCameraQuadrant.push_back(TheWorld_Utils::Utils::max2(numQuadrantsFromCameraQuadrant[idx], i + 1));
			//break;	// SUPERDEBUGRIC only Z+1 quadrant
		}
	}

	vector<TheWorld_Utils::QuadrantPos> quadrantToDelete;

	bool addedQuandrantsForInititalization = false;
	bool counterStartedNow = false;
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

		//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (map stuff)");
		// insert needed quadrants not present in map and refresh the ones which are already in it
		//vector<QuadrantPos> quadrantToAdd;
		std::timespec refreshTime;

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.0 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (get time)");

			int ret = timespec_get(&refreshTime, TIME_UTC);
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create/refresh)");

			size_t size = quadrantPosNeeded.size();
			for (int idx = 0; idx < size; idx++)
			{
				WorldQuadrantsMap::iterator it = m_worldQuadrantsMap.find(quadrantPosNeeded[idx]);
				if (it == m_worldQuadrantsMap.end())
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.1.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (create new quads)");

					//if (it != m_worldQuadrantsMap.end() /* && it->second->statusToErase() // useless for the previous if */)
					//{
					//	// extract element from map without destroying it than remember to destory later (next _process)
					//	std::_Node_handle h = m_worldQuadrantsMap.extract(it->first);
					//	TheWorld_Utils::QuadrantPos pos = h.key();
					//	m_toErase[pos] = true;
					//}

					m_worldQuadrantsMap[quadrantPosNeeded[idx]] = make_unique<QuadTree>(this, quadrantPosNeeded[idx]);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->refreshTime(refreshTime);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->quadrantDepth(quadrantDepth[idx]);
					m_worldQuadrantsMap[quadrantPosNeeded[idx]]->numQuadrantsFromCameraQuadrant(numQuadrantsFromCameraQuadrant[idx]);
					addedQuandrantsForInititalization = true;
					newQuadAdded = true;

					if (!m_streamingTime.counterStarted())
					{
						counterStartedNow = true;
						writeStreamingNewQuadrantsStatistics();
						if (m_notStreamingTime.counterStarted())
							m_notStreamingTime.tock();
						PLOG_DEBUG /* << "\n" */  << "Start initializing of a stream of quadrant";
						m_streamingTime.tick();

						startNewStreamOfQuadrants();
					}
				}
				else
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (refresh old quads)");

					if (it->second->statusToErase())
					{
						// recycle quadrant flagged to erase because it is now usefull
						it->second->setPrevStatus();
						if (m_toErase.contains(it->first))
							m_toErase.erase(it->first);
					}

					it->second->refreshTime(refreshTime);
					it->second->setTag(quadrantPosNeeded[idx].getTag());
					it->second->quadrantDepth(quadrantDepth[idx]);
					it->second->numQuadrantsFromCameraQuadrant(numQuadrantsFromCameraQuadrant[idx]);
				}
			}
		}

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.2 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (second loop)");

			// look for the quadrant to delete (the ones not refreshed)
			for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
			{
				//size_t distanceInPerimeterFromCameraQuadrant = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_computedWorldCameraQuadrantPos);

				if (itQuadTree->second->getRefreshTime().tv_nsec == refreshTime.tv_nsec && itQuadTree->second->getRefreshTime().tv_sec == refreshTime.tv_sec)
				{
					bool visibilityChanged = false;
					int distanceInPerimeterFromCameraQuadOnEstXPlus = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_gridWorldCameraQuadrantPos, TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
					int distanceInPerimeterFromCameraQuadOnSouthZPlus = itQuadTree->second->getQuadrant()->getPos().distanceInPerimeter(m_gridWorldCameraQuadrantPos, TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
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
						//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (ForAllChunk)");
						Chunk::VisibilityChangedChunkAction action(m_visibleInTree, Globals());
						itQuadTree->second->ForAllChunk(action, QuadType::NotSet);
					}
				}
				else
				{
					// Quadrant too far from camera: need to be deleted
					//if (itQuadTree->second->statusInitialized() || itQuadTree->second->statusUninitialized() || itQuadTree->second->statusRefreshTerrainDataNeeded() || itQuadTree->second->statusLoadTerrainDataFromCacheNeeded())
					//{
						itQuadTree->second->setVisible(false);
						quadrantToDelete.push_back(itQuadTree->first);
					//}

					//if (m_cameraQuadTree != nullptr && itQuadTree->first == m_cameraQuadTree->getQuadrant()->getPos())
					//{
					//	// if we delete the ones containing old camera chunk we invalidate it
					//	m_cameraChunk = nullptr;
					//	m_cameraQuadTree = nullptr;
					//}
				}
				
				if (itQuadTree->second->getQuadrant()->needReloadTerrainDataFromCache())
					itQuadTree->second->setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);

				Quadrant::QuadrantTypeInGrid type = Quadrant::QuadrantTypeInGrid::NotSet;
				godot::Vector2i quadrantPosInGrid = itQuadTree->second->getQuadrant()->quadrantPosInGrid(type);
				itQuadTree->second->quadrantTypeInGrid(type);
			}
		}
		
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.3 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase all quads)");

			for (int idx = 0; idx < quadrantToDelete.size(); idx++)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("recalcGridOfQuadrantsInView 1.1.3.1 ") + __FUNCTION__, "Adjust Quadtrees: recalc quadrants (erase quad)");
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

	recalcGridOfQuadrantsInViewJustExecuted(true);
	
	return newQuadAdded;
}

bool GDN_TheWorld_Viewer::streamingQuadrantsInProgress(void) const
{
	return worldStreamingInProgress();
}

void GDN_TheWorld_Viewer::startNewStreamOfQuadrants(void)
{
	worldStreamingInProgress(true);
}

void GDN_TheWorld_Viewer::endLastStreamOfQuadrants(void)
{
	worldStreamingInProgress(false);
	finishedResizingGrid(false);
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

bool GDN_TheWorld_Viewer::isInputEventTracked(TrackedInputEvent::TrackedInputEventType eventType, TrackedInputEvent* trackedInputEvent, TrackedInputEventContainer* inputEvents)
{
	if (inputEvents == nullptr)
		inputEvents = &m_trackedInputEventsForProcess;

	TrackedInputEvent event(nullptr, eventType);
	if (inputEvents->contains(event))
	{
		if (trackedInputEvent != nullptr)
		{
			TrackedInputEventContainer::iterator it = inputEvents->find(event);
			*trackedInputEvent = it->first;
		}
		return true;
	}
	else
		return false;
}

void GDN_TheWorld_Viewer::compressTrackedInputEvents(TrackedInputEventContainer* inputEvents, TrackedInputEventQueue* trackedInputEventQueue)
{
	if (inputEvents == nullptr)
		inputEvents = &m_trackedInputEventsForProcess;
	if (trackedInputEventQueue == nullptr)
		trackedInputEventQueue = &m_trackedInputEventsQueueForProcess;

	if (!trackedInputEventQueue->empty())
	{
		for (auto& trackedInputEvent : *trackedInputEventQueue)
		{
			if (trackedInputEvent.isAvailable())
			{
				(*inputEvents)[trackedInputEvent] = true;
				trackedInputEvent.reset();
			}
		}
	}

	//// DEBUG
	//{
	//	bool printHeader = true;
	//	TrackedInputEvent trackedInputEvent(nullptr);
	//	if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT, &trackedInputEvent))
	//	{
	//		if (printHeader)
	//		{
	//			PLOG_DEBUG << "===============";
	//			printHeader = false;
	//		}
	//		PLOG_DEBUG << "Mouse button LEFT " << trackedInputEvent.strTime();
	//	}
	//	if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT, &trackedInputEvent))
	//	{
	//		if (printHeader)
	//		{
	//			PLOG_DEBUG << "===============";
	//			printHeader = false;
	//		}
	//		PLOG_DEBUG << "Mouse button RIGHT " << trackedInputEvent.strTime();
	//	}
	//	if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT, &trackedInputEvent))
	//	{
	//		if (printHeader)
	//		{
	//			PLOG_DEBUG << "===============";
	//			printHeader = false;
	//		}
	//		PLOG_DEBUG << "Double mouse button LEFT " << trackedInputEvent.strTime();
	//	}
	//	if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT, &trackedInputEvent))
	//	{
	//		if (printHeader)
	//		{
	//			PLOG_DEBUG << "===============";
	//			printHeader = false;
	//		}
	//		PLOG_DEBUG << "Double mouse button RIGHT " << trackedInputEvent.strTime();
	//	}
	//}
	//// DEBUG
}

void GDN_TheWorld_Viewer::resetTrackedInputEvents(TrackedInputEventContainer* inputEvents, TrackedInputEventQueue* trackedInputEventQueue)
{
	if (inputEvents == nullptr)
		inputEvents = &m_trackedInputEventsForProcess;
	if (trackedInputEventQueue == nullptr)
		trackedInputEventQueue = &m_trackedInputEventsQueueForProcess;

	if (!inputEvents->empty())
		inputEvents->clear();
	if (!trackedInputEventQueue->empty())
	{
		bool allItemsReset = true;
		for (auto& trackedInputEvent : *trackedInputEventQueue)
		{
			//if (trackedInputEvent.isAvailable())		// trackedInputEvent was available to check so we can reset it now
			//	trackedInputEvent.reset();
			//else if (not trackedInputEvent.isReset())
			//	allItemsReset = false;
			if (not trackedInputEvent.isReset())
				allItemsReset = false;
		}

		if (allItemsReset)
			trackedInputEventQueue->clear();
	}
	//else
	//	assert(trackedInputEventQueue->empty());
}

void GDN_TheWorld_Viewer::_process(double _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Godot::print("GDN_TheWorld_Viewer::_process");

	m_processSteps.initForCurrentExecution();

	m_savedViewportSize = getViewportSize();
	
	GDN_TheWorld_Globals* globals = Globals();
	if (globals == nullptr)
		return;

//#define MAX_PROCESS_DURATION	100000
//#define MAX_PROCESS_DURATION	200000
#define MAX_PROCESS_DURATION	300000
	m_maxProcessDuration = MAX_PROCESS_DURATION;
	if (globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed)
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
	//m_lastProcessedStep = "";

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

		if (initRequired())
		{
			init();
			globals->setStatus(TheWorldStatus::sessionInitialized);
		}

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);		// WARNING
		std::unique_lock<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing, std::try_to_lock);
		if (!lock.owns_lock())
		{
			m_numProcessNotOwnsLock++;
			return;
		}

		compressTrackedInputEvents();

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
		{
			TimerMcs clock;
			clock.tick();
			auto save_duration = TheWorld_Viewer_Utils::finally([&clock, this] {
				clock.tock();
				this->m_lastProcessImplDuration = clock.duration().count();
				this->m_numProcessImplExecution++;
				this->m_sumProcessImplDuration += this->m_lastProcessImplDuration;
				});

			ADD_PROCESS_STEP(process_beforeProcessImpl);

			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.1 ") + __FUNCTION__, "process_impl");
			process_impl(_delta, activeMapCamera, activeWorldCamera);

			ADD_PROCESS_STEP(EndOfProcessExecution);


			m_lastProcessDuration = m_processSteps.elapsed();
			m_processSteps.finalizeCurrentExecution();
			m_numProcessExecution++;
			m_sumProcessDuration += (m_lastProcessDuration / 1000);
		}

		resetTrackedInputEvents();
	
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.2 ") + __FUNCTION__, "Process Statistics");
			process_statistics(globals);
		}

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("_process 1.3 ") + __FUNCTION__, "Process Dump");
			process_dump(globals);
		}
	}

	if ((m_lastProcessExecutionInterrupted || m_lastProcessDuration > m_maxProcessDuration) && (globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed))
	{
		std::vector<ProcessStep>& steps = m_processSteps.steps();
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
				msg1 += step.msg() + " ";
			globals->debugPrint(msg1.c_str());
		}
	}
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
		m_firstProcess = false;
	}

	if (adjustEnvironmentRequired())
	{
		adjustEnvironment();
		adjustEnvironmentRequired(false);
	}
	
	if (globals->status() == TheWorldStatus::worldDeployInProgress && m_loadingWorldRefresh < 10)
		if (m_num > 10)
		{
			m_refreshRequired = true;
			m_num = 0;
			m_loadingWorldRefresh++;
		}
		else
			m_num++;

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

	if (globals->statusMinimalWorldDeployed() and worldStreamingTerminated())
		globals->setStatus(TheWorldStatus::worldDeployed);

	calcDelayDegree();
	
	ADD_PROCESS_STEP(process_initialStuff_0);

	// erase out-of-view quadrants
	PROCESS_STEP_WITH_CHECK(process_eraseOutOfViewQuads(editModeUIControl), process_eraseOutOfViewQuadrants, "process_impl 1.01 ", "Erase out-of-view quadrants");

	PROCESS_STEP_WITH_CHECK(process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution(_delta), process_mapModder, "process_impl 1.02 ", "Modder work");

	if (mapMode())
	{
		process_mapModeInputEvents();

		if (adjustWorldCameraRequired())
		{
			adjustWorldCamera(m_desideredCameraWorldPos);

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
				map->setWorldFlag("WorldCamera", GDN_TheWorld_Globals::g_color_red, true);

			GDN_TheWorld_HudControls* hudControls = HudControls();
			if (hudControls != nullptr)
			{
				if (globals->loadingScreen())
				{
					hudControls->showLoadingBar();
				}
				else
					hudControls->hideLoadingBar();
			}

			adjustWorldCameraRequired(false);
		}

		ADD_PROCESS_STEP(process_mapQuadrantsUI);
	}
	
	if (delayedWorldMode())
	{
		if (worldMode())
			delayedWorldMode(false);
		else
		{
			if (worldStreamingTerminated())
			{
				mapMode(false);
				delayedWorldMode(false);
			} 
			else
			{
				GDN_TheWorld_HudControls* hudControls = HudControls();
				if (hudControls != nullptr)
				{
					// Set progress bar
					float loadingProgress = getLoadingProgress();
					hudControls->setLoadingProgress(loadingProgress);
					//float numQuadrant = (globals->loadingScreenExitOnFullWorldDeploy() ? float(getMaxNumQuadrant()) : float(getNumQuadrant()));
					//float numInitializedQuadrant = float(getNumInitializedQuadrant());
					//if (numQuadrant > 1 and numInitializedQuadrant > 0)
					//{
					//	float progress = numInitializedQuadrant / numQuadrant;
					//	hudControls->setLoadingProgress(progress * 100);
					//}
				}
			}
		}

		ADD_PROCESS_STEP(process_delayedWorldMode);
	}

	GDN_TheWorld_Map*map = Map();
	if (map != nullptr)
	{
		if (recalcGridOfQuadrantsInViewJustExecuted())
		{
			map->worldQuadrantsMapReset();
			recalcGridOfQuadrantsInViewJustExecuted(false);

			ADD_PROCESS_STEP(process_adjustMapQuadrants);
		}
	}
	
	godot::Vector3 worldCameraWorldPos;
	{
		if (IS_EDITOR_HINT())
			worldCameraWorldPos = activeWorldCamera->get_global_transform().get_origin();
		else
			worldCameraWorldPos = ((GDN_TheWorld_Camera*)activeWorldCamera)->getWorldPos();
	}
	godot::Vector2 mapCameraWorldPos;
	{
		if (IS_EDITOR_HINT())
			mapCameraWorldPos = activeMapCamera->get_global_transform().get_origin();
		else
			mapCameraWorldPos = ((GDN_TheWorld_MapCamera*)activeMapCamera)->getCameraWorldPos();
	}

	bool processCameraPosForQuadrants = true;
	bool processCameraPosForChunks = true;
	if (globals->status() == TheWorldStatus::worldDeployed or globals->status() == TheWorldStatus::minimalWorldDeployed)
	{
		processCameraPosForQuadrants = false;
		processCameraPosForChunks = false;
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (m_worldCameraTerrainChunk == nullptr || m_lastWorldCameraTerrainChunk == nullptr || (TIME_INTERVAL_BETWEEN_ELAB_CAMERA != -1 && timeElapsed - m_timeElapsedFromLastProcessWorldCameraPosForQuadrants > TIME_INTERVAL_BETWEEN_ELAB_CAMERA))
		{
			m_timeElapsedFromLastProcessWorldCameraPosForQuadrants = timeElapsed;
			processCameraPosForQuadrants = true;
			processCameraPosForChunks = true;
			if (m_lastWorldCameraTerrainChunk != m_worldCameraTerrainChunk)
				m_lastWorldCameraTerrainChunk = m_worldCameraTerrainChunk;
		}
		else
		{
			float distance = (float)worldCameraWorldPos.distance_to(m_lastProcessedWorldCameraPosForQuadrants);
			float threshold = globals->getRefreshQuadrantTreeThreshold();
			if (m_lastProcessedWorldCameraPosForQuadrants == godot::Vector3() || distance > threshold)
			{
				m_lastProcessedWorldCameraPosForQuadrants = worldCameraWorldPos;
				processCameraPosForQuadrants = true;
			}
			distance = (float)worldCameraWorldPos.distance_to(m_lastProcessedWorldCameraPosForChunks);
			threshold = globals->getRefreshChunkLodThreshold();
			if (m_lastProcessedWorldCameraPosForChunks == godot::Vector3() || distance > threshold)
			{
				m_lastProcessedWorldCameraPosForChunks = worldCameraWorldPos;
				processCameraPosForChunks = true;
			}
			else
			{
				if (m_lastWorldCameraTerrainChunk != m_worldCameraTerrainChunk)
				{
					processCameraPosForChunks = true;
					m_lastWorldCameraTerrainChunk = m_worldCameraTerrainChunk;
				}
			}
		}

		ADD_PROCESS_STEP(process_checkCameraMovement);
	}

	//Transform globalTransform = internalTransformGlobalCoord();
	//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos

	if (processCameraPosForQuadrants)
	{
		PROCESS_STEP_WITH_CHECK(process_updateWorldCameraQuadrant(globals, worldCameraWorldPos), process_updateCameraQuadrant, "process_impl 1.03 ", "Adjust Quadtrees");
	}

	PROCESS_STEP_WITH_CHECK(process_initNewQuads(processCameraPosForQuadrants), process_initNewQuads, "process_impl 1.04 ", "Init new quadrants");

	PROCESS_STEP_WITH_CHECK(process_trackMouse(activeWorldCamera), process_trackMouse, "process_impl 1.05 ", "Track mouse");

	if (worldMode()) 
	{
		PROCESS_STEP_WITH_CHECK(process_castRayFromCamera(activeWorldCamera), process_castRayFromCamera, "process_impl 1.06 ", "Cast Ray From Camera");
	}

	//if (worldMode())	 // 1
	PROCESS_STEP_WITH_CHECK(process_updateCameraPosInShader(activeWorldCamera), process_updateCameraInQuadrants, "process_impl 1.07 ", "Update camera in quadrants");

	//if (worldMode())
	PROCESS_STEP_WITH_CHECK(process_updateOriginShift(activeMapCamera, activeWorldCamera), process_updateOriginShift, "process_impl 1.08 ", "Update origin shift");

	//if (worldMode())	 // 1
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.09 ") + __FUNCTION__, "Update quadrants");
		if (processCameraPosForChunks || !finishedResizingGrid())
		{
			PROCESS_STEP_WITH_CHECK(process_updateLodInQuads(worldCameraWorldPos), process_updateLodInQuads, "process_impl 1.09.1 ", "Update quadrants LOD");
		}
		else
		{
			for (auto& itQuadTree : m_worldQuadrantsMap)
			{
				itQuadTree.second->checkMouseHitOnTerrain();
			}
		}
	}

	//if (worldMode())	 // 1
	PROCESS_STEP_WITH_CHECK(process_updateMeshForUpdatedChunks(), process_updateMeshForUpdatedChunks, "process_impl 1.10 ", "Update chunks");

	//if (worldMode())
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.11 ") + __FUNCTION__, "Update Grid Dimension");
		int64_t elapsedFromLastUpdateGridDimension = TIME()->get_ticks_msec() - m_timeElapsedFromLastUpdateGridDimension;
		if (processCameraPosForChunks || !finishedResizingGrid() || elapsedFromLastUpdateGridDimension  > TIME_INTERVAL_BETWEEN_RECALC_QUADRANT)
		{
			PROCESS_STEP_WITH_CHECK(process_updateGridDimension(globals, activeWorldCamera), process_updateGridDimension, "process_impl 1.11 ", "Update Grid Dimension");
		}
	}

	//if (worldMode()) // 1
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
		PROCESS_STEP_WITH_CHECK(process_lookDev(), process_lookDev, "process_impl 1.13 ", "Process Lookdev");
	}
	
	//if (worldMode())
	PROCESS_STEP_WITH_CHECK(process_materialParams(worldCameraWorldPos, processCameraPosForQuadrants, processCameraPosForChunks), process_materialParams, "process_impl 1.14 ", "Process Material Params");

	if (mapMode())
	{
		PROCESS_STEP_WITH_CHECK(process_mapQuadrants(mapCameraWorldPos), process_mapQuadrants, "process_impl 1.15 ", "Process Map Quadrants");
	}

	PROCESS_STEP_WITH_CHECK(process_updateHud(activeWorldCamera), process_updateHud, "process_impl 1.16 ", "Update HUD");

	//m_desideredLookDevChanged = false;
}

void GDN_TheWorld_Viewer::process_mapModeInputEvents(void)
{
	// *********************
	// MapMode Mouse buttons
	// *********************
	//	EditMode:
	//		* Double Left Click: if pointed quadrant is already selected deselect it saving parameters from editor panel otherwise select pointed quadrant and activate the editing of that quadrant setting editor panel parameter with its actual values,
	//			if it is not actually in world map editor panel parameters are used
	//		* Double Right Click: if a quadrant is actually selected deselect it without modifying its parameters regardless which quadrant is pointed
	//		* Single Left Click: if exist an override for pointed quadrant its parameters are displayed in editor panel 
	//		* Single Right Click: if quadrant editing is active and no quadrant is selected do next things:
	//			* if pointed quadrant has an override remove it
	//			* if pointed quadrant does not have an override insert one using editor panel parameters
	//	Non EditMode:
	//		* Double Left Click: move camera on selected point (same direction) and switch to world mode
	
	if (editMode())
	{
		GDN_TheWorld_Map* map = Map();
		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr and editModeUIControl->initialized() and map != nullptr and !editModeUIControl->mouseInsideEditPanel())
		{
			TrackedInputEvent trackedInputEvent(this);

			//if (editModeUIControl->editingMap())
			{
				if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_RIGHT, &trackedInputEvent))
				{
					if (/*not m_selectedMapQuadrantPos.isInitialized()*/ not isMapQuadrantSelectedForEdit() and not map->dragBoxOn())
					{
						godot::Vector2 mousePosInViewport = trackedInputEvent.mousePosInViewport();
						godot::Vector2 mouseGlobalPos = map->getMousePosInGlobalCoord(mousePosInViewport);
						godot::Vector2 mouseWorldPos = map->globalPosToWorldPos(mouseGlobalPos);
						TheWorld_Utils::QuadrantPos pos(mouseWorldPos.x, mouseWorldPos.y, m_worldViewerLevel, (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
						addClearOverrideMapQuadrantRef(pos);
					}
				}
			}

			if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_BUTTON_LEFT, &trackedInputEvent))
			{
				if (/*not m_selectedMapQuadrantPos.isInitialized()*/ not isMapQuadrantSelectedForEdit() and not map->dragBoxOn())
				{
					godot::Vector2 mousePosInViewport = trackedInputEvent.mousePosInViewport();
					godot::Vector2 mouseGlobalPos = map->getMousePosInGlobalCoord(mousePosInViewport);
					godot::Vector2 mouseWorldPos = map->globalPosToWorldPos(mouseGlobalPos);
					TheWorld_Utils::QuadrantPos pos(mouseWorldPos.x, mouseWorldPos.y, m_worldViewerLevel, (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());

					GDN_TheWorld_Map::MapQuadrantOverride* MapQuadrantOverride = map->getMapQuadrantOverride(pos);
					if (MapQuadrantOverride != nullptr and MapQuadrantOverride->getTerrainType() != TheWorld_Utils::TerrainEdit::TerrainType::notset)
					{
						TheWorld_Utils::TerrainEdit terrainEdit = MapQuadrantOverride->getTerrainEdit();
						editModeUIControl->setTerrainEditValues(terrainEdit, MapQuadrantOverride->getPos(), true);
					}
					else
					{
						editModeUIControl->resetPanelsOnQuadrantDeselection();
					}
				}
			}

			if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT, &trackedInputEvent))
			{
				if (mapMode())
				{
					godot::Vector2 mousePosInViewport = trackedInputEvent.mousePosInViewport();
					godot::Vector2 mouseGlobalPos = map->getMousePosInGlobalCoord(mousePosInViewport);
					godot::Vector2 mouseWorldPos = map->globalPosToWorldPos(mouseGlobalPos);
					TheWorld_Utils::QuadrantPos pos(mouseWorldPos.x, mouseWorldPos.y, m_worldViewerLevel, (int)Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU());
					selectMapQuadrantRef(pos);
					selectWorldQuadrantRef(pos);
				}
			}

			if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT, &trackedInputEvent))
			{
				deselectMapQuadrant();
				deselectWorldQuadrant();
			}
		}
	}
	else
	{
		if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT, nullptr))
		{
			godot::Vector2 mouseScreenPos = getMousePosInViewport();
			godot::Vector2 screenCenter = getViewportSize() / 2;
			godot::Vector2 mouseOffsetFromScreencenter = mouseScreenPos - screenCenter;
			Camera2D* mapCamera = MapCamera();
			if (mapCamera != nullptr)
			{
				GDN_TheWorld_MapCamera* cam = godot::Object::cast_to<GDN_TheWorld_MapCamera>(mapCamera);
				mouseOffsetFromScreencenter *= (1 / cam->getZoomFactor());
				godot::Vector2 globalCameraScreencenterPos = mapCamera->get_screen_center_position();
				godot::Vector2 desideredCameraGlobalPos = globalCameraScreencenterPos + mouseOffsetFromScreencenter;
				godot::Vector2 desideredCameraWorldPos = globalPosToWorldPos(desideredCameraGlobalPos);
				adjustWorldCameraRequired(true, desideredCameraWorldPos);
				//adjustWorldCamera(desideredCameraWorldPos);
			}
		}
	}
}

void GDN_TheWorld_Viewer::clearOverrideMapQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	clearOverrideMapQuadrantRef(pos);
}

void GDN_TheWorld_Viewer::clearOverrideMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos)
{
	GDN_TheWorld_Map* map = Map();
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

	bool addingOverride = true;
	GDN_TheWorld_Map::MapQuadrantOverride* MapQuadrantOverride = map->getMapQuadrantOverride(pos);
	if (MapQuadrantOverride != nullptr and MapQuadrantOverride->getTerrainType() != TheWorld_Utils::TerrainEdit::TerrainType::notset)
		addingOverride = false;

	if (addingOverride)
	{
	}
	else
	{
		// removing override
		if (m_selectedMapQuadrantPos.isInitialized() and m_selectedMapQuadrantPos == pos)
		{
			map->editMode(m_selectedMapQuadrantPos, false);
			m_selectedMapQuadrantPos.reset();
		}
		map->removeEditedMapQuadrantOverride(pos);
	}
}

void GDN_TheWorld_Viewer::addClearOverrideMapQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	addClearOverrideMapQuadrantRef(pos);
}

void GDN_TheWorld_Viewer::addClearOverrideMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos)
{
	GDN_TheWorld_Map* map = Map();
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();

	bool addingOverride = true;
	GDN_TheWorld_Map::MapQuadrantOverride* MapQuadrantOverride = map->getMapQuadrantOverride(pos);
	if (MapQuadrantOverride != nullptr and MapQuadrantOverride->getTerrainType() != TheWorld_Utils::TerrainEdit::TerrainType::notset)
		addingOverride = false;

	if (addingOverride)
	{
		// adding override
		TheWorld_Utils::TerrainEdit terrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);
		if (editModeUIControl->editingMap(terrainEdit))
		{
			map->addEditedMapQuadrantOverride(pos, terrainEdit);
		}
	}
	else
	{
		// removing override
		if (m_selectedMapQuadrantPos.isInitialized() and m_selectedMapQuadrantPos == pos)
		{
			map->editMode(m_selectedMapQuadrantPos, false);
			m_selectedMapQuadrantPos.reset();
		}
		map->removeEditedMapQuadrantOverride(pos);
	}
}

void GDN_TheWorld_Viewer::selectMapQuadrant(TheWorld_Utils::QuadrantPos pos)
{
	selectMapQuadrantRef(pos);
}

void GDN_TheWorld_Viewer::selectMapQuadrantRef(TheWorld_Utils::QuadrantPos& pos)
{
	doSelectMapQuadrant(pos);
}

void GDN_TheWorld_Viewer::doSelectMapQuadrant(TheWorld_Utils::QuadrantPos& pos)
{
	bool sameQuadrant = false;
	bool deselectRequired = false;
	if (isMapQuadrantSelectedForEdit())
	{
		deselectRequired = true;
		if (m_selectedMapQuadrantPos == pos)
			sameQuadrant = true;
	}

	GDN_TheWorld_Map* map = Map();
	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr and editModeUIControl->initialized() and map != nullptr)
	{
		TheWorld_Utils::TerrainEdit terrainEdit(TheWorld_Utils::TerrainEdit::TerrainType::notset);
		GDN_TheWorld_Map::MapQuadrantOverride* selectingMapQuadrantOverride = map->getMapQuadrantOverride(pos);
		if (selectingMapQuadrantOverride != nullptr and selectingMapQuadrantOverride->getTerrainType() != TheWorld_Utils::TerrainEdit::TerrainType::notset)
		{
			terrainEdit = selectingMapQuadrantOverride->getTerrainEdit();
		}
		else if (m_worldQuadrantsMap.contains(pos) and m_worldQuadrantsMap[pos]->statusInitialized())
		{
			TheWorld_Utils::TerrainEdit* t = m_worldQuadrantsMap[pos]->getQuadrant()->getTerrainEdit(false);
			if (t != nullptr && t->terrainType != TheWorld_Utils::TerrainEdit::TerrainType::notset)
			{
				terrainEdit = *t;
				selectingMapQuadrantOverride = map->addEditedMapQuadrantOverride(pos, terrainEdit);
			}
		}
		else
		{
			std::string dir = GDN_TheWorld_Globals::getClientDataDir();
			std::string mapName = Globals()->getMapName();
			TheWorld_Utils::MeshCacheBuffer cache(dir, mapName, pos.getGridStepInWU(), pos.getNumVerticesPerSize(), pos.getLevel(), pos.getLowerXGridVertex(), pos.getLowerZGridVertex());
			TheWorld_Utils::MemoryBuffer dummyBuffer;
			TheWorld_Utils::TerrainEdit t(TheWorld_Utils::TerrainEdit::TerrainType::unknown);
			cache.readBufferFromDisk("", dummyBuffer, false, &t, true);
			terrainEdit = t;
			selectingMapQuadrantOverride = map->addEditedMapQuadrantOverride(pos, terrainEdit);
			//TheWorld_Utils::TerrainEdit terrainEditUnknown(TheWorld_Utils::TerrainEdit::TerrainType::unknown);
			//MapQuadrantOverride = map->addEditedMapQuadrantOverride(pos, terrainEditUnknown);
			//terrainEdit = terrainEditUnknown;
		}

		if (sameQuadrant)	// if double click on previously selected quadrant quit edit mode and persist edited values
		{
			// if double click on previously selected quadrant persist edited values
			editModeUIControl->getEditedTerrainEditValues(terrainEdit);
			if (terrainEdit.terrainType != TheWorld_Utils::TerrainEdit::TerrainType::notset)
				selectingMapQuadrantOverride = map->addEditedMapQuadrantOverride(pos, terrainEdit);
			deselectMapQuadrant();
		}
		else
		{
			if (deselectRequired)		// deselect previously selected quadrant if exist as it is different from the new one to select
				deselectMapQuadrant();

			if (selectingMapQuadrantOverride != nullptr and terrainEdit.terrainType != TheWorld_Utils::TerrainEdit::TerrainType::notset)
			{
				editModeUIControl->setTerrainEditValues(terrainEdit, selectingMapQuadrantOverride->getPos());
				map->editMode(pos, true);
				m_selectedMapQuadrantPos = pos;
			}
		}

		editModeUIControl->expandTerrainGenerationPanel();
	}
}

void GDN_TheWorld_Viewer::deselectMapQuadrant(void)
{
	doDeselectMapQuadrant();
}

void GDN_TheWorld_Viewer::doDeselectMapQuadrant(void)
{
	GDN_TheWorld_Map* map = Map();
	if (isMapQuadrantSelectedForEdit())
	{
		map->editMode(m_selectedMapQuadrantPos, false);
		m_selectedMapQuadrantPos.reset();
		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr and editModeUIControl->initialized() and map != nullptr)
			editModeUIControl->resetPanelsOnQuadrantDeselection();
	}
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

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	std::vector<TheWorld_Utils::QuadrantPos> toErase;

	bool canProceedErasingutOfViewQuads = true;
	if (editModeUIControl != nullptr and editModeUIControl->initialized() and editModeUIControl->terrainDataLocked())
		canProceedErasingutOfViewQuads = false;

	if (canProceedErasingutOfViewQuads)
	{
		for (auto& item : m_toErase)
		{
			size_t numErased = 0;

			TheWorld_Utils::QuadrantPos pos = item.first;

			if (m_worldQuadrantsMap.contains(pos))
			{
				bool locked = m_worldQuadrantsMap[pos]->getQuadrant()->lockInternalData();
				if (locked)
				{
					QuadTree* quadTreeToErase = m_worldQuadrantsMap[pos].get();
					if (editModeUIControl != nullptr && editModeUIControl->initialized())
					{
						bool manualSave;
						if (!editModeUIControl->isQuadToSave(pos, manualSave) && !quadTreeToErase->getQuadrant()->needUploadToServer())
						{
							m_adjacentQuadrantsHit.remove(quadTreeToErase);
							if (m_quadrantHit == quadTreeToErase)
								m_quadrantHit = nullptr;
							if (m_editMode)
								if (editModeUIControl->getQuadrantHit() == quadTreeToErase)
									editModeUIControl->setQuadrantHit(nullptr);
							//requestDataRefreshToShader(quadTreeToErase, false);
							numErased = m_worldQuadrantsMap.erase(pos);
							toErase.push_back(pos);
							//break;
						}
						else
							m_worldQuadrantsMap[pos]->getQuadrant()->unlockInternalData();
					}
					else
					{
						//requestDataRefreshToShader(quadTreeToErase, false);
						numErased = m_worldQuadrantsMap.erase(pos);
						toErase.push_back(pos);
						//break;
					}
				}
			}
		}

		//m_toErase.clear();
		for (auto& pos : toErase)
		{
			m_toErase.erase(pos);
		}
	}

	toErase.clear();
	for (auto& item : m_extraQuadsMap)
		if (item.second->statusInitialized() and item.second->getQuadrant()->mapModderWorkDone())	// XXXXX
			toErase.push_back(item.first);
	for (auto& pos : toErase)
	{
		pos.trackQuadrantForDebug("", "Erasing quadrant from extra quads map");

		enum class QuadrantStatus status = QuadrantStatus::notSet;
		IF_PLOG(plog::verbose)
		{
			if (m_extraQuadsMap.contains(pos))
			{
				status = m_extraQuadsMap[pos]->status();
			}
		}
		m_extraQuadsMap.erase(pos);
		PLOG_VERBOSE << "Erased quadrant from extra quads map " << pos.getIdStr() << " - Status=" << QuadTree::statusToString(status);
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

	std::lock_guard<std::recursive_mutex> lock1(m_mtxQuadrantsAndMainProcessing);

	//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1 ") + __FUNCTION__, "Inside Lock");

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

	std::list<TheWorld_Utils::QuadrantPos> quadrantsToBlend;
	
	//m_worldQuadrantsMapByDepth.clear();
	m_worldQuadrantsMapByDistanceFromCameraQuadrant.clear();

	bool _allVisibleQuadrantsInitialized = true;
	bool _allQuadrantsInitialized = true;
	//bool _allNeighbourQuadrantInitialized = true;		// DEBUG

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1 ") + __FUNCTION__, "Loop m_worldQuadrantsMap");

		for (auto& item : m_worldQuadrantsMap)
		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1 ") + __FUNCTION__, "Elab Single Quadrant");

			QuadTree* quadTree = item.second.get();
			Quadrant* quadrant = quadTree->getQuadrant();

			//m_worldQuadrantsMapByDepth[quadTree->quadrantDepth()][item.first] = quadTree;
			m_worldQuadrantsMapByDistanceFromCameraQuadrant[quadTree->numQuadrantsFromCameraQuadrant()][item.first] = quadTree;

			if (globals->getParamShowSeaChanged())
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 1");
				Chunk::VisibilityChangedChunkAction action(globals->getParamShowSea(), globals);
				quadTree->ForAllChunk(action, QuadType::Sea);
			}

			if (globals->getParamShowTerrainChanged())
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 2");
				Chunk::VisibilityChangedChunkAction action(globals->getParamShowTerrain(), globals);
				quadTree->ForAllChunk(action, QuadType::Terrain);
			}

			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 3");
				if (quadTree->initDelayedRequested())
					quadTree->initDelayed();

				quadrant->checkInternalData(_delta);

				quadrant->lockInternalDataUntilCompleteProcessExecution(false);
			}


			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 4");
				quadrant->process_mapModder(_delta, true);
			}

			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 5");
				quadrant->process_checkBlendNeeded(_delta);
				if (quadrant->blendNeeded())
					quadrantsToBlend.push_back(quadrant->getPos());;
			}

			if (quadTree->statusLoadTerrainDataFromCacheNeeded())
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 6");
				quadTree->setStatus(QuadrantStatus::loadTerrainDataFromCacheInProgress);
				std::function<void(void)> f = std::bind(&Quadrant::loadTerrainDataFromCache, quadrant);
				m_tpClientWorker.QueueJob(f);
			}

			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1.1 ") + __FUNCTION__, "Elab Single Quadrant 7");
				quadTree->checkSeaChunksToDelete();
			}

			process_neighbourQuadrantData(quadTree);	// Occhio!!!

			if (quadTree->status() < QuadrantStatus::initialized)
			{
				_allQuadrantsInitialized = false;
				enum class QuadTree::QuadrantDepth quadrantDepth = quadTree->quadrantDepth();
				if (quadrantDepth != QuadTree::QuadrantDepth::Cache)
					_allVisibleQuadrantsInitialized = false;
			}

			//if (quadTree->quadrantDepth() == QuadTree::QuadrantDepth::Cache)
			//	continue;
			//if (quadTree->statusToErase())
			//	continue;
			//if (not quadTree->statusInitialized())
			//{
			//	_allVisibleQuadrantInitialized = false;
			//}
		}
	}

	allQuadrantsInitialized(_allQuadrantsInitialized);
	allVisibleQuadrantsInitialized(_allVisibleQuadrantsInitialized);

	GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
	if (editModeUIControl != nullptr and editModeUIControl->initialized() and not editModeUIControl->terrainDataLocked() and canProcessWorldDataUpdate())
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1 ") + __FUNCTION__, "Loop process_blend");

		for (auto& item : m_worldQuadrantsMap)
		{
			Quadrant* quadrant = item.second->getQuadrant();
			//if (quadrant->getPos().getIndexPosX() == -9 and quadrant->getPos().getIndexPosZ() == -10)	// #####
			//	assert(editModeUIControl != nullptr);									// #####

			if (quadrant->blendNeeded() and not editModeUIControl->isDelayedBlend(quadrant->getPos()))
			{
				bool processed = quadrant->process_blend();
				if (processed)
					break;
			}
		}
	}

	if (globals->getParamShowSeaChanged())
	{
		globals->setParamShowSeaChanged(false);
		if (editModeUIControl != nullptr and editModeUIControl->initialized())
			editModeUIControl->showSeaPressed(globals->getParamShowSea());
	}

	if (globals->getParamShowTerrainChanged())
	{
		globals->setParamShowTerrainChanged(false);
		if (editModeUIControl != nullptr and editModeUIControl->initialized())
			editModeUIControl->showTerrainPressed(globals->getParamShowTerrain());
	}

	if (globals->getParamShowFogChanged())
	{
		adjustEnvironment();
		globals->setParamShowFogChanged(false);
		if (editModeUIControl != nullptr and editModeUIControl->initialized())
			editModeUIControl->showFogPressed(globals->getParamShowFog());
	}
		
	if (Globals()->status() == TheWorldStatus::worldDeployed or Globals()->status() == TheWorldStatus::minimalWorldDeployed)
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution 1.1.1 ") + __FUNCTION__, "Loop m_extraQuadsMap");

		for (auto& item : m_extraQuadsMap)
		{
			if (canProcessWorldDataUpdate())
			{
				if (item.second->initDelayedRequested())
					item.second->initDelayed();

				if (item.second->statusUninitialized())
				{
					bool forced = true;
					item.second->initDelayed(forced);
				}

				item.second->getQuadrant()->process_mapModder(_delta, false);
			}
		}
	}

	if (editModeUIControl != nullptr and editModeUIControl->initialized())
		editModeUIControl->setQuadrantsToBlend(quadrantsToBlend);

	//size_t numBlendNeeded = 0;
	//if (editModeUIControl != nullptr and editModeUIControl->initialized())
	//	numBlendNeeded = editModeUIControl->getNumQuadrantsToBlend();
	//if (numBlendNeeded > 0)
	//	PLOG_DEBUG << "numBlendNeeded=" << numBlendNeeded;
}

bool GDN_TheWorld_Viewer::process_updateGridDimension(GDN_TheWorld_Globals* globals, godot::Camera3D* activeWorldCamera)
{
	bool processed = false;

	if (globals == nullptr)
		return processed;

	//int64_t elapsedFromLastUpdateGridDimension = TIME()->get_ticks_msec() - m_timeElapsedFromLastUpdateGridDimension;
	if ((Globals()->status() == TheWorldStatus::worldDeployed or Globals()->status() == TheWorldStatus::minimalWorldDeployed or Globals()->status() == TheWorldStatus::worldDeployInProgress)
		&& !recalcGridOfQuadrantsInViewNeeded()
		&& allVisibleCollidersReady()
		//&& !worldStreamingInProgress()		// *** WARNING ***
		//&& elapsedFromLastRecalc > TIME_INTERVAL_BETWEEN_RECALC_QUADRANT
		&& m_computedWorldCameraQuadrantPos.isInitialized()
		&& Globals()->getStreamWorld())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_impl 1.11.01 ") + __FUNCTION__, "Update Grid Dimension");

		processed = true;

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
			godot::Vector3 castDirection = godot::Vector3(0, 0, -(m_numVisibleQuadrantOnPerimeterOnNorthSide + 1) * m_computedWorldCameraQuadrantPos.getSizeInWU());

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
				if (m_numVisibleQuadrantOnPerimeterOnNorthSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnNorthSide - 1) * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnNorthSide--;
					worldGridDimensionChanged(true);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnNorthSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnNorthSide * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnNorthSide++;
					worldGridDimensionChanged(true);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnNorthSide < getMaxDepthQuadOnPerimeter())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnNorthSide++;
					worldGridDimensionChanged(true);
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
			godot::Vector3 castDirection = godot::Vector3(0, 0, (m_numVisibleQuadrantOnPerimeterOnSouthSide + 1) * m_computedWorldCameraQuadrantPos.getSizeInWU());

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
				if (m_numVisibleQuadrantOnPerimeterOnSouthSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnSouthSide - 1) * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnSouthSide--;
					worldGridDimensionChanged(true);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnSouthSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnSouthSide * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnSouthSide++;
					worldGridDimensionChanged(true);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnSouthSide < getMaxDepthQuadOnPerimeter())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnSouthSide++;
					worldGridDimensionChanged(true);
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
			godot::Vector3 castDirection = godot::Vector3(-(m_numVisibleQuadrantOnPerimeterOnWestSide + 1) * m_computedWorldCameraQuadrantPos.getSizeInWU(), 0, 0);

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
				if (m_numVisibleQuadrantOnPerimeterOnWestSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnWestSide - 1) * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnWestSide--;
					worldGridDimensionChanged(true);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnWestSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnWestSide * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnWestSide++;
					worldGridDimensionChanged(true);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnWestSide < getMaxDepthQuadOnPerimeter())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnWestSide++;
					worldGridDimensionChanged(true);
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
			godot::Vector3 castDirection = godot::Vector3((m_numVisibleQuadrantOnPerimeterOnEastSide + 1) * m_computedWorldCameraQuadrantPos.getSizeInWU(), 0, 0);

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
				if (m_numVisibleQuadrantOnPerimeterOnEastSide > m_minVisibleQuadrantOnPerimeter && maxDistanceFromCamera < (m_numVisibleQuadrantOnPerimeterOnEastSide - 1) * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnEastSide--;
					worldGridDimensionChanged(true);
				}
				else if (m_numVisibleQuadrantOnPerimeterOnEastSide < getMaxDepthQuadOnPerimeter() && maxDistanceFromCamera > m_numVisibleQuadrantOnPerimeterOnEastSide * m_computedWorldCameraQuadrantPos.getSizeInWU())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnEastSide++;
					worldGridDimensionChanged(true);
				}
			}
			else
			{
				if (m_numVisibleQuadrantOnPerimeterOnEastSide < getMaxDepthQuadOnPerimeter())
				{
					recalcGridOfQuadrantsInViewNeeded(true);
					finishedResizingGrid(false);
					m_numVisibleQuadrantOnPerimeterOnEastSide++;
					worldGridDimensionChanged(true);
				}
			}
		}

		if (recalcGridOfQuadrantsInViewNeeded())
		{
			GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
			if (cam != nullptr)
				cam->forceRecalcCameraFar();
		}

		m_timeElapsedFromLastUpdateGridDimension = TIME()->get_ticks_msec();
	}

	return processed;
}

void GDN_TheWorld_Viewer::process_updateWorldCameraQuadrant(GDN_TheWorld_Globals* globals, godot::Vector3& worldCameraWorldPos)
{
	TimerMcs clock1("GDN_TheWorld_Viewer::_process", "Update Camera Quad", false, false);
	clock1.tick();
	auto save_duration = TheWorld_Viewer_Utils::finally([&clock1, this] {
		clock1.tock();
		this->m_lastUpdateCameraQuad = clock1.duration().count();
		this->m_numUpdateCameraQuad++;
		this->m_sumUpdateCameraQuad += this->m_lastUpdateCameraQuad;
		});

	TheWorld_Utils::QuadrantPos worldCameraQuadrantPos;

	// WARNING
	// Look for Camera QuadTree: it will be used as first element of quadrantPosNeeded -- We try not to look for it in world quadrants map so its pos will be created from camera coords
	//for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
	//{
	//	if (itQuadTree->second->statusToErase())
	//		continue;

	//	float quadrantSizeInWU = itQuadTree->second->getQuadrant()->getPos().getSizeInWU();
	//	if (cameraWorldPos.x >= itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() && cameraWorldPos.x < (itQuadTree->second->getQuadrant()->getPos().getLowerXGridVertex() + quadrantSizeInWU)
	//		&& cameraWorldPos.z >= itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() && cameraWorldPos.z < (itQuadTree->second->getQuadrant()->getPos().getLowerZGridVertex() + quadrantSizeInWU))
	//	{
	//		worldCameraQuadrantPos = itQuadTree->second->getQuadrant()->getPos();
	//		worldCameraQuadrantPos.setTag("Camera");
	//		break;
	//	}
	//}

	if (worldCameraQuadrantPos.empty())	// not found camera-quad-tree ...
	{
		// ... we create new quad tree containing camera
		float x = (float)worldCameraWorldPos.x, z = (float)worldCameraWorldPos.z;
		float _gridStepInWU = globals->gridStepInWU();
		TheWorld_Utils::QuadrantPos q(x, z, m_worldViewerLevel, m_numWorldVerticesPerSize, _gridStepInWU);
		worldCameraQuadrantPos = q;
		worldCameraQuadrantPos.setTag("Camera");
	}

	//
	// now quadrantPosNeeded has 1 element and quadrantPosNeeded[0] is the camera
	//

	// if forced or the camera has changed quadrant the cache is repopulated
	//m_computedWorldCameraQuadrantPos = quadrantPosNeeded[0];	// DEBUG
	if ((m_refreshQuadTreeMap || !m_computedWorldCameraQuadrantPos.isInitialized() || m_computedWorldCameraQuadrantPos != worldCameraQuadrantPos) && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
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
		m_computedWorldCameraQuadrantPos = worldCameraQuadrantPos;

		//GDN_TheWorld_Map* map = Map();
		//if (map != nullptr)
		//{
		//	godot::Vector2 worldCameraQuadrantPosInMap = map->worldPosToGlobalPos(godot::Vector2(m_computedWorldCameraQuadrantPos.getLowerXGridVertex(), m_computedWorldCameraQuadrantPos.getLowerZGridVertex()));
		//	map->setMapGridWorldCameraQuadrantPos(worldCameraQuadrantPosInMap);
		//}

		if ((Globals()->status() != TheWorldStatus::worldDeployed and Globals()->status() != TheWorldStatus::minimalWorldDeployed) || Globals()->getStreamWorld())
			recalcGridOfQuadrantsInViewNeeded(true);
	}

	for (auto& itQuadTree : m_worldQuadrantsMap)
	{
		itQuadTree.second->setQuadrantOptimizations(worldCameraWorldPos);	// Occhio!!! E' stato commentato il 25/11/2024
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

	std::unique_lock<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing, std::try_to_lock);
	if (lock.owns_lock())
	{
		bool allQuadrantInitialized = allQuadrantsInitialized();
		bool allVisibleQuadrantInitialized = allVisibleQuadrantsInitialized();

		{
			if (not allQuadrantInitialized)
			{
				size_t numInitialized = 0;
				for (size_t numQuadrantsFromCameraQuadrant = 0; numQuadrantsFromCameraQuadrant <= getMaxDepthQuadOnPerimeter() + m_numCacheQuadrantOnPerimeter; numQuadrantsFromCameraQuadrant++)
				{
					bool exitForNow = initNewQuadsAtPriority(numQuadrantsFromCameraQuadrant, numInitialized);
					if (exitForNow)
						break;
				}
			}
		}

		lock.unlock();

		if (allVisibleQuadrantInitialized)
		{
			// m_minVisibleQuadrantOnPerimeter == 1
			if (Globals()->status() == TheWorldStatus::worldDeployInProgress && ((m_minVisibleQuadrantOnPerimeter == 0 && m_worldQuadrantsMap.size() == 1) || m_worldQuadrantsMap.size() > 1))
			{
				Globals()->setStatus(TheWorldStatus::minimalWorldDeployed);
				m_refreshRequired = true;
			}
		}
		
		if (allQuadrantInitialized)
		{
			if (m_streamingTime.counterStarted())
			{
				// Initialization of last stream of quadrant COMPLETED
				m_lastCameraAltitudeSetToShader = -1.0f;
				m_lastCameraPosSetToShaderInitialized = false;
				writeStreamingNewQuadrantsStatistics();
				m_streamingTime.tock();
				m_notStreamingTime.tick();
				
				endLastStreamOfQuadrants();
			}
		}
	}
}

bool GDN_TheWorld_Viewer::initNewQuadsAtPriority(size_t numQuadrantsFromCameraQuadrant, size_t& numInitialized)
{
	bool exitForNow = false;
	
	//allQuadrantInitialized = true;
	//allVisibleQuadrantInitialized = true;

	for (auto& itQuadTree : m_worldQuadrantsMapByDistanceFromCameraQuadrant[numQuadrantsFromCameraQuadrant])								// new
	{
		QuadTree* quadTree = itQuadTree.second;
		Quadrant* quadrant = quadTree->getQuadrant();

		quadTree->checkAndInitSeaQuadrant();

		if (quadrant->internalDataLocked())
			continue;

		if (quadTree->statusUninitialized() || quadTree->statusRefreshTerrainDataNeeded())
		{
			//size_t distanceInPerimeterFromCameraQuadrant = quadrant->getPos().distanceInPerimeter(m_computedWorldCameraQuadrantPos);
			if (quadTree->numQuadrantsFromCameraQuadrant() == numQuadrantsFromCameraQuadrant)
			{
				bool proceed = true;
				if (TIME_INTERVAL_BETWEEN_QUADRANT_INIT_AT_LOW_PRIORITY > 0 and quadTree->quadrantDepth() == QuadTree::QuadrantDepth::Cache and finishedResizingGrid())
				{
					int64_t elapsedFromLastQuadInitAtLowPriority = TIME()->get_ticks_msec() - m_timeElapsedFromLastQuadrandtInit;
					if (elapsedFromLastQuadInitAtLowPriority < TIME_INTERVAL_BETWEEN_QUADRANT_INIT_AT_LOW_PRIORITY)
						proceed = false;
				}

				if (proceed)
				{
					m_timeElapsedFromLastQuadrandtInit = TIME()->get_ticks_msec();

					float x = 0, y = 0, z = 0, yaw = 0, pitch = 0, roll = 0;
					if (quadTree->statusUninitialized())
					{
						quadTree->init(x, y, z, yaw, pitch, roll, false, 0.0f);
						//Globals()->debugPrint((std::string("Initializing quadrant: sending data for quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
						PLOG_VERBOSE << "Initializing quadrant: sending data for quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId());
					}
					else	// quadTree->statusRefreshTerrainDataNeeded()
					{
						quadTree->refreshTerrainData(x, y, z, yaw, pitch, roll, false, 0.0f);
						//Globals()->debugPrint((std::string("Refreshing quadrant: sending data for quadrant ") + quadTree->getQuadrant()->getPos().getIdStr() + " Hash=" + TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId())).c_str());
						PLOG_VERBOSE << "Refreshing quadrant: sending data for quadrant " << quadTree->getQuadrant()->getPos().getIdStr() << " Hash=" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(quadTree->getQuadrant()->getMeshId());
					}

					numInitialized++;
					
					if (numInitialized > 0)		// WARN: modifying lag loading!!!
					{
						exitForNow = true;
						break;
					}
				}
			}
		}

		//if (quadTree->statusLoadTerrainDataFromCacheNeeded())
		//{
		//	quadTree->setStatus(QuadrantStatus::loadTerrainDataFromCacheInProgress);
		//	std::function<void(void)> f = std::bind(&Quadrant::loadTerrainDataFromCache, quadTree->getQuadrant());
		//	m_tpClientWorker.QueueJob(f);
		//}
	}

	//allVisibleQuadrantInitialized = allQuadrantInitialized;
	
	return exitForNow;
}

bool GDN_TheWorld_Viewer::allVisibleCollidersReady(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	bool ready = true;
	for (auto& item : m_worldQuadrantsMap)
	{
		if (item.second->quadrantDepth() == QuadTree::QuadrantDepth::Cache)
			continue;

		if (item.second->statusToErase())
			continue;

		if (item.second->colliderNeedReset(QuadType::NotSet))
		{
			ready = false;
			break;
		}
	}

	return ready;
}

bool GDN_TheWorld_Viewer::allCollidersReady(void) const
{
	return m_allCollidersReady;
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

void GDN_TheWorld_Viewer::writeStreamingNewQuadrantsStatistics(void)
{
	size_t num = m_numVisibleQuadrantsStreamed + m_numInvisibleQuadrantsStreamed;
	
	if (!m_streamingTime.counterStarted() && !m_notStreamingTime.counterStarted())
		return;
	
	m_mtxQuadrantsAndMainProcessing.lock();
	size_t numQuadrants = m_worldQuadrantsMap.size();
	m_mtxQuadrantsAndMainProcessing.unlock();

	GDN_TheWorld_Map* map = Map();
	std::string mapQuadrantsMessage = "MAP QUADRANTS\n"
		+ std::string("Num Map Quadrants = ") + std::to_string(m_mapQuadrantImagesMap.size())
		+ std::string(" Num Updated Map Quadrants = ") + std::to_string(fetchUpdatedMapQuadrants())
		+ std::string(" Num Map Texture Rects = ") + std::to_string(m_mapQuadrantRects /*map->getNumTextureRects()*/);

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

godot::Transform3D GDN_TheWorld_Viewer::globalToWorld3d(godot::Transform3D global) const
{
	return globalToWorld(global);
}

godot::Transform3D GDN_TheWorld_Viewer::globalToWorld(godot::Transform3D global) const
{
	// DEBUG
	//if (not IS_EDITOR_HINT())
	//{
	//	godot::Vector3 viewerPos = get_global_position();
	//	godot::Vector3 originShiftPos = getAnchorForOriginShift()->get_global_position();
	//	godot::Vector3 cameraPos = getWorldCamera()->get_global_position();
	//	PLOG_DEBUG << "globalToWorld: viewerPos = " << viewerPos.x << "," << viewerPos.y << "," << viewerPos.z
	//		<< " originShiftPos = " << originShiftPos.x << "," << originShiftPos.y << "," << originShiftPos.z
	//		<< " cameraPos = " << cameraPos.x << "," << cameraPos.y << "," << cameraPos.z;
	//}
	// DEBUG
	
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	return viewerT * global;
}

godot::Transform2D GDN_TheWorld_Viewer::globalToWorld2d(godot::Transform2D global)
{
	return globalToWorld(global);
}

godot::Transform2D GDN_TheWorld_Viewer::globalToWorld(godot::Transform2D global)
{
	//godot::Transform3D viewerT3d = getInternalGlobalTransform().affine_inverse();
	//godot::Transform2D viewerT2d;
	//viewerT2d.set_origin(godot::Vector2(viewerT3d.get_origin().x, viewerT3d.get_origin().z));
	//return viewerT2d * global;
	GDN_TheWorld_Map* map = Map();
	godot::Transform2D viewerT2d = map->get_transform();
	viewerT2d.set_origin(globalPosToWorldPos(global.get_origin()));
	return viewerT2d;
}

godot::Transform3D GDN_TheWorld_Viewer::globalPosToWorldTransform(godot::Vector3 global) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	return viewerT * godot::Transform3D(godot::Basis(), global);
}

godot::Vector3 GDN_TheWorld_Viewer::globalPosToWorldPos3d(godot::Vector3 global) const
{
	return globalPosToWorldPos(global);
}

godot::Vector3 GDN_TheWorld_Viewer::globalPosToWorldPos(godot::Vector3 global) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform().affine_inverse();
	godot::Transform3D t = viewerT * godot::Transform3D(godot::Basis(), global);
	return t.origin;
}

godot::Vector2 GDN_TheWorld_Viewer::globalPosToWorldPos2d(godot::Vector2 global)
{
	return globalPosToWorldPos(global);
}

godot::Vector2 GDN_TheWorld_Viewer::globalPosToWorldPos(godot::Vector2 global)
{
	GDN_TheWorld_Map* map = Map();
	return map->globalPosToWorldPos(global);
}

godot::Transform3D GDN_TheWorld_Viewer::worldToGlobal(godot::Transform3D world) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	return viewerT * world;
}

godot::Transform3D GDN_TheWorld_Viewer::worldPosToGlobalTransform(godot::Vector3 worldPos) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	return viewerT * godot::Transform3D(godot::Basis(), worldPos);
}

godot::Vector3 GDN_TheWorld_Viewer::worldPosToGlobalPos3d(godot::Vector3 world) const
{
	return worldPosToGlobalPos(world);
}

godot::Vector3 GDN_TheWorld_Viewer::worldPosToGlobalPos(godot::Vector3 world) const
{
	godot::Transform3D viewerT = getInternalGlobalTransform();
	godot::Transform3D t = viewerT * godot::Transform3D(godot::Basis(), world);
	return t.origin;
}

godot::Vector2 GDN_TheWorld_Viewer::worldPosToGlobalPos2d(godot::Vector2 worldPos)
{
	return worldPosToGlobalPos(worldPos);
}

godot::Vector2 GDN_TheWorld_Viewer::worldPosToGlobalPos(godot::Vector2 worldPos)
{
	GDN_TheWorld_Map* map = Map();
	return map->worldPosToGlobalPos(worldPos);
}

void GDN_TheWorld_Viewer::process_updateOriginShift(godot::Camera2D* activeMapCamera, godot::Camera3D* activeWorldCamera)
{
	//return;
	GDN_TheWorld_Globals* globals = Globals();

	int64_t paramOriginShiftFactor = globals->getParamOriginShiftFactor();
	if (paramOriginShiftFactor == 0)
		return;

	if (IS_EDITOR_HINT())		// we can't use origin shift in editor because of the way editor cameras work (we should manually move editor camera accordingly to the ancjor but anycase the anchor will save its new position in the project
		return;					// instead being reset at every run). Anycase it will generate confusion in the editing work
	
	godot::Node3D* anchorForOriginShift = getAnchorForOriginShift();
	if (anchorForOriginShift == nullptr)
		return;

	if (anchorForOriginShift == this)	// origin shift anchor atm must be a node3d parent of the viewer and the camera (because when anchor is relocated both viewer and camera must relocate accordingly), atm the viewer
		return;							// is not parent of the camera so to anable origin shift an external node must be set using setAnchorForOriginShift

	if (globals->status() != TheWorldStatus::worldDeployed and globals->status() != TheWorldStatus::minimalWorldDeployed)
		return;
	
	if (worldStreamingInProgress())
		return;

	float distance;
	godot::Transform3D cameraWorldTransform;
	godot::Vector2 cameraWorldPos;
	if (worldMode())
	{
		cameraWorldTransform = globalPosToWorldTransform(activeWorldCamera->get_global_position() * godot::Vector3(1, 0, 1));
		distance = cameraWorldTransform.origin.distance_to(m_lastOriginShiftCameraPosInMap);
	}
	else
	{
		godot::Vector2 lastOriginShiftCameraPosInMap(m_lastOriginShiftCameraPosInMap.x, m_lastOriginShiftCameraPosInMap.z);
		cameraWorldPos = globalPosToWorldPos(activeMapCamera->get_global_position());
		distance = cameraWorldPos.distance_to(lastOriginShiftCameraPosInMap);
	}

	//if (m_originShiftedQuadrantPos != m_computedWorldCameraQuadrantPos)
	if (distance > globals->heightmapSizeInWUs() * paramOriginShiftFactor)
	{
		godot::Vector3 originShift;
		originShift.x = -m_computedWorldCameraQuadrantPos.getLowerXGridVertex();
		originShift.y = 0;
		originShift.z = -m_computedWorldCameraQuadrantPos.getLowerZGridVertex();

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

		m_originShiftedQuadrantPos = m_computedWorldCameraQuadrantPos;
		if (worldMode())
			m_lastOriginShiftCameraPosInMap = cameraWorldTransform.origin;
		else
			m_lastOriginShiftCameraPosInMap = godot::Vector3(cameraWorldPos.x, 0, cameraWorldPos.y);

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
	m_savedTimeToFadeHudMessage = 0;
	if (m_fadingTime > 0)
		m_savedTimeToFadeHudMessage = TIME()->get_ticks_msec();
}

void GDN_TheWorld_Viewer::process_updateHud(godot::Camera3D* activeWorldCamera)
{
	GDN_TheWorld_Globals* globals = Globals();

	if (m_hudMessageLabel != nullptr)
	{
		//std::string currentHudMessage = m_hudMessageLabel->get_text().utf8().get_data();

		if (m_savedTimeToFadeHudMessage > 0)
		{
			if (TIME()->get_ticks_msec() - m_savedTimeToFadeHudMessage > m_fadingTime)
			{
				m_hudMessage = "";
				m_savedTimeToFadeHudMessage = 0;
			}
		}

		m_hudMessageLabel->set_text(m_hudMessage.c_str());
	}

	if (m_hudStreamingInfoLabel != nullptr /* and (globals->statusWorldDeployInProgress() or globals->statusMinimalWorldDeployed() or globals->statusWorldDeployed())*/)
	{
		std::string hudStreaming = m_hudStreamingInfoLabel->get_text().utf8().get_data();
		int numQuadsToRefreshToShader = getNumNeedShaderResetQuadrants() + getNumNeedShaderUpdateQuadrants();
		int numQuadrant = getNumQuadrants() - getNumToEraseQuadrants();
		int numQuadsToInitialize = numQuadrant - getNumInitializedQuadrants();
		int averageQuadsToProcess = (numQuadsToRefreshToShader + numQuadsToInitialize) / 2;
		int maxNumQuadrant = getMaxNumQuadrants();
		if (numQuadrant == 0 or averageQuadsToProcess == 0)
		{
			if (m_streamingMessageOn and (globals->statusMinimalWorldDeployed() or globals->statusWorldDeployed()))
			{
				if (m_savedTimeToFadeStreamingMessage == 0)
				{
					int averageQuadsProcessed = maxNumQuadrant - averageQuadsToProcess;
					int totalProgress = averageQuadsProcessed * 100 / maxNumQuadrant;
					hudStreaming = "Streaming World " + std::to_string(totalProgress) + "%)";
					m_savedTimeToFadeStreamingMessage = TIME()->get_ticks_msec();
				}
				else
				{
					if (m_savedTimeToFadeStreamingMessage == 0 or TIME()->get_ticks_msec() - m_savedTimeToFadeStreamingMessage > 1000)
					{
						hudStreaming = "";
						m_savedTimeToFadeStreamingMessage = 0;
						m_streamingMessageOn = false;
					}
				}
			}
			else
			{
				hudStreaming = "";
				m_savedTimeToFadeStreamingMessage = 0;
				m_streamingMessageOn = false;
			}
		}
		else
		{
			int averageQuadsProcessed = maxNumQuadrant - averageQuadsToProcess;
			int totalProgress = averageQuadsProcessed * 100 / maxNumQuadrant;
			hudStreaming = "Streaming World " + std::to_string(totalProgress) + "%)";
			m_savedTimeToFadeStreamingMessage = 0;
			m_streamingMessageOn = true;
		}
		if (m_hudStreamingInfoLabel->get_text().length() == 0)
			m_hudStreamingInfoLabel->set_text("");
		if (hudStreaming.length() == 0)
			m_hudStreamingInfoLabel->set_text("");
		m_hudStreamingInfoLabel->set_text(hudStreaming.c_str());

		float delayDegree = getDelayDegree();
		//delayDegree = delayDegree < 1 ? 1 : delayDegree > 5 ? 20 : delayDegree;
		delayDegree = std::clamp(delayDegree, 1.0f, 20.0f);
		Color c = TheWorld_Viewer_Utils::Utils::mix<godot::Color>(GDN_TheWorld_Globals::g_color_red, GDN_TheWorld_Globals::g_color_green, 1 / delayDegree);
		m_hudStreamingInfoLabel->add_theme_color_override("font_color", c);
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
			m_satelliteCamera->set_fov(float(fov));
		}
	}
}

void GDN_TheWorld_Viewer::process_updateCameraPosInShader(godot::Camera3D* activeWorldCamera)
{
	godot::RenderingServer* rs = godot::RenderingServer::get_singleton();

	GDN_TheWorld_Globals* globals = Globals();
	
	GDN_TheWorld_Map* map = Map();
	if (map != nullptr and worldGridDimensionChanged())
	{
		//map->setMapGridNumVisibleQuadrantsOnNorthSide(m_numVisibleQuadrantOnPerimeterOnNorthSide);
		//map->setMapGridNumVisibleQuadrantsOnSouthSide(m_numVisibleQuadrantOnPerimeterOnSouthSide);
		//map->setMapGridNumVisibleQuadrantsOnWestSide(m_numVisibleQuadrantOnPerimeterOnWestSide);
		//map->setMapGridNumVisibleQuadrantsOnEastSide(m_numVisibleQuadrantOnPerimeterOnEastSide);
		//map->setMapGridNumCacheQuadrants(m_numCacheQuadrantOnPerimeter);
		worldGridDimensionChanged(false);
	}
	
	if (globals->status() != TheWorldStatus::worldDeployed and globals->status() != TheWorldStatus::minimalWorldDeployed)
		return;

	//GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(activeWorldCamera);
	Camera3D* customCamera = getCustomCamera();
	GDN_TheWorld_Camera* cam = godot::Object::cast_to<GDN_TheWorld_Camera>(customCamera);
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

		//godot::Vector3 cameraPos = cam->get_global_position();
		//if (!m_lastCameraPosSetToShaderInitialized || cameraPos != m_lastCameraPosSetToShader)
		//{
		//	rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_POS, cameraPos);
		//	m_lastCameraPosSetToShader = cameraPos;
		//	m_lastCameraPosSetToShaderInitialized = true;
		//}

		bool cameraTerrainChunkContainsSea = false;
		if (m_worldCameraTerrainChunk != nullptr)
			cameraTerrainChunkContainsSea = m_worldCameraTerrainChunk->containsSea();
		if (!m_cameraVerticalOnSeaSet or m_cameraVerticalOnSea != cameraTerrainChunkContainsSea)
		{
			m_cameraVerticalOnSea = cameraTerrainChunkContainsSea;
			m_cameraVerticalOnSeaSet = true;
			//rs->global_shader_parameter_set(GLOBAL_SHADER_PARAM_CAM_ON_SEA, m_cameraVerticalOnSea);
			godot::Ref<godot::ShaderMaterial> mat = m_postProcessingUnderWaterEffectMesh->get_surface_override_material(0);
			if (mat.is_valid())
			{
				mat->set_shader_parameter(SHADER_PARAM_CAM_ON_SEA, m_cameraVerticalOnSea);
			}
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
			if (!showPositionOnCursorHit())
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
		if (trackMouse())
		{
			m_mouseTrackedOnSomething = false;

			m_mouseQuadrantHitTag = "";

			GDN_TheWorld_Map* map = Map();
			if (map != nullptr)
			{
				godot::Vector2 mouseGlobalPos = map->getMousePosInGlobalCoord();
				m_trackedMouseHitGlobalPos = godot::Vector3(mouseGlobalPos.x, 0, mouseGlobalPos.y);

				godot::Vector2 mouseWorldPos = map->globalPosToWorldPos(mouseGlobalPos);
				m_trackedMouseHitWorldPos = godot::Vector3(mouseWorldPos.x, 0, mouseWorldPos.y);

				int numVerticesPerQuadrantSize = (int)Globals()->numVerticesPerQuadrantSize();
				float gridStepInWU = Globals()->gridStepInWU();
				TheWorld_Utils::QuadrantPos pos(mouseWorldPos.x, mouseWorldPos.y, m_worldViewerLevel, numVerticesPerQuadrantSize, gridStepInWU);
				m_mouseQuadrantHitName = pos.getName();
				m_mouseQuadrantHitPos = godot::Vector3(pos.getLowerXGridVertex(), 0, pos.getLowerZGridVertex());

				GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
				if (m_editMode && editModeUIControl && editModeUIControl->initialized())
				{
					editModeUIControl->setMouseHitPos(m_trackedMouseHitWorldPos);
					editModeUIControl->setMouseQuadHitLabelText(m_mouseQuadrantHitName);
					editModeUIControl->setMouseQuadHitPosLabelText(std::string("X=") + std::format("{:.2f}", m_mouseQuadrantHitPos.x) + " Z=" + std::format("{:.2f}", m_mouseQuadrantHitPos.z) + " " + std::format("{:.2f}", pos.getSizeInWU()));
					QuadTree* quadrantHit = nullptr;
					if (m_worldQuadrantsMap.contains(pos))
						quadrantHit = m_worldQuadrantsMap[pos].get();
					editModeUIControl->setQuadrantHit(quadrantHit);
				}
			}
		}
	}
	else
	{
		godot::Node* collider = nullptr;
		if (trackMouse())
		{
			compressTrackedInputEvents(&m_trackedInputEventsForTrackMouse, &m_trackedInputEventsQueueForTrackMouse);

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
					m_trackedMouseHitGlobalPos = rayArray["position"];
					godot::Transform3D t(godot::Basis(), m_trackedMouseHitGlobalPos);
					m_trackedMouseHitWorldPos = globalToWorld(t).origin;
					m_mouseTrackedOnSomething = true;
				}
				else
					m_mouseTrackedOnSomething = false;

				if (m_mouseTrackedOnSomething)
				{
					if (m_editMode && editModeUIControl && editModeUIControl->initialized())
					{
						editModeUIControl->setMouseHitPos(m_trackedMouseHitWorldPos);
					}

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
									trackedMouseHitChanged(activeWorldCamera, quadrantHit, adjacentQuadrantsHit, true, m_trackedMouseHitWorldPos, normalHit);
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
								trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, true, m_trackedMouseHitWorldPos, normalHit);
							}
						}
						else
						{
							trackedMouseHitChanged(activeWorldCamera, m_quadrantHit, m_adjacentQuadrantsHit, true, m_trackedMouseHitWorldPos, normalHit);
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
					m_previousTrackedMouseHitWorldPos = godot::Vector3();
				}

				resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse, &m_trackedInputEventsQueueForTrackMouse);

				m_timeElapsedFromLastMouseTrack = timeElapsed;
			}
		}
		else
		{
			resetTrackedInputEvents(&m_trackedInputEventsForTrackMouse, &m_trackedInputEventsQueueForTrackMouse);

			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (m_editMode && editModeUIControl && editModeUIControl->initialized())
			{
				editModeUIControl->setQuadrantHit(nullptr);
			}
		}
	}
}

void GDN_TheWorld_Viewer::trackedMouseHitChanged(godot::Camera3D* activeWorldCamera, QuadTree* quadrantHit, std::list<QuadTree*>& adjacentQuadrantsHit, bool hit, godot::Vector3 mouseHitWorldPos, godot::Vector3 normalHit)
{

	//if (quadrantHit != nullptr)
	//{
		if (hit)
		{
			if (showPositionOnCursorHit())
			{
				if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT, nullptr, &m_trackedInputEventsForTrackMouse))
					trackingDistance(true, mouseHitWorldPos);
				if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT, nullptr, &m_trackedInputEventsForTrackMouse))
					trackingDistance(false);
			}

			if (highlightHitQuadrant())
			{
				if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_LEFT, nullptr, &m_trackedInputEventsForTrackMouse))
				{
					if (quadrantHit != nullptr)
						selectWorldQuadrant(quadrantHit->getQuadrant()->getPos());
				}
				if (isInputEventTracked(TrackedInputEvent::TrackedInputEventType::MOUSE_DOUBLE_BUTTON_RIGHT, nullptr, &m_trackedInputEventsForTrackMouse))
				{
					if (quadrantHit != nullptr)
						deselectWorldQuadrant();
				}
			}

			if (m_previousTrackedMouseHitWorldPos != mouseHitWorldPos)
			{
				if (quadrantHit != nullptr)
					quadrantHit->mouseHitOnTerrainChanged(mouseHitWorldPos, true);
				for (auto& item : adjacentQuadrantsHit)
					item->mouseHitOnTerrainChanged(mouseHitWorldPos, true);

				bool _normalVisible = false;
				if (normalVisible())
				{
					godot::Vector3 normal = normalHit;
					if (normal == Vector3Zero && quadrantHit != nullptr)
					{
						size_t index = quadrantHit->getQuadrant()->getIndexFromMap((float)mouseHitWorldPos.x, (float)mouseHitWorldPos.z);
						if (index != -1)
						{
							bool ok;
							normal = quadrantHit->getQuadrant()->getNormal(index, ok);
							if (!ok)
								normal = Vector3Zero;
						}
					}
					if (normal != Vector3Zero)
					{
						godot::Vector3 cameraPosInMap = globalPosToWorldPos(activeWorldCamera->get_global_position());
						float distanceFromCamera = (float)cameraPosInMap.distance_to(mouseHitWorldPos);

						const float lineSize = 50 * distanceFromCamera / 500; // Adjust the line size based on distance (you can tweak the divisor for different scaling)
						godot::Vector3 end = mouseHitWorldPos + normal * lineSize;
						float radAngleWithVertical = (float)acos(normal.dot(Vector3Up));
						float slope = (radAngleWithVertical * 180) / TheWorld_Utils::kPi;
						const float radius = lineSize / 10;
						//float label3dOffset = distanceFromCamera > 2000 ? 50.0f : distanceFromCamera > 1000 ? 5.0f : 3.0f;
						float label3dOffset = lineSize / 5;

						if (m_normalLineIdx != -1 && m_normalSphereIdx != -1)
						{
							_normalVisible = true;
							m_normalDrawer->updateLine(m_normalLineIdx, mouseHitWorldPos, end, std::to_string(int32_t(slope)) + "°", GDN_TheWorld_Drawer::Drawing::LabelPos::end, label3dOffset);
							m_normalDrawer->updateSphere(m_normalSphereIdx, end, radius);
						}
					}
					
					//size_t index = quadrantHit->getQuadrant()->getIndexFromMap(mouseHitWorldPos.x, mouseHitWorldPos.z);
					//if (index != -1)
					//{
					//	bool ok;
					//	godot::Vector3 normal = quadrantHit->getQuadrant()->getNormalFromNormalmap(index, ok);
					//	if (ok)
					//	{
					//		const float lineSize = 50;
					//		godot::Vector3 end = mouseHitWorldPos + normal * lineSize;
					//		float radAngleWithVertical = acos(normal.dot(Vector3Up));
					//		float slope = (radAngleWithVertical * 180) / TheWorld_Utils::kPi;
					//		
					//		if (m_normalLineIdx != -1 && m_normalSphereIdx != -1)
					//		{
					//			_normalVisible = true;
					//			m_normalDrawer->updateLine(m_normalLineIdx, mouseHitWorldPos, end, std::to_string(int32_t(slope)) + "°");
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
				if (showPositionOnCursorHit())
				{
					if (m_labelPositionXIdx != -1 && m_labelPositionYIdx != -1 && m_labelPositionZIdx != -1 && m_labelPositionDistIdx != -1 && m_labelPositionDirIdx != -1)
					{
						_positionVisible = true;

						m_positionDrawer->updateLabel2d(m_labelPositionXIdx, mouseHitWorldPos, std::string("X=") + std::format("{:.2f}", mouseHitWorldPos.x));
						m_positionDrawer->updateLabel2d(m_labelPositionZIdx, mouseHitWorldPos, std::string("Z=") + std::format("{:.2f}", mouseHitWorldPos.z));
						m_positionDrawer->updateLabel2d(m_labelPositionYIdx, mouseHitWorldPos, std::string("H=") + std::format("{:.2f}", mouseHitWorldPos.y));

						godot::Vector3 cameraPosInMap = globalPosToWorldPos(activeWorldCamera->get_global_position());
						float distanceFromCamera = (float)cameraPosInMap.distance_to(mouseHitWorldPos);
						float distanceFromCameraInQuads = distanceFromCamera / Globals()->heightmapSizeInWUs();
						m_positionDrawer->updateLabel2d(m_labelPositionDistIdx, mouseHitWorldPos, std::string("Dist=") + std::format("{:.2f}", distanceFromCamera) + "m (" + std::format("{:.2f}", distanceFromCamera/1000) + "km) quads=" + std::format("{:.2f}", distanceFromCameraInQuads));

						godot::Vector3 hitDirection = (mouseHitWorldPos - cameraPosInMap).normalized();
						godot::Vector3 northDirection(0, 0, -1);
						float degreeWithNorthDirection = (northDirection.angle_to(godot::Vector3(hitDirection.x, 0, hitDirection.z)) * 180) / TheWorld_Utils::kPi;
						if (mouseHitWorldPos.x < 0)
							degreeWithNorthDirection = 360 - degreeWithNorthDirection;

						godot::Vector3 upDirection(0, 1, 0);
						float pitch = 90 - (upDirection.angle_to(hitDirection) * 180) / TheWorld_Utils::kPi;

						std::string text = "Deg=" + std::format("{:.2f}", degreeWithNorthDirection) + "° pitch=" + std::format("{:.2f}", pitch) + "°";
						m_positionDrawer->updateLabel2d(m_labelPositionDirIdx, mouseHitWorldPos, text);
					}

					if (trackingDistance())
					{
						float distance = (float)mouseHitWorldPos.distance_to(m_savedPosForTrackingDistance);
						float distanceInQuads = distance / Globals()->heightmapSizeInWUs();
						std::string text = "Dist=" + std::format("{:.2f}", distance) + "m (" + std::format("{:.2f}", distance / 1000)  + "km) quads=" + std::format("{:.2f}", distanceInQuads);
						m_positionDrawer->updateLine(m_linePositionIdx, m_savedPosForTrackingDistance, mouseHitWorldPos, text);
						m_positionDrawer->updateSphere(m_spherePositionStartIdx, m_savedPosForTrackingDistance, 5);
						m_positionDrawer->updateSphere(m_spherePositionEndIdx, mouseHitWorldPos, 5);
					}
				}

				if (_positionVisible)
					m_positionDrawer->set_visible(true);
				else
					m_positionDrawer->set_visible(false);

				if (gizmoVisible())
				{
					m_gizmo->set_visible(true);
					m_gizmo->set_global_position(worldPosToGlobalPos(mouseHitWorldPos));
				}
				else
					m_gizmo->set_visible(false);

				m_previousTrackedMouseHitWorldPos = mouseHitWorldPos;
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

void GDN_TheWorld_Viewer::process_mapQuadrants(godot::Vector2 mapCameraWorldPos)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1. ") + __FUNCTION__, "ALL");

	GDN_TheWorld_Map* map = Map();
	if (map != nullptr)
	{
		m_mapQuadrantImagesMap.lock();

		// Check quadrant update request timed out
		constexpr int64_t  secondsBetweenCheck = 1;
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastUpdatingQuadrantsCheck > secondsBetweenCheck * 1000)
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.1. ") + __FUNCTION__, "Check All Rects Rendered");

			// Erasing MapQuadrants with refreshTime less than textureRectRefreshTime (the updated time on disk of the texture png)
			{
				std::map<TheWorld_Utils::QuadrantPos, bool> quadrantsPosToErase;
				for (auto& item : m_mapQuadrantImagesMap)
				{
					MapQuadrantImage* mapQuadrant = item.second.get();
					std::time_t quadrantRefreshTime = mapQuadrant->getRefreshTime();
					if (quadrantRefreshTime > 0)
					{
						std::time_t mapRectTextureRefreshTime = 0;

						TheWorld_Utils::QuadrantPos quadrantPos = mapQuadrant->getPos();
						GDN_TheWorld_Map::MapTextureRect* mapTextureRect = map->getMapTextureRect(quadrantPos, false);
						if (mapTextureRect == nullptr)
							quadrantsPosToErase[quadrantPos] = true;	// if quadrant belongs to e texture rect not in map (texture rect no more in screen) we can delete it
						if (mapTextureRect != nullptr and !mapTextureRect->fileTimeStr().empty())
							mapRectTextureRefreshTime = TheWorld_Utils::str_time_to_time_t(mapTextureRect->fileTimeStr());

						if (quadrantRefreshTime < mapRectTextureRefreshTime)
							quadrantsPosToErase[quadrantPos] = true;
					}
					else if (quadrantRefreshTime == 0)	// Empty quadrant
					{
						TheWorld_Utils::QuadrantPos quadrantPos = mapQuadrant->getPos();
						GDN_TheWorld_Map::MapTextureRect* mapTextureRect = map->getMapTextureRect(quadrantPos, false);
						if (mapTextureRect == nullptr)
							quadrantsPosToErase[quadrantPos] = true;	// if empty quadrant and it's not in map (texture rect no more in screen) we can delete it
						if (mapTextureRect != nullptr and !mapTextureRect->fileTimeStr().empty())
							quadrantsPosToErase[quadrantPos] = true;	// if empty quadrant and its map texture rect is rendered we can delete it
					}
				}
				
				for (auto& [pos, dummy] : m_mapQuadrantImagesToDeleteMap)	// add previously accumulated quadrants to delete (those are quadrants that were not deleted in previous checks because they were still in use by the map)
				{
					quadrantsPosToErase[pos] = true;
				}
				m_mapQuadrantImagesToDeleteMap.clear();
				
				for (auto& [pos, dummy] : quadrantsPosToErase)
				{
					bool canBeErased = true;
					if (m_mapQuadrantImagesMap.contains(pos))
						if (not m_mapQuadrantImagesMap[pos]->lockInternalData())
							canBeErased = false;	// if we can't lock the internal data of the quadrant it means that it's still in use by the map (map is reading data from server to update texture rects) so we can't delete it now
					if (canBeErased)
						m_mapQuadrantImagesMap.eraseByValue(pos);
					else
						m_mapQuadrantImagesToDeleteMap[pos] = true;	// if the quadrant is still in use by the map we will try to delete it later (in next checks)
				}
				
				m_mapQuadrantImagesMap.clearThrash();
			}

			bool statusChanged = m_mapQuadrantImagesMap.checkUpdatingQuadrants();
			if (statusChanged)
				forceProcessNewMapQuadrants(true);

			bool allTextureRendered = map->eraseNotInScreenTextureRects();
			if (not allTextureRendered)
				forceProcessNewMapQuadrants(true);

			m_timeElapsedFromLastUpdatingQuadrantsCheck = timeElapsed;
		}

		// Request update for new quadrants (when map camera moves / scales)
		if (forceProcessNewMapQuadrants()
			or m_mapQuadrantImagesMap.updated()
			or m_lastProcessedCameraForMapQuadrants == godot::Vector2()
			or (m_lastProcessedCameraForMapQuadrants != mapCameraWorldPos))
		{
			TheWorld_Utils::GuardProfiler profiler(std::string("process_mapQuadrants 1.2. ") + __FUNCTION__, "Acq. and render texture");

			bool proceed = true;
			GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
			if (editModeUIControl != nullptr and editModeUIControl->initialized())
			{
				if (editModeUIControl->uploadToServerInProgress())
					proceed = false;
			}

			if (proceed)
			{
				bool breakLoop = false;

				int numVerticesPerQuadrantSize = (int)Globals()->numVerticesPerQuadrantSize();
				float gridStepInWU = Globals()->gridStepInWU();

				std::vector<TheWorld_Utils::MapTextureRectPos> rectsPos;
				map->getScreenTextureRects(rectsPos);

				// get quadrants for not cached map textures
				for (auto& rectPosItem : rectsPos)
				{
					bool fullRectRendered = false;

					std::vector<TheWorld_Utils::QuadrantPos> quadrantsPos;
					rectPosItem.getAllContainedQuadrantPos(quadrantsPos, m_worldViewerLevel, numVerticesPerQuadrantSize, gridStepInWU);

					bool rendered = map->renderInternalTexture(rectPosItem);
					if (rendered)
					{
						fullRectRendered = true;
					}
					else
					{
						if (not map->isTextureRectRendered(rectPosItem))
						{
							rendered = map->renderCachedTexture(rectPosItem);
							if (rendered)
							{
								fullRectRendered = true;
							}
							else
							{
								if (map->CheckFromServerNotDone(rectPosItem))
								{
									map->setStatus(rectPosItem, GDN_TheWorld_Map::MapTextureRect::Status::CheckFromServerRequired);
								}
							}
						}
					}

					if (not map->CheckFromServerInProgress(rectPosItem) and not map->CheckFromServerRequired(rectPosItem))
					{
						if (not map->isTextureRectRendered(rectPosItem) or map->AcqQuadrantsNeeded(rectPosItem) or map->AcqQuadrantsInProgress(rectPosItem))
						{
							map->setStatus(rectPosItem, GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsInProgress);

							std::vector<TheWorld_Utils::QuadrantPos> quadrantsPos;
							rectPosItem.getAllContainedQuadrantPos(quadrantsPos, m_worldViewerLevel, numVerticesPerQuadrantSize, gridStepInWU);
							for (auto& quadrantPosItem : quadrantsPos)
							{
								if (m_mapQuadrantImagesMap.contains(quadrantPosItem))
								{
									MapQuadrantImage* mapQuadrant = m_mapQuadrantImagesMap[quadrantPosItem].get();
									if (mapQuadrant->updated())
									{
										//PLOG_DEBUG << "#################### mapQuadrant->updated() - Pos: " << mapQuadrant->getPos().getIndexPosX() << "," << mapQuadrant->getPos().getIndexPosZ();
										bool b = map->setMapQuadrantData(mapQuadrant->getMapImage(), quadrantPosItem, TheWorld_Utils::MeshCacheBuffer::getTimeFromMeshId(mapQuadrant->getMeshId()));
										if (b)
											fullRectRendered = true;
										mapQuadrant->updated(false);
										map->removeRefreshingMapQuadrantOverride(quadrantPosItem);
									}
									else
									{
										if (m_mapQuadrantImagesMap.updateNeeded(quadrantPosItem))
										{
											bool scheduled = m_mapQuadrantImagesMap.scheduleQuadrantForUpdate(quadrantPosItem);
											if (scheduled)
											{
												map->addRefreshingMapQuadrantOverride(quadrantPosItem);
												breakLoop = true;
												//PLOG_DEBUG << "#################### map->addRefreshingMapQuadrantOverride - Pos: " << mapQuadrant->getPos().getIndexPosX() << "," << mapQuadrant->getPos().getIndexPosZ();
											}
										}
									}
								}
								else
								{
									bool scheduled = m_mapQuadrantImagesMap.scheduleQuadrantForUpdate(quadrantPosItem);
									if (scheduled)
									{
										map->addRefreshingMapQuadrantOverride(quadrantPosItem);
										breakLoop = true;
										//PLOG_DEBUG << "#################### map->addRefreshingMapQuadrantOverride - Pos: " << quadrantPosItem.getIndexPosX() << "," << quadrantPosItem.getIndexPosZ();
									}
								}
							}
						}
					}

					if (fullRectRendered)
					{
						//atLeastAFullRectRendered = true;
						for (auto& pos : quadrantsPos)
						{
							m_mapQuadrantImagesToDeleteMap[pos] = true;
							//m_mapQuadrantImagesMap.eraseByRef(pos);
						}
						if (map->AcqQuadrantsNeeded(rectPosItem) or map->AcqQuadrantsInProgress(rectPosItem))
							map->setStatus(rectPosItem, GDN_TheWorld_Map::MapTextureRect::Status::AcqQuadrantsDone);
					}

					if (breakLoop)
						break;
				}

				m_lastProcessedCameraForMapQuadrants = mapCameraWorldPos;

				if (breakLoop)
					forceProcessNewMapQuadrants(true);
				else
					forceProcessNewMapQuadrants(false);
			}
		}

		m_mapQuadrantImagesMap.unlock();
	}
}

void GDN_TheWorld_Viewer::process_materialParams(godot::Vector3& worldCameraWorldPos, bool processCameraPosForQuadrants, bool processCameraPosForChunks)
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

	if (getWorldCamera2(true) == nullptr)
		return;
	
	if (ReloadSelectedQuadrantShaderRequired())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 1 ") + __FUNCTION__, "ReloadSelectedQuadrantShaderRequired");

		QuadTree* quadTreeSel = nullptr;
		TheWorld_Utils::QuadrantPos quadrantSelPos = getWorldQuadrantSelForEdit(&quadTreeSel);

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
		
		//int32_t numMaterialParamReset = 0;
		//int32_t numMaterialParamUpdate = 0;
		//int32_t numColliderUpdate = 0;

		int32_t numLoop = 0;	 int32_t maxLoop = 10;
		bool loopCompleted = true;
		bool exitLoopRequired = false;
		for (auto& it : m_worldQuadrantsMapByDistanceFromCameraQuadrant)
		{
			for (auto& itQuadTree : it.second)
			{
				QuadTree* quadTree = itQuadTree.second;
				Quadrant* quadrant = quadTree->getQuadrant();


				//if (processCameraPosForChunks || quadTree->forceRecalcOptimizationLevel() || !finishedResizingGrid())	// occhio!!!
				//	quadTree->setQuadrantOptimizations(worldCameraWorldPos);

				bool materialParamsReset = false;
				bool materialParamsUpdated = false;
				bool colliderUpdated = false;

				bool locked = quadrant->lockInternalData();
				if (locked)
				{
					//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1 ") + __FUNCTION__, "Quadrant locked");

					process_neighbourQuadrantData(quadTree);

					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.1 ") + __FUNCTION__, "resetMaterialParams");
						materialParamsReset = quadTree->resetMaterialParams(m_lastProcessExecutionInterrupted, worldCameraWorldPos);
						//if (materialParamsReset)
						//	numMaterialParamReset++;
						if (m_lastProcessExecutionInterrupted)
						{
							quadrant->unlockInternalData();
							loopCompleted = false;
							exitLoopRequired = true;
							break;
						}
						//if (materialParamsReset)			// DEBUG
						//	materialParamsReset = true;		// DEBUG
					}

					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.2 ") + __FUNCTION__, "updateMaterialParams");
						materialParamsUpdated = quadTree->updateMaterialParams();
						//if (materialParamsUpdated)
						//	numMaterialParamUpdate++;
						if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_updateMaterialParams))
						{
							m_lastProcessExecutionInterrupted = true;
							quadrant->unlockInternalData();
							loopCompleted = false;
							exitLoopRequired = true;
							break;
						}
						//if (materialParamsUpdated)		// DEBUG
						//	materialParamsUpdated = true;	// DEBUG
					}

					bool colliderCanHaveHiResCollisionInfo = quadrant->canHaveHiResCollisionInfo();
					if (!quadrant->getCollider()->hasData() ||
						(colliderCanHaveHiResCollisionInfo && !quadrant->getCollider()->hasHiResData()) || (!colliderCanHaveHiResCollisionInfo && quadrant->getCollider()->hasHiResData()))
						processCameraPosForQuadrants = true;

					if (quadTree->colliderNeedReset(QuadType::NotSet) || processCameraPosForQuadrants)
					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.3 ") + __FUNCTION__, "updateCollider");
						colliderUpdated = quadTree->updateCollider(QuadType::NotSet);
						//if (colliderUpdated)
						//	numColliderUpdate++;
						if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_updateCollider))
						{
							m_lastProcessExecutionInterrupted = true;
							quadrant->unlockInternalData();
							loopCompleted = false;
							exitLoopRequired = true;
							break;
						}
						//if (colliderUpdated)		// DEBUG
						//	colliderUpdated = true;	// DEBUG
					}

					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.4 ") + __FUNCTION__, "releaseTerrainValuesMemory");
						quadrant->releaseTerrainValuesMemory(quadTree->isVisible());
					}

					{
						//TheWorld_Utils::GuardProfiler profiler(std::string("process_materialParam 4.1.5 ") + __FUNCTION__, "unlockInternalData");
						quadrant->unlockInternalData();
					}
				}
				else
					loopCompleted = false;

				if (materialParamsUpdated)
					m_previousTrackedMouseHitWorldPos = godot::Vector3();

				//if (materialParamsReset /* or materialParamsUpdated*/ or colliderUpdated)		// WARN: modifying lag scrolling fast!!!
				numLoop++;

#define MAX_SHADER_PARAM_RESET_LOOP 10
//#define MAX_SHADER_PARAM_RESET_LOOP 20	// WARN: modifying lag scrolling fast!!!
				if ((materialParamsReset /* or materialParamsUpdated*/ or colliderUpdated) and !m_desideredLookDevChanged and numLoop > MAX_SHADER_PARAM_RESET_LOOP /* and quadTree->quadrantDepth() == QuadTree::QuadrantDepth::Cache*/)
				{
					//if (numMaterialParamReset > 0 or numMaterialParamUpdate > 0 or numColliderUpdate > 0)
					//	PLOG_DEBUG << "Quadrants Updated: params set for quadrant = " << numMaterialParamReset << ", shaders updated = " << numMaterialParamUpdate << ", collider set = " << numColliderUpdate;
					loopCompleted = false;
					exitLoopRequired = true;
					break;
				}
			}
			if (exitLoopRequired)
				break;
		}

		if (loopCompleted)
			allCollidersReady(true);
	}

	//updateTerrainMaterial(worldCameraWorldPos, processCameraPosForQuadrants, processCameraPosForChunks);
}

//void GDN_TheWorld_Viewer::updateTerrainMaterial(godot::Vector3& worldCameraWorldPos, bool processCameraPosForQuadrants, bool processCameraPosForChunks)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1 ") + __FUNCTION__, "ALL");
//
//	size_t numQuadrantsToRefreshToShader = getNumQuadrantsToRefreshToShader();
//
//	if (m_forceUpdateTerrainMaterial or numQuadrantsToRefreshToShader > 0)
//	{
//		bool _streamingQuadrantsInProgress = streamingQuadrantsInProgress();
//		if (m_forceUpdateTerrainMaterial or !_streamingQuadrantsInProgress /*or allVisibleQuadrantInitialized()*/ or numQuadrantsToRefreshToShader > 100)
//		{
//			// TODO: si entra qui per ogni quadrant->optimizationLevelChanged() anche senza streaming, gestire ...
//
//			uint32_t _numQuadrantsPerGridSize = (uint32_t)numQuadrantsPerGridSize();
//			if (m_worldQuadrantsMap.size() == static_cast<unsigned long long>(_numQuadrantsPerGridSize) * _numQuadrantsPerGridSize)
//			{
//				uint32_t optimizationLevelIncrement = 0;
//				godot::Vector2 gridPos;
//				godot::Ref<godot::Texture2D> gridControl1Texture;
//				godot::Ref<godot::Texture2D> gridControl2Texture;
//				godot::Ref<godot::Texture2D> gridControl3Texture;
//				godot::Ref<godot::Texture2DArray> heightmapTexArrayOptlvlInc_0;
//				godot::Ref<godot::Texture2DArray> heightmapTexArrayOptlvlInc_1;
//				godot::Ref<godot::Texture2DArray> heightmapTexArrayOptlvlInc_2;
//				godot::Ref<godot::Texture2DArray> heightmapTexArrayOptlvlInc_3;
//
//				{
//					TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1.1 ") + __FUNCTION__, "Creating Terrain Shader Data");
//					std::vector<QuadTree*> quadrantsToRefresh;
//					getQuadrantsToRefreshToShader(quadrantsToRefresh);
//					gridPos = getGridPosInWUs();
//					m_gridControlData.setData(this, gridPos, m_worldQuadrantsMap, _numQuadrantsPerGridSize, quadrantsToRefresh);
//				}
//
//				m_forceUpdateTerrainMaterial = false;
//
//				optimizationLevelIncrement = (uint32_t)Globals()->getParamTextureOptimizationResolutionIncrement();
//
//				{
//					TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1.2 ") + __FUNCTION__, "Getting Grid Control Textures");
//					gridControl1Texture = m_gridControlData.getGridControl1Tex();
//					gridControl2Texture = m_gridControlData.getGridControl2Tex();
//					gridControl3Texture = m_gridControlData.getGridControl3Tex();
//				}
//					
//				{
//					TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1.3 ") + __FUNCTION__, "Getting Grid Heightmap Arrays");
//					heightmapTexArrayOptlvlInc_0 = m_gridControlData.getHeightmapTexArrayOptlvlInc(0);
//					heightmapTexArrayOptlvlInc_1 = m_gridControlData.getHeightmapTexArrayOptlvlInc(1);
//					heightmapTexArrayOptlvlInc_2 = m_gridControlData.getHeightmapTexArrayOptlvlInc(2);
//					heightmapTexArrayOptlvlInc_3 = m_gridControlData.getHeightmapTexArrayOptlvlInc(3);
//				}
//
//				{
//					TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1.4 ") + __FUNCTION__, "Updating Terrain Shader");
//
//					// update shader
//					WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin();
//					QuadTree* quadTree = itQuadTree->second.get();
//					Quadrant* quadrant = quadTree->getQuadrant();
//					ShaderTerrainData* shaderTerrainData = quadrant->getShaderTerrainData();
//					godot::Ref<godot::ShaderMaterial> mat = shaderTerrainData->getRegularTerrainMaterial();
//					mat->set_shader_parameter(SHADER_PARAM_GRID_POS, gridPos);
//					mat->set_shader_parameter(SHADER_PARAM_OPTLVL_INCREMENT, optimizationLevelIncrement);
//					mat->set_shader_parameter(SHADER_PARAM_NUM_QUADRANTS_PER_GRID_SIZE, _numQuadrantsPerGridSize);
//					mat->set_shader_parameter(SHADER_PARAM_GRID_CONTROL_1, gridControl1Texture);
//					mat->set_shader_parameter(SHADER_PARAM_GRID_CONTROL_2, gridControl2Texture);
//					mat->set_shader_parameter(SHADER_PARAM_GRID_CONTROL_3, gridControl3Texture);
//					mat->set_shader_parameter(SHADER_PARAM_HEIGHTMAP_OPTLVL_INC_0, heightmapTexArrayOptlvlInc_0);
//					mat->set_shader_parameter(SHADER_PARAM_HEIGHTMAP_OPTLVL_INC_1, heightmapTexArrayOptlvlInc_1);
//					mat->set_shader_parameter(SHADER_PARAM_HEIGHTMAP_OPTLVL_INC_2, heightmapTexArrayOptlvlInc_2);
//					mat->set_shader_parameter(SHADER_PARAM_HEIGHTMAP_OPTLVL_INC_3, heightmapTexArrayOptlvlInc_3);
//				}
//
//				{
//					TheWorld_Utils::GuardProfiler profiler(std::string("Update Terrain Material 1.5 ") + __FUNCTION__, "Releasing Terrain Shader Data");
//					m_gridControlData.releaseData();
//				}
//
//				if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::process_updateTerrainMaterial))
//				{
//					m_lastProcessExecutionInterrupted = true;
//				}
//			}
//		}
//	}
//}

void GDN_TheWorld_Viewer::process_neighbourQuadrantData(QuadTree* quadTree)
{
	//return;

	TheWorld_Utils::GuardProfiler profiler(std::string("process_neighbourQuadrantData 1 ") + __FUNCTION__, "ALL");

	bool debugLog = false;

	if (quadTree->statusInitialized())
	{
		Quadrant* quadrant = quadTree->getQuadrant();
		TheWorld_Utils::QuadrantPos quadrantPos = quadrant->getPos();

		Quadrant::NeighbourQuadrantData& northWestQuadrantData = quadrant->northWestQuadrantData();
		if (!northWestQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos northWestPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus);
			auto item = m_worldQuadrantsMap.find(northWestPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* northWestQuadTree = item->second.get();
				if (northWestQuadTree->statusInitialized())
				{
					northWestQuadrantData.init(quadrant, northWestQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus, quadrantPos);
					northWestQuadTree->getQuadrant()->southEastQuadrantData().init(northWestQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::SoutheastZPlusXPlus, northWestPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& northEastQuadrantData = quadrant->northEastQuadrantData();
		if (!northEastQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos northEastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NortheastZMinusXPlus);
			auto item = m_worldQuadrantsMap.find(northEastPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* northEastQuadTree = item->second.get();
				if (northEastQuadTree->statusInitialized())
				{
					northEastQuadrantData.init(quadrant, northEastQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::NortheastZMinusXPlus, quadrantPos);
					northEastQuadTree->getQuadrant()->southWestQuadrantData().init(northEastQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::SouthwestZPlusXMinus, northEastPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& southWestQuadrantData = quadrant->southWestQuadrantData();
		if (!southWestQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos southWestPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthwestZPlusXMinus);
			auto item = m_worldQuadrantsMap.find(southWestPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* southWestQuadTree = item->second.get();
				if (southWestQuadTree->statusInitialized())
				{
					southWestQuadrantData.init(quadrant, southWestQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::SouthwestZPlusXMinus, quadrantPos);
					southWestQuadTree->getQuadrant()->northEastQuadrantData().init(southWestQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::NortheastZMinusXPlus, southWestPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& southEastQuadrantData = quadrant->southEastQuadrantData();
		if (!southEastQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos southEastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SoutheastZPlusXPlus);
			auto item = m_worldQuadrantsMap.find(southEastPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* southEastQuadTree = item->second.get();
				if (southEastQuadTree->statusInitialized())
				{
					southEastQuadrantData.init(quadrant, southEastQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::SoutheastZPlusXPlus, quadrantPos);
					southEastQuadTree->getQuadrant()->northWestQuadrantData().init(southEastQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus, southEastPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& northQuadrantData = quadrant->northQuadrantData();
		if (!northQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos northPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
			auto item = m_worldQuadrantsMap.find(northPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* northQuadTree = item->second.get();
				if (northQuadTree->statusInitialized())
				{
					northQuadrantData.init(quadrant, northQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus, quadrantPos);
					northQuadTree->getQuadrant()->southQuadrantData().init(northQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus, northPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& southQuadrantData = quadrant->southQuadrantData();
		if (!southQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos southPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
			auto item = m_worldQuadrantsMap.find(southPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* southQuadTree = item->second.get();
				if (southQuadTree->statusInitialized())
				{
					southQuadrantData.init(quadrant, southQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus, quadrantPos);
					southQuadTree->getQuadrant()->northQuadrantData().init(southQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus, southPos);
				}
			}
		}

		Quadrant::NeighbourQuadrantData& westQuadrantData = quadrant->westQuadrantData();
		if (!westQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos westPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
			auto item = m_worldQuadrantsMap.find(westPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* westQuadTree = item->second.get();
				if (westQuadTree->statusInitialized())
				{
					westQuadrantData.init(quadrant, westQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus, quadrantPos);
					westQuadTree->getQuadrant()->eastQuadrantData().init(westQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus, westPos);
				}
			}
		}
		
		Quadrant::NeighbourQuadrantData& eastQuadrantData = quadrant->eastQuadrantData();
		if (!eastQuadrantData.initialized())
		{
			TheWorld_Utils::QuadrantPos eastPos = quadrantPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
			auto item = m_worldQuadrantsMap.find(eastPos);
			if (item != m_worldQuadrantsMap.end())
			{
				QuadTree* eastQuadTree = item->second.get();
				if (eastQuadTree->statusInitialized())
				{
					eastQuadrantData.init(quadrant, eastQuadTree->getQuadrant(), TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus, quadrantPos);
					eastQuadTree->getQuadrant()->westQuadrantData().init(eastQuadTree->getQuadrant(), quadrant, TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus, eastPos);
				}
			}
		}

		if (is_inside_tree() && quadTree->getLookDev() == ShaderTerrainData::LookDev::NotSet)
		{
			//if (southEastQuadrantData.initialized() && !southEastQuadrantData.shaderUpdated())
			//{
			//	godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
			//	currentMaterial->set_shader_parameter(SHADER_PARAM_SOUTH_EAST_GROUND_LAYERS, southEastQuadrantData.groundTextureLayers());
			//	southEastQuadrantData.shaderUpdated(true);
			//}

			//bool updateHeightmapBorderRowsToShader = false;

			if (northWestQuadrantData.initialized() && !northWestQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_NORTH_WEST_GROUND_LAYERS, northWestQuadrantData.groundTextureLayers());
				//currentMaterial->set_shader_parameter(SHADER_PARAM_NORTH_WEST_CORNER_SPLATMAP_TEXEL, northWestQuadrantData.getUpperLeftCornerSplatmapTexel());
				northWestQuadrantData.shaderUpdated(true);
			}

			if (northQuadrantData.initialized() && !northQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_NORTH_GROUND_LAYERS, northQuadrantData.groundTextureLayers());
				//currentMaterial->set_shader_parameter(SHADER_PARAM_NORTH_BORDER_SPLATMAP, northQuadrantData.getSplatmapBorderRowTexture());
				northQuadrantData.shaderUpdated(true);

				//updateHeightmapBorderRowsToShader = true;
			}

			if (southQuadrantData.initialized() && !southQuadrantData.shaderUpdated())
			{
				//updateHeightmapBorderRowsToShader = true;
				southQuadrantData.shaderUpdated(true);
			}

			if (westQuadrantData.initialized() && !westQuadrantData.shaderUpdated())
			{
				godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
				currentMaterial->set_shader_parameter(SHADER_PARAM_WEST_GROUND_LAYERS, westQuadrantData.groundTextureLayers());
				//currentMaterial->set_shader_parameter(SHADER_PARAM_WEST_BORDER_SPLATMAP, westQuadrantData.getSplatmapBorderRowTexture());
				westQuadrantData.shaderUpdated(true);

				//updateHeightmapBorderRowsToShader = true;
			}

			if (eastQuadrantData.initialized() && !eastQuadrantData.shaderUpdated())
			{
				//updateHeightmapBorderRowsToShader = true;
				eastQuadrantData.shaderUpdated(true);
			}

			//if (updateHeightmapBorderRowsToShader)
			//{
			//	int32_t numValidBorders = 0;
			//	godot::Ref<godot::ImageTexture> heightmapBorderQuadrantRowsTexture;
			//	{
			//		TheWorld_Utils::GuardProfiler profiler(std::string("process_neighbourQuadrantData 1.1 ") + __FUNCTION__, "quadrant->getHeightmapBorderQuadrantRowsTexture");
			//		heightmapBorderQuadrantRowsTexture = quadrant->getHeightmapBorderQuadrantRowsTexture(numValidBorders);
			//	}

			//	if (heightmapBorderQuadrantRowsTexture.is_valid())
			//	{
			//		godot::Ref<godot::ShaderMaterial> currentMaterial = quadrant->getShaderTerrainData()->getRegularTerrainMaterial();
			//		currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP_BORDER_ROWS, heightmapBorderQuadrantRowsTexture);

			//		if (debugLog)
			//		{
			//			m_debugProcessedNeighbourQuadrant[quadrantPos] = numValidBorders;
			//			//PLOG_DEBUG << "Updating Heightmap Border Rows Texture for Quadrant Pos: " << quadrantPos.getIndexPosX() << "," << quadrantPos.getIndexPosZ() << " - Num valid borders " << numValidBorders;
			//		}
			//	}
			//}
		}
	}
}

void GDN_TheWorld_Viewer::process_dump(GDN_TheWorld_Globals* globals)
{
//#pragma warning(push, 1)
//#pragma warning( disable : 6237 )
 
	// Check for Dump
	if (TIME_INTERVAL_BETWEEN_PERIDODIC_DUMP != 0 && globals->isDebugEnabled())
	{
//#pragma warning(pop)
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastDump > TIME_INTERVAL_BETWEEN_PERIDODIC_DUMP * 1000)
		{
			m_dumpRequired = true;
			m_timeElapsedFromLastDump = timeElapsed;
		}
	}

	if (m_dumpRequired and not m_dumpScheduled)
	{
		m_dumpRequired = false;

		m_processStepsForDump.clear();
		std::vector<ProcessStep>& processSteps = m_processSteps.steps();
		m_processStepsForDump.resize(processSteps.size());
		int idx = 0;
		for (auto& processStep : processSteps)
		{
			m_processStepsForDump[idx] = processStep;
			assert(idx == int(m_processStepsForDump[idx].getIdx()));
			idx++;
			processStep.resetAverageValues();
		}

		m_dumpScheduled = true;
	}
}

void GDN_TheWorld_Viewer::process_statistics(GDN_TheWorld_Globals* globals)
{
	// Check for Statistics
	if (TIME_INTERVAL_BETWEEN_STATISTICS != 0 and infoPanelVisible())
	{
		int64_t timeElapsed = TIME()->get_ticks_msec();
		if (timeElapsed - m_timeElapsedFromLastStatistic > TIME_INTERVAL_BETWEEN_STATISTICS)
		{
			//m_numProcessNotOwnsLock = 0;

			if (m_notStreamingTime.counterStarted() && m_notStreamingTime.partialDuration().count() > 60000)
			{
				writeStreamingNewQuadrantsStatistics();
				m_notStreamingTime.tock();
				m_notStreamingTime.tick();
			}

			refreshQuadrantsStatistics();

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

	godot::Input* input = godot::Input::get_singleton();

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
			ctrlPressed(true);
		else
			ctrlPressed(false);

		if (input->is_action_pressed("ui_alt"))
			m_altPressed = true;
		else
			m_altPressed = false;
	}
}

void GDN_TheWorld_Viewer::highlightHitQuadrant(bool b, bool force)
{
	if (m_highlightingHitQuadrant != b or force)
	{
		m_highlightingHitQuadrant = b;
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_HIGHLIGHT_HIT_QUAD, m_highlightingHitQuadrant);
	}
}

void GDN_TheWorld_Viewer::clearQuadrantsMaps(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	m_worldQuadrantsMapValid = false;
	m_worldQuadrantsMap.clear();
	//m_worldQuadrantsMapByDepth.clear();
	m_worldQuadrantsMapByDistanceFromCameraQuadrant.clear();
	m_extraQuadsMap.clear();
	//resetDataRefreshToShaderFlags();
	//m_cameraQuadTree = nullptr;
	m_quadrantHit = nullptr;
	m_adjacentQuadrantsHit.clear();
	m_previousTrackedMouseHitWorldPos = godot::Vector3();
	m_selectedWorldQuadrantPos.reset();
	m_selectedMapQuadrantPos.reset();
}

size_t GDN_TheWorld_Viewer::getExtraQuadsMapSize(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
	return m_extraQuadsMap.size();
}

void GDN_TheWorld_Viewer::getAllQuadrantPos(std::vector<TheWorld_Utils::QuadrantPos>& allQuandrantPos, bool withExtraQuads)
{
	allQuandrantPos.clear();

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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

Chunk* GDN_TheWorld_Viewer::getActiveChunkAt(TheWorld_Utils::QuadrantPos& pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter, enum class QuadType type)
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
		m_mtxQuadrantsAndMainProcessing.lock();
		editModeUIControl = memnew(GDN_TheWorld_Edit);
		m_mtxQuadrantsAndMainProcessing.unlock();
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

bool GDN_TheWorld_Viewer::isWorldQuadrantSelectedForEdit(void)
{
	if (m_selectedWorldQuadrantPos.empty())
		return false;
	else
		return true;
}

bool GDN_TheWorld_Viewer::isMapQuadrantSelectedForEdit(void)
{
	if (m_selectedMapQuadrantPos.empty())
		return false;
	else
		return true;
}

TheWorld_Utils::QuadrantPos GDN_TheWorld_Viewer::getWorldQuadrantSelForEdit(QuadTree** quadTreeSel)
{
	*quadTreeSel = nullptr;
	TheWorld_Utils::QuadrantPos ret = m_selectedWorldQuadrantPos;

	if (ret.empty())
		return ret;

	{
		*quadTreeSel = getQuadTree(ret);
		if (*quadTreeSel == nullptr)
			return TheWorld_Utils::QuadrantPos();
		else
			return ret;

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
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

GDN_TheWorld_Map* GDN_TheWorld_Viewer::Map(bool canUseCache, bool mustUseCache)
{
	if (m_map == nullptr and mustUseCache)
		return nullptr;
	
	if (m_map == nullptr or not canUseCache)
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

godot::Camera3D* GDN_TheWorld_Viewer::getCustomCamera(void)
{
	return WorldCamera();
}

godot::Camera3D* GDN_TheWorld_Viewer::getWorldCamera(void)
{
	return getWorldCamera2(true);
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

	return EditorInterface::get_singleton()->get_editor_viewport_3d(0)->get_camera_3d();

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

Chunk* GDN_TheWorld_Viewer::worldCameraTerrainChunk(void)
{
	return m_worldCameraTerrainChunk;
}

void GDN_TheWorld_Viewer::worldCameraTerrainChunk(Chunk* c)
{
	m_worldCameraTerrainChunk = c;
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

	PLOG_DEBUG << "Deploy world - Camera world pos=" << cameraX << "," << cameraY << "," << cameraZ << " - Yaw: " << cameraYaw <<", Pitch: " << cameraPitch << ", Roll: " << cameraRoll;

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

		m_numWorldVerticesPerSize = (int)Globals()->numVerticesPerQuadrantSize();
		
		Globals()->setStatus(TheWorldStatus::worldDeployInProgress);

		float _gridStepInWU = Globals()->gridStepInWU();

		Globals()->Client()->MapManagerDeployWorld(isInEditor(), level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);
		
		//Globals()->Client()->MapManagerDeployLevel(level, (size_t)m_numWorldVerticesPerSize, _gridStepInWU, true, cameraX, cameraY, cameraZ, cameraDistanceFromTerrainForced, cameraYaw, cameraPitch, cameraRoll);

		//QuadrantPos quadrantPos(cameraX, cameraZ, level, m_numWorldVerticesPerSize, _gridStepInWU);
		//quadrantPos.setTag("Camera");

		//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

		//clearQuadrantsMaps();
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
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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

AABB GDN_TheWorld_Viewer::getWorldCameraTerrainChunkLocalAABB(void)
{
	if (m_worldCameraTerrainChunk && !m_worldCameraTerrainChunk->isMeshNull())
		return m_worldCameraTerrainChunk->getAABB();
	else
		return AABB();
}

AABB GDN_TheWorld_Viewer::getWorldCameraTerrainChunkLocalDebugAABB(void)
{
	if (m_worldCameraTerrainChunk && !m_worldCameraTerrainChunk->isDebugMeshNull())
		return m_worldCameraTerrainChunk->getDebugMeshAABB();
	else
		return AABB();
}

Transform3D GDN_TheWorld_Viewer::getWorldCameraTerrainChunkGlobalTransformApplied(void)
{
	if (m_worldCameraTerrainChunk && !m_worldCameraTerrainChunk->isMeshNull())
		return m_worldCameraTerrainChunk->getGlobalTransformApplied();
	else
		return Transform3D();
}

Transform3D GDN_TheWorld_Viewer::getWorldCameraTerrainChunkDebugGlobalTransformApplied(void)
{
	if (m_worldCameraTerrainChunk && !m_worldCameraTerrainChunk->isDebugMeshNull())
		return m_worldCameraTerrainChunk->getDebugGlobalTransformApplied();
	else
		return Transform3D();
}


String GDN_TheWorld_Viewer::getWorldCameraTerrainChunkId(void)
{
	if (m_worldCameraTerrainChunk)
		return m_worldCameraTerrainChunk->getPos().getIdStr().c_str();
	else
		return "";
}

String GDN_TheWorld_Viewer::getWorldCameraTerrainQuadrantName(void)
{
	if (m_worldCameraTerrainChunk)
		return m_worldCameraTerrainChunk->getQuadTree()->getQuadrant()->getPos().getName().c_str();
	else
		return "";
}

void GDN_TheWorld_Viewer::refreshQuadrantsStatistics()
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

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
	//int numQuadrantsLoadingFromLocalCache = 0;
	int numFlushedQuadrant = 0;
	int numQuadrantWithCollider = 0;
	int numQuadrantWithEnabledCollider = 0;
	int numQuadrantWithHiResCollider = 0;
	int numQuadrantWithLowResCollider = 0;
	int numinitializedVisibleQuadrant = 0;
	//int numQuadrantNeedResetMaterialParams = 0;
	//int numQuadrantNeedUpdateMaterialParams = 0;
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
			//if (itQuadTree.second->materialParamsNeedReset())
			//	numQuadrantNeedResetMaterialParams++;
			//if (itQuadTree.second->materialParamsNeedUpdate())
			//	numQuadrantNeedUpdateMaterialParams++;
		}
		
		if (itQuadTree.second->statusInitialized() and !itQuadTree.second->statusToErase())
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

		//if (itQuadTree.second->statusLoadTerrainDataFromCacheNeeded() || itQuadTree.second->statusLoadTerrainDataFromCacheInProgress())
		//	numQuadrantsLoadingFromLocalCache++;

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
	m_mapQuadrantNum = m_mapQuadrantImagesMap.size();
	m_updatingMapQuadrantNum = m_mapQuadrantImagesMap.getNumUpdatingQuadrants();
	m_toShowMapQuadrantNum = m_mapQuadrantImagesMap.getNumQuadrantsToShow();
	m_mapQuadrantOverrideNum = map->getAllMapQuadrantOverrideNum(false);
	m_mapQuadrantEditedOverrideNum = map->getAllMapQuadrantOverrideNum(true);
	m_mapQuadrantColorOverrideNum = map->getMapQuadrantColorOverrideNum();
	m_mapQuadrantTextureOverrideNum = map->getMapQuadrantTextureOverrideNum();
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
	//m_numQuadrantLoadingFromLocalCache = numQuadrantsLoadingFromLocalCache;
	m_numFlushedQuadrant = numFlushedQuadrant;
	m_numQuadrantWithCollider = numQuadrantWithCollider;
	m_numQuadrantWithEnabledCollider = numQuadrantWithEnabledCollider;
	m_numQuadrantWithHiResCollider = numQuadrantWithHiResCollider;
	m_numQuadrantWithLowResCollider = numQuadrantWithLowResCollider;
	m_numInitializedVisibleQuadrant = numinitializedVisibleQuadrant;
	//m_numQuadrantNeedResetMaterialParam = numQuadrantNeedResetMaterialParams;
	//m_numQuadrantNeedUpdateMaterialParam = numQuadrantNeedUpdateMaterialParams;
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

float GDN_TheWorld_Viewer::getLoadingProgress()
{
	//if (recalcGridOfQuadrantsInViewNeeded())
	//{
	//	float loadingProgress = 0;
	//	int numInitializedQuadrant = getNumInitializedQuadrants();
	//	int numQuadrant = getNumQuadrants();
	//	int maxNumQuadrant = getMaxNumQuadrants();
	//	if (numQuadrant == 0 or maxNumQuadrant == 0 or numInitializedQuadrant == numQuadrant)
	//		loadingProgress = 0;
	//	else
	//	{
	//		int progress = numInitializedQuadrant * 100 / numQuadrant;
	//		int totalProgress = numInitializedQuadrant * 100 / maxNumQuadrant;
	//		loadingProgress = (float)totalProgress;
	//	}
	//	return loadingProgress;
	//}
	
	// escludere cache quadrants
	GDN_TheWorld_Globals* globals = Globals();
	int _numReadyQuadrants = 0, _numQuadrants = 0;
	float numReadyQuadrants = 0, numQuadrants = 0;
	if (globals->loadingScreenExitOnFullWorldDeploy())
	{
		//numQuadrants = float(getMaxNumQuadrants());
		//numInitializedQuadrants = float(getNumReadyQuadrants());
		numQuadrants = float(getMaxNumVisibleQuadrants());
		_numReadyQuadrants = getNumReadyQuadrantsAtDepth(_numQuadrants, numReadyQuadrants, QuadTree::QuadrantDepth::Initial, QuadTree::QuadrantDepth::Growing);
	}
	else
	{
		//numQuadrants = float(getMinNumQuadrants());
		//numInitializedQuadrants = float(getNumReadyQuadrantsAtDepth(QuadTree::QuadrantDepth::Initial, QuadTree::QuadrantDepth::Cache));
		numQuadrants = float(getMinNumVisibleQuadrants());
		_numReadyQuadrants = getNumReadyQuadrantsAtDepth(_numQuadrants, numReadyQuadrants, QuadTree::QuadrantDepth::Initial);
	}
	float loadingProgress = 0;
	if (numQuadrants > 1 and numReadyQuadrants > 0 and numReadyQuadrants <= numQuadrants)
		loadingProgress = (float(_numReadyQuadrants) / numQuadrants) * 100;

	//if (loadingProgress == 100)
	//	assert(loadingProgress == 100);
	
	return loadingProgress;
}

int GDN_TheWorld_Viewer::getNumQuadrants2(bool recalcNow)
{
	if (recalcNow)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		m_numQuadrant = (int)m_worldQuadrantsMap.size();
	}
	
	return m_numQuadrant;
}

int GDN_TheWorld_Viewer::getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1)
{
	return getNumReadyQuadrantsAtDepth(numEvaluated, numReady, depth1, QuadTree::QuadrantDepth::NotSet, QuadTree::QuadrantDepth::NotSet);
}

int GDN_TheWorld_Viewer::getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1, QuadTree::QuadrantDepth depth2)
{
	return getNumReadyQuadrantsAtDepth(numEvaluated, numReady, depth1, depth2, QuadTree::QuadrantDepth::NotSet);
}

int GDN_TheWorld_Viewer::getNumReadyQuadrantsAtDepth(int& numEvaluated, float& numReady, QuadTree::QuadrantDepth depth1, QuadTree::QuadrantDepth depth2, QuadTree::QuadrantDepth depth3)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	numEvaluated = 0;
	int _numReady = 0, initialized = 0, needCollider = 0, needMatParamReset = 0, needMatParamUpdate = 0;
	for (auto& itQuadrantMapItem : m_worldQuadrantsMap)
	{
		QuadTree::QuadrantDepth currentDepth = itQuadrantMapItem.second->quadrantDepth();
		if ((depth1 != QuadTree::QuadrantDepth::NotSet and currentDepth == depth1)
			or (depth2 != QuadTree::QuadrantDepth::NotSet and currentDepth == depth2)
			or (depth3 != QuadTree::QuadrantDepth::NotSet and currentDepth == depth3))
		{
			numEvaluated++;

			if (itQuadrantMapItem.second->statusInitialized() //and !itQuadrantMapItem.second->statusToErase()
				and !itQuadrantMapItem.second->colliderNeedReset(QuadType::NotSet)
				and !itQuadrantMapItem.second->materialParamsNeedReset()
				and !itQuadrantMapItem.second->materialParamsNeedUpdate())
				_numReady++;

			if (itQuadrantMapItem.second->statusInitialized())
				initialized++;
			if (itQuadrantMapItem.second->colliderNeedReset(QuadType::NotSet))
				needCollider++;
			if (itQuadrantMapItem.second->materialParamsNeedReset())
				needMatParamReset++;
			if (itQuadrantMapItem.second->materialParamsNeedUpdate())
				needMatParamUpdate++;
		}
	}

	numReady = float(_numReady);
	//{
	//	int colliderSet = initialized - needCollider;									// colliderNeedReset is set only for intialized quadrants
	//	int paramResetDone = initialized - needMatParamReset;							// materialParamsNeedReset is set only for intialized quadrants
	//	int paramUpdateDone = initialized - needMatParamReset - needMatParamUpdate;		// materialParamsNeedUpdate is set only for quadrants which have done resetting params which are set for initialized quadrants
	//	numReady = initialized * 0.25f + colliderSet * 0.25f + paramResetDone * 0.25f + paramUpdateDone * 0.25f;
	//	//PLOG_DEBUG << "all quadrants = " << m_worldQuadrantsMap.size() << " evaluated = " << numEvaluated << " ready = " << _numReady << " " << numReady << " init = " << initialized << " need collider = " << needCollider << " collider set = " << colliderSet << " need param reset = " << needMatParamReset << " param reset done = " << paramResetDone << " need param update = " << needMatParamUpdate << " param update done = " << paramUpdateDone;
	//}


	//int numInitial = 0, numGrowing = 0, numCache = 0, numNotSet = 0;
	//for (auto& item : m_worldQuadrantsMap)
	//{
	//	if (item.second->quadrantDepth() == QuadTree::QuadrantDepth::Initial)
	//		numInitial++;
	//	if (item.second->quadrantDepth() == QuadTree::QuadrantDepth::Growing)
	//		numGrowing++;
	//	if (item.second->quadrantDepth() == QuadTree::QuadrantDepth::Cache)
	//		numCache++;
	//	if (item.second->quadrantDepth() == QuadTree::QuadrantDepth::NotSet)
	//		numNotSet++;
	//}

	return _numReady;
}

int GDN_TheWorld_Viewer::getNumReadyQuadrants(void)
{
	int numEvaluated;
	float numReady;
	return getNumReadyQuadrantsAtDepth(numEvaluated, numReady, QuadTree::QuadrantDepth::Initial, QuadTree::QuadrantDepth::Growing, QuadTree::QuadrantDepth::Cache);
}

int GDN_TheWorld_Viewer::getNumInitializedQuadrants2(bool recalcNow)
{
	if (recalcNow)
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		m_numInitializedQuadrant = 0;
		for (auto& itQuadrantMapItem : m_worldQuadrantsMap)
			if (itQuadrantMapItem.second->statusInitialized() and !itQuadrantMapItem.second->statusToErase())
				m_numInitializedQuadrant++;
	}

	return m_numInitializedQuadrant;
}

bool GDN_TheWorld_Viewer::recalcAllVisibleQuadrantsInitialized(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

	if (m_worldQuadrantsMap.size() == 0)
		return false;

	bool allInitialized = true;
	for (auto& itQuadrantMapItem : m_worldQuadrantsMap)
	{
		if (itQuadrantMapItem.second->quadrantDepth() == QuadTree::QuadrantDepth::Cache)
			continue;
		if (itQuadrantMapItem.second->statusToErase())
			continue;
		if (not itQuadrantMapItem.second->statusInitialized())
		{
			allInitialized = false;
			break;
		}
	}
	return allInitialized;
}

int GDN_TheWorld_Viewer::getNumToEraseQuadrants(void) const
{
	return m_numToEraseQuadrant;
}

int GDN_TheWorld_Viewer::getNumVisibleQuadrants(void) const
{
	return m_numVisibleQuadrant;
}

int GDN_TheWorld_Viewer::getNumEmptyQuadrants(void) const
{
	return m_numEmptyQuadrant;
}

int GDN_TheWorld_Viewer::getNumQuadrantsLoadingFromServer(void) const
{
	return m_numQuadrantLoadingFromServer;
}

int GDN_TheWorld_Viewer::getNumQuadrantsLoadingFromLocalCache(void) const
{
	return m_numQuadrantLoadingFromLocalCache;
}

int GDN_TheWorld_Viewer::getNumFlushedQuadrants(void) const
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
int GDN_TheWorld_Viewer::getNumInitializedVisibleQuadrants(void) const
{
	return m_numInitializedVisibleQuadrant;
}

float GDN_TheWorld_Viewer::getDelayDegree(void)
{
	return m_delayDegree;
}

void GDN_TheWorld_Viewer::calcDelayDegree(void)
{
	int numQuadsToRefreshToShader = getNumNeedShaderResetQuadrants() + getNumNeedShaderUpdateQuadrants();
	int numQuadsToInitialize = getNumQuadrants() - getNumInitializedQuadrants();
	int quadsToProcess = numQuadsToRefreshToShader + numQuadsToInitialize;
	int numQuadsToProcessPerMovement = (maxActualDepthQuadInAllDirections() + getCacheQuadOnPerimeter()) * 2 + 1;
	m_delayDegree = float(quadsToProcess) / float(numQuadsToProcessPerMovement);
}

int GDN_TheWorld_Viewer::getNumNeedShaderResetQuadrants(void) const
{
	return m_numQuadrantNeedResetMaterialParam;
}

int GDN_TheWorld_Viewer::getNumNeedShaderUpdateQuadrants(void) const
{
	return m_numQuadrantNeedUpdateMaterialParam;
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
	GDN_TheWorld_Globals* globals = Globals();

	globals->debugPrint("*************");
	globals->debugPrint("STARTING DUMP");

	double f = Engine::get_singleton()->get_frames_per_second();
	globals->debugPrint("FPS: " + String(std::to_string(f).c_str()));

	{
		map<int, size_t> mapNumActiveChunksPerLod;
		map<int, size_t> mapNumActiveTerrainChunksPerLod;
		map<int, size_t> mapNumActiveSeaChunksPerLod;

		GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
		if (editModeUIControl != nullptr && !editModeUIControl->initialized())
			editModeUIControl = nullptr;

		size_t numQuadrantsWithSea = 0;
		std::map<size_t, size_t> mapNumQuadrantsPerOptimizationLevel, mapNumChunksPerOptimizationLevel, mapNumTerrainChunksPerOptimizationLevel, mapNumSeaChunksPerOptimizationLevel, mapNumActiveChunksPerOptimizationLevel, mapNumActiveTerrainChunksPerOptimizationLevel, mapNumActiveSeaChunksPerOptimizationLevel;
		std::map<int, size_t> mapQuadrantsToSaveTerrainType, mapQuadrantsToUploadTerrainType;
		size_t mapMemoryOccupation = 0, heightmap16MemoryOccupation = 0, heightmap32MemoryOccupation = 0, normalmapMemoryOccupation = 0, splatmapMemoryOccupation = 0, globalmapMemoryOccupation = 0, colormapMemoryOccupation = 0,
			terrainEditMemoryOccupation = 0, colliderHeightsMemoryOccupation = 0, borderQuadrantsHeightEdges = 0, borderQuadrantsSplatEdges = 0;
		size_t texMemoryOccupation = 0;
		size_t resizedTexMemoryOccupation = 0;
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		for (auto& quadTree : m_worldQuadrantsMap)
		{
			if (quadTree.second->containsSeaChunks())
				numQuadrantsWithSea++;

			mapMemoryOccupation += quadTree.second->getQuadrant()->calcMapMemoryOccupation();
			heightmap16MemoryOccupation += quadTree.second->getQuadrant()->calcHeightmap16MemoryOccupation();
			borderQuadrantsHeightEdges += quadTree.second->getQuadrant()->calcBorderHeightEdges();
			borderQuadrantsSplatEdges += quadTree.second->getQuadrant()->calcBorderSplatEdges();
			heightmap32MemoryOccupation += quadTree.second->getQuadrant()->calcHeightmap32MemoryOccupation();
			normalmapMemoryOccupation += quadTree.second->getQuadrant()->calcNormalmapMemoryOccupation();
			splatmapMemoryOccupation += quadTree.second->getQuadrant()->calcSplatmapMemoryOccupation();
			globalmapMemoryOccupation += quadTree.second->getQuadrant()->calcGlobalmapMemoryOccupation();
			colormapMemoryOccupation += quadTree.second->getQuadrant()->calcColormapMemoryOccupation();
			terrainEditMemoryOccupation += quadTree.second->getQuadrant()->calcTerrainEditMemoryOccupation();
			colliderHeightsMemoryOccupation += quadTree.second->getQuadrant()->calcColliderHeightsMemoryOccupation();

			size_t _resizedTexMemoryOccupation = 0;
			texMemoryOccupation += quadTree.second->getQuadrant()->calcTexMemoryOccupation(_resizedTexMemoryOccupation);
			resizedTexMemoryOccupation += _resizedTexMemoryOccupation;
			//resizedTexMemoryOccupation += quadTree.second->getQuadrant()->calcResizedTexMemoryOccupation();
			
			int numActiveChunksInQuadrant = 0, numActiveTerrainChunksInQuadrant = 0, numActiveSeaChunksInQuadrant = 0;
			int numChunksInQuadrant = 0, numTerrainChunksInQuadrant = 0, numSeaChunksInQuadrant = 0;
			//std::string activeChunksInQuadrant, activeTerrainChunksInQuadrant, activeSeaChunksInQuadrant;
			if (quadTree.second->statusInitialized() and !quadTree.second->statusToErase())
			{
				map<int, size_t> mapNumActiveChunksPerLodInQuadrant;
				numActiveChunksInQuadrant = quadTree.second->getNumActiveChunks(mapNumActiveChunksPerLodInQuadrant, QuadType::NotSet);
				numChunksInQuadrant = quadTree.second->getNumChunks(false, QuadType::NotSet);
				for (auto& item : mapNumActiveChunksPerLodInQuadrant)
				{
					if (mapNumActiveChunksPerLod.contains(item.first))
					{
						mapNumActiveChunksPerLod[item.first] += item.second;
					}
					else
					{
						mapNumActiveChunksPerLod[item.first] = item.second;
					}
					//activeChunksInQuadrant += "  - lod " + std::to_string(item.first) + " = " + std::to_string(item.second);
				}

				map<int, size_t> mapNumActiveTerrainChunksPerLodInQuadrant;
				numActiveTerrainChunksInQuadrant = quadTree.second->getNumActiveChunks(mapNumActiveTerrainChunksPerLodInQuadrant, QuadType::Terrain);
				numTerrainChunksInQuadrant = quadTree.second->getNumChunks(false, QuadType::Terrain);
				for (auto& item : mapNumActiveTerrainChunksPerLodInQuadrant)
				{
					if (mapNumActiveTerrainChunksPerLod.contains(item.first))
					{
						mapNumActiveTerrainChunksPerLod[item.first] += item.second;
					}
					else
					{
						mapNumActiveTerrainChunksPerLod[item.first] = item.second;
					}
					//activeTerrainChunksInQuadrant += "  - lod " + std::to_string(item.first) + " = " + std::to_string(item.second);
				}

				map<int, size_t> mapNumActiveSeaChunksPerLodInQuadrant;
				numActiveSeaChunksInQuadrant = quadTree.second->getNumActiveChunks(mapNumActiveSeaChunksPerLodInQuadrant, QuadType::Sea);
				numSeaChunksInQuadrant = quadTree.second->getNumChunks(false, QuadType::Sea);
				for (auto& item : mapNumActiveSeaChunksPerLodInQuadrant)
				{
					if (mapNumActiveSeaChunksPerLod.contains(item.first))
					{
						mapNumActiveSeaChunksPerLod[item.first] += item.second;
					}
					else
					{
						mapNumActiveSeaChunksPerLod[item.first] = item.second;
					}
					//activeSeaChunksInQuadrant += "  - lod " + std::to_string(item.first) + " = " + std::to_string(item.second);
				}
			}

			size_t optimizationLevel = quadTree.second->getQuadrant()->optimizationLevel();
			if (mapNumQuadrantsPerOptimizationLevel.contains(optimizationLevel))
			{
				mapNumQuadrantsPerOptimizationLevel[optimizationLevel]++;
				mapNumChunksPerOptimizationLevel[optimizationLevel] += numChunksInQuadrant;
				mapNumTerrainChunksPerOptimizationLevel[optimizationLevel] += numTerrainChunksInQuadrant;
				mapNumSeaChunksPerOptimizationLevel[optimizationLevel] += numSeaChunksInQuadrant;
				mapNumActiveChunksPerOptimizationLevel[optimizationLevel] += numActiveChunksInQuadrant;
				mapNumActiveTerrainChunksPerOptimizationLevel[optimizationLevel] += numActiveTerrainChunksInQuadrant;
				mapNumActiveSeaChunksPerOptimizationLevel[optimizationLevel] += numActiveSeaChunksInQuadrant;
			}
			else
			{
				mapNumQuadrantsPerOptimizationLevel[optimizationLevel] = 1;
				mapNumChunksPerOptimizationLevel[optimizationLevel] = numChunksInQuadrant;
				mapNumTerrainChunksPerOptimizationLevel[optimizationLevel] = numTerrainChunksInQuadrant;
				mapNumSeaChunksPerOptimizationLevel[optimizationLevel] = numSeaChunksInQuadrant;
				mapNumActiveChunksPerOptimizationLevel[optimizationLevel] = numActiveChunksInQuadrant;
				mapNumActiveTerrainChunksPerOptimizationLevel[optimizationLevel] = numActiveTerrainChunksInQuadrant;
				mapNumActiveSeaChunksPerOptimizationLevel[optimizationLevel] = numActiveSeaChunksInQuadrant;
			}

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

		float thresholdCacheHeights = globals->getParamThresholdCacheHeights();
		char _thresholdCacheHeights[24]; // dummy size, you should take care of the size!
		sprintf_s(_thresholdCacheHeights, "%.2f", thresholdCacheHeights);
		globals->debugPrint(String("Threshold value to cache maps and images : ") + std::string(_thresholdCacheHeights).c_str() + " m");
		globals->debugPrint(String("Quadrants : ") + String(std::to_string(m_worldQuadrantsMap.size()).c_str()) + ", cached map memory : " + String(std::to_string((float)mapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB, cached image memory : " + String(std::to_string((float)texMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB (resized cached image memory : " + String(std::to_string((float)resizedTexMemoryOccupation / (1024 * 1024 * 1024)).c_str())  + " GB)");
		globals->debugPrint(String("   Border Quadrants Height Edges : ") + String(std::to_string((float)borderQuadrantsHeightEdges / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Border Quadrants Splat Edges : ") + String(std::to_string((float)borderQuadrantsSplatEdges / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Heigthmap16 : ") + String(std::to_string((float)heightmap16MemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Heigthmap32 : ") + String(std::to_string((float)heightmap32MemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Normalmap : ") + String(std::to_string((float)normalmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Splatmap : ") + String(std::to_string((float)splatmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Globalmap : ") + String(std::to_string((float)globalmapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Colormap : ") + String(std::to_string((float)colormapMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   TerrainEdit : ") + String(std::to_string((float)terrainEditMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("   Collider Heights : ") + String(std::to_string((float)colliderHeightsMemoryOccupation / (1024 * 1024 * 1024)).c_str()) + " GB");
		globals->debugPrint(String("Engine monitor : "));
		globals->debugPrint(String("   Static memory in use : ") + std::to_string(perf->get_monitor(godot::Performance::Monitor::MEMORY_STATIC) / (1024 * 1024 * 1024)).c_str() + " GB");
		globals->debugPrint(String("   Render video memory in use : ") + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_VIDEO_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB, for texture " + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_TEXTURE_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB, for buffers " + std::to_string(perf->get_monitor(godot::Performance::Monitor::RENDER_BUFFER_MEM_USED) / (1024 * 1024 * 1024)).c_str() + " GB");

		globals->printParameterInfo();
		
		globals->debugPrint(String("Num. quadrants with sea = ") + std::to_string(numQuadrantsWithSea).c_str());

		globals->debugPrint(String("Min. num. visible quadrants on perimeter : ") + std::to_string(m_minVisibleQuadrantOnPerimeter).c_str());
		globals->debugPrint(String("Max. num. visible quadrants on perimeter : ") + std::to_string(getMaxDepthQuadOnPerimeter()).c_str());
		globals->debugPrint(String("Num. cache quadrants on perimeter : ") + std::to_string(m_numCacheQuadrantOnPerimeter).c_str());
		globals->debugPrint(String("Actual num quadrants on perimeter on north side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnNorthSide).c_str());
		globals->debugPrint(String("Actual num quadrants on perimeter on east side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnEastSide).c_str());
		globals->debugPrint(String("Actual num quadrants on perimeter on south side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnSouthSide).c_str());
		globals->debugPrint(String("Actual num quadrants on perimeter on west side : ") + std::to_string(m_numVisibleQuadrantOnPerimeterOnWestSide).c_str());

		for (auto& item : mapNumQuadrantsPerOptimizationLevel)
		{
			//size_t resizedMapSize = (globals->heightmapResolution() / (size_t)pow(2, item.first)) + 1;
			size_t resizedMapSize = globals->resolutionForOptimizationLevel(item.first);
			std::string msg = "Num quadrants with optimization level " + std::to_string(item.first) + " : " + std::to_string(item.second) + " - map size : " + std::to_string(resizedMapSize)
				+ " - Chunks (active) : " + std::to_string(mapNumChunksPerOptimizationLevel[item.first]) + "(" + std::to_string(mapNumActiveChunksPerOptimizationLevel[item.first]) + ")"
				+ " - Terrain Chunks (active) : " + std::to_string(mapNumTerrainChunksPerOptimizationLevel[item.first]) + "(" + std::to_string(mapNumActiveTerrainChunksPerOptimizationLevel[item.first]) + ")"
				+ " - Sea Chunks (active) : " + std::to_string(mapNumSeaChunksPerOptimizationLevel[item.first]) + "(" + std::to_string(mapNumActiveSeaChunksPerOptimizationLevel[item.first]) + ")";
				globals->debugPrint(msg.c_str());
		}

		if (mapQuadrantsToSaveTerrainType.size() > 0)
		{
			for (auto& item : mapQuadrantsToSaveTerrainType)
			{
				std::string msg = "Num quadrants to save with terrain type <" + TheWorld_Utils::TerrainEdit::terrainTypeString((TheWorld_Utils::TerrainEdit::TerrainType)item.first) + "> : " + std::to_string(item.second);
				globals->debugPrint(msg.c_str());
			}
		}
		else
		{
			globals->debugPrint("No quadrant to save");
		}
		if (mapQuadrantsToUploadTerrainType.size() > 0)
		{
			for (auto& item : mapQuadrantsToUploadTerrainType)
			{
				std::string msg = "Num quadrants to upload with terrain type <" + TheWorld_Utils::TerrainEdit::terrainTypeString((TheWorld_Utils::TerrainEdit::TerrainType)item.first) + "> : " + std::to_string(item.second);
				globals->debugPrint(msg.c_str());
			}
		}
		else
		{
			globals->debugPrint("No quadrant to upload");
		}

		globals->debugPrint("Active Chunks");
		for (auto& item : mapNumActiveChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = globals->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = globals->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num active chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(globals->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * globals->gridStepInHeightmapWUs(lod));

			globals->debugPrint(msg.c_str());
		}
		globals->debugPrint("Active Terrain Chunks");
		for (auto& item : mapNumActiveTerrainChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = globals->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = globals->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num active chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(globals->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * globals->gridStepInHeightmapWUs(lod));

			globals->debugPrint(msg.c_str());
		}
		globals->debugPrint("Active Sea Chunks");
		for (auto& item : mapNumActiveSeaChunksPerLod)
		{
			int lod = item.first;
			int numChunksPerSide = globals->numChunksPerHeightmapSide(lod);
			int numVerticesPerSide = globals->numVerticesPerChuckSide() * numChunksPerSide;
			std::string msg = "Num active chunks at lod " + std::to_string(lod) + " = " + std::to_string(item.second)
				+ " - Num chunks per quadrant side = " + std::to_string(numChunksPerSide)
				+ " - Grid step in WUs = " + std::to_string(globals->gridStepInHeightmapWUs(lod))
				+ " - Num vertices per quadrant side = " + std::to_string(numVerticesPerSide)
				+ " - Quadrant size in WUs = " + std::to_string(numVerticesPerSide * globals->gridStepInHeightmapWUs(lod));

			globals->debugPrint(msg.c_str());
		}
	}

	//Node* node = get_node(NodePath("/root"));
	//if (node != nullptr)
	//{
	//	globals->debugPrint("=== NODES ===========================================");
	//	globals->debugPrint("");
	//	globals->debugPrint("@@2 = res://native/GDN_TheWorld_Viewer.gdns");
	//	globals->debugPrint("");
	//	globals->debugPrint(node->get_name());
	//	Array nodes = node->get_children();
	//	dumpRecurseIntoChildrenNodes(nodes, 1);
	//	globals->debugPrint("=== NODES ===========================================");
	//}
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
		for (WorldQuadrantsMap::iterator itQuadTree = m_worldQuadrantsMap.begin(); itQuadTree != m_worldQuadrantsMap.end(); itQuadTree++)
		{
			itQuadTree->second->dump(true);
		}
	}

	std::vector<std::string> names = TheWorld_Utils::Profiler::names("");
	if (names.size() > 0)
	{
		globals->debugPrint("== PROFILE DATA ==========================================");
		for (auto& name : names)
		{
			size_t num = 0;
			size_t elapsed = TheWorld_Utils::Profiler::elapsedMcs("", name, num);
			size_t avgElapsed = TheWorld_Utils::Profiler::averageElapsedMcs("", name, num);
			size_t minElapsed = TheWorld_Utils::Profiler::minElapsedMcs("", name);
			size_t maxElapsed = TheWorld_Utils::Profiler::maxElapsedMcs("", name);
			if (num > 0)
			{
				globals->debugPrint(String(name.c_str()) + " num=" + std::to_string(num).c_str() + " Elapsed=" + std::to_string(elapsed).c_str() + " MinElapsed=" + std::to_string(minElapsed).c_str() + " MaxElapsed=" + std::to_string(maxElapsed).c_str() + " AvgElapsed=" + std::to_string(avgElapsed).c_str());
				TheWorld_Utils::Profiler::reset("", name);
			}
		}
		globals->debugPrint("== PROFILE DATA ==========================================");
	}

	if (m_processStepsForDump.size() > 0)
	{
		globals->debugPrint("== PROCESS STEPS AVERAGE DURATION ========================");

		class SortedByTotDiffFromPreviousStep
		{
		public:
			SortedByTotDiffFromPreviousStep(enum class ProcessStep::ProcessStepIdx idx, size_t totDiffFromPreviousStep)
			{
				m_idx = idx;
				m_totDiffFromPreviousStep = totDiffFromPreviousStep;
			}
			bool operator<(SortedByTotDiffFromPreviousStep& processStep) const
			{
				return m_totDiffFromPreviousStep > processStep.m_totDiffFromPreviousStep;
			}
			enum class ProcessStep::ProcessStepIdx getIdx(void) const
			{
				return m_idx;
			}
		private:
			enum class ProcessStep::ProcessStepIdx m_idx = ProcessStep::ProcessStepIdx::not_set;
			size_t m_totDiffFromPreviousStep = 0;
		};

		std::vector<SortedByTotDiffFromPreviousStep> sortedByTotDiffFromPreviousSteps;
		for (auto& processStep : m_processStepsForDump)
			sortedByTotDiffFromPreviousSteps.push_back(SortedByTotDiffFromPreviousStep(processStep.getIdx(), processStep.getTotDiffFromPreviousStep()));
		std::sort(sortedByTotDiffFromPreviousSteps.begin(), sortedByTotDiffFromPreviousSteps.end());

		int idx = 0;
		for (auto& processStepidx : m_processSteps.stepsOrder())
		{
			std::vector<ProcessStep>::iterator itFound = m_processStepsForDump.end();
			for (std::vector<ProcessStep>::iterator it = m_processStepsForDump.begin(); it != m_processStepsForDump.end(); it++)
			{
				if (it->getIdx() == processStepidx)
				{
					itFound = it;
					break;
				}
			}
			if (itFound != m_processStepsForDump.end())
			{
				bool reset = true;
				itFound->computeAverageValues(reset);
				size_t num = 0;
				size_t averageDiffFromPreviousStep = itFound->getAverageDiffFromPreviousStep(num);
				if (num > 0)
				{
					size_t idxWorst = 1;
					for (auto& processStep : sortedByTotDiffFromPreviousSteps)
					{
						if (processStep.getIdx() == itFound->getIdx())
							break;
						idxWorst++;
					}
					globals->debugPrint((std::to_string(++idx) + " " + itFound->msg() + " - Avg Diff = " + TheWorld_Utils::quickAddThousandSeparators(std::to_string(averageDiffFromPreviousStep / 1000)) + " mcs" + " - Idx Worst = " + std::to_string(idxWorst) + (idxWorst == 1 ? " - WORST" : "")).c_str());
				}
			}
		}

		globals->debugPrint("== PROCESS STEPS AVERAGE DURATION ========================");
	}
	
	if (m_debugProcessedNeighbourQuadrant.size() > 0)
	{
		globals->debugPrint("== PROCESSED NEIGHBOUR QUADRANTS ========================");
		for (auto& [quadrantPos, numValidBorders] : m_debugProcessedNeighbourQuadrant)
		{
			globals->debugPrint((std::string("Updating Heightmap Border Rows Texture for Quadrant Pos: ") + std::to_string(quadrantPos.getIndexPosX()) + "," + std::to_string(quadrantPos.getIndexPosZ()) + " - valid borders " + std::to_string(numValidBorders)).c_str());
		}
		globals->debugPrint("== PROCESSED NEIGHBOUR QUADRANTS ========================");
		
		m_debugProcessedNeighbourQuadrant.clear();
	}
	
	globals->debugPrint("DUMP COMPLETE");
	globals->debugPrint("*************");

	//godot::Viewport* vp = get_viewport();
	//godot::Ref<godot::ViewportTexture> vpt = vp->get_texture();
	//godot::Vector2 size = vpt->get_size();
	//godot::Ref<godot::Image> image = vpt->get_image();
	//std::string outFileName = GDN_TheWorld_Globals::getClientDataDir() + "\\screen.png";
	//image->save_png(outFileName.c_str());
}

template <typename T> void GDN_TheWorld_Viewer::getChildrenOfType(godot::Array& outNodes, godot::Node* parent, int level, bool recurse)
{
	//if (IS_EDITOR_HINT())
	//	return;
	
	if (parent == nullptr)
	{
		SceneTree* scene = get_tree();
		if (scene == nullptr)
			return;
		parent = scene->get_root();
	}
	
	godot::Array nodes = parent->get_children();
	for (int i = 0; i < nodes.size(); i++)
	{
		godot::Node* node = godot::Object::cast_to<T>(nodes[i]);
		if (node != nullptr)
			outNodes.append(node);
		node = godot::Object::cast_to<godot::Node>(nodes[i]);
		if (recurse and node != nullptr)
			getChildrenOfType<T>(outNodes, node, level + 1, recurse);
	}
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

void GDN_TheWorld_Viewer::setWorldCameraTerrainChunk(Chunk* chunk, QuadTree* quadTree)
{
	if (m_worldCameraTerrainChunk != nullptr && m_worldCameraTerrainChunk != chunk)
	{
		m_worldCameraTerrainChunk->resetCameraVerticalOnChunk();
		quadTree->addChunkUpdate(m_worldCameraTerrainChunk, chunk->getOwnQuadType());
	}

	m_worldCameraTerrainChunk = chunk;
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
				// Schedule quadrants to be erased in next _process
				{
					//std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);
					std::unique_lock<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing, std::try_to_lock);
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
				
				// recalcGridOfQuadrantsInView
				{
					size_t numNeededQuads = static_cast<size_t>(1 + m_numVisibleQuadrantOnPerimeterOnNorthSide + m_numVisibleQuadrantOnPerimeterOnSouthSide + 2 * getCacheQuadOnPerimeter())
						* static_cast<size_t>(1 + m_numVisibleQuadrantOnPerimeterOnWestSide + m_numVisibleQuadrantOnPerimeterOnEastSide + 2 * getCacheQuadOnPerimeter());
					//(Globals()->statusMinimalWorldDeployed() or Globals()->statusWorldDeployed())
					if ((int)Globals()->status() >= (int)TheWorldStatus::minimalWorldDeployed && m_worldQuadrantsMap.size() < numNeededQuads)
						recalcGridOfQuadrantsInViewNeeded(true);

					if (m_numVisibleQuadrantOnPerimeterOnNorthSideApplied != m_numVisibleQuadrantOnPerimeterOnNorthSide
						|| m_numVisibleQuadrantOnPerimeterOnSouthSideApplied != m_numVisibleQuadrantOnPerimeterOnSouthSide
						|| m_numVisibleQuadrantOnPerimeterOnWestSideApplied != m_numVisibleQuadrantOnPerimeterOnWestSide
						|| m_numVisibleQuadrantOnPerimeterOnEastSideApplied != m_numVisibleQuadrantOnPerimeterOnEastSide
						)
						recalcGridOfQuadrantsInViewNeeded(true);

					{
						std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

						bool _allVisibleQuadrantInitialized = recalcAllVisibleQuadrantsInitialized();
						if (recalcGridOfQuadrantsInViewNeeded()
							and _allVisibleQuadrantInitialized /* *** WARNING *** and !worldStreamingInProgress()*/
							and (Globals()->statusWorldDeployInProgress() or Globals()->statusMinimalWorldDeployed() or Globals()->statusWorldDeployed()) )
						{
							bool newQuadAdded = recalcGridOfQuadrantsInView();
							if (not newQuadAdded)
								finishedResizingGrid(true);
							recalcGridOfQuadrantsInViewNeeded(false);
						}
					}
				}

				// Sync with server
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				if ((now - lastSyncTime).count() > STREAMER_SYNC_PERIOD_TIME)
				{
					bool editorUploadToServerInProgress = false;
					GDN_TheWorld_Edit* editModeUIControl = EditModeUIControl();
					if (editModeUIControl != nullptr and editModeUIControl->initialized())
						editorUploadToServerInProgress = editModeUIControl->uploadToServerInProgress();
					//if (editorUploadToServerInProgress)			// DEBUG
					//	editorUploadToServerInProgress = true;	// DEBUG
					
					// *
					// The client sends the list of quadrants and map texture rects it has and the server reply with the list of quadrants and map texture rects that need to be refreshed on the client side because they are different
					// from the server ones (for example because they have been modified after last synchronization)
					// *
					// - MapManagerSync sends the list of the quadrants and the map texture rects (only in map mode) currently owned by the client
					// - The server (MapManager::sync) checks if the client quadrants and map texture rects are up to date or not compared to the server ones and send back the list of quadrants and map texture rects that need to be
					//   refreshed on the client side because they are different from the server ones (for example because they have been modified after last synchronization). The server answer with 
					//   - replyClientQuadrants: the quadrants currently owned by the client that need refresh (because they are newer on server and/or becasue they need client work to create splatmap/globalmap)
					//   - replyExtraQuadrants: the quadrants currently not owned by the client that need client work to create splatmap/globalmap
					//   - replyMapTextureRects: the map texture rects that need refresh
					//   - replyProceGenStatus: a strucutr containing the status of the procedural generation for the world
					// - The client (GDN_TheWorld_Viewer::replyFromServer, method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC) receive the above structures and:
					//   - for the quadrants in replyClientQuadrants that need refresh or for the quadrants in replyExtraQuadrants owned by the client (they are contained in m_worldQuadrantsMap but don't need a refresh):
					//     - set quadrant status to "need refresh" for the quadrants in m_worldQuadrantsMap (quadrants currently owned by the client) ==> quadTree->setStatus(QuadrantStatus::refreshTerrainDataNeeded)
					//     - in the process procedure (GDN_TheWorld_Viewer::process_initNewQuads \ GDN_TheWorld_Viewer::initNewQuadsAtPriority) refresh is scheduled to server for the quadrant in status QuadrantStatus::refreshTerrainDataNeeded
					//       using QuadTree::refreshTerrainData / Quadrant::populateGridVertices and status change to refreshTerrainDataInProgress ==> quadTree->setStatus(QuadrantStatus::refreshTerrainDataInProgress)
					//     - The client (GDN_TheWorld_Viewer::replyFromServer, method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER) receive quadrant server data and change status to initialized ==> quadTree->setStatus(QuadrantStatus::initialized)
					//   - for the quadrants in replyExtraQuadrants not owned by the client (not contained in m_worldQuadrantsMap), they are the ones that need client work but are not owned by the client:
					//     - add them to m_extraQuadsMap initializing new quadrant (status = QuadrantStatus::uninitialized) with the pos and the meshid received from the server for the quadrant and call 
					//       m_extraQuadsMap[pos]->requestInitDelayed
					//     - in the process procedure (GDN_TheWorld_Viewer::process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution) for the quadrants in m_extraQuadsMap with status == QuadrantStatus::uninitialized, call
					//       m_extraQuadsMap[pos]->initDelayed that will call for the quadrant buffer data to server changing the status to getTerrainDataInProgress ==> setStatus(QuadrantStatus::getTerrainDataInProgress)
					//     - The client (GDN_TheWorld_Viewer::replyFromServer, method == THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTBUFFER) receive quadrant server data and change status to initialized ==> quadTree->setStatus(QuadrantStatus::initialized)
					//   - for the rects in replyMapTextureRects that need refresh:
					//     - call for the map texture rects that need refresh map->addMapTextureRectRefreshScheduled(pos);
					//     - ???
					//   - for the structure replyProceGenStatus manage the procedural generation status for the world passing its values to the editor
					// - From there the process procedure (GDN_TheWorld_Viewer::process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution \ getQuadrant()->process_mapModder) if splatmap / globalmap is to generate 
					//   calls m_mapModder->initForSplatMap / m_mapModder->initForGlobalMap and so the map modder manage the creation of the splatmap / globalmap for the quadrants and when ready quadrantModderWorkDone perform the upload
					//   of the modified buffer to the server updating the hash of the quadrant (the meshid) so that the new version of the quadrant data are refreshed with a new sync ...

					std::string quadrants;
					std::string mapTextureRects;
					if (not m_quadrantSyncInProgress and not editorUploadToServerInProgress and (Globals()->status() == TheWorldStatus::worldDeployed or Globals()->status() == TheWorldStatus::minimalWorldDeployed))
					{
						std::unique_lock<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing, std::try_to_lock);
						if (lock.owns_lock())
						{

							m_quadrantSyncInProgress = true;

							size_t allocationSlot = 100000;
							size_t numAllocationSlots = 1;
							TheWorld_Utils::MemoryBuffer buffer;
							buffer.reserve(allocationSlot * numAllocationSlots++);		// preallocation for optimization purpose
							
							*buffer.ptr() = '0';
							size_t cursor = 1;
							buffer.adjustSize(cursor);

							// Check with server if updated versions of world quadrant data exist server side
							{
								size_t size;
								TheWorld_Utils::serializeToByteStream<size_t>(m_worldQuadrantsMap.size(), buffer.ptr() + cursor, size);
								cursor += size;
								buffer.adjustSize(cursor);

								for (auto& worldQuadrantsMapItem : m_worldQuadrantsMap)
								{
									TheWorld_Utils::MemoryBuffer quadrantPosBuffer;
									worldQuadrantsMapItem.first.serialize(quadrantPosBuffer);

									buffer.reserve(cursor + sizeof(size_t));		// to avoid memory overflow
									TheWorld_Utils::serializeToByteStream<size_t>(quadrantPosBuffer.size(), buffer.ptr() + cursor, size);
									cursor += size;
									buffer.adjustSize(cursor);

									buffer.reserve(cursor + quadrantPosBuffer.size());		// to avoid memory overflow
									memcpy(buffer.ptr() + cursor, quadrantPosBuffer.ptr(), quadrantPosBuffer.size());
									cursor += quadrantPosBuffer.size();
									buffer.adjustSize(cursor);

									std::string meshId = worldQuadrantsMapItem.second->getQuadrant()->getMeshId();

									buffer.reserve(cursor + 8);		// to avoid memory overflow
									TheWorld_Utils::serializeToByteStream<size_t>(meshId.size(), buffer.ptr() + cursor, size);
									cursor += size;
									buffer.adjustSize(cursor);

									buffer.reserve(cursor + meshId.size());		// to avoid memory overflow
									memcpy(buffer.ptr() + cursor, meshId.c_str(), meshId.size());
									cursor += meshId.size();
									buffer.adjustSize(cursor);
								}

								//buffer.adjustSize(cursor);
								buffer.toString(quadrants);
							}


							if (mapMode() and not serverGenerationInProgress() and not worldStreamingInProgress())
							{
								// Check with server if updated versions of map texture rects exist server side

								GDN_TheWorld_Map* map = Map();
								if (map != nullptr)
								{
									std::vector<TheWorld_Utils::MapTextureRectPos> rectsPos;
									map->getScreenTextureRects(rectsPos);

									buffer.clear();
									buffer.reserve(100000);		// preallocation for optimization purpose

									*buffer.ptr() = '0';
									cursor = 1;

									buffer.serializeToBufferAndTruncate<size_t>(rectsPos.size(), cursor);
									//TheWorld_Utils::serializeToByteStream<size_t>(rectsPos.size(), buffer.ptr() + cursor, size);
									//cursor += size;
									//buffer.adjustSize(cursor);

									TheWorld_Utils::MapTextureRectPos nullMapTextureRectPos;
									GDN_TheWorld_Map::MapTextureRect nullMapTextureRect(map, nullMapTextureRectPos);

									for (auto& textureRectPos : rectsPos)
									{
										std::lock_guard<std::recursive_mutex> lock(getMainProcessingMutex());
										
										GDN_TheWorld_Map::MapTextureRect* mapTextureRect = map->getMapTextureRect(textureRectPos, false);
										if (mapTextureRect == nullptr or !mapTextureRect->canCheckForRefresh())
											mapTextureRect = &nullMapTextureRect;

										// TheWorld_Utils::MapTextureRectPos
										assert(textureRectPos.initialized());
										TheWorld_Utils::MemoryBuffer textureRectPosBuffer;
										textureRectPos.serialize(textureRectPosBuffer);
										buffer.serializeToBufferAndTruncate(textureRectPosBuffer, cursor);

										// File Time Str
										std::string fileTime = mapTextureRect->fileTimeStr();
										buffer.serializeToBufferAndTruncate(fileTime, cursor);
									}
									
									//buffer.adjustSize(cursor);
									buffer.toString(mapTextureRects);
								}
							}

							lock.unlock();
						}
					}
					
					Globals()->Client()->MapManagerSync(isInEditor(), isEditMode(), Globals()->numVerticesPerQuadrantSize(), Globals()->gridStepInWU(), quadrants, mapTextureRects);
					
					lastSyncTime = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				}

				// Upload Built Texture Rect
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

					if (not m_uploadTextureRectInProgress)
					{
						m_uploadTextureRectInProgress = true;

						try
						{
							bool uploadInProgress = false;

							if (m_map != nullptr)
							{
								GDN_TheWorld_Map* map = Map();
								if (map != nullptr)
								{
									uploadInProgress = map->uploadSelectedTextureRect();
								}
							}
							
							if (!uploadInProgress)
								m_uploadTextureRectInProgress = false;
						}
						catch (std::exception& e)
						{
							m_uploadTextureRectInProgress = false;
							Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::streamer - Upload Built Texture Rect - std::exception caught - ") + e.what()).c_str());
							//throw(e);
						}
						catch (...)
						{
							m_uploadTextureRectInProgress = false;
							Globals()->errorPrint("GDN_TheWorld_Viewer::streamer - Upload Built Texture Rect - Exception caught");
							//throw(new exception("exception caught"));
						}
					}
				}

				// Check Server Map Texture Rect Available
				{
					std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrantsAndMainProcessing);

					if (not m_checkServerTextureRectInProgress)
					{
						m_checkServerTextureRectInProgress = true;

						try
						{
							bool checkInProgress = false;
							
							if (m_map != nullptr)
							{
								GDN_TheWorld_Map* map = Map();
								if (map != nullptr)
								{
									checkInProgress = map->checkRequiredTextureRect();
								}
							}

							if (!checkInProgress)
								m_checkServerTextureRectInProgress = false;
						}
						catch (std::exception& e)
						{
							m_checkServerTextureRectInProgress = false;
							Globals()->errorPrint((std::string("GDN_TheWorld_Viewer::streamer - Check Server Map Texture Rect Available - std::exception caught - ") + e.what()).c_str());
							//throw(e);
						}
						catch (...)
						{
							m_checkServerTextureRectInProgress = false;
							Globals()->errorPrint("GDN_TheWorld_Viewer::streamer - Check Server Map Texture Rect Available - Exception caught");
							//throw(new exception("exception caught"));
						}
					}
				}
				
				if (m_dumpScheduled)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("streamer 1 ") + __FUNCTION__, "Dump stuff");

					dump();
					m_dumpScheduled = false;
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

	m_testQuadrantHeightmapImage = ShaderTerrainData::createHeightmapImage(heights16Buffer, (int)numVerticesPerSize, nullptr);
	m_testQuadrantNormalmapImage = ShaderTerrainData::createNormalmapImage(normalsBuffer, (int)numVerticesPerSize);
	m_testQuadrantSplatmapImage = ShaderTerrainData::createSplatlmapImage(splatmapBuffer, (int)numVerticesPerSize);
	m_testQuadrantGlobalmapImage = ShaderTerrainData::createGlobalmapImage(globalmapBuffer, (int)numVerticesPerSize);
	m_testQuadrantColormapImage = ShaderTerrainData::createColormapImage(colormapBuffer, (int)numVerticesPerSize);
	
	return true;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getTestQuadrantHeightmapImage()
{
	return m_testQuadrantHeightmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getTestQuadrantNormalmapImage()
{
	return m_testQuadrantNormalmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getTestQuadrantSplatmapImage()
{
	return m_testQuadrantSplatmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getTestQuadrantGlobalmapImage()
{
	return m_testQuadrantGlobalmapImage;
}

godot::Ref<godot::Image> GDN_TheWorld_Viewer::getTestQuadrantColormapImage()
{
	return m_testQuadrantColormapImage;
}

godot::Vector2 GDN_TheWorld_Viewer::getQuadrantPos(void)
{
	return godot::Vector2(m_tempPos.getLowerXGridVertex(), m_tempPos.getLowerZGridVertex());
}

godot::Vector2i GDN_TheWorld_Viewer::getQuadrantPosIndex(void)
{
	return godot::Vector2i(m_tempPos.getIndexPosX(), m_tempPos.getIndexPosZ());
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

int GDN_TheWorld_Viewer::getGroundTextureIdx(godot::String& layout, godot::String texName)
{
	std::string _layout = layout.utf8().get_data();
	std::string _texName = texName.utf8().get_data();
	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string textureLayout;
	return groundTextures.getTextureIdx(_layout, _texName, textureLayout);
}

godot::Vector4 GDN_TheWorld_Viewer::getGroundTextureIdxs(godot::String layout)
{
	godot::Vector4 v(
		(float)getGroundTextureIdx(layout, m_tempTerrainEdit.extraValues.lowElevationTexName_r),
		(float)getGroundTextureIdx(layout, m_tempTerrainEdit.extraValues.highElevationTexName_g),
		(float)getGroundTextureIdx(layout, m_tempTerrainEdit.extraValues.dirtTexName_b),
		(float)getGroundTextureIdx(layout, m_tempTerrainEdit.extraValues.rocksTexName_a)
		);
	return v;
}

void GDN_TheWorld_Viewer::clearTestQuadrantData(void)
{
	m_testQuadrantHeightmapImage.unref();
	m_testQuadrantNormalmapImage.unref();
	m_testQuadrantSplatmapImage.unref();
	m_testQuadrantGlobalmapImage.unref();
	m_testQuadrantColormapImage.unref();
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

