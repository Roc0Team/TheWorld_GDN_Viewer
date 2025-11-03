//#include "pch.h"
#include "QuadTree.h"
#include "Viewer_Utils.h"
#include "json/json.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Quadrant.h"
#include "GDN_TheWorld_Edit.h"
#include "Viewer_Utils.h"

#include "Profiler.h"
#include "half.h"
#include "Utils.h"

#include "assert.h"
#include <filesystem>
#include <iostream>
#include <fstream>

#pragma warning(push, 0)
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/resource_loader.hpp>
#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/compressed_texture2d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/color_rect.hpp>
#include <godot_cpp/classes/viewport_texture.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/texture2D_array.hpp>
#pragma warning(pop)

using namespace godot;
namespace fs = std::filesystem;

GDN_TheWorld_ShaderTexture* ShaderTerrainData::g_lookDevShaderTexture = nullptr;
const std::string Quadrant::c_defaultTextureLayout = "default";

//extern int g_num = 0;

// QuadTree coordinates are local to Viewer Node which contains the grid of quads: the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

MapQuadrantImage::MapQuadrantImage(TheWorld_Utils::QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, size_t optimizationLevel)
{
	m_quadrantPos = quadrantPos;
	m_viewer = viewer;
	m_optimizationLevel = optimizationLevel;	// 0 = 2049, 2 = 513, 4 = 129, 6 = 33
	m_imageResolution = (int32_t)m_viewer->Globals()->resolutionForOptimizationLevel((size_t)m_optimizationLevel);
	std::string mapName = m_viewer->Globals()->getMapName();
	std::string dir = GDN_TheWorld_Globals::getClientDataDir();
	m_cache = TheWorld_Utils::MeshCacheBuffer(dir, mapName, m_quadrantPos.getGridStepInWU(), m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getLevel(), m_quadrantPos.getLowerXGridVertex(), m_quadrantPos.getLowerZGridVertex());
}

MapQuadrantImage::~MapQuadrantImage()
{
}

TheWorld_Utils::MeshCacheBuffer& MapQuadrantImage::getMeshCacheBuffer(void)
{
	return m_cache;
}

std::time_t MapQuadrantImage::getRefreshTime(void)
{
	std::time_t ret = -1;

	if (!m_meshId.empty())
	{
		std::string meshIdWithoutBinaryTime;
		std::chrono::system_clock::time_point tp = TheWorld_Utils::MeshCacheBuffer::getTimeFromMeshId(m_meshId, meshIdWithoutBinaryTime);
		ret = std::chrono::system_clock::to_time_t(tp);
	}
	
	return ret;		// -1 quadrant not intialized, 0 = empty quadrant, >0 = quadrant last refresh time
}

godot::Ref<godot::Image> MapQuadrantImage::getMapImage(void)
{
	if (m_mapImage == nullptr)
	{
		m_mapImage = godot::Image::create(getImageResolution(), getImageResolution(), false, godot::Image::Format::FORMAT_RGB8);
		//m_mapImage->fill(godot::Color(GDN_TheWorld_Globals::g_color_white));
		m_mapImage->fill(water_globalmap_gdshader_u_sea_color);
	}

	return m_mapImage;
}

void MapQuadrantImage::setMapData(godot::Ref<godot::Image> mapImage)
{
	m_mapImage = mapImage;
	m_lastUpdateTime = TIME()->get_ticks_msec();
	updated(true);
	//PLOG_DEBUG << "#################### updated(true) - Pos: " << getPos().getIndexPosX() << "," << getPos().getIndexPosZ();
}

void MapQuadrantImage::refreshDataFromServer(std::string& bufferFromServer, std::string& meshIdFromServer, std::string outMeshIdFromBuffer, bool updateCache)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("refreshDataFromServer 1 ") + __FUNCTION__, "Global");

	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	outMeshIdFromBuffer.clear();

	TheWorld_Utils::MemoryBuffer globalmapBuffer;
	float minAltitude = 0, maxAltitude = 0;
	TheWorld_Utils::MemoryBuffer float16HeigthsBuffer;
	TheWorld_Utils::MemoryBuffer float32HeigthsBuffer;
	TheWorld_Utils::MemoryBuffer normalsBuffer;
	TheWorld_Utils::MemoryBuffer splatmapBuffer;
	TheWorld_Utils::MemoryBuffer colormapBuffer;
	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("refreshDataFromServer 1.1 ") + __FUNCTION__, "cache.refreshMapsFromBuffer");

		//TheWorld_Utils::MeshCacheBuffer cache;
		m_cache.refreshMapsFromBuffer(bufferFromServer, outMeshIdFromBuffer, terrainEditValuesBuffer, minAltitude, maxAltitude, float16HeigthsBuffer, float32HeigthsBuffer, normalsBuffer, splatmapBuffer, colormapBuffer, globalmapBuffer, updateCache);
	}

	if (outMeshIdFromBuffer.size() > 0)
	{
		size_t l1 = outMeshIdFromBuffer.size();
		size_t l2 = meshIdFromServer.size();
		const char* c1 = outMeshIdFromBuffer.c_str();
		const char* c2 = meshIdFromServer.c_str();
		//assert(outMeshIdFromBuffer == meshIdFromServer);
		if (outMeshIdFromBuffer != meshIdFromServer)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("MeshId from buffer not equal to meshId from server").c_str()));
	}
	
	if (outMeshIdFromBuffer.size() == 0 or terrainEditValuesBuffer.empty())
	{
		// Buffer from server was empty
		TheWorld_Utils::GuardProfiler profiler(std::string("refreshDataFromServer 1.1 ") + __FUNCTION__, "cache.refreshMapsFromDisk");
		bool empty = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), meshIdFromServer, terrainEditValuesBuffer, minAltitude, maxAltitude, float16HeigthsBuffer, float32HeigthsBuffer, normalsBuffer, splatmapBuffer, colormapBuffer, globalmapBuffer);
	}

	setMeshId(meshIdFromServer);
	
	if (globalmapBuffer.empty())
	{
		setMapData(nullptr);
	}
	else
	{
		int _resolution = globals->heightmapResolution() + 1;
		godot::PackedByteArray data;
		size_t globalmapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
		data.resize((int)globalmapBufferSizeInBytes);
		assert(globalmapBufferSizeInBytes == globalmapBuffer.size());
		{
			memcpy((char*)data.ptrw(), globalmapBuffer.ptr(), globalmapBufferSizeInBytes);
		}
		godot::Ref<godot::Image> hiResImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8, data);

		godot::Ref<godot::Image> resizedImage = godot::Image::create_from_data(hiResImage->get_width(), hiResImage->get_height(), false, hiResImage->get_format(), hiResImage->get_data());
		resizedImage->resize(getImageResolution(), getImageResolution(), godot::Image::Interpolation::INTERPOLATE_BILINEAR);

		{
			TheWorld_Utils::GuardProfiler profiler(std::string("refreshDataFromServer 1.2 ") + __FUNCTION__, "resize image (adjust borders) " + std::to_string(_resolution) + " ==> " + std::to_string(getImageResolution()));

			//size_t optimizationLevel = 4;	// 0 = 2049, 2 = 513, 4 = 129, 6 = 33;
			//int32_t newStep = 1 << optimizationLevel;
			int32_t newStep = (_resolution - 1) / (getImageResolution() - 1);
			int32_t hiResImageWidth = hiResImage->get_width();
			int32_t hiResImageHeight = hiResImage->get_height();
			int32_t resizedImageWidth = resizedImage->get_width();
			int32_t resizedImageHeight = resizedImage->get_height();
			for (int32_t x = 0; x < resizedImageWidth; x++)
			{
				resizedImage->set_pixel(x, 0, hiResImage->get_pixel(x * newStep, 0));
				resizedImage->set_pixel(x, resizedImageHeight - 1, hiResImage->get_pixel(x * newStep, hiResImageHeight - 1));
			}
			for (int32_t y = 0; y < resizedImageHeight; y++)
			{
				resizedImage->set_pixel(0, y, hiResImage->get_pixel(0, y * newStep));
				resizedImage->set_pixel(resizedImageWidth - 1, y, hiResImage->get_pixel(hiResImageWidth - 1, y * newStep));
			}
		}

		setMapData(resizedImage);
	}

	return;
}

Quad::Quad(enum class QuadType quadType, int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_quadType = quadType;
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = posInQuad;
	m_viewer = viewer;
	m_chunk = nullptr;
	m_chunkSizeInWUs = 0;
	m_splitRequired = false;

	m_quadTree = quadTree;

	assignChunk();
}

Quad::~Quad()
{
	clearChildren();
}

Quad* Quad::getChild(int idx)
{
	if (isLeaf())
		return nullptr;

	return m_children[idx].get();
}

bool Quad::isLeaf(void)
{
	return (m_children[0].get() == nullptr ? true : false);
}

void Quad::split(void)
{
	assert(m_lod > 0);
	assert(m_children[0] == nullptr);
	
	for (int i = 0; i < 4; i++)
	{
		//int x = (m_slotPosX * 2) + (i & 1);
		//int z = (m_slotPosZ * 2) + ((i & 2) >> 1);
		m_children[i] = make_unique<Quad>(m_quadType, (m_slotPosX * 2) + (i & 1), (m_slotPosZ * 2) + ((i & 2) >> 1), m_lod - 1, enum PosInQuad(i + 1), m_viewer, m_quadTree);
	}
	
	recycleChunk();
}

void Quad::recycleChunk(void)
{
	if (m_chunk)
	{
		m_chunk->setPosInQuad(PosInQuad::NotSet);
		m_chunk->setVisible(false);
		m_chunk->setActive(false);
		m_chunk = nullptr;
	}
}

void Quad::clearChildren(void)
{
	assert(m_children.size() == 0 || m_children.size() == 4);
	if (m_children.size() != 0)
		for (int i = 0; i < 4; i++)
		{
			if (m_children[i] != nullptr)
			{
				m_children[i]->recycleChunk();
				m_children[i].reset();
			}
		}
}

void Quad::assignChunk(void)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("Quad::assignChunk ") + __FUNCTION__, "ALL");

	assert(m_chunk == nullptr);
	
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("Quad::assignChunk ") + __FUNCTION__, "m_quadTree->getChunkAt");
		m_chunk = m_quadTree->getChunkAt(Chunk::ChunkPos(m_slotPosX, m_slotPosZ, m_lod), m_quadType);
	}

	if (m_chunk == nullptr)
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("Quad::assignChunk ") + __FUNCTION__, "new Chunk");
		// create chunk for current quad because this is the first time that is needed for current lod and pos
		if (m_quadType == QuadType::Terrain)
		{
			Ref<Material> mat = m_quadTree->getCurrentMaterial(m_quadType, m_lod);
			if (_DEBUG_AAB)
				m_chunk = new DebugTerrainChunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
			else
				m_chunk = new TerrainChunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
			m_chunk->setVisible(m_viewer->Globals()->getParamShowTerrain());
		}
		else
		{
			Ref<Material> mat = m_quadTree->getCurrentMaterial(m_quadType, m_lod);
			m_chunk = new SeaChunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
			m_chunk->setVisible(m_viewer->Globals()->getParamShowSea());
		}

		// Set Viewer Pos in Global Coord
		//m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk, m_quadType);
	}
	else
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("Quad::assignChunk ") + __FUNCTION__, "m_chunk->setMaterial");
		if (m_quadType == QuadType::Terrain)
		{
			Ref<Material> mat = m_quadTree->getCurrentMaterial(m_quadType, m_lod);
			m_chunk->setMaterial(mat);
			m_chunk->setVisible(m_viewer->Globals()->getParamShowTerrain());
		}
		else
		{
			Ref<Material> mat = m_quadTree->getCurrentMaterial(m_quadType, m_lod);
			m_chunk->setMaterial(mat);
			m_chunk->setVisible(m_viewer->Globals()->getParamShowSea());
		}
	}

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("Quad::assignChunk ") + __FUNCTION__, "set chunk values");

		m_chunk->setPosInQuad(m_posInQuad, this);
		m_quadTree->addChunkUpdate(m_chunk, m_quadType);
		m_chunk->setActive(true);
		m_chunkAABB = m_chunk->getAABB();
		m_chunkSizeInWUs = m_chunk->getChunkSizeInWUs();
		GDN_TheWorld_Globals::ChunkDebugMode debugMode = m_viewer->getChunkDebugMode();
		m_chunk->setDebugMode(debugMode);
		m_chunk->applyDebugMesh();
		if (debugMode != GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
			m_chunk->applyAABB();
	}
}

void Quad::setCameraPosInMap(Vector3 cameraPosInMap)
{
	if (isLeaf())
	{
		Chunk* chunk = getChunk();
		if (chunk != nullptr)
			chunk->setCameraPosInMap(cameraPosInMap);
	}
}
	
std::string QuadTree::quadTypeToString(enum class QuadType type)
{
	switch (type)
	{
	case QuadType::Terrain:
		return "Terrain";
	case QuadType::Sea:
		return "Sea";
	case QuadType::NotUsed:
		return "NotUsed";
	default:
		return "NotSet";
	}
}

std::string QuadTree::statusToString(enum class QuadrantStatus status)
{
	switch (status)
	{
	case QuadrantStatus::loadTerrainDataFromCacheNeeded:
		return "loadTerrainDataFromCacheNeeded";
	case QuadrantStatus::loadTerrainDataFromCacheInProgress:
		return "loadTerrainDataFromCacheInProgress";
	case QuadrantStatus::refreshTerrainDataNeeded:
		return "refreshTerrainDataNeeded";
	case QuadrantStatus::refreshTerrainDataInProgress:
		return "refreshTerrainDataInProgress";
	case QuadrantStatus::uninitialized:
		return "uninitialized";
	case QuadrantStatus::getTerrainDataInProgress:
		return "getTerrainDataInProgress";
	case QuadrantStatus::initialized:
		return "initialized";
	case QuadrantStatus::toErase:
		return "toErase";
	case QuadrantStatus::initializationStopped:
		return "initializationStopped";
	default:
		return "";
	}
}

std::string QuadTree::quadrantDepthToString(enum class QuadrantDepth depth)
{
	switch (depth)
	{
	case QuadrantDepth::NotSet:
		return "NotSet";
	case QuadrantDepth::Initial:
		return "Initial";
	case QuadrantDepth::Growing:
		return "Growing";
	case QuadrantDepth::Cache:
		return "Cache";
	default:
		return "";
	}
}

QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer, TheWorld_Utils::QuadrantPos quadrantPos)
{
	setStatus(QuadrantStatus::uninitialized);
	m_isVisible = false;
	m_viewer = viewer;
	m_numSplits = 0;
	m_numJoins = 0;
	m_numLeaf = 0;
	m_worldQuadrant = new Quadrant(quadrantPos, viewer, this);
	m_GDN_Quadrant = nullptr;
	m_tag = quadrantPos.getTag();
	int ret = timespec_get(&m_refreshTime, TIME_UTC);
	//m_lookDev = ShaderTerrainData::LookDev::NotSet;
	m_lookDev = m_viewer->getDesideredLookDev();
	m_quads[QuadType::Terrain] = std::make_unique<Quads>();
}

void QuadTree::refreshTerrainData(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrain)
{
	my_assert(statusInitialized() || statusRefreshTerrainDataNeeded());
	if (!(statusInitialized() || statusRefreshTerrainDataNeeded()))
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	setStatus(QuadrantStatus::refreshTerrainDataInProgress);

	m_worldQuadrant->populateGridVertices(cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll, setCamera, cameraDistanceFromTerrain);
}

void QuadTree::init(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "ALL");

	my_assert(status() == QuadrantStatus::uninitialized);
	if (status() != QuadrantStatus::uninitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "Part 1");

		m_viewer->getMainProcessingMutex().lock();
		m_GDN_Quadrant = memnew(GDN_TheWorld_Quadrant);
		m_viewer->getMainProcessingMutex().unlock();
		m_GDN_Quadrant->init(this, m_viewer);
		std::string quadrantNodeName = m_worldQuadrant->getPos().getName();
		m_GDN_Quadrant->set_name(quadrantNodeName.c_str());
		//m_viewer->add_child(m_GDN_Quadrant);
		m_viewer->call_deferred("add_child", m_GDN_Quadrant);
		onGlobalTransformChanged();

		setStatus(QuadrantStatus::getTerrainDataInProgress);
	}

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "m_worldQuadrant->populateGridVertices");
		m_worldQuadrant->populateGridVertices(cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll, setCamera, cameraDistanceFromTerrainForced);
	}

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "Part 2");
		getQuadrant()->getShaderTerrainData()->init();
	}
		
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "Part 3");

		m_quads[QuadType::Terrain]->m_quadRoot = std::make_unique<Quad>(QuadType::Terrain, 0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::Root, m_viewer, this);

		if (isVisible())
			m_viewer->incNumVisibleQuadrantsStreamed();
		else
			m_viewer->incNumInvisibleQuadrantsStreamed();
	}
}

void QuadTree::checkAndInitSeaQuadrant(void)
{
	if (status() == QuadrantStatus::initialized)
	{
		if (canContainSea() == QUADRANT_SEA_INFO_TRUE && !containsQuadOfType(QuadType::Sea))
		{
			m_viewer->getMainProcessingMutex().lock();
			m_quads[QuadType::Sea] = std::make_unique<Quads>();
			m_quads[QuadType::Sea]->m_quadRoot = make_unique<Quad>(QuadType::Sea, 0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::Root, m_viewer, this);
			m_viewer->getMainProcessingMutex().unlock();

			getQuadrant()->setHeightsUpdated(true);
			//getQuadrant()->setNormalsUpdated(true);
			//getQuadrant()->setSplatmapUpdated(true);
			//getQuadrant()->setColorsUpdated(true);
			//getQuadrant()->setGlobalmapUpdated(true);
			materialParamsNeedReset(true);
		}
	}
}

void QuadTree::onGlobalTransformChanged(void)
{
	//TheWorld_Utils::QuadrantPos quadrantPos = getQuadrant()->getPos();
	//Transform3D gt = m_viewer->getInternalGlobalTransform();
	//Transform3D t = gt * Transform3D(Basis(), Vector3(quadrantPos.getLowerXGridVertex(), 0, quadrantPos.getLowerZGridVertex()));
	//m_GDN_Quadrant->set_global_transform(t);
}

QuadTree::~QuadTree()
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree Desc 1 ") + __FUNCTION__, "ALL");

	//godot::GDN_TheWorld_Globals::s_num++;
	//TheWorld_Viewer_Utils::TimerMs clock;
	//clock.tick();

	if (m_GDN_Quadrant)
	{
		m_GDN_Quadrant->deinit();
		Node* parent = m_GDN_Quadrant->get_parent();
		if (parent)
		{
			//parent->remove_child(m_GDN_Quadrant);
			parent->call_deferred("remove_child", m_GDN_Quadrant);	// Mah
		}
		m_GDN_Quadrant->queue_free();
		m_GDN_Quadrant = nullptr;
	}

	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed1 += clock.duration().count();

	// Delete all Chunks
	//clock.tick();
	for (auto& item : m_quads)
	{
		item.second->m_quadRoot.reset();

		clearChunkUpdate(item.first);

		for (Chunk::MapChunk::iterator it = item.second->m_chunks.begin(); it != item.second->m_chunks.end(); it++)
		{
			for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
			{
				delete it1->second;
			}
			it->second.clear();
		}
		item.second->m_chunks.clear();
	}
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed4 += clock.duration().count();

	//clock.tick();
	if (m_worldQuadrant)
		delete m_worldQuadrant;
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed5 += clock.duration().count();
}

void QuadTree::checkSeaChunksToDelete(void)
{
	if (m_quads.contains(QuadType::Sea) && !canContainSea())
	{
		m_quads[QuadType::Sea]->m_quadRoot.reset();
		clearChunkUpdate(QuadType::Sea);
		for (Chunk::MapChunk::iterator it = m_quads[QuadType::Sea]->m_chunks.begin(); it != m_quads[QuadType::Sea]->m_chunks.end(); it++)
		{
			for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
			{
				delete it1->second;
			}
			it->second.clear();
		}
		m_quads[QuadType::Sea]->m_chunks.clear();
		m_quads.erase(QuadType::Sea);
	}
}

bool QuadTree::statusInProgressOnServer(void) const
{
	if (m_status == QuadrantStatus::getTerrainDataInProgress || m_status == QuadrantStatus::refreshTerrainDataInProgress || m_status == QuadrantStatus::loadTerrainDataFromCacheInProgress)
		return true;
	else
		return false;
}

enum class QuadrantStatus QuadTree::status(void)
{
	if (statusInProgressOnServer())
	{
		TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
		long long elapsedFromStatusChange = (now - m_lastStatusChange).count();
		if (elapsedFromStatusChange > (THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE))
		{
			m_status = QuadrantStatus::uninitialized;
			//PLOG_DEBUG << "TRACKING status (set to QuadrantStatus::uninitialized for timeout) " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << getQuadrant()->getPos().getIndexPosX() << "Z=" << getQuadrant()->getPos().getIndexPosZ();
		}
	}
	return m_status;
}

void QuadTree::setPrevStatus(void)
{
	assert(m_status == QuadrantStatus::toErase);
	if (m_status != QuadrantStatus::toErase)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not toErase" + std::to_string((int)status()) + ")").c_str()));
	assert(m_prevStatus != QuadrantStatus::toErase);
	if (m_prevStatus == QuadrantStatus::toErase)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant prev status cannot be toErase").c_str()));
	m_status = m_prevStatus;
}

void QuadTree::setStatus(enum class QuadrantStatus status)
{
	//if (m_worldQuadrant != nullptr && m_worldQuadrant->getPos().isInitialized() && m_worldQuadrant->getPos().getIndexPosX() == -2 && m_worldQuadrant->getPos().getIndexPosZ() == 3)
	//{
	//	PLOG_DEBUG << "TRACKING setStatus to " << statusToString(status) << " " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << getQuadrant()->getPos().getIndexPosX() << "Z=" << getQuadrant()->getPos().getIndexPosZ();
	//}
	
	m_lastStatusChange = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	if (status == QuadrantStatus::toErase && m_status != QuadrantStatus::toErase)
		m_prevStatus = m_status;

	//if (status == QuadrantStatus::refreshTerrainDataNeeded)	// DEBUG ONLY
	//	m_status = status;									// DEBUG ONLY
	m_status = status;
	Quadrant* quadrant = getQuadrant();
	if (quadrant != nullptr)
		quadrant->lockInternalDataUntilCompleteProcessExecution();

	if (m_status == QuadrantStatus::initialized && canContainSea() == QUADRANT_SEA_INFO_NOTSET)
	{
		//if (this->getQuadrant()->getPos().getIndexPosX() == 2 && this->getQuadrant()->getPos().getIndexPosZ() == 3)
		//{
		//	PLOG_DEBUG << "TRACKING Chunk::TerrainHeightsChangedChunkAction::preExec " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << this->getQuadrant()->getPos().getIndexPosX() << "Z=" << this->getQuadrant()->getPos().getIndexPosZ();
		//}

		refreshSeaInfo();
	}

	switch (m_status)
	{
		case QuadrantStatus::initialized:
		{
			TheWorld_Utils::QuadrantPos pos = getQuadrant()->getPos();
			quadrantReady(true);
			m_viewer->worldQuadrantReady(pos);
		}
		break;
		case QuadrantStatus::loadTerrainDataFromCacheNeeded:
		case QuadrantStatus::loadTerrainDataFromCacheInProgress:
		case QuadrantStatus::refreshTerrainDataNeeded:
		case QuadrantStatus::refreshTerrainDataInProgress:
		case QuadrantStatus::getTerrainDataInProgress:
		{
			TheWorld_Utils::QuadrantPos pos = getQuadrant()->getPos();
			quadrantReady(false);
			m_viewer->worldQuadrantWorkInProgress(pos);
		}
		break;
	}
}

void QuadTree::refreshSeaInfo(void)
{
	TheWorld_Utils::TerrainEdit* terrainEdit = getQuadrant()->getTerrainEdit();
	if (terrainEdit->extraValues.seaInfoSet == QUADRANT_SEA_INFO_TRUE)
	{
		if (terrainEdit->extraValues.canContainSea == QUADRANT_SEA_INFO_TRUE)
			canContainSea(QUADRANT_SEA_INFO_TRUE);
		else
			canContainSea(QUADRANT_SEA_INFO_FALSE);
	}
	else
	{
		if (terrainEdit->heightsNeedRegen)
		{
			// leaving m_canContainSea not modified
		}
		else
		{
			if (terrainEdit->canContainSea())
				canContainSea(QUADRANT_SEA_INFO_TRUE);
			else
				canContainSea(QUADRANT_SEA_INFO_FALSE);
		}
	}
}

void QuadTree::ForAllChunk(Chunk::ChunkAction& chunkAction, enum class QuadType type)
{
	chunkAction.preExec(this);

	if (type == QuadType::NotUsed || type == QuadType::NotSet)
	{
		int stop = int(QuadType::NotUsed);
		for (int i = 0; i < stop; i++)
		{
			type = (enum class QuadType)i;
			if (containsQuadOfType(type))
			{
				if (isValid() || isRefreshingData())
				{
					for (Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.begin(); it != m_quads[type]->m_chunks.end(); it++)
					{
						for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
						{
							chunkAction.exec(it1->second);
						}
					}
				}
			}
		}
	}
	else
	{
		if (containsQuadOfType(type))
		{
			if (isValid() || isRefreshingData())
			{
				for (Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.begin(); it != m_quads[type]->m_chunks.end(); it++)
				{
					for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
					{
						chunkAction.exec(it1->second);
					}
				}
			}
		}
	}

	chunkAction.postExec(this);
}

void QuadTree::mouseHitOnTerrainChanged(godot::Vector3 mouseHit, bool hit)
{
	mouseHitChanged(mouseHit, hit, QuadType::Terrain);
}

void QuadTree::mouseHitChanged(godot::Vector3 mouseHit, bool hit, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	m_quads[type]->m_hitByMouse = hit;

	godot::Vector2 viewportMouseHit;

	if (hit)
	{
		TheWorld_Utils::QuadrantPos pos = getQuadrant()->getPos();
		//float s = pos.getSizeInWU();							// DEBUG ONLY
		//float dx = mouseHit.x - pos.getLowerXGridVertex();		// DEBUG ONLY
		//float dz = mouseHit.z - pos.getLowerZGridVertex();		// DEBUG ONLY
		viewportMouseHit.x = (mouseHit.x - pos.getLowerXGridVertex()) / pos.getSizeInWU();
		viewportMouseHit.y = (mouseHit.z - pos.getLowerZGridVertex()) / pos.getSizeInWU();
	}
	else
	{
		viewportMouseHit.x = -1;
		viewportMouseHit.y = -1;
	}

	getQuadrant()->getShaderTerrainData()->mouseHitChanged(viewportMouseHit);
}

void QuadTree::getAdjacentQuadrants(std::list<QuadTree*>& adjacentQuadrantsHit)
{
	adjacentQuadrantsHit.clear();
	TheWorld_Utils::QuadrantPos currentPos = getQuadrant()->getPos();

	TheWorld_Utils::QuadrantPos pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
	QuadTree* quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NortheastZMinusXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthwestZPlusXMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SoutheastZPlusXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
}

void QuadTree::forceUpdateForAllChunks(enum class QuadType type)
{
	for (auto& item : m_quads[type]->m_chunks)
	{
		for (auto& item1 : item.second)
		{
			item1.second->update(m_viewer->is_visible_in_tree());
		}
	}
}

void QuadTree::checkMouseHitOnTerrain(void)
{
	checkMouseHit(QuadType::Terrain);
}

void QuadTree::checkMouseHit(enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	if (statusLoadTerrainDataFromCacheInProgress() || statusLoadTerrainDataFromCacheNeeded())
	{
		m_viewer->forceProcessWorldCameraPosForChunks();
		return;
	}

	if (!isValid())
		return;

	if (!isVisible())
		return;

	if (getQuadrant()->internalDataLocked())
		return;

	internalCheckMouseHit(m_quads[type]->m_quadRoot.get());
}

void QuadTree::internalCheckMouseHit(Quad* quad)
{
	if (quad->isLeaf())
	{
		quad->getChunk()->checkMouseHit();
	}
	else
	{
		for (int i = 0; i < 4; i++)
		{
			Quad* child = quad->getChild(i);
			internalCheckMouseHit(child);
		}
	}
}

void QuadTree::update(Vector3 cameraPosInMap, enum class UpdateStage updateStage, int& numSplitRequired, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	if (statusLoadTerrainDataFromCacheInProgress() || statusLoadTerrainDataFromCacheNeeded())
	{
		m_viewer->forceProcessWorldCameraPosForChunks();
		return;
	}
	
	if (!isValid())
		return;

	if (!isVisible())
		return;

	if (getQuadrant()->internalDataLocked())
		return;

	m_numLeaf = 0;
	internalUpdate(cameraPosInMap, m_quads[type]->m_quadRoot.get(), updateStage, numSplitRequired, type);
	assert(!m_quads[type]->m_quadRoot->isLeaf() || m_quads[type]->m_quadRoot->getChunk() != nullptr);
}

