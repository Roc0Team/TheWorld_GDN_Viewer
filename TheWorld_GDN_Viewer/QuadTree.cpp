//#include "pch.h"
#include "QuadTree.h"
#include "Viewer_Utils.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Quadrant.h"
#include "GDN_TheWorld_Edit.h"

#include "Profiler.h"
#include "half.h"
#include "Utils.h"

#include "assert.h"
#include <filesystem>

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
#pragma warning(pop)

using namespace godot;
namespace fs = std::filesystem;

GDN_TheWorld_ShaderTexture* ShaderTerrainData::g_lookDevShaderTexture = nullptr;

//extern int g_num = 0;

// QuadTree coordinates are local to Viewer Node which contains the grid of quads: the first chunk (posX = posZ = 0) has its lower vertex in the origin of the Viwer Node

Quad::Quad(int slotPosX, int slotPosZ, int lod, enum PosInQuad posInQuad, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
{
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
		m_children[i] = make_unique<Quad>((m_slotPosX * 2) + (i & 1), (m_slotPosZ * 2) + ((i & 2) >> 1), m_lod - 1, enum PosInQuad(i + 1), m_viewer, m_quadTree);
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
	assert(m_chunk == nullptr);
	
	m_chunk = m_quadTree->getChunkAt(Chunk::ChunkPos(m_slotPosX, m_slotPosZ, m_lod));

	Ref<Material> mat = m_quadTree->getCurrentMaterial(m_lod);

	if (m_chunk == nullptr)
	{
		// create chunk for current quad because this is the first time that is needed for current lod and pos
		if (_DEBUG_AAB)
			m_chunk = new ChunkDebug(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);
		else
			m_chunk = new Chunk(m_slotPosX, m_slotPosZ, m_lod, m_viewer, m_quadTree, mat);

		// Set Viewer Pos in Global Coord
		//m_chunk->setParentGlobalTransform(m_viewer->internalTransformGlobalCoord());
		m_quadTree->addChunk(m_chunk);
	}
	else
		m_chunk->setMaterial(mat);

	m_chunk->setPosInQuad(m_posInQuad, this);
	m_quadTree->addChunkUpdate(m_chunk);
	m_chunk->setActive(true);
	m_chunkAABB = m_chunk->getAABB();
	m_chunkSizeInWUs = m_chunk->getChunkSizeInWUs();
}

void Quad::setCameraPos(Vector3 globalCoordCameraLastPos)
{
	if (isLeaf())
	{
		Chunk* chunk = getChunk();
		if (chunk != nullptr)
			chunk->setCameraPos(globalCoordCameraLastPos);
	}
}
	
QuadTree::QuadTree(GDN_TheWorld_Viewer* viewer, QuadrantPos quadrantPos)
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
	m_lookDev = ShaderTerrainData::LookDev::NotSet;
}

void QuadTree::refreshTerrainData(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrain)
{
	my_assert(statusInitialized() || statusRefreshTerrainDataNeeded());
	if (!(statusInitialized() || statusRefreshTerrainDataNeeded()))
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrant);

	setStatus(QuadrantStatus::refreshTerrainDataInProgress);

	m_worldQuadrant->populateGridVertices(cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll, setCamera, cameraDistanceFromTerrain);
}

void QuadTree::init(float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, bool setCamera, float cameraDistanceFromTerrainForced)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "ALL");

	my_assert(status() == QuadrantStatus::uninitialized);
	if (status() != QuadrantStatus::uninitialized)
		throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("Quadrant status not comply: " + std::to_string((int)status())).c_str()));

	std::lock_guard<std::recursive_mutex> lock(m_mtxQuadrant);

	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "Inside Mutex");

		{
			//TheWorld_Utils::GuardProfiler profiler(std::string("QuadTree::init ") + __FUNCTION__, "Part 1");

			m_viewer->getMainProcessingMutex().lock();
			m_GDN_Quadrant = memnew(GDN_TheWorld_Quadrant);
			m_viewer->getMainProcessingMutex().unlock();
			m_GDN_Quadrant->init(this);
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

			m_root = make_unique<Quad>(0, 0, m_viewer->Globals()->lodMaxDepth(), PosInQuad::Root, m_viewer, this);

			if (isVisible())
				m_viewer->incNumVisibleQuadrantsStreamed();
			else
				m_viewer->incNumInvisibleQuadrantsStreamed();
		}
	}
}

void QuadTree::onGlobalTransformChanged(void)
{
	QuadrantPos quadrantPos = getQuadrant()->getPos();
	Transform3D gt = m_viewer->get_global_transform();
	Transform3D t = gt * Transform3D(Basis(), Vector3(quadrantPos.getLowerXGridVertex(), 0, quadrantPos.getLowerZGridVertex()));
	m_GDN_Quadrant->set_global_transform(t);
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
			parent->call_deferred("remove_child", m_GDN_Quadrant);
		}
		m_GDN_Quadrant->queue_free();
		m_GDN_Quadrant = nullptr;
	}

	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed1 += clock.duration().count();

	//clock.tick();
	m_root.reset();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed2 += clock.duration().count();

	//clock.tick();
	clearChunkUpdate();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed3 += clock.duration().count();

	// Delete all Chunks
	//clock.tick();
	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			delete it1->second;
		}
		it->second.clear();
	}
	m_mapChunk.clear();
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed4 += clock.duration().count();

	//clock.tick();
	if (m_worldQuadrant)
		delete m_worldQuadrant;
	//clock.tock();
	//godot::GDN_TheWorld_Globals::s_elapsed5 += clock.duration().count();
}

void QuadTree::ForAllChunk(Chunk::ChunkAction& chunkAction)
{
	if (!isValid())
		return;

	for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
	{
		for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
		{
			chunkAction.exec(it1->second);
		}
	}
}

void QuadTree::mouseHitChanged(godot::Vector3 mouseHit, bool hit)
{
	m_hitByMouse = hit;

	godot::Vector2 viewportMouseHit;

	if (hit)
	{
		QuadrantPos pos = getQuadrant()->getPos();
		float s = pos.getSizeInWU();							// DEBUG ONLY
		float dx = mouseHit.x - pos.getLowerXGridVertex();		// DEBUG ONLY
		float dz = mouseHit.z - pos.getLowerZGridVertex();		// DEBUG ONLY
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
	QuadrantPos currentPos = getQuadrant()->getPos();

	QuadrantPos pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::WestXMinus);
	QuadTree* quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::EastXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::NorthZMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::SouthZPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::NorthwestZMinusXMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::NortheastZMinusXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::SouthwestZPlusXMinus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
	pos = currentPos.getQuadrantPos(QuadrantPos::DirectionSlot::SoutheastZPlusXPlus);
	quadTree = m_viewer->getQuadTree(pos);
	if (quadTree != nullptr)
		adjacentQuadrantsHit.push_back(quadTree);
}

void QuadTree::update(Vector3 cameraPosGlobalCoord, enum class UpdateStage updateStage, int& numSplitRequired)
{
	if (!isValid())
		return;

	if (!isVisible())
		return;

	if (getQuadrant()->internalDataLocked())
		return;

	m_numLeaf = 0;
	internalUpdate(cameraPosGlobalCoord, m_root.get(), updateStage, numSplitRequired);
	assert(!m_root->isLeaf() || m_root->getChunk() != nullptr);
}

