//#include "pch.h"
#include "Chunk.h"
#include "QuadTree.h"
#include "MeshCache.h"

#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "Viewer_Utils.h"

#pragma warning (push, 0)
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/classes/base_material3d.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#pragma warning (pop)

using namespace godot;

Chunk::Chunk(enum class ChunkType type, int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat, godot::Ref<godot::Material>& seaMat)
{
	m_type = type;
	m_distanceFromCamera = 0;
	m_slotPosX = slotPosX;
	m_slotPosZ = slotPosZ;
	m_lod = lod;
	m_posInQuad = PosInQuad::NotSet;
	m_isCameraVerticalOnChunk = false;
	m_viewer = viewer;
	m_quadTree = quadTree;
	m_quad = nullptr;
	m_debugMode = m_viewer->getRequiredChunkDebugMode();
	m_debugContentVisible = m_viewer->getDebugContentVisibility();
	GDN_TheWorld_Globals* globals = m_viewer->Globals();
	m_numVerticesPerChuckSide = globals->numVerticesPerChuckSide();
	m_numChunksPerWorldGridSide = globals->numChunksPerHeightmapSide(m_lod);
	m_gridStepInGridInWGVs = globals->gridStepInVertices(m_lod);
	m_gridStepInWUs = globals->gridStepInHeightmapWUs(m_lod);
	m_chunkSizeInWGVs = m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_chunkSizeInWUs = m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_originXInGridInWGVs = m_slotPosX * m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_originZInGridInWGVs = m_slotPosZ * m_numVerticesPerChuckSide * m_gridStepInGridInWGVs;
	m_originXInWUsLocalToGrid = m_slotPosX * m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_originZInWUsLocalToGrid = m_slotPosZ * m_numVerticesPerChuckSide * m_gridStepInWUs;
	m_firstWorldVertCol = m_slotPosX * m_chunkSizeInWGVs;
	m_lastWorldVertCol = (m_slotPosX + 1) * m_chunkSizeInWGVs;
	m_firstWorldVertRow = m_slotPosZ * m_chunkSizeInWGVs;
	m_lastWorldVertRow = (m_slotPosZ + 1) * m_chunkSizeInWGVs;
	m_active = false;
	m_visible = m_quadTree->isVisible();
	m_pendingUpdate = false;
	m_justJoined = false;

	m_originXInWUsGlobal = m_quadTree->getQuadrant()->getPos().getLowerXGridVertex() + m_originXInWUsLocalToGrid;
	m_originZInWUsGlobal = m_quadTree->getQuadrant()->getPos().getLowerZGridVertex() + m_originZInWUsLocalToGrid;

	m_initialGlobalTransform = Transform3D(Basis(), Vector3(m_originXInWUsGlobal, 0, m_originZInWUsGlobal));

	//checkAndCalcAABB();

	m_globalTransformApplied = getGlobalTransform();

	initVisual(mat);

	m_seaMat = seaMat;
	m_seaMeshInstanceRID = RID();
	m_seaMeshRID = RID();
	checkAndInitSeaVisual(m_seaMat);

	//globals->debugPrint("Chunk::Chunk - ID: " + String(getPos().getPos().c_str()));
}

Chunk::~Chunk()
{
	deinit();
}

void Chunk::initVisual(godot::Ref<godot::Material>& mat)
{
	m_meshInstanceRID = RID();
	m_meshRID = RID();

	m_active = true;

	godot::RenderingServer* vs = godot::RenderingServer::get_singleton();
	m_meshInstanceRID = vs->instance_create();

	if (mat != nullptr)
		vs->instance_geometry_set_material_override(m_meshInstanceRID, mat->get_rid());

	enterWorld();

	vs->instance_set_visible(m_meshInstanceRID, isVisible());
	if (m_containsSea)
		vs->instance_set_visible(m_seaMeshInstanceRID, isVisible());

	m_globalTransformApplied = getGlobalTransform();

	godot::RenderingServer::get_singleton()->instance_set_transform(m_meshInstanceRID, m_globalTransformApplied);		// World coordinates
	if (m_containsSea)
		godot::RenderingServer::get_singleton()->instance_set_transform(m_seaMeshInstanceRID, m_globalTransformApplied);		// World coordinates

	printDebugInfo("initVisual");
}

void Chunk::checkAndInitSeaVisual(godot::Ref<godot::Material>& mat)
{
	m_containsSea = false;	// DEBUG
	return;
	if (m_quadTree->getQuadrant()->getTerrainEdit()->extraValues1.seaInfoSet != 0 && m_quadTree->getQuadrant()->getTerrainEdit()->extraValues1.containsSea != 0)
		m_containsSea = true;
	else
		m_containsSea = false;

	if (m_containsSea && m_seaMeshInstanceRID == RID())
		initSeaVisual(mat);
}

void Chunk::initSeaVisual(godot::Ref<godot::Material>& mat)
{
	if (m_containsSea && m_seaMeshInstanceRID == RID())
	{
		godot::RenderingServer* vs = godot::RenderingServer::get_singleton();
		m_seaMeshInstanceRID = vs->instance_create();

		if (mat != nullptr)
			vs->instance_geometry_set_material_override(m_seaMeshInstanceRID, mat->get_rid());

		if (m_worldEntered)
		{
			godot::Ref<godot::World3D> world = m_viewer->get_world_3d();
			if (world != nullptr && world.is_valid())
				vs->instance_set_scenario(m_seaMeshInstanceRID, world->get_scenario());
		}

		vs->instance_set_visible(m_seaMeshInstanceRID, isVisible());

		vs->instance_set_transform(m_seaMeshInstanceRID, m_globalTransformApplied);		// World coordinates

		printDebugInfo("initSeaVisual");
	}
}

void Chunk::checkAndCalcAABB()
{
	AABB aabb;
	if (m_aabb == aabb && m_quadTree->statusInitialized())
	{
		getGlobalCoordAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
		m_globalCoordAABB = m_aabb;
		m_aabb.position.x = 0;	// AABB is relative to the chunk
		m_aabb.position.z = 0;
	}
}