void QuadTree::internalUpdate(godot::Vector3 cameraPosInMap, Quad* quad, enum class UpdateStage updateStage, int& numSplitRequired, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	if (updateStage == UpdateStage::Stage1)
	{
		quad->setJustSplitted(false);
		quad->setJustJoined(false);

		// cameraPosViewerNodeLocalCoord are in grid local coordinates (WUs)
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		float chunkSizeInWUs = globals->gridStepInHeightmapWUs(quad->Lod()) * globals->numVerticesPerChuckSide();									// chunk size in World Units
		AABB chunkAABB = quad->getChunkAABB();
		Vector3 quadCenterInMap(float((quad->slotPosX() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerXGridVertex() + chunkSizeInWUs / 2),
			(chunkAABB.position + chunkAABB.size / 2).y,
			float((quad->slotPosZ() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerZGridVertex() + chunkSizeInWUs / 2));

		//Transform3D gt = m_viewer->getInternalGlobalTransform();
		//Transform3D t = gt * Transform3D(Basis(), quadCenterGlobal);
		Transform3D t = Transform3D(Basis(), quadCenterInMap);
		quadCenterInMap = t.origin;

		float splitDistance = chunkSizeInWUs * globals->splitScale();
		//float splitDistance = m_worldQuadrant->getPos().getSizeInWU() * globals->splitScale();
		if (quad->isLeaf())
		{
			bool trackChunk = false;
			if (m_viewer->trackMouse() && getQuadrant()->getPos().getName() == m_viewer->getMouseQuadrantHitName())
				trackChunk = true;

			bool splitted = false;
			m_numLeaf++;
			if (quad->Lod() > 0)
			{
				float distance = (float)quadCenterInMap.distance_to(cameraPosInMap);
				quad->getChunk()->setDistanceFromCamera(distance);
				if (distance < splitDistance)
				{
					if (statusInitialized() && !getQuadrant()->hasHeigthsForChunkSplitsAndShaderRendering())	// split only if there are Heights16
					{
						setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);
						m_viewer->forceProcessWorldCameraPosForChunks();
					}
					else if (statusInitialized())
					{
						// Split
						quad->split();
						for (int i = 0; i < 4; i++)
						{
							quad->getChild(i)->setCameraPosInMap(cameraPosInMap);
							if (trackChunk)
								quad->getChild(i)->getChunk()->checkMouseHit();
							quad->getChild(i)->getChunk()->setDistanceFromCamera(distance);
						}
						quad->setJustSplitted(true);
						splitted = true;
						m_numSplits++;
						m_forceRecalcOptimizationLevel = true;
					}
					else
					{
						m_viewer->forceProcessWorldCameraPosForChunks();
					}
				}
			}

			if (!splitted && trackChunk)
				quad->getChunk()->checkMouseHit();
		}
		else
		{
			bool allChildrenAreLeaf = true;
			for (int i = 0; i < 4; i++)
			{
				Quad* child = quad->getChild(i);
				internalUpdate(cameraPosInMap, child, updateStage, numSplitRequired, type);
				if (!child->isLeaf())
					allChildrenAreLeaf = false;
			}

			if (allChildrenAreLeaf)
			{
				float distance = (float)quadCenterInMap.distance_to(cameraPosInMap);
				if (distance > splitDistance)
				{
					// Join
					quad->clearChildren();
					quad->assignChunk();
					quad->getChunk()->setJustJoined(true);
					quad->setJustJoined(true);
					m_numJoins++;
					quad->getChunk()->checkMouseHit();
					quad->getChunk()->setDistanceFromCamera(distance);
					m_forceRecalcOptimizationLevel = true;
				}
			}
		}

		// after all if quad is leaf: just joined or was leaf and it has not been splitted
		quad->setCameraPosInMap(cameraPosInMap);
	}
	else if (updateStage == UpdateStage::Stage2)
	{
		if (quad->isLeaf())
		{
			PosInQuad posInQuad = quad->getPosInQuad();
			Chunk* chunk = quad->getChunk();
			assert(chunk != nullptr);
			
			//std::string id = chunk->getIdStr();
			//if ((chunk->getLod() == 0 && chunk->getSlotPosX() == 31 && chunk->getSlotPosZ() == 31) || id == "LOD:0-X:31-Z:31")
			//{
			//	PLOG_DEBUG << "eccolo";
			//}

			if (chunk != nullptr)
			{
				switch (posInQuad)
				{
					case PosInQuad::First:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (First) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (First) - " << " ZMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
					}
					break;
					case PosInQuad::Second:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Second) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Second) - " << " ZMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
					}
					break;
					case PosInQuad::Third:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Third) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Third) - " << " ZPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
					}
					break;
					case PosInQuad::Forth:
					{
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Forth) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod, type);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Forth) - " << " ZPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
					}
					break;
				}
			}
		}
		else
		{
			for (int i = 0; i < 4; i++)
				internalUpdate(cameraPosInMap, quad->getChild(i), updateStage, numSplitRequired, type);
		}

	}
	else	// (updateStage == UpdateStage::Stage3)
	{
		bool splitted = false;
		if (quad->isLeaf())
		{
			bool trackChunk = false;
			if (m_viewer->trackMouse() && getQuadrant()->getPos().getName() == m_viewer->getMouseQuadrantHitName())
				trackChunk = true;

			if (quad->splitRequired() && quad->Lod() > 0)
			{
				if (statusInitialized() && !getQuadrant()->hasHeigthsForChunkSplitsAndShaderRendering())	// split only if there are Heights16
				{
					//godot::Vector3 quadrantCenter = getQuadrant()->getCenter();
					//quadrantCenter.y = (getQuadrant()->getTerrainEdit()->maxHeight + getQuadrant()->getTerrainEdit()->minHeight) / 2;
					//float lastComputedDistanceFromCamera = abs((cameraPosGlobalCoord - quadrantCenter).length());
					setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);
					m_viewer->forceProcessWorldCameraPosForChunks();
				}
				else if (statusInitialized())
				{
					// Split
					quad->split();
					for (int i = 0; i < 4; i++)
					{
						quad->getChild(i)->setCameraPosInMap(cameraPosInMap);
						if (trackChunk)
							quad->getChild(i)->getChunk()->checkMouseHit();
						quad->getChild(i)->getChunk()->setDistanceFromCamera(0);
					}
					quad->setJustSplitted(true);
					splitted = true;
					m_numSplits++;
					m_forceRecalcOptimizationLevel = true;
				}
				else
				{
					m_viewer->forceProcessWorldCameraPosForChunks();
				}
			}

			if (!splitted && trackChunk)
				quad->getChunk()->checkMouseHit();
		}
		else
		{
			bool allChildrenAreLeaf = true;
			for (int i = 0; i < 4; i++)
			{
				Quad* child = quad->getChild(i);
				internalUpdate(cameraPosInMap, child, updateStage, numSplitRequired, type);
				if (!child->isLeaf())
					allChildrenAreLeaf = false;
			}
		}

		quad->setSplitRequired(false);
	}
}

bool QuadTree::isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, TheWorld_Utils::QuadrantPos& XMinusQuadrantPos, TheWorld_Utils::QuadrantPos& XPlusQuadrantPos, TheWorld_Utils::QuadrantPos& ZMinusQuadrantPos, TheWorld_Utils::QuadrantPos& ZPlusQuadrantPos)
{
	bool ret = false;

	if (pos.getSlotPosX() == 0)
	{
		XMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
		ret = true;
	}

	if (pos.getSlotPosZ() == 0)
	{
		ZMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
		ret = true;
	}

	int numChunksPerSide = m_viewer->Globals()->numChunksPerHeightmapSide(pos.getLod());

	if (pos.getSlotPosX() == (numChunksPerSide - 1))
	{
		XPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
		ret = true;
	}

	if (pos.getSlotPosZ() == (numChunksPerSide - 1))
	{
		ZPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
		ret = true;
	}

	return ret;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir, enum class QuadType type)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos, type);
		break;
	case Chunk::DirectionSlot::XMinusChunk:
		//
		// - o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1), type);
		break;
	case Chunk::DirectionSlot::XPlusChunk:
		//
		//   o -
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1), type);
		break;
	case Chunk::DirectionSlot::ZMinusChunk:
		//   -
		//   o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1), type);
		break;
	case Chunk::DirectionSlot::ZPlusChunk:
		//
		//   o
		//   -
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1), type);
		break;
	case Chunk::DirectionSlot::XMinusQuadSecondChunk:
		//  
		//	- o =
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1), type);
		break;
	case Chunk::DirectionSlot::XMinusQuadForthChunk:
		//  
		//	  o =
		//	- = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1), type);
		break;
	case Chunk::DirectionSlot::XPlusQuadFirstChunk:
		//  
		//	  o = -
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1), type);
		break;
	case Chunk::DirectionSlot::XPlusQuadThirdChunk:
		//  
		//	  o = 
		//	  = = -
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1), type);
		break;
	case Chunk::DirectionSlot::ZMinusQuadThirdChunk:
		//    -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1), type);
		break;
	case Chunk::DirectionSlot::ZMinusQuadForthChunk:
		//      -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1), type);
		break;
	case Chunk::DirectionSlot::ZPlusQuadFirstChunk:
		//    
		//	  o =
		//	  = =
		//    -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1), type);
		break;
	case Chunk::DirectionSlot::ZPlusQuadSecondChunk:
		//      
		//	  o =
		//	  = =
		//      -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 2, -1), type);
		break;
	//default:
	}

	return chunk;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return nullptr;

	Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.find(pos.getLod());
	if (it == m_quads[type]->m_chunks.end())
		return nullptr;
	else
	{
		Chunk::MapChunkPerLod::iterator it1 = m_quads[type]->m_chunks[pos.getLod()].find(pos);
		if (it1 == m_quads[type]->m_chunks[pos.getLod()].end())
			return nullptr;
		else
			return m_quads[type]->m_chunks[pos.getLod()][pos];
	}
}

void QuadTree::addChunk(Chunk* chunk, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	if (getChunkAt(chunk->getPos(), type))
		return;
	
	Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.find(chunk->getLod());
	if (it == m_quads[type]->m_chunks.end())
	{
		Chunk::MapChunkPerLod m;
		m_quads[type]->m_chunks[chunk->getLod()] = m;
	}

	m_quads[type]->m_chunks[chunk->getLod()][chunk->getPos()] = chunk;
}

void QuadTree::addChunkUpdate(Chunk* chunk, enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	if (chunk->isPendingUpdate())
		return;

	m_quads[type]->m_updatedChunks.push_back(chunk);
	chunk->setPendingUpdate(true);
}

void QuadTree::clearChunkUpdate(enum class QuadType type)
{
	if (!containsQuadOfType(type))
		return;

	m_quads[type]->m_updatedChunks.clear();
}

void QuadTree::setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, enum class QuadType type, bool reload)
{
	switch (type)
	{
	case QuadType::Terrain:
	{
		if (!containsQuadOfType(type))
			return;

		my_assert(lookDev != ShaderTerrainData::LookDev::NotSet);
		if (lookDev == ShaderTerrainData::LookDev::NotSet)
			return;

		if (!reload && m_lookDev == lookDev)
			return;

		m_lookDev = lookDev;

		getQuadrant()->setHeightsUpdated(true);
		getQuadrant()->setNormalsUpdated(true);
		getQuadrant()->setSplatmapUpdated(true);
		getQuadrant()->setColorsUpdated(true);
		getQuadrant()->setGlobalmapUpdated(true);

		materialParamsNeedReset(true);

		if (lookDev == ShaderTerrainData::LookDev::ChunkLod)
		{
			if (isValid())
			{
				if (reload)
				{
					std::map<int, godot::Ref<godot::ShaderMaterial>> lookDevChunkLodMaterials = getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterials();
					for (auto& item : lookDevChunkLodMaterials)
					{
						getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterial(item.first, reload);
					}
				}

				for (Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.begin(); it != m_quads[type]->m_chunks.end(); it++)
				{
					for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
					{
						Ref<Material> mat = getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterial(it1->second->getLod());
						it1->second->setMaterial(mat);
					}
				}
			}

			getQuadrant()->getShaderTerrainData()->releaseRegularTerrainMaterial();
			getQuadrant()->getShaderTerrainData()->releaseLookDevMaterial();
		}
		else
		{
			Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getLookDevMaterial(lookDev, reload));
			ForAllChunk(action, type);
			getQuadrant()->getShaderTerrainData()->releaseRegularTerrainMaterial();
			getQuadrant()->getShaderTerrainData()->releaseLookDevChunkLodMaterials();
		}
	}
	break;
	case QuadType::Sea:
	{
		if (containsQuadOfType(type))
		{
			m_lookDev = lookDev;
		}
	}
	break;
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		setLookDevMaterial(lookDev, QuadType::Terrain, reload);
		setLookDevMaterial(lookDev, QuadType::Sea, reload);
		m_lookDev = lookDev;
	}
	break;
	}
}

void QuadTree::setRegularMaterial(enum class QuadType type, bool reload)
{
	switch (type)
	{
	case QuadType::Terrain:
	{
		if (!containsQuadOfType(type))
			return;

		materialParamsNeedReset(true);

		getQuadrant()->setHeightsUpdated(true);
		getQuadrant()->setNormalsUpdated(true);
		getQuadrant()->setSplatmapUpdated(true);
		getQuadrant()->setColorsUpdated(true);
		getQuadrant()->setGlobalmapUpdated(true);

		getQuadrant()->northQuadrantData().shaderUpdated(false);
		getQuadrant()->southQuadrantData().shaderUpdated(false);
		getQuadrant()->westQuadrantData().shaderUpdated(false);
		getQuadrant()->eastQuadrantData().shaderUpdated(false);
		getQuadrant()->northWestQuadrantData().shaderUpdated(false);
		getQuadrant()->southEastQuadrantData().shaderUpdated(false);

		Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getRegularTerrainMaterial(reload));
		ForAllChunk(action, type);

		getQuadrant()->getShaderTerrainData()->releaseLookDevMaterial();
		getQuadrant()->getShaderTerrainData()->releaseLookDevChunkLodMaterials();

		m_lookDev = ShaderTerrainData::LookDev::NotSet;
	}
	break;
	case QuadType::Sea:
	{
		if (containsQuadOfType(type))
		{
			materialParamsNeedUpdate(true);
			Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getRegularSeaMaterial(reload));
			ForAllChunk(action, type);
			getQuadrant()->getShaderTerrainData()->releaseLookDevMaterial();
			getQuadrant()->getShaderTerrainData()->releaseLookDevChunkLodMaterials();
			m_lookDev = ShaderTerrainData::LookDev::NotSet;

			getQuadrant()->getShaderTerrainData()->getGlobalmapSeaMaterial(reload);
		}
	}
	break;
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		setRegularMaterial(QuadType::Terrain, reload);
		setRegularMaterial(QuadType::Sea, reload);
		m_lookDev = ShaderTerrainData::LookDev::NotSet;
	}
	break;
	}
}

Ref<Material> QuadTree::getCurrentMaterial(enum class QuadType type, int lod)
{
	switch (type)
	{
	case QuadType::Terrain:
	{
		if (m_lookDev == ShaderTerrainData::LookDev::NotSet)
			return getQuadrant()->getShaderTerrainData()->getRegularTerrainMaterial();
		else
			if (m_lookDev == ShaderTerrainData::LookDev::ChunkLod)
			{
				return getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterial(lod);
			}
			else
				return getQuadrant()->getShaderTerrainData()->getLookDevMaterial(m_lookDev);
	}
	break;
	case QuadType::Sea:
	{
		return getQuadrant()->getShaderTerrainData()->getRegularSeaMaterial();
	}
	break;
	default:
		return nullptr;
	}
}

void QuadTree::reloadCurrentMaterial(enum class QuadType type)
{
	bool reload = true;

	if (m_lookDev == ShaderTerrainData::LookDev::NotSet)
	{
		setRegularMaterial(type, reload);
	}
	else
	{
		setLookDevMaterial(m_lookDev, type, reload);
	}
}

void QuadTree::setQuadrantOptimizations(godot::Vector3& worldCameraWorldPos)
{
	godot::Vector3 quadrantCenter  = getQuadrant()->getCenter();
	quadrantCenter.y = (getQuadrant()->getTerrainEdit()->maxHeight + getQuadrant()->getTerrainEdit()->minHeight) / 2;
	m_lastComputedQuadrantDistanceFromCamera = (float)abs((worldCameraWorldPos - quadrantCenter).length());

	{
		if (m_viewer->Globals()->getParamThresholdForGlobalmap() > 0 && m_lastComputedQuadrantDistanceFromCamera > m_viewer->Globals()->getParamThresholdForGlobalmap())
		{
			if (!getQuadrant()->renderGlobalMap())
				getQuadrant()->renderGlobalMap(true);
		}
		else
		{
			if (getQuadrant()->renderGlobalMap())
				getQuadrant()->renderGlobalMap(false);
		}
	}

	enum class QuadType quadrantTypeForOptimizations = QuadType::Terrain;

	if (!containsQuadOfType(quadrantTypeForOptimizations))
		return;

	{
		size_t newOptimizationLevel = 0;	// 0 ==> no optimization
		//if (getQuadrant()->getMeshResolutionLevel() == Quadrant::MeshResolutionLevel::low)
		if (getQuadrant()->quadrantAtMinLod(quadrantTypeForOptimizations))	// optimization > 0 only for quadrant with a unique chunk coincident with the root: for those quadrants we used cached resized texture for the whole quad and we don't split it in chunks
		{
			newOptimizationLevel += m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement();
			float thresholdForTextureOptimization = m_viewer->Globals()->getParamThresholdForMapTextureOptimization();
			if (m_forceRecalcOptimizationLevel || (m_lastComputedQuadrantDistanceFromCamera > thresholdForTextureOptimization && m_viewer->Globals()->heightmapResolutionShift() > MIN_RESOLUTION_SHIFT_FOR_TEXTURE_OPTIMIZATIONS))
			{
				//newOptimizationLevel += m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement();
				size_t maxOptimizationLevel = m_viewer->Globals()->getMaxTextureOptimizationLevel();
				float increment = m_viewer->Globals()->heightmapSizeInWUs() * m_viewer->Globals()->getParamTextureOptimizationIncrementFactor();
				thresholdForTextureOptimization += increment;
				while (thresholdForTextureOptimization < m_lastComputedQuadrantDistanceFromCamera && newOptimizationLevel < maxOptimizationLevel)
				{
					thresholdForTextureOptimization += increment;
					newOptimizationLevel += m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement();
				}
			}
			m_forceRecalcOptimizationLevel = false;
		}
		//optimizationLevel = 0;	// DEBUG
		getQuadrant()->optimizationLevel(newOptimizationLevel);
	}

	{
		//if (getQuadrant()->getMeshResolutionLevel() == Quadrant::MeshResolutionLevel::low 
		if (getQuadrant()->quadrantAtMinLod(quadrantTypeForOptimizations)
			&&
			((m_viewer->Globals()->getParamThresholdCacheHeights() > 0 && m_lastComputedQuadrantDistanceFromCamera > m_viewer->Globals()->getParamThresholdCacheHeights()) /* || !m_viewer->Globals()->getParamCacheHeightsForLowResQuadrants()*/))
		{
			getQuadrant()->canCacheHeigthBuffers(false);
		}
		else
		{
			bool changed = getQuadrant()->canCacheHeigthBuffers(true);
			if (changed && getQuadrant()->needReloadTerrainDataFromCache())
				setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);
		}
	}
}

bool QuadTree::resetMaterialParams(bool& lastProcessExecutionInterrupted, godot::Vector3& worldCameraWorldPos, bool force)
{
	bool reset = false;

	if (force)
	{
		materialParamsNeedReset(true);
		//if (!getQuadrant()->internalDataLocked())
		//{
		//	getQuadrant()->getShaderTerrainData()->resetMaterialParams(m_lookDev);
		//	materialParamsNeedReset(false);
		//	reset = true;
		//}
	}
	
	if (materialParamsNeedReset() /* && !getQuadrant()->internalDataLocked()*/)
	{
		//TimerMs clock;	clock.tick();
		if (status() != QuadrantStatus::initialized)
		{
		}
		else
		{
			setQuadrantOptimizations(worldCameraWorldPos);
			getQuadrant()->getShaderTerrainData()->resetMaterialParams(QuadType::NotSet, lastProcessExecutionInterrupted, m_lookDev);
			if (!lastProcessExecutionInterrupted)
			{
				getQuadrant()->terrainDataJustLoaded(false);
				materialParamsNeedReset(false);
				reset = true;
				//clock.tock();	size_t elapsed = clock.duration().count();	PLOG_DEBUG << "resetMaterialParams duration = " << elapsed;
			}
		}
	}
	else
		getQuadrant()->terrainDataJustLoaded(false);

	return reset;
}

bool QuadTree::materialParamsNeedReset(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedReset();
}

void QuadTree::materialParamsNeedReset(bool b)
{
	if (b)
	{
		getQuadrant()->getShaderTerrainData()->materialParamsNeedReset(b);

		//getQuadrant()->northQuadrantData().initialized(false);
		//getQuadrant()->southQuadrantData().initialized(false);
		//getQuadrant()->westQuadrantData().initialized(false);
		//getQuadrant()->eastQuadrantData().initialized(false);
	}
	else
		getQuadrant()->getShaderTerrainData()->materialParamsNeedReset(b);
}

bool QuadTree::updateCollider(enum class QuadType type)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5 ") + __FUNCTION__, "ALL - Quadrant type=" + QuadTree::quadTypeToString(type));

	bool updated = false;

	switch (type)
	{
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		if (updateCollider(QuadType::Terrain))
			updated = true;
		if (updateCollider(QuadType::Sea))
			updated = true;
	}
	break;
	case QuadType::Terrain:
	{
		if (containsQuadOfType(type))
		{
			if (isValid() /*&& !getQuadrant()->internalDataLocked()*/ && !getQuadrant()->internalDataLockedUntilCompleteProcessExecution())
			{
				Collider* collider = m_worldQuadrant->getCollider();
				if (collider != nullptr)
				{
					if (colliderNeedReset(type))
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.0 ") + __FUNCTION__, "Quadrant: colliderNeedReset");
						collider->exitWorld();
						collider->deinit();
						colliderNeedReset(false, type);
					}

					bool colliderCanHaveHiResCollisionInfo = getQuadrant()->canHaveHiResCollisionInfo();

					if (!collider->hasData() || (colliderCanHaveHiResCollisionInfo && !collider->hasHiResData()) || (!colliderCanHaveHiResCollisionInfo && collider->hasHiResData()))
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.1 ") + __FUNCTION__, "Quadrant: set/reset collider");

						std::string s = colliderCanHaveHiResCollisionInfo ? "hires" : "lowres";

						if (statusInitialized() && getQuadrant()->isFlushed())
						{
							TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.2 ") + __FUNCTION__, std::string("Quadrant: missing heigths for") + s + " collider requesting ...");
							setStatus(QuadrantStatus::loadTerrainDataFromCacheNeeded);
							colliderNeedReset(true, type);
						}
						else if (statusInitialized())
						{
							TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.3 ") + __FUNCTION__, std::string("Quadrant: updating ") + s + " collider");

							TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

							if (collider->worldEntered())
								collider->exitWorld();
							if (collider->initialized())
								collider->deinit();
							collider->init(m_GDN_Quadrant, THEWORLD_TERRAIN_COLLISION_LAYER, 4294967295);
							collider->enterWorld();
							{
								TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.3.1 ") + __FUNCTION__, "Quadrant: set data for hires collider");
								updated = collider->setData(colliderCanHaveHiResCollisionInfo);
							}

							TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
							if (colliderCanHaveHiResCollisionInfo)
								m_viewer->incNumSetHiResCollisionData((now - start).count());
							else
								m_viewer->incNumSetLowResCollisionData((now - start).count());
						}
						else
						{
							TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5.4 ") + __FUNCTION__, std::string("Quadrant: missing heigths for ") + s + " collider will retry ...");
							colliderNeedReset(true, type);
						}
					}
				}
			}
		}
	}
	break;
	case QuadType::Sea:
	{
		if (containsQuadOfType(type))
		{
			if (colliderNeedReset(type))
			{
				colliderNeedReset(false, type);
			}
		}
	}
	break;
	}

	return updated;
}

bool QuadTree::colliderNeedReset(enum class QuadType type)
{
	switch (type)
	{
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		for (int i = 0; i < (int)QuadType::NotUsed; i++)
		{
			if (containsQuadOfType((enum class QuadType)i) && m_quads[(enum class QuadType)i]->m_colliderNeedReset)
				return true;
		}
		return false;
	}
	break;
	default:
	{
		if (!containsQuadOfType(type))
			return false;

		return m_quads[type]->m_colliderNeedReset;
	}
	break;
	}
}

void QuadTree::colliderNeedReset(bool b, enum class QuadType type)
{
	switch (type)
	{
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		for (int i = 0; i < (int)QuadType::NotUsed; i++)
		{
			if (containsQuadOfType((enum class QuadType)i))
			{
				m_quads[(enum class QuadType)i]->m_colliderNeedReset = b;
				if (b)
					m_viewer->allCollidersReady(false);
			}
		}
	}
	break;
	default:
	{
		if (!containsQuadOfType(type))
			return;

		m_quads[type]->m_colliderNeedReset = b;
		if (b)
			m_viewer->allCollidersReady(false);
	}
	break;
	}
}

bool QuadTree::updateMaterialParams(bool updateTerrainData)
{
	bool updated = false;

	if (!isValid())
		return updated;

	//if (getQuadrant()->internalDataLocked())
	//	return updated;

	//if ((int)m_viewer->Globals()->status() < (int)TheWorldStatus::worldDeployed)
	//	return updated;

	if (getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4 ") + __FUNCTION__, "Quadrant updateMaterialParams");

		bool freeMaps = true;
		getQuadrant()->getShaderTerrainData()->updateMaterialParams(QuadType::NotSet, m_lookDev, freeMaps, updateTerrainData);

		if (!getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdateTransformChanged())
			updated = true;

		getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(false);

	}

	return updated;
}

bool QuadTree::materialParamsNeedUpdate(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate();
}

void QuadTree::materialParamsNeedUpdate(bool b, bool transformChanged)
{
	getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(b, transformChanged);
}

Transform3D QuadTree::getInternalGlobalTransform(void)
{
	Transform3D t(Basis(), Vector3((float)getQuadrant()->getPos().getLowerXGridVertex(), 0, (float)getQuadrant()->getPos().getLowerZGridVertex()));
	Transform3D gt = m_viewer->getInternalGlobalTransform();
	Transform3D t1 = gt * t;
	return t1;
}

void QuadTree::dump(bool minimal)
{
	return;
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	if (!minimal)
		globals->debugPrint("====================================================================================================================================");

	bool needSave = false;
	bool manualSave = false;
	Quadrant* quadrant = getQuadrant();
	TheWorld_Utils::QuadrantPos pos = quadrant->getPos();
	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl != nullptr)
		if (editModeUIControl->isQuadToSave(pos, manualSave))
			needSave = true;

	std::string line;
	line = "Quadrant " + quadrant->getPos().getIdStr() + " " + quadrant->getPos().getName().c_str() + " " + m_tag.c_str() + " TerrainChunks=" + std::to_string(getNumChunks(false, QuadType::Terrain))
		+ " WithSea=" + std::to_string(containsQuadOfType(QuadType::Sea)) + " SeaChunks=" + std::to_string(getNumChunks(false, QuadType::Sea))
		+ " Status=" + statusToString(status())
		+ " Valid=" + std::to_string(isValid())
		+ " Visible=" + std::to_string(isVisible())
		+ " OptLevel=" + std::to_string(quadrant->optimizationLevel())
		+ " NeedSave=" + std::to_string(needSave)
		+ " ManualSave=" + std::to_string(manualSave)
		+ " NeedUpload=" + std::to_string(quadrant->needUploadToServer())
		+ " CollEnabled=" + std::to_string(quadrant->getCollider() == nullptr ? false : quadrant->getCollider()->colliderEnabled())
		+ " HasHeights16=" + std::to_string(!quadrant->getFloat16HeightsBuffer(false).empty())
		+ " HasHeights32=" + std::to_string(!quadrant->getFloat32HeightsBuffer(false).empty())
		+ " HasNormalmap=" + std::to_string(!quadrant->getNormalsBuffer(false).empty())
		+ " HasSplatmap=" + std::to_string(!quadrant->getSplatmapBuffer(false).empty())
		+ " HasGlobalmap=" + std::to_string(!quadrant->getGlobalmapBuffer(false).empty())
		+ " HasColormap=" + std::to_string(!quadrant->getColormapBuffer(false).empty())
		;
	globals->debugPrint(line.c_str());

	if (minimal)
		return;

	globals->debugPrint(String("DUMP QUADRANT ") + getQuadrant()->getPos().getIdStr().c_str() + " TAG=" + m_tag.c_str());

	if (isValid() && isVisible())
	{
		globals->debugPrint(String("Num vertices per chunk side: ") + String(std::to_string(globals->numVerticesPerChuckSide()).c_str())
			+ String(" - ") + String("Num vertices per grid size: ") + String(std::to_string(globals->heightmapResolution()).c_str())
			+ String(" - ") + String("Grid size [WUs]: ") + String(std::to_string(globals->heightmapSizeInWUs()).c_str())
			+ String(" - ") + String("Lod max depth: ") + String(std::to_string(globals->lodMaxDepth()).c_str()));

		globals->debugPrint(String("FROM LAST DUMP - Num quad split: ") + String(std::to_string(m_numSplits).c_str())
			+ String(" - ") + String("Num quad join: ") + String(std::to_string(m_numJoins).c_str())
			+ String(" - ") + String("Num leaves: ") + String(std::to_string(m_numLeaf).c_str()));

		globals->debugPrint(String("GRID POS (global)")
			+ String(" - ") + String("X = ") + String(std::to_string(getInternalGlobalTransform().origin.x).c_str())
			+ String(" - ") + String("Z = ") + String(std::to_string(getInternalGlobalTransform().origin.z).c_str()));

		for (int i = 0; i < (int)QuadType::NotUsed; i++)
		{
			QuadType type = (QuadType)i;
			if (containsQuadOfType(type))
			{
				globals->debugPrint(String("QuadType: ") + QuadTree::quadTypeToString(type).c_str());

				int numChunks = 0;
				int numActiveChunks = 0;
				Chunk* cameraChunk = nullptr;
				for (int i = globals->lodMaxDepth(); i >= 0; i--)
				{
					int numChunksAtLod = 0;
					Chunk::MapChunk::iterator it = m_quads[type]->m_chunks.find(i);
					if (it != m_quads[type]->m_chunks.end())
						numChunksAtLod = int(m_quads[type]->m_chunks[i].size());
					int numChunksActiveAtLod = 0;
					for (Chunk::MapChunkPerLod::iterator it1 = m_quads[type]->m_chunks[i].begin(); it1 != m_quads[type]->m_chunks[i].end(); it1++)
					{
						if (it1->second->isActive())
							numChunksActiveAtLod++;
						if (it1->second->isCameraVerticalOnChunk())
							cameraChunk = it1->second;
					}
					globals->debugPrint("Num chunks at lod " + String(std::to_string(i).c_str()) + ": " + String(std::to_string(numChunksAtLod).c_str())
						+ String("\t") + " Active " + String(std::to_string(numChunksActiveAtLod).c_str()));
					numChunks += numChunksAtLod;
					numActiveChunks += numChunksActiveAtLod;
				}

				globals->debugPrint("Num chunks: " + String(std::to_string(numChunks).c_str()));
				globals->debugPrint("Num active chunks: " + String(std::to_string(numActiveChunks).c_str()));

				// DEBUGRIC
				//if (cameraChunk != nullptr)
				//{
				//	Ref<Mesh> cameraMesh = cameraChunk->getMesh();
				//	Array cameraArrays = cameraMesh->surface_get_arrays(0);
				//	PoolVector3Array cameraVerts = cameraArrays[Mesh::ARRAY_VERTEX];
				//	globals->debugPrint("camerMesh Verts: " + String(std::to_string(cameraVerts.size()).c_str()));
				//	Transform cameraT = cameraChunk->getMeshGlobalTransformApplied();
				//	globals->debugPrint("camerMesh t= " + String(cameraT));
				//	int numPointsPerSize = (int)sqrt(cameraVerts.size());
				//	String s;	int i = 0;
				//	for (int z = 0; z < numPointsPerSize; z++)
				//	{
				//		for (int x = 0; x < numPointsPerSize; x++)
				//		{
				//			s += cameraVerts[i];
				//			s += " ";
				//			i++;
				//		}
				//		globals->debugPrint("camerMesh r=" + String(std::to_string(z + 1).c_str()) + " " + s);
				//		s = "";
				//	}

				//	Chunk* cameraChunkXMinus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XMinusChunk);
				//	if (cameraChunkXMinus != nullptr)
				//	{
				//		Ref<Mesh> camerMeshXMinus = cameraChunkXMinus->getMesh();
				//		Array cameraArraysXMinus = camerMeshXMinus->surface_get_arrays(0);
				//		PoolVector3Array cameraVertsXMinus = cameraArraysXMinus[Mesh::ARRAY_VERTEX];
				//		globals->debugPrint("camerMeshXMinus Verts: " + String(std::to_string(cameraVertsXMinus.size()).c_str()));
				//		Transform cameraT = cameraChunkXMinus->getMeshGlobalTransformApplied();
				//		globals->debugPrint("camerMeshXMinus t= " + String(cameraT));
				//		int numPointsPerSize = (int)sqrt(cameraVertsXMinus.size());
				//		String s;	int i = 0;
				//		for (int z = 0; z < numPointsPerSize; z++)
				//		{
				//			for (int x = 0; x < numPointsPerSize; x++)
				//			{
				//				s += cameraVertsXMinus[i];
				//				s += " ";
				//				i++;
				//			}
				//			globals->debugPrint("camerMeshXMinus r=" + String(std::to_string(z + 1).c_str()) + " " + s);
				//			s = "";
				//		}
				//	}
				//	Chunk* cameraChunkXPlus = getChunkAt(cameraChunk->getPos(), Chunk::DirectionSlot::XPlusChunk);
				//	if (cameraChunkXPlus != nullptr)
				//	{
				//		Ref<Mesh> camerMeshXPlus = cameraChunkXPlus->getMesh();
				//		Array cameraArraysXPlus = camerMeshXPlus->surface_get_arrays(0);
				//		PoolVector3Array cameraVertsXPlus = cameraArraysXPlus[Mesh::ARRAY_VERTEX];
				//		globals->debugPrint("camerMeshXPlus Verts: " + String(std::to_string(cameraVertsXPlus.size()).c_str()));
				//		Transform cameraT = cameraChunkXPlus->getMeshGlobalTransformApplied();
				//		globals->debugPrint("camerMeshXPlus t= " + String(cameraT));
				//		int numPointsPerSize = (int)sqrt(cameraVertsXPlus.size());
				//		String s;	int i = 0;
				//		for (int z = 0; z < numPointsPerSize; z++)
				//		{
				//			for (int x = 0; x < numPointsPerSize; x++)
				//			{
				//				s += cameraVertsXPlus[i];
				//				s += " ";
				//				i++;
				//			}
				//			globals->debugPrint("camerMeshXPlus r=" + String(std::to_string(z + 1).c_str()) + " " + s);
				//			s = "";
				//		}
				//	}
				//}
				// DEBUGRIC

				Chunk::DumpChunkAction action;
				ForAllChunk(action, type);
			}
		}

	}
	else
	{
		if (!isValid())
			globals->debugPrint("QUADRANT NOT VALID YET");
		else
			globals->debugPrint("QUADRANT ACTUALLY NOT VISIBLE");
	}


	globals->debugPrint(String("COMPLETED DUMP QUADRANT ") + getQuadrant()->getPos().getIdStr().c_str() + " TAG=" + m_tag.c_str());

	globals->debugPrint("====================================================================================================================================");

	m_numSplits = 0;
	m_numJoins = 0;
}

