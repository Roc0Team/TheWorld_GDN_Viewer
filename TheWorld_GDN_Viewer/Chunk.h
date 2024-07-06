#pragma once

#include <map>
#include <string>

#include "GDN_TheWorld_Globals.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/array_mesh.hpp>
#pragma warning(pop)

#define GD_CHUNK_MESHINSTANCE_GROUP			"ChunkMeshInstanceGroup"
#define GD_DEBUGCHUNK_MESHINSTANCE_GROUP	"DebugChunkMeshInstanceGroup"

namespace godot
{
#define DEBUG_WIRECUBE_MESH			"debug_wirecube_mesh_lod_"
#define DEBUG_WIRESQUARE_MESH		"debug_wiresquare_mesh_lod_"

	class GDN_TheWorld_Viewer;
	class Chunk;
	class Quad;
	class QuadTree;

	//class GDN_Chunk_MeshInstance : public MeshInstance
	//{
	//	GODOT_CLASS(GDN_Chunk_MeshInstance, MeshInstance)

	//public:
	//	GDN_Chunk_MeshInstance();
	//	~GDN_Chunk_MeshInstance();
	//	void init(Chunk* chunk);
	//	void deinit(void);

	//	static void _register_methods();

	//	//
	//	// Godot Standard Functions
	//	//
	//	void _init(void); // our initializer called by Godot
	//	void _ready(void);
	//	void _process(float _delta);
	//	void _input(const godot::Ref<godot::InputEvent> event);

	//	int getLod(void);
	//	int getSlotPosX(void);
	//	int getSlotPosZ(void);
	//	String getIdStr(void);

	//private:
	//	bool m_initialized;
	//	Chunk* m_chunk;
	//};

	enum class PosInQuad
	{
		NotSet,

		//  
		//	o =
		//	= =
		//  
		First,

		//  
		//	= o
		//	= =
		//  
		Second,

		//  
		//	= =
		//	o =
		//  
		Third,

		//  
		//	= =
		//	= o
		//  
		Forth,
		Root
	};

	class Chunk
	{
	public:
		enum class ChunkType
		{
			NotSet = -1
			, Terrain = 0
			, Sea = 1
		};

		enum class LookForChunk
		{
			All = 0,
			SameLod = 1,
			LowerLod = 2,
			HigherLod = 3
		};

		enum class DirectionSlot
		{
			Center = -1,

			//
			// - o
			//
			XMinusChunk = 0,

			//
			//   o -
			//
			XPlusChunk = 1,

			//   -
			//   o
			//
			ZMinusChunk = 2,

			//
			//   o
			//   -
			ZPlusChunk = 3,

			//  
			//	- o =
			//	  = =
			//  
			XMinusQuadSecondChunk = 4,

			//  
			//	  o =
			//	- = =
			//  
			XMinusQuadForthChunk = 5,

			//  
			//	  o = -
			//	  = =
			//  
			XPlusQuadFirstChunk = 6,

			//  
			//	  o = 
			//	  = = -
			//  
			XPlusQuadThirdChunk = 7,

			//    -
			//	  o =
			//	  = =
			//    
			ZMinusQuadThirdChunk = 8,

			//      -
			//	  o =
			//	  = =
			//    
			ZMinusQuadForthChunk = 9,

			//    
			//	  o =
			//	  = =
			//    -
			ZPlusQuadFirstChunk = 10,

			//      
			//	  o =
			//	  = =
			//      -
			ZPlusQuadSecondChunk = 11
		};

		class ChunkAction
		{
		public:
			ChunkAction() {}
			virtual ~ChunkAction() {}
			virtual void preExec(QuadTree* quadTree) {};
			virtual void exec(Chunk* chunk) {};
			virtual void postExec(QuadTree* quadTree) {};
		private:
		};

		class SwitchDebugModeAction : public ChunkAction
		{
		public:
			SwitchDebugModeAction(enum class GDN_TheWorld_Globals::ChunkDebugMode mode)
			{
				m_mode = mode; 
			}
			virtual ~SwitchDebugModeAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				if (m_mode != GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet && chunk->isQuadTreeVisible())
				{
					chunk->setDebugMode(m_mode);
					chunk->applyDebugMesh();
					if (m_mode != GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
						chunk->applyAABB();
				}
			}
		private:
			enum class GDN_TheWorld_Globals::ChunkDebugMode m_mode;
		};