void Chunk::printDebugInfo(std::string method)
{
	return;
	TheWorld_Utils::QuadrantPos pos = m_quadTree->getQuadrant()->getPos();
	if (pos.getIndexPosX() == 103 && pos.getIndexPosZ() == 98)
	{
		if (m_lod == 0 && ((m_slotPosX == 96 && m_slotPosZ == 126) || (m_slotPosX == 96 && m_slotPosZ == 127)))
		{
			GDN_TheWorld_Globals* globals = m_viewer->Globals();

			if (method == "initVisual" || method == "onGlobalTransformChanged")
			{
				std::string s = method + " " + pos.getName() + " " + getIdStr() + " ";
				s += "origin=" + std::to_string(m_globalTransformApplied.origin.x) + "," + std::to_string(m_globalTransformApplied.origin.y) + "," + std::to_string(m_globalTransformApplied.origin.z);
				globals->debugPrint(s.c_str());
			}
			else if (method == "setMesh")
			{
				if (m_mesh == nullptr)
				{
					std::string s = method + " " + pos.getName() + " " + getIdStr() + " ";
					s += "Mesh null";
					globals->debugPrint(s.c_str());
				}
				else
				{
					std::string s = method + " " + pos.getName() + " " + getIdStr() + " ";
					godot::AABB meshAABB = m_mesh->get_aabb();
					s += "mesh AABB=" + std::to_string(meshAABB.position.x) + "," + std::to_string(meshAABB.position.y) + "," + std::to_string(meshAABB.position.z) + " " + std::to_string(meshAABB.size.x) + "," + std::to_string(meshAABB.size.y) + "," + std::to_string(meshAABB.size.z);
					globals->debugPrint(s.c_str());

					int64_t num = m_mesh->get_surface_count();
					godot::Array arrays = m_mesh->surface_get_arrays(0);
					PackedVector3Array positions = arrays[ArrayMesh::ARRAY_VERTEX];
					num = positions.size();
					if (num > 0)
					{
						float numElementsPerSize = sqrtf(float(num));
						int x = 0, z = 0;
						std::string s = "Z=" + std::to_string(z);
						for (int idx = 0; idx < num; idx++)
						{
							godot::Vector3 p = positions[idx];
							s += " " + std::to_string(x) + ":" + std::to_string(p.x + m_globalTransformApplied.origin.x) + "," + std::to_string(p.y + m_globalTransformApplied.origin.y) + "," + std::to_string(p.z + m_globalTransformApplied.origin.z);
							x++;
							if (x == numElementsPerSize)
							{
								globals->debugPrint(s.c_str());
								x = 0;
								z++;
								s = "Z=" + std::to_string(z);
							}
						}
					}
				}
			}

		}
	}
}

void Chunk::deinit(void)
{
	if (m_meshInstanceRID != godot::RID())
	{
		exitWorld(); 
		
		godot::Ref<godot::Material> nullMaterial;
		setMaterial(nullMaterial);
		godot::RenderingServer::get_singleton()->free_rid(m_meshInstanceRID);
		m_meshInstanceRID = godot::RID();
	}

	if (m_seaMeshInstanceRID != godot::RID())
	{
		godot::RenderingServer::get_singleton()->free_rid(m_seaMeshInstanceRID);
		m_seaMeshInstanceRID = godot::RID();
	}

	if (m_viewer->m_cameraChunk == this)
		m_viewer->m_cameraChunk = nullptr;
	if (m_viewer->m_mouseHitChunk == this)
		m_viewer->m_mouseHitChunk = nullptr;
}

void Chunk::enterWorld(void)
{
	if (!m_worldEntered)
	{
		assert(m_meshInstanceRID != godot::RID());
		godot::Ref<godot::World3D> world = m_viewer->get_world_3d();
		if (world != nullptr && world.is_valid())
		{
			godot::RenderingServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, world->get_scenario());
			if (m_containsSea)
			{
				assert(m_seaMeshInstanceRID != godot::RID());
				godot::RenderingServer::get_singleton()->instance_set_scenario(m_seaMeshInstanceRID, world->get_scenario());
			}
		}

		m_worldEntered = true;
	}
}

void Chunk::exitWorld(void)
{
	if (m_worldEntered)
	{
		assert(m_meshInstanceRID != godot::RID());
		godot::RenderingServer::get_singleton()->instance_set_scenario(m_meshInstanceRID, godot::RID());
		if (m_containsSea)
		{
			assert(m_seaMeshInstanceRID != godot::RID());
			godot::RenderingServer::get_singleton()->instance_set_scenario(m_seaMeshInstanceRID, godot::RID());
		}

		m_worldEntered = false;
	}
}

void Chunk::onGlobalTransformChanged(void)
{
	assert(m_meshInstanceRID != godot::RID());
	m_globalTransformApplied = getGlobalTransform();
	// Set the mesh pos in global coord
	godot::RenderingServer::get_singleton()->instance_set_transform(m_meshInstanceRID, m_globalTransformApplied);		// World coordinates
	if (m_containsSea)
	{
		assert(m_seaMeshInstanceRID != godot::RID());
		godot::RenderingServer::get_singleton()->instance_set_transform(m_seaMeshInstanceRID, m_globalTransformApplied);		// World coordinates
	}

	printDebugInfo("onGlobalTransformChanged");
}