ShaderTerrainData::ShaderTerrainData(GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_viewer = viewer;
	m_quadTree = quadTree;
	m_materialParamsNeedUpdate = false;
	m_materialParamsNeedReset = false;
	m_heightMapTexModified = false;
	m_normalMapTexModified = false;
	m_splatMapTexModified = false;
	m_colorMapTexModified = false;
	m_globalMapTexModified = false;
}

ShaderTerrainData::~ShaderTerrainData()
{
	//// useless, but just in case ...
	//m_regularMaterial.unref();
	//m_lookDevMaterial.unref();

	//m_heightmapImage.unref();
	//for (auto& item : m_resizedHeightmapImageMap)
	//	item.second.unref();
	//m_heightmapTexture.unref();

	//m_normalmapImage.unref();
	//for (auto& item : m_resizedNormalmapImageMap)
	//	item.second.unref();
	//m_normalmapTexture.unref();

	//m_splatmapImage.unref();
	//for (auto& item : m_resizedSplatmapImageMap)
	//	item.second.unref();
	//m_splatmapTexture.unref();

	//m_globalmapImage.unref();
	//for (auto& item : m_resizedGlobalmapImageMap)
	//	item.second.unref();
	//m_globalmapTexture.unref();

	//m_colormapImage.unref();
	//for (auto& item : m_resizedColormapImageMap)
	//	item.second.unref();
	//m_colormapTexture.unref();
}

void ShaderTerrainData::init(void)
{
}

godot::Ref<godot::Material> ShaderTerrainData::getRegularSeaMaterial(bool reload)
{
	if (m_regularSeaMaterial == nullptr || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
		String shaderPath = "res://addons/twviewer/shaders/water.gdshader";
		//String shaderPath = "res://addons/twviewer/test/water.gdshader";
		godot::Ref<godot::Shader> shader;
		if (reload)
			shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		else
			shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		//mat->set_render_priority(0);
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, m_viewer->Globals()->gridStepInWU());
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
		m_regularSeaMaterial = mat;
	}

	return m_regularSeaMaterial;
}

godot::Ref<godot::Material> ShaderTerrainData::getGlobalmapSeaMaterial(bool reload)
{
	if (m_globalmapSeaMaterial == nullptr || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
		//String shaderPath = "res://addons/twviewer/shaders/water_globalmap.gdshader";
		String shaderPath = "res://addons/twviewer/test/water_globalmap.gdshader";
		godot::Ref<godot::Shader> shader;
		if (reload)
			shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		else
			shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		//mat->set_render_priority(0);
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, m_viewer->Globals()->gridStepInWU());
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
		m_globalmapSeaMaterial = mat;
	}

	return m_globalmapSeaMaterial;
}

godot::Ref<godot::Material> ShaderTerrainData::getRegularTerrainMaterial(bool reload)
{
	freeLookDevSubViewport();

	if (m_regularTerrainMaterial == nullptr || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
									//String shaderPath = "res://addons/twviewer/test/test_quadrant.tres";
									//String shaderPath = "res://addons/twviewer/shaders/regularChunkOld.gdshader";
									//String shaderPath = "res://addons/twviewer/shaders/regularChunk.gdshader";
									//String shaderPath = "res://addons/twviewer/shaders/regularChunk.tres";
									//String shaderPath = "res://addons/twviewer/test/regularChunk.tres";
		//String shaderPath = "res://addons/twviewer/shaders/terrain.gdshader";
		String shaderPath = "res://addons/twviewer/test/terrain.gdshader";
		//String shaderPath = "res://addons/twviewer/test/terrain_test.gdshader";
		//m_sendTextureToShader = true;		// ATTENZIONE: commentato perché sembra inutile mandare le singole texture allo shader quando gli mandiamo gli indici delle texture nell'array globale
		godot::Ref<godot::Shader> shader;
		if (reload)
			shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		else
			shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		//mat->set_render_priority(0);
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, m_viewer->Globals()->gridStepInWU());
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
		mat->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, m_viewer->getForceShaderElevation0());
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING, false);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_NORTH, false);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_SOUTH, false);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_EAST, false);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_WEST, false);
		mat->set_shader_parameter(SHADER_PARAM_WIRE_ALBEDO, GDN_TheWorld_Globals::g_color_green1);
		mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_ALBEDO, GDN_TheWorld_Globals::g_color_light_cyan);
		mat->set_shader_parameter(SHADER_PARAM_SELECTED_ALBEDO, GDN_TheWorld_Globals::g_color_cyan);

		////     Test PBR	// SUPERDEBUGRIC 
		//{
		//	mat->set_shader_parameter("texture_albedo", resLoader->load("res://assets/textures/ground/Ground037_1K/Ground037_1K-PNG_Color.png"));
		//	mat->set_shader_parameter("texture_roughness", resLoader->load("res://assets/textures/ground/Ground037_1K/Ground037_1K-PNG_Roughness.png"));
		//	mat->set_shader_parameter("texture_normal", resLoader->load("res://assets/textures/ground/Ground037_1K/Ground037_1K-PNG_NormalGL.png"));
		//	mat->set_shader_parameter("texture_ambient_occlusion", resLoader->load("res://assets/textures/ground/Ground037_1K/Ground037_1K-PNG_AmbientOcclusion.png"));
		//	mat->set_shader_parameter("texture_displacement", resLoader->load("res://assets/textures/ground/Ground037_1K/Ground037_1K-PNG_Displacement.png"));
		//}
		////     Test PBR	// SUPERDEBUGRIC

		m_regularTerrainMaterial = mat;
	}
	
	return m_regularTerrainMaterial;
}

void ShaderTerrainData::releaseRegularTerrainMaterial(void)
{
	m_regularTerrainMaterial.unref();
}

void ShaderTerrainData::freeLookDevSubViewport(void)
{
	if (g_lookDevShaderTexture != nullptr)
	{
		g_lookDevShaderTexture->deinit();
		delete g_lookDevShaderTexture;
		g_lookDevShaderTexture = nullptr;
	}
}

godot::Ref<godot::Material> ShaderTerrainData::getLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload)
{
	if (lookDev == ShaderTerrainData::LookDev::Universe 
		|| lookDev == ShaderTerrainData::LookDev::ShaderArt 
		|| lookDev == ShaderTerrainData::LookDev::Lightning
		|| lookDev == ShaderTerrainData::LookDev::StarNest
		|| lookDev == ShaderTerrainData::LookDev::FlaringStar
		|| lookDev == ShaderTerrainData::LookDev::AnimatedDiamond
		|| lookDev == ShaderTerrainData::LookDev::PlasmaWaves)
	{
		if (g_lookDevShaderTexture == nullptr || reload)
		{
			freeLookDevSubViewport();

			g_lookDevShaderTexture = new GDN_TheWorld_ShaderTexture;
			std::string shaderPath;
			if (lookDev == ShaderTerrainData::LookDev::Universe)
				shaderPath = "res://addons/twviewer/shaders/Universe.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::ShaderArt)
				shaderPath = "res://addons/twviewer/shaders/ShaderArt.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::Lightning)
				shaderPath = "res://addons/twviewer/shaders/Lightning.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::StarNest)
				shaderPath = "res://addons/twviewer/shaders/StarNest.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::FlaringStar)
				shaderPath = "res://addons/twviewer/shaders/FlaringStar.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::AnimatedDiamond)
				shaderPath = "res://addons/twviewer/shaders/AnimatedDiamond.gdshader";
			else if (lookDev == ShaderTerrainData::LookDev::PlasmaWaves)
				shaderPath = "res://addons/twviewer/shaders/PlasmaWaves.gdshader";
			
			g_lookDevShaderTexture->init(m_viewer, godot::Vector2i(1024, 1024), shaderPath);
			
			if (lookDev == ShaderTerrainData::LookDev::Lightning)
			{
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("lightning_color", godot::Color(0.0f, 1.0f, 1.0f, 1.0f));
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("size", 0.01f);			// how thick bolt line should be 0/1
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("width", 0.1f);			// how long the range of the bolt should be 0/1
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("speed", 10.0f);		// how fast the lightning should change
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("cycle", 2.0f);			// how many cycles the zigzag should be
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("ratio", 10.0f);
				g_lookDevShaderTexture->getMaterial()->set_shader_parameter("time_shift", 1.0f);	// because we use the sine function as a random, the same `TIME` will produce the same bolt, so if we create different bolts, we need to shift the time a little bit
			}
		}
	}
	else
	{
		freeLookDevSubViewport();
	}

	if (m_lookDevMaterial == nullptr || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		Ref<ShaderMaterial> mat = memnew(ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		ResourceLoader* resLoader = ResourceLoader::get_singleton();
		String shaderPath = "res://addons/twviewer/shaders/lookdevChunk.gdshader";
		//Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		Ref<Shader> shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
		mat->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, m_viewer->getForceShaderElevation0());
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
		m_lookDevMaterial = mat;
	}

	return m_lookDevMaterial;
}

void ShaderTerrainData::releaseLookDevMaterial(void)
{
	m_lookDevMaterial.unref();
}

std::map<int, godot::Ref<godot::ShaderMaterial>>& ShaderTerrainData::getLookDevChunkLodMaterials(void)
{
	return m_lookDevChunkLodMaterials;
}

void ShaderTerrainData::releaseLookDevChunkLodMaterials(void)
{
	for (auto& item : m_lookDevChunkLodMaterials)
	{
		item.second.unref();
	}
	m_lookDevChunkLodMaterials.clear();
}

godot::Ref<godot::Material> ShaderTerrainData::getLookDevChunkLodMaterial(int lod, bool reload)
{
	if (!m_lookDevChunkLodMaterials.contains(lod) || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		Ref<ShaderMaterial> mat = memnew(ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		ResourceLoader* resLoader = ResourceLoader::get_singleton();
		String shaderPath = "res://addons/twviewer/shaders/lookdevChunkLod.gdshader";
		//Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		Ref<Shader> shader = resLoader->load(shaderPath);
		mat->set_shader(shader);
		float numVerticesPerChuckSide = (float)m_viewer->Globals()->numVerticesPerChuckSide();
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		float lodGridStepInWU = m_viewer->Globals()->gridStepInHeightmapWUs(lod);
		float lodChunkSizeInWu = numVerticesPerChuckSide * lodGridStepInWU;
		mat->set_shader_parameter(SHADER_PARAM_LOOKDEV_LOD, float(lod));
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_QUADRANT_SIZE, m_viewer->Globals()->heightmapSizeInWUs());
		mat->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, m_viewer->getForceShaderElevation0());
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, numVerticesPerChuckSide + 1.0f);
		mat->set_shader_parameter(SHADER_PARAM_LOD_GRID_STEP, lodGridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_LOD_CHUNK_SIZE, lodChunkSizeInWu);
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
		m_lookDevChunkLodMaterials[lod] = mat;
	}

	return m_lookDevChunkLodMaterials[lod];
}

ShaderTerrainData::GroundTextures ShaderTerrainData::s_groundTextures;
std::recursive_mutex ShaderTerrainData::s_mtxStaticData;
std::map<int32_t, std::unique_ptr<ShaderTerrainData::Continent>> ShaderTerrainData::s_continents;

std::string ShaderTerrainData::layoutFromQuadrantPos(TheWorld_Utils::QuadrantPos pos)
{
	std::string continentName;
	return ShaderTerrainData::layoutFromQuadrantPos(pos, continentName);
}

std::string ShaderTerrainData::layoutFromQuadrantPos(TheWorld_Utils::QuadrantPos pos, std::string& continentName)
{
	std::lock_guard<std::recursive_mutex> lock(s_mtxStaticData);

	std::string ret = Quadrant::c_defaultTextureLayout;
	continentName = "";
	
	if (pos.empty())
		return ret;

	int quadrantPosX = pos.getIndexPosX();
	int quadrantPosZ = pos.getIndexPosZ();

	for (auto& continent : s_continents)
	{
		for (int idx = 0; ; idx++)
		{
			Continent::Margin* margin = continent.second->getMargin(idx);
			if (margin == nullptr)
				break;

			Continent::Margin::Delimiter* delimiter = margin->getDelimiter(Continent::Margin::Delimiter::DelimiterType::NW);
			if (delimiter == nullptr)
				break;
			Continent::Margin::Delimiter NW_delimiter = *delimiter;
			delimiter = margin->getDelimiter(Continent::Margin::Delimiter::DelimiterType::SE);
			if (delimiter == nullptr)
				break;
			Continent::Margin::Delimiter SE_delimiter = *delimiter;

			if (quadrantPosX >= NW_delimiter.quadrantPosX() and quadrantPosX <= SE_delimiter.quadrantPosX()
				and quadrantPosZ >= NW_delimiter.quadrantPosZ() and quadrantPosZ <= SE_delimiter.quadrantPosZ())
			{
				continentName = continent.second->name();
				return continent.second->layout();
			}
		}
	}

	return ret;
}

//std::string ShaderTerrainData::layoutFromContinentId(int32_t id)
//{
//	if (s_continents.contains(id))
//		return s_continents[id]->layout();
//	else
//		return Quadrant::c_defaultTextureLayout;
//}


//godot::Ref<godot::Image> ShaderTerrainData::readGroundTexture(godot::String fileName, bool& ok, GDN_TheWorld_Viewer* viewer)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1 ") + __FUNCTION__, "ALL");
//
//	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//
//	godot::Error e;
//
//	ok = true;
//	
//	const char* s = fileName.utf8().get_data();
//	std::string _fileName = std::string(s);
//
//	const godot::String webpFilename = fileName + ".cache_image";
//	const char* s1 = webpFilename.utf8().get_data();
//	std::string _webpFilename = std::string(s1);
//
//	const godot::String pngFilename = fileName + ".png";
//	const char* s2 = pngFilename.utf8().get_data();
//	std::string _pngFilename = std::string(s2);
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint((std::string("Reading ground texture ") + _fileName + " ...").c_str());
//
//	if (viewer != nullptr)
//		viewer->getMainProcessingMutex().lock();
//	godot::Ref<godot::FileAccess> file = godot::FileAccess::open(fileName, godot::FileAccess::READ);
//	if (file == nullptr)
//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error reading file ") + _fileName + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
//	if (viewer != nullptr)
//		viewer->getMainProcessingMutex().unlock();
//
//	int64_t fileSize = file->get_length();
//	size_t imageSize = (size_t)sqrt(fileSize / sizeof(struct TheWorld_Utils::_RGBA));
//	assert(imageSize * imageSize * sizeof(struct TheWorld_Utils::_RGBA) == fileSize);
//	if (imageSize * imageSize * sizeof(struct TheWorld_Utils::_RGBA) != fileSize)
//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Wrong image size ") + std::to_string(imageSize) + " file size " + std::to_string(fileSize)).c_str()));
//
//	if (viewer != nullptr)
//		viewer->getMainProcessingMutex().lock();
//	godot::Ref<godot::Image> image = godot::Image::create((int32_t)imageSize, (int32_t)imageSize, true, godot::Image::Format::FORMAT_RGBA8);
//	if (viewer != nullptr)
//		viewer->getMainProcessingMutex().unlock();
//
//	if (godot::FileAccess::file_exists(webpFilename))
//	{
//		file->close();
//
//		//ResourceLoader* resLoader = ResourceLoader::get_singleton();
//		//godot::Ref<godot::Texture2D> tex = resLoader->load(webpFilename);
//		//image = tex->get_image();
//		{
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().lock();
//			godot::Ref<godot::FileAccess> file = godot::FileAccess::open(webpFilename, godot::FileAccess::READ);
//			if (file == nullptr)
//				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error reading file ") + _webpFilename + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().unlock();
//			uint64_t size = file->get_64();
//			godot::PackedByteArray array = file->get_buffer(size);
//			file->close();
//
//			e = image->load_webp_from_buffer(array);
//			if (e != godot::Error::OK)
//				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("reading image error ") + std::to_string(int(e))).c_str()));
//		}
//	}
//	else
//	{
//		godot::PackedByteArray imageRowBuffer;
//		size_t imageRowSize = imageSize * sizeof(struct TheWorld_Utils::_RGBA);
//
//		for (size_t y = 0; y < imageSize; y++)
//		{
//			{
//				//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.1 ") + __FUNCTION__, "read file from disk");
//				imageRowBuffer = file->get_buffer(imageRowSize);
//			}
//			assert(imageRowBuffer.size() == imageRowSize);
//
//			{
//				//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.2 ") + __FUNCTION__, "set_pixel");
//
//				size_t idx = 0;
//				for (size_t x = 0; x < imageSize; x++)
//				{
//					godot::Color c;
//					c.r = float(imageRowBuffer[int(idx)]) / 255;
//					c.g = float(imageRowBuffer[int(idx) + 1]) / 255;
//					c.b = float(imageRowBuffer[int(idx) + 2]) / 255;
//					c.a = float(imageRowBuffer[int(idx) + 3]) / 255;
//					idx += 4;
//					image->set_pixel((int32_t)x, (int32_t)y, c);
//				}
//			}
//		}
//
//		file->close();
//
//		e = image->save_png(pngFilename);
//
//		//e = image->save_webp(webpFilename);
//		{
//			godot::PackedByteArray array = image->save_webp_to_buffer();
//
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().lock();
//			godot::Ref<godot::FileAccess> file = godot::FileAccess::open(webpFilename, godot::FileAccess::WRITE);
//			if (file == nullptr)
//				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing file ") + _webpFilename + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().unlock();
//			uint64_t size = array.size();
//			file->store_64(size);
//			file->store_buffer(array);
//			file->close();
//		}
//	}
//	
//	{
//		//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.3 ") + __FUNCTION__, "generate_mipmaps");
//		e = image->generate_mipmaps();
//		if (e != godot::Error::OK)
//			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("generate_mipmaps error ") + std::to_string(int(e))).c_str()));
//
//	}
//
//	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//	long long elapsed = (now - start).count();
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint((std::string("Reading ground texture ") + _fileName + " done! - Elapsed " + std::to_string(elapsed)  + " ms").c_str());
//
//	return image;
//}

std::string ShaderTerrainData::s_groundTexturesDir;
std::recursive_mutex ShaderTerrainData::s_internalMutex;

std::string ShaderTerrainData::getGroundTexturesDir(void)
{
	std::lock_guard<std::recursive_mutex> lock(s_internalMutex);
	if (s_groundTexturesDir.size() == 0)
	{
		std::string configFilePath = GDN_TheWorld_Globals::configFilePath();
		Json::Value root;
		std::ifstream jsonFile(configFilePath);
		jsonFile >> root;
		s_groundTexturesDir = root["GroundTexturesDir"].asString();
		if (s_groundTexturesDir.find(':') == std::string::npos)
		{
			// dir acquired from json not contains ":"
			s_groundTexturesDir = GDN_TheWorld_Globals::moduleLoadPath() + "\\" + s_groundTexturesDir;
		}
	}
	return s_groundTexturesDir;
}

godot::Ref<godot::Image> ShaderTerrainData::loadGroundImage(std::string filePath, std::string mapType, std::string groundImageName, int desiredImageSize, godot::Image::Format desiredImageFormat, bool returnEmpty, GDN_TheWorld_Viewer* viewer)
{
	godot::Ref<godot::Image> image;

	if (filePath.size() > 0)
		image = godot::Image::load_from_file(filePath.c_str());

	if (image != nullptr)
	{
		int32_t width = image->get_width();
		int32_t height = image->get_height();
		if (width != desiredImageSize or height != desiredImageSize)
		{
			image->resize(desiredImageSize, desiredImageSize, godot::Image::Interpolation::INTERPOLATE_BILINEAR);
			width = image->get_width();		// DEBUG
			height = image->get_height();		// DEBUG
			if (viewer != nullptr)
			{
				std::string msg = std::string("WRONG DIMENSION (not ") + std::to_string(desiredImageSize) + "x" + std::to_string(desiredImageSize) + ") FOR (" + mapType + " texture of " + groundImageName + " ground texture";
				viewer->Globals()->errorPrint(msg.c_str());
			}
		}

		if (int(desiredImageFormat) >= 0)
		{
			godot::Image::Format format = image->get_format();
			if (format != desiredImageFormat)
			{
				image->convert(desiredImageFormat);
				format = image->get_format();	// DEBUG
				if (viewer != nullptr)
				{
					std::string msg = std::string("WRONG FORMAT (not ") + std::to_string(desiredImageFormat) + ") FOR (" + mapType + " texture of " + groundImageName + " ground texture";
					viewer->Globals()->errorPrint(msg.c_str());
				}
			}
		}
	}
	else
	{
		if (returnEmpty)
		{
			godot::Ref<godot::Image> emptyImage = godot::Image::create_empty(desiredImageSize, desiredImageSize, false, desiredImageFormat);
			image = emptyImage;
			if (viewer != nullptr)
				viewer->Globals()->errorPrint(std::string("Missing PBR " + mapType + " texture of " + groundImageName + " ground texture").c_str());
		}
	}

	return image;
}

godot::Ref<godot::Texture2D> ShaderTerrainData::loadGroundTexture(std::string filePath, std::string mapType, std::string groundImageName, int desiredImageSize, godot::Image::Format desiredImageFormat, bool returnEmpty, GDN_TheWorld_Viewer* viewer)
{
	godot::Ref<godot::Texture2D> tex;
	godot::Ref<godot::Image> image = loadGroundImage(filePath, mapType, groundImageName, desiredImageSize, desiredImageFormat, returnEmpty, viewer);
	if (image != nullptr)
	{
		tex = godot::ImageTexture::create_from_image(image);
	}
	return tex;
}