		class SetMaterialChunkAction : public ChunkAction
		{
		public:
			SetMaterialChunkAction(Ref<Material> mat)
			{
				m_material = mat;
			}
			virtual ~SetMaterialChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->setMaterial(m_material);
			}
		private:
			Ref<Material> m_material;
		};

		class EnterWorldChunkAction : public ChunkAction
		{
		public:
			EnterWorldChunkAction() {}
			virtual ~EnterWorldChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->enterWorld();
			}
		private:
		};

		class ExitWorldChunkAction : public ChunkAction
		{
		public:
			ExitWorldChunkAction() {}
			virtual ~ExitWorldChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->exitWorld();
			}
		private:
		};

		class VisibilityChangedChunkAction : public ChunkAction
		{
		public:
			VisibilityChangedChunkAction(bool isVisibleInTree, GDN_TheWorld_Globals* globals)
			{
				m_isVisibleInTree = isVisibleInTree; 
				m_globals = globals;
			}
			virtual ~VisibilityChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);

				bool isVisibleInTree = m_isVisibleInTree;
				if (chunk->chunkType() == ChunkType::Sea && isVisibleInTree)
				{
					isVisibleInTree = m_globals->getParamShowSea();
				}
				if (chunk->chunkType() == ChunkType::Terrain && isVisibleInTree)
				{
					isVisibleInTree = m_globals->getParamShowTerrain();
				}
				chunk->setVisible(isVisibleInTree);
			}
		private:
			bool m_isVisibleInTree = false;
			GDN_TheWorld_Globals* m_globals = nullptr;
		};

		class DebugVisibilityChangedChunkAction : public ChunkAction
		{
		public:
			DebugVisibilityChangedChunkAction(bool isVisible)
			{
				m_isVisible = isVisible; 
			}
			virtual ~DebugVisibilityChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->setDebugContentVisible(m_isVisible);
			}
		private:
			bool m_isVisible;
		};

		class TransformChangedChunkAction : public ChunkAction
		{
		public:
			TransformChangedChunkAction(void) {}
			virtual ~TransformChangedChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->onGlobalTransformChanged();
			}
		private:
		};

		class DumpChunkAction : public ChunkAction
		{
		public:
			DumpChunkAction(void) {}
			virtual ~DumpChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->dump();
			}
		private:
		};

		class RefreshChunkAction : public ChunkAction
		{
		public:
			RefreshChunkAction(bool isVisible) 
			{
				m_isVisible = isVisible; 
			}
			virtual ~RefreshChunkAction() {}
			virtual void exec(Chunk* chunk)
			{
				ChunkAction::exec(chunk);
				chunk->refresh(m_isVisible);
			}
		private:
			bool m_isVisible;
		};

		class SeaInfoChangedChunkAction : public ChunkAction
		{
		public:
			SeaInfoChangedChunkAction() {}
			virtual ~SeaInfoChangedChunkAction() {}
			virtual void preExec(QuadTree* quadTree);
			virtual void postExec(QuadTree* quadTree);
			virtual void exec(Chunk* chunk);

		private:
		};

		class TerrainHeightsChangedChunkAction : public ChunkAction
		{
		public:
			TerrainHeightsChangedChunkAction() {}
			virtual ~TerrainHeightsChangedChunkAction() {}
			virtual void preExec(QuadTree* quadTree);
			virtual void postExec(QuadTree* quadTree);
			virtual void exec(Chunk* chunk);

		private:
		};

		class ChunkPos
		{
		public:
			ChunkPos(int slotPosX, int slotPosZ, int lod)
			{
				m_slotPosX = slotPosX;
				m_slotPosZ = slotPosZ;
				m_lod = lod;
			}
			~ChunkPos() {}
			// needed to use an istance of ChunkPos as a key in a map (to keep the map sorted by y and by x for equal y)
			// first row, second row, ... etc
			bool operator<(const ChunkPos& pos) const
			{
				if (m_lod == pos.m_lod)
				{
					if (m_slotPosZ < pos.m_slotPosZ)
						return true;
					if (m_slotPosZ > pos.m_slotPosZ)
						return false;
					else
						return m_slotPosX < pos.m_slotPosX;
				}
				else
					if (m_lod < pos.m_lod)
						return true;
					else
						return false;
			}
			bool operator==(const ChunkPos& pos) const
			{
				if (m_slotPosX == pos.m_slotPosX && m_slotPosZ == pos.m_slotPosZ && m_lod == pos.m_lod)
					return true;
				else
					return false;
			}
			ChunkPos operator=(const ChunkPos& pos)
			{
				m_slotPosX = pos.m_slotPosX;
				m_slotPosZ = pos.m_slotPosZ;
				m_lod = pos.m_lod;
				return *this;
			}
			ChunkPos operator+(const ChunkPos& pos)
			{
				m_slotPosX += pos.m_slotPosX;
				m_slotPosZ += pos.m_slotPosZ;
				//m_lod = pos.m_lod;
				return *this;
			}
			int getLod(void) const
			{
				return m_lod; 
			}
			int getSlotPosX(void) const
			{
				return m_slotPosX; 
			}
			int getSlotPosZ(void) const
			{
				return m_slotPosZ; 
			}
			std::string getIdStr(void) const
			{
				return "LOD:" + std::to_string(m_lod) + "-X:" + std::to_string(m_slotPosX) + "-Z:" + std::to_string(m_slotPosZ);
			}
		
		private:
			int m_slotPosX;
			int m_slotPosZ;
			int m_lod;
		};

		typedef std::map<Chunk::ChunkPos, Chunk*> MapChunkPerLod;
		typedef std::map<int, MapChunkPerLod> MapChunk;

		Chunk(enum class ChunkType type, int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat /*, godot::Ref<godot::Material>& seaMat*/);
		virtual ~Chunk();
		
		static enum class QuadType getQuadType(enum class ChunkType type);
		enum class QuadType getOwnQuadType(void) const;
		void initVisual(godot::Ref<godot::Material>& mat);
		//void initSeaVisual(godot::Ref<godot::Material>& seaMat);
		void deinit(void);
		void printDebugInfo(std::string method);

		// Actions
		virtual void enterWorld(void);
		virtual void exitWorld(void);
		virtual void onGlobalTransformChanged(void);
		virtual void setVisible(bool b);
		virtual void setDebugContentVisible(bool b);
		virtual void applyAABB(void);
		virtual void checkAndCalcAABB() = 0;
		virtual bool containsSea() = 0;
		//virtual void checkAndInitSeaVisual(godot::Ref<godot::Material>& seaMat);
		virtual void dump(void);
		virtual void setCameraPosInMap(Vector3 cameraPosInMap);
		virtual void setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode);
		virtual void applyDebugMesh(void);
		virtual Transform3D getDebugGlobalTransform(void);
		virtual Transform3D getDebugGlobalTransformApplied(void);
		virtual AABB getDebugMeshAABB(void) { return AABB(); };
		virtual bool isMeshNull(void);
		virtual bool isDebugMeshNull(void);
		virtual void refresh(bool isVisible);
		virtual void terrainHeightsChanged(void);
		virtual void seaInfoChanged(void);
		virtual void setMaterial(Ref<Material>& mat);
		virtual void update(bool isVisible);
		virtual void setActive(bool b);

		bool isActive(void) const
		{
			return m_active; 
		}
		bool isVisible(void) const
		{
			return m_visible; 
		}
		bool isDebugContentVisible(void) const
		{
			return m_debugContentVisible; 
		}
		bool isPendingUpdate(void) const
		{
			return m_pendingUpdate; 
		}
		void setPendingUpdate(bool b)
		{
			m_pendingUpdate = b; 
		}
		bool gotJustJoined(void) const
		{
			return m_justJoined; 
		}
		void setJustJoined(bool b)
		{
			m_justJoined = b; 
		}
		int getLod(void) const
		{
			return m_lod; 
		}
		int getSlotPosX(void) const
		{
			return m_slotPosX; 
		}
		int getSlotPosZ(void) const
		{
			return m_slotPosZ; 
		}
		std::string getIdStr(void)
		{
			return ChunkPos(m_slotPosX, m_slotPosZ, m_lod).getIdStr(); 
		};
		ChunkPos getPos(void)
		{
			return ChunkPos(m_slotPosX, m_slotPosZ, m_lod); 
		}
		float getChunkSizeInWUs(void) const
		{
			return m_chunkSizeInWUs;
		}
		AABB getAABB(void)
		{
			checkAndCalcAABB();
			return m_aabb;
		};
		//void getCameraPosInMap(Vector3& lastCameraPosInMap) const;
		bool isCameraVerticalOnChunk(void) const
		{
			return m_isCameraVerticalOnChunk;
		}
		void resetCameraVerticalOnChunk(void)
		{
			m_isCameraVerticalOnChunk = false;
		}
		Transform3D getGlobalTransform(void);
		virtual Transform3D getGlobalTransformApplied(void);
		void setPosInQuad(enum PosInQuad posInQuad, Quad* quad = nullptr) 
		{
			m_posInQuad = posInQuad;
			if (m_posInQuad == PosInQuad::NotSet)
				m_quad = nullptr;
			else
				m_quad = quad;
		};
		enum PosInQuad getPosInQuad(void) const
		{
			return m_posInQuad;
		}
		QuadTree* getQuadTree(void) 
		{
			return m_quadTree; 
		};
		bool isQuadTreeVisible(void);
		float getLowerXInWUsGlobal(void) const
		{
			return m_originXInWUsGlobal;
		}
		float getLowerZInWUsGlobal(void) const
		{
			return m_originZInWUsGlobal;
		}

		int getFirstWorldVertRow(void) const
		{
			return m_firstWorldVertRow;
		}
		int getFirstWorldVertCol(void) const
		{
			return m_firstWorldVertCol;
		}
		int getLastWorldVertRow(void) const
		{
			return m_lastWorldVertRow;
		}
		int getLastWorldVertCol(void) const
		{
			return m_lastWorldVertCol;
		}
		void checkMouseHit(void);
		bool checkHit(godot::Vector3 hit);
		void setDistanceFromCamera(float distanceFromCamera);
		float getDistanceFromCamera(void) const;
		Quad* getQuad(void)
		{
			return m_quad;
		}
		//godot::AABB getSeaAABB(void);
		enum class ChunkType chunkType(void) const
		{
			return m_chunkType;
		}
		void getGlobalCoordTerrainAABB(AABB& aabb, int firstWorldVertCol, int lastWorldVertCol, int firstWorldVertRow, int lastWorldVertRow, int step);

	private:
		void setMesh(Ref<Mesh> mesh);
		//void setSeaMesh(Ref<Mesh> mesh);

	protected:
		bool m_worldEntered = false;
		enum PosInQuad m_posInQuad;
		GDN_TheWorld_Viewer* m_viewer;
		QuadTree* m_quadTree;
		Quad* m_quad;
		AABB m_aabb;						// AABB of the chunk in WUs relative to the chunk so X and Z are 0
		AABB m_globalCoordAABB;				// AABB of the chunk in WUs in global coord.
		int m_lod;
		Vector3 m_lastCameraPosInMap;
		bool m_isCameraVerticalOnChunk;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_debugMode;
		bool m_debugContentVisible;
		Transform3D m_initialGlobalTransform;
		Transform3D m_globalTransformApplied;

		float m_distanceFromCamera;

		int m_numVerticesPerChuckSide;		// Number of vertices of the side of a chunk (-1) which is fixed (not a function of the lod) and is a multiple of 2
		int m_numChunksPerWorldGridSide;	// The number of chunks required to cover every side of the grid at the current lod value
		int m_gridStepInGridInWGVs;			// The number of World Grid vertices (-1) or grid vertices (-1) which separate the vertices of the mesh at the specified lod value
		float m_gridStepInWUs;				// The number of World Units which separate the vertices of the mesh at the specified lod value
		int m_chunkSizeInWGVs;				// The number of World Grid vertices (-1) of the chunk at the current lod
		float m_chunkSizeInWUs;				// The size in Word Units of the chunk at the current lod
		int m_originXInGridInWGVs;			// X coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is expressed in number of grid vertices
		int m_originZInGridInWGVs;			// Z coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is expressed in number of grid vertices
		float m_originXInWUsLocalToGrid;	// X coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is local to the Grid Map and is expressed in Viewer Node local coordinate System
		float m_originZInWUsLocalToGrid;	// Z coord. of the origin of the chunk (lower left corner) inside the Grid Map: it is local to the Grid Map and is expressed in Viewer Node local coordinate System
		float m_originXInWUsGlobal;			// X coord. of the origin of the chunk (lower left corner) inside the Grid Map: expressed in global coordinate System
		float m_originZInWUsGlobal;			// Z coord. of the origin of the chunk (lower left corner) inside the Grid Map: expressed in global coordinate System
		int m_firstWorldVertCol;			// 0-based starting column in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_lastWorldVertCol;				// 0-based ending column in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_firstWorldVertRow;			// 0-based starting row in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)
		int m_lastWorldVertRow;				// 0-based endif row in the World Grid vertices array (Map Manager grid map) of the chunk/mesh (viewer m_worldVertices)

	private:
		enum class ChunkType m_chunkType = ChunkType::NotSet;
		
		int m_slotPosX;	// express the horizontal (X) and vertical (Z) position of the chunk in the grid of chunks
		int m_slotPosZ;	// at the specific lod : 0 the first chunk, 1 the following to the max number of chunks on a size for the specific lod

		bool m_active;
		bool m_visible;
		bool m_pendingUpdate;
		bool m_justJoined = false;

		godot::RID m_meshInstanceRID;
		godot::RID m_meshRID;
		godot::Ref<godot::Mesh> m_mesh;
	};

	class TerrainChunk : public Chunk
	{
	public:
		TerrainChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat /*, godot::Ref<godot::Material>& seaMat*/);
		virtual ~TerrainChunk();
		
		virtual void terrainHeightsChanged(void);
		virtual void seaInfoChanged(void);
		virtual bool containsSea();

	protected:
		virtual void checkAndCalcAABB();

	protected:
		int m_containsSea = QUADRANT_SEA_INFO_SET_NOTSET;
	};

	class DebugTerrainChunk : public TerrainChunk
	{
	public:
		DebugTerrainChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat /*, godot::Ref<godot::Material>& seaMat*/);
		virtual ~DebugTerrainChunk();

		virtual void enterWorld(void);
		virtual void exitWorld(void);
		virtual void onGlobalTransformChanged(void);
		virtual void setVisible(bool b);
		virtual void setDebugContentVisible(bool b);
		virtual void applyAABB(void);
		virtual void checkAndCalcDebugMeshAABB();
		virtual void dump(void);
		virtual void setCameraPosInMap(Vector3 cameraPosInMap);
		virtual void setDebugMode(enum class GDN_TheWorld_Globals::ChunkDebugMode mode);
		virtual void applyDebugMesh(void);
		//virtual Transform getGlobalTransformOfAABB(void);
		virtual Transform3D getDebugGlobalTransform(void);
		virtual Transform3D getDebugGlobalTransformApplied(void);
		virtual AABB getDebugMeshAABB(void) { return m_debugMeshAABB; };
		virtual bool isDebugMeshNull(void);
		virtual void refresh(bool isVisible);
		virtual void terrainHeightsChanged(void);
		virtual void seaInfoChanged(void);
		virtual void update(bool isVisible);
		virtual void setActive(bool b);

	private:
		void setDebugMesh(Ref<Mesh> mesh, enum class GDN_TheWorld_Globals::ChunkDebugMode debugModeApplied);
		godot::Ref<godot::Mesh> createWireCubeMesh(Color c = Color(1, 1, 1));
		godot::Ref<godot::ArrayMesh> createWireSquareMesh(Color c = Color(1, 1, 1));
		//godot::Ref<godot::Mesh> createDebugBoxMesh(Color c);

	private:
		MeshInstance3D* m_debugMeshInstance;
		bool m_debugWorldEntered = false;
		RID m_debugMeshInstanceRID;
		RID m_debugMeshRID;
		Ref<Mesh> m_debugMesh;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_debugModeApplied = GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
		AABB m_debugMeshAABB;
		Transform3D m_debugGlobaTransformApplied;
		bool m_debugWirecubeMeshNeedRegen;
		bool m_debugWiresquareMeshNeedRegen;
	};

	class SeaChunk : public Chunk
	{
	public:
		SeaChunk(int slotPosX, int slotPosZ, int lod, GDN_TheWorld_Viewer* viewer, QuadTree* quadTree, godot::Ref<godot::Material>& mat /*, godot::Ref<godot::Material>& seaMat*/);
		virtual ~SeaChunk();

		virtual void terrainHeightsChanged(void);
		virtual void seaInfoChanged(void);
		virtual bool containsSea(void);

	protected:
		virtual void checkAndCalcAABB();

	protected:
		int m_containsSea = QUADRANT_SEA_INFO_SET_NOTSET;
	};

}