void Chunk::setVisible(bool b)
{
	if (!isActive())
		b = false;

	if (!m_quadTree->isVisible())
		b = false;

	assert(m_meshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_visible(m_meshInstanceRID, b);
	if (m_containsSea)
	{
		assert(m_seaMeshInstanceRID != godot::RID());
		godot::RenderingServer::get_singleton()->instance_set_visible(m_seaMeshInstanceRID, b);
	}

	m_visible = b;
}

void Chunk::setDebugContentVisible(bool b)
{
	if (!isActive())
		b = false;

	if (!m_quadTree->isVisible())
		b = false;

	m_debugContentVisible = b;
}

godot::AABB Chunk::getSeaAABB(void)
{
	AABB aabb;
	godot::Vector3 startPosition(m_aabb.position.x, THEWORLD_SEA_LEVEL - 20, m_aabb.position.z);
	godot::Vector3 endPosition(m_aabb.position.x + m_chunkSizeInWUs, THEWORLD_SEA_LEVEL + 20, m_aabb.position.z + m_chunkSizeInWUs);
	godot::Vector3 size = endPosition - startPosition;
	aabb.set_position(startPosition);
	aabb.set_size(size);
	return aabb;
}

void Chunk::applyAABB(void)
{
	checkAndCalcAABB();
	
	assert(m_meshInstanceRID != godot::RID());
	if (m_meshRID != RID())
		godot::RenderingServer::get_singleton()->instance_set_custom_aabb(m_meshInstanceRID, m_aabb);

	if (m_containsSea)
	{
		assert(m_seaMeshInstanceRID != godot::RID());
		if (m_seaMeshRID != RID())
		{
			godot::AABB aabb = getSeaAABB();
			godot::RenderingServer::get_singleton()->instance_set_custom_aabb(m_seaMeshInstanceRID, aabb);
		}
	}
}

void Chunk::setMesh(godot::Ref<godot::Mesh> mesh)
{
	godot::RID meshRID = (mesh != nullptr ? mesh->get_rid() : godot::RID());

	if (meshRID == m_meshRID)
		return;

	assert(m_meshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_base(m_meshInstanceRID, meshRID);

	m_meshRID = meshRID;
	m_mesh = mesh;

	printDebugInfo("setMesh");
}

void Chunk::setSeaMesh(godot::Ref<godot::Mesh> mesh)
{
	godot::RID meshRID = (mesh != nullptr ? mesh->get_rid() : godot::RID());

	if (meshRID == m_seaMeshRID)
		return;

	assert(m_seaMeshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_base(m_seaMeshInstanceRID, meshRID);

	m_seaMeshRID = meshRID;
	m_seaMesh = mesh;

	printDebugInfo("setSeaMesh");
}

bool Chunk::isMeshNull(void)
{
	return (m_meshRID == godot::RID() ? true : false);
}

bool Chunk::isDebugMeshNull(void)
{
	return true;
}

bool Chunk::isQuadTreeVisible(void)
{
	return m_quadTree->isVisible() && m_quadTree->isValid();
}

// TODORIC mah Material stuff
//func set_material(material: Material) :
//	assert(_mesh_instance != RID())
//	VisualServer.instance_geometry_set_material_override(\
//		_mesh_instance, material.get_rid() if material != null else RID())

void Chunk::setMaterial(godot::Ref<godot::Material>& mat)
{
	assert(m_meshInstanceRID != godot::RID());
	//assert(mat != nullptr);

	godot::RID materialRid;
	if (mat != nullptr)
		materialRid = mat->get_rid();
	else
		materialRid = godot::RID();
	godot::RenderingServer::get_singleton()->instance_geometry_set_material_override(m_meshInstanceRID, materialRid);
}

void Chunk::update(bool isVisible)
{
	if (!isActive())
	{
		setVisible(false);
		setPendingUpdate(false);
		return;
	}
	
	int seams = 0;

	// Seams are against grater chunks (greater lod = less resolution)
	ChunkPos posGreaterLodChunkContainingThisOne(m_slotPosX / 2, m_slotPosZ / 2, m_lod + 1);
	int numChunksPerSideAtGreaterLod = 0;
	if (posGreaterLodChunkContainingThisOne.getLod() <= m_viewer->Globals()->lodMaxDepth())
		numChunksPerSideAtGreaterLod = m_viewer->Globals()->numChunksPerHeightmapSide(posGreaterLodChunkContainingThisOne.getLod());

	int numChunksPerSideAtCurrentLod = m_viewer->Globals()->numChunksPerHeightmapSide(m_lod);

	switch (m_posInQuad)
	{
	case PosInQuad::First:
	{
		//  
		//	o =
		//	= =
		//  

		// check left chunk
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;

		// if we are on left border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosX == 0)
		{
			TheWorld_Utils::QuadrantPos XMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
			QuadTree* XMinusQuadTree = m_viewer->getQuadTree(XMinusQuadrantPos);
			if (XMinusQuadTree != nullptr && XMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(numChunksPerSideAtGreaterLod - 1, posGreaterLodChunkContainingThisOne.getSlotPosZ(), posGreaterLodChunkContainingThisOne.getLod());
				chunk = XMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_LEFT;
			}
		}

		// check up chunk
		chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;

		// if we are on top border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosZ == 0)
		{
			TheWorld_Utils::QuadrantPos ZMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
			QuadTree* ZMinusQuadTree = m_viewer->getQuadTree(ZMinusQuadrantPos);
			if (ZMinusQuadTree != nullptr && ZMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterLodChunkContainingThisOne.getSlotPosX(), numChunksPerSideAtGreaterLod - 1, posGreaterLodChunkContainingThisOne.getLod());
				chunk = ZMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_BOTTOM;
			}
		}
	}
	break;
	case PosInQuad::Second:
	{
		//  
		//	= o
		//	= =
		//  

		// check right chunk
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;

		// if we are on right border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosX == numChunksPerSideAtCurrentLod - 1)
		{
			TheWorld_Utils::QuadrantPos XPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
			QuadTree* XPlusQuadTree = m_viewer->getQuadTree(XPlusQuadrantPos);
			if (XPlusQuadTree != nullptr && XPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(0, posGreaterLodChunkContainingThisOne.getSlotPosZ(), posGreaterLodChunkContainingThisOne.getLod());
				chunk = XPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_RIGHT;
			}
		}

		// check up chunk
		chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::ZMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_BOTTOM;

		// if we are on top border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosZ == 0)
		{
			TheWorld_Utils::QuadrantPos ZMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZMinus);
			QuadTree* ZMinusQuadTree = m_viewer->getQuadTree(ZMinusQuadrantPos);
			if (ZMinusQuadTree != nullptr && ZMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterLodChunkContainingThisOne.getSlotPosX(), numChunksPerSideAtGreaterLod - 1, posGreaterLodChunkContainingThisOne.getLod());
				chunk = ZMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_BOTTOM;
			}
		}
	}
	break;
	case PosInQuad::Third:
	{
		//  
		//	= =
		//	o =
		//  
		
		// check left chunk
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::XMinusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_LEFT;

		// if we are on left border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosX == 0)
		{
			TheWorld_Utils::QuadrantPos XMinusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XMinus);
			QuadTree* XMinusQuadTree = m_viewer->getQuadTree(XMinusQuadrantPos);
			if (XMinusQuadTree != nullptr && XMinusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(numChunksPerSideAtGreaterLod - 1, posGreaterLodChunkContainingThisOne.getSlotPosZ(), posGreaterLodChunkContainingThisOne.getLod());
				chunk = XMinusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_LEFT;
			}
		}

		// check down chunk
		chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;

		// if we are on bottom border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosZ == numChunksPerSideAtCurrentLod - 1)
		{
			TheWorld_Utils::QuadrantPos ZPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
			QuadTree* ZPlusQuadTree = m_viewer->getQuadTree(ZPlusQuadrantPos);
			if (ZPlusQuadTree != nullptr && ZPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterLodChunkContainingThisOne.getSlotPosX(), 0, posGreaterLodChunkContainingThisOne.getLod());
				chunk = ZPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_TOP;
			}
		}
	}
	break;
	case PosInQuad::Forth:
	{
		//  
		//	= =
		//	= o
		//  
		
		// check right chunk
		Chunk* chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::XPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_RIGHT;

		// if we are on right border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosX == numChunksPerSideAtCurrentLod - 1)
		{
			TheWorld_Utils::QuadrantPos XPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::XPlus);
			QuadTree* XPlusQuadTree = m_viewer->getQuadTree(XPlusQuadrantPos);
			if (XPlusQuadTree != nullptr && XPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(0, posGreaterLodChunkContainingThisOne.getSlotPosZ(), posGreaterLodChunkContainingThisOne.getLod());
				chunk = XPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_RIGHT;
			}
		}

		// check down chunk
		chunk = m_quadTree->getChunkAt(posGreaterLodChunkContainingThisOne, Chunk::DirectionSlot::ZPlusChunk);
		if (chunk != nullptr && chunk->isActive())
			seams |= SEAM_TOP;

		// if we are on bottom border of quadrant
		if (numChunksPerSideAtGreaterLod > 0 && m_slotPosZ == numChunksPerSideAtCurrentLod - 1)
		{
			TheWorld_Utils::QuadrantPos ZPlusQuadrantPos = m_quadTree->getQuadrant()->getPos().getQuadrantPos(TheWorld_Utils::QuadrantPos::DirectionSlot::ZPlus);
			QuadTree* ZPlusQuadTree = m_viewer->getQuadTree(ZPlusQuadrantPos);
			if (ZPlusQuadTree != nullptr && ZPlusQuadTree->isValid())
			{
				ChunkPos posGreaterChunkOnAdjacentQuadrant(posGreaterLodChunkContainingThisOne.getSlotPosX(), 0, posGreaterLodChunkContainingThisOne.getLod());
				chunk = ZPlusQuadTree->getChunkAt(posGreaterChunkOnAdjacentQuadrant);
				if (chunk != nullptr && chunk->isActive())
					seams |= SEAM_TOP;
			}
		}
	}
	break;
	}

	setMesh(m_viewer->getMeshCache()->getMesh(seams, m_lod));
	
	checkAndInitSeaVisual(m_seaMat);
	if (m_containsSea)
		setSeaMesh(m_viewer->getMeshCache()->getMesh(seams, m_lod));
	
	applyDebugMesh();
	applyAABB();

	setVisible(isVisible);

	setPendingUpdate(false);
}