void QuadTree::internalUpdate(godot::Vector3 cameraPosGlobalCoord, Quad* quad, enum class UpdateStage updateStage, int& numSplitRequired)
{
	if (updateStage == UpdateStage::Stage1)
	{
		quad->setJustSplitted(false);
		quad->setJustJoined(false);

		// cameraPosViewerNodeLocalCoord are in grid local coordinates (WUs)
		GDN_TheWorld_Globals* globals = m_viewer->Globals();
		float chunkSizeInWUs = globals->gridStepInHeightmapWUs(quad->Lod()) * globals->numVerticesPerChuckSide();									// chunk size in World Units
		AABB chunkAABB = quad->getChunkAABB();
		m_quadCenterGlobal = godot::Vector3(real_t((quad->slotPosX() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerXGridVertex() + chunkSizeInWUs / 2),
			(chunkAABB.position + chunkAABB.size / 2).y,
			real_t((quad->slotPosZ() * chunkSizeInWUs) + m_worldQuadrant->getPos().getLowerZGridVertex() + chunkSizeInWUs / 2));

		Transform3D gt = m_viewer->get_global_transform();
		Transform3D t = gt * Transform3D(Basis(), m_quadCenterGlobal);
		m_quadCenterGlobal = t.origin;

		m_splitDistance = chunkSizeInWUs * globals->splitScale();
		//real_t splitDistance = m_worldQuadrant->getPos().getSizeInWU() * globals->splitScale();
		if (quad->isLeaf())
		{
			bool trackChunk = false;
			if (m_viewer->trackMouse() && getQuadrant()->getPos().getName() == m_viewer->getMouseQuadrantHitName())
				trackChunk = true;

			bool splitted = false;
			m_numLeaf++;
			if (quad->Lod() > 0)
			{
				real_t distance = m_quadCenterGlobal.distance_to(cameraPosGlobalCoord);
				quad->getChunk()->setDistanceFromCamera(distance);
				if (distance < m_splitDistance)
				{
					// Split
					quad->split();
					for (int i = 0; i < 4; i++)
					{
						quad->getChild(i)->setCameraPos(cameraPosGlobalCoord);
						if (trackChunk)
							quad->getChild(i)->getChunk()->checkMouseHit();
						quad->getChild(i)->getChunk()->setDistanceFromCamera(distance);
					}
					quad->setJustSplitted(true);
					splitted = true;
					m_numSplits++;
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
				internalUpdate(cameraPosGlobalCoord, child, updateStage, numSplitRequired);
				if (!child->isLeaf())
					allChildrenAreLeaf = false;
			}

			//if (allChildrenAreLeaf)
			//{
			//	real_t distance = m_quadCenterGlobal.distance_to(cameraPosGlobalCoord);
			//	if (distance > m_splitDistance)
			//	{
			//		// Join
			//		quad->clearChildren();
			//		quad->assignChunk();
			//		quad->getChunk()->setJustJoined(true);
			//		quad->setJustJoined(true);
			//		m_numJoins++;
			//		quad->getChunk()->checkMouseHit();
			//		quad->getChunk()->setDistanceFromCamera(distance);
			//	}
			//}
		}

		// after all if quad is leaf: just joined or was leaf and it has not been splitted
		quad->setCameraPos(cameraPosGlobalCoord);
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
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (First) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
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
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Second) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZMinusChunk, Chunk::LookForChunk::HigherLod);
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
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XMinusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Third) - " << " XMinus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
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
						Chunk* neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::XPlusChunk, Chunk::LookForChunk::HigherLod);
						if (neighbourChunk != nullptr && neighbourChunk->getLod() > 0 && (neighbourChunk->getLod() - quad->Lod()) > 1 && neighbourChunk->getQuad() != nullptr && !neighbourChunk->getQuadTree()->getQuadrant()->internalDataLocked())
						{
							neighbourChunk->getQuad()->setSplitRequired(true);
							numSplitRequired++;
							//if (neighbourChunk->getLod() - quad->Lod() > 2)
							//	PLOG_DEBUG << getQuadrant()->getPos().getName() << " " << chunk->getIdStr() << " (Forth) - " << " XPlus neighbour " << neighbourChunk->getQuadTree()->getQuadrant()->getPos().getName() << " " << neighbourChunk->getIdStr();
						}
						neighbourChunk = m_viewer->getActiveChunkAt(chunk, Chunk::DirectionSlot::ZPlusChunk, Chunk::LookForChunk::HigherLod);
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
				internalUpdate(cameraPosGlobalCoord, quad->getChild(i), updateStage, numSplitRequired);
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
				// Split
				quad->split();
				for (int i = 0; i < 4; i++)
				{
					quad->getChild(i)->setCameraPos(cameraPosGlobalCoord);
					if (trackChunk)
						quad->getChild(i)->getChunk()->checkMouseHit();
					quad->getChild(i)->getChunk()->setDistanceFromCamera(0);
				}
				quad->setJustSplitted(true);
				splitted = true;
				m_numSplits++;
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
				internalUpdate(cameraPosGlobalCoord, child, updateStage, numSplitRequired);
				if (!child->isLeaf())
					allChildrenAreLeaf = false;
			}

			if (allChildrenAreLeaf)
			{
				real_t distance = m_quadCenterGlobal.distance_to(cameraPosGlobalCoord);
				if (distance > m_splitDistance && !quad->gotJustSplitted())
				{
					// Join
					quad->clearChildren();
					quad->assignChunk();
					quad->getChunk()->setJustJoined(true);
					quad->setJustJoined(true);
					m_numJoins++;
					quad->getChunk()->checkMouseHit();
					quad->getChunk()->setDistanceFromCamera(distance);
				}
			}
		}

		quad->setSplitRequired(false);
	}
}

bool QuadTree::isChunkOnBorderOfQuadrant(Chunk::ChunkPos pos, QuadrantPos& XMinusQuadrantPos, QuadrantPos& XPlusQuadrantPos, QuadrantPos& ZMinusQuadrantPos, QuadrantPos& ZPlusQuadrantPos)
{
	bool ret = false;

	if (pos.getSlotPosX() == 0)
	{
		XMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XMinus);
		ret = true;
	}

	if (pos.getSlotPosZ() == 0)
	{
		ZMinusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZMinus);
		ret = true;
	}

	int numChunksPerSide = m_viewer->Globals()->numChunksPerHeightmapSide(pos.getLod());

	if (pos.getSlotPosX() == (numChunksPerSide - 1))
	{
		XPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::XPlus);
		ret = true;
	}

	if (pos.getSlotPosZ() == (numChunksPerSide - 1))
	{
		ZPlusQuadrantPos = getQuadrant()->getPos().getQuadrantPos(QuadrantPos::DirectionSlot::ZPlus);
		ret = true;
	}

	return ret;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos, enum class Chunk::DirectionSlot dir)
{
	Chunk* chunk = nullptr;

	switch (dir)
	{
	case Chunk::DirectionSlot::Center:
		chunk = getChunkAt(pos);
		break;
	case Chunk::DirectionSlot::XMinusChunk:
		//
		// - o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusChunk:
		//
		//   o -
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 0, -1));
		break;
	case Chunk::DirectionSlot::ZMinusChunk:
		//   -
		//   o
		//
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusChunk:
		//
		//   o
		//   -
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 1, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadSecondChunk:
		//  
		//	- o =
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 0, -1));
		break;
	case Chunk::DirectionSlot::XMinusQuadForthChunk:
		//  
		//	  o =
		//	- = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(-1, 1, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadFirstChunk:
		//  
		//	  o = -
		//	  = =
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 0, -1));
		break;
	case Chunk::DirectionSlot::XPlusQuadThirdChunk:
		//  
		//	  o = 
		//	  = = -
		//  
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(2, 1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadThirdChunk:
		//    -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, -1, -1));
		break;
	case Chunk::DirectionSlot::ZMinusQuadForthChunk:
		//      -
		//	  o =
		//	  = =
		//    
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, -1, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadFirstChunk:
		//    
		//	  o =
		//	  = =
		//    -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(0, 2, -1));
		break;
	case Chunk::DirectionSlot::ZPlusQuadSecondChunk:
		//      
		//	  o =
		//	  = =
		//      -
		// assuming pos is the first chunk of the quad
		chunk = getChunkAt(pos + Chunk::ChunkPos(1, 2, -1));
		break;
	//default:
	}

	return chunk;
}

Chunk* QuadTree::getChunkAt(Chunk::ChunkPos pos)
{
	Chunk::MapChunk::iterator it = m_mapChunk.find(pos.getLod());
	if (it == m_mapChunk.end())
		return nullptr;
	else
	{
		Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[pos.getLod()].find(pos);
		if (it1 == m_mapChunk[pos.getLod()].end())
			return nullptr;
		else
			return m_mapChunk[pos.getLod()][pos];
	}
}

void QuadTree::addChunk(Chunk* chunk)
{
	if (getChunkAt(chunk->getPos()))
		return;
	
	Chunk::MapChunk::iterator it = m_mapChunk.find(chunk->getLod());
	if (it == m_mapChunk.end())
	{
		Chunk::MapChunkPerLod m;
		m_mapChunk[chunk->getLod()] = m;
	}

	m_mapChunk[chunk->getLod()][chunk->getPos()] = chunk;
}

void QuadTree::addChunkUpdate(Chunk* chunk)
{
	if (chunk->isPendingUpdate())
		return;

	m_vectChunkUpdate.push_back(chunk);
	chunk->setPendingUpdate(true);
}

void QuadTree::clearChunkUpdate(void)
{
	m_vectChunkUpdate.clear();
}

void QuadTree::setLookDevMaterial(enum class ShaderTerrainData::LookDev lookDev, bool reload)
{
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

			for (Chunk::MapChunk::iterator it = m_mapChunk.begin(); it != m_mapChunk.end(); it++)
			{
				for (Chunk::MapChunkPerLod::iterator it1 = it->second.begin(); it1 != it->second.end(); it1++)
				{
					Ref<Material> mat = getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterial(it1->second->getLod());
					it1->second->setMaterial(mat);
				}
			}
		}

		getQuadrant()->getShaderTerrainData()->releaseRegularMaterial();
		getQuadrant()->getShaderTerrainData()->releaseLookDevMaterial();
	}
	else
	{
		Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getLookDevMaterial(lookDev, reload));
		ForAllChunk(action);
		getQuadrant()->getShaderTerrainData()->releaseRegularMaterial();
		getQuadrant()->getShaderTerrainData()->releaseLookDevChunkLodMaterials();
	}
}

void QuadTree::setRegularMaterial(bool reload)
{
	materialParamsNeedReset(true);

	getQuadrant()->setHeightsUpdated(true);
	getQuadrant()->setNormalsUpdated(true);
	getQuadrant()->setSplatmapUpdated(true);
	getQuadrant()->setColorsUpdated(true);
	getQuadrant()->setGlobalmapUpdated(true);

	Chunk::SetMaterialChunkAction action(getQuadrant()->getShaderTerrainData()->getRegularMaterial(reload));
	ForAllChunk(action);

	m_lookDev = ShaderTerrainData::LookDev::NotSet;

	getQuadrant()->getShaderTerrainData()->releaseLookDevMaterial();
	getQuadrant()->getShaderTerrainData()->releaseLookDevChunkLodMaterials();
}

Ref<Material> QuadTree::getCurrentMaterial(int lod)
{
	if (m_lookDev == ShaderTerrainData::LookDev::NotSet)
		return getQuadrant()->getShaderTerrainData()->getRegularMaterial();
	else
		if (m_lookDev == ShaderTerrainData::LookDev::ChunkLod)
		{
			return getQuadrant()->getShaderTerrainData()->getLookDevChunkLodMaterial(lod);
		}
		else
			return getQuadrant()->getShaderTerrainData()->getLookDevMaterial(m_lookDev);
}

void QuadTree::reloadCurrentMaterial(void)
{
	bool reload = true;

	if (m_lookDev == ShaderTerrainData::LookDev::NotSet)
	{
		setRegularMaterial(reload);
	}
	else
	{
		setLookDevMaterial(m_lookDev, reload);
	}
}

