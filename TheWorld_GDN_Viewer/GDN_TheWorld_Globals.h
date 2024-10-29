#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/classes/time.hpp>
#pragma warning(pop)

#include "assert.h"
#include "vector"

#include <plog/Log.h>
#include "GDN_TheWorld_exception.h"
#include "ClientServer.h"
//#include "QuadTree.h"
//#include <MapManager.h>
#include "Viewer_Utils.h"
#include "Utils.h"

//#define THEWORLD_SEA_LEVEL														0

#define THEWORLD_VIEWER_CHUNK_SIZE_SHIFT										3	// 4	// 5
#define THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT								11	/* 11 */	/* 10 */	// exponent of 2
#define MIN_RESOLUTION_SHIFT_FOR_TEXTURE_OPTIMIZATIONS							5	// exponent of 2

#define THEWORLD_TERRAIN_COLLISION_LAYER										1
#define THEWORLD_PLAYER_COLLISION_LAYER											2
#define THEWORLD_OBJECTS_COLLISION_LAYER										3

#define THEWORLD_DEFAULT_STREAM_WORLD											true
#define THEWORLD_DEFAULT_DEPTH_QUAD_ON_PERIMETER_IN_EDITOR						1
#define THEWORLD_DEFAULT_CACHE_QUAD_ON_PERIMETER_IN_EDITOR						1
#define THEWORLD_DEFAULT_DEPTH_QUAD_ON_PERIMETER								6	// 3
#define THEWORLD_DEFAULT_CACHE_QUAD_ON_PERIMETER								2	// 1
//#define THEWORLD_DEFAULT_CACHE_TERRAIN_IMAGES									true
//#define THEWORLD_DEFAULT_CACHE_TERRAIN_TEXTURES_ASYNC							true
//#define THEWORLD_DEFAULT_CACHE_HEIGHTS_FOR_INVISIBLE_QUADRANTS				true
//#define THEWORLD_DEFAULT_CACHE_HEIGHTS_FOR_LOWRES_QUADRANTS					false
#define THEWORLD_DEFAULT_THRESHOLD_CACHE_HEIGHTS								20000.0f	// 0.0f	// 20000.0F	// 15000.0F
#define THEWORLD_DEFAULT_FORCE_HIRES_COLLISION_INFO								false
#define THEWORLD_DEFAULT_DISABLE_TERRAIN_COLLISIONS								false
#define THEWORLD_DEFAULT_THRESHOLD_FOR_GLOBALMAP								0.0f	// 200000.0f
#define THEWORLD_DEFAULT_REFRESH_QUADRANTS_FACTOR								1.0f
#define THEWORLD_DEFAULT_REFRESH_CHUNKS_FACTOR									1.0f	// 0.1f
#define THEWORLD_DEFAULT_THRESHOLD_FOR_MAP_TEXTURE_OPTIMIZATION					5000.0f
#define THEWORLD_DEFAULT_TEXTURE_OPTIMIZATION_INCREMENT_FACTOR					4
#define THEWORLD_DEFAULT_TEXTURE_OPTIMIZATION_RESOLUTION_INCREMENT				2
#define THEWORLD_DEFAULT_ORIGIN_SHIFT_FACTOR									2
#define THEWORLD_DEFAULT_SHOW_SEA												true
#define THEWORLD_DEFAULT_SHOW_TERRAIN												true
#define THEWORLD_DEFAULT_SHOW_CHUNKS											false
//#define THEWORLD_DEFAULT_DEPTH_QUAD_WITH_COLLISION_INFO							0

#define PLOG_DEFAULT_LEVEL plog::Severity::info
#define PLOG_DEBUG_LEVEL plog::Severity::verbose

#define VISUAL_SERVER_WIREFRAME_MODE	true
#define DEFAULT_DEBUG_ENABLED			true