void Chunk::refresh(bool isVisible)
{
	if (!isActive())
		return;
	
	// if needed the AABB can be re-computed
	//m_viewer->getPartialAABB(m_aabb, m_firstWorldVertCol, m_lastWorldVertCol, m_firstWorldVertRow, m_lastWorldVertRow, m_gridStepInGridInWGVs);
	//m_globalCoordAABB = m_aabb;
	//m_aabb.position.x = 0;	// AABB is relative to the chunk
	//m_aabb.position.z = 0;

	setMesh(nullptr);
	setSeaMesh(nullptr);
	m_quadTree->addChunkUpdate(this);
}

void Chunk::heightsChanged(void)
{
	m_aabb = AABB();
	Quad* quad = getQuad();
	if (quad != nullptr)
		quad->resetChachedChunkAABB();
}

void Chunk::setActive(bool b)
{
	m_active = b;
	if (!b)
	{
		m_justJoined = false;
		if (m_viewer->m_cameraChunk == this)
			m_viewer->m_cameraChunk = nullptr;
		m_isCameraVerticalOnChunk = false;
		m_lastCameraPosInMap = Vector3(0, 0, 0);
	}
}

void Chunk::setCameraPosInMap(Vector3 cameraPosInMap)
{
	if (!isActive())
	{
		m_lastCameraPosInMap = Vector3(0, 0, 0);
		if (m_viewer->m_cameraChunk == this)
			m_viewer->m_cameraChunk = nullptr;
		m_isCameraVerticalOnChunk = false;
	}
	else
	{
		m_lastCameraPosInMap = cameraPosInMap;

		if (cameraPosInMap.x >= m_originXInWUsGlobal && cameraPosInMap.x < m_originXInWUsGlobal + m_chunkSizeInWUs
			&& cameraPosInMap.z >= m_originZInWUsGlobal && cameraPosInMap.z < m_originZInWUsGlobal + m_chunkSizeInWUs)
			m_isCameraVerticalOnChunk = true;
		else
		{
			if (m_viewer->m_cameraChunk == this)
				m_viewer->m_cameraChunk = nullptr;
			m_isCameraVerticalOnChunk = false;
		}
	}

	if (m_isCameraVerticalOnChunk)
		m_viewer->setCameraChunk(this, m_quadTree);
}

void Chunk::getCameraPosInMap(Vector3& lastCameraPosInMap) const
{
	lastCameraPosInMap = m_lastCameraPosInMap;
}

Transform3D Chunk::getGlobalTransform(void)
{
	Transform3D igt = m_viewer->getInternalGlobalTransform();
	Transform3D gt = igt * m_initialGlobalTransform;

	return gt;
}

Transform3D Chunk::getGlobalTransformApplied(void)
{
	return m_globalTransformApplied;
}

Transform3D Chunk::getDebugGlobalTransform(void)
{
	return Transform3D();
}

Transform3D Chunk::getDebugGlobalTransformApplied(void)
{
	return Transform3D();
}

void Chunk::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	m_debugMode = mode;
}

void Chunk::applyDebugMesh()
{
}