void QuadTree::setOptimizations(godot::Vector3& cameraPosGlobalCoord)
{
	godot::Vector3 quadrantCenter  = getQuadrant()->getCenter();
	quadrantCenter.y = (getQuadrant()->getTerrainEdit()->maxHeight + getQuadrant()->getTerrainEdit()->minHeight) / 2;

	float distanceFromCamera = abs((cameraPosGlobalCoord - quadrantCenter).length());

	if (distanceFromCamera > m_viewer->Globals()->getParamThresholdForGlobalmap())
	{
		if (!getQuadrant()->renderGlobalMap())
			getQuadrant()->renderGlobalMap(true);
	}
	else
	{
		if (getQuadrant()->renderGlobalMap())
			getQuadrant()->renderGlobalMap(false);
	}

//#define THRESHOLD_FOR_TEXTURE_OPTIMIZATION		5000.0f
#define MIN_RESOLUTION_SHIFT					5	// exponent of 2
//#define RESOLUTION_INCREMENT					1
//#define THRESHOLD_INCREMENT_FACTOR				2

	size_t optimizationLevel = 0;	// 0 ==> no optimization
	float thresHoldForTextureOptimization = m_viewer->Globals()->getParamThresholdForMapTextureOptimization();
	if (distanceFromCamera > thresHoldForTextureOptimization && m_viewer->Globals()->heightmapResolutionShift() > MIN_RESOLUTION_SHIFT)
	{
		optimizationLevel += m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement();
		size_t maxOptimizationLevel = m_viewer->Globals()->heightmapResolutionShift() - MIN_RESOLUTION_SHIFT;
		float increment = m_viewer->Globals()->heightmapSizeInWUs() * m_viewer->Globals()->getParamThresholdTextureOptimizationIncrementFactor();
		thresHoldForTextureOptimization += increment;
		while (thresHoldForTextureOptimization < distanceFromCamera && optimizationLevel < maxOptimizationLevel)
		{
			thresHoldForTextureOptimization += increment;
			optimizationLevel += m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement();
		}
	}

	//optimizationLevel = 0;	// DEBUG
	getQuadrant()->optimizationLevel(optimizationLevel);
}

bool QuadTree::resetMaterialParams(bool force)
{
	bool reset = false;

	if (force)
	{
		materialParamsNeedReset(true);
		if (!getQuadrant()->internalDataLocked())
		{
			getQuadrant()->getShaderTerrainData()->resetMaterialParams(m_lookDev);
			materialParamsNeedReset(false);
			reset = true;
		}
	}
	else if (materialParamsNeedReset() && !getQuadrant()->internalDataLocked())
	{
		//if (status() == QuadrantStatus::getTerrainDataInProgress || status() == QuadrantStatus::uninitialized || status() == QuadrantStatus::initializationStopped)
		if (status() != QuadrantStatus::initialized)
		{
		}
		else
		{
			getQuadrant()->getShaderTerrainData()->resetMaterialParams(m_lookDev);
			materialParamsNeedReset(false);
			reset = true;
		}
	}

	return reset;
}

bool QuadTree::materialParamsNeedReset(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedReset();
}

void QuadTree::materialParamsNeedReset(bool b)
{
	getQuadrant()->getShaderTerrainData()->materialParamsNeedReset(b);
}

void QuadTree::updateCollider(void)
{
	if (!isValid())
		return;

	if (getQuadrant()->internalDataLocked())
		return;

	if (colliderNeedUpdate())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4 ") + __FUNCTION__, "Quadrant updateCollider");

		QuadrantPos pos = getQuadrant()->getPos();
		bool colliderCanHaveCollisionInfo = false;
		int64_t depthForCollider = m_viewer->Globals()->getParamDepthQuadWithCollisionInfo();
		if (depthForCollider == 0 || getQuadrant()->getLastDistanceInPerimeterFromCameraQuadrant() <= (size_t)depthForCollider)
			colliderCanHaveCollisionInfo = true;

		if (colliderCanHaveCollisionInfo)
		{
			m_worldQuadrant->getCollider()->exitWorld();
			m_worldQuadrant->getCollider()->deinit();
			m_worldQuadrant->getCollider()->init(m_GDN_Quadrant, 1, 1);
			m_worldQuadrant->getCollider()->enterWorld();
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 4.1.1 ") + __FUNCTION__, "QuadTree set data for collider");
				m_worldQuadrant->getCollider()->setData();
			}

			colliderNeedUpdate(false);
		}
	}
}

bool QuadTree::colliderNeedUpdate(void)
{
	return m_colliderNeedUpdate;
}

void QuadTree::colliderNeedUpdate(bool b)
{
	m_colliderNeedUpdate = b;
}

bool QuadTree::updateMaterialParams(void)
{
	bool updated = false;

	if (!isValid())
		return updated;

	if (getQuadrant()->internalDataLocked())
		return updated;

	//if ((int)m_viewer->Globals()->status() < (int)TheWorldStatus::worldDeployed)
	//	return updated;

	if (getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate())
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 5 ") + __FUNCTION__, "Quadrant updateMaterialParams");

		getQuadrant()->getShaderTerrainData()->updateMaterialParams(m_lookDev);

		updated = true;

		getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(false);

	}

	return updated;
}

bool QuadTree::materialParamsNeedUpdate(void)
{
	return getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate();
}

void QuadTree::materialParamsNeedUpdate(bool b)
{
	getQuadrant()->getShaderTerrainData()->materialParamsNeedUpdate(b);
}

Transform3D QuadTree::getInternalGlobalTransform(void)
{
	Transform3D t(Basis(), Vector3((real_t)getQuadrant()->getPos().getLowerXGridVertex(), 0, (real_t)getQuadrant()->getPos().getLowerZGridVertex()));
	Transform3D gt = m_viewer->getInternalGlobalTransform();
	Transform3D t1 = gt * t;
	return t1;
}

void QuadTree::dump(void)
{
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint("====================================================================================================================================");

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

		int numChunks = 0;
		int numActiveChunks = 0;
		Chunk* cameraChunk = nullptr;
		for (int i = globals->lodMaxDepth(); i >= 0; i--)
		{
			int numChunksAtLod = 0;
			Chunk::MapChunk::iterator it = m_mapChunk.find(i);
			if (it != m_mapChunk.end())
				numChunksAtLod = int(m_mapChunk[i].size());
			int numChunksActiveAtLod = 0;
			for (Chunk::MapChunkPerLod::iterator it1 = m_mapChunk[i].begin(); it1 != m_mapChunk[i].end(); it1++)
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
		ForAllChunk(action);
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
	m_regularMaterial.unref();
	m_lookDevMaterial.unref();
	m_heightmapImage.unref();
	m_heightmapTexture.unref();
	m_normalmapImage.unref();
	m_normalmapTexture.unref();
	m_splatmapImage.unref();
	m_splatmapTexture.unref();
	m_globalmapImage.unref();
	m_globalmapTexture.unref();
	m_colormapImage.unref();
	m_colormapTexture.unref();
}

void ShaderTerrainData::init(void)
{
}

godot::Ref<godot::Material> ShaderTerrainData::getRegularMaterial(bool reload)
{
	freeLookDevSubViewport();

	if (m_regularMaterial == nullptr || reload)
	{
		m_viewer->getMainProcessingMutex().lock();
		godot::Ref<godot::ShaderMaterial> mat = memnew(godot::ShaderMaterial);
		m_viewer->getMainProcessingMutex().unlock();
		godot::ResourceLoader* resLoader = godot::ResourceLoader::get_singleton();
		String shaderPath = "res://addons/twviewer/shaders/regularChunk.gdshader";		// from simple4.gdshader
		godot::Ref<godot::Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		mat->set_shader(shader);
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		m_regularMaterial = mat;
	}
	
	return m_regularMaterial;
}

void ShaderTerrainData::releaseRegularMaterial(void)
{
	m_regularMaterial.unref();
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
		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		mat->set_shader(shader);
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
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
		Ref<Shader> shader = resLoader->load(shaderPath, "", godot::ResourceLoader::CacheMode::CACHE_MODE_IGNORE);
		mat->set_shader(shader);
		float numVerticesPerChuckSide = (float)m_viewer->Globals()->numVerticesPerChuckSide();
		float gridStepInWU = m_viewer->Globals()->gridStepInWU();
		float lodGridStepInWU = m_viewer->Globals()->gridStepInHeightmapWUs(lod);
		float lodChunkSizeInWu = numVerticesPerChuckSide * lodGridStepInWU;
		mat->set_shader_parameter(SHADER_PARAM_LOOKDEV_LOD, float(lod));
		mat->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, numVerticesPerChuckSide);
		mat->set_shader_parameter(SHADER_PARAM_LOD_GRID_STEP, lodGridStepInWU);
		mat->set_shader_parameter(SHADER_PARAM_LOD_CHUNK_SIZE, lodChunkSizeInWu);
		mat->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
		mat->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		m_lookDevChunkLodMaterials[lod] = mat;
	}

	return m_lookDevChunkLodMaterials[lod];
}

std::map<std::string, std::unique_ptr<ShaderTerrainData::GroundTexture>> ShaderTerrainData::s_groundTextures;
bool ShaderTerrainData::s_groundTexturesNeedProcessing = false;
//std::recursive_mutex ShaderTerrainData::s_groundTexturesMutex;