#define THEWORLD_MAIN_NODE_NAME							"GDN_TW_Main"
#define THEWORLD_GLOBALS_NODE_NAME						"GDN_TW_Globals"
#define THEWORLD_WORLD_VIEWER_NODE_NAME					"GDN_TW_WorldViewer"
#define THEWORLD_WORLD_VIEWPORT_CONTAINER_NODE_NAME		"GDN_TW_WorldViewportContainer"
#define THEWORLD_WORLD_VIEWPORT_NODE_NAME				"GDN_TW_WorldViewport"
#define THEWORLD_HUD_CONTROLS_NODE_NAME					"GDN_TW_HudControls"
#define THEWORLD_CAMERA_NODE_NAME						"GDN_TW_Camera"
#define THEWORLD_EDIT_MODE_UI_CONTROL_NAME				"GDN_TW_EditModeUI"
#define THEWORLD_MAP_VIEWER_NODE_NAME					"GDN_TW_MapViewer"
#define THEWORLD_MAP_VIEWPORT_CONTAINER_NODE_NAME		"GDN_TW_MapViewportContainer"
#define THEWORLD_MAP_VIEWPORT_NODE_NAME					"GDN_TW_MapViewport"
#define THEWORLD_MAP_NODE_NAME							"GDN_TW_Map"
#define THEWORLD_MAP_CAMERA_NODE_NAME					"GDN_TW_MapCamera"

#define THEWORLD_MEMORY_OPTIMIZATION	false

namespace godot
{
	class GridVertex;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Viewer;
	class TheWorld_MapManager::MapManager;

	enum class TheWorldStatus
	{
		error = -1,
		uninitialized = 0,
		initialized = 1,
		connectedToServer = 2,
		sessionInitialized = 3,
		worldDeployInProgress = 4,
		minimalWorldDeployed = 5,
		worldDeployed = 6,
		worldUnDeployInProgress = 100
	};

	static bool equal(Vector3 v1, Vector3 v2, const float epsilon = 0.00001)
	{
		return ::equal((float)v1.x, (float)v2.x, epsilon) && ::equal((float)v1.y, (float)v2.y, epsilon) && ::equal((float)v1.z, (float)v2.z, epsilon);
	}
	
	class GDN_TheWorld_Globals_Client : public TheWorld_ClientServer::ClientInterface
	{
	public:
		GDN_TheWorld_Globals_Client(GDN_TheWorld_Globals* globals, plog::Severity sev);

		void MapManagerSetLogMaxSeverity(plog::Severity sev);
		void ServerInitializeSession(bool isInEditor, plog::Severity sev);
		void MapManagerGetQuadrantVertices(bool isInEditor, float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, float lowerXGridVertex, float lowerZGridVertex, int anchorType, int numVerticesPerSize, float gridStepInWU, int level, bool setCamera, float cameraDistanceFromTerrainForced, std::string meshId);
		void MapManagerSync(bool isInEditor, bool inGameEditMode, size_t numVerticesPerSize, float gridStepInWU, std::string& quadrants, std::string& mapTextureRects);
		void MapManagerUploadBuffer(bool isInEditor, float lowerXGridVertex, float lowerZGridVertex, int numVerticesPerSize, float gridStepInWU, int level, std::string buffer, int32_t numQuadrantsPerSize, float quadrantGlobalSize, float quadrantWorldSize);
		void MapManagerDeployLevel(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepInWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll);
		void MapManagerDeployWorld(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepInWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll);
		void MapManagerUndeployWorld(bool isInEditor);
		void MapManagerUploadTextureRects(bool isInEditor, bool inGameEditMode, size_t numVerticesPerSize, float gridStepInWU, std::string& mapTextureRects);
		void MapManagerCheckTextureRects(bool isInEditor, bool inGameEditMode, size_t numVerticesPerSize, float gridStepInWU, std::string& mapTextureRects);
		virtual bool quitting(void);

	private:
		//GDN_TheWorld_Globals* Globals(bool useCache);

	private:
		GDN_TheWorld_Globals* m_globals;
	};


	class GDN_TheWorld_Globals : public Node, public TheWorld_ClientServer::ClientCallback
	{
		GDCLASS(GDN_TheWorld_Globals, Node)

		const float c_splitScale = 2.0F;	// 21/05/2024
		//const float c_splitScale = 1.0F;
		//const float c_splitScale = 2.0F;
		//const float c_splitScale = 1.5F;

	public:
		static const std::string c_groundTexturesDir;
		static const std::string c_albedo_bump_file_ext;
		static const std::string c_normal_roughness_file_ext;

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		const static Color g_color_white;
		const static Color g_color_red;
		const static Color g_color_dark_red;
		const static Color g_color_red_salmon;
		const static Color g_color_pink;
		const static Color g_color_light_pink;
		const static Color g_color_pink_amaranth;
		const static Color g_color_pink_bisque;
		const static Color g_color_pink_cerise;
		const static Color g_color_green;
		const static Color g_color_green1;
		const static Color g_color_pale_green;
		const static Color g_color_light_green;
		const static Color g_color_darksea_green;
		const static Color g_color_aquamarine_green;
		const static Color g_color_blue;
		const static Color g_color_yellow;
		const static Color g_color_yellow_almond;
		const static Color g_color_yellow_amber;
		const static Color g_color_yellow_apricot;
		const static Color g_color_cyan;
		const static Color g_color_light_cyan;
		const static Color g_color_black;