bool ShaderTerrainData::loadGroundImagesByName(std::string imagesDirPath, std::string imagesDirName, std::string textureName, GDN_TheWorld_Viewer* viewer,
	godot::Ref<godot::Image>& albedoDisplImage, 
	godot::Ref<godot::Image>& normalRoughnessImage, 
	godot::Ref<godot::Texture2D>& texturePreview)
{
	bool loaded = true;

	std::string searchingDirPath = imagesDirPath + imagesDirName;
	if (not fs::is_directory(searchingDirPath))
		return false;
	
	const int desiredImageSize = 1024;
	const int texturePreviewDesiredImageSize = 512;
	const godot::Image::Format desiredImageFormat = godot::Image::Format::FORMAT_RGB8;

	std::string albedoDisplTextureFilePath, normalRoughnessTextureFilePath;
	std::string albedoTextureFilePath, normalTextureFilePath, roughnessTextureFilePath, displacementTextureFilePath, ambientOcclusionFilePath, texturePreviewFilePath;

	// find all needed textures in input folder
	for (const auto& entry : fs::directory_iterator(searchingDirPath))
	{
		if (entry.is_regular_file())
		{
			std::string filename = entry.path().filename().generic_string();

			if (GDN_TheWorld_Globals::isImageExt(filename))
			{
				if (GDN_TheWorld_Globals::isAlbedoDisplTexture(filename))
				{
					albedoDisplTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isNormalRoughnessTexture(filename))
				{
					normalRoughnessTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isAlbedoTexture(filename))
				{
					albedoTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isNormalTexture(filename))
				{
					normalTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isRoughnessTexture(filename))
				{
					roughnessTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isDisplacementTexture(filename))
				{
					displacementTextureFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isAmbientOcclusionTexture(filename))
				{
					ambientOcclusionFilePath = searchingDirPath + "/" + filename;
				}
				else if (GDN_TheWorld_Globals::isTexturePreview(filename))
				{
					texturePreviewFilePath = searchingDirPath + "/" + filename;
				}
			}
		}
	}

	// check if albedo_displ texture need to be recreated
	if (albedoDisplTextureFilePath.size() > 0 and (albedoTextureFilePath.size() > 0 or displacementTextureFilePath.size() > 0 or ambientOcclusionFilePath.size() > 0))
	{
		std::chrono::system_clock::time_point albedoDisplTextureTime = TheWorld_Utils::last_write_time_point_utc(albedoDisplTextureFilePath);
		if (albedoTextureFilePath.size() > 0)
		{
			std::chrono::system_clock::time_point albedoTextureTime = TheWorld_Utils::last_write_time_point_utc(albedoTextureFilePath);
			if (albedoTextureTime > albedoDisplTextureTime)
				albedoDisplTextureFilePath.clear();
		}
		if (displacementTextureFilePath.size() > 0)
		{
			std::chrono::system_clock::time_point displacementTextureTime = TheWorld_Utils::last_write_time_point_utc(displacementTextureFilePath);
			if (displacementTextureTime > albedoDisplTextureTime)
				albedoDisplTextureFilePath.clear();
		}
		if (ambientOcclusionFilePath.size() > 0)
		{
			std::chrono::system_clock::time_point ambientOcclusionTextureTime = TheWorld_Utils::last_write_time_point_utc(ambientOcclusionFilePath);
			if (ambientOcclusionTextureTime > albedoDisplTextureTime)
				albedoDisplTextureFilePath.clear();
		}
	}

	// check if normal_roughness texture need to be recreated
	if (normalRoughnessTextureFilePath.size() > 0 and (normalTextureFilePath.size() > 0 or roughnessTextureFilePath.size() > 0))
	{
		std::chrono::system_clock::time_point normalRoughnessTextureTime = TheWorld_Utils::last_write_time_point_utc(normalRoughnessTextureFilePath);
		if (normalTextureFilePath.size() > 0)
		{
			std::chrono::system_clock::time_point normalTextureTime = TheWorld_Utils::last_write_time_point_utc(normalTextureFilePath);
			if (normalTextureTime > normalRoughnessTextureTime)
				normalRoughnessTextureFilePath.clear();
		}
		if (roughnessTextureFilePath.size() > 0)
		{
			std::chrono::system_clock::time_point roughnessTextureTime = TheWorld_Utils::last_write_time_point_utc(roughnessTextureFilePath);
			if (roughnessTextureTime > normalRoughnessTextureTime)
				normalRoughnessTextureFilePath.clear();
		}
	}

	// check if preview texture need to be recreated
	if (texturePreviewFilePath.size() > 0 and albedoTextureFilePath.size() > 0)
	{
		std::chrono::system_clock::time_point previewTextureTime = TheWorld_Utils::last_write_time_point_utc(texturePreviewFilePath);
		std::chrono::system_clock::time_point albedoTextureTime = TheWorld_Utils::last_write_time_point_utc(normalTextureFilePath);
		if (albedoTextureTime > previewTextureTime)
			texturePreviewFilePath.clear();
	}


	if (albedoDisplTextureFilePath.size() > 0 and normalRoughnessTextureFilePath.size() > 0 and texturePreviewFilePath.size() > 0)
	{
		// we have albedo_displ, normal_roughness and preview textures and all are up to date
		
		godot::Ref<godot::Image> image1 = godot::Image::load_from_file(albedoDisplTextureFilePath.c_str());
		my_assert(image1 != nullptr);
		//int32_t w = image1->get_width();				// DEBUG
		//int32_t h = image1->get_height();				// DEBUG
		//godot::Image::Format f = image1->get_format();	// DEBUG
		albedoDisplImage = image1;

		godot::Ref<godot::Image> image2 = godot::Image::load_from_file(normalRoughnessTextureFilePath.c_str());
		my_assert(image2 != nullptr);
		//w = image2->get_width();	// DEBUG
		//h = image2->get_height();	// DEBUG
		//f = image2->get_format();	// DEBUG
		normalRoughnessImage = image2;

		godot::Ref<godot::Image> image3 = godot::Image::load_from_file(texturePreviewFilePath.c_str());
		my_assert(image3 != nullptr);
		//w = image3->get_width();	// DEBUG
		//h = image3->get_height();	// DEBUG
		//f = image3->get_format();	// DEBUG
		godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image3);
		texturePreview = tex;
	}
	else
	{
		// we have to create albedo_displ, normal_roughness textures and preview texture because almost one of them is not available or is not up to date
	
		if (albedoTextureFilePath.size() > 0)
		{
			// AlbedoDispl image contains as forth color (a) the value from ambient occlusion map istead of the value from displacement map
			godot::Ref<godot::Image> albedoImage = loadGroundImage(albedoTextureFilePath, "ALBEDO", textureName, desiredImageSize, godot::Image::Format(-1), false, viewer);
			my_assert(albedoImage != nullptr);
			//godot::Ref<godot::Image> displImage = loadGroundImage(displacementTextureFilePath, "DISPLACEMENT", textureName, desiredImageSize, godot::Image::Format(-1), true, viewer);
			//my_assert(displImage != nullptr);
			godot::Ref<godot::Image> aoImage = loadGroundImage(ambientOcclusionFilePath, "AMBIENTOCCLUSION", textureName, desiredImageSize, godot::Image::Format(-1), true, viewer);
			my_assert(aoImage != nullptr);

			godot::Ref<godot::Image> image = godot::Image::create_empty(desiredImageSize, desiredImageSize, false, godot::Image::Format::FORMAT_RGBA8);
			for(int y = 0; y < desiredImageSize; y++)
				for (int x = 0; x < desiredImageSize; x++)
				{
					godot::Color color = albedoImage->get_pixel(x, y);
					//if (displImage->is_empty())
					//	color.a = 0;
					//else
					//	color.a = displImage->get_pixel(x, y).r;
					if (aoImage->is_empty())
						color.a = 1;
					else
						color.a = aoImage->get_pixel(x, y).r;
					image->set_pixel(x, y, color);
				}
			albedoDisplImage = image;
			std::string s = searchingDirPath + "/" + imagesDirName + GDN_TheWorld_Globals::s_albedo_displ_ext;
			godot::Error e = image->save_png(s.c_str());
			my_assert(e == godot::Error::OK);

			godot::Ref<godot::Image> normalImage = loadGroundImage(normalTextureFilePath, "NORMAL", textureName, desiredImageSize, godot::Image::Format(-1), true, viewer);
			my_assert(normalImage != nullptr);
			godot::Ref<godot::Image> roughnessImage = loadGroundImage(roughnessTextureFilePath, "ROUGHNESS", textureName, desiredImageSize, godot::Image::Format(-1), true, viewer);
			my_assert(roughnessImage != nullptr);
			for (int y = 0; y < desiredImageSize; y++)
				for (int x = 0; x < desiredImageSize; x++)
				{
					godot::Color color(1, 1, 1, 1);
					if (not normalImage->is_empty())
					{
						godot::Color normal = normalImage->get_pixel(x, y);
						color.r = normal.r;
						color.g = normal.g;
						color.b = normal.b;
					}
					if (not roughnessImage->is_empty())
						color.a = roughnessImage->get_pixel(x, y).r;
					image->set_pixel(x, y, color);
				}
			normalRoughnessImage = image;
			s = searchingDirPath + "/" + imagesDirName + GDN_TheWorld_Globals::s_normal_roughness_ext;
			e = image->save_png(s.c_str());
			my_assert(e == godot::Error::OK);

			godot::Ref<godot::Image> previewImage = godot::Image::create_from_data(albedoImage->get_width(), albedoImage->get_height(), false, albedoImage->get_format(), albedoImage->get_data());;
			if (previewImage->get_width() != texturePreviewDesiredImageSize or previewImage->get_height() != texturePreviewDesiredImageSize)
				previewImage->resize(texturePreviewDesiredImageSize, texturePreviewDesiredImageSize, godot::Image::Interpolation::INTERPOLATE_BILINEAR);
			if (previewImage->get_format() != desiredImageFormat)
				previewImage->convert(desiredImageFormat);
			godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(previewImage);
			texturePreview = tex;
			s = searchingDirPath + "/" + imagesDirName + "_texture_preview.png";
			e = previewImage->save_png(s.c_str());
			my_assert(e == godot::Error::OK);
		}
		else
		{
			if (viewer != nullptr)
			{
				std::string msg = std::string("Missing albedo file for texture [") + textureName + "]!";
				viewer->Globals()->debugPrint(msg.c_str());
			}
		}
	}

	return loaded;
}

void ShaderTerrainData::loadAllGroundTextures(std::string groundTexturesDir, GDN_TheWorld_Viewer* viewer)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTextures 1 ") + __FUNCTION__, "all");
	
	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	std::lock_guard<std::recursive_mutex> lock(s_mtxStaticData);

	s_groundTextures.clear();
	s_continents.clear();

	if (viewer != nullptr)
		viewer->Globals()->debugPrint("Reading ground textures");

	if (groundTexturesDir.length() == 0)
	{
		groundTexturesDir = ShaderTerrainData::getGroundTexturesDir().c_str();
	}
	
	godot::String globalizedDir = ProjectSettings::get_singleton()->globalize_path(groundTexturesDir.c_str());

	struct TextureEntry
	{
		godot::Color baseAlbedo = godot::Color(1.0, 1.0, 1.0);		// in shader base_values0.rgb 
		float unused_1 = 1;											// in shader base_values0.a to shader
		float baseScaleAO	 = 1;									// in shader base_values1.r to shader
		float baseScaleNormal = 1;									// in shader base_values1.g to shader
		float baseScaleRoughness = 1;								// in shader base_values1.b to shder
		float baseScaleUV = 1;										// in shader base_values1.a to shader
		std::string folder;
		std::string layout;
		std::string textureName;
	};
	std::map<std::string, TextureEntry> textures;

	std::string globalizedGroundTexturesDir = std::string(globalizedDir.utf8().get_data());
	if (!fs::exists(globalizedGroundTexturesDir))
	{
		if (viewer != nullptr)
			viewer->Globals()->debugPrint((std::string("Ground texture dir <") + globalizedGroundTexturesDir + "> does not exist!").c_str());
		return;
	}

	std::string groundTexturesJsonPath = viewer->Globals()->getGroundJsonFilePath();
	if (!fs::exists(groundTexturesJsonPath))
	{
		if (viewer != nullptr)
			viewer->Globals()->debugPrint((std::string("Ground texture json file <") + groundTexturesJsonPath + "> does not exist!").c_str());
		return;
	}

	std::ifstream groundTexturesJson(groundTexturesJsonPath);
	if (groundTexturesJson.fail() || !groundTexturesJson.is_open())
	{
		if (viewer != nullptr)
			viewer->Globals()->debugPrint((std::string("Could not open ground texture json file <") + groundTexturesJsonPath + ">!").c_str());
		return;
	}
	Json::Value root;
	try
	{
		// loading/parsing json
		groundTexturesJson >> root;
	}
	catch (std::exception)
	{
		if (viewer != nullptr)
			viewer->Globals()->debugPrint((std::string("Error parsing json file <") + groundTexturesJsonPath + ">!").c_str());
		return;
	}

	// iterating json
	const Json::Value jsonContinentsArray = root["continents"];
	Json::Value::UInt size = jsonContinentsArray.size();	// DEBUG
	for (Json::Value::UInt idx = 0; idx < jsonContinentsArray.size(); idx++)
	{
		const int32_t id = jsonContinentsArray[idx]["id"].asInt();
		const std::string name = jsonContinentsArray[idx]["name"].asString();
		const std::string layout = jsonContinentsArray[idx]["layout"].asString();
		s_continents[id] = std::make_unique<Continent>(id, name, layout);
		for (int i = 1; ; i++)
		{
			char buffer[10];
			sprintf_s<sizeof(buffer)>(buffer, "%02d", i);
			const std::string marginName = std::string("margin_") + buffer;
			const std::string marginAsString = jsonContinentsArray[idx][marginName].asString();
			if (marginAsString.length() == 0)
				break;
			bool ok = s_continents[id]->addMargin(marginAsString);
			if (not ok)
				if (viewer != nullptr)
					viewer->Globals()->debugPrint((std::string("Margin value syntax is not correct <") + marginAsString + "> for continent <" + name + " (" + std::to_string(id)  + ")>").c_str());
		}
	}

	const Json::Value jsonGroundTextureLayoutsArray = root["ground-texture-layouts"];
	size = jsonGroundTextureLayoutsArray.size();	// DEBUG
	for (Json::Value::UInt idxLayout = 0; idxLayout < jsonGroundTextureLayoutsArray.size(); idxLayout++)
	{
		const std::string layoutName = jsonGroundTextureLayoutsArray[idxLayout]["layout-name"].asString();
		const Json::Value jsonGroundTexturesForLayoutArray = jsonGroundTextureLayoutsArray[idxLayout]["layout-textures"];
		size = jsonGroundTexturesForLayoutArray.size();	// DEBUG
		for (Json::Value::UInt idxTexturesForLayout = 0; idxTexturesForLayout < jsonGroundTexturesForLayoutArray.size(); idxTexturesForLayout++)
		{
			const std::string textureType = jsonGroundTexturesForLayoutArray[idxTexturesForLayout]["type"].asString();
			const Json::Value jsonTextureTypeQualifierArray = jsonGroundTexturesForLayoutArray[idxTexturesForLayout]["sub-types"];
			size = jsonTextureTypeQualifierArray.size();	// DEBUG
			for (Json::Value::UInt textureTypeQualifierIdx = 0; textureTypeQualifierIdx < jsonTextureTypeQualifierArray.size(); textureTypeQualifierIdx++)
			{
				const Json::Value jsonTextureSubTypeObject = jsonTextureTypeQualifierArray[textureTypeQualifierIdx];
				const std::string textureSubTypeQualifier = jsonTextureSubTypeObject["qualifier"].asString();
				struct TextureEntry textureEntry;
				Json::Value v = jsonTextureSubTypeObject["folder"];
				const std::string textureName = textureType + "_" + textureSubTypeQualifier;
				const std::string textureKey = layoutName + "_" + textureName;
				if (v.isNull())
				{
					if (viewer != nullptr)
						viewer->Globals()->debugPrint((std::string("Missing folder file for texture <") + textureName + "> layout <" + layoutName + ">!").c_str());
				}
				else
				{
					textureEntry.layout = layoutName;
					textureEntry.textureName = textureName;
					textureEntry.folder = v.asString();
					v = jsonTextureSubTypeObject["base-albedo"];
					if (not v.isNull())
					{
						std::vector<std::string> vect = TheWorld_Utils::Utils::split(v.asString(), ',');
						if (vect.size() == 3)
						{
							if (TheWorld_Utils::Utils::StringContains(vect[0], "."))
								textureEntry.baseAlbedo.r = std::stof(vect[0]);
							else
								textureEntry.baseAlbedo.r = std::stof(vect[0]) / 255.0f;
							if (TheWorld_Utils::Utils::StringContains(vect[1], "."))
								textureEntry.baseAlbedo.g = std::stof(vect[1]);
							else
								textureEntry.baseAlbedo.g = std::stof(vect[1]) / 255.0f;
							if (TheWorld_Utils::Utils::StringContains(vect[2], "."))
								textureEntry.baseAlbedo.b = std::stof(vect[2]);
							else
								textureEntry.baseAlbedo.b = std::stof(vect[2]) / 255.0f;
						}
						else
							if (viewer != nullptr)
								viewer->Globals()->debugPrint((std::string("Base-albedo syntax error for texture <") + textureName + "> layout <" + layoutName + ">!").c_str());
					}
					v = jsonTextureSubTypeObject["base-scale-ao"];
					if (not v.isNull())
						textureEntry.baseScaleAO = float(v.asDouble()) / 255.0f;
					v = jsonTextureSubTypeObject["base-scale-normal"];
					if (not v.isNull())
						textureEntry.baseScaleNormal = float(v.asDouble()) / 255.0f;
					v = jsonTextureSubTypeObject["base-scale-roughness"];
					if (not v.isNull())
						textureEntry.baseScaleRoughness = float(v.asDouble()) / 255.0f;
					v = jsonTextureSubTypeObject["base-scale-uv"];
					if (not v.isNull())
						textureEntry.baseScaleUV = float(v.asDouble()) / 255.0f;
					textures[textureKey] = textureEntry;
				}
			}
		}
	}

	// loading images and assigning a layer index
	int idx = 0;
	godot::TypedArray<godot::Image> albedoDisplImageArray, normalRoughnessImageArray;
	godot::TypedArray<godot::Color> baseValues0Array, baseValues1Array;
	for (auto& texture : textures)
	{
		TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

		godot::Ref<godot::Image> albedoDisplImage, normalRoughnessImage;
		godot::Ref<godot::Texture2D> texturePreview;
		bool loaded = loadGroundImagesByName(globalizedGroundTexturesDir, texture.second.folder, texture.first, viewer, albedoDisplImage, normalRoughnessImage, texturePreview);
		if (loaded)
		{
			//{
			//	int32_t w = albedoDisplImage->get_width();
			//	int32_t h = albedoDisplImage->get_height();
			//	godot::Image::Format f = albedoDisplImage->get_format();
			//	viewer->Globals()->debugPrint((std::string("size ") + std::to_string(w) + "x" + std::to_string(h) + " format " + std::to_string(int(f))).c_str());
			//}
			albedoDisplImageArray.push_back(albedoDisplImage);
			normalRoughnessImageArray.push_back(normalRoughnessImage);
			baseValues0Array.push_back(texture.second.baseAlbedo);	// ATM only rgb values are used, a channel is free for future implementations
			baseValues1Array.push_back(godot::Color(texture.second.baseScaleAO, texture.second.baseScaleNormal, texture.second.baseScaleRoughness, texture.second.baseScaleUV));

			s_groundTextures[texture.first] = std::make_unique<ShaderTerrainData::GroundTexture>(idx, texture.second.layout, texture.second.textureName, texturePreview);

			TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			long long elapsed = (now - start).count();
			if (viewer != nullptr)
				viewer->Globals()->debugPrint((std::string("Loaded ground_texture [") + texture.first + "] ==> idx = " + std::to_string(idx) + " - Elapsed = " + std::to_string(elapsed) + " ms").c_str());

			idx++;
		}
		else
		{
			if (viewer != nullptr)
			{
				std::string msg = std::string("Ground texture [") + texture.first + "] not loaded!";
				viewer->Globals()->debugPrint(msg.c_str());
			}
		}
	}

	// sending textures to shader: GLOBAL_SHADER_PARAM_ALBEDO_DISPL_ARRAY
	godot::Ref<godot::Texture2DArray> albedoDisplTexture2DArray = memnew(godot::Texture2DArray);
	godot::Error e = albedoDisplTexture2DArray->create_from_images(albedoDisplImageArray);
	my_assert(e == godot::Error::OK);
	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_ALBEDO_DISPL_ARRAY, albedoDisplTexture2DArray);
	
	// sending textures to shader: GLOBAL_SHADER_PARAM_NORMAL_ROUGHNESS_ARRAY
	godot::Ref<godot::Texture2DArray> normalRoughnessTexture2DArray = memnew(godot::Texture2DArray);
	e = normalRoughnessTexture2DArray->create_from_images(normalRoughnessImageArray);
	my_assert(e == godot::Error::OK);
	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_NORMAL_ROUGHNESS_ARRAY, normalRoughnessTexture2DArray);

	if (viewer != nullptr)
		viewer->Globals()->debugPrint((std::string("Loaded all ground_textures (") + std::to_string(albedoDisplImageArray.size()) + ")").c_str());

	// sending textures to shader: GLOBAL_SHADER_PARAM_BASE_VALUES_ARRAY
	//godot::Ref<godot::Image> baseValuesImage = godot::Image::create_empty(idx, 2, false, godot::Image::Format::FORMAT_RGBA8);
	godot::Ref<godot::Image> baseValuesImage = godot::Image::create_empty(idx, 2, false, godot::Image::Format::FORMAT_RGBAF);
	for (int x = 0; x < idx; x++)
	{
		godot::Color c0 = baseValues0Array[x];
		godot::Color c1 = baseValues1Array[x];
		baseValuesImage->set_pixel(x, 0, c0);
		//if (viewer != nullptr)	// DEBUG
		//	viewer->Globals()->debugPrint((std::string("BaseValueImage Row 0, Col ") + std::to_string(x) + " R=" + std::to_string(c0.r) + " G=" + std::to_string(c0.g) + " B=" + std::to_string(c0.b) + " A=" + std::to_string(c0.a)).c_str());	// DEBUG
		baseValuesImage->set_pixel(x, 1, c1);
		//if (viewer != nullptr)	// DEBUG
		//	viewer->Globals()->debugPrint((std::string("BaseValueImage Row 1, Col ") + std::to_string(x) + " R=" + std::to_string(c1.r) + " G=" + std::to_string(c1.g) + " B=" + std::to_string(c1.b) + " A=" + std::to_string(c1.a)).c_str());	// DEBUG
	}
	//baseValuesImage->save_exr("C:\\Users\\Riccardo\\AppData\\Roaming\\Godot\\app_userdata\\TheWorld\\temp\\a.exr");		// DEBUG - WARNING: BEFORE SAVING PNG/WEBP/JPG GODOT TRANSFORM FORMAT_RGBAF to FORMAT_RGB8 SO THE SAVED IMAGE HAS LESS RESOLUTION, ONLY EXR IS SAVED WITH THE CORRECT RESOLUTION
	godot::Ref<godot::Texture2D> tex = godot::ImageTexture::create_from_image(baseValuesImage);
	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_BASE_VALUES_ARRAY, tex);

	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
	long long elapsed = (now - start).count();

	if (viewer != nullptr)
		viewer->Globals()->debugPrint((std::string("Reading ground textures done! - Elapsed ") + std::to_string(elapsed) + " ms").c_str());
}

//void ShaderTerrainData::getGroundTexture(godot::String fileName, ShaderTerrainData::GroundTexture* groundTexture, GDN_TheWorld_Viewer* viewer)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1 ") + __FUNCTION__, "all");
//
//	godot::ResourceLoader* resLoader = ResourceLoader::get_singleton();
//
//	try
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1.1 ") + __FUNCTION__, "albedo_bump");
//
//		godot::String path = fileName + GDN_TheWorld_Globals::c_albedo_bump_file_ext.c_str();
//		std::string _path = std::string(path.utf8().get_data());
//
//		Ref<godot::CompressedTexture2D> c_tex = nullptr;
//		//c_tex = resLoader->load(path + ".png");		// must reload as image to create mipmaps (?)
//		if (c_tex == nullptr)
//		{
//			bool ok;
//			godot::Ref<godot::Image> image = readGroundTexture(path, ok, viewer);
//			//godot::UtilityFunctions::print((std::string(fileName.utf8().get_data()) + " (albedo_bump) ==> " + std::to_string(image->get_width()) + "x" + std::to_string(image->get_height()) + " " + std::to_string(image->get_format())).c_str());
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().lock();
//			// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
//			//tex->create_from_image(image, godot::ImageTexture::FLAG_FILTER | godot::ImageTexture::FLAG_MIPMAPS | godot::ImageTexture::FLAG_REPEAT);
//			Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().unlock();
//			groundTexture->m_albedo_bump_tex = tex;
//			groundTexture->m_tex = resLoader->load(path + ".png");
//		}
//		else
//			groundTexture->m_albedo_bump_tex = c_tex;
//
//	}
//	catch (...) {}
//
//	try
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1.2 ") + __FUNCTION__, "normal_roughness");
//
//		godot::String path = fileName + GDN_TheWorld_Globals::c_normal_roughness_file_ext.c_str();
//		std::string _path = std::string(path.utf8().get_data());
//
//		Ref<godot::CompressedTexture2D> c_tex = nullptr;
//		//c_tex = resLoader->load(path + ".png");		// must reload as image to create mipmaps (?)
//		if (c_tex == nullptr)
//		{
//			bool ok;
//			godot::Ref<godot::Image> image = readGroundTexture(path, ok, viewer);
//			//godot::UtilityFunctions::print((std::string(fileName.utf8().get_data()) + " (normal_roughness) ==> " + std::to_string(image->get_width()) + "x" + std::to_string(image->get_height()) + " " + std::to_string(image->get_format())).c_str());
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().lock();
//			// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
//			//tex->create_from_image(image, godot::ImageTexture::FLAG_FILTER | godot::ImageTexture::FLAG_MIPMAPS | godot::ImageTexture::FLAG_REPEAT);
//			Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
//			if (viewer != nullptr)
//				viewer->getMainProcessingMutex().unlock();
//			groundTexture->m_normal_roughness_tex = tex;
//		}
//		else
//			groundTexture->m_normal_roughness_tex = c_tex;
//	}
//	catch (...) {}
//
//	groundTexture->initialized = true;
//}

void ShaderTerrainData::mouseHitChanged(godot::Vector2 mouseHit)
{
	enum class ShaderTerrainData::LookDev lookdev = m_quadTree->getLookDev();

	if (lookdev == LookDev::ChunkLod)
	{
		std::map<int, godot::Ref<godot::ShaderMaterial>>& lookDevChunkLodMaterials = getLookDevChunkLodMaterials();
		for (auto& item : lookDevChunkLodMaterials)
			if (item.second != nullptr)
			{
				if (m_viewer->showPositionOnCursorHit() or m_viewer->highlightHitQuadrant())
				{
					item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, mouseHit);
					item.second->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, m_viewer->gridOn());
				}
				else
				{
					item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
					//item.second->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
				}
			}
	}
	else
	{
		godot::Ref<godot::ShaderMaterial> currentMaterial = (lookdev == LookDev::NotSet ? getRegularTerrainMaterial() : getLookDevMaterial(lookdev));
		godot::Ref<godot::ShaderMaterial> currentSeaMaterial = getRegularSeaMaterial();
		if (currentMaterial != nullptr && (m_viewer->showPositionOnCursorHit() or m_viewer->highlightHitQuadrant()))
		{
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, mouseHit);
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, m_viewer->gridOn());
			currentSeaMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, mouseHit);
			currentSeaMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, m_viewer->gridOn());
		}
		else
		{
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
			currentSeaMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_UV_QUADRANT, godot::Vector2(-1, -1));
			//currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, false);
		}
	}
}

//void ShaderTerrainData::clearCachedTexture(void)
//{
//	m_heightmapImage.unref();
//	m_resizedHeightmapImageMap.clear();	// occhio!!!
//	// other texture
//}

void ShaderTerrainData::releaseGlobals(void)
{
	std::lock_guard<std::recursive_mutex> lock(s_mtxStaticData);
	s_groundTextures.clear();
}

godot::Ref<godot::Image> ShaderTerrainData::getOrCreateHeightmapImage()
{
	if (m_heightmapImage == nullptr)
	{
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		std::string s;
		if (m_quadTree->getQuadrant()->executingProcess())
			s = "DURING PROCESS ";
		else
			s = "OUTSIDE PROCESS ";
		TheWorld_Utils::GuardProfiler profiler(std::string("getOrCreateHeightmapImage ") + s + __FUNCTION__, "create heightmap image");
		int _resolution = globals->heightmapResolution() + 1;
		TheWorld_Utils::MemoryBuffer& heightmapBuffer = m_quadTree->getQuadrant()->getFloat16HeightsBuffer();
		m_heightmapImage = createHeightmapImage(heightmapBuffer, _resolution);
		//godot::PackedByteArray data;
		//size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
		//size_t heightmapBufferSizeInBytes = _resolution * _resolution * sizeof(uint16_t);
		//assert(heightmapBufferSizeInBytes == heightsBufferSize);
		//data.resize((int)heightmapBufferSizeInBytes);
		//{
		//	memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
		//}
		//m_heightmapImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RH, data);
	}
	return m_heightmapImage;
}

godot::Ref<godot::Image> ShaderTerrainData::createHeightmapImage(TheWorld_Utils::MemoryBuffer& heightmapBuffer, int _resolution)
{
	godot::PackedByteArray data;
	size_t heightsBufferSize = heightmapBuffer.size();
	size_t heightmapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(uint16_t);
	assert(heightmapBufferSizeInBytes == heightsBufferSize);
	data.resize((int)heightmapBufferSizeInBytes);
	{
		memcpy((char*)data.ptrw(), heightmapBuffer.ptr(), heightsBufferSize);
	}
	return godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RH, data);
}

godot::Ref<godot::Image> ShaderTerrainData::getOrCreateNormalmapImage()
{
	if (m_normalmapImage == nullptr)
	{
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		std::string s;
		if (m_quadTree->getQuadrant()->executingProcess())
			s = "DURING PROCESS ";
		else
			s = "OUTSIDE PROCESS ";
		TheWorld_Utils::GuardProfiler profiler(std::string("getOrCreateNormalmapImage ") + s + __FUNCTION__, "create normalmap image");
		int _resolution = globals->heightmapResolution() + 1;
		TheWorld_Utils::MemoryBuffer& normalsBuffer = m_quadTree->getQuadrant()->getNormalsBuffer();
		m_normalmapImage = createNormalmapImage(normalsBuffer, _resolution);
		//size_t normalsBufferSize = normalsBuffer.size();
		//if (normalsBufferSize == 0)
		//{
		//	m_normalmapImage = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8);
		//	m_normalmapImage->fill(godot::Color(0.5f, 0.5f, 1.0f, 0.0f));	// packed normal vertical pointing up
		//}
		//else
		//{
		//	godot::PackedByteArray data;
		//	size_t normalmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
		//	data.resize((int)normalmapBufferSizeInBytes);
		//	assert(normalmapBufferSizeInBytes == normalsBufferSize);
		//	{
		//		memcpy((char*)data.ptrw(), normalsBuffer.ptr(), normalsBufferSize);
		//	}
		//	m_normalmapImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8, data);
		//}
		////{
		////	struct TheWorld_Utils::_RGB* rgb = (struct TheWorld_Utils::_RGB*)normalsBuffer.ptr();
		////	//image->lock();
		////	Color c = image->get_pixel(0, 0);
		////	size_t c_r = (size_t)(c.r * 255);
		////	size_t c_g = (size_t)(c.g * 255);
		////	size_t c_b = (size_t)(c.b * 255);
		////	//image->unlock();
		////}
	}
	return m_normalmapImage;
}

godot::Ref<godot::Image> ShaderTerrainData::createNormalmapImage(TheWorld_Utils::MemoryBuffer& normalmapBuffer, int _resolution)
{
	size_t normalsBufferSize = normalmapBuffer.size();
	if (normalsBufferSize == 0)
	{
		godot::Ref<godot::Image> image = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8);
		image->fill(godot::Color(0.5f, 0.5f, 1.0f, 0.0f));	// packed normal vertical pointing up
		return image;
	}
	else
	{
		godot::PackedByteArray data;
		size_t normalmapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
		data.resize((int)normalmapBufferSizeInBytes);
		assert(normalmapBufferSizeInBytes == normalsBufferSize);
		{
			memcpy((char*)data.ptrw(), normalmapBuffer.ptr(), normalsBufferSize);
		}
		return godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8, data);
		//{
		//	struct TheWorld_Utils::_RGB* rgb = (struct TheWorld_Utils::_RGB*)normalsBuffer.ptr();
		//	//image->lock();
		//	Color c = image->get_pixel(0, 0);
		//	size_t c_r = (size_t)(c.r * 255);
		//	size_t c_g = (size_t)(c.g * 255);
		//	size_t c_b = (size_t)(c.b * 255);
		//	//image->unlock();
		//}
	}
}

godot::Ref<godot::Image> ShaderTerrainData::getOrCreateSplatmapImage()
{
	if (m_splatmapImage == nullptr)
	{
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		std::string s;
		if (m_quadTree->getQuadrant()->executingProcess())
			s = "DURING PROCESS ";
		else
			s = "OUTSIDE PROCESS ";
		TheWorld_Utils::GuardProfiler profiler(std::string("getOrCreateSplatmapImage ") + s + __FUNCTION__, "create splatmap image");
		int _resolution = globals->heightmapResolution() + 1;
		TheWorld_Utils::MemoryBuffer& splatmapBuffer = m_quadTree->getQuadrant()->getSplatmapBuffer();
		m_splatmapImage = createSplatlmapImage(splatmapBuffer, _resolution);
		//size_t splatmapBufferSize = splatmapBuffer.size();
		//if (splatmapBufferSize == 0)
		//{
		//	m_splatmapImage = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8);
		//	m_splatmapImage->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));		// first texture TheWorld_Utils::TerrainEdit::extraValues.lowElevationTexName_r
		//}
		//else
		//{
		//	godot::PackedByteArray data;
		//	size_t splatmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
		//	data.resize((int)splatmapBufferSizeInBytes);
		//	assert(splatmapBufferSizeInBytes == splatmapBufferSize);
		//	{
		//		memcpy((char*)data.ptrw(), splatmapBuffer.ptr(), splatmapBufferSizeInBytes);
		//	}
		//	m_splatmapImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8, data);
		//}
	}
	
	return m_splatmapImage;
}