godot::Ref<godot::Image> ShaderTerrainData::readGroundTexture(godot::String fileName, bool& ok, GDN_TheWorld_Viewer* viewer)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1 ") + __FUNCTION__, "ALL");

	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	godot::Error e;

	ok = true;
	
	const char* s = fileName.utf8().get_data();
	std::string _fileName = std::string(s);

	const godot::String webpFilename = fileName + ".cache_image";
	const char* s1 = webpFilename.utf8().get_data();
	std::string _webpFilename = std::string(s1);

	const godot::String pngFilename = fileName + ".png";
	const char* s2 = pngFilename.utf8().get_data();
	std::string _pngFilename = std::string(s2);

	if (viewer != nullptr)
		viewer->Globals()->debugPrint((std::string("Reading ground texture ") + _fileName + " ...").c_str());

	if (viewer != nullptr)
		viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::FileAccess> file = godot::FileAccess::open(fileName, godot::FileAccess::READ);
	if (file == nullptr)
		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error reading file ") + _fileName + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
	if (viewer != nullptr)
		viewer->getMainProcessingMutex().unlock();

	int64_t fileSize = file->get_length();
	size_t imageSize = (size_t)sqrt(fileSize / sizeof(struct TheWorld_Utils::_RGBA));
	assert(imageSize * imageSize * sizeof(struct TheWorld_Utils::_RGBA) == fileSize);
	if (imageSize * imageSize * sizeof(struct TheWorld_Utils::_RGBA) != fileSize)
		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Wrong image size ") + std::to_string(imageSize) + " file size " + std::to_string(fileSize)).c_str()));

	if (viewer != nullptr)
		viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::Image> image = godot::Image::create((int32_t)imageSize, (int32_t)imageSize, true, godot::Image::FORMAT_RGBA8);
	if (viewer != nullptr)
		viewer->getMainProcessingMutex().unlock();

	if (godot::FileAccess::file_exists(webpFilename))
	{
		file->close();

		//ResourceLoader* resLoader = ResourceLoader::get_singleton();
		//godot::Ref<godot::Texture2D> tex = resLoader->load(webpFilename);
		//image = tex->get_image();
		{
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().lock();
			godot::Ref<godot::FileAccess> file = godot::FileAccess::open(webpFilename, godot::FileAccess::READ);
			if (file == nullptr)
				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error reading file ") + _webpFilename + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().unlock();
			uint64_t size = file->get_64();
			godot::PackedByteArray array = file->get_buffer(size);
			file->close();

			e = image->load_webp_from_buffer(array);
			if (e != godot::Error::OK)
				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("reading image error ") + std::to_string(int(e))).c_str()));
		}
	}
	else
	{
		godot::PackedByteArray imageRowBuffer;
		size_t imageRowSize = imageSize * sizeof(struct TheWorld_Utils::_RGBA);

		for (size_t y = 0; y < imageSize; y++)
		{
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.1 ") + __FUNCTION__, "read file from disk");
				imageRowBuffer = file->get_buffer(imageRowSize);
			}
			assert(imageRowBuffer.size() == imageRowSize);

			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.2 ") + __FUNCTION__, "set_pixel");

				size_t idx = 0;
				for (size_t x = 0; x < imageSize; x++)
				{
					godot::Color c;
					c.r = float(imageRowBuffer[int(idx)]) / 255;
					c.g = float(imageRowBuffer[int(idx) + 1]) / 255;
					c.b = float(imageRowBuffer[int(idx) + 2]) / 255;
					c.a = float(imageRowBuffer[int(idx) + 3]) / 255;
					idx += 4;
					image->set_pixel((int32_t)x, (int32_t)y, c);
				}
			}
		}

		file->close();

		e = image->save_png(pngFilename);

		//e = image->save_webp(webpFilename);
		{
			godot::PackedByteArray array = image->save_webp_to_buffer();

			if (viewer != nullptr)
				viewer->getMainProcessingMutex().lock();
			godot::Ref<godot::FileAccess> file = godot::FileAccess::open(webpFilename, godot::FileAccess::WRITE);
			if (file == nullptr)
				throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing file ") + _webpFilename + " error " + std::to_string(int(godot::FileAccess::get_open_error()))).c_str()));
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().unlock();
			uint64_t size = array.size();
			file->store_64(size);
			file->store_buffer(array);
			file->close();
		}
	}
	
	{
		//TheWorld_Utils::GuardProfiler profiler(std::string("readGroundTexture 1.3 ") + __FUNCTION__, "generate_mipmaps");
		e = image->generate_mipmaps();
		if (e != godot::Error::OK)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("generate_mipmaps error ") + std::to_string(int(e))).c_str()));

	}

	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
	long long elapsed = (now - start).count();

	if (viewer != nullptr)
		viewer->Globals()->debugPrint((std::string("Reading ground texture ") + _fileName + " done! - Elapsed " + std::to_string(elapsed)  + " ms").c_str());

	return image;
}

void ShaderTerrainData::getGroundTextures(godot::String dirName, GDN_TheWorld_Viewer* viewer)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTextures 1 ") + __FUNCTION__, "all");
	
	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());

	if (viewer != nullptr)
		viewer->Globals()->debugPrint("Reading ground textures");

	godot::String globalizedDirName = ProjectSettings::get_singleton()->globalize_path(dirName);
	
	std::map<std::string, bool> textures;

	std::string dir = std::string(globalizedDirName.utf8().get_data());
	if (!fs::exists(dir))
		return;

	for (const auto& entry : fs::directory_iterator(dir))
		if (entry.is_regular_file())
		{
			bool isTexture = false;
			std::string filename = entry.path().filename().generic_string();
			if (filename.ends_with(GDN_TheWorld_Globals::c_albedo_bump_file_ext))
			{
				filename = filename.substr(0, filename.length() - GDN_TheWorld_Globals::c_albedo_bump_file_ext.length());
				isTexture = true;
			}
			else if (filename.ends_with(GDN_TheWorld_Globals::c_normal_roughness_file_ext))
			{
				filename = filename.substr(0, filename.length() - GDN_TheWorld_Globals::c_normal_roughness_file_ext.length());
				isTexture = true;
			}
			
			if (isTexture)
			{
				std::string albedo_bump_filename = dir + filename + GDN_TheWorld_Globals::c_albedo_bump_file_ext;
				std::string normal_roughness_filename = dir + filename + GDN_TheWorld_Globals::c_normal_roughness_file_ext;
				if (fs::exists(albedo_bump_filename) && fs::exists(normal_roughness_filename))
				{
					if (!textures.contains(filename))
						textures[filename] = true;
				}
			}
		}

	for (const auto& texture : textures)
	{
		//std::lock_guard<std::recursive_mutex> lock(s_groundTexturesMutex);

		if (!s_groundTextures.contains(texture.first))
		{
			s_groundTextures[texture.first] = std::make_unique<ShaderTerrainData::GroundTexture>();
			s_groundTexturesNeedProcessing = true;
			//ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[texture.first].get();
			//getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + texture.first.c_str(), groundTexture, viewer);
		}
	}

	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now()); 
	long long elapsed = (now - start).count();

	if (viewer != nullptr)
		viewer->Globals()->debugPrint((std::string("Reading ground textures done! - Elapsed ") + std::to_string(elapsed) + " ms").c_str());
}

void ShaderTerrainData::getGroundTexture(godot::String fileName, ShaderTerrainData::GroundTexture* groundTexture, GDN_TheWorld_Viewer* viewer)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1 ") + __FUNCTION__, "all");

	godot::ResourceLoader* resLoader = ResourceLoader::get_singleton();

	try
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1.1 ") + __FUNCTION__, "albedo_bump");

		godot::String path = fileName + GDN_TheWorld_Globals::c_albedo_bump_file_ext.c_str();
		std::string _path = std::string(path.utf8().get_data());

		Ref<godot::CompressedTexture2D> c_tex = nullptr;
		//c_tex = resLoader->load(path + ".png");		// must reload as image to create mipmaps (?)
		if (c_tex == nullptr)
		{
			bool ok;
			godot::Ref<godot::Image> image = readGroundTexture(path, ok, viewer);
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().lock();
			// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
			//tex->create_from_image(image, godot::ImageTexture::FLAG_FILTER | godot::ImageTexture::FLAG_MIPMAPS | godot::ImageTexture::FLAG_REPEAT);
			Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().unlock();
			groundTexture->m_albedo_bump_tex = tex;
			groundTexture->m_tex = resLoader->load(path + ".png");
		}
		else
			groundTexture->m_albedo_bump_tex = c_tex;

	}
	catch (...) {}

	try
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTexture 1.2 ") + __FUNCTION__, "normal_roughness");

		godot::String path = fileName + GDN_TheWorld_Globals::c_normal_roughness_file_ext.c_str();
		std::string _path = std::string(path.utf8().get_data());

		Ref<godot::CompressedTexture2D> c_tex = nullptr;
		//c_tex = resLoader->load(path + ".png");		// must reload as image to create mipmaps (?)
		if (c_tex == nullptr)
		{
			bool ok;
			godot::Ref<godot::Image> image = readGroundTexture(path, ok, viewer);
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().lock();
			// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
			//tex->create_from_image(image, godot::ImageTexture::FLAG_FILTER | godot::ImageTexture::FLAG_MIPMAPS | godot::ImageTexture::FLAG_REPEAT);
			Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
			if (viewer != nullptr)
				viewer->getMainProcessingMutex().unlock();
			groundTexture->m_normal_roughness_tex = tex;
		}
		else
			groundTexture->m_normal_roughness_tex = c_tex;
	}
	catch (...) {}

	groundTexture->initialized = true;
}

void ShaderTerrainData::mouseHitChanged(godot::Vector2 mouseHit)
{
	enum class ShaderTerrainData::LookDev lookdev = m_quadTree->getLookDev();

	if (lookdev == LookDev::ChunkLod)
	{
		std::map<int, godot::Ref<godot::ShaderMaterial>>& lookDevChunkLodMaterials = getLookDevChunkLodMaterials();
		for (auto& item : lookDevChunkLodMaterials)
			if (item.second != nullptr && m_viewer->positionVisible())
				item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, mouseHit);
			else
				item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
	}
	else
	{
		godot::Ref<godot::ShaderMaterial> currentMaterial = (lookdev == LookDev::NotSet ? getRegularMaterial() : getLookDevMaterial(lookdev));
		if (currentMaterial != nullptr && m_viewer->positionVisible())
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, mouseHit);
		else
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
	}
}

void ShaderTerrainData::releaseGlobals(void)
{
	//std::lock_guard<std::recursive_mutex> lock(s_groundTexturesMutex);

	for (auto& it : s_groundTextures)
	{
		it.second->m_albedo_bump_tex.unref();
		it.second->m_normal_roughness_tex.unref();
	}

	s_groundTextures.clear();
}