		enum class ChunkDebugMode
		{
			DoNotSet = -1,
			NoDebug = 0,
			WireframeOnAABB = 1,
			WireframeSquare = 2
		};

		static ChunkDebugMode rotateChunkDebugMode(ChunkDebugMode mode)
		{
			//if (mode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
			//	return GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
			//if (mode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
			//	return GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare;
			//if (mode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeSquare)
			//	return GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
			//else
			//	return GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet;
			if (mode == GDN_TheWorld_Globals::ChunkDebugMode::NoDebug)
				return GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB;
			if (mode == GDN_TheWorld_Globals::ChunkDebugMode::WireframeOnAABB)
				return GDN_TheWorld_Globals::ChunkDebugMode::NoDebug;
			else
				return GDN_TheWorld_Globals::ChunkDebugMode::DoNotSet;

		}

		GDN_TheWorld_Globals();
		~GDN_TheWorld_Globals();
		void init(bool isInEditor);
		void preDeinit(void);
		bool canDeinit(void);
		void deinit(void);
		bool initialized(void) const
		{
			return m_initialized;
		}
		void connectToServer(void);
		bool connectedToServer(void);
		void prepareDisconnectFromServer(void);
		bool canDisconnectFromServer(void);
		void disconnectFromServer(void);
		bool quitting(void) const
		{
			return m_quitting;
		}

		static std::string getClientDataDir(void);
		godot::String getCacheDir(int level);
		godot::String getConfigFilename(int level);

		virtual void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply) override;
		
		enum class TheWorldStatus status(void) const
		{
			return m_status;
		}
		bool statusError(void) const
		{
			return status() == TheWorldStatus::error;
		}
		bool statusUninitialized(void) const
		{
			return status() == TheWorldStatus::uninitialized;
		}
		bool statusInitialized(void) const
		{
			return status() == TheWorldStatus::initialized;
		}
		bool statusConnectedToServer(void) const
		{
			return status() == TheWorldStatus::connectedToServer;
		}
		bool statusSessionInitialized(void) const
		{
			return status() == TheWorldStatus::sessionInitialized;
		}
		bool statusWorldDeployInProgress(void) const
		{
			return status() == TheWorldStatus::worldDeployInProgress;
		}
		bool statusMinimalWorldDeployed(void) const
		{
			return status() == TheWorldStatus::minimalWorldDeployed;
		}
		bool statusWorldDeployed(void) const
		{
			return status() == TheWorldStatus::worldDeployed;
		}
		bool statusWorldUnDeployInProgress(void) const
		{
			return status() == TheWorldStatus::worldUnDeployInProgress;
		}
		void setStatus(enum class TheWorldStatus status);
		int get_status(void) const
		{
			return (int)status();
		}
		void set_status(int status)
		{
			setStatus((enum class TheWorldStatus)status);
		}

		std::string statusStr(enum class TheWorldStatus status)
		{
			std::string s = "NotDefined";
			switch (status)
			{
				case TheWorldStatus::error:
				{
					s = "error";
				}
				break;
				case TheWorldStatus::uninitialized:
				{
					s = "uninitialized";
				}
				break;
				case TheWorldStatus::initialized:
				{
					s = "initialized";
				}
				break;
				case TheWorldStatus::connectedToServer:
				{
					s = "connectedToServer";
				}
				break;
				case TheWorldStatus::sessionInitialized:
				{
					s = "sessionInitialized";
				}
				break;
				case TheWorldStatus::worldDeployInProgress:
				{
					s = "worldDeployInProgress";
				}
				break;
				case TheWorldStatus::minimalWorldDeployed:
				{
					s = "minimalWorldDeployed";
				}
				break;
				case TheWorldStatus::worldDeployed:
				{
					s = "worldDeployed";
				}
				break;
				case TheWorldStatus::worldUnDeployInProgress:
				{
					s = "worldUnDeployInProgress";
				}
				break;
			}

			return s;
		}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		void debugPrint(String message, bool godotPrint = true)
		{
			if (m_isDebugEnabled)
			{
				String editor_string = "";
				if (IS_EDITOR_HINT())
					editor_string = "***EDITOR*** ";
				String msg = "DEBUG - " + editor_string + message;
				if (godotPrint)
					godot::UtilityFunctions::print(msg);
				godot::PackedByteArray array = msg.to_ascii_buffer();
				std::string m((char*)array.ptr(), array.size());
				//std::string m = msg.utf8().get_data();
				PLOG_DEBUG << m;
			}
		}