godot::Ref<godot::Image> ShaderTerrainData::createSplatlmapImage(TheWorld_Utils::MemoryBuffer& splatmapBuffer, int _resolution)
{
	size_t splatmapBufferSize = splatmapBuffer.size();
	if (splatmapBufferSize == 0)
	{
		godot::Ref<godot::Image> image = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8);
		image->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));		// first texture TheWorld_Utils::TerrainEdit::extraValues.lowElevationTexName_r
		return image;
	}
	else
	{
		godot::PackedByteArray data;
		size_t splatmapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
		data.resize((int)splatmapBufferSizeInBytes);
		assert(splatmapBufferSizeInBytes == splatmapBufferSize);
		{
			memcpy((char*)data.ptrw(), splatmapBuffer.ptr(), splatmapBufferSizeInBytes);
		}
		return godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8, data);
	}
}

godot::Ref<godot::Image> ShaderTerrainData::getOrCreateGlobalmapImage()
{
	if (m_globalmapImage == nullptr)
	{
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		std::string s;
		if (m_quadTree->getQuadrant()->executingProcess())
			s = "DURING PROCESS ";
		else
			s = "OUTSIDE PROCESS ";
		TheWorld_Utils::GuardProfiler profiler(std::string("getOrCreateGlobalmapImage ") + s + __FUNCTION__, "create globalmap image");
		int _resolution = globals->heightmapResolution() + 1;
		TheWorld_Utils::MemoryBuffer& globalmapBuffer = m_quadTree->getQuadrant()->getGlobalmapBuffer();
		m_globalmapImage = createGloablmapImage(globalmapBuffer, _resolution);
		//size_t globalmapBufferSize = globalmapBuffer.size();
		//if (globalmapBufferSize == 0)
		//{
		//	//m_globalmapImage = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8);
		//	//m_globalmapImage->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
		//}
		//else
		//{
		//	godot::PackedByteArray data;
		//	size_t globalmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
		//	data.resize((int)globalmapBufferSizeInBytes);
		//	assert(globalmapBufferSizeInBytes == globalmapBufferSize);
		//	{
		//		memcpy((char*)data.ptrw(), globalmapBuffer.ptr(), globalmapBufferSizeInBytes);
		//	}
		//	m_globalmapImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8, data);
		//}
	}
	return m_globalmapImage;
}

godot::Ref<godot::Image> ShaderTerrainData::createGloablmapImage(TheWorld_Utils::MemoryBuffer& globalmapBuffer, int _resolution)
{
	size_t globalmapBufferSize = globalmapBuffer.size();
	if (globalmapBufferSize == 0)
	{
		godot::Ref<godot::Image> image;
		//image = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8);
		//image->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
		return image;
	}
	else
	{
		godot::PackedByteArray data;
		size_t globalmapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
		data.resize((int)globalmapBufferSizeInBytes);
		assert(globalmapBufferSizeInBytes == globalmapBufferSize);
		{
			memcpy((char*)data.ptrw(), globalmapBuffer.ptr(), globalmapBufferSizeInBytes);
		}
		return godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGB8, data);
	}
}

godot::Ref<godot::Image> ShaderTerrainData::getOrCreateColormapImage()
{
	if (m_colormapImage == nullptr)
	{
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		std::string s;
		if (m_quadTree->getQuadrant()->executingProcess())
			s = "DURING PROCESS ";
		else
			s = "OUTSIDE PROCESS ";
		TheWorld_Utils::GuardProfiler profiler(std::string("getOrCreateColormapImage ") + s + __FUNCTION__, "create colormap image");
		int _resolution = globals->heightmapResolution() + 1;
		TheWorld_Utils::MemoryBuffer& colormapBuffer = m_quadTree->getQuadrant()->getColormapBuffer();
		m_colormapImage = createColormapImage(colormapBuffer, _resolution);
		//size_t colormapBufferSize = colormapBuffer.size();
		//if (colormapBufferSize == 0)
		//{
		//	//m_colormapImage = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8);
		//	//m_colormapImage->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
		//}
		//else
		//{
		//	godot::PackedByteArray data;
		//	size_t colormapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
		//	data.resize((int)colormapBufferSizeInBytes);
		//	assert(colormapBufferSizeInBytes == colormapBufferSize);
		//	{
		//		memcpy((char*)data.ptrw(), colormapBuffer.ptr(), colormapBufferSizeInBytes);
		//	}
		//	m_colormapImage = godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8, data);
		//}
	}
	return m_colormapImage;
}

godot::Ref<godot::Image> ShaderTerrainData::createColormapImage(TheWorld_Utils::MemoryBuffer& colormapBuffer, int _resolution)
{
	size_t colormapBufferSize = colormapBuffer.size();
	if (colormapBufferSize == 0)
	{
		godot::Ref<godot::Image> image;
		//image = godot::Image::create(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8);
		//image->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
		return image;
	}
	else
	{
		godot::PackedByteArray data;
		size_t colormapBufferSizeInBytes = (size_t)_resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
		data.resize((int)colormapBufferSizeInBytes);
		size_t colormapBufferSize = colormapBuffer.size();
		assert(colormapBufferSizeInBytes == colormapBufferSize);
		{
			memcpy((char*)data.ptrw(), colormapBuffer.ptr(), colormapBufferSizeInBytes);
		}

		return godot::Image::create_from_data(_resolution, _resolution, false, godot::Image::Format::FORMAT_RGBA8, data);
	}
}

godot::Ref<godot::Image> ShaderTerrainData::getImageAtResolution(std::string& profileHeader, enum class ImageType imageType, size_t optimizationLevel, size_t resolution)
{
	TheWorld_Utils::GuardProfiler profiler(profileHeader + __FUNCTION__, std::to_string(resolution));

	godot::Ref<godot::Image> image;

	// godot::Ref<godot::Image> hiResImage, std::map<size_t, godot::Ref<godot::Image>>&resizedImageMap
	if (imageType != ImageType::heightmap && imageType != ImageType::normalmap && imageType != ImageType::splatmap && imageType != ImageType::globalmap && imageType != ImageType::colormap)
		return image;

	std::string _profileHeader = profileHeader;
	if (_profileHeader.back() == ' ')
	{
		_profileHeader.back() = '.';
	}
	else
		_profileHeader += ".";

	if (optimizationLevel == 0)
	{
		TheWorld_Utils::GuardProfiler profiler(_profileHeader + "1 " + __FUNCTION__, "get HiRes image " + std::to_string(resolution));

		if (imageType == ImageType::heightmap)
			return getOrCreateHeightmapImage();
		else if (imageType == ImageType::normalmap)
			return getOrCreateNormalmapImage();
		else if (imageType == ImageType::splatmap)
			return getOrCreateSplatmapImage();
		else if (imageType == ImageType::globalmap)
			return getOrCreateGlobalmapImage();
		else //if (imageType == ImageType::colormap)
			return getOrCreateColormapImage();
	}

	std::map<size_t, godot::Ref<godot::Image>>* resizedImageMap;

	if (imageType == ImageType::heightmap)
		resizedImageMap = &getResizedHeightmapImageMap();
	else if (imageType == ImageType::normalmap)
		resizedImageMap = &getResizedNormalmapImageMap();
	else if (imageType == ImageType::splatmap)
		resizedImageMap = &getResizedSplatmapImageMap();
	else if (imageType == ImageType::globalmap)
		resizedImageMap = &getResizedGlobalmapImageMap();
	else //if (imageType == ImageType::colormap)
		resizedImageMap = &getResizedColormapImageMap();

	if (resizedImageMap->contains(resolution))
	{
		TheWorld_Utils::GuardProfiler profiler(_profileHeader + "2 " + __FUNCTION__, "get resized image from cache " + std::to_string(resolution));
		image = (*resizedImageMap)[resolution];
	}
	else
	{
		int hiResolution = m_viewer->Globals()->heightmapResolution() + 1;
		TheWorld_Utils::GuardProfiler profiler(_profileHeader + "3 " + __FUNCTION__, "resize image " + std::to_string(hiResolution) + " ==> " + std::to_string(resolution));

		godot::Ref<godot::Image> hiResImage;

		if (imageType == ImageType::heightmap)
			hiResImage = getOrCreateHeightmapImage();
		else if (imageType == ImageType::normalmap)
			hiResImage = getOrCreateNormalmapImage();
		else if (imageType == ImageType::splatmap)
			hiResImage = getOrCreateSplatmapImage();
		else if (imageType == ImageType::globalmap)
			hiResImage = getOrCreateGlobalmapImage();
		else //if (imageType == ImageType::colormap)
			hiResImage = getOrCreateColormapImage();

		if (hiResImage != nullptr)
		{
			std::string _profileHeader1 = _profileHeader + "4 ";
			image = resizeImage(_profileHeader1, *hiResImage, resolution, *resizedImageMap, optimizationLevel);
		}
	}

	return image;
}

godot::Ref<godot::Image> ShaderTerrainData::getImageAtResolution(std::string& profileHeader, godot::Ref<godot::Image> hiResImage, std::map<size_t, godot::Ref<godot::Image>>&resizedImageMap, size_t optimizationLevel, size_t resolution)
{
	godot::Ref<godot::Image> image;
	
	if (optimizationLevel == 0)
		return hiResImage;

	size_t hiResolution = hiResImage->get_width();

	if (resizedImageMap.contains(resolution))
	{
		TheWorld_Utils::GuardProfiler profiler(profileHeader + __FUNCTION__, "get resized image from cache " + std::to_string(hiResolution) + " ==> " + std::to_string(resolution));
		image = resizedImageMap[resolution];
	}
	else
	{
		TheWorld_Utils::GuardProfiler profiler(profileHeader + __FUNCTION__, "resize image " + std::to_string(hiResolution) + " ==> " + std::to_string(resolution));

		if (profileHeader.back() == ' ')
		{
			profileHeader.back() = '.';
			profileHeader += "1 ";
		}
		else
			profileHeader += ".1 ";
		image = resizeImage(profileHeader, hiResImage, resolution, resizedImageMap, optimizationLevel);
	}

	return image;
}

godot::Ref<godot::Image> ShaderTerrainData::resizeImage(std::string& profileHeader, godot::Ref<godot::Image> hiResImage, size_t newResolution, std::map<size_t, godot::Ref<godot::Image>>& resizedImageMap, size_t optimizationLevel)
{
	std::string _profileHeader = profileHeader;

	m_viewer->incNumTextureResize();
	
	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	size_t resolution = hiResImage->get_width();

	//std::string outFileName = quadrant->getCacheFilePath() + ".heightmap." + std::to_string(resolution) + ".png";		// DEBUG
	//hiResImage->save_png(outFileName.c_str());																		// DEBUG

	// doing a copy to resize
	//m_viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::Image> resizedImage = godot::Image::create_from_data(hiResImage->get_width(), hiResImage->get_height(), false, hiResImage->get_format(), hiResImage->get_data());
	//m_viewer->getMainProcessingMutex().unlock();

	resizedImage->resize((int32_t)newResolution, (int32_t)newResolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

	{
		TheWorld_Utils::GuardProfiler profiler(_profileHeader + __FUNCTION__, "resize image (adjust borders) " + std::to_string(resolution) + " ==> " + std::to_string(newResolution));

		int32_t newStep = 1 << optimizationLevel;
		int32_t hiResImageWidth = hiResImage->get_width();
		int32_t hiResImageHeight = hiResImage->get_height();
		int32_t resizedImageWidth = resizedImage->get_width();
		int32_t resizedImageHeight = resizedImage->get_height();
		for (int32_t x = 0; x < resizedImageWidth; x++)
		{
			resizedImage->set_pixel(x, 0, hiResImage->get_pixel(x * newStep, 0));
			resizedImage->set_pixel(x, resizedImageHeight - 1, hiResImage->get_pixel(x * newStep, hiResImageHeight - 1));
		}
		for (int32_t y = 0; y < resizedImageHeight; y++)
		{
			resizedImage->set_pixel(0, y, hiResImage->get_pixel(0, y * newStep));
			resizedImage->set_pixel(resizedImageWidth - 1, y, hiResImage->get_pixel(hiResImageWidth - 1, y * newStep));
		}
	}

	//outFileName = quadrant->getCacheFilePath() + ".heightmap." + std::to_string(newResolution) + ".png";		// DEBUG
	//resizedImage->save_png(outFileName.c_str());																// DEBUG

	resizedImageMap[newResolution] = resizedImage;

	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
	m_viewer->addToTextureResizingElapsed((now - start).count());

	//if (newResolution == 129)	// DEBUG ONLY
	//{
	//	int32_t w = resizedImage->get_width();
	//	int32_t h = resizedImage->get_height();
	//	int32_t size = w * h;
	//	godot::PackedByteArray array = resizedImage->get_data();
	//	int64_t size1 = array.size();
	//	{
	//		int32_t x = 0;
	//		int32_t y = 0;
	//		godot::Color c = resizedImage->get_pixel(x, y);
	//		uint16_t* p = (uint16_t*)array.ptr();
	//		TheWorld_Utils::FLOAT_32 f;
	//		int32_t index = (y * w) + x;
	//		f.u32 = half_to_float(*(p + index));
	//		size1 = array.size();
	//	}
	//	{
	//		int32_t x = 10;
	//		int32_t y = 35;
	//		godot::Color c = resizedImage->get_pixel(x, y);
	//		uint16_t* p = (uint16_t*)array.ptr();
	//		TheWorld_Utils::FLOAT_32 f;
	//		int32_t index = (y * w) + x;
	//		f.u32 = half_to_float(*(p + index));
	//		size1 = array.size();
	//	}
	//}

	return resizedImage;
}

void ShaderTerrainData::materialParamsNeedUpdate(bool b, bool transformChanged)
{
	m_materialParamsNeedUpdate = b;
	if (b)
		m_materialParamsNeedUpdateTransformChanged = transformChanged;
	else
		m_materialParamsNeedUpdateTransformChanged = false;
	//if (b and m_quadTree->getQuadrant()->getPos().getIndexPosX() == -4 and m_quadTree->getQuadrant()->getPos().getIndexPosZ() == 16)	// DEBUG
	//{
	//	m_materialParamsNeedUpdateTransformChanged = transformChanged;
	//}	// DEBUG
}

void ShaderTerrainData::materialParamsNeedReset(bool b)
{
	if (b)
	{
		m_materialParamsNeedReset = b;
		//if (!m_quadTree->statusInitialized())	// DEBUG ONLY
		//	assert(!m_quadTree->statusInitialized());
	}
	else
		m_materialParamsNeedReset = b;
}

// it is expected that globals and World Datas are loaded
// TODORIC: maybe usefull for performance reasons specify which texture need update and which rect of the texture 
void ShaderTerrainData::resetMaterialParams(enum class QuadType type, bool& lastProcessExecutionInterrupted, LookDev lookdev)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3 ") + __FUNCTION__, "Quadrant resetMaterialParams - Quadrant type=" + QuadTree::quadTypeToString(type));

	switch (type)
	{
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		resetMaterialParams(QuadType::Terrain, lastProcessExecutionInterrupted, lookdev);
		resetMaterialParams(QuadType::Sea, lastProcessExecutionInterrupted, lookdev);
	}
	break;
	case QuadType::Terrain:
	{
		if (m_quadTree->containsQuadOfType(type))
		{
			//PLOG_DEBUG << "ShaderTerrainData::resetMaterialParams - Type=" << int(type) << " - " << m_quadTree->getQuadrant()->getPos().getName();

			TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

			GDN_TheWorld_Globals* globals = m_viewer->Globals();
			Quadrant* quadrant = m_quadTree->getQuadrant();

			quadrant->executingProcess(true);

			int _resolution = globals->heightmapResolution() + 1;

			TheWorld_Utils::TerrainEdit* terrainEdit = quadrant->getTerrainEdit();

			//std::string lowElevationTexName = terrainEdit->extraValues.lowElevationTexName_r;
			//if (lowElevationTexName.size() == 0)
			//{
			//	terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
			//	//lowElevationTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
			//	lowElevationTexName = terrainEdit->extraValues.lowElevationTexName_r;
			//}
			//std::string highElevationTexName = terrainEdit->extraValues.highElevationTexName_g;
			//if (highElevationTexName.size() == 0)
			//{
			//	terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::highElevation);
			//	//highElevationTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::highElevation);
			//	highElevationTexName = terrainEdit->extraValues.highElevationTexName_g;
			//}
			//std::string dirtTexName = terrainEdit->extraValues.dirtTexName_b;
			//if (dirtTexName.size() == 0)
			//{
			//	terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::dirt);
			//	//dirtTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::dirt);
			//	dirtTexName = terrainEdit->extraValues.dirtTexName_b;
			//}
			//std::string rocksTexName = terrainEdit->extraValues.rocksTexName_a;
			//if (rocksTexName.size() == 0)
			//{
			//	terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::rocks);
			//	//rocksTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::rocks);
			//	rocksTexName = terrainEdit->extraValues.rocksTexName_a;
			//}

			//{
			//	if (!s_groundTextures.contains(lowElevationTexName) || !s_groundTextures[lowElevationTexName]->initialized)
			//	{
			//		s_groundTextures[lowElevationTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			//		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[lowElevationTexName].get();
			//		std::string groundTexturesDir = ShaderTerrainData::getGroundTexturesDir();
			//		//getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + lowElevationTexName.c_str(), groundTexture, m_viewer);
			//		getGroundTexture(godot::String(groundTexturesDir.c_str()) + lowElevationTexName.c_str(), groundTexture, m_viewer);
			//	}

			//	if (!s_groundTextures.contains(highElevationTexName) || !s_groundTextures[highElevationTexName]->initialized)
			//	{
			//		s_groundTextures[highElevationTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			//		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[highElevationTexName].get();
			//		std::string groundTexturesDir = ShaderTerrainData::getGroundTexturesDir();
			//		//getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + highElevationTexName.c_str(), groundTexture, m_viewer);
			//		getGroundTexture(godot::String(groundTexturesDir.c_str()) + highElevationTexName.c_str(), groundTexture, m_viewer);
			//	}

			//	if (!s_groundTextures.contains(dirtTexName) || !s_groundTextures[dirtTexName]->initialized)
			//	{
			//		s_groundTextures[dirtTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			//		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[dirtTexName].get();
			//		std::string groundTexturesDir = ShaderTerrainData::getGroundTexturesDir();
			//		//getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + dirtTexName.c_str(), groundTexture, m_viewer);
			//		getGroundTexture(godot::String(groundTexturesDir.c_str()) + dirtTexName.c_str(), groundTexture, m_viewer);
			//	}

			//	if (!s_groundTextures.contains(rocksTexName) || !s_groundTextures[rocksTexName]->initialized)
			//	{
			//		s_groundTextures[rocksTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			//		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[rocksTexName].get();
			//		std::string groundTexturesDir = ShaderTerrainData::getGroundTexturesDir();
			//		//getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + rocksTexName.c_str(), groundTexture, m_viewer);
			//		getGroundTexture(godot::String(groundTexturesDir.c_str()) + rocksTexName.c_str(), groundTexture, m_viewer);
			//	}
			//}

			size_t optimizationLevel = quadrant->optimizationLevel();
			size_t newResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			bool optimizationLevelChanged = quadrant->optimizationLevelChanged();
			bool resetOptimizationChangeDelayed = false;

			// if heights updated create texture for shader (m_heightmapTexture), it will be freed we passed to the shader
			if (quadrant->heightsUpdated() || optimizationLevelChanged)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1 ") + __FUNCTION__, "Heightmap");

				std::string profileHeader("WorldDeploy 3.1.1 ");
				godot::Ref<godot::Image> image = getImageAtResolution(profileHeader, ImageType::heightmap, optimizationLevel, newResolution);

				if (image != nullptr)
				{
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1.3 ") + __FUNCTION__, "Create Texture from heights image");

						// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
						//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
						m_heightmapTexture = godot::ImageTexture::create_from_image(image);
						m_heightMapTexModified = true;
						//debugPrintTexture(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
					}
				}

				quadrant->setHeightsUpdated(false);
			}

			if (m_viewer->processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_resetMaterialParams_heightmap))
			{
				lastProcessExecutionInterrupted = true;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->incNumResetShaderParam((now - start).count());
				quadrant->executingProcess(false);
				return;
			}

			// if normals updated create texture for shader (m_normalmapTexture), it will be freed we passed to the shader. If normals have not been computed we fill the texture with Color(0.5f, 0.5f, 1.0f, 0.0f) (rgb 127, 127, 255, 0 ==> packed vertical normal pointing up)
			if (quadrant->normalsUpdated() || optimizationLevelChanged)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2 ") + __FUNCTION__, "Normalmap");

				std::string profileHeader("WorldDeploy 3.2.1 ");
				godot::Ref<godot::Image> image = getImageAtResolution(profileHeader, ImageType::normalmap, optimizationLevel, newResolution);

				if (image != nullptr)
				{
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2.3 ") + __FUNCTION__, "Create Texture from normal image");

						// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
						//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
						Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
						m_normalmapTexture = tex;
						m_normalMapTexModified = true;
						//debugPrintTexture(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
					}
				}

				quadrant->setNormalsUpdated(false);
			}

			if (m_viewer->processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_resetMaterialParams_normalmap))
			{
				lastProcessExecutionInterrupted = true;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->incNumResetShaderParam((now - start).count());
				quadrant->executingProcess(false);
				return;
			}

			if (!quadrant->renderGlobalMap() && (quadrant->splatmapUpdated() || quadrant->renderGlobalMapChanged() || optimizationLevelChanged))
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3 ") + __FUNCTION__, "Splatmap");

				std::string profileHeader("WorldDeploy 3.3.1 ");
				godot::Ref<godot::Image> image = getImageAtResolution(profileHeader, ImageType::splatmap, optimizationLevel, newResolution);

				if (image != nullptr)
				{
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3.3 ") + __FUNCTION__, "Create Texture from splatmap image");

						// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
						//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
						m_splatmapTexture = godot::ImageTexture::create_from_image(image);
						m_splatMapTexModified = true;
						//debugPrintTexture(SHADER_PARAM_TERRAIN_SPLATMAP, m_splatmapTexture);
					}
				}

				quadrant->renderGlobalMapChanged(false);
				quadrant->setSplatmapUpdated(false);
			}
			if (m_viewer->processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_resetMaterialParams_splatmap))
			{
				lastProcessExecutionInterrupted = true;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->incNumResetShaderParam((now - start).count());
				quadrant->executingProcess(false);
				return;
			}

			if (quadrant->renderGlobalMap())	// to always pass globalmap to shader: comment this line
			{
				if (quadrant->globalmapUpdated() || quadrant->renderGlobalMapChanged() || optimizationLevelChanged)
				{
					TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4 ") + __FUNCTION__, "Globalmap");

					std::string profileHeader("WorldDeploy 3.4.1 ");
					godot::Ref<godot::Image> image = getImageAtResolution(profileHeader, ImageType::globalmap, optimizationLevel, newResolution);

					if (image != nullptr)
					{
						{
							TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4.3 ") + __FUNCTION__, "Create Texture from globalmap image");

							// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
							//tex->create_from_image(image, godot::Texture::FLAG_FILTER | godot::Texture::FLAG_MIPMAPS);
							m_globalmapTexture = godot::ImageTexture::create_from_image(image);
							m_globalMapTexModified = true;
							//debugPrintTexture(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
						}
					}

					quadrant->renderGlobalMapChanged(false);
					quadrant->setGlobalmapUpdated(false);
				}
			}
			if (m_viewer->processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_resetMaterialParams_globalmap))
			{
				lastProcessExecutionInterrupted = true;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->incNumResetShaderParam((now - start).count());
				quadrant->executingProcess(false);
				return;
			}

			if (quadrant->colorsUpdated() || optimizationLevelChanged)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5 ") + __FUNCTION__, "Colormap");

				std::string profileHeader("WorldDeploy 3.5.1 ");
				godot::Ref<godot::Image> image = getImageAtResolution(profileHeader, ImageType::colormap, optimizationLevel, newResolution);

				if (image != nullptr)
				{
					{
						TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5.3 ") + __FUNCTION__, "Create Texture from colormap image");

						// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
						//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
						m_colormapTexture = godot::ImageTexture::create_from_image(image);
						m_colorMapTexModified = true;
						//debugPrintTexture(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
					}
				}

				quadrant->setColorsUpdated(false);
			}

			if (m_viewer->processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::Quadrant_resetMaterialParams_colormap))
			{
				lastProcessExecutionInterrupted = true;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->incNumResetShaderParam((now - start).count());
				quadrant->executingProcess(false);
				return;
			}

			if (optimizationLevelChanged && !resetOptimizationChangeDelayed)
				quadrant->optimizationLevelChanged(false);

			quadrant->executingProcess(false);
			quadrant->terrainDataJustLoaded(false);

			materialParamsNeedUpdate(true);

			TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_viewer->incNumResetShaderParam((now - start).count());
		}
	}
	break;
	case QuadType::Sea:
	{
		if (m_quadTree->containsQuadOfType(type))
		{
			//PLOG_DEBUG << "ShaderTerrainData::resetMaterialParams - Type=" << int(type) << " - " << m_quadTree->getQuadrant()->getPos().getName();
		}
	}
	break;
	}
}

//Color ShaderTerrainData::encodeNormal(Vector3 normal)
//{
//	normal = 0.5 * (normal + Vector3::ONE);
//	return Color(normal.x, normal.z, normal.y);
//}