void Chunk::getGlobalCoordAABB(godot::AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step)
{
	//TheWorld_Utils::GuardProfiler profiler(std::string("getGlobalCoordAABB 1 ") + __FUNCTION__, "ALL");

	int numWorldVerticesPerSize = m_quadTree->getQuadrant()->getPos().getNumVerticesPerSize();
	int idxFirstColFirstRowWorldVert = numWorldVerticesPerSize * firstWorldVertRow + firstWorldVertCol;
	int idxLastColFirstRowWorldVert = numWorldVerticesPerSize * firstWorldVertRow + lastWorldVertCol;
	int idxFirstColLastRowWorldVert = numWorldVerticesPerSize * lastWorldVertRow + firstWorldVertCol;
	int idxLastColLastRowWorldVert = numWorldVerticesPerSize * lastWorldVertRow + lastWorldVertCol;

	godot::AABB emptyAABB;
	Quad* root = m_quadTree->getTerrainQuadRoot();
	Quad* quad = getQuad();
	if (root == quad)
	{
		aabb = m_quadTree->getQuadrant()->getGlobalAABB();
		if (aabb != emptyAABB)
			return;
	}

	// altitudes
	float minHeight = 0, maxHeight = 0;
	bool firstTime = true;
	for (int idxRow = 0; idxRow < lastWorldVertRow - firstWorldVertRow + 1; idxRow += step)
	{
		for (int idxVert = idxFirstColFirstRowWorldVert + idxRow * numWorldVerticesPerSize; idxVert < idxLastColFirstRowWorldVert + idxRow * numWorldVerticesPerSize + 1; idxVert += step)
		{
			if (firstTime)
			{
				minHeight = maxHeight = m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert);
				firstTime = false;
			}
			else
			{
				minHeight = TheWorld_Viewer_Utils::Utils::min2(minHeight, m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert));
				maxHeight = TheWorld_Viewer_Utils::Utils::max2(maxHeight, m_quadTree->getQuadrant()->getAltitudeFromHeigthmap(idxVert));
			}
		}
	}

	godot::Vector3 startPosition(m_quadTree->getQuadrant()->getPosXFromHeigthmap(idxFirstColFirstRowWorldVert), minHeight, m_quadTree->getQuadrant()->getPosZFromHeigthmap(idxFirstColFirstRowWorldVert));
	//godot::Vector3 endPosition(m_worldVertices[idxLastColLastRowWorldVert].posX() - m_worldVertices[idxFirstColFirstRowWorldVert].posX(), maxHeigth, m_worldVertices[idxLastColLastRowWorldVert].posZ() - m_worldVertices[idxFirstColFirstRowWorldVert].posZ());
	godot::Vector3 endPosition(m_quadTree->getQuadrant()->getPosXFromHeigthmap(idxLastColLastRowWorldVert), maxHeight, m_quadTree->getQuadrant()->getPosZFromHeigthmap(idxLastColLastRowWorldVert));

	godot::Vector3 size = endPosition - startPosition;
	//size.x += 1;
	//size.z += 1;

	aabb.set_position(startPosition);
	aabb.set_size(size);
}

void Chunk::setDistanceFromCamera(float distanceFromCamera)
{
	m_distanceFromCamera = distanceFromCamera;
}

float Chunk::getDistanceFromCamera(void) const
{
	return m_distanceFromCamera;
}

void Chunk::checkMouseHit(void)
{
	godot::Vector3 mouseHit = m_viewer->getMouseHitInMap();
	if (checkHit(mouseHit))
	{
		//m_viewer->setMouseHitQuadTree(m_quadTree);
		m_viewer->setMouseHitChunk(this);
	}

	//godot::Point2 origOfChunk(getLowerXInWUsGlobal(), getLowerZInWUsGlobal());
	//godot::Point2 size(getChunkSizeInWUs(), getChunkSizeInWUs());
	////godot::Rect2 rect(origOfChunk, size);
	//if (mouseHit.x >= origOfChunk.x && mouseHit.x <= origOfChunk.x + size.x && mouseHit.z >= origOfChunk.y && mouseHit.z <= origOfChunk.y + size.y)
	////if (rect.has_point(godot::Point2(mouseHit.x, mouseHit.z)))
	//{
	//	m_viewer->setMouseHitQuadTree(m_quadTree);
	//	m_viewer->setMouseHitChunk(this);
	//}
}

bool Chunk::checkHit(godot::Vector3 hit)
{
	godot::Point2 origOfChunk(getLowerXInWUsGlobal(), getLowerZInWUsGlobal());
	godot::Point2 size(getChunkSizeInWUs(), getChunkSizeInWUs());
	//godot::Rect2 rect(origOfChunk, size);
	//if (rect.has_point(godot::Point2(mouseHit.x, mouseHit.z)))
	if (hit.x >= origOfChunk.x && hit.x <= origOfChunk.x + size.x && hit.z >= origOfChunk.y && hit.z <= origOfChunk.y + size.y)
		return true;
	else
		return false;
}

void Chunk::dump(void)
{
	if (!isActive())
		return;
	
	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	//Transform localT(Basis(), Vector3((float)m_originXInWUsLocalToGrid, 0, (float)m_originZInWUsLocalToGrid));
	//Transform worldT = m_parentTransform * localT;
	//Transform worldT = m_initialGlobalTransform;
	//Vector3 v1 = worldT * Vector3((float)m_originXInWUsLocalToGrid, 0, (float)m_originZInWUsLocalToGrid);
	//Vector3 v2 = m_parentTransform * Vector3((float)m_originXInWUsLocalToGrid, 0, (float)m_originZInWUsLocalToGrid);
	//Vector3 v3 = m_viewer->to_global(Vector3((float)m_originXInWUsLocalToGrid, 0, (float)m_originZInWUsLocalToGrid));

	//float globalOriginXInGridInWUs = (float)m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
	float globalOriginXInGridInWUs = (float)m_viewer->getInternalGlobalTransform().origin.x + m_originXInWUsLocalToGrid;
	//float globalOriginZInGridInWUs = (float)m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;
	float globalOriginZInGridInWUs = (float)m_viewer->getInternalGlobalTransform().origin.z + m_originZInWUsLocalToGrid;

	globals->debugPrint(String("Chunk ID: ") + getPos().getIdStr().c_str()
		+ " - chunk size (WUs) " + to_string(m_chunkSizeInWUs).c_str()
		+ " - Pos in GRID (local):"
		+ " X = " + to_string(m_originXInWUsLocalToGrid).c_str()
		+ ", Z = " + to_string(m_originZInWUsLocalToGrid).c_str()
		+ ", X1 = " + to_string(m_originXInWUsLocalToGrid + m_chunkSizeInWUs).c_str()
		+ ", Z1 = " + to_string(m_originZInWUsLocalToGrid + m_chunkSizeInWUs).c_str()
		+ " - Pos in GRID (global):"
		+ " X = " + to_string(globalOriginXInGridInWUs).c_str()
		+ ", Z = " + to_string(globalOriginZInGridInWUs).c_str()
		+ ", X1 = " + to_string(globalOriginXInGridInWUs + m_chunkSizeInWUs).c_str()
		+ ", Z1 = " + to_string(globalOriginZInGridInWUs + m_chunkSizeInWUs).c_str()
		+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
	globals->debugPrint(String("t: ") + getGlobalTransformApplied() + String(" - t (debug): ") + getDebugGlobalTransformApplied());
}

TerrainChunk::TerrainChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat, godot::Ref<godot::Material>& seaMat)
	: Chunk(Chunk::ChunkType::Terrain, slotPosX, slotPosZ, lod, viewer, quadTree, mat, seaMat)
{
}