godot::Ref<godot::ImageTexture> ShaderTerrainData::getAlbedoBumpTexture(std::string textureName)
{
	std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
	std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);
	
	if (!s_groundTextures.contains(textureName) || !s_groundTextures[textureName]->initialized)
	{
		s_groundTextures[textureName] = std::make_unique<ShaderTerrainData::GroundTexture>();
		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[textureName].get();
		getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + textureName.c_str(), groundTexture, m_viewer);
	}

	return s_groundTextures[textureName]->m_albedo_bump_tex;
}

godot::Ref<godot::ImageTexture> ShaderTerrainData::getNormalRoughnessTexture(std::string textureName)
{
	std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
	std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);

	if (!s_groundTextures.contains(textureName) || !s_groundTextures[textureName]->initialized)
	{
		s_groundTextures[textureName] = std::make_unique<ShaderTerrainData::GroundTexture>();
		ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[textureName].get();
		getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + textureName.c_str(), groundTexture, m_viewer);
	}

	return s_groundTextures[textureName]->m_normal_roughness_tex;
}

// it is expected that globals and World Datas are loaded
// TODORIC: maybe usefull for performance reasons specify which texture need update and which rect of the texture 
void ShaderTerrainData::resetMaterialParams(LookDev lookdev)
{
	TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3 ") + __FUNCTION__, "Quadrant resetMaterialParams");

	std::recursive_mutex& quadrantMutex = m_quadTree->getQuadrantMutex();
	std::lock_guard<std::recursive_mutex> lock(quadrantMutex);
	
	int _resolution = m_viewer->Globals()->heightmapResolution() + 1;

	TheWorld_Utils::TerrainEdit* terrainEdit = m_quadTree->getQuadrant()->getTerrainEdit();

	std::string lowElevationTexName = terrainEdit->extraValues.lowElevationTexName_r;
	if (lowElevationTexName.size() == 0)
	{
		terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
		//lowElevationTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
		lowElevationTexName = terrainEdit->extraValues.lowElevationTexName_r;
	}
	std::string highElevationTexName = terrainEdit->extraValues.highElevationTexName_g;
	if (highElevationTexName.size() == 0)
	{
		terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::highElevation);
		//highElevationTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::highElevation);
		highElevationTexName = terrainEdit->extraValues.highElevationTexName_g;
	}
	std::string dirtTexName = terrainEdit->extraValues.dirtTexName_b;
	if (dirtTexName.size() == 0)
	{
		terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::dirt);
		//dirtTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::dirt);
		dirtTexName = terrainEdit->extraValues.dirtTexName_b;
	}
	std::string rocksTexName = terrainEdit->extraValues.rocksTexName_a;
	if (rocksTexName.size() == 0)
	{
		terrainEdit->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::rocks);
		//rocksTexName = TheWorld_Utils::TerrainEdit::getDefaultTextureNameForTerrainType(terrainEdit->terrainType, TheWorld_Utils::TerrainEdit::TextureType::rocks);
		rocksTexName = terrainEdit->extraValues.rocksTexName_a;
	}

	m_viewer->getMainProcessingMutex().lock();
	
	{
		//std::lock_guard<std::recursive_mutex> lock(s_groundTexturesMutex);
	
		if (!s_groundTextures.contains(lowElevationTexName) || !s_groundTextures[lowElevationTexName]->initialized)
		{
			s_groundTextures[lowElevationTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[lowElevationTexName].get();
			//std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
			//std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);
			getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + lowElevationTexName.c_str(), groundTexture, m_viewer);
		}

		if (!s_groundTextures.contains(highElevationTexName) || !s_groundTextures[highElevationTexName]->initialized)
		{
			s_groundTextures[highElevationTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[highElevationTexName].get();
			//std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
			//std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);
			getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + highElevationTexName.c_str(), groundTexture, m_viewer);
		}

		if (!s_groundTextures.contains(dirtTexName) || !s_groundTextures[dirtTexName]->initialized)
		{
			s_groundTextures[dirtTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[dirtTexName].get();
			//std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
			//std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);
			getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + dirtTexName.c_str(), groundTexture, m_viewer);
		}

		if (!s_groundTextures.contains(rocksTexName) || !s_groundTextures[rocksTexName]->initialized)
		{
			s_groundTextures[rocksTexName] = std::make_unique<ShaderTerrainData::GroundTexture>();
			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[rocksTexName].get();
			//std::recursive_mutex& viewerMainProcessingMutex = m_viewer->getMainProcessingMutex();
			//std::lock_guard<std::recursive_mutex> lock(viewerMainProcessingMutex);
			getGroundTexture(godot::String(GDN_TheWorld_Globals::c_groundTexturesDir.c_str()) + rocksTexName.c_str(), groundTexture, m_viewer);
		}
	}

	m_viewer->getMainProcessingMutex().unlock();

	size_t new_resolution;
	size_t optimizationLevel = m_quadTree->getQuadrant()->optimizationLevel();
	if (optimizationLevel > 0)
	{
		size_t new_resolution_shift = m_viewer->Globals()->heightmapResolutionShift() - optimizationLevel;
		new_resolution = (1 << new_resolution_shift) + 1;
	}
	bool optimizationLevelChanged = m_quadTree->getQuadrant()->optimizationLevelChanged();

	// if heights updated create texture for shader (m_heightmapTexture), it will be freed we passed to the shader
	if (m_quadTree->getQuadrant()->heightsUpdated() || optimizationLevelChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1 ") + __FUNCTION__, "Heightmap");

		if (m_quadTree->getQuadrant()->heightsUpdated() || m_heightmapImage == nullptr)
		{
			if (m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size() > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1.1 ") + __FUNCTION__, "Create Image from heights buffer");

				TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
				godot::PackedByteArray data;
				size_t heightmapBufferSizeInBytes = _resolution * _resolution * uint16_t_size;
				data.resize((int)heightmapBufferSizeInBytes);
				{
					size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
					assert(heightmapBufferSizeInBytes == heightsBufferSize);
					memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
				}
				m_viewer->incNumTextureResize();
				m_viewer->getMainProcessingMutex().lock();
				godot::Ref<godot::Image> im = godot::Image::create_from_data(_resolution, _resolution, false, Image::FORMAT_RH, data);
				m_viewer->getMainProcessingMutex().unlock();
				m_heightmapImage = im;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->addToTextureResizingElapsed((now - start).count());
			}
		}
		
		if (m_heightmapImage != nullptr)
		{
			godot::Ref<godot::Image> image;

			if (optimizationLevel > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1.2 ") + __FUNCTION__, "resize image " + std::to_string(_resolution) + " ==> " + std::to_string(new_resolution));

				//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".heightmap." + std::to_string(_resolution) + ".png";		// DEBUG
				//image->save_png(outFileName.c_str());																								// DEBUG

				// doing a copy to resize
				m_viewer->getMainProcessingMutex().lock();
				Ref<Image> im = godot::Image::create_from_data(m_heightmapImage->get_width(), m_heightmapImage->get_height(), false, m_heightmapImage->get_format(), m_heightmapImage->get_data());
				m_viewer->getMainProcessingMutex().unlock();

				im->resize((int32_t)new_resolution, (int32_t)new_resolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

				//outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".heightmap." + std::to_string(new_resolution) + ".png";		// DEBUG
				//image->save_png(outFileName.c_str());		
				
				image = im;
			}
			else
				image = m_heightmapImage;

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.1.3 ") + __FUNCTION__, "Create Texture from heights image");

				m_viewer->getMainProcessingMutex().lock();
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				m_viewer->getMainProcessingMutex().unlock();
				m_heightmapTexture = tex;
				m_heightMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
			}

			if (!m_viewer->Globals()->getParamCacheTerrainTextures())
				m_heightmapImage.unref();
		}

		m_quadTree->getQuadrant()->setHeightsUpdated(false);
	}

	// if normals updated create texture for shader (m_normalmapTexture), it will be freed we passed to the shader. If normals have not been computed we fill the texture with Color(0.5f, 0.5f, 1.0f, 0.0f) (rgb 127, 127, 255, 0 ==> packed vertical normal pointing up)
	if (m_quadTree->getQuadrant()->normalsUpdated() || optimizationLevelChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2 ") + __FUNCTION__, "Normalmap");

		if (m_quadTree->getQuadrant()->normalsUpdated() || m_normalmapImage == nullptr)
		{
			TheWorld_Utils::MemoryBuffer& normalsBuffer = m_quadTree->getQuadrant()->getNormalsBuffer();
			size_t normalsBufferSize = normalsBuffer.size();

			if (normalsBufferSize == 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2.1 ") + __FUNCTION__, "Create empty Image for normals");
				m_viewer->getMainProcessingMutex().lock();
				godot::Ref<godot::Image> im = godot::Image::create(_resolution, _resolution, false, godot::Image::FORMAT_RGB8);
				m_viewer->getMainProcessingMutex().unlock();
				im->fill(godot::Color(0.5f, 0.5f, 1.0f, 0.0f));	// packed noromal vertical pointing up
				m_normalmapImage = im;
			}
			else
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2.1 ") + __FUNCTION__, "Create Image from normal buffer");

				TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				godot::PackedByteArray data;
				size_t normalmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
				data.resize((int)normalmapBufferSizeInBytes);
				assert(normalmapBufferSizeInBytes == normalsBufferSize);
				{
					memcpy((char*)data.ptrw(), normalsBuffer.ptr(), normalsBufferSize);
				}
				m_viewer->incNumTextureResize();
				m_viewer->getMainProcessingMutex().lock();
				godot::Ref<godot::Image> im = godot::Image::create_from_data(_resolution, _resolution, false, Image::FORMAT_RGB8, data);
				m_viewer->getMainProcessingMutex().unlock();
				m_normalmapImage = im;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->addToTextureResizingElapsed((now - start).count());

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
		
		if (m_normalmapImage != nullptr)
		{
			godot::Ref<godot::Image> image;

			if (optimizationLevel > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2.2 ") + __FUNCTION__, "resize image " + std::to_string(_resolution) + " ==> " + std::to_string(new_resolution));

				m_viewer->getMainProcessingMutex().lock();
				Ref<Image> im = godot::Image::create_from_data(m_normalmapImage->get_width(), m_normalmapImage->get_height(), false, m_normalmapImage->get_format(), m_normalmapImage->get_data());
				m_viewer->getMainProcessingMutex().unlock();

				im->resize((int32_t)new_resolution, (int32_t)new_resolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

				image = im;
			}
			else
				image = m_normalmapImage;

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.2.3 ") + __FUNCTION__, "Create Texture from normal image");

				m_viewer->getMainProcessingMutex().lock();
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				m_viewer->getMainProcessingMutex().unlock();
				m_normalmapTexture = tex;
				m_normalMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
			}

			if (!m_viewer->Globals()->getParamCacheTerrainTextures())
				m_normalmapImage.unref();
		}

		m_quadTree->getQuadrant()->setNormalsUpdated(false);
	}

	if (!m_quadTree->getQuadrant()->renderGlobalMap() && (m_quadTree->getQuadrant()->splatmapUpdated() || m_quadTree->getQuadrant()->renderGlobalMapChanged() || optimizationLevelChanged))
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3 ") + __FUNCTION__, "Splatmap");

		if (m_quadTree->getQuadrant()->splatmapUpdated() || m_splatmapImage == nullptr)
		{
			TheWorld_Utils::MemoryBuffer& splatmapBuffer = m_quadTree->getQuadrant()->getSplatmapBuffer();
			size_t splatmapBufferSize = splatmapBuffer.size();

			//splatmapBufferSize = 0;
			if (splatmapBufferSize == 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3.1 ") + __FUNCTION__, "Create empty Image for splatmap");
				m_viewer->getMainProcessingMutex().lock();
				Ref<godot::Image> im = godot::Image::create(_resolution, _resolution, false, godot::Image::FORMAT_RGBA8);
				m_viewer->getMainProcessingMutex().unlock();
				im->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));		// first texture TheWorld_Utils::TerrainEdit::extraValues.lowElevationTexName_r
				m_splatmapImage = im;
			}
			else
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3.1 ") + __FUNCTION__, "Create Image from splatmap buffer");

				TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				godot::PackedByteArray data;
				size_t splatmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
				data.resize((int)splatmapBufferSizeInBytes);
				assert(splatmapBufferSizeInBytes == splatmapBufferSize);
				{
					memcpy((char*)data.ptrw(), splatmapBuffer.ptr(), splatmapBufferSizeInBytes);
				}
				m_viewer->incNumTextureResize();
				m_viewer->getMainProcessingMutex().lock();
				Ref<godot::Image> im = godot::Image::create_from_data(_resolution, _resolution, false, Image::FORMAT_RGBA8, data);
				m_viewer->getMainProcessingMutex().unlock();
				m_splatmapImage = im;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->addToTextureResizingElapsed((now - start).count());
			}
		}

		if (m_splatmapImage != nullptr)
		{
			godot::Ref<godot::Image> image;

			if (optimizationLevel > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3.2 ") + __FUNCTION__, "resize image " + std::to_string(_resolution) + " ==> " + std::to_string(new_resolution));

				m_viewer->getMainProcessingMutex().lock();
				Ref<Image> im = godot::Image::create_from_data(m_splatmapImage->get_width(), m_splatmapImage->get_height(), false, m_splatmapImage->get_format(), m_splatmapImage->get_data());
				m_viewer->getMainProcessingMutex().unlock();

				im->resize((int32_t)new_resolution, (int32_t)new_resolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

				image = im;
			}
			else
				image = m_splatmapImage;
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.3.3 ") + __FUNCTION__, "Create Texture from splatmap image");

				m_viewer->getMainProcessingMutex().lock();
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				m_viewer->getMainProcessingMutex().unlock();
				m_splatmapTexture = tex;
				m_splatMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_SPLATMAP, m_splatmapTexture);
			}

			if (!m_viewer->Globals()->getParamCacheTerrainTextures())
				m_splatmapImage.unref();
		}

		m_quadTree->getQuadrant()->renderGlobalMapChanged(false);
		m_quadTree->getQuadrant()->setSplatmapUpdated(false);
	}

	if (m_quadTree->getQuadrant()->renderGlobalMap() && (m_quadTree->getQuadrant()->globalmapUpdated() || m_quadTree->getQuadrant()->renderGlobalMapChanged() || optimizationLevelChanged))
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4 ") + __FUNCTION__, "Globalmap");

		if (m_quadTree->getQuadrant()->globalmapUpdated() || m_globalmapImage == nullptr)
		{
			TheWorld_Utils::MemoryBuffer& globalmapBuffer = m_quadTree->getQuadrant()->getGlobalmapBuffer();
			size_t globalmapBufferSize = globalmapBuffer.size();

			if (globalmapBufferSize == 0)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4.1 ") + __FUNCTION__, "Create empty Image for globalmap");
				//m_viewer->getMainProcessingMutex().lock();
				//godot::Ref<godot::Image> im = godot::Image::create(_resolution, _resolution, false, Image::FORMAT_RGB8);
				//m_viewer->getMainProcessingMutex().unlock();
				//image = im;
				//image->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4.1 ") + __FUNCTION__, "Create Image from globalmap buffer");

				TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				godot::PackedByteArray data;
				size_t globalmapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGB);
				data.resize((int)globalmapBufferSizeInBytes);
				assert(globalmapBufferSizeInBytes == globalmapBufferSize);
				{
					memcpy((char*)data.ptrw(), globalmapBuffer.ptr(), globalmapBufferSizeInBytes);
				}
				m_viewer->incNumTextureResize();
				m_viewer->getMainProcessingMutex().lock();
				godot::Ref<godot::Image> im = godot::Image::create_from_data(_resolution, _resolution, false, Image::FORMAT_RGB8, data);
				m_viewer->getMainProcessingMutex().unlock();
				m_globalmapImage = im;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->addToTextureResizingElapsed((now - start).count());
			}
		}

		if (m_globalmapImage != nullptr)
		{
			Ref<Image> image;

			if (optimizationLevel > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4.2 ") + __FUNCTION__, "resize image " + std::to_string(_resolution) + " ==> " + std::to_string(new_resolution));

				m_viewer->getMainProcessingMutex().lock();
				Ref<Image> im = godot::Image::create_from_data(m_globalmapImage->get_width(), m_globalmapImage->get_height(), false, m_globalmapImage->get_format(), m_globalmapImage->get_data());
				m_viewer->getMainProcessingMutex().unlock();

				//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".globalmap." + std::to_string(_resolution) + ".png";		// DEBUG
				//im->save_png(outFileName.c_str());																									// DEBUG

				im->resize((int32_t)new_resolution, (int32_t)new_resolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

				//outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".globalmap." + std::to_string(new_resolution) + ".png";				// DEBUG
				//im->save_png(outFileName.c_str());																									// DEBUG

				image = im;
			}
			else
				image = m_globalmapImage;

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.4.3 ") + __FUNCTION__, "Create Texture from globalmap image");

				m_viewer->getMainProcessingMutex().lock();
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER | godot::Texture::FLAG_MIPMAPS);
				godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				m_viewer->getMainProcessingMutex().unlock();
				m_globalmapTexture = tex;
				m_globalMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
			}

			if (!m_viewer->Globals()->getParamCacheTerrainTextures())
				m_globalmapImage.unref();
		}

		m_quadTree->getQuadrant()->renderGlobalMapChanged(false);
		m_quadTree->getQuadrant()->setGlobalmapUpdated(false);
	}

	if (m_quadTree->getQuadrant()->colorsUpdated() || optimizationLevelChanged)
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5 ") + __FUNCTION__, "Colormap");

		if (m_quadTree->getQuadrant()->colorsUpdated() || m_colormapImage == nullptr)
		{
			TheWorld_Utils::MemoryBuffer& colormapBuffer = m_quadTree->getQuadrant()->getColormapBuffer();
			size_t colormapBufferSize = colormapBuffer.size();

			if (colormapBufferSize == 0)
			{
				//TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5.1 ") + __FUNCTION__, "Create empty Image for colors");
				//m_viewer->getMainProcessingMutex().lock();
				//godot::Ref<godot::Image> im = godot::Image::create(_resolution, _resolution, false, Image::FORMAT_RGBA8);
				//m_viewer->getMainProcessingMutex().unlock();
				//image = im;
				//image->fill(godot::Color(1.0f, 1.0f, 1.0f, 1.0f));
			}
			else
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5.1 ") + __FUNCTION__, "Create Image from colormap buffer");

				TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				godot::PackedByteArray data;
				size_t colormapBufferSizeInBytes = _resolution * _resolution * sizeof(struct TheWorld_Utils::_RGBA);
				data.resize((int)colormapBufferSizeInBytes);
				assert(colormapBufferSizeInBytes == colormapBufferSize);
				{
					memcpy((char*)data.ptrw(), colormapBuffer.ptr(), colormapBufferSizeInBytes);
				}
				m_viewer->incNumTextureResize();
				m_viewer->getMainProcessingMutex().lock();
				godot::Ref<godot::Image> im = godot::Image::create_from_data(_resolution, _resolution, false, Image::FORMAT_RGBA8, data);
				m_viewer->getMainProcessingMutex().unlock();
				m_colormapImage = im;
				TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
				m_viewer->addToTextureResizingElapsed((now - start).count());
			}
		}

		if (m_colormapImage != nullptr)
		{
			Ref<Image> image;

			if (optimizationLevel > 0)
			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5.2 ") + __FUNCTION__, "resize image " + std::to_string(_resolution) + " ==> " + std::to_string(new_resolution));

				m_viewer->getMainProcessingMutex().lock();
				Ref<Image> im = godot::Image::create_from_data(m_colormapImage->get_width(), m_colormapImage->get_height(), false, m_colormapImage->get_format(), m_colormapImage->get_data());
				m_viewer->getMainProcessingMutex().unlock();

				im->resize((int32_t)new_resolution, (int32_t)new_resolution, godot::Image::Interpolation::INTERPOLATE_BILINEAR);

				image = im;
			}
			else
				image = m_colormapImage;

			{
				TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 3.5.3 ") + __FUNCTION__, "Create Texture from colormap image");

				m_viewer->getMainProcessingMutex().lock();
				// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
				//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
				godot::Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
				m_viewer->getMainProcessingMutex().unlock();
				m_colormapTexture = tex;
				m_colorMapTexModified = true;
				//debugPrintTexture(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
			}

			if (!m_viewer->Globals()->getParamCacheTerrainTextures())
				m_colormapImage.unref();
		}

		m_quadTree->getQuadrant()->setColorsUpdated(false);
	}

	// _update_all_vertical_bounds ???	// TODORIC mah
	//	# RGF image where R is min heightand G is max height
	//	var _chunked_vertical_bounds : = Image.new()	// _chunked_vertical_bounds.create(csize_x, csize_y, false, Image.FORMAT_RGF)
	//	_update_vertical_bounds(0, 0, _resolution - 1, _resolution - 1)

	if (optimizationLevelChanged)
		m_quadTree->getQuadrant()->optimizationLevelChanged(false);

	materialParamsNeedUpdate(true);
}