		void warningPrint(String message, bool godotPrint = true)
		{
			String editor_string = "";
			if (IS_EDITOR_HINT())
				editor_string = "***EDITOR*** ";
			String msg = "WARNING - " + editor_string + message;
			if (godotPrint)
				godot::UtilityFunctions::print(msg);
			godot::PackedByteArray array = msg.to_ascii_buffer();
			std::string m((char*)array.ptr(), array.size());
			//std::string m = msg.utf8().get_data();
			PLOG_WARNING << m;
		}

		void errorPrint(String message, bool godotPrint = true)
		{
			String editor_string = "";
			if (IS_EDITOR_HINT())
				editor_string = "***EDITOR*** ";
			String msg = "ERROR - " + editor_string + message;
			if (godotPrint)
				godot::UtilityFunctions::print(msg);
			godot::PackedByteArray array = msg.to_ascii_buffer();
			std::string m((char*)array.ptr(), array.size());
			//std::string m = msg.utf8().get_data();
			PLOG_ERROR << m;
		}

		void infoPrint(String message, bool godotPrint = true)
		{
			String editor_string = "";
			//static godot::Engine* engine = godot::Engine::get_singleton();
			//if (engine->is_editor_hint())
			if (IS_EDITOR_HINT())
				editor_string = "***EDITOR*** ";
			String msg = "INFO - " + editor_string + message;
			GDN_TheWorld_Globals::print(msg, godotPrint);
		}
		
		void print(String message, bool godotPrint = true)
		{
			if (godotPrint)
				godot::UtilityFunctions::print(message);
			godot::PackedByteArray array = message.to_ascii_buffer();
			std::string m((char*)array.ptr(), array.size());
			//std::string m = message.utf8().get_data();
			PLOG_INFO << m;
		}

		GDN_TheWorld_Globals* Globals(bool useCache = true)
		{
			return this;
		}
			
		static godot::Error connectSignal(godot::Node* node, godot::String nodeType, godot::String signal, godot::Object* callableObject, godot::String callableMethod, godot::Variant custom1 = "", godot::Variant custom2 = "", godot::Variant custom3 = "");
		
		bool isEngineInEditor()
		{
			godot::Engine* engine = godot::Engine::get_singleton();
			return engine->is_editor_hint();
			//return m_isInEditor;	// it should be fixed in https://github.com/godotengine/godot/pull/77633 commit 31/05
		}

		bool resize(int chunkSizeShift, int heightmapResolutionShift, bool force = false);

		// WORLD GRID
		// The world is rapresented by a grid map (World Grid) streamed from external source which defines also the distance of each vertex of the grid map in WUs. Such a grid is a flat grid of vertices whose elevations
		//		are contained in a heightmap.
		// 
		// THE HEIGHTMAP: ELEVATIONS
		// The elevations are contained in a heightmap (for shader purposes) each for every vertex of the World Grid. The side of the heightmap has a number of vertices (heightmapResolution) which is multiple of the number
		//		of vertices of every chunk (numVerticesPerChuckSide); all th vertices of the heightmap relate to a quad tree of chunks and such vertices don't know about their positions in global coordinate system but
		//		only to their relative position in the chunk: to compute it, it is necessary to know the chunk position in global coords and its lod (which expresses how the vertices are spaced in number of World Grid vertices).
		//
		// CHUNKS AND QUADS
		// The world is subdivided in chunks by one or more mesh : 1 mesh for every chunk which contains the grid of vertices whose elevations are all contained in a heightmap (mostly in a part of it only at lower resolution
		//		there is only a chunk in which tge vertices are more spaced).
		// The world is subdivided in chunks organized in quads which means that every chunk can be split in 4 chunk (upper-left, upper-right, lesser-left, lesser-right) at every split the lod value is decremented by 1
		//		until lod value = 0 at which the chunk has the most defined mesh while at lod = numLods - 1 has the less defined mesh.
		//		Every chunks contain a mesh which contains the grid of vertices whose elevations are all contained in a heightmap (mostly in a part of it only at lower resolution there is only a chunk in which the vertices
		//		are more spaced).
		//		Every chunk has a fixed number of vertices for every side at every lod (numVerticesPerChuckSide), at higher lods the vertices are more spaced (in terms of World Grid vertices), at lod 0 the vertices are at
		//		the maximum density. The lod value is an index whose range is from 0 (the maximum level of details in which the vertex are less spaced and the details are more defined at his level the highest number of
		//		chunk is required to fullfill the elevation heightmap and the granularity of the vertices is the same of the World Grid) to max depth (maxDepth) = number of lods (numLods) - 1 (the maximum value of the index
		//		which matches the less defined mesh in which the vertices are more spaced and only 1 chunk is required to cover the elevation hryghtmap).
		//		The meshes of the chunks are expressed in local coord and the chunk is responsible to position them in he real world knowing their positions in global coordinate system.
		//
		// IN SHORT
		// In short: meshes and hryghtmap are expressed in local coordinate system, the heightmap is related to a quad tree of chunks and the chunks know the position aof every mesh in global coordinate system as the shaders know
		//		the elvation of every vertex in the heightmap (???)
		// At maximum lod (numLods - 1) the world is rapresented by a single chunk/mesh which can be split until lod = 0 for the maximun number of chunks, the point is that not every chunk of the world rapresentaion are at the
		//		same lod : the nearest are at maximun lod (lod value = 0), the farther at lod = numLods - 1
		// The maximum lod value (lodMaxDepth) is the maximum number of split - 1 that can be done so that all the vertices of all the chunks can be contained in the heightmap : it is computed as the power of 2 of the maximum number
		//		of chunks of a side of the heightmap (heightmapResolution / numVerticesPerChuckSide), the number of lods (numLods) is equal to lodMaxDepth + 1