void ShaderTerrainData::updateMaterialParams(enum class QuadType type, LookDev lookdev, bool freeMaps, bool updateTerrainData)
{
	switch (type)
	{
	case QuadType::NotSet:
	case QuadType::NotUsed:
	{
		bool _freeMaps = false;
		updateMaterialParams(QuadType::Terrain, lookdev, _freeMaps, updateTerrainData);
		updateMaterialParams(QuadType::Sea, lookdev, freeMaps, updateTerrainData);
	}
	break;
	case QuadType::Terrain:
	{
		if (m_quadTree->containsQuadOfType(type))
		{
			//PLOG_DEBUG << "ShaderTerrainData::updateMaterialParams - Type=" << int(type) << " - " << m_quadTree->getQuadrant()->getPos().getName();

			TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

			if (m_viewer->is_inside_tree())
			{
				Transform3D globalTransform = m_quadTree->getInternalGlobalTransform();
				//m_viewer->Globals()->debugPrint("getInternalGlobalTransform" + String(" t=") + String(globalTransform));

				Transform3D inverseTransform = globalTransform.affine_inverse();

				// This is needed to properly transform normals if the terrain is scaled
				Basis normalBasis = globalTransform.basis.inverse().transposed();

				bool selected = m_quadTree->editModeQuadrantSelected();
				float editMode = 0.0f;
				if (selected)
					editMode = 1.0f;

				bool gridOn = false;
				//if (selected)
					gridOn = m_viewer->gridOn();

				Quadrant* quadrant = m_quadTree->getQuadrant();

				TheWorld_Utils::TerrainEdit* terrainEdit = quadrant->getTerrainEdit();

				if (lookdev == LookDev::ChunkLod)
				{
					std::map<int, godot::Ref<godot::ShaderMaterial>>& lookDevChunkLodMaterials = getLookDevChunkLodMaterials();

					for (auto& item : lookDevChunkLodMaterials)
					{
						item.second->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
						item.second->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);

						if (materialParamsNeedUpdateTransformChanged())
							continue;

						item.second->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, editMode);
						item.second->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, gridOn);
						item.second->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
						item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_RADIUS, m_viewer->getShaderParamMouseHitRadius());
						item.second->set_shader_parameter(SHADER_PARAM_OPTIMIZATION_LEVEL, quadrant->optimizationLevel());
						item.second->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, m_viewer->getForceShaderElevation0());
						item.second->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
						if (m_heightMapTexModified && m_heightmapTexture != nullptr and m_heightmapTexture.is_valid())
							item.second->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);

						if (m_normalMapTexModified && m_normalmapTexture != nullptr and m_normalmapTexture.is_valid())
							item.second->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
					}
				}
				else
				{
					godot::Ref<godot::ShaderMaterial> currentMaterial;
					if (lookdev == LookDev::NotSet)
						currentMaterial = getRegularTerrainMaterial();
					else
						currentMaterial = getLookDevMaterial(lookdev);

					currentMaterial->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
					currentMaterial->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);

					if (!materialParamsNeedUpdateTransformChanged())
					{
						currentMaterial->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, editMode);
						currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, gridOn);
						currentMaterial->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
						currentMaterial->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, m_viewer->getForceShaderElevation0());
						currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_RADIUS, m_viewer->getShaderParamMouseHitRadius());
						currentMaterial->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);

						if (quadrant->textureUpdated() && lookdev == LookDev::NotSet)
						{
							godot::Vector4 v = quadrant->quadrantTextureLayers();
							currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_LAYERS, v);

							//if (m_sendTextureToShader)
							//{
							//	godot::Ref<godot::ImageTexture> lowElevation_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.lowElevationTexName_r]->m_albedo_bump_tex;
							//	if (lowElevation_albedo_bump_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_0));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_0, lowElevation_albedo_bump_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> lowElevation_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.lowElevationTexName_r]->m_normal_roughness_tex;
							//	if (lowElevation_normal_roughness_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0, lowElevation_normal_roughness_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> highElevation_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.highElevationTexName_g]->m_albedo_bump_tex;
							//	if (highElevation_albedo_bump_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_1));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_1, highElevation_albedo_bump_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> highElevation_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.highElevationTexName_g]->m_normal_roughness_tex;
							//	if (highElevation_normal_roughness_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1, highElevation_normal_roughness_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> dirt_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.dirtTexName_b]->m_albedo_bump_tex;
							//	if (dirt_albedo_bump_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_2));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_2, dirt_albedo_bump_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> dirt_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.dirtTexName_b]->m_normal_roughness_tex;
							//	if (dirt_normal_roughness_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2, dirt_normal_roughness_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> rocks_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.rocksTexName_a]->m_albedo_bump_tex;
							//	if (rocks_albedo_bump_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_3));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_3, rocks_albedo_bump_tex);
							//	}

							//	godot::Ref<godot::ImageTexture> rocks_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.rocksTexName_a]->m_normal_roughness_tex;
							//	if (rocks_normal_roughness_tex != nullptr)
							//	{
							//		//m_viewer->Globals()->debugPrint("setting terrain shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3));
							//		currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3, rocks_normal_roughness_tex);
							//	}
							//}

							quadrant->setTextureUpdated(false);
						}

						if (updateTerrainData)
						{
							if (m_heightMapTexModified && m_heightmapTexture != nullptr and m_heightmapTexture.is_valid())
							{
								//PLOG_DEBUG << "setting terrain shader_param=" << SHADER_PARAM_TERRAIN_HEIGHTMAP;
								currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
							}

							if (m_normalMapTexModified && m_normalmapTexture != nullptr and m_normalmapTexture.is_valid())
							{
								//PLOG_DEBUG << "setting terrain shader_param=" << SHADER_PARAM_TERRAIN_NORMALMAP;
								currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
							}

							//if (lookdev == LookDev::NotSet && m_globalMapTexModified && m_globalmapTexture != nullptr and m_globalmapTexture.is_valid())
							//{
							//	//PLOG_DEBUG << "setting terrain shader_param=" << SHADER_PARAM_TERRAIN_GLOBALMAP;
							//	currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_GLOBALMAP, m_globalmapTexture);
							//	//currentMaterial->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, false);			// to always pass globalmap to shader: UNCOMMENT this line
							//	currentMaterial->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, true);			// to always pass globalmap to shader: comment this line
							//	currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_SPLATMAP, nullptr);		// to always pass globalmap to shader: comment this line
							//}

							if (lookdev == LookDev::NotSet && m_splatMapTexModified && m_splatmapTexture != nullptr and m_splatmapTexture.is_valid())
							{
								//PLOG_DEBUG << "setting terrain shader_param=" << SHADER_PARAM_TERRAIN_SPLATMAP;
								currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_SPLATMAP, m_splatmapTexture);
								//currentMaterial->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, false);
								//currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_GLOBALMAP, nullptr);		// to always pass globalmap to shader: comment this line
							}

							if (lookdev == LookDev::NotSet && m_colorMapTexModified && m_colormapTexture != nullptr and m_colormapTexture.is_valid())
							{
								//PLOG_DEBUG << "setting terrain shader_param=" << SHADER_PARAM_TERRAIN_COLORMAP;
								currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
							}
						}

						godot::Plane ground_uv_scale = m_viewer->getShaderParamGroundUVScale();
						if (lookdev == LookDev::NotSet /* && ground_uv_scale.get_normal() != godot::Vector3(0.0f, 0.0f, 0.0f) && ground_uv_scale.d != 0.0f*/)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE, ground_uv_scale);
						}

						currentMaterial->set_shader_parameter(SHADER_PARAM_OPTIMIZATION_LEVEL, quadrant->optimizationLevel());

						bool depthBlending = m_viewer->getShaderParamDepthBlending();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_DEPTH_BLENDING, depthBlending);
						}

						bool triplanar = m_viewer->getShaderParamTriplanar();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_TRIPLANAR, triplanar);
						}

						godot::Plane tileReduction = m_viewer->getShaderParamTileReduction();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_TILE_REDUCTION_PER_TEXTURE, tileReduction);
						}

						float globalmapBlendStart = m_viewer->getShaderParamGlobalmapBlendStart();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_START, globalmapBlendStart);
						}

						float globalmapBlendDistance = m_viewer->getShaderParamGlobalmapBlendDistance();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_GLOBALMAP_BLEND_DISTANCE, globalmapBlendDistance);
						}

						godot::Plane colormapOpacity = m_viewer->getShaderParamColormapOpacity();
						if (lookdev == LookDev::NotSet)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_COLORMAP_OPACITY_PER_TEXTURE, colormapOpacity);
						}

						if (lookdev == LookDev::Heights && m_heightmapTexture != nullptr and m_heightmapTexture.is_valid())
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_heightmapTexture);
						}

						if (lookdev == LookDev::Normals && m_normalmapTexture != nullptr and m_normalmapTexture.is_valid())
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_normalmapTexture);
						}

						if (lookdev == LookDev::Splat && m_splatmapTexture != nullptr and m_splatmapTexture.is_valid())
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_splatmapTexture);
						}

						if (lookdev == LookDev::Color && m_colormapTexture != nullptr and m_colormapTexture.is_valid())
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_colormapTexture);
						}

						if (lookdev == LookDev::Global && m_globalmapTexture != nullptr and m_globalmapTexture.is_valid())
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_globalmapTexture);
						}

						if ((lookdev == LookDev::Universe
							|| lookdev == LookDev::ShaderArt
							|| lookdev == LookDev::Lightning
							|| lookdev == LookDev::StarNest
							|| lookdev == LookDev::FlaringStar
							|| lookdev == LookDev::AnimatedDiamond
							|| lookdev == LookDev::PlasmaWaves)
							&& g_lookDevShaderTexture != nullptr)
						{
							currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, g_lookDevShaderTexture->getTexture());
						}
					}
				}

				if (freeMaps)
				{
					m_heightMapTexModified = false;
					m_normalMapTexModified = false;
					m_splatMapTexModified = false;
					m_colorMapTexModified = false;
					m_globalMapTexModified = false;

					m_heightmapTexture.unref();
					m_normalmapTexture.unref();
					m_splatmapTexture.unref();
					m_colormapTexture.unref();
					m_globalmapTexture.unref();
				}
			}

			TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
			m_viewer->incNumUpdateShaderParam((now - start).count());
		}
		else
		{
			if (freeMaps)
			{
				m_heightMapTexModified = false;
				m_normalMapTexModified = false;
				m_splatMapTexModified = false;
				m_colorMapTexModified = false;
				m_globalMapTexModified = false;

				m_heightmapTexture.unref();
				m_normalmapTexture.unref();
				m_splatmapTexture.unref();
				m_colormapTexture.unref();
				m_globalmapTexture.unref();
			}
		}
	}
	break;
	case QuadType::Sea:
	{
		if (m_quadTree->containsQuadOfType(type))
		{
			godot::Transform3D globalTransform = m_quadTree->getInternalGlobalTransform();
			godot::Transform3D inverseTransform = globalTransform.affine_inverse();

			godot::Ref<godot::ShaderMaterial> currentMaterial;
			if (lookdev == LookDev::NotSet)
				currentMaterial = getRegularSeaMaterial();
			else
				currentMaterial = getLookDevMaterial(lookdev);	// LookDev::ChunkLod not available for QuadType::Sea

			bool selected = m_quadTree->editModeQuadrantSelected();
			float editMode = 0.0f;
			if (selected)
				editMode = 1.0f;

			bool gridOn = false;
			if (selected)
				gridOn = m_viewer->gridOn();

			Quadrant* quadrant = m_quadTree->getQuadrant();

			//PLOG_DEBUG << "ShaderTerrainData::updateMaterialParams - Type=" << int(type) << " - " << m_quadTree->getQuadrant()->getPos().getName()
			//	<< " " << SHADER_PARAM_OPTIMIZATION_LEVEL << "=" << quadrant->optimizationLevel()
			//	<< " " << SHADER_PARAM_EDITMODE_SELECTED << "=" << editMode
			//	<< " " << SHADER_PARAM_MOUSE_GRID_ON << "=" << gridOn
			//	<< " " << SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS << "=" << m_viewer->getShaderParamQuadBorderLineThickness()
			//	<< " " << SHADER_PARAM_MOUSE_HIT_RADIUS << "=" << m_viewer->getShaderParamMouseHitRadius()
			//	<< " " << SHADER_PARAM_VERTICES_PER_CHUNK << "=" << (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f
			//	//<< SHADER_PARAM_INVERSE_TRANSFORM << "=" << gridOn
			//	<< " " << SHADER_PARAM_TERRAIN_HEIGHTMAP << "=" << m_heightmapTexture.is_valid();

			currentMaterial->set_shader_parameter(SHADER_PARAM_OPTIMIZATION_LEVEL, quadrant->optimizationLevel());
			currentMaterial->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, editMode);
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_GRID_ON, gridOn);
			currentMaterial->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_RADIUS, m_viewer->getShaderParamMouseHitRadius());
			currentMaterial->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, (float)m_viewer->Globals()->numVerticesPerChuckSide() + 1.0f);
			currentMaterial->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);

			if (updateTerrainData and m_heightMapTexModified and m_heightmapTexture != nullptr and m_heightmapTexture.is_valid())
			{
				//PLOG_DEBUG << "setting sea shader_param=" << SHADER_PARAM_TERRAIN_HEIGHTMAP;
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
			}

			if (freeMaps)
			{
				m_heightMapTexModified = false;
				m_normalMapTexModified = false;
				m_splatMapTexModified = false;
				m_colorMapTexModified = false;
				m_globalMapTexModified = false;

				m_heightmapTexture.unref();
				m_normalmapTexture.unref();
				m_splatmapTexture.unref();
				m_colormapTexture.unref();
				m_globalmapTexture.unref();
			}
		}
		else
		{
			if (freeMaps)
			{
				m_heightMapTexModified = false;
				m_normalMapTexModified = false;
				m_splatMapTexModified = false;
				m_colorMapTexModified = false;
				m_globalMapTexModified = false;

				m_heightmapTexture.unref();
				m_normalmapTexture.unref();
				m_splatmapTexture.unref();
				m_colormapTexture.unref();
				m_globalmapTexture.unref();
			}
		}
	}
	break;
	}
}

//void ShaderTerrainData::debugPrintTexture(std::string tex_name, Ref<Texture> tex)
//{
//	File* _file = File::_new();
//	_file->open("res://" + String(tex_name.c_str()) + ".txt", File::WRITE);
//	Ref<Image> _image = tex->get_data();
//	int64_t width = _image->get_width();
//	int64_t height = _image->get_height();
//	Image::Format format = _image->get_format();
//	std::string formatStr = std::to_string(format);
//	if (format == Image::Format::FORMAT_RH)
//		formatStr = "FORMAT_RH";
//	else if (format == Image::Format::FORMAT_RGB8)
//		formatStr = "FORMAT_RGB8";
//	else if (format == Image::Format::FORMAT_RGBA8)
//		formatStr = "FORMAT_RGBA8";
//	std::string text = "Name=" + tex_name + " Format=" + formatStr + " W=" + std::to_string(width) + " H=" + std::to_string(height);
//	_file->store_string(String(text.c_str()) + "\n");
//	_image->lock();
//	for (int h = 0; h < height; h++)
//	{
//		std::string row = "";
//		for (int w = 0; w < width; w++)
//		{
//			row += std::to_string(w) + ":";
//			if (format == Image::Format::FORMAT_RH)
//				row += std::to_string(_image->get_pixel(w, h).r) + " ";
//			else if (format == Image::Format::FORMAT_RGB8)
//				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + " ";
//			else if (format == Image::Format::FORMAT_RGBA8)
//				row += std::to_string(_image->get_pixel(w, h).r) + "-" + std::to_string(_image->get_pixel(w, h).g) + "-" + std::to_string(_image->get_pixel(w, h).b) + "-" + std::to_string(_image->get_pixel(w, h).a) + " ";
//			else
//			{
//				String s = String(_image->get_pixel(w, h));
//				char* str = s.alloc_c_string();
//				row += str;
//				row += " ";
//				godot::api->godot_free(str);
//			}
//		}
//		text = "H=" + std::to_string(h) + " " + row;
//		_file->store_string(String(text.c_str()) + "\n");
//	}
//	_image->unlock();
//	_file->close();
//}

/*
QuadrantPos QuadrantPos::getQuadrantPos(enum class DirectionSlot dir, int numSlot)
{
	QuadrantPos pos = *this;

	switch (dir)
	{
	case DirectionSlot::XMinus:
	{
		pos.m_lowerXGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::XPlus:
	{
		pos.m_lowerXGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZMinus:
	{
		pos.m_lowerZGridVertex -= (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZPlus:
	{
		pos.m_lowerZGridVertex += (m_sizeInWU * numSlot);
	}
	break;
	case DirectionSlot::ZMinusXMinus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZMinus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XMinus, numSlot);
	}
	break;
	case DirectionSlot::ZMinusXPlus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZMinus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XPlus, numSlot);
	}
	break;
	case DirectionSlot::ZPlusXMinus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZPlus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XMinus, numSlot);
	}
	break;
	case DirectionSlot::ZPlusXPlus:
	{
		QuadrantPos pos1 = getQuadrantPos(DirectionSlot::ZPlus, numSlot);
		pos = pos1.getQuadrantPos(DirectionSlot::XPlus, numSlot);
	}
	break;
	}

	pos.resetName();

	return pos;
}

size_t QuadrantPos::distanceInPerimeter(QuadrantPos& q) const
{
	size_t distanceOnX = (size_t)ceil(abs(q.getLowerXGridVertex() - getLowerXGridVertex()) / q.m_sizeInWU);
	size_t distanceOnZ = (size_t)ceil(abs(q.getLowerZGridVertex() - getLowerZGridVertex()) / q.m_sizeInWU);
	if (distanceOnX > distanceOnZ)
		return distanceOnX;
	else
		return distanceOnZ;
}
*/

Quadrant::InternalQuadrantDataLockHolder::InternalQuadrantDataLockHolder(TheWorld_Utils::QuadrantPos pos, GDN_TheWorld_Viewer* viewer, bool lock, bool lockOnlyInitializedQuadrants)
{
	QuadTree* quadTree = viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		init(quadTree->getQuadrant(), lock, lockOnlyInitializedQuadrants);
}

Quadrant::InternalQuadrantDataLockHolder::InternalQuadrantDataLockHolder(QuadTree* quadTree, bool lock, bool lockOnlyInitializedQuadrants)
{
	init(quadTree->getQuadrant(), lock, lockOnlyInitializedQuadrants);
}

Quadrant::InternalQuadrantDataLockHolder::InternalQuadrantDataLockHolder(Quadrant* quadrant, bool lock, bool lockOnlyInitializedQuadrants)
{
	init(quadrant, lock, lockOnlyInitializedQuadrants);
}

Quadrant::InternalQuadrantDataLockHolder::~InternalQuadrantDataLockHolder()
{
	unlock();
}

void Quadrant::InternalQuadrantDataLockHolder::init(Quadrant* quadrant, bool lock, bool lockOnlyInitializedQuadrants)
{
	m_quadrant = quadrant;

	if (lockOnlyInitializedQuadrants and quadrant != nullptr and not quadrant->getQuadTree()->statusInitialized())	// occhio!!!
		return;

	if (lock)
		Quadrant::InternalQuadrantDataLockHolder::lock();
}

bool Quadrant::InternalQuadrantDataLockHolder::lock(void)
{
	if (m_quadrant == nullptr)
		return false;

	if (m_lockAcquired)
		m_numLocks++;
	else
	{
		m_lockAcquired = m_quadrant->lockInternalData();
		if (m_lockAcquired)
			m_numLocks = 1;
	}

	return m_lockAcquired;
}

void Quadrant::InternalQuadrantDataLockHolder::unlock(void)
{
	if (m_lockAcquired)
	{
		m_numLocks--;
		if (m_numLocks == 0)
		{
			m_quadrant->unlockInternalData();
			m_lockAcquired = false;
		}
	}
}

Quadrant::Quadrant(TheWorld_Utils::QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
	m_quadrantPos = quadrantPos;
	m_viewer = viewer;
	m_quadTree = quadTree;
	std::string dir = GDN_TheWorld_Globals::getClientDataDir();
	if (dir.length() < 10)
	{
		std::string msg = std::string("Beccato " + dir);
		PLOG_ERROR << msg;
		throw(GDN_TheWorld_Exception(__FUNCTION__, msg.c_str()));
	}
	std::string mapName = viewer->Globals()->getMapName();
	m_cache = TheWorld_Utils::MeshCacheBuffer(dir, mapName, m_quadrantPos.getGridStepInWU(), m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getLevel(), m_quadrantPos.getLowerXGridVertex(), m_quadrantPos.getLowerZGridVertex());
	m_shaderTerrainData = make_unique<ShaderTerrainData>(viewer, quadTree);
	m_collider = make_unique<Collider>(quadTree);
	Collider* c = m_collider.get();
	m_heightsUpdated = false;
	m_normalsUpdated = false;
	m_colorsUpdated = false;
	m_splatmapUpdated = false;
	m_globalmapUpdated = false;
	m_texturesUpdated = false;
	//m_terrainValuesCanBeCleared = false;
	m_needUploadToServer = false;
	m_internalDataLocked = false;

	m_mapModder = memnew(GDN_TheWorld_MapModder);
	bool mapModderWithDebug = false;
	m_mapModder->custom_ready(m_viewer, m_viewer, mapModderWithDebug);
	godot::Error e = m_mapModder->connect("tw_modder_work_done", Callable(m_viewer, "quadrant_modder_work_done"));

	m_quadrantCenter = godot::Vector3(m_quadrantPos.getLowerXGridVertex(), 0, m_quadrantPos.getLowerZGridVertex()) + godot::Vector3(m_viewer->Globals()->heightmapSizeInWUs(), 0, m_viewer->Globals()->heightmapSizeInWUs()) / 2;

	m_textureLayout = ShaderTerrainData::layoutFromQuadrantPos(m_quadrantPos, m_continentName);
}

Quadrant::~Quadrant()
{
	if (m_mapModder != nullptr)
	{
		Node* parent = m_mapModder->get_parent();
		if (parent != nullptr)
			parent->call_deferred("remove_child", m_mapModder);		// Mah
			//parent->remove_child(m_mapModder);
		m_mapModder->queue_free();
		m_mapModder = nullptr;
	}

	//TheWorld_Viewer_Utils::TimerMs clock;
	//clock.tick();
	m_shaderTerrainData.reset();
	m_collider.reset();
	m_terrainEdit.reset();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed1 += clock.duration().count();
}

bool Quadrant::isFlushed(void)
{
	if (m_quadTree->statusUninitialized())
		return false;
	
	if (getFloat16HeightsBuffer(false).size() == 0 || getFloat32HeightsBuffer(false).size() == 0)
		return true;
	else
		return false;
}

bool Quadrant::hasHeigthsForChunkSplitsAndShaderRendering(void)
{
	if (getFloat16HeightsBuffer(false).size() == 0)
		return false;
	else
		return true;
}

bool Quadrant::needReloadTerrainDataFromCache(void)
{
	if (m_quadTree->isVisible() && m_quadTree->statusInitialized() && canCacheHeigthBuffers() && !hasHeigthsForChunkSplitsAndShaderRendering())
		return true;
	else
		return false;
}

void Quadrant::lockInternalDataUntilCompleteProcessExecution(bool b)
{
	m_lockInternalDataUntilCompleteProcessExecution = b;
}

bool Quadrant::internalDataLockedUntilCompleteProcessExecution(void) const
{
	return m_lockInternalDataUntilCompleteProcessExecution;
}

bool Quadrant::quadrantAtMinLod(enum class QuadType type)
{
	Quad* root = m_quadTree->getQuadRoot(type);
	if (root == nullptr)
		return true;

	if (root->isLeaf())
		return true;
	else
		return false;
}

//enum class Quadrant::MeshResolutionLevel Quadrant::getMeshResolutionLevel(void)
//{
//	Quad* root = m_quadTree->getRoot();
//	if (root == nullptr)
//		return MeshResolutionLevel::not_set;
//
//	if (root->isLeaf())
//		return MeshResolutionLevel::low;
//	else
//		return MeshResolutionLevel::high;
//}

void Quadrant::setHeightsUpdated(bool b)
{
	m_heightsUpdated = b;
	if (b)
	{
		getShaderTerrainData()->getHeightmapImageRef().unref();
		getShaderTerrainData()->getResizedHeightmapImageMap().clear();
		setSplatmapUpdated(true);
		setTextureUpdated(true);
	}
}

void Quadrant::setNormalsUpdated(bool b)
{
	if (b)
	{
		m_normalsUpdated = b;
		getShaderTerrainData()->getNormalmapImageRef().unref();
		getShaderTerrainData()->getResizedNormalmapImageMap().clear();
	}
	else
		m_normalsUpdated = b;
}

void Quadrant::setSplatmapUpdated(bool b)
{
	m_splatmapUpdated = b;
	if (b)
	{
		getShaderTerrainData()->getSplatmapImageRef().unref();
		getShaderTerrainData()->getResizedSplatmapImageMap().clear();
	}
}

void Quadrant::setColorsUpdated(bool b)
{
	m_colorsUpdated = b;
	if (b)
	{
		getShaderTerrainData()->getColormapImageRef().unref();
		getShaderTerrainData()->getResizedColormapImageMap().clear();
	}
}

void Quadrant::setGlobalmapUpdated(bool b)
{
	m_globalmapUpdated = b;
	if (b)
	{
		getShaderTerrainData()->getGlobalmapImageRef().unref();
		getShaderTerrainData()->getResizedGlobalmapImageMap().clear();
	}
}

//enum class Quadrant::TexResolutionLevel Quadrant::getTexResolutionLevel(void)
//{
//	size_t optLevel = optimizationLevel();
//	if (optLevel == 0)
//		return Quadrant::TexResolutionLevel::high;
//	else if (optLevel > 0 && optLevel  < 4)		// 1, 2 ,3
//		return Quadrant::TexResolutionLevel::med;
//	else
//		return Quadrant::TexResolutionLevel::low;
//}

//void Quadrant::clearCachedTexture(void)
//{
//	getShaderTerrainData()->clearCachedTexture();
//}
	
bool Quadrant::empty(void)
{
	//return m_empty;
	std::string meshid = m_cache.getMeshId();
	if (meshid.size() == 0)
		return true;

	if (TheWorld_Utils::MeshCacheBuffer::isEmptyBuffer(meshid))
	{
		return true;
	}
	else
	{
		return false;
	}
}

void Quadrant::releaseTerrainValuesMemory(bool visible)
{
	//return;

	bool manualSave;
	if (!m_quadTree->isValid() || needUploadToServer() || needSaveToCache(manualSave) || internalDataLockedUntilCompleteProcessExecution())
		return;

	if (m_quadTree->materialParamsNeedReset() || m_quadTree->materialParamsNeedUpdate() || m_quadTree->colliderNeedReset(QuadType::Terrain) || terrainDataJustLoaded())
		return;

	// i buffers durante il caricamento devono essere distrutti appena lavorati perché fanno esaurire la memoria di sistema rallentando notevolmente il caricamento
	if (canCacheHeigthBuffers())	// set according to camera distance from camera: heights can be cached if distance is under a threshold value because it is supposed splits will be necessary
	{
		//manualSave = true;	// DEBUG ONLY
	}
	else
	{
		// Needed to create/recreate imeages: at init are created asincronously but they have to be recreated in process to update the shader when we pass from optimizatrion level > 0 (using cached resized images) to optimization level = 0 (we need full textures)
		if (m_float16HeigthsBuffer.size() > 0)		// for split (calc AABB): if they are need in split operation they wiil be reloaded from disk cache before split
			m_float16HeigthsBuffer.clear(__FUNCTION__);

		//if (m_float32HeigthsBuffer.size() > 0)		// needed for collider but releaseTerrainValuesMemory will run after collider
		//	m_float32HeigthsBuffer.clear(__FUNCTION__);
		//if (m_normalsBuffer.size() > 0)
		//	m_normalsBuffer.clear(__FUNCTION__);
		//if (m_splatmapBuffer.size() > 0)
		//	m_splatmapBuffer.clear(__FUNCTION__);
		//if (m_colormapBuffer.size() > 0)
		//	m_colormapBuffer.clear(__FUNCTION__);
		//if (m_globalmapBuffer.size() > 0)
		//	m_globalmapBuffer.clear(__FUNCTION__);

		//// Needed for Shader: at init time or when optimization level changes from level > 0 to level == 0 because for level > 0 we use resized imaages which are cached for all the lifecycle of the quadrant)
		//getShaderTerrainData()->getHeightmapImageRef().unref();
		//getShaderTerrainData()->getNormalmapImageRef().unref();
		//getShaderTerrainData()->getSplatmapImageRef().unref();
		//getShaderTerrainData()->getColormapImageRef().unref();
		//getShaderTerrainData()->getGlobalmapImageRef().unref();
	}

	if (m_float32HeigthsBuffer.size() > 0)		// needed for collider but releaseTerrainValuesMemory will run after collider
		m_float32HeigthsBuffer.clear(__FUNCTION__);
	if (m_normalsBuffer.size() > 0)
		m_normalsBuffer.clear(__FUNCTION__);
	if (m_splatmapBuffer.size() > 0)
		m_splatmapBuffer.clear(__FUNCTION__);
	if (m_colormapBuffer.size() > 0)
		m_colormapBuffer.clear(__FUNCTION__);
	if (m_globalmapBuffer.size() > 0)
		m_globalmapBuffer.clear(__FUNCTION__);

	// Needed for Shader: at init time or when optimization level changes from level > 0 to level == 0 because for level > 0 we use resized imaages which are cached for all the lifecycle of the quadrant)
	getShaderTerrainData()->getHeightmapImageRef().unref();
	getShaderTerrainData()->getNormalmapImageRef().unref();
	getShaderTerrainData()->getSplatmapImageRef().unref();
	getShaderTerrainData()->getColormapImageRef().unref();
	getShaderTerrainData()->getGlobalmapImageRef().unref();

	int64_t size = m_heightsForCollider.size();
	m_heightsForCollider.resize(0);
}

size_t Quadrant::calcMapMemoryOccupation(void)
{
	size_t occupation = m_float16HeigthsBuffer.reserved() + m_float32HeigthsBuffer.reserved() + m_normalsBuffer.reserved() + m_splatmapBuffer.reserved() + m_colormapBuffer.reserved() + m_globalmapBuffer.reserved() + (m_terrainEdit == nullptr ? 0 : m_terrainEdit->size) + m_heightsForCollider.size() * sizeof(float);
	return occupation;
}

size_t Quadrant::calcHeightmap16MemoryOccupation(void)
{
	return m_float16HeigthsBuffer.reserved();
}

size_t Quadrant::calcHeightmap32MemoryOccupation(void)
{
	return m_float32HeigthsBuffer.reserved();
}

size_t Quadrant::calcNormalmapMemoryOccupation(void)
{
	return m_normalsBuffer.reserved();
}

size_t Quadrant::calcSplatmapMemoryOccupation(void)
{
	return m_splatmapBuffer.reserved();
}

size_t Quadrant::calcGlobalmapMemoryOccupation(void)
{
	return m_globalmapBuffer.reserved();
}

size_t Quadrant::calcColormapMemoryOccupation(void)
{
	return m_colormapBuffer.reserved();
}

size_t Quadrant::calcTerrainEditMemoryOccupation(void)
{
	return (m_terrainEdit == nullptr ? 0 : m_terrainEdit->size);
}

size_t Quadrant::calcColliderHeightsMemoryOccupation(void)
{
	return m_heightsForCollider.size() * sizeof(real_t);
}

size_t Quadrant::calcTexMemoryOccupation(void)
{
	return m_shaderTerrainData->calcTexMemoryOccupation();
}

size_t Quadrant::calcResizedTexMemoryOccupation(void)
{
	return m_shaderTerrainData->calcResizedTexMemoryOccupation();
}

void Quadrant::checkInternalData(double _delta)
{
	//if (!empty() && getMeshId().length() > 0 && getMeshCacheBuffer().getMeshId().length() > 0 && getMeshId() != getMeshCacheBuffer().getMeshId())
	//{
	//	PLOG_DEBUG << "WARNING: quadrant meshId [" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(getMeshId()) << "] not equal cache meshId [" << TheWorld_Utils::MeshCacheBuffer::meshIdToPrint1(getMeshCacheBuffer().getMeshId()) << "]";
	//}
}

void Quadrant::setBlendNeeded(bool blendNeeded, bool northSideBlendNeeded, bool southSideBlendNeeded, bool eastSideBlendNeeded, bool westSideBlendNeeded)
{
	if (m_blendNeeded != blendNeeded or m_northSideBlendNeeded != northSideBlendNeeded or m_southSideBlendNeeded != southSideBlendNeeded or m_eastSideBlendNeeded != eastSideBlendNeeded or m_westSideBlendNeeded != westSideBlendNeeded)
	{
		m_blendNeeded = blendNeeded;
		m_northSideBlendNeeded = northSideBlendNeeded;
		m_southSideBlendNeeded = southSideBlendNeeded;
		m_eastSideBlendNeeded = eastSideBlendNeeded;
		m_westSideBlendNeeded = westSideBlendNeeded;

		{
			godot::Ref<godot::ShaderMaterial> mat = getShaderTerrainData()->getRegularTerrainMaterial();
			mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING, m_blendNeeded);
			mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_NORTH, m_northSideBlendNeeded);
			mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_SOUTH, m_southSideBlendNeeded);
			mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_EAST, m_eastSideBlendNeeded);
			mat->set_shader_parameter(SHADER_PARAM_NEED_BLENDING_WEST, m_westSideBlendNeeded);
		}

		{
			GDN_TheWorld_MapViewer* mapViewer = m_viewer->MapViewer();
			if (mapViewer != nullptr)
			{
				GDN_TheWorld_Map* map = mapViewer->getMap();
				if (map != nullptr)
				{
					GDN_TheWorld_Map::MapQuadrantOverride* mapQuadrantOverride = map->getMapQuadrantOverride(getInternalPos());
					if (mapQuadrantOverride != nullptr)
					{
						if (mapQuadrantOverride->blendNeeded() != m_blendNeeded)
						{
							mapQuadrantOverride->blendNeeded(m_blendNeeded);
							mapQuadrantOverride->applyColor();
						}
					}
				}
			}
		}
	}
}