TerrainChunk::~TerrainChunk()
{
}

DebugTerrainChunk::DebugTerrainChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat, godot::Ref<godot::Material>& seaMat)
	: TerrainChunk(slotPosX, slotPosZ, lod, viewer, quadTree, mat, seaMat)
{
	m_debugMeshInstanceRID = RID();
	m_debugMeshRID = RID();
	m_debugMeshInstance = nullptr;
	m_debugWirecubeMeshNeedRegen = false;
	m_debugWiresquareMeshNeedRegen = false;

	RenderingServer* vs = RenderingServer::get_singleton();
	m_debugMeshInstanceRID = vs->instance_create();

	enterWorld();

	vs->instance_set_visible(m_debugMeshInstanceRID, isDebugContentVisible());

	//checkAndCalcDebugMeshAABB;

	//applyDebugMesh();
}

void DebugTerrainChunk::checkAndCalcDebugMeshAABB()
{
	AABB aabb;
	if (m_debugMeshAABB == aabb && m_quadTree->statusInitialized())
	{
		checkAndCalcAABB();
		m_debugMeshAABB = m_aabb;
	}
}

DebugTerrainChunk::~DebugTerrainChunk()
{
	if (m_debugMeshInstanceRID != godot::RID())
	{
		exitWorld();
		
		godot::RenderingServer::get_singleton()->free_rid(m_debugMeshInstanceRID);
		m_debugMeshInstanceRID = godot::RID();
	}
}

void DebugTerrainChunk::enterWorld(void)
{
	TerrainChunk::enterWorld();

	if (!m_debugWorldEntered)
	{
		assert(m_debugMeshInstanceRID != godot::RID());
		godot::Ref<godot::World3D> world = m_viewer->get_world_3d();
		if (world != nullptr && world.is_valid())
			godot::RenderingServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, world->get_scenario());

		m_debugWorldEntered = true;
	}
}

void DebugTerrainChunk::exitWorld(void)
{
	TerrainChunk::exitWorld();
	
	if (m_debugWorldEntered)
	{
		assert(m_debugMeshInstanceRID != godot::RID());
		godot::RenderingServer::get_singleton()->instance_set_scenario(m_debugMeshInstanceRID, godot::RID());

		m_debugWorldEntered = false;
	}
}

void DebugTerrainChunk::refresh(bool isVisible)
{
	TerrainChunk::refresh(isVisible);
	setDebugMesh(nullptr);
	checkAndCalcAABB();
	m_debugMeshAABB = m_aabb;
}

void DebugTerrainChunk::heightsChanged(void)
{
	TerrainChunk::heightsChanged();
	m_debugMeshAABB = AABB();
	m_debugWirecubeMeshNeedRegen = true;
	m_debugWiresquareMeshNeedRegen = true;
}

void DebugTerrainChunk::update(bool isVisible)
{
	TerrainChunk::update(isVisible);
}

godot::Transform3D DebugTerrainChunk::getDebugGlobalTransform(void)
{
	checkAndCalcAABB();
	Transform3D worldTransform = getGlobalTransformApplied() * godot::Transform3D(Basis().scaled(m_aabb.size));

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
		worldTransform.origin.y = 0;

	return worldTransform;
}

void DebugTerrainChunk::onGlobalTransformChanged()
{
	TerrainChunk::onGlobalTransformChanged();

	godot::Transform3D worldTransform = getDebugGlobalTransform();

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	
	m_debugGlobaTransformApplied = worldTransform;

	//assert(m_debugGlobaTransformApplied.origin == m_meshGlobaTransformApplied.origin);	// DEBUGRIC
}

Transform3D DebugTerrainChunk::getDebugGlobalTransformApplied(void)
{
	return m_debugGlobaTransformApplied;
}

void DebugTerrainChunk::setActive(bool b)
{
	TerrainChunk::setActive(b);
}

void DebugTerrainChunk::setVisible(bool b)
{
	TerrainChunk::setVisible(b);
	
	//TerrainChunk::setVisible(false);	// SUPERDEBUGRIC
	//if (m_isCameraVerticalOnChunk)	// SUPERDEBUGRIC
	//	TerrainChunk::setVisible(true);	// SUPERDEBUGRIC
	//else							// SUPERDEBUGRIC
	//	TerrainChunk::setVisible(false);	// SUPERDEBUGRIC

	if (!isVisible())
		b = false;

	if (!isDebugContentVisible())
		b = false;

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, b);
}

void DebugTerrainChunk::setDebugContentVisible(bool b)
{
	TerrainChunk::setDebugContentVisible(b);

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_visible(m_debugMeshInstanceRID, isDebugContentVisible());
}

void DebugTerrainChunk::applyAABB(void)
{
	TerrainChunk::applyAABB();

	checkAndCalcDebugMeshAABB();

	assert(m_debugMeshInstanceRID != godot::RID());
	if (isDebugContentVisible())
		if (m_debugMeshRID != godot::RID())
			godot::RenderingServer::get_singleton()->instance_set_custom_aabb(m_debugMeshInstanceRID, m_debugMeshAABB);
}

void DebugTerrainChunk::setDebugMesh(godot::Ref<godot::Mesh> mesh)
{
	if (m_isCameraVerticalOnChunk)
		godot::RID();
	
	godot::RID meshRID = (mesh != nullptr ? mesh->get_rid() : godot::RID());

	if (m_debugMeshRID == meshRID)
		return;

	//if (mesh == nullptr)
	//	RID();

	assert(m_debugMeshInstanceRID != godot::RID());
	godot::RenderingServer::get_singleton()->instance_set_base(m_debugMeshInstanceRID, meshRID);

	m_debugMeshRID = meshRID;
	m_debugMesh = mesh;
}

bool DebugTerrainChunk::isDebugMeshNull(void)
{
	return (m_debugMeshRID == godot::RID() ? true : false);
}