		size_t heightmapResolutionShift(void)
		{
			return THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT;
		}

		// Number of vertices of the side of a chunk (-1) which is fixed (not a function of the lod) and is a multiple of 2
		int numVerticesPerChuckSide(void) const
		{
			if (m_numVerticesPerChuckSide == 0)
				PLOG_DEBUG << "Ahi ahi m_numVerticesPerChuckSide == 0";
			return m_numVerticesPerChuckSide; /* m_numVerticesPerChuckSide ==> 32 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5, 16 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 4, 8 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 3 */
		}	// Chunk num vertices -1
		
		// m_mapManager->gridStepInWU();
		float gridStepInWU(void) const;
		std::string getMapName(void);
			
		size_t numVerticesPerQuadrantSize(void) const
		{
			return (size_t)heightmapResolution() + 1;
		}
		
		// Number of vertices of the side of the heightmap (-1) with the elevations which is fixed and is a multiple of the number of vertices of the side of a chunk (numVerticesPerChuckSide) and is for this a multiple of 2 too
		int heightmapResolution(void) const
		{ 
			return m_heightmapResolution; /* m_heightmapResolution = 1024 con THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 10, m_heightmapResolution = 2048 con THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 11 */
		}	// Resolution of the heightmap = num point of the heightmap -1;
		// Size of the heightmap in WUs
		float heightmapSizeInWUs(void) const
		{
			return heightmapResolution() * gridStepInWU();
		}

		// Max value of the lod index (numLods - 1)
		int lodMaxDepth(void) const
		{	
			return m_lodMaxDepth; /* m_lodMaxDepth = 5 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5 e THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 10,  m_lodMaxDepth = 6 con THEWORLD_VIEWER_CHUNK_SIZE_SHIFT = 5 e THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT = 11 */
		}

		// Max number of quad split that can be done until the the granularity of the vertices is the same of the World Grid map and of the heightmap (lodMaxDepth + 1)
		int numLods(void) const
		{ 
			return m_numLods; /* m_numLods = 6 m_lodMaxDepth = 5 */
		}