//Color ShaderTerrainData::encodeNormal(Vector3 normal)
//{
//	normal = 0.5 * (normal + Vector3::ONE);
//	return Color(normal.x, normal.z, normal.y);
//}

void ShaderTerrainData::updateMaterialParams(LookDev lookdev)
{
	// Completare da _update_material_params
	//m_viewer->Globals()->debugPrint("Updating terrain material params");

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

		TheWorld_Utils::TerrainEdit* terrainEdit = m_quadTree->getQuadrant()->getTerrainEdit();

		if (lookdev == LookDev::ChunkLod)
		{
			std::map<int, godot::Ref<godot::ShaderMaterial>>& lookDevChunkLodMaterials = getLookDevChunkLodMaterials();

			for (auto& item : lookDevChunkLodMaterials)
			{
				item.second->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
				item.second->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);
				item.second->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, editMode);
				item.second->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
				item.second->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_RADIUS, m_viewer->getShaderParamMouseHitRadius());
				item.second->set_shader_parameter(SHADER_PARAM_OPTIMIZATION_LEVEL, m_quadTree->getQuadrant()->optimizationLevel());
				if (m_heightMapTexModified && m_heightmapTexture != nullptr)
					item.second->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
		
				if (m_normalMapTexModified && m_normalmapTexture != nullptr)
					item.second->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
			}
		}
		else
		{
			godot::Ref<godot::ShaderMaterial> currentMaterial;
			if (lookdev == LookDev::NotSet)
				currentMaterial = getRegularMaterial();
			else
				currentMaterial = getLookDevMaterial(lookdev);

			currentMaterial->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
			currentMaterial->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);
			currentMaterial->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, editMode);
			currentMaterial->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());
			currentMaterial->set_shader_parameter(SHADER_PARAM_MOUSE_HIT_RADIUS, m_viewer->getShaderParamMouseHitRadius());

			m_viewer->getMainProcessingMutex().lock();
			if (m_quadTree->getQuadrant()->textureUpdated() && lookdev == LookDev::NotSet)
			{
				godot::Ref<godot::ImageTexture> lowElevation_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.lowElevationTexName_r]->m_albedo_bump_tex;
				if (lowElevation_albedo_bump_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_0));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_0, lowElevation_albedo_bump_tex);
				}

				godot::Ref<godot::ImageTexture> lowElevation_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.lowElevationTexName_r]->m_normal_roughness_tex;
				if (lowElevation_normal_roughness_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_0, lowElevation_normal_roughness_tex);
				}

				godot::Ref<godot::ImageTexture> highElevation_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.highElevationTexName_g]->m_albedo_bump_tex;
				if (highElevation_albedo_bump_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_1));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_1, highElevation_albedo_bump_tex);
				}

				godot::Ref<godot::ImageTexture> highElevation_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.highElevationTexName_g]->m_normal_roughness_tex;
				if (highElevation_normal_roughness_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_1, highElevation_normal_roughness_tex);
				}

				godot::Ref<godot::ImageTexture> dirt_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.dirtTexName_b]->m_albedo_bump_tex;
				if (dirt_albedo_bump_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_2));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_2, dirt_albedo_bump_tex);
				}

				godot::Ref<godot::ImageTexture> dirt_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.dirtTexName_b]->m_normal_roughness_tex;
				if (dirt_normal_roughness_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_2, dirt_normal_roughness_tex);
				}

				godot::Ref<godot::ImageTexture> rocks_albedo_bump_tex = s_groundTextures[terrainEdit->extraValues.rocksTexName_a]->m_albedo_bump_tex;
				if (rocks_albedo_bump_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_ALBEDO_BUMP_3));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_ALBEDO_BUMP_3, rocks_albedo_bump_tex);
				}

				godot::Ref<godot::ImageTexture> rocks_normal_roughness_tex = s_groundTextures[terrainEdit->extraValues.rocksTexName_a]->m_normal_roughness_tex;
				if (rocks_normal_roughness_tex != nullptr)
				{
					//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3));
					currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_NORMAL_ROUGHNESS_3, rocks_normal_roughness_tex);
				}

				m_quadTree->getQuadrant()->setTextureUpdated(false);
			}
			m_viewer->getMainProcessingMutex().unlock();

			if (m_heightMapTexModified && m_heightmapTexture != nullptr)
			{
				//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_HEIGHTMAP));
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, m_heightmapTexture);
				m_heightMapTexModified = false;
			}

			if (m_normalMapTexModified && m_normalmapTexture != nullptr)
			{
				//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_NORMALMAP));
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, m_normalmapTexture);
				m_normalMapTexModified = false;
			}

			if (lookdev == LookDev::NotSet && m_globalMapTexModified && m_globalmapTexture != nullptr)
			{
				//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_GLOBALMAP));
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_GLOBALMAP, m_globalmapTexture);
				currentMaterial->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, true);
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_SPLATMAP, nullptr);
				m_globalMapTexModified = false;
			}

			if (lookdev == LookDev::NotSet && m_splatMapTexModified && m_splatmapTexture != nullptr)
			{
				//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_SPLATMAP));
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_SPLATMAP, m_splatmapTexture);
				currentMaterial->set_shader_parameter(SHADER_PARAM_USE_GLOBALMAP, false);
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_GLOBALMAP, nullptr);
				m_splatMapTexModified = false;
			}

			if (lookdev == LookDev::NotSet && m_colorMapTexModified && m_colormapTexture != nullptr)
			{
				//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_TERRAIN_COLORMAP));
				currentMaterial->set_shader_parameter(SHADER_PARAM_TERRAIN_COLORMAP, m_colormapTexture);
				m_colorMapTexModified = false;
			}

			godot::Plane ground_uv_scale = m_viewer->getShaderParamGroundUVScale();
			if (lookdev == LookDev::NotSet /* && ground_uv_scale.get_normal() != godot::Vector3(0.0f, 0.0f, 0.0f) && ground_uv_scale.d != 0.0f*/)
			{
				currentMaterial->set_shader_parameter(SHADER_PARAM_GROUND_UV_SCALE_PER_TEXTURE, ground_uv_scale);
			}

			currentMaterial->set_shader_parameter(SHADER_PARAM_OPTIMIZATION_LEVEL, m_quadTree->getQuadrant()->optimizationLevel());

			bool depthBlending = m_viewer->getShaderParamDepthBlening();
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

			if (lookdev == LookDev::Heights && m_heightmapTexture != nullptr)
			{
				currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_heightmapTexture);
			}

			if (lookdev == LookDev::Normals && m_normalmapTexture != nullptr)
			{
				currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_normalmapTexture);
			}

			if (lookdev == LookDev::Splat && m_splatmapTexture != nullptr)
			{
				currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_splatmapTexture);
			}

			if (lookdev == LookDev::Color && m_colormapTexture != nullptr)
			{
				currentMaterial->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, m_colormapTexture);
			}

			if (lookdev == LookDev::Global && m_globalmapTexture != nullptr)
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

			//Vector3 cameraPosViewerNodeLocalCoord = globalTransform.affine_inverse() * cameraPosGlobalCoord;	// Viewer Node (grid) local coordinates of the camera pos
		}

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