void Quadrant::process_checkBlendNeeded(double _delta)
{
	if (!m_viewer->canProcessWorldDataUpdate())
	{
		setBlendNeeded(false, false, false, false, false);
		return;
	}

	if (internalDataLocked())
		return;

	if (!m_quadTree->isValid())
		return;

	if (m_quadTree->getQuadrant()->empty())
		return;

	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl == nullptr or not editModeUIControl->initialized() or editModeUIControl->terrainDataLocked())
	{
		return;
	}

	if (not editModeUIControl->checkBlend())
	{
		setBlendNeeded(false, false, false, false, false);
		return;
	}

	editModeUIControl->terrainDataLocked(true);

	{
		TheWorld_Utils::QuadrantPos currentPos = getPos();
		TheWorld_Utils::QuadrantPos pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
		QuadTree* northQuadrantTree = m_viewer->getQuadTree(pos, true, false, true);
		pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
		QuadTree* southQuadrantTree = m_viewer->getQuadTree(pos, true, false, true);
		pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
		QuadTree* eastQuadrantTree = m_viewer->getQuadTree(pos, true, false, true);
		pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
		QuadTree* westQuadrantTree = m_viewer->getQuadTree(pos, true, false, true);

		bool northSideBlendNeeded = false;
		bool southSideBlendNeeded = false;
		bool eastSideBlendNeeded = false;
		bool westSideBlendNeeded = false;

		//if (currentPos.getIndexPosX() == -14 and currentPos.getIndexPosZ() == 24)	// #####
		//	assert(northSideBlendNeeded == false);									// #####

		bool blendNeeded = checkBlendNeeded(northSideBlendNeeded, southSideBlendNeeded, eastSideBlendNeeded, westSideBlendNeeded, northQuadrantTree, southQuadrantTree, eastQuadrantTree, westQuadrantTree);
	}

	editModeUIControl->terrainDataLocked(false);
}

bool Quadrant::mapModdingRequired(void)
{
	if (!m_quadTree->isValid())
		return false;

	if (m_quadTree->getQuadrant()->empty())
		return false;

	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();

	if (terrainEdit->normalsNeedRegen or terrainEdit->splatmapNeedRegen() or terrainEdit->globalmapNeedRegen)
		return true;

	return false;
}

void Quadrant::process_mapModder(double _delta, bool worldQuadrant)
{
	if (m_mapModder == nullptr)
		return;

	if (m_mapModder->startProcessRequired())
	{
		m_mapModder->startProcessing();
	}

	if (m_mapModder->deinitRequired())
	{
		m_mapModder->deinit();
	}

	m_mapModder->debug_process(_delta);

	//if (GDN_TheWorld_MapModder::getNumWorking() > 0)	// DEBUG
	//	return;											// DEBUG

	if (GDN_TheWorld_MapModder::getNumWorking() > 10)
		return;

	if (internalDataLocked())
		return;

	if (worldQuadrant and m_quadTree->materialParamsNeedReset())
		return;

	//if (m_quadTree->currentStatus() != QuadrantStatus::initialized)
	//	return;

	if (!m_quadTree->isValid())
		return;

	if (m_quadTree->getQuadrant()->empty())
	{
		mapModderWorkDone(true);
		//{	// DEBUG
		//	if (m_quadTree->getQuadrant()->getPos().getIndexPosX() == -2 && m_quadTree->getQuadrant()->getPos().getIndexPosZ() == 3)
		//	{
		//		PLOG_DEBUG << "TRACKING process_mapModder exit empty " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << m_quadTree->getQuadrant()->getPos().getIndexPosX() << "Z=" << m_quadTree->getQuadrant()->getPos().getIndexPosZ();
		//	}
		//	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();
		//	if (terrainEdit->heightsNeedRegen)
		//		terrainEdit->heightsNeedRegen = true;
		//	else
		//		terrainEdit->heightsNeedRegen = false;
		//}	// DEBUG
		return;
	}
	
	if (!m_mapModder->ready())
		return;

	//if (m_quadTree->getQuadrant()->getPos().getIndexPosX() == -2 && m_quadTree->getQuadrant()->getPos().getIndexPosZ() == 3)
	//{
	//	PLOG_DEBUG << "TRACKING process_mapModder " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << m_quadTree->getQuadrant()->getPos().getIndexPosX() << "Z=" << m_quadTree->getQuadrant()->getPos().getIndexPosZ();
	//}

	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl == nullptr or not editModeUIControl->initialized() or editModeUIControl->terrainDataLocked())
	{
		return;
	}
	editModeUIControl->terrainDataLocked(true);

	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();

	if (terrainEdit->heightsNeedRegen)
	{
		editModeUIControl->terrainDataLocked(false);
		return;
	}
		
	if (terrainEdit->normalsNeedRegen)
	{
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForNormals(m_quadTree, &getFloat32HeightsBuffer(), /*1024*/ getPos().getNumVerticesPerSize(), 0, true);		// atm border = 0 will be the same
		}
		editModeUIControl->terrainDataLocked(false);
		return;
	}

	if (terrainEdit->splatmapNeedRegen())
	{
		//if (!terrainEdit->splatmapNeedRegen)	// DEBUG ONLY
		//	bool b = true;						// DEBUG ONLY
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForSplatmap(m_quadTree, &getFloat32HeightsBuffer(), &getNormalsBuffer(), terrainEdit->extraValues.slopeVerticalFactor, terrainEdit->extraValues.slopeFlatFactor, terrainEdit->extraValues.dirtOnRocksFactor, terrainEdit->extraValues.highElevationFactor, terrainEdit->extraValues.lowElevationFactor, terrainEdit->minHeight, terrainEdit->maxHeight, /*1024*/ getPos().getNumVerticesPerSize(), 0, true);
		}
		editModeUIControl->terrainDataLocked(false);
		return;
	}

	//if (terrainEdit->extraValues.colormapNeedRegen)
	//{
	//	// TODO
	//}

	if (terrainEdit->globalmapNeedRegen)
	{
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForGlobalMap(m_quadTree, getPos().getNumVerticesPerSize() / 4, true);
		}
		editModeUIControl->terrainDataLocked(false);
		return;
	}

	bool manual = true;
	if (editModeUIControl->isQuadToSave(m_quadrantPos, manual))
	{
		if (!manual)
		{
			bool lockAcquired = lockInternalData();
			if (lockAcquired)
			{
				saveQuadrantToDiskCache2(true, false);
				editModeUIControl->removeQuadToSave(m_quadrantPos);
				terrainEdit->needUploadToServer = false;
				std::string buffer;
				getQuadrantBufferForCache(buffer);
				m_viewer->Globals()->Client()->MapManagerUploadBuffer(m_viewer->isInEditor(), m_quadrantPos.getLowerXGridVertex(), m_quadrantPos.getLowerZGridVertex(), m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_quadrantPos.getLevel(), buffer, m_viewer->getNumQuadrantsPerSize(), m_viewer->getQuadrantGlobalSize(), m_viewer->getQuadrantWorldSize());
				unlockInternalData();
				editModeUIControl->terrainDataLocked(false);
				return;
			}
		}
	}

	//if (m_quadTree->getQuadrant()->getPos().getIndexPosX() == -2 && m_quadTree->getQuadrant()->getPos().getIndexPosZ() == 3)
	//{
	//	PLOG_DEBUG << "TRACKING process_mapModder exit " << THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES << " X=" << m_quadTree->getQuadrant()->getPos().getIndexPosX() << "Z=" << m_quadTree->getQuadrant()->getPos().getIndexPosZ();
	//}

	mapModderWorkDone(true);

	editModeUIControl->terrainDataLocked(false);
}

bool Quadrant::process_blend(void)
{
	bool processed = false;

	if (blendNeeded())
	{
		if ((northSideBlendNeeded() and northQuadrantValid())		// occhio!!!
			or (southSideBlendNeeded() and southQuadrantValid())
			or (eastSideBlendNeeded() and eastQuadrantValid())
			or (westSideBlendNeeded() and westQuadrantValid()))
		{
			GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
			if (editModeUIControl == nullptr or not editModeUIControl->initialized() or editModeUIControl->terrainDataLocked())
			{
				return processed;
			}
			bool quadrantBusy = false;
			processed = editModeUIControl->blendAction(getPos(), false, true, true, true, true, quadrantBusy, false);
		}
	}
	
	return processed;
}

TheWorld_Utils::MeshCacheBuffer& Quadrant::getInternalMeshCacheBuffer(void)
{
	return m_cache;
}

size_t Quadrant::getIndexFromMap(float posX, float posZ)
{
	if (posX < m_quadrantPos.getLowerXGridVertex() || posX > m_quadrantPos.getLowerXGridVertex() + m_quadrantPos.getSizeInWU()
		|| posZ < m_quadrantPos.getLowerZGridVertex() || posZ > m_quadrantPos.getLowerZGridVertex() + m_quadrantPos.getSizeInWU())
		return -1;

	size_t numStepsXAxis = size_t((posX - m_quadrantPos.getLowerXGridVertex()) / m_quadrantPos.getGridStepInWU());
	size_t numStepsZAxis = size_t((posZ - m_quadrantPos.getLowerZGridVertex()) / m_quadrantPos.getGridStepInWU());

	return numStepsZAxis * m_quadrantPos.getNumVerticesPerSize() + numStepsXAxis;
}

float Quadrant::getAltitudeFromHeigthmap(size_t index)
{
	//float* p = (float*)getFloat32HeightsBuffer().ptr();
	//return *(p + index);
	
	uint16_t* p = (uint16_t*)getFloat16HeightsBuffer().ptr();
	TheWorld_Utils::FLOAT_32 f(0);
	f.u32 = half_to_float(*(p + index));
	return f.f32;
}

float Quadrant::getPosXFromHeigthmap(size_t index)
{
	size_t numStepsXAxis = index - (index / m_quadrantPos.getNumVerticesPerSize()) * m_quadrantPos.getNumVerticesPerSize();
	return m_quadrantPos.getLowerXGridVertex() + numStepsXAxis * m_quadrantPos.getGridStepInWU();
}

float Quadrant::getPosZFromHeigthmap(size_t index)
{
	size_t numStepsZAxis = size_t(index / m_quadrantPos.getNumVerticesPerSize());
	return m_quadrantPos.getLowerZGridVertex() + numStepsZAxis * m_quadrantPos.getGridStepInWU();
}

void Quadrant::getQuadrantBufferForCache(std::string& buffer)
{
	TheWorld_Utils::MemoryBuffer _buffer;
	getQuadrantBufferForCache(_buffer);
	buffer.clear();
	buffer.reserve(_buffer.size());
	buffer.assign((char*)_buffer.ptr(), _buffer.size());
}

void Quadrant::getQuadrantBufferForCache(TheWorld_Utils::MemoryBuffer& buffer)
{
	TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
	cacheQuadrantData.meshId = m_cache.getMeshId();
	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();
	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
	cacheQuadrantData.minHeight = terrainEdit->minHeight;
	cacheQuadrantData.maxHeight = terrainEdit->maxHeight;
	cacheQuadrantData.terrainEditValues = &terrainEditValuesBuffer;
	cacheQuadrantData.heights16Buffer = &getFloat16HeightsBuffer(false);
	cacheQuadrantData.heights32Buffer = &getFloat32HeightsBuffer(false);
	cacheQuadrantData.normalsBuffer = &getNormalsBuffer(false);
	cacheQuadrantData.splatmapBuffer = &getSplatmapBuffer(false);
	cacheQuadrantData.colormapBuffer = &getColormapBuffer(false);
	cacheQuadrantData.globalmapBuffer = &getGlobalmapBuffer(false);
	TheWorld_Utils::QuadrantPos pos = getPos();
	m_cache.setBufferFromCacheQuadrantData(pos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
}

bool Quadrant::saveQuadrantToDiskCache2(std::string& buffer, bool needUploadToserver, bool newMeshId, bool lock)
{
	TheWorld_Utils::MemoryBuffer _buffer;
	bool saved = saveQuadrantToDiskCache2(_buffer, needUploadToserver, newMeshId, lock);
	if (saved)
	{
		buffer.clear();
		buffer = std::string((char*)_buffer.ptr(), _buffer.size());
	}
	return saved;
}

bool Quadrant::saveQuadrantToDiskCache2(TheWorld_Utils::MemoryBuffer& buffer, bool needUploadToserver, bool newMeshId, bool lock)
{
	bool locked = true;
	if (lock)
		locked = lockInternalData();
	if (locked)
	{
		setNeedUploadToServer(needUploadToserver);

		if (newMeshId)
		{
			m_cache.generateNewInternalMeshId();
			setMeshId(m_cache.getMeshId());
		}

		TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
		cacheQuadrantData.meshId = m_cache.getMeshId();
		TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();
		terrainEdit->needUploadToServer = needUploadToserver;
		TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer((BYTE*)terrainEdit, terrainEdit->size);
		cacheQuadrantData.minHeight = terrainEdit->minHeight;
		cacheQuadrantData.maxHeight = terrainEdit->maxHeight;
		cacheQuadrantData.terrainEditValues = &terrainEditValuesBuffer;
		cacheQuadrantData.heights16Buffer = &getFloat16HeightsBuffer(true);
		cacheQuadrantData.heights32Buffer = &getFloat32HeightsBuffer(true);
		cacheQuadrantData.normalsBuffer = &getNormalsBuffer(true);
		cacheQuadrantData.splatmapBuffer = &getSplatmapBuffer(true);
		cacheQuadrantData.colormapBuffer = &getColormapBuffer(true);
		cacheQuadrantData.globalmapBuffer = &getGlobalmapBuffer(true);
		TheWorld_Utils::QuadrantPos pos = getPos();
		m_cache.setBufferFromCacheQuadrantData(pos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
		std::string writtenMeshId;
		m_cache.writeBufferToDiskCache(buffer, writtenMeshId);
		m_quadTree->autoSaveRequired(true);

		if (lock)
			unlockInternalData();

		return true;
	}
	else
		return false;
}

bool Quadrant::saveQuadrantToDiskCache2(bool newMeshId, bool lock)
{
	TheWorld_Utils::MemoryBuffer buffer;
	return saveQuadrantToDiskCache2(buffer, true, newMeshId, lock);
}

godot::AABB Quadrant::getGlobalAABB(void)
{
	godot::AABB aabb;

	if (m_quadTree->getQuadrant()->getTerrainEdit(false) != nullptr)
	{
		float minHeight = m_quadTree->getQuadrant()->getTerrainEdit(false)->minHeight;
		float maxHeight = m_quadTree->getQuadrant()->getTerrainEdit(false)->maxHeight;
		godot::Vector3 startPosition = godot::Vector3(m_quadTree->getQuadrant()->getPos().getLowerXGridVertex(), minHeight, m_quadTree->getQuadrant()->getPos().getLowerZGridVertex());
		godot::Vector3 endPosition = godot::Vector3(m_quadTree->getQuadrant()->getPos().getLowerXGridVertex() + m_quadTree->getQuadrant()->getPos().getSizeInWU(), maxHeight, m_quadTree->getQuadrant()->getPos().getLowerZGridVertex() + m_quadTree->getQuadrant()->getPos().getSizeInWU());
		godot::Vector3 size = endPosition - startPosition;
		aabb.set_position(startPosition);
		aabb.set_size(size);
	}

	return aabb;
}

bool Quadrant::needSaveToCache(bool& manual)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant::needSaveToCache ") + __FUNCTION__, "needSaveToCache");

	bool needToSave = false;
	manual = true;

	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl != nullptr)
	{
		//editModeUIControl->eraseQuadToSave(pos);
		TheWorld_Utils::QuadrantPos pos = getPos();
		if (editModeUIControl->isQuadToSave(pos, manual))
			needToSave = true;
	}

	return needToSave;
}

bool Quadrant::lockInternalData(void)
{
	bool lockAcquired = false;
	
	{
		std::lock_guard<std::recursive_mutex> lock(m_mtxInternalData);
		if (!m_internalDataLocked)
		{
			m_internalDataLocked = true;
			m_internalDataLockingTid = GetCurrentThreadId();
			lockAcquired = true;
		}
	}

	return lockAcquired;
}

void Quadrant::unlockInternalData(void)
{
	std::lock_guard<std::recursive_mutex> lock(m_mtxInternalData);
	if (m_internalDataLocked)
	{
		m_internalDataLocked = false;
	}
	else
	{
		m_internalDataLocked = false;
	}
	m_internalDataLockingTid = 0;
}

void Quadrant::renderGlobalMapChanged(bool b)
{
	m_renderGlobalMapChanged = b;
	if (m_renderGlobalMapChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant renderGlobalMapChanged 1 ") + __FUNCTION__, "ALL");
		m_quadTree->materialParamsNeedReset(true);
	}
}

void Quadrant::optimizationLevel(size_t level)
{
	if (m_optimizationLevel != level)
	{
		m_optimizationLevel = level;
		optimizationLevelChanged(true);
		m_quadTree->materialParamsNeedReset(true);
	}
}

void Quadrant::optimizationLevelChanged(bool b)
{
	m_optimizationLevelChanged = b;
	if (m_optimizationLevelChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("Quadrant optimizationLevelChanged 1 ") + __FUNCTION__, "ALL");
		m_quadTree->materialParamsNeedReset(true);
		m_viewer->incNumOptimizationLevelChange();
	}
}

void Quadrant::loadTerrainDataFromCache(void)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1 ") + __FUNCTION__, "ALL");

	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	//PLOG_DEBUG << "START loadTerrainDataFromCache";

	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	int resolution = globals->heightmapResolution() + 1;

	enum class TheWorldStatus status = globals->status();
	if (status > TheWorldStatus::worldDeployed)
		return;

	while (lockInternalData() == false)
		Sleep(1);

	m_quadTree->setStatus(QuadrantStatus::loadTerrainDataFromCacheInProgress);

	float minAltitude, maxAltitude;
	TheWorld_Utils::MemoryBuffer terrainEditValues;
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.1 ") + __FUNCTION__, "m_cache.refreshMapsFromDisk");
		bool empty = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), m_meshId, terrainEditValues, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
		//if (terrainEditValues.size() > 0)
		//	getTerrainEdit()->deserialize(terrainEditValues);
		//if (empty)
		//	setEmpty(true);
		//else
		//	setEmpty(false);
	}

	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.2 ") + __FUNCTION__, "Cache heightmap images");

		std::string profileHeader("loadTerrainDataFromCache 1.2.1 ");
		getShaderTerrainData()->getHeightmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("loadTerrainDataFromCache 1.2.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, optimizationLevel, cachedTextureResolution);
		}
	}

	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.3 ") + __FUNCTION__, "Cache normalmap images");

		std::string profileHeader("loadTerrainDataFromCache 1.3.1 ");
		getShaderTerrainData()->getNormalmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::normalmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("loadTerrainDataFromCache 1.3.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::normalmap, optimizationLevel, cachedTextureResolution);
		}
	}

	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.4  ") + __FUNCTION__, "Cache splatmap images");

		std::string profileHeader("loadTerrainDataFromCache 1.4.1 ");
		getShaderTerrainData()->getSplatmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::splatmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("loadTerrainDataFromCache 1.4.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::splatmap, optimizationLevel, cachedTextureResolution);
		}
	}

	if (m_globalmapBuffer.size() > 0)
	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.5  ") + __FUNCTION__, "Cache globalmap images");

		std::string profileHeader("loadTerrainDataFromCache 1.5.1 ");
		getShaderTerrainData()->getGlobalmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::globalmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("loadTerrainDataFromCache 1.5.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::globalmap, optimizationLevel, cachedTextureResolution);
		}
	}

	if (m_colormapBuffer.size() > 0)
	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.6  ") + __FUNCTION__, "Cache colormap images");

		std::string profileHeader("loadTerrainDataFromCache 1.6.1 ");
		getShaderTerrainData()->getColormapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::colormap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("loadTerrainDataFromCache 1.6.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::colormap, optimizationLevel, cachedTextureResolution);
		}
	}

	size_t numFloat32Heights = m_float32HeigthsBuffer.size() / sizeof(float);

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("loadTerrainDataFromCache 1.7 ") + __FUNCTION__, "SetColliderHeights");

		m_heightsForCollider.resize((int)numFloat32Heights);
#ifdef REAL_T_IS_DOUBLE
		float* f = (float*)m_float32HeigthsBuffer.ptr();
		for (int idx = 0; idx < numFloat32Heights; idx++)
		{
			m_heightsForCollider.set(idx, (real_t)*f);
			f++;
		}
#else
		memcpy((char*)m_heightsForCollider.ptrw(), m_float32HeigthsBuffer.ptr(), m_float32HeigthsBuffer.size());
#endif
	}

	m_viewer->getMainProcessingMutex().lock();
	m_quadTree->setStatus(QuadrantStatus::initialized);
	m_quadTree->getQuadrant()->terrainDataJustLoaded(true);
	unlockInternalData();
	m_viewer->getMainProcessingMutex().unlock();
	
	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
	m_viewer->incNumLoadTerrainData((now - start).count());

	//PLOG_DEBUG << "END loadTerrainDataFromCache";
}

bool Quadrant::canHaveHiResCollisionInfo(void)
{
	//GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	//if (editModeUIControl != nullptr && editModeUIControl->initialized())
	//{
	//	if (editModeUIControl->hiResCollisionGridEnabled())
	//		return true;
	//	if (editModeUIControl->lowResCollisionGridEnabled())
	//		return false;
	//}

	bool colliderCanHaveHiResCollisionInfo = true;

	//int64_t depthForCollider = m_viewer->Globals()->getParamDepthQuadWithCollisionInfo();
	//if (depthForCollider != 0 && getLastDistanceInPerimeterFromCameraQuadrant() > (size_t)depthForCollider)
	//	colliderCanHaveHiResCollisionInfo = false;
	
	if (quadrantAtMinLod(QuadType::Terrain) && !m_viewer->Globals()->getParamForceHiResCollisionInfo())
		colliderCanHaveHiResCollisionInfo = false;
	
	return colliderCanHaveHiResCollisionInfo;
}

void Quadrant::updateTerrainEditTextureLayers(void)
{
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();

	if (northQuadrantData().initialized())
	{
		godot::Vector4 layers = northQuadrantData().groundTextureLayers();
		std::string lowElevationTexName = groundTextures.getTextureName((int32_t)layers.x);
		std::string highElevationTexName = groundTextures.getTextureName((int32_t)layers.y);
		std::string dirtTexName = groundTextures.getTextureName((int32_t)layers.z);
		std::string rocksTexName = groundTextures.getTextureName((int32_t)layers.w);
		terrainData->setNorthGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);
	}

	if (westQuadrantData().initialized())
	{
		godot::Vector4 layers = westQuadrantData().groundTextureLayers();
		std::string lowElevationTexName = groundTextures.getTextureName((int32_t)layers.x);
		std::string highElevationTexName = groundTextures.getTextureName((int32_t)layers.y);
		std::string dirtTexName = groundTextures.getTextureName((int32_t)layers.z);
		std::string rocksTexName = groundTextures.getTextureName((int32_t)layers.w);
		terrainData->setWestGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);
	}

	if (northWestQuadrantData().initialized())
	{
		godot::Vector4 layers = northWestQuadrantData().groundTextureLayers();
		std::string lowElevationTexName = groundTextures.getTextureName((int32_t)layers.x);
		std::string highElevationTexName = groundTextures.getTextureName((int32_t)layers.y);
		std::string dirtTexName = groundTextures.getTextureName((int32_t)layers.z);
		std::string rocksTexName = groundTextures.getTextureName((int32_t)layers.w);
		terrainData->setNorthwestGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);
	}
}

godot::Vector4 Quadrant::northQuadrantTextureLayersFromTerrainData(void)
{
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	std::string lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName;
	terrainData->getNorthGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);

	TheWorld_Utils::QuadrantPos northQuadrantPos = getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
	std::string northQuadrantTextureLayout = ShaderTerrainData::layoutFromQuadrantPos(northQuadrantPos);

	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string layoutForFoundTexture;
	godot::Vector4 v(-1, -1, -1, -1);
	v.x = (float)groundTextures.getTextureIdx(northQuadrantTextureLayout, lowElevationTexName, layoutForFoundTexture);
	v.y = (float)groundTextures.getTextureIdx(northQuadrantTextureLayout, highElevationTexName, layoutForFoundTexture);
	v.z = (float)groundTextures.getTextureIdx(northQuadrantTextureLayout, dirtTexName, layoutForFoundTexture);
	v.w = (float)groundTextures.getTextureIdx(northQuadrantTextureLayout, rocksTexName, layoutForFoundTexture);

	return v;
}

godot::Vector4 Quadrant::westQuadrantTextureLayersFromTerrainData(void)
{
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	std::string lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName;
	terrainData->getWestGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);

	TheWorld_Utils::QuadrantPos westQuadrantPos = getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
	std::string westQuadrantTextureLayout = ShaderTerrainData::layoutFromQuadrantPos(westQuadrantPos);

	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string layoutForFoundTexture;
	godot::Vector4 v(-1, -1, -1, -1);
	v.x = (float)groundTextures.getTextureIdx(westQuadrantTextureLayout, lowElevationTexName, layoutForFoundTexture);
	v.y = (float)groundTextures.getTextureIdx(westQuadrantTextureLayout, highElevationTexName, layoutForFoundTexture);
	v.z = (float)groundTextures.getTextureIdx(westQuadrantTextureLayout, dirtTexName, layoutForFoundTexture);
	v.w = (float)groundTextures.getTextureIdx(westQuadrantTextureLayout, rocksTexName, layoutForFoundTexture);

	return v;
}

godot::Vector4 Quadrant::northwestQuadrantTextureLayersFromTerrainData(void)
{
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	std::string lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName;
	terrainData->getNorthwestGroundTextures(lowElevationTexName, highElevationTexName, dirtTexName, rocksTexName);

	TheWorld_Utils::QuadrantPos northwestQuadrantPos = getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
	std::string northwestQuadrantTextureLayout = ShaderTerrainData::layoutFromQuadrantPos(northwestQuadrantPos);

	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string layoutForFoundTexture;
	godot::Vector4 v(-1, -1, -1, -1);
	v.x = (float)groundTextures.getTextureIdx(northwestQuadrantTextureLayout, lowElevationTexName, layoutForFoundTexture);
	v.y = (float)groundTextures.getTextureIdx(northwestQuadrantTextureLayout, highElevationTexName, layoutForFoundTexture);
	v.z = (float)groundTextures.getTextureIdx(northwestQuadrantTextureLayout, dirtTexName, layoutForFoundTexture);
	v.w = (float)groundTextures.getTextureIdx(northwestQuadrantTextureLayout, rocksTexName, layoutForFoundTexture);

	return v;
}

void Quadrant::getSplatmapBorderRow(std::chrono::system_clock::time_point& lastBorderRefreshTs, TheWorld_Utils::MemoryBuffer& splatmapBorderRow, TheWorld_Utils::QuadrantPos::DirectionSlot requiredBorderDir)
{
	TheWorld_Utils::MemoryBuffer* requiredSplatmapBorderRow = nullptr;
	//std::chrono::system_clock::time_point m_lastBorderRefreshTs;

	switch (requiredBorderDir)
	{
		case TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus:
		{
			requiredSplatmapBorderRow = &m_splatmapNorthBorderRow;
		}
		break;
		case TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus:
		{
			requiredSplatmapBorderRow = &m_splatmapSouthBorderRow;
		}
		break;
		case TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus:
		{
			requiredSplatmapBorderRow = &m_splatmapWestBorderRow;
		}
		break;
		case TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus:
		{
			requiredSplatmapBorderRow = &m_splatmapEastBorderRow;
		}
		break;
	}

	if (requiredSplatmapBorderRow != nullptr)
	{
		if (TheWorld_Utils::null_time_point(m_lastBorderRefreshTs))
		{
			splatmapBorderRow.clear();
			lastBorderRefreshTs = m_lastBorderRefreshTs;
		}
		else
		{
			if (m_lastBorderRefreshTs > lastBorderRefreshTs)
			{
				splatmapBorderRow = *requiredSplatmapBorderRow;
				lastBorderRefreshTs = m_lastBorderRefreshTs;
			}
		}
	}
}

godot::Color Quadrant::getLowerRightCornerSplatmapTexel(void)
{
	TheWorld_Utils::MemoryBuffer& splatmapBuffer = getSplatmapBuffer(false);
	size_t numVerticesPerSize = m_quadrantPos.getNumVerticesPerSize();
	if (splatmapBuffer.size() != (numVerticesPerSize * numVerticesPerSize * sizeof(struct TheWorld_Utils::_RGBA)))
		return godot::Color(0.0, 0.0, 0.0, 0.0);
	
	struct TheWorld_Utils::_RGBA* rgba = splatmapBuffer.at_ptr<TheWorld_Utils::_RGBA>(numVerticesPerSize - 1, numVerticesPerSize - 1, numVerticesPerSize);
	godot::Color c(float(rgba->r) / 255, float(rgba->g) / 255, float(rgba->b) / 255, float(rgba->a) / 255);
	return c;
}

godot::Vector4 Quadrant::quadrantTextureLayers(void) const
{
	godot::Vector4 v(-1, -1, -1, -1);
	ShaderTerrainData::GroundTextures& groundTextures = ShaderTerrainData::getGroundTextures();
	std::string layoutForFoundTexture;
	v.x = (float)groundTextures.getTextureIdx(m_textureLayout, m_terrainEdit->extraValues.lowElevationTexName_r, layoutForFoundTexture);
	v.y = (float)groundTextures.getTextureIdx(m_textureLayout, m_terrainEdit->extraValues.highElevationTexName_g, layoutForFoundTexture);
	v.z = (float)groundTextures.getTextureIdx(m_textureLayout, m_terrainEdit->extraValues.dirtTexName_b, layoutForFoundTexture);
	v.w = (float)groundTextures.getTextureIdx(m_textureLayout, m_terrainEdit->extraValues.rocksTexName_a, layoutForFoundTexture);

	return v;
}