		// The number of chunks required to cover every side of the heightmap at the specified lod value
		int numChunksPerHeightmapSide(int lod) const
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (heightmapResolution() / numVerticesPerChuckSide()) >> lod;
		}
		
		// The number of World Grid vertices (-1) or heightmap vertices (-1) which separate the vertices of the mesh at the specified lod value
		int strideInWorldGrid(int lod) const
		{
			return gridStepInVertices(lod);
		}
		int gridStepInVertices(int lod) const
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			//int ret = heightmapResolution() / (numChunksPerHeightmapSide(lod) * numVerticesPerChuckSide());
			//int ret1 = 1 << lod;
			//assert(ret == ret1);
			return (1 << lod);
		}

		// The number of World Units which separate the vertices of the mesh at the specified lod value
		float strideInWorldGridWUs(int lod) const
		{
			return gridStepInHeightmapWUs(lod);
		}
		float gridStepInHeightmapWUs(int lod) const
		{
			assert(!(lod < 0 || lod > lodMaxDepth()));
			if (lod < 0 || lod > lodMaxDepth())
				return -1;
			return (gridStepInVertices(lod) * gridStepInWU());
		}

		float splitScale(void) const
		{
			return c_splitScale;
		}

		bool isDebugEnabled(void) const
		{ 
			return m_isDebugEnabled;
		}
		void setDebugEnabled(bool b = true);

		size_t resolutionForOptimizationLevel(size_t optimizationLevel);
		size_t getMaxTextureOptimizationLevel(void);

		void setAppInError(int errorCode, String errorText)
		{
			godot::PackedByteArray array = errorText.to_ascii_buffer();
			std::string err((char*)array.ptr(), array.size());
			//const char* m = errorText.utf8().get_data();
			_setAppInError(errorCode, err.c_str());
		}
		void _setAppInError(int errorCode, std::string errorText)
		{
			m_lastErrorCode = errorCode;
			m_lastErrorText = errorText;
			m_errorText.push_back(errorText);
			m_bAppInError = true;
			PLOG_ERROR << errorText;
		}
		bool getAppInError(void) const
		{
			return _getAppInError();
		}
		int getAppInErrorCode(void) const
		{
			int lastErrorCode = 0;
			string message;
			_getAppInError(lastErrorCode, message);
			return lastErrorCode;
		}
		String getAppInErrorMsg(void) const
		{
			int lastErrorCode = 0;
			string message;
			_getAppInError(lastErrorCode, message);
			return message.c_str();
		}
		bool _getAppInError(void) const
		{ 
			return m_bAppInError; 
		}
		bool _getAppInError(int& lastErrorCode, std::string& lastErrorText) const
		{
			lastErrorCode = m_lastErrorCode;
			lastErrorText = m_lastErrorText;
			return m_bAppInError;
		}

		GDN_TheWorld_Viewer* Viewer(bool useCache = true);
		GDN_TheWorld_Globals_Client* Client(void) const
		{ 
			return m_client; 
		}

		int getTotElementToInitialize() const
		{
			return (int)m_totElementToInitialize;
		}
		void addElementToInitialize(int num = 1)
		{
			m_totElementToInitialize += num;
		}
		int getNumElementInitialized() const
		{
			return (int)m_numElementInitialized;
		}
		void addNumElementInitialized(int num = 1)
		{
			m_numElementInitialized += num;
		}

		bool getStreamWorld(void) const
		{
			return m_streamWorld;
		}
		void setStreamWorld(bool b);
		bool getParamProfilerActive(void)
		{
			return TheWorld_Utils::Profiler::isActive();
		}
		void setParamProfilerActive(bool b);
		//bool getParamCacheTerrainImages(void)
		//{
		//	return m_paramCacheTerrainImages;
		//}
		//void setParamCacheTerrainImages(bool b);
		//bool getParamCacheTerrainTexturesAsync(void)
		//{
		//	return m_paramCacheTerrainTexturesAsync;
		//}
		//void setParamCacheTerrainTexturesAsync(bool b);
		//bool getParamCacheHeightsForInvisibleQuadrants(void)
		//{
		//	return m_paramCacheHeightsForInvisibleQuadrants;
		//}
		//void setParamCacheHeightsForInvisibleQuadrants(bool b);
		//bool getParamCacheHeightsForLowResQuadrants(void)
		//{
		//	return m_paramCacheHeightsForLowResQuadrants;
		//}
		//void setParamCacheHeightsForLowResQuadrants(bool b);
		float getParamThresholdCacheHeights(void) const
		{
			return m_paramThresholdCacheHeights;
		}
		void setParamThresholdCacheHeights(float f);
		bool getParamForceHiResCollisionInfo(void) const
		{
			return m_paramForceHiResCollisionInfo;
		}
		void setParamForceHiResCollisionInfo(bool b);
		bool getParamDisableTerrainCollisions(void) const
		{
			return m_paramDisableTerrainCollisions;
		}
		void setParamDisableTerrainCollisions(bool b);
		float getRefreshQuadrantTreeThreshold(void) const
		{
			float factor = getParamRefreshQuadrantsFactor();
			if (factor == 0)
				return 0;
			else
				return heightmapSizeInWUs() * factor;
		}
		float getParamRefreshQuadrantsFactor(void) const
		{
			return m_paramRefreshQuadrantsFactor;
		}
		void setParamRefreshQuadrantsFactor(float f);
		float getRefreshChunkLodThreshold(void) const
		{
			float factor = getParamRefreshChunksFactor();
			if (factor == 0)
				return 0;
			else
				return  (numVerticesPerChuckSide() * gridStepInHeightmapWUs(0)) * factor;
		}
		float getParamRefreshChunksFactor(void) const
		{
			return m_paramRefreshChunksFactor;
		}
		void setParamRefreshChunksFactor(float f);
		float getParamThresholdForGlobalmap(void) const
		{
			return m_paramThresholdForGlobalmap;
		}
		void setParamThresholdForGlobalmap(float f);
		float getParamThresholdForMapTextureOptimization(void) const
		{
			return m_paramThresholdForMapTextureOptimization;
		}
		void setParamThresholdForMapTextureOptimization(float f);
		float getParamTextureOptimizationIncrementFactor(void) const
		{
			return m_paramTextureOptimizationIncrementFactor;
		}
		void setParamTextureOptimizationIncrementFactor(float f);
		int64_t getParamTextureOptimizationResolutionIncrement(void) const
		{
			return m_paramTextureOptimizationResolutionIncrement;
		}
		void setParamTextureOptimizationResolutionIncrement(int64_t f);
		int64_t getParamOriginShiftFactor(void) const
		{
			return m_paramOriginShiftFactor;
		}
		void setParamOriginShiftFactor(int64_t f);
		bool getParamShowSea(void) const
		{
			return m_paramShowSea;
		}
		void setParamShowSea(bool b);
		bool getParamShowSeaChanged(void) const
		{
			return m_paramShowSeaChanged;
		}
		void setParamShowSeaChanged(bool b);
		bool getParamShowTerrain(void) const
		{
			return m_paramShowTerrain;
		}
		void setParamShowTerrain(bool b);
		bool getParamShowTerrainChanged(void) const
		{
			return m_paramShowTerrainChanged;
		}
		void setParamShowTerrainChanged(bool b);
		bool getParamShowChunks(void) const
		{
			return m_paramShowChunks;
		}
		void setParamShowChunks(bool f);
		//int64_t getParamDepthQuadWithCollisionInfo(void)
		//{
		//	return m_paramDepthQuadWithCollisionInfo;
		//}
		//void setParamDepthQuadWithCollisionInfo(int64_t f);
		bool getParamElevationZero(void) const
		{
			return m_paramElevationZero;
		}
		void setParamElevationZero(bool f);

		void setTextureBlendingSize(float f)
		{
			m_textureBlendingSize = f;
		}
		float getTextureBlendingSize(void) const
		{
			return m_textureBlendingSize;
		}
		void setSplatmapBlendingSize(float f)
		{
			m_splatmapBlendingSize = f;
		}
		float getSplatmapBlendingSize(void) const
		{
			return m_splatmapBlendingSize;
		}

		uint64_t elapsedFromStart(void)
		{
			if (m_startTime == 0)
				m_startTime = godot::Time::get_singleton()->get_ticks_msec();

			return godot::Time::get_singleton()->get_ticks_msec() - m_startTime;
		}

		double elapsedFromStartInSeconds(void)
		{
			return double(elapsedFromStart()) / 1000;
		}

		double internalTimeInSeconds(double _delta)
		{
			m_startInternalTimeInSeconds += _delta;
			return m_startInternalTimeInSeconds;
		}

		double shaderParamGlobalTimeApplied(void) const
		{
			return m_shaderGlobalTimeApplied;
		}
		void shaderParamGlobalTimeApplied(double d);

		bool loadingScreen(void) const
		{
			return m_loadingScreen;

		}
		void loadingScreen(bool b)
		{
			m_loadingScreen = b;

		}
		void setLoadingScreen(bool b)
		{
			loadingScreen(b);
		}

		bool loadingScreenExitOnFullWorldDeploy(void) const
		{
			return m_loadingScreenExitOnFullWorldDeploy;
		}
		void loadingScreenExitOnFullWorldDeploy(bool b)
		{
			m_loadingScreenExitOnFullWorldDeploy = b;

		}
		void setLoadingScreenExitOnFullWorldDeploy(bool b)
		{
			loadingScreenExitOnFullWorldDeploy(b);
		}
		
		// DEBUG
		//static size_t s_num;
		//static size_t s_elapsed1;
		//static size_t s_elapsed2;
		//static size_t s_elapsed3;
		//static size_t s_elapsed4;
		//static size_t s_elapsed5;

	private:
		bool m_initialized;
		bool m_quitting;
		bool m_ready;
		enum class TheWorldStatus m_status;
		TimerMs m_statusChangeClock;
		bool m_isDebugEnabled;
		int m_chunkSizeShift;
		int m_heightmapResolutionShift;
		int m_numVerticesPerChuckSide;
		int m_heightmapResolution;
		int m_lodMaxDepth;
		int m_numLods;
		//TheWorld_MapManager::MapManager *m_mapManager;
		bool m_bAppInError;
		int m_lastErrorCode;
		std::string m_lastErrorText;
		std::vector<std::string> m_errorText;
		float m_gridStepInWU;
		std::string m_mapName;
		GDN_TheWorld_Globals_Client* m_client;
		bool m_isInEditor;

		size_t m_totElementToInitialize;
		size_t m_numElementInitialized;

		float m_textureBlendingSize = 0.0f;
		float m_splatmapBlendingSize = 0.0f;
		
		// Node cache
		GDN_TheWorld_Viewer* m_viewer = nullptr;

		uint64_t m_startTime = 0;
		double m_startInternalTimeInSeconds = 0;
		double m_shaderGlobalTimeApplied = 0;

		// Node parameters
		bool m_streamWorld = THEWORLD_DEFAULT_STREAM_WORLD;
		//bool m_paramCacheTerrainImages = THEWORLD_DEFAULT_CACHE_TERRAIN_IMAGES;
		//bool m_paramCacheTerrainTexturesAsync = THEWORLD_DEFAULT_CACHE_TERRAIN_TEXTURES_ASYNC;
		//bool m_paramCacheHeightsForInvisibleQuadrants = THEWORLD_DEFAULT_CACHE_HEIGHTS_FOR_INVISIBLE_QUADRANTS;
		//bool m_paramCacheHeightsForLowResQuadrants = THEWORLD_DEFAULT_CACHE_HEIGHTS_FOR_LOWRES_QUADRANTS;
		float m_paramThresholdCacheHeights = THEWORLD_DEFAULT_THRESHOLD_CACHE_HEIGHTS;
		bool m_paramForceHiResCollisionInfo = THEWORLD_DEFAULT_FORCE_HIRES_COLLISION_INFO;
		bool m_paramDisableTerrainCollisions = THEWORLD_DEFAULT_DISABLE_TERRAIN_COLLISIONS;
		float m_paramRefreshQuadrantsFactor = THEWORLD_DEFAULT_REFRESH_QUADRANTS_FACTOR;		// 1 ==> very low rate of refresh, the higher the more frequent the refresh is
		float m_paramRefreshChunksFactor = THEWORLD_DEFAULT_REFRESH_CHUNKS_FACTOR;		// 1 ==> very low rate of refresh, the higher the more frequent the refresh is
		float m_paramThresholdForGlobalmap = THEWORLD_DEFAULT_THRESHOLD_FOR_GLOBALMAP;
		float m_paramThresholdForMapTextureOptimization = THEWORLD_DEFAULT_THRESHOLD_FOR_MAP_TEXTURE_OPTIMIZATION;
		float m_paramTextureOptimizationIncrementFactor = THEWORLD_DEFAULT_TEXTURE_OPTIMIZATION_INCREMENT_FACTOR;
		int64_t m_paramTextureOptimizationResolutionIncrement = THEWORLD_DEFAULT_TEXTURE_OPTIMIZATION_RESOLUTION_INCREMENT;
		int64_t m_paramOriginShiftFactor = THEWORLD_DEFAULT_ORIGIN_SHIFT_FACTOR;
		bool m_paramShowSea = THEWORLD_DEFAULT_SHOW_SEA;
		bool m_paramShowSeaChanged = false;
		bool m_paramShowTerrain = THEWORLD_DEFAULT_SHOW_TERRAIN;
		bool m_paramShowTerrainChanged = false;
		bool m_paramShowChunks = THEWORLD_DEFAULT_SHOW_CHUNKS;
		bool m_paramElevationZero = false;
		bool m_loadingScreen = true;
		bool m_loadingScreenExitOnFullWorldDeploy = true;
		//int64_t m_paramDepthQuadWithCollisionInfo = THEWORLD_DEFAULT_DEPTH_QUAD_WITH_COLLISION_INFO;
	};
}

#define THEWORLD_VIEWER_GENERIC_ERROR				1