size_t QuadrantPos::distanceInPerimeter(QuadrantPos& q)
{
	size_t distanceOnX = (size_t)ceil(abs(q.getLowerXGridVertex() - getLowerXGridVertex()) / q.m_sizeInWU);
	size_t distanceOnZ = (size_t)ceil(abs(q.getLowerZGridVertex() - getLowerZGridVertex()) / q.m_sizeInWU);
	if (distanceOnX > distanceOnZ)
		return distanceOnX;
	else
		return distanceOnZ;
}

Quadrant::Quadrant(QuadrantPos& quadrantPos, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree)
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
	m_heightsUpdated = false;
	m_normalsUpdated = false;
	m_colorsUpdated = false;
	m_splatmapUpdated = false;
	m_globalmapUpdated = false;
	m_texturesUpdated = false;
	//m_terrainValuesCanBeCleared = false;
	m_needUploadToServer = false;
	m_internalDataLocked = false;
	m_empty = true;

	m_mapModder = memnew(GDN_TheWorld_MapModder);
	m_mapModder->custom_ready(m_viewer, m_viewer, false);
	godot::Error e = m_mapModder->connect("tw_modder_work_done", Callable(m_viewer, "quadrant_modder_work_done"));

	m_quadrantCenter = godot::Vector3(m_quadrantPos.getLowerXGridVertex(), 0, m_quadrantPos.getLowerZGridVertex()) + godot::Vector3(m_viewer->Globals()->heightmapSizeInWUs(), 0, m_viewer->Globals()->heightmapSizeInWUs()) / 2;
}