void DebugTerrainChunk::setCameraPosInMap(godot::Vector3 cameraPosInMap)
{
	bool prevCameraVerticalOnChunk = isCameraVerticalOnChunk();

	TerrainChunk::setCameraPosInMap(cameraPosInMap);

	//return;
	
	if (isCameraVerticalOnChunk() && !prevCameraVerticalOnChunk)
	{
		//float globalOriginXInGridInWUs = (float)m_viewer->get_global_transform().origin.x + m_originXInWUsLocalToGrid;
		float globalOriginXInGridInWUs = (float)m_viewer->getInternalGlobalTransform().origin.x + m_originXInWUsLocalToGrid;
		//float globalOriginZInGridInWUs = (float)m_viewer->get_global_transform().origin.z + m_originZInWUsLocalToGrid;
		float globalOriginZInGridInWUs = (float)m_viewer->getInternalGlobalTransform().origin.z + m_originZInWUsLocalToGrid;

		//m_viewer->Globals()->debugPrint("Camera vertical on chunk ("
		//	+ String(to_string(getPos().getSlotPosX()).c_str()) + ":" + to_string(getPos().getSlotPosZ()).c_str()
		//	+ ") of " + m_quadTree->getQuadrant()->getPos().getName().c_str() + " - Chunk pos (Global) = "
		//	+ to_string(globalOriginXInGridInWUs).c_str() + ":" + to_string(globalOriginZInGridInWUs).c_str()
		//	+ " - MinH = " + to_string(m_aabb.position.y).c_str()
		//	+ " - MaxH = " + to_string((m_aabb.position + m_aabb.size).y).c_str()
		//	+ " - Chunk Size in WUs = " + to_string(getChunkSizeInWUs()).c_str()
		//);
		
		applyDebugMesh();
		applyAABB();
	}
}

void DebugTerrainChunk::setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
{
	TerrainChunk::setDebugMode(mode);
}