bool Quadrant::checkBlendNeeded(void)
{
	if (empty())
		return false;
	
	TheWorld_Utils::QuadrantPos currentPos = getPos();
	TheWorld_Utils::QuadrantPos _pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus);
	QuadTree* northQuadrantTree = m_viewer->getQuadTree(_pos, true, false, true);
	_pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus);
	QuadTree* southQuadrantTree = m_viewer->getQuadTree(_pos, true, false, true);
	_pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus);
	QuadTree* eastQuadrantTree = m_viewer->getQuadTree(_pos, true, false, true);
	_pos = currentPos.getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus);
	QuadTree* westQuadrantTree = m_viewer->getQuadTree(_pos, true, false, true);

	bool northSideBlendNeeded = false;
	bool southSideBlendNeeded = false;
	bool eastSideBlendNeeded = false;
	bool westSideBlendNeeded = false;

	//if (currentPos.getIndexPosX() == -16 and currentPos.getIndexPosZ() == 22)	// #####
	//	assert(northSideBlendNeeded == false);									// #####

	bool blendNeeded = checkBlendNeeded(northSideBlendNeeded, southSideBlendNeeded, eastSideBlendNeeded, westSideBlendNeeded, northQuadrantTree, southQuadrantTree, eastQuadrantTree, westQuadrantTree);
	return blendNeeded;
}

bool Quadrant::checkBlendNeeded(bool& northSideBlendNeeded, bool& southSideBlendNeeded, bool& eastSideBlendNeeded, bool& westSideBlendNeeded, QuadTree* northQuadrantTree, QuadTree* southQuadrantTree, QuadTree* eastQuadrantTree, QuadTree* westQuadrantTree)
{
	northSideBlendNeeded = Quadrant::checkNorthSideBlendNeeded(northQuadrantTree);
	southSideBlendNeeded = Quadrant::checkSouthSideBlendNeeded(southQuadrantTree);
	eastSideBlendNeeded = Quadrant::checkEastSideBlendNeeded(eastQuadrantTree);
	westSideBlendNeeded = Quadrant::checkWestSideBlendNeeded(westQuadrantTree);

	bool blendNeeded;

	if (northSideBlendNeeded or southSideBlendNeeded or eastSideBlendNeeded or westSideBlendNeeded)
		blendNeeded = true;
	else
		blendNeeded = false;

	setBlendNeeded(blendNeeded, northSideBlendNeeded, southSideBlendNeeded, eastSideBlendNeeded, westSideBlendNeeded);
	return blendNeeded;
}

bool Quadrant::checkNorthSideBlendNeeded(QuadTree* northQuadrantTree)
{
	m_northQuadrantValid = northQuadrantTree != nullptr and northQuadrantTree->isValid();

	if (getTerrainEdit()->northSideZMinus.needBlend)
		return true;

	if (not m_northQuadrantValid)
		return false;

	//return not getTerrainEdit()->northSideZMinus.almostEqual(northQuadrantTree->getQuadrant()->getTerrainEdit()->southSideZPlus);
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	TheWorld_Utils::TerrainEdit* northQuadrantTerrainData = northQuadrantTree->getQuadrant()->getTerrainEdit();
	return not terrainData->northSideZMinus.almostEqual(northQuadrantTerrainData->southSideZPlus);
}

bool Quadrant::checkSouthSideBlendNeeded(QuadTree* southQuadrantTree)
{
	m_southQuadrantValid = southQuadrantTree != nullptr and southQuadrantTree->isValid();

	if (getTerrainEdit()->southSideZPlus.needBlend)
		return true;

	if (not m_southQuadrantValid)
		return false;

	//return not getTerrainEdit()->southSideZPlus.almostEqual(southQuadrantTree->getQuadrant()->getTerrainEdit()->northSideZMinus);
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	TheWorld_Utils::TerrainEdit* southQuadrantTerrainData = southQuadrantTree->getQuadrant()->getTerrainEdit();
	return not terrainData->southSideZPlus.almostEqual(southQuadrantTerrainData->northSideZMinus);
}

bool Quadrant::checkEastSideBlendNeeded(QuadTree* eastQuadrantTree)
{
	m_eastQuadrantValid = eastQuadrantTree != nullptr and eastQuadrantTree->isValid();

	if (getTerrainEdit()->eastSideXPlus.needBlend)
		return true;

	if (not m_eastQuadrantValid)
		return false;

	//return not getTerrainEdit()->eastSideXPlus.almostEqual(eastQuadrantTree->getQuadrant()->getTerrainEdit()->westSideXMinus);
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	TheWorld_Utils::TerrainEdit* eastQuadrantTerrainData = eastQuadrantTree->getQuadrant()->getTerrainEdit();
	return not terrainData->eastSideXPlus.almostEqual(eastQuadrantTerrainData->westSideXMinus);
}

bool Quadrant::checkWestSideBlendNeeded(QuadTree* westQuadrantTree)
{
	m_westQuadrantValid = westQuadrantTree != nullptr and westQuadrantTree->isValid();

	if (getTerrainEdit()->westSideXMinus.needBlend)
		return true;

	if (not m_westQuadrantValid)
		return false;

	//return not getTerrainEdit()->westSideXMinus.almostEqual(westQuadrantTree->getQuadrant()->getTerrainEdit()->eastSideXPlus);
	TheWorld_Utils::TerrainEdit* terrainData = getTerrainEdit();
	TheWorld_Utils::TerrainEdit* westQuadrantTerrainData = westQuadrantTree->getQuadrant()->getTerrainEdit();
	return not terrainData->westSideXMinus.almostEqual(westQuadrantTerrainData->eastSideXPlus);
}

godot::Vector3 Quadrant::getNormalFromNormalmap(size_t index, bool& ok)
{
	ok = true;

	struct TheWorld_Utils::_RGB* rgb = (struct TheWorld_Utils::_RGB*)getNormalsBuffer(true, true).ptr();
	if (rgb == nullptr)
	{
		ok = false;
		return godot::Vector3();
	}

	rgb += index;
	godot::Vector3 packedNormal(float(rgb->r) / 255, float(rgb->g) / 255, float(rgb->b) / 255);
	float normalX = 0, normalY = 0, normalZ = 0;
	TheWorld_Utils::unpackNormal((float)packedNormal.x, (float)packedNormal.y, (float)packedNormal.z, normalX, normalY, normalZ);
	godot::Vector3 normal(normalX, normalY, normalZ);
	return normal;
}

void Quadrant::populateGridVertices(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced)
{
	//BYTE shortBuffer[256 + 1];
	//TheWorld_Viewer_Utils::TimerMs clock;

	//{
	//	GridVertex v(0.12F, 0.1212F, 0.1313F, 1);
	//	v.serialize(shortBuffer, size);
	//	GridVertex v1(shortBuffer, size);
	//	assert(v.posX() == v1.posX());
	//	assert(v.altitude() == v1.altitude());
	//	assert(v.posZ() == v1.posZ());
	//	assert(v.lvl() == v1.lvl());
	//}

	//size_t serializedVertexSize = 0;
	//TheWorld_Utils::GridVertex v;
	// Serialize an empty GridVertex only to obtain the size of a serialized GridVertex
	//v.serialize(shortBuffer, serializedVertexSize);

	float lowerXGridVertex = m_quadrantPos.getLowerXGridVertex();
	float lowerZGridVertex = m_quadrantPos.getLowerZGridVertex();
	float gridStepInWU = m_quadrantPos.getGridStepInWU();
	int numVerticesPerSize = m_quadrantPos.getNumVerticesPerSize();
	int lvl = m_quadrantPos.getLevel();
	std::string buffGridVertices;

	// look for cache in file system
	TheWorld_Utils::MeshCacheBuffer cache = getInternalMeshCacheBuffer();
	std::string meshId = cache.getMeshIdFromDisk(false, nullptr, true);

	if (meshId.length() > 0)
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetQuadrantVertices(m_quadTree->Viewer()->isInEditor(), cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepInWU, lvl, setCamera, cameraDistanceFromTerrainForced, meshId);
		//clock.tock();
		//m_quadTree->Viewer()->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + getId().getId().c_str() + " TAG=" + getId().getTag().c_str() + " - client MapManagerGetVertices (quadrant found in cache) " + std::to_string(clock.duration().count()).c_str() + " ms");
	}
	else
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetQuadrantVertices(m_quadTree->Viewer()->isInEditor(), cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepInWU, lvl, setCamera, cameraDistanceFromTerrainForced, meshId);
		//clock.tock();
		//m_quadTree->Viewer()->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + getId().getId().c_str() + " TAG=" + getId().getTag().c_str() + " - client MapManagerGetVertices " + std::to_string(clock.duration().count()).c_str() + " ms");
	}

	return;
}

void Quadrant::setMeshId(std::string meshId)
{
	if (meshId.length() == 0)
		meshId = "";
	m_meshId = meshId;
}

void Quadrant::refreshSplatmapBorderRows(void)
{
	size_t borderMapSize = m_viewer->Globals()->numVerticesPerQuadrantSize();
	
	if (m_splatmapBuffer.size() != borderMapSize * borderMapSize * sizeof(struct TheWorld_Utils::_RGBA))
	{
		m_splatmapNorthBorderRow.clear();
		m_splatmapSouthBorderRow.clear();
		m_splatmapWestBorderRow.clear();
		m_splatmapEastBorderRow.clear();
		m_lastBorderRefreshTs = TheWorld_Utils::null_time_point();
		return;
	}
	
	BYTE* splatmapBufferPtr = m_splatmapBuffer.ptr();

	// Splatmap North Border Row
	{
		m_splatmapNorthBorderRow.set(splatmapBufferPtr, borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
	}

	// Splatmap South Border Row
	{
		BYTE* offset = splatmapBufferPtr + (borderMapSize * (borderMapSize - 1) * sizeof(struct TheWorld_Utils::_RGBA));
		m_splatmapSouthBorderRow.set(offset, borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
	}

	// Splatmap West Border Row
	{
		m_splatmapWestBorderRow.reserve(borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
		for (int idx = 0; idx < borderMapSize; idx++)
		{
			struct TheWorld_Utils::_RGBA* element = m_splatmapBuffer.at_ptr<struct TheWorld_Utils::_RGBA>(0, idx, borderMapSize);
			m_splatmapWestBorderRow.set_at<struct TheWorld_Utils::_RGBA>(*element, idx, 0, borderMapSize);
		}
		m_splatmapWestBorderRow.adjustSize(borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
	}

	// Splatmap East Border Row
	{
		m_splatmapEastBorderRow.reserve(borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
		for (int idx = 0; idx < borderMapSize; idx++)
		{
			struct TheWorld_Utils::_RGBA* element = m_splatmapBuffer.at_ptr<struct TheWorld_Utils::_RGBA>(borderMapSize - 1, idx, borderMapSize);
			m_splatmapEastBorderRow.set_at<struct TheWorld_Utils::_RGBA>(*element, idx, 0, borderMapSize);
		}
		m_splatmapEastBorderRow.adjustSize(borderMapSize * sizeof(struct TheWorld_Utils::_RGBA));
	}

	m_lastBorderRefreshTs = TheWorld_Utils::current_utc_time_point();

	// $$$$$$
	/*
	{
		if (getPos().getIndexPosX() == 15 and getPos().getIndexPosZ() == 1)
		{
			// North Side
			godot::PackedByteArray dataNorth;
			size_t splatmapBufferSizeRowInBytes = (size_t)borderMapSize * sizeof(struct TheWorld_Utils::_RGBA);
			dataNorth.resize((int)splatmapBufferSizeRowInBytes);
			assert(splatmapBufferSizeRowInBytes == m_splatmapNorthBorderRow.size());
			{
				memcpy((char*)dataNorth.ptrw(), m_splatmapNorthBorderRow.ptr(), splatmapBufferSizeRowInBytes);
			}
			Ref<godot::Image> imageNorth = godot::Image::create_from_data((int32_t)borderMapSize, 1, false, godot::Image::Format::FORMAT_RGBA8, dataNorth);
			std::string name = "C:\\Users\\Riccardo\\AppData\\Roaming\\Godot\\app_userdata\\TheWorld\\temp\\Quadrant_" + std::to_string(getPos().getIndexPosX()) + "-" + std::to_string(getPos().getIndexPosZ()) + "_North_Border.png";
			imageNorth->save_png(name.c_str());

			// South Side
			godot::PackedByteArray dataSouth;
			splatmapBufferSizeRowInBytes = (size_t)borderMapSize * sizeof(struct TheWorld_Utils::_RGBA);
			dataSouth.resize((int)splatmapBufferSizeRowInBytes);
			assert(splatmapBufferSizeRowInBytes == m_splatmapSouthBorderRow.size());
			{
				memcpy((char*)dataSouth.ptrw(), m_splatmapSouthBorderRow.ptr(), splatmapBufferSizeRowInBytes);
			}
			Ref<godot::Image> imageSouth = godot::Image::create_from_data((int32_t)borderMapSize, 1, false, godot::Image::Format::FORMAT_RGBA8, dataSouth);
			name = "C:\\Users\\Riccardo\\AppData\\Roaming\\Godot\\app_userdata\\TheWorld\\temp\\Quadrant_" + std::to_string(getPos().getIndexPosX()) + "-" + std::to_string(getPos().getIndexPosZ()) + "_South_Border.png";
			imageSouth->save_png(name.c_str());

			// West Side
			godot::PackedByteArray dataWest;
			splatmapBufferSizeRowInBytes = (size_t)borderMapSize * sizeof(struct TheWorld_Utils::_RGBA);
			dataWest.resize((int)splatmapBufferSizeRowInBytes);
			assert(splatmapBufferSizeRowInBytes == m_splatmapWestBorderRow.size());
			{
				memcpy((char*)dataWest.ptrw(), m_splatmapWestBorderRow.ptr(), splatmapBufferSizeRowInBytes);
			}
			Ref<godot::Image> imageWest = godot::Image::create_from_data((int32_t)borderMapSize, 1, false, godot::Image::Format::FORMAT_RGBA8, dataWest);
			name = "C:\\Users\\Riccardo\\AppData\\Roaming\\Godot\\app_userdata\\TheWorld\\temp\\Quadrant_" + std::to_string(getPos().getIndexPosX()) + "-" + std::to_string(getPos().getIndexPosZ()) + "_West_Border.png";
			imageWest->save_png(name.c_str());

			// East Side
			godot::PackedByteArray dataEast;
			splatmapBufferSizeRowInBytes = (size_t)borderMapSize * sizeof(struct TheWorld_Utils::_RGBA);
			dataEast.resize((int)splatmapBufferSizeRowInBytes);
			assert(splatmapBufferSizeRowInBytes == m_splatmapEastBorderRow.size());
			{
				memcpy((char*)dataEast.ptrw(), m_splatmapEastBorderRow.ptr(), splatmapBufferSizeRowInBytes);
			}
			Ref<godot::Image> imageEast = godot::Image::create_from_data((int32_t)borderMapSize, 1, false, godot::Image::Format::FORMAT_RGBA8, dataEast);
			name = "C:\\Users\\Riccardo\\AppData\\Roaming\\Godot\\app_userdata\\TheWorld\\temp\\Quadrant_" + std::to_string(getPos().getIndexPosX()) + "-" + std::to_string(getPos().getIndexPosZ()) + "_East_Border.png";
			imageEast->save_png(name.c_str());
		}
	}
	*/
	// $$$$$$
}

void Quadrant::refreshGridVerticesFromServer(std::string& bufferFromServer, std::string meshIdFromServer, std::string& outMeshIdFromBuffer, bool updateCache)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	float minAltitude = 0, maxAltitude = 0;
	outMeshIdFromBuffer.clear();
	
	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.1 ") + __FUNCTION__, "m_cache.refreshMapsFromBuffer");

		m_cache.refreshMapsFromBuffer(bufferFromServer, outMeshIdFromBuffer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer, updateCache);
		if (terrainEditValuesBuffer.size() > 0)
		{
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);
			//m_textureLayout = ShaderTerrainData::layoutFromContinentId(getTerrainEdit()->extraValues1.continentId);
			//m_textureLayout = ShaderTerrainData::layoutFromQuadrantPos(m_cache.getQuadrantPos());
		}
	}

	if (outMeshIdFromBuffer.size() > 0)
	{
		size_t l1 = outMeshIdFromBuffer.size();
		size_t l2 = meshIdFromServer.size();
		const char* c1 = outMeshIdFromBuffer.c_str();
		const char* c2 = meshIdFromServer.c_str();
		//assert(outMeshIdFromBuffer == meshIdFromServer);
		if (outMeshIdFromBuffer != meshIdFromServer)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("MeshId from buffer not equal to meshId from server").c_str()));
	}

	//setEmpty(false);

	if (outMeshIdFromBuffer.size() == 0 || terrainEditValuesBuffer.empty())
	{
		// Buffer from server was empty
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.2 ") + __FUNCTION__, "m_cache.readMapsFromMeshCache");

		bool empty = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), meshIdFromServer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
		if (terrainEditValuesBuffer.size() > 0)
		{
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);
			//m_textureLayout = ShaderTerrainData::layoutFromContinentId(getTerrainEdit()->extraValues1.continentId);
			//m_textureLayout = ShaderTerrainData::layoutFromQuadrantPos(m_cache.getQuadrantPos());
		}

		//if (empty)
		//	setEmpty(true);
	}
	else
	{
		// Buffer from server was valid ...
		if (updateCache)
		{
			// ... and was written to disk cache, we can remove from list of quadrant to save if marked to save
			GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
			if (editModeUIControl != nullptr)
			{
				bool manualSave = false;
				if (editModeUIControl->isQuadToSave(m_quadrantPos, manualSave))
				{
					if (manualSave)
						editModeUIControl->removeQuadToSave(m_quadrantPos);
				}
			}
		}
	}
	
	refreshSplatmapBorderRows();

	//assert(!m_float16HeigthsBuffer.empty());
	if (m_float16HeigthsBuffer.empty())
	{
		getTerrainEdit()->heightsNeedRegen = true;
		getTerrainEdit()->normalsNeedRegen = true;
		getTerrainEdit()->splatmapNeedRegen(true);
		getTerrainEdit()->globalmapNeedRegen = true;
		getTerrainEdit()->colormapNeedRegen = true;
	}
	else
	{
		if (m_normalsBuffer.empty() && !getTerrainEdit()->normalsNeedRegen)
			getTerrainEdit()->normalsNeedRegen = true;
		if (m_splatmapBuffer.empty() && !getTerrainEdit()->splatmapNeedRegen())
			getTerrainEdit()->splatmapNeedRegen(true);
		if (m_globalmapBuffer.empty() && !getTerrainEdit()->globalmapNeedRegen)
			getTerrainEdit()->globalmapNeedRegen = true;
		if (m_colormapBuffer.empty() && !getTerrainEdit()->colormapNeedRegen)
			getTerrainEdit()->colormapNeedRegen = true;
	}


	//if ((getTerrainEdit()->extraValues.seaInfoSet != QUADRANT_SEA_INFO_FALSE && getTerrainEdit()->extraValues.seaInfoSet != QUADRANT_SEA_INFO_TRUE)
	//	|| (!getTerrainEdit()->heightsNeedRegen && getTerrainEdit()->extraValues.seaInfoSet == QUADRANT_SEA_INFO_FALSE))
	//{
	//	bool done = getTerrainEdit()->detectSeaInQuadrant();
	//}

	if (getTerrainEdit()->needUploadToServer)
		setNeedUploadToServer(true);
	else
		setNeedUploadToServer(false);

	int resolution = globals->heightmapResolution() + 1;

	m_quadTree->getQuadrant()->terrainDataJustLoaded(true);

	bool cache = true;

	setHeightsUpdated(true);
	if (cache)
	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.3 ") + __FUNCTION__, "Cache heightmap images");

		std::string profileHeader("WorldDeploy 2.1.1.3.1 ");
		getShaderTerrainData()->getHeightmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("WorldDeploy 2.1.1.3.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::heightmap, optimizationLevel, cachedTextureResolution);
		}
	}

	setNormalsUpdated(true);
	if (cache)
	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.4 ") + __FUNCTION__, "Cache normalmap images");

		std::string profileHeader("WorldDeploy 2.1.1.4.1 ");
		getShaderTerrainData()->getNormalmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::normalmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("WorldDeploy 2.1.1.4.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::normalmap, optimizationLevel, cachedTextureResolution);
		}
	}

	setSplatmapUpdated(true);
	if (cache)
	{
		// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.5 ") + __FUNCTION__, "Cache splatmap images");

		std::string profileHeader("WorldDeploy 2.1.1.5.1 ");
		getShaderTerrainData()->getSplatmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::splatmap, 0, resolution);

		size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
		size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
		for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
		{
			size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
			std::string profileHeader("WorldDeploy 2.1.1.5.2 ");
			godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::splatmap, optimizationLevel, cachedTextureResolution);
		}
	}

	size_t optimizationLevelForMapImage = (size_t)MapQuadrantImagesMap::getMapQuadrantOptimizationLevel();	// 0 = 2049, 2 = 513, 4 = 129, 6 = 33;
	size_t textureNumPixelsPerSize = globals->resolutionForOptimizationLevel(optimizationLevelForMapImage);

	if (m_globalmapBuffer.size() > 0)
	{
		setGlobalmapUpdated(true);

		if (cache)
		{
			// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.6 ") + __FUNCTION__, "Cache globalmap images");

			std::string profileHeader("WorldDeploy 2.1.1.6.1 ");
			getShaderTerrainData()->getGlobalmapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::globalmap, 0, resolution);

			size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
			size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
			for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
			{
				size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
				std::string profileHeader("WorldDeploy 2.1.1.6.1 ");
				godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::globalmap, optimizationLevel, cachedTextureResolution);
			}
		}

		std::string profileHeader("WorldDeploy 2.1.1.6.1 ");
		m_mapImage = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::globalmap, optimizationLevelForMapImage, textureNumPixelsPerSize);
		godot::Vector2i sz = m_mapImage->get_size();	// DEBUG
		sz = m_mapImage->get_size();	// DEBUG
		//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".map.png";
		//m_mapImage->save_png(outFileName.c_str());
	}

	if (m_colormapBuffer.size() > 0)
	{
		setColorsUpdated(true);

		if (cache)
		{
			// Pre cache images for shader: HiRes Image and all LowRes images that will be used during rendering
			TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.7 ") + __FUNCTION__, "Cache colormap images");

			std::string profileHeader("WorldDeploy 2.1.1.7.1 ");
			getShaderTerrainData()->getColormapImageRef() = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::colormap, 0, resolution);

			size_t maxOptimizationLevel = globals->getMaxTextureOptimizationLevel();
			size_t textureOptimizationResolutionIncrement = globals->getParamTextureOptimizationResolutionIncrement();
			for (size_t optimizationLevel = textureOptimizationResolutionIncrement; optimizationLevel <= maxOptimizationLevel; optimizationLevel += textureOptimizationResolutionIncrement)
			{
				size_t cachedTextureResolution = globals->resolutionForOptimizationLevel(optimizationLevel);
				std::string profileHeader("WorldDeploy 2.1.1.7.2 ");
				godot::Ref<godot::Image> image = getShaderTerrainData()->getImageAtResolution(profileHeader, ShaderTerrainData::ImageType::colormap, optimizationLevel, cachedTextureResolution);
			}
		}
	}

	int areaSize = resolution * resolution;

	//BYTE shortBuffer[256 + 1];
	size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
	//TheWorld_Utils::serializeToByteStream<uint16_t>(0, shortBuffer, uint16_t_size);
	size_t float_size = sizeof(float);	// the size of a float
	//TheWorld_Utils::serializeToByteStream<float>(0, shortBuffer, float_size);

	size_t numFloat16Heights = m_float16HeigthsBuffer.size() / uint16_t_size;
	size_t numFloat32Heights = m_float32HeigthsBuffer.size() / float_size;
	size_t numNormals = m_normalsBuffer.size() / sizeof(struct TheWorld_Utils::_RGB);
	size_t numSplatmapVertices = m_splatmapBuffer.size() / sizeof(struct TheWorld_Utils::_RGBA);
	size_t numColorVertices = m_colormapBuffer.size() / sizeof(struct TheWorld_Utils::_RGBA);
	size_t numGlobalmapVertices = m_globalmapBuffer.size() / sizeof(struct TheWorld_Utils::_RGB);

	assert(numFloat16Heights == numFloat32Heights);
	assert(numNormals == 0 || numFloat16Heights == numNormals);
	assert(numSplatmapVertices == 0 || numFloat16Heights == numSplatmapVertices);
	assert(numColorVertices == 0 || numFloat16Heights == numColorVertices);
	assert(numGlobalmapVertices == 0 || numFloat16Heights == numGlobalmapVertices);
	assert(numFloat16Heights == areaSize);
	if (numFloat16Heights != numFloat32Heights || (numNormals > 0 && numFloat16Heights != numNormals) || (numSplatmapVertices > 0 && numFloat16Heights != numSplatmapVertices) || (numColorVertices > 0 && numFloat16Heights != numColorVertices) || (numGlobalmapVertices > 0 && numFloat16Heights != numGlobalmapVertices) || numFloat16Heights != areaSize)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Grid Vertices not of the correct size").c_str()));

	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.8 ") + __FUNCTION__, "SetColliderHeights");

		m_heightsForCollider.resize((int)numFloat32Heights);
#ifdef REAL_T_IS_DOUBLE
		float* f = (float*)m_float32HeigthsBuffer.ptr();
		for (int idx = 0; idx < numFloat32Heights; idx++)
		{
			m_heightsForCollider.set(idx, (real_t)*f);
			f++;
		}
#else
		memcpy((char*)m_heightsForCollider.ptrw(), m_float32HeigthsBuffer.ptr(), m_float32HeigthsBuffer.size());
#endif // REAL_T_IS_DOUBLE
		//{
		//	uint16_t* ptrFrom = (uint16_t*)m_float16HeigthsBuffer.ptr();
		//	m_heights.resize((int)numFloat16Heights);
		//	godot::PoolRealArray::Write w = m_heights.write();
		//	float* ptrTo = w.ptr();
		//	for (size_t idx = 0; idx < numFloat16Heights; idx++)
		//	{
		//		TheWorld_Utils::FLOAT_32 f;
		//		f.u32 = half_to_float(*ptrFrom);
		//		*ptrTo = f.f32;
		//		ptrFrom++;
		//		ptrTo++;
		//	}
		//}
	}

	Vector3 startPosition(m_quadrantPos.getLowerXGridVertex(), minAltitude, m_quadrantPos.getLowerZGridVertex());
	Vector3 endPosition(startPosition.x + m_quadrantPos.getSizeInWU(), maxAltitude, startPosition.z + m_quadrantPos.getSizeInWU());
	Vector3 size = endPosition - startPosition;

	m_globalCoordAABB.set_position(startPosition);
	m_globalCoordAABB.set_size(size);
	
	if (meshIdFromServer.length() == 0)
		meshIdFromServer = "";
	m_meshId = meshIdFromServer;

	//m_collider->setData();
}

void Quadrant::NeighbourQuadrantData::init(Quadrant* owningQuadrant, Quadrant* neighbourQuadrant, TheWorld_Utils::QuadrantPos::DirectionSlot neighbourQuadrantDir, TheWorld_Utils::QuadrantPos& owningQuadrantPos)
{
	m_owningQuadrant = owningQuadrant;

	TheWorld_Utils::QuadrantPos::DirectionSlot requiredBorderRow = TheWorld_Utils::QuadrantPos::DirectionSlot::Center;

	switch (neighbourQuadrantDir)
	{
	case TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus:
	{
		requiredBorderRow = TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus;
	}
	break;
	case TheWorld_Utils::QuadrantPos::DirectionSlot::SouthZPlus:
	{
		requiredBorderRow = TheWorld_Utils::QuadrantPos::DirectionSlot::NorthZMinus;
	}
	break;
	case TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus:
	{
		requiredBorderRow = TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus;
	}
	break;
	case TheWorld_Utils::QuadrantPos::DirectionSlot::EastXPlus:
	{
		requiredBorderRow = TheWorld_Utils::QuadrantPos::DirectionSlot::WestXMinus;
	}
	break;
	}
	if (requiredBorderRow != TheWorld_Utils::QuadrantPos::DirectionSlot::Center)
		neighbourQuadrant->getSplatmapBorderRow(m_lastBorderRefreshTs, m_splatmapBorderRow, requiredBorderRow);

	if (neighbourQuadrantDir == TheWorld_Utils::QuadrantPos::DirectionSlot::NorthwestZMinusXMinus)
		m_upperLeftCornerSplatmapTexel = neighbourQuadrant->getLowerRightCornerSplatmapTexel();
	
	m_textureLayout = neighbourQuadrant->textureLayout();
	m_groundTextureLayers = neighbourQuadrant->quadrantTextureLayers();

	initialized(true);
	shaderUpdated(false);

	//// #####
	//m_owningQuadrantPos = owningQuadrantPos;
	//m_neighbourQuadrantPos = neighbourQuadrant->getPos();
	//if (m_owningQuadrantPos.getIndexPosX() == 5 and m_owningQuadrantPos.getIndexPosZ() == 12 and m_neighbourQuadrantPos.getIndexPosX() == 4 and m_neighbourQuadrantPos.getIndexPosZ() == 12)
	//	assert(m_owningQuadrantPos.getIndexPosX() == 5);
	//// #####
}

godot::Ref<godot::Texture2D> Quadrant::NeighbourQuadrantData::getSplatmapBorderRowTexture(void)
{
	int numVerticesPerQuadrantSize = (int)m_owningQuadrant->getQuadTree()->Viewer()->Globals()->numVerticesPerQuadrantSize();

	godot::Ref<godot::Texture2D> tex;

	if (TheWorld_Utils::null_time_point(m_lastBorderRefreshTs))
	{
		Ref<godot::Image> image = godot::Image::create(numVerticesPerQuadrantSize, 1, false, godot::Image::Format::FORMAT_RGBA8);
		image->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));
		tex = godot::ImageTexture::create_from_image(image);
	}
	else
	{
		godot::PackedByteArray data;
		size_t splatmapBufferSizeRowInBytes = (size_t)numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGBA);
		data.resize((int)splatmapBufferSizeRowInBytes);
		assert(splatmapBufferSizeRowInBytes == m_splatmapBorderRow.size());
		{
			memcpy((char*)data.ptrw(), m_splatmapBorderRow.ptr(), splatmapBufferSizeRowInBytes);
		}
		Ref<godot::Image> image = godot::Image::create_from_data(numVerticesPerQuadrantSize, 1, false, godot::Image::Format::FORMAT_RGBA8, data);
		tex = godot::ImageTexture::create_from_image(image);
	}

	return tex;
}