Quadrant::~Quadrant()
{
	if (m_mapModder != nullptr)
	{
		Node* parent = m_mapModder->get_parent();
		if (parent != nullptr)
			parent->call_deferred("remove_child", m_mapModder);
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

void Quadrant::releaseTerrainValuesMemory(bool visible)
{
	if (!m_quadTree->isValid() || internalDataLocked() || needUploadToServer() || needSaveToCache())
		return;

	//return;

	if (!visible && !m_viewer->Globals()->getParamCacheHeightsForInvisibleQuadrants())
		m_float16HeigthsBuffer.clear();	// needed to calc aabb of chunks during split/join (Quadrant::getAltitudeFromHeigthmap & Chunk::getGlobalCoordAABB): it is kept for performance reason to keep it for visible quadrants and avoid to reload from disk for new chunks
										// if m_viewer->Globals()->getParamCacheHeightsForInvisibleQuadrants() we keep also for invisible quadrant to avoid reload from disk when quadrant become visible (reason is we want it happen during load of invisible quadrants)
	m_float32HeigthsBuffer.clear();		// needed for collider but releaseTerrainValuesMemory will run after collider
	m_normalsBuffer.clear();			// we cache image
	m_splatmapBuffer.clear();			// we cache image
	m_colormapBuffer.clear();
	m_globalmapBuffer.clear();
	int64_t size = m_heightsForCollider.size();
	m_heightsForCollider.resize(0);
}

size_t Quadrant::calcMapMemoryOccupation(void)
{
	size_t occupation = m_float16HeigthsBuffer.reserved() + m_float32HeigthsBuffer.reserved() + m_normalsBuffer.reserved() + m_splatmapBuffer.reserved() + m_colormapBuffer.reserved() + m_globalmapBuffer.reserved() + (m_terrainEdit == nullptr ? 0 : m_terrainEdit->size) + m_heightsForCollider.size() * sizeof(real_t);
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

void Quadrant::process_mapModder(double _delta)
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
	//	return;

	if (internalDataLocked())
		return;

	if (m_quadTree->currentStatus() != QuadrantStatus::initialized)
		return;

	if (!m_quadTree->isValid())
		return;

	if (m_quadTree->getQuadrant()->empty())
		return;

	if (!m_mapModder->ready())
		return;

	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl != nullptr && editModeUIControl->blendingInProgress())
		return;

	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();

	if (terrainEdit->normalsNeedRegen)
	{
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForNormals(m_quadTree, &getFloat32HeightsBuffer(), 1024, 1, true);		// atm border = 0 will be the same
		}
		return;
	}

	if (terrainEdit->extraValues.splatmapNeedRegen)
	{
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForSplatmap(m_quadTree, &getFloat32HeightsBuffer(), &getNormalsBuffer(), terrainEdit->extraValues.slopeVerticalFactor, terrainEdit->extraValues.slopeFlatFactor, terrainEdit->extraValues.dirtOnRocksFactor, terrainEdit->extraValues.highElevationFactor, terrainEdit->extraValues.lowElevationFactor, terrainEdit->minHeight, terrainEdit->maxHeight, 1024, 0, true);
		}
		return;
	}

	//if (terrainEdit->extraValues.colormapNeedRegen)
	//{
	//	// TODO
	//}

	if (terrainEdit->extraValues.globalmapNeedRegen)
	{
		bool lockAcquired = lockInternalData();
		if (lockAcquired)
		{
			m_mapModder->initForGlobalMap(m_quadTree, true);
		}
		return;
	}
}

TheWorld_Utils::MeshCacheBuffer& Quadrant::getMeshCacheBuffer(void)
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
	TheWorld_Utils::FLOAT_32 f;
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

void Quadrant::saveQuadrantToDiskCache(void)
{
	setNeedUploadToServer(true);
	
	TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData cacheQuadrantData;
	cacheQuadrantData.meshId = m_cache.getMeshId();
	TheWorld_Utils::TerrainEdit* terrainEdit = getTerrainEdit();
	terrainEdit->needUploadToServer = true;
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
	QuadrantPos pos = getPos();
	TheWorld_Utils::MemoryBuffer buffer;
	m_cache.setBufferFromCacheQuadrantData(pos.getNumVerticesPerSize(), cacheQuadrantData, buffer);
	m_cache.writeBufferToDiskCache(buffer);
}

bool Quadrant::needSaveToCache(void)
{
	bool needToSave = false;

	GDN_TheWorld_Edit* editModeUIControl = m_viewer->EditModeUIControl();
	if (editModeUIControl != nullptr)
	{
		//editModeUIControl->eraseQuadToSave(pos);
		QuadrantPos pos = getPos();
		if (editModeUIControl->isQuadToSave(pos))
			needToSave = true;
	}

	return needToSave;
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
	godot::Vector3 normal;
	TheWorld_Utils::unpackNormal(packedNormal.x, packedNormal.y, packedNormal.z, normal.x, normal.y, normal.z);
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
	float gridStepinWU = m_quadrantPos.getGridStepInWU();
	int numVerticesPerSize = m_quadrantPos.getNumVerticesPerSize();
	int lvl = m_quadrantPos.getLevel();
	std::string buffGridVertices;

	// look for cache in file system
	TheWorld_Utils::MeshCacheBuffer cache = getMeshCacheBuffer();
	std::string meshId = cache.getMeshIdFromDisk();

	if (meshId.length() > 0)
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetQuadrantVertices(m_quadTree->Viewer()->isInEditor(), cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepinWU, lvl, setCamera, cameraDistanceFromTerrainForced, meshId);
		//clock.tock();
		//m_quadTree->Viewer()->Globals()->debugPrint(String("ELAPSED - QUADRANT ") + getId().getId().c_str() + " TAG=" + getId().getTag().c_str() + " - client MapManagerGetVertices (quadrant found in cache) " + std::to_string(clock.duration().count()).c_str() + " ms");
	}
	else
	{
		//clock.tick();
		m_quadTree->Viewer()->Globals()->Client()->MapManagerGetQuadrantVertices(m_quadTree->Viewer()->isInEditor(), cameraX, cameraY, cameraZ, cameraYaw, cameraPitch, cameraRoll,
																		 lowerXGridVertex, lowerZGridVertex,
																		 1, // TheWorld_MapManager::MapManager::anchorType::upperleftcorner
																		 numVerticesPerSize, gridStepinWU, lvl, setCamera, cameraDistanceFromTerrainForced, meshId);
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

void Quadrant::refreshGridVerticesFromServer(std::string& bufferFromServer, std::string meshIdFromServer, std::string& outMeshIdFromBuffer, bool updateCache)
{
	float minAltitude = 0, maxAltitude = 0;
	
	{
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.1 ") + __FUNCTION__, "m_cache.refreshMapsFromBuffer");

		TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
		m_cache.refreshMapsFromBuffer(bufferFromServer, outMeshIdFromBuffer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer, updateCache);
		if (terrainEditValuesBuffer.size() > 0)
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);
	}

	if (outMeshIdFromBuffer.size() > 0)
	{
		assert(outMeshIdFromBuffer == meshIdFromServer);
		if (outMeshIdFromBuffer != meshIdFromServer)
			throw(GDN_TheWorld_Exception(__FUNCTION__, std::string("MeshId from buffer not equal to meshId from server").c_str()));
	}

	setEmpty(false);

	TheWorld_Utils::MemoryBuffer terrainEditValuesBuffer;
	if (outMeshIdFromBuffer.size() == 0 || m_float16HeigthsBuffer.empty())
	{
		// Buffer from server was empty
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.2 ") + __FUNCTION__, "m_cache.readMapsFromMeshCache");

		bool empty = m_cache.refreshMapsFromDisk(m_quadrantPos.getNumVerticesPerSize(), m_quadrantPos.getGridStepInWU(), meshIdFromServer, terrainEditValuesBuffer, minAltitude, maxAltitude, m_float16HeigthsBuffer, m_float32HeigthsBuffer, m_normalsBuffer, m_splatmapBuffer, m_colormapBuffer, m_globalmapBuffer);
		if (terrainEditValuesBuffer.size() > 0)
			getTerrainEdit()->deserialize(terrainEditValuesBuffer);

		if (empty)
			setEmpty(true);
	}

	assert(!m_float16HeigthsBuffer.empty());

	if (getTerrainEdit()->needUploadToServer)
		setNeedUploadToServer(true);
	else
		setNeedUploadToServer(false);

	setHeightsUpdated(true);
	//setColorsUpdated(true);

	if (m_normalsBuffer.size() > 0)
		setNormalsUpdated(true);

	if (m_splatmapBuffer.size() > 0)
		setSplatmapUpdated(true);

	if (m_colormapBuffer.size() > 0)
		setColorsUpdated(true);

	if (m_globalmapBuffer.size() > 0)
		setGlobalmapUpdated(true);

	int areaSize = (m_viewer->Globals()->heightmapResolution() + 1) * (m_viewer->Globals()->heightmapResolution() + 1);

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
		TheWorld_Utils::GuardProfiler profiler(std::string("WorldDeploy 2.1.1.3 ") + __FUNCTION__, "SetColliderHeights");

		m_heightsForCollider.resize((int)numFloat32Heights);
		memcpy((char*)m_heightsForCollider.ptrw(), m_float32HeigthsBuffer.ptr(), m_float32HeigthsBuffer.size());
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