void DebugTerrainChunk::applyDebugMesh()
{
	TerrainChunk::applyDebugMesh();
	
	checkAndCalcAABB();

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
	{
		m_debugMeshAABB = m_aabb;
		setDebugMesh(nullptr);
		return;
	}

	Variant mesh;

	if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
	{
		m_debugMeshAABB = m_aabb;

		if (isCameraVerticalOnChunk())
		{
			// set special Wirecube
			godot::Ref<godot::Mesh> _mesh = createWireCubeMesh(GDN_TheWorld_Globals::g_color_blue);
			m_viewer->getMainProcessingMutex().lock();
			godot::StandardMaterial3D* mat = memnew(godot::StandardMaterial3D);
			m_viewer->getMainProcessingMutex().unlock();
			mat->set_shading_mode(godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
			mat->set_flag(godot::BaseMaterial3D::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(godot::GDN_TheWorld_Globals::g_color_blue);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRECUBE_MESH + to_string(m_lod);
			
			if (m_debugWirecubeMeshNeedRegen)
			{
				if (m_viewer->has_meta(metaNameMesh.c_str()))
					m_viewer->remove_meta(metaNameMesh.c_str());
				m_debugWirecubeMeshNeedRegen = false;
			}

			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				godot::Color wiredMeshColor;
				if (m_lod == 0)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_yellow_apricot;
				else if (m_lod == 1)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_aquamarine_green;
				else if (m_lod == 2)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_green;
				else if (m_lod == 3)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_cyan;
				else if (m_lod == 4)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_pink;
				else
					wiredMeshColor = GDN_TheWorld_Globals::g_color_red;

				godot::Ref<godot::ArrayMesh> _mesh = createWireCubeMesh(wiredMeshColor);
				m_viewer->getMainProcessingMutex().lock();
				godot::StandardMaterial3D* mat = memnew(godot::StandardMaterial3D);
				m_viewer->getMainProcessingMutex().unlock();
				mat->set_shading_mode(godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
				mat->set_flag(godot::BaseMaterial3D::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
				mat->set_albedo(wiredMeshColor);
				_mesh->surface_set_material(0, mat);
				mesh = _mesh;
				m_viewer->set_meta(metaNameMesh.c_str(), mesh);
			}
			else
				mesh = m_viewer->get_meta(metaNameMesh.c_str());
		}
	}
	else if (m_debugMode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
	{
		//worldTransform.origin.y = 0;	inutile
		m_debugMeshAABB = m_aabb;
		m_debugMeshAABB.position.y = 0;
		m_debugMeshAABB.size.y = 0;

		if (isCameraVerticalOnChunk())
		{
			// set special Wirecube
			godot::Color wiredMeshColor = GDN_TheWorld_Globals::g_color_blue;
			godot::Ref<godot::Mesh> _mesh = createWireSquareMesh(wiredMeshColor);
			m_viewer->getMainProcessingMutex().lock();
			godot::StandardMaterial3D* mat = memnew(godot::StandardMaterial3D);
			m_viewer->getMainProcessingMutex().unlock();
			mat->set_shading_mode(godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
			mat->set_flag(godot::BaseMaterial3D::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
			mat->set_albedo(wiredMeshColor);
			_mesh->surface_set_material(0, mat);
			mesh = _mesh;
		}
		else
		{
			// reset normal Wirecube
			string metaNameMesh = DEBUG_WIRESQUARE_MESH + to_string(m_lod);

			if (m_debugWiresquareMeshNeedRegen)
			{
				if (m_viewer->has_meta(metaNameMesh.c_str()))
					m_viewer->remove_meta(metaNameMesh.c_str());
				m_debugWiresquareMeshNeedRegen = false;
			}

			if (!m_viewer->has_meta(metaNameMesh.c_str()))
			{
				godot::Color wiredMeshColor;
				if (m_lod == 0)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_yellow_apricot;
				else if (m_lod == 1)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_aquamarine_green;
				else if (m_lod == 2)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_green;
				else if (m_lod == 3)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_cyan;
				else if (m_lod == 4)
					wiredMeshColor = GDN_TheWorld_Globals::g_color_pink;
				else
					wiredMeshColor = GDN_TheWorld_Globals::g_color_red;

				godot::Ref<godot::ArrayMesh> _mesh = createWireSquareMesh(wiredMeshColor);
				m_viewer->getMainProcessingMutex().lock();
				godot::StandardMaterial3D* mat = memnew(godot::StandardMaterial3D);
				m_viewer->getMainProcessingMutex().unlock();
				mat->set_shading_mode(godot::BaseMaterial3D::ShadingMode::SHADING_MODE_UNSHADED);
				mat->set_flag(godot::BaseMaterial3D::Flags::FLAG_ALBEDO_FROM_VERTEX_COLOR, true);
				mat->set_albedo(wiredMeshColor);
				_mesh->surface_set_material(0, mat);
				mesh = _mesh;
				m_viewer->set_meta(metaNameMesh.c_str(), mesh);
			}
			else
				mesh = m_viewer->get_meta(metaNameMesh.c_str());
		}
	}

	setDebugMesh(nullptr);
	setDebugMesh(mesh);

	godot::Transform3D worldTransform = getDebugGlobalTransform();
	godot::RenderingServer::get_singleton()->instance_set_transform(m_debugMeshInstanceRID, worldTransform);
	m_debugGlobaTransformApplied = worldTransform;

	/*GDN_TheWorld_Globals* globals = m_viewer->Globals();
	globals->debugPrint(String("Debug Mesh - ID: ") + getPos().getPos().c_str()
		+ String(" POS ==> ") + String("X = ") + String(to_string(worldTransform.origin.x).c_str())
		+ String(", ") + String("Y = ") + String(to_string(worldTransform.origin.y).c_str())
		+ String(", ") + String("Z = ") + String(to_string(worldTransform.origin.z).c_str())
		+ (isCameraVerticalOnChunk() ? "CAMERA" : ""));*/
}

godot::Ref<godot::ArrayMesh> DebugTerrainChunk::createWireCubeMesh(Color c)
{
	godot::PackedVector3Array positions;
	godot::PackedColorArray colors;
	godot::PackedInt32Array indices;

	positions.append(godot::Vector3(0, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 1, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 1, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 1, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 1, 1));		colors.append(c);

	// lower face quad
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// upper face quad
	indices.append(4); indices.append(5);
	indices.append(5); indices.append(6);
	indices.append(6); indices.append(7);
	indices.append(7); indices.append(4);
	// vertical lines
	indices.append(0); indices.append(4);
	indices.append(1); indices.append(5);
	indices.append(2); indices.append(6);
	indices.append(3); indices.append(7);
	// lower face diagonals
	indices.append(0); indices.append(2);
	indices.append(1); indices.append(3);

	godot::Array arrays;
	arrays.resize(godot::ArrayMesh::ARRAY_MAX);
	arrays[godot::ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[godot::ArrayMesh::ARRAY_COLOR] = colors;
	arrays[godot::ArrayMesh::ARRAY_INDEX] = indices;

	m_viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::ArrayMesh> mesh = memnew(godot::ArrayMesh);
	m_viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_LINES, arrays);
	
	return mesh;
}

godot::Ref<godot::ArrayMesh> DebugTerrainChunk::createWireSquareMesh(Color c)
{
	godot::PackedVector3Array positions;
	godot::PackedColorArray colors;
	godot::PackedInt32Array indices;

	positions.append(godot::Vector3(0, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 0));		colors.append(c);
	positions.append(godot::Vector3(1, 0, 1));		colors.append(c);
	positions.append(godot::Vector3(0, 0, 1));		colors.append(c);

	// square
	indices.append(0); indices.append(1);
	indices.append(1); indices.append(2);
	indices.append(2); indices.append(3);
	indices.append(3); indices.append(0);
	// diagonals
	indices.append(0); indices.append(2);
	indices.append(1); indices.append(3);

	godot::Array arrays;
	arrays.resize(godot::ArrayMesh::ARRAY_MAX);
	arrays[godot::ArrayMesh::ARRAY_VERTEX] = positions;
	arrays[godot::ArrayMesh::ARRAY_COLOR] = colors;
	arrays[godot::ArrayMesh::ARRAY_INDEX] = indices;

	m_viewer->getMainProcessingMutex().lock();
	godot::Ref<godot::ArrayMesh> mesh = memnew(godot::ArrayMesh);
	m_viewer->getMainProcessingMutex().unlock();
	mesh->add_surface_from_arrays(godot::Mesh::PRIMITIVE_LINES, arrays);

	return mesh;
}

void DebugTerrainChunk::dump(void)
{
	TerrainChunk::dump();

	if (!isActive())
		return;

	GDN_TheWorld_Globals* globals = m_viewer->Globals();

	globals->debugPrint(String("Chunk ID: ") + getPos().getIdStr().c_str()
		+ " DEBUG MESH - MinH = " + to_string(m_debugMeshAABB.position.y).c_str()
		+ " - MaxH = " + to_string((m_debugMeshAABB.position + m_debugMeshAABB.size).y).c_str()
		+ (m_isCameraVerticalOnChunk ? " - CAMERA" : ""));
}

SeaChunk::SeaChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat, godot::Ref<godot::Material>& seaMat)
	: Chunk(Chunk::ChunkType::Sea, slotPosX, slotPosZ, lod, viewer, quadTree, mat, seaMat)
{
}

SeaChunk::~SeaChunk()
{
}

//void GDN_Chunk_MeshInstance::_register_methods()
//{
//	register_method("_ready", &GDN_Chunk_MeshInstance::_ready);
//	register_method("_process", &GDN_Chunk_MeshInstance::_process);
//	register_method("_input", &GDN_Chunk_MeshInstance::_input);
//
//	register_method("get_lod", &GDN_Chunk_MeshInstance::getLod);
//	register_method("get_slot_pos_x", &GDN_Chunk_MeshInstance::getSlotPosX);
//	register_method("get_slot_pos_z", &GDN_Chunk_MeshInstance::getSlotPosZ);
//	register_method("get_id", &GDN_Chunk_MeshInstance::getIdStr);
//}
//
//GDN_Chunk_MeshInstance::GDN_Chunk_MeshInstance()
//{
//	m_initialized = false;
//	m_chunk = nullptr;
//}
//
//GDN_Chunk_MeshInstance::~GDN_Chunk_MeshInstance()
//{
//	deinit();
//}
//
//void GDN_Chunk_MeshInstance::init(Chunk* chunk)
//{
//	m_chunk = chunk;
//}
//
//void GDN_Chunk_MeshInstance::deinit(void)
//{
//	if (m_initialized)
//	{
//		m_initialized = false;
//	}
//}
//
//void GDN_Chunk_MeshInstance::_init(void)
//{
//	//Godot::print("GDN_Template::Init");
//}
//
//void GDN_Chunk_MeshInstance::_ready(void)
//{
//	//Godot::print("GDN_Template::_ready");
//	//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
//}
//
//void GDN_Chunk_MeshInstance::_input(const Ref<InputEvent> event)
//{
//}
//
//void GDN_Chunk_MeshInstance::_process(float _delta)
//{
//	// To activate _process method add this Node to a Godot Scene
//	//Godot::print("GDN_Template::_process");
//}
//
//int GDN_Chunk_MeshInstance::getLod(void)
//{
//	return m_chunk->getLod();
//}
//
//int GDN_Chunk_MeshInstance::getSlotPosX(void)
//{
//	return m_chunk->getSlotPosX();
//}
//
//int GDN_Chunk_MeshInstance::getSlotPosZ(void)
//{
//	return m_chunk->getSlotPosZ();
//}
//
//String GDN_Chunk_MeshInstance::getIdStr(void)
//{
//	return m_chunk->getIdStr().c_str();
//}
