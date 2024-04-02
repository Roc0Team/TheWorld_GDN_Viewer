#pragma once

#pragma warning(push, 0)
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/shader_material.hpp>
#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/texture.hpp>
#include <godot_cpp/classes/editor_plugin.hpp>
#include <godot_cpp/classes/camera3d.hpp>
#include <godot_cpp/classes/mesh_instance3d.hpp>
#include <godot_cpp/classes/immediate_mesh.hpp>
#include <godot_cpp/classes/standard_material3d.hpp>
#include <godot_cpp/classes/sprite2d.hpp>
#pragma warning(pop)

#include <map>
#include <memory>
#include <thread>

#include "GDN_TheWorld_Globals.h"
#include "QuadTree.h"
//#include <MapManager.h>

#define MAX_DEPTH_ON_PERIMETER	6
#define MAX_CACHE_ON_PERIMETER	4

namespace godot
{
	class MeshCache;
	class QuadTree;
	class Chunk;
	class GDN_TheWorld_Globals;
	class GDN_TheWorld_Camera;
	class GDN_TheWorld_Edit;
	class GDN_TheWorld_Drawer;
	class GDN_TheWorld_Gizmo3d;

	typedef std::map<TheWorld_Utils::QuadrantPos, std::unique_ptr<QuadTree>> MapQuadTree;

#define STREAMER_SLEEP_TIME			10
#define STREAMER_SYNC_PERIOD_TIME	1000
#define FAR_HORIZON_MULTIPLIER	50

#define MIN_MAP_SCALE 0.01F
#define TIME_INTERVAL_BETWEEN_DUMP 0			// secs, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_STATISTICS 1000	// ms, 0 = diasble periodic dump
#define TIME_INTERVAL_BETWEEN_MOUSE_TRACK 50	// 250	// ms
#define TIME_INTERVAL_BETWEEN_ELAB_CAMERA -1	// 2000	// ms

	class GDN_TheWorld_MapModder : public godot::Node
	{
		GDCLASS(GDN_TheWorld_MapModder, Node)

	public:
		enum class MapType
		{
			uninitialized = 0
			,initializedForNormals = 1
			,initializedForSplatmap = 2
			,initializedForGlobalmap = 3
		};

	protected:
		static void _bind_methods();
		void _notification(int p_what);

	public:
		GDN_TheWorld_MapModder();
		~GDN_TheWorld_MapModder();

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;

		void custom_ready(GDN_TheWorld_Viewer* viewer, Node* parent, bool debug = false);
		void debug_process(double _delta);

		void framePostDraw(void);

		void initForNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
		void initForSplatmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer,
			float slopeVerticalFactor,
			float slopeFlatFactor,
			float dirtOnRocksFactor,
			float highElevationFactor,
			float lowElevationFactor,
			float minHeigth,
			float maxHeigth,
			size_t tileSize, size_t viewportBorder = 0, bool signalRequired = false);
		void initForGlobalMap(QuadTree* quadtree, bool signalRequired = false);
		void deinit(void);
		void startProcessing(void);
		bool working(void);
		bool ready(void);
		TheWorld_Utils::MemoryBuffer* getOutBuffer(void);
		void startProcessRequired(bool b);
		bool startProcessRequired(void) const;
		void deinitRequired(bool b);
		bool deinitRequired(void) const;
		static void incNumWorking()
		{
			s_numWorking++;
		}
		static void decNumWorking()
		{
			std::lock_guard<std::recursive_mutex> lock(s_mtxInternal);
			if (s_numWorking > 0)
				s_numWorking--;
		}
		static size_t getNumWorking()
		{
			return s_numWorking;
		}

	private:
		bool workToDo(void);

	private:
		//const int32_t c_uninitialized = 0;
		//const int32_t c_initializedForNormals = 1;
		//const int32_t c_initializedForSplatmap = 2;
		//const int32_t c_initializedForGlobalmap = 3;
		enum class MapType m_initialized = MapType::uninitialized;

		bool m_startProcessingRequired = false;
		bool m_deinitRequired = false;

		static int32_t s_numWorking;
		static std::recursive_mutex s_mtxInternal;

		bool m_signalRequired = false;

		GDN_TheWorld_Viewer* m_viewer = nullptr;
		Node* m_parent = nullptr;
		int32_t m_numProcessExecutions = 0;
		size_t m_tileSize = 0;
		size_t m_viewportBorder = 0;
		size_t m_imageSize = 0;
		TheWorld_Utils::MemoryBuffer* m_float32HeigthsBuffer = nullptr;
		size_t m_numHeights = 0;
		TheWorld_Utils::MemoryBuffer* m_normalsBuffer = nullptr;
		size_t m_numNormals = 0;
		float m_slopeVerticalFactor = 0;
		float m_slopeFlatFactor = 0;
		float m_dirtOnRocksFactor = 0;
		float m_highElevationFactor = 0;
		float m_lowElevationFactor = 0;
		float m_minHeigth = 0;
		float m_maxHeigth = 0;
		QuadTree* m_quadTree = nullptr;
		godot::SubViewport* m_viewport = nullptr;
		godot::Sprite2D* m_map = nullptr;
		godot::MeshInstance3D* m_plane = nullptr;
		godot::Camera3D* m_camera = nullptr;
		godot::Vector3 m_displacementPlane;

		const int16_t c_tileStatusWaiting = 0;
		const int16_t c_tileStatusProcessing = 1;
		std::map<godot::Vector2i, int16_t> m_tiles;
		std::queue<godot::Vector2i> m_tilesQueue;
		godot::Vector2i m_currentTile;
		bool m_processingTile = false;

		godot::Ref<godot::Image> m_outImage;
		bool m_outImageValid = false;
		std::unique_ptr<TheWorld_Utils::MemoryBuffer> m_outBuffer;
		bool m_frameAvailable = false;

		godot::Sprite2D* m_debugSprite = nullptr;
	};

	// World Node Local Coordinate System is the same as MapManager coordinate system
	// Viewer Node origin is in the lower corner (X and Z) of the vertex bitmap at altitude 0
	// Chunk and QuadTree coordinates are in Viewer Node local coordinate System
	// X growing on East direction
	// Z growing on South direction
	class GDN_TheWorld_Viewer : public godot::Node3D, public TheWorld_ClientServer::ClientCallback, public TheWorld_Utils::ThreadInitDeinit
	{
		GDCLASS(GDN_TheWorld_Viewer, Node3D)

		enum class TrackedInputEvent
		{
			MOUSE_BUTTON_LEFT = 0
			, MOUSE_BUTTON_RIGHT = 1
			, MOUSE_DOUBLE_BUTTON_LEFT = 2
			, MOUSE_DOUBLE_BUTTON_RIGHT = 3
		};
			
		enum class Margin {
			MARGIN_LEFT,
			MARGIN_TOP,
			MARGIN_RIGHT,
			MARGIN_BOTTOM
		};

		friend class GDN_TheWorld_Edit;
		friend class Chunk;
		//friend class QuadTree;
		//friend class ShaderTerrainData;

	protected:
		static void _bind_methods();
		void _notification(int p_what);
		void _init(void);

	public:
		class ProcessStep
		{
		public:

			ProcessStep() {}

			enum class ProcessStepIdx
			{
				not_set = -1
				, process_updateCameraQuad = 0
				, process_initNewQuads = 1
				, process_trackMouse = 2
				, process_updateLodInQuads = 3
				, process_updateMeshForUpdatedChunks = 4
				, UpdateTerrainVisibility = 5
				, UpdateDebugMode = 6
				, process_lookDev = 7
				, ReloadSelectedQuadrantShader = 8
				, ReloadAllShaders = 9
				, ShaderParamChanged = 10
				, Quadrant_resetMaterialParams_heightmap = 11
				, Quadrant_resetMaterialParams_normalmap = 12
				, Quadrant_resetMaterialParams_splatmap = 13
				, Quadrant_resetMaterialParams_globalmap = 14
				, Quadrant_resetMaterialParams_colormap = 15
				, Quadrant_updateMaterialParams = 16
				, Quadrant_updateCollider = 17
				, EndOfProcessExecution = 18
				, process_updateCameraInQuadrants = 19
				, process_updateOriginShift = 20
			};
			ProcessStep(enum class ProcessStepIdx idx, size_t elapsed, int num = 1)
			{
				m_idx = idx;
				m_num = num;
				m_elapsed = elapsed;
			}
			void setElapsed(size_t elapsed)
			{
				m_elapsed = elapsed;
				m_num++;
			}
			static std::string idxToString(enum class ProcessStepIdx idx)
			{
				switch (idx)
				{
				case ProcessStepIdx::process_updateCameraQuad:
					return "process_updateCameraQuad";
				case ProcessStepIdx::process_initNewQuads:
					return "process_initNewQuads";
				case ProcessStepIdx::process_trackMouse:
					return "process_trackMouse";
				case ProcessStepIdx::process_updateLodInQuads:
					return "process_updateLodInQuads";
				case ProcessStepIdx::process_updateMeshForUpdatedChunks:
					return "process_updateMeshForUpdatedChunks";
				case ProcessStepIdx::UpdateTerrainVisibility:
					return "UpdateTerrainVisibility";
				case ProcessStepIdx::UpdateDebugMode:
					return "UpdateDebugMode";
				case ProcessStepIdx::process_lookDev:
					return "process_lookDev";
				case ProcessStepIdx::ReloadSelectedQuadrantShader:
					return "ReloadSelectedQuadrantShader";
				case ProcessStepIdx::ReloadAllShaders:
					return "ReloadAllShaders";
				case ProcessStepIdx::ShaderParamChanged:
					return "ShaderParamChanged";
				case ProcessStepIdx::Quadrant_resetMaterialParams_heightmap:
					return "Quadrant_resetMaterialParams_heightmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_normalmap:
					return "Quadrant_resetMaterialParams_normalmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_splatmap:
					return "Quadrant_resetMaterialParams_splatmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_globalmap:
					return "Quadrant_resetMaterialParams_globalmap";
				case ProcessStepIdx::Quadrant_resetMaterialParams_colormap:
					return "Quadrant_resetMaterialParams_colormap";
				case ProcessStepIdx::Quadrant_updateMaterialParams:
					return "Quadrant_updateMaterialParams";
				case ProcessStepIdx::Quadrant_updateCollider:
					return "Quadrant_updateCollider";
				case ProcessStepIdx::EndOfProcessExecution:
					return "EndOfProcessExecution";
				default:
					return "not_set";
				}
			}
			std::string name(void)
			{
				return idxToString(m_idx);
			}
			std::string msg(void)
			{
				return name() + (m_num > 1 ? "(" + std::to_string(m_num)  + ")" : "") + " = " + std::to_string(m_elapsed / 1000);
			}
			
		private:
			enum class ProcessStepIdx m_idx = ProcessStepIdx::not_set;
			int m_num = 0;
			size_t m_elapsed = 0;
		};

		class ProcessSteps
		{
		public:

			ProcessSteps() {}

			void init()
			{
				m_processClock.tick();
				m_steps.clear();
			}

			size_t addStep(enum class ProcessStep::ProcessStepIdx idx)
			{
				size_t elapsed = (size_t)m_processClock.partialDuration().count();
				if (m_steps.contains(idx))
					m_steps[idx].setElapsed(elapsed);
				else
					m_steps[idx] = ProcessStep(idx, elapsed);
				m_lastStepIdx = idx;
				return elapsed;
			}

			size_t elapsed()
			{
				return (size_t)m_processClock.partialDuration().count();
			}

			std::map<enum class ProcessStep::ProcessStepIdx, ProcessStep>& steps(void)
			{
				return m_steps;
			}

			std::string lastAddedStepName(void)
			{
				return ProcessStep::idxToString(m_lastStepIdx);
			}

		private:
			std::map<enum class ProcessStep::ProcessStepIdx, ProcessStep> m_steps;
			TimerMcs m_processClock;
			enum class ProcessStep::ProcessStepIdx m_lastStepIdx = ProcessStep::ProcessStepIdx::not_set;
		};

		GDN_TheWorld_Viewer();
		~GDN_TheWorld_Viewer();
		bool initRequired(void) const
		{
			return m_initRequired;
		};
		void initRequired(bool b)
		{
			m_initRequired = b;
		};
		bool init(void);
		bool canDeinit(void);
		void preDeinit(void);
		void deinit(void);
		bool quitting() const
		{
			return m_quitting;
		}
		
		virtual void threadInit(void) {}
		virtual void threadDeinit(void) {}

		void replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply);

		void setDesideredLookDev(enum class ShaderTerrainData::LookDev desideredLookDev)
		{
			if (m_desiderdLookDev != desideredLookDev)
			{
				m_desiderdLookDev = desideredLookDev;
				m_desideredLookDevChanged = true;
			}
		}

		enum class ShaderTerrainData::LookDev getDesideredLookDev(void) const
		{
			return m_desiderdLookDev;
		}

		//
		// Godot Standard Functions
		//
		virtual void _ready(void) override;
		virtual void _process(double _delta) override;
		virtual void _physics_process(double _delta) override;
		virtual void _input(const Ref<InputEvent>& event) override;
		
		void process_impl(double _delta, godot::Camera3D* activeCamera);

		//Ref<Material> getRegularMaterial(void);
		//Ref<Material> getLookDevMaterial(void);
		godot::Plane getShaderParamGroundUVScale() const
		{
			return m_shaderParam_groundUVScale;
		}
		bool getShaderParamDepthBlening() const
		{
			return m_shaderParam_depthBlending;
		}
		bool getShaderParamTriplanar() const
		{
			return m_shaderParam_triplanar;
		}
		godot::Plane getShaderParamTileReduction() const
		{
			return m_shaderParam_tileReduction;
		}
		float getShaderParamGlobalmapBlendStart() const
		{
			return m_shaderParam_globalmapBlendStart;
		}
		float getShaderParamGlobalmapBlendDistance() const
		{
			return m_shaderParam_globalmapBlendDistance;
		}
		godot::Plane getShaderParamColormapOpacity() const
		{
			return m_shaderParam_colormapOpacity;
		}

		float getShaderParamQuadBorderLineThickness(void) const
		{
			return m_shaderParam_quad_border_line_thickness;
		}
		float getShaderParamMouseHitRadius(void) const
		{
			//if (positionVisible())
				return m_shaderParam_mouse_hit_radius;
			//else
			//	return 0;
		}
		void setShaderParam(godot::String name, godot::Variant value);
		void debugPrint(String message)
		{
			Globals()->debugPrint(message);
		}
		void errorPrint(String message)
		{
			Globals()->errorPrint(message);
		}
		void warningPrint(String message)
		{
			Globals()->warningPrint(message);
		}
		void infoPrint(String message)
		{
			Globals()->infoPrint(message);
		}
		void print(String message)
		{
			Globals()->print(message);
		}
		void setEditorInterface(godot::Object* editorInterface)
		{
			m_editorInterface = (godot::EditorInterface*)editorInterface;
		}
		void setEditor3dOverlay(godot::Object* editor3dOverlay)
		{
			m_editor3dOverlay = (godot::Control*)editor3dOverlay;
		}
		bool trackMouse(void) const;
		void trackMouse(bool b);
		void toggleTrackMouse(void);
		void toggleEditMode(void);
		void setEditMode(bool editMode);
		bool isEditMode(void) const;
		void toggleDebugVisibility(void);
		void rotateChunkDebugMode(void);
		void toggleWireframe(void) const;
		void rotateDrawingMode(void);
		bool getTrackMouseState(void) const
		{
			return trackMouse();
		}
		//void toggleFullScreen(void);
		void toggleQuadrantSelected(void);
		godot::Camera3D* getCamera(void);
		godot::Camera3D* getCameraInEditor(void);
		void setEditorCamera(godot::Camera3D* editorCamera);
		void setDepthQuadOnPerimeter(int depth);
		int getDepthQuadOnPerimeter(void) const;
		void setCacheQuadOnPerimeter(int cache);
		int getCacheQuadOnPerimeter(void) const;
		void setForceShaderElevation0(bool elevation0);
		bool getForceShaderElevation0(void) const;
		String getInfoCamera(void);
		int getCameraProjectionMode(void);
		GDN_TheWorld_Globals* Globals(bool useCache = true);
		GDN_TheWorld_Camera* CameraNode(bool useCache = true);
		godot::GDN_TheWorld_Edit* EditModeUIControl(bool useCache = true);
		godot::Control* getOrCreateEditModeUIControl(void);
		void createEditModeUI(void);
		void deployWorld(float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll, int level, int chunkSizeShift, int heightmapResolutionShift);
		void undeployWorld(void);
		void destroyModeUIControl(void);
		bool initialWordlViewerPosSet(void) const
		{
			return m_initialWordlViewerPosSet;
		}
		void recalcQuadrantsInView(void);
		Node3D* getWorldNode(void) const;
		MeshCache* getMeshCache(void)
		{
			return m_meshCache.get(); 
		}
		Transform3D getInternalGlobalTransform(void) const;
		void setDumpRequired(void)
		{
			m_dumpRequired = true; 
		}
		void dump(void);
		void dumpRecurseIntoChildrenNodes(Array nodes, int level);
		void setCameraChunk(Chunk* chunk, QuadTree* quadTree);
		AABB getCameraChunkLocalAABB(void);
		AABB getCameraChunkLocalDebugAABB(void);
		Transform3D getCameraChunkGlobalTransformApplied(void);
		Transform3D getCameraChunkDebugGlobalTransformApplied(void);
		String getCameraChunkId(void);
		String getCameraQuadrantName(void);
		int getNumSplits(void) const;
		int getNumJoins(void) const;
		int getNumChunks(void) const;
		int getNumActiveChunks(void) const;
		int getProcessDuration(void) const;
		int getRefreshMapQuadDuration(void) const;
		int getUpdateQuads1Duration(void) const;
		int getUpdateQuads2Duration(void) const;
		int getUpdateQuads3Duration(void) const;
		int getUpdateChunksDuration(void) const;
		int getUpdateMaterialParamsDuration(void) const;
		int getMouseTrackHitDuration(void) const;
		int getProcessNotOwnsLock(void) const;
		int getNumQuadrant(void) const;
		int getNuminitializedQuadrant(void) const;
		int getNumVisibleQuadrant(void) const;
		int getNuminitializedVisibleQuadrant(void) const;
		int getNumNeedShaderResetQuadrant(void) const;
		int getNumEmptyQuadrant(void) const;
		int getNumQuadrantReloadingFromCache(void) const;
		int getNumFlushedQuadrant(void) const;
		int getQuadrantNumWithCollider(void) const;
		int getQuadrantNumWithHiResCollider(void) const;
		int getQuadrantNumWithLowResCollider(void) const;
		void refreshQuadTreeStatistics(void);
		void resetMaterialParamsForEveryQuadrant(void);
		void updateMaterialParamsForEveryQuadrant(void);
		void setColliderNeedUpdateForEveryQuadrant(void);
		GDN_TheWorld_Globals::ChunkDebugMode getRequiredChunkDebugMode(void) const
		{
			return m_requiredChunkDebugMode;
		}
		String getChunkDebugModeStr(void) const;
		bool getDebugContentVisibility(void) const
		{
			return m_debugContentVisibility; 
		}
		String getDebugDrawMode(void);
		bool useVisualServer(void) const
		{
			return m_useVisualServer; 
		}
		void forceRefreshMapQuadTree(void) 
		{
			m_refreshMapQuadTree = true; 
		}
		void getAllQuadrantPos(std::vector<TheWorld_Utils::QuadrantPos>& allQuandrantPos);
		QuadTree* getQuadTree(TheWorld_Utils::QuadrantPos pos, bool excludeToErase = true, bool lockQuadrant = false);
		Chunk* getActiveChunkAt(TheWorld_Utils::QuadrantPos pos, Chunk::ChunkPos chunkPos, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter);
		Chunk* getActiveChunkAt(Chunk* chunk, enum class Chunk::DirectionSlot dir, Chunk::LookForChunk filter);
		bool terrainShiftPermitted(void);
		godot::Vector3 getMouseHitInMap(void) const
		{
			if (m_mouseTrackedOnSomething)
				return m_trackedMouseHitInMap;
			else
				return godot::Vector3();
		}
		float getMouseHitDistanceFromCamera(void)
		{
			if (m_mouseTrackedOnSomething)
			{
				godot::Camera3D* camera = getCamera();
				return (float)camera->get_global_transform().origin.distance_to(m_trackedMouseHitGlobal);
			}
			else
				return 0.0f;
		}
		godot::String _getMouseQuadrantHitName(void)
		{
			return getMouseQuadrantHitName().c_str();
		}
		std::string getMouseQuadrantHitName(void)
		{
			return m_mouseQuadrantHitName;
		}
		godot::String _getMouseQuadrantHitTag(void)
		{
			return getMouseQuadrantHitTag().c_str();
		}
		std::string getMouseQuadrantHitTag(void)
		{
			return m_mouseQuadrantHitTag;
		}
		godot::Vector3 getMouseQuadrantHitPos(void) const
		{
			return m_mouseQuadrantHitPos;
		}
		float getMouseQuadrantHitSize(void) const
		{
			return m_mouseQuadrantHitSize;
		}
		godot::String getMouseChunkHitId(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return std::string(m_mouseHitChunk->getQuadTree()->getQuadrant()->getPos().getName() + " " + m_mouseHitChunk->getIdStr()).c_str();
				//return m_mouseHitChunk->getIdStr().c_str();
			}
			else
				return "";
		}
		godot::Vector3 getMouseChunkHitPos(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return Vector3(m_mouseHitChunk->getLowerXInWUsGlobal(), 0, m_mouseHitChunk->getLowerZInWUsGlobal());
			}
			else
				return Vector3();
		}
		float getMouseChunkHitSize(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitChunk->getChunkSizeInWUs();
			}
			else
				return 0.0;
		}

		float getMouseChunkHitDistFromCam(void)
		{
			if (m_mouseTrackedOnSomething && m_mouseHitChunk != nullptr && m_mouseHitChunk->isActive() /* && m_mouseHitQuadTree != nullptr*/)
			{
				return m_mouseHitChunk->getDistanceFromCamera();
			}
			else
				return 0.0;
		}

		void setMouseHitChunk(Chunk* chunk)
		{
			m_mouseHitChunk = chunk;
		}
		//void setMouseHitQuadTree(QuadTree* quadTree)
		//{
		//	m_mouseHitQuadTree = quadTree;
		//}

		bool editMode(void) const
		{
			return m_editMode;
		}

		//void GenerateHeigths(void);

		std::recursive_mutex& getMainProcessingMutex(void)
		{
			return m_mtxQuadTreeAndMainProcessing;
		}

		std::string to_string(godot::String s)
		{
			//godot::PackedByteArray byteStream = s.to_utf8_buffer();
			const char* str = s.utf8().get_data();
			std::string ret = s.utf8().get_data();
			//char* str = s.alloc_c_string();
			//std::string ret = str;
			//godot::api->godot_free(str);
			return ret;
		}

		bool isInEditor(void)
		{
			if (m_isInEditorInitialized)
				return m_isInEditor;

			GDN_TheWorld_Globals* g = Globals();
			if (g == nullptr)
				return false;

			m_isInEditor = IS_EDITOR_HINT();
			m_isInEditorInitialized = true;
			
			return m_isInEditor;
		}

		godot::Vector2 getMousPosInViewport(void)
		{
			if (isInEditor())
			{
				if (m_editor3dOverlay != nullptr)
				{
					return m_editor3dOverlay->get_local_mouse_position();
				}
				else
					return godot::Vector2(0.0f, 0.0f);
			}
			else
				return get_viewport()->get_mouse_position();
		}
		
		godot::Size2 getViewportSize(void)
		{
			if (isInEditor())
			{
				if (m_editor3dOverlay != nullptr)
				{
					godot::Rect2 rect = m_editor3dOverlay->get_rect();
					return godot::Size2(rect.size);
				}
				else
					return godot::Size2(0.0f, 0.0f);
			}
			else
			{
				godot::Rect2 rect = get_viewport()->get_visible_rect();
				return godot::Size2(rect.size);
			}
		}
		
		void viewportSizeChanged(void);
		bool ReloadAllShadersRequired(void) const
		{
			return m_reloadAllShadersRequired;
		}
		void ReloadAllShadersRequired(bool b)
		{
			m_reloadAllShadersRequired = b;
		}
		bool ReloadSelectedQuadrantShaderRequired(void) const
		{
			return m_reloadSelectedQuadrantShaderRequired;
		}
		void ReloadSelectedQuadrantShaderRequired(bool b)
		{
			m_reloadSelectedQuadrantShaderRequired = b;
		}
		void trackedMouseHitChanged(QuadTree* quadrantHit, std::list<QuadTree*>& adjacentQuadrantsHit, bool hit, godot::Vector3 mouseHit, godot::Vector3 normalHit = Vector3Zero);
		void clearMapQuadTree(void);
		bool gizmoVisible(void) const
		{
			return m_gizmoVisible;
		}
		void gizmoVisible(bool b)
		{
			m_gizmoVisible = b;
		}
		bool normalVisible(void) const
		{
			return m_normalVisible;
		}
		void normalVisible(bool b)
		{
			m_normalVisible = b;
		}
		bool positionVisible(void) const
		{
			return m_positionVisible;
		}
		void positionVisible(bool b)
		{
			m_positionVisible = b;
			if (!m_positionVisible)
				trackingDistance(false);
		}
		bool gridOn(void) const
		{
			return m_gridOn;
		}
		void gridOn(bool b)
		{
			if (m_gridOn != b)
			{
				if (isQuadrantSelectedForEdit())
				{
					QuadTree* quadTreeSelectedForEdit = nullptr;
					TheWorld_Utils::QuadrantPos pos = getQuadrantSelForEdit(&quadTreeSelectedForEdit);
					quadTreeSelectedForEdit->materialParamsNeedUpdate(true);
				}

				if (!b)
					updateMaterialParamsForEveryQuadrant();
			}

			m_gridOn = b;
		}
		bool trackingDistance(void) const
		{
			return m_trackingDistance;
		}
		void trackingDistance(bool b, godot::Vector3 savedPos = godot::Vector3())
		{
			m_trackingDistance = b;
			if (m_trackingDistance)
				m_savedPosForTrackingDistance = savedPos;
			else
				if (m_positionDrawer != nullptr)
				{
					if (m_positionLineIdx != -1)
						m_positionDrawer->updateLine(m_positionLineIdx, godot::Vector3(), godot::Vector3());
					if (m_positionSphereStartIdx != -1)
						m_positionDrawer->updateSphere(m_positionSphereStartIdx, godot::Vector3(), 0.0f);
					if (m_positionSphereEndIdx != -1)
						m_positionDrawer->updateSphere(m_positionSphereEndIdx, godot::Vector3(), 0.0f);
				}
		}
		bool trackedInputEvent(TrackedInputEvent event, std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (inputEvents->contains(event))
				return (*inputEvents)[event];
			else
				return false;
		}
		void setTrackedInputEvents(std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (!m_trackedInputEvents.empty())
			{
				for (auto& trackedInputEvent : m_trackedInputEvents)
				{
					(*inputEvents)[trackedInputEvent] = true;
				}
			}
		}
		void resetTrackedInputEvents(std::map<enum class TrackedInputEvent, bool>* inputEvents = nullptr)
		{
			if (inputEvents == nullptr)
				inputEvents = &m_trackedInputEventsForProcess;

			if (!inputEvents->empty())
				inputEvents->clear();
			if (!m_trackedInputEvents.empty())
				m_trackedInputEvents.clear();
		}

		bool mouseInsideMainEditPanel(void);

		void quadrantModderWorkDone(int32_t map_type, float x, float z, int32_t num_vertices_per_side, int32_t level, float grid_step);

		void incNumVisibleQuadrantsStreamed(void)
		{
			m_numVisibleQuadrantsStreamed++;
		}
		void incNumInvisibleQuadrantsStreamed(void)
		{
			m_numInvisibleQuadrantsStreamed++;
		}
		void incNumTextureResize(void)
		{
			m_numTextureResizing++;
		}
		void addToTextureResizingElapsed(size_t elapsed)
		{
			m_elapsedTextureResizing += elapsed;
		}
		void incNumOptimizationLevelChange(void)
		{
			m_numOptimizationLevelChange++;
		}
		void incNumLoadTerrainData(size_t elapsedLoadTerrainData)
		{
			m_numLoadTerrainData++;
			m_elapsedLoadTerrainData += elapsedLoadTerrainData;
		}

		void incNumResetShaderParam(size_t elapsedResetShaderParam)
		{
			m_numResetShaderParam++;
			m_elapsedResetShaderParam += elapsedResetShaderParam;
		}
		void incNumUpdateShaderParam(size_t elapsedUpdateShaderParam)
		{
			m_numUpdateShaderParam++;
			m_elapsedUpdateShaderParam += elapsedUpdateShaderParam;
		}
		void incNumSetHiResCollisionData(size_t elapsedSetHiResCollisionData)
		{
			m_numSetHiResCollisionData++;
			m_elapsedSetHiResCollisionData += elapsedSetHiResCollisionData;
		}
		void incNumSetLowResCollisionData(size_t elapsedSetLowResCollisionData)
		{
			m_numSetLowResCollisionData++;
			m_elapsedSetLowResCollisionData += elapsedSetLowResCollisionData;
		}

		size_t addProcessStep(enum class ProcessStep::ProcessStepIdx idx)
		{
			return m_processSteps.addStep(idx);
		}
		//void forceProcessCameraPosForQuadrants(void)
		//{
		//	m_lastProcessedCameraPosForQuadrants = godot::Vector3();
		//}
		void forceProcessCameraPosForChunks(void)
		{
			m_lastProcessedCameraPosForChunks = godot::Vector3();
		}
		bool processCanBeInterrupted(enum class ProcessStep::ProcessStepIdx idx)
		{
			if (m_processCanBeInterrupted)
			{
				size_t elapsed = addProcessStep(idx);
				if (elapsed > m_maxProcessDuration)
					return true;
				else
					return false;
			}
			else
				return false;
		}

		void recalcQuadrantsInViewNeeded(bool b)
		{
			m_recalcQuadrantsInViewNeeded = b;
		}

		bool loadQuadrantData(float posX, float posZ, int level);
		bool loadQuadrantDataIndex(int posX, int posZ, int level);
		void clearQuadrantData(void);
		godot::Ref<godot::Image> getQuadrantHeightmapImage(void);
		godot::Ref<godot::Image> getQuadrantNormalmapImage(void);
		godot::Ref<godot::Image> getQuadrantSplatmapImage(void);
		godot::Ref<godot::Image> getQuadrantGlobalmapImage(void);
		godot::Ref<godot::Image> getQuadrantColormapImage(void);
		godot::Vector2 getQuadrantPos(void);
		godot::Vector2 getQuadrantPosIndex(void);
		godot::String getGroundTextureName(int type);
		godot::Ref<godot::ImageTexture> getGroundTextureAlbedoBump(godot::String texName);
		godot::Ref<godot::ImageTexture> getGroundTextureNormalRoughness(godot::String texName);
		int getGroundTextureLayer(godot::String texName);
		godot::Vector4 getGroundTextureLayers(void);

		godot::Transform3D globalToMap(godot::Transform3D global) const;
		godot::Transform3D globalPosToMapTransform(godot::Vector3 global) const;
		godot::Transform3D mapToGlobal(godot::Transform3D map) const;
		godot::Transform3D mapPosToGlobalTransform(godot::Vector3 map) const;

		void startInitializingStreamOfQuadrants(void);
		void endInitializingStreamOfQuadrants(void);

	private:
		void _findChildNodes(godot::Array& foundNodes, godot::Array& searchNodes, String searchClass);
		void onTransformChanged(void);
		GDN_TheWorld_Camera* WorldCamera(bool useCache = true)
		{ 
			return m_worldCamera;
		};
		void assignWorldCamera(GDN_TheWorld_Camera* camera)
		{
			m_worldCamera = camera;
		};
		void printKeyboardMapping(void);
		void streamer(void);
		void process_eraseOutOfViewQuads(GDN_TheWorld_Edit* editModeUIControl);
		void process_mapModderAndUnlockInternalDataUntilCompleteProcessExecution(double _delta);
		void process_updateCameraQuad(GDN_TheWorld_Globals* globals, godot::Vector3& cameraPosInMap);
		void process_initNewQuads(bool processCameraPosForQuadrants);
		void process_updateCameraPosInShader(godot::Camera3D* activeCamera);
		void process_updateOriginShift(godot::Camera3D* activeCamera);
		void setAnchorForOriginShift(godot::Node3D* anchorForOriginShift);
		godot::Node3D* getAnchorForOriginShift(void);
		void process_trackMouse(godot::Camera3D* activeCamera);
		void process_updateLodInQuads(godot::Vector3& cameraPosInMap);
		void process_updateMeshForUpdatedChunks(void);
		void process_lookDev(void);
		void process_materialParam(godot::Vector3& cameraPosInMap, bool processCameraPosForQuadrants, bool processCameraPosForChunks);
		void process_neighbourQuadrantData(QuadTree* quadTree);
		void process_dump(GDN_TheWorld_Globals* globals);
		void process_statistics(GDN_TheWorld_Globals* globals);

		void writeStatisticsNewQuadrants(void);
		TheWorld_Utils::QuadrantPos getQuadrantSelForEdit(QuadTree** quadTreeSel);
		bool isQuadrantSelectedForEdit(void);
		godot::Camera3D* m_editorCamera;

	private:
		bool m_initialized;
		godot::Node3D* m_anchorForOriginShift = nullptr;
		bool m_worldEntered = false;
		bool m_quitting;
		bool m_visibleInTree;
		bool m_initRequired;
		bool m_isInEditor;
		bool m_isInEditorInitialized;
		bool m_ready;
		bool m_useVisualServer;
		bool m_altPressed;
		bool m_ctrlPressed;
		bool m_shiftPressed;
		bool m_firstProcess;
		int64_t m_timeElapsedFromLastDump;
		bool m_initialWordlViewerPosSet;
		bool m_dumpRequired;
		Chunk* lastTrackedChunk;
		//Vector3 m_mapScaleVector;
		enum class ShaderTerrainData::LookDev m_desiderdLookDev;
		bool m_desideredLookDevChanged;
		bool m_stopDeploy;

		//GDN_TheWorld_MapModder* m_mapModder = nullptr;

		// Statistics data
		//TheWorld_Viewer_Utils::TimerMcs m_processClock;
		size_t m_maxProcessDuration = 0;
		bool m_processCanBeInterrupted = false;
		bool m_lastProcessExecutionInterrupted = false;
		ProcessSteps m_processSteps;
		//std::vector<ProcessStep> m_processSteps;
		//std::string m_lastProcessedStep;
		size_t m_numProcessExecution = 0;				long long m_sumProcessDuration = 0;					size_t m_averageProcessDuration = 0;				size_t m_lastProcessDuration = 0;
		size_t m_numProcessImplExecution = 0;			long long m_sumProcessImplDuration = 0;				size_t m_averageProcessImplDuration = 0;			size_t m_lastProcessImplDuration = 0;
		//size_t m_numRefreshMapQuad = 0;				long long m_sumRefreshMapQuadDuration = 0;			size_t m_averageRefreshMapQuadDuration = 0;			size_t m_lastRefreshMapQuadDuration = 0;
		size_t m_numUpdateQuads1 = 0;					long long m_sumUpdateQuads1Duration = 0;			size_t m_averageUpdateQuads1Duration = 0;			size_t m_lastUpdateQuads1Duration = 0;
		size_t m_numUpdateQuads2 = 0;					long long m_sumUpdateQuads2Duration = 0;			size_t m_averageUpdateQuads2Duration = 0;			size_t m_lastUpdateQuads2Duration = 0;
		size_t m_numUpdateQuads3 = 0;					long long m_sumUpdateQuads3Duration = 0;			size_t m_averageUpdateQuads3Duration = 0;			size_t m_lastUpdateQuads3Duration = 0;
		size_t m_numUpdateChunks = 0;					long long m_sumUpdateChunksDuration = 0;			size_t m_averageUpdateChunksDuration = 0;			size_t m_lastUpdateChunksDuration = 0;
		size_t m_numUpdateMaterialParams = 0;			long long m_sumUpdateMaterialParamsDuration = 0;	size_t m_averageUpdateMaterialParamsDuration = 0;	size_t m_lastUpdateMaterialParamsDuration = 0;
		size_t m_numMouseTrackHit = 0;					long long m_sumMouseTrackHitDuration = 0;			size_t m_averageMouseTrackHitDuration = 0;			size_t m_lastMouseTrackHitDuration = 0;
	
		size_t m_numEraseQuads = 0;						long long m_sumEraseQuads = 0;						size_t m_averageEraseQuads = 0;						size_t m_lastEraseQuads = 0;
		size_t m_numProcessMapModder = 0;				long long m_sumProcessMapModder = 0;				size_t m_averageProcessMapModder = 0;				size_t m_lastProcessMapModder = 0;
		size_t m_numUpdateCameraQuad = 0;				long long m_sumUpdateCameraQuad = 0;				size_t m_averageUpdateCameraQuad = 0;				size_t m_lastUpdateCameraQuad = 0;
		size_t m_numStreamingQuadrantStuff = 0;			long long m_sumStreamingQuadrantStuff = 0;			size_t m_averageStreamingQuadrantStuff = 0;			size_t m_lastStreamingQuadrantStuff = 0;

		int64_t m_timeElapsedFromLastStatistic;
		int m_numSplits;
		int m_numJoins;
		int m_numChunks = 0;
		map<int, size_t> m_mapNumActiveChunksPerLod;
		int m_numActiveChunks;
		int m_numProcessNotOwnsLock;
		int m_numQuadrant;
		int m_numinitializedQuadrant;
		int m_numVisibleQuadrant;
		int m_numEmptyQuadrant;
		int m_numQuadrantReloadingFromCache = 0;
		int m_numFlushedQuadrant;
		int m_numQuadrantWithCollider = 0;
		int m_numQuadrantWithHiResCollider = 0;
		int m_numQuadrantWithLowResCollider = 0;
		int m_numinitializedVisibleQuadrant;
		int m_numQuadrantNeedResetMaterialParam = 0;
		
		TimerMs m_streamingTime;
		TimerMs m_notStreamingTime;
		bool m_streamingInProgress = false;
		size_t m_numVisibleQuadrantsStreamed = 0;
		size_t m_numInvisibleQuadrantsStreamed = 0;
		size_t m_numTextureResizing = 0;
		size_t m_elapsedTextureResizing = 0;
		size_t m_numOptimizationLevelChange = 0;
		size_t m_numLoadTerrainData = 0;
		size_t m_elapsedLoadTerrainData = 0;
		size_t m_numResetShaderParam = 0;
		size_t m_elapsedResetShaderParam = 0;
		size_t m_numUpdateShaderParam = 0;
		size_t m_elapsedUpdateShaderParam = 0;
		size_t m_numSetHiResCollisionData = 0;
		size_t m_elapsedSetHiResCollisionData = 0;
		size_t m_numSetLowResCollisionData = 0;
		size_t m_elapsedSetLowResCollisionData = 0;
		// Statistics data

		std::map<enum class TrackedInputEvent, bool> m_trackedInputEventsForProcess;
		std::map<enum class TrackedInputEvent, bool> m_trackedInputEventsForTrackMouse;
		std::list<enum class TrackedInputEvent> m_trackedInputEvents;

		bool m_trackMouse = false;
		bool m_editMode = false;
		int64_t m_timeElapsedFromLastMouseTrack;
		godot::Vector3 m_trackedMouseHitGlobal;
		godot::Vector3 m_trackedMouseHitInMap;
		godot::Vector3 m_previousTrackedMouseHitInMap;
		bool m_mouseTrackedOnSomething = false;
		bool m_mouseTrackedOnTerrain = false;
		std::string m_mouseQuadrantHitName;
		std::string m_mouseQuadrantHitTag;
		float m_mouseQuadrantHitSize;
		godot::Vector3 m_mouseQuadrantHitPos;
		TheWorld_Utils::QuadrantPos m_quadrantHitPos;
		QuadTree* m_quadrantHit = nullptr;
		std::list<QuadTree*> m_adjacentQuadrantsHit;
		TheWorld_Utils::QuadrantPos m_quadrantSelPos;
		Chunk* m_mouseHitChunk;
		GDN_TheWorld_Drawer* m_normalDrawer = nullptr;
		GDN_TheWorld_Drawer* m_positionDrawer = nullptr;
		int32_t m_normalLineIdx = -1;
		int32_t m_normalSphereIdx = -1;
		int32_t m_positionLabelXIdx = -1;
		int32_t m_positionLabelYIdx = -1;
		int32_t m_positionLabelZIdx = -1;
		int32_t m_positionLineIdx = -1;
		int32_t m_positionSphereStartIdx = -1;
		int32_t m_positionSphereEndIdx = -1;
		bool m_normalVisible = false;
		GDN_TheWorld_Gizmo3d* m_gizmo = nullptr;
		bool m_gizmoVisible = false;
		bool m_positionVisible = false;
		bool m_trackingDistance = false;
		bool m_gridOn = false;
		godot::Vector3 m_savedPosForTrackingDistance;

		bool m_debugContentVisibility;
		bool m_updateTerrainVisibilityRequired;
		
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_currentChunkDebugMode;
		enum class GDN_TheWorld_Globals::ChunkDebugMode m_requiredChunkDebugMode;
		bool m_updateDebugModeRequired;
		
		Viewport::DebugDraw m_debugDraw;

		bool m_refreshRequired;

		//RID m_viewFrustumMeshInstance;
		//RID m_viewFrustumMeshRID;
		//Ref<Mesh> m_viewFrustumMesh;

		// MeshCache
		std::unique_ptr<MeshCache> m_meshCache;

		// Viewer (Camera)
		GDN_TheWorld_Camera* m_worldCamera = nullptr;
		Chunk* m_cameraChunk = nullptr;
		//QuadTree* m_cameraQuadTree;
		
		// World Data
		MapQuadTree m_mapQuadTree;	// Actually only 1 level is managed (to manage more levels we shold have a map for each level)
		std::recursive_mutex m_mtxQuadTreeAndMainProcessing;
		std::map<TheWorld_Utils::QuadrantPos, bool> m_toErase;
		TheWorld_Utils::QuadrantPos m_computedCameraQuadrantPos;
		TheWorld_Utils::QuadrantPos m_originShiftedQuadrantPos;
		godot::Vector3 m_lastOriginShiftCameraPosInMap;
		bool m_recalcQuadrantsInViewNeeded;
		bool m_refreshMapQuadTree;
		int m_numWorldVerticesPerSize;
		int m_worldViewerLevel;		// actually world viewer manage one level at the time, otherwise we should have multiple quadtrees
		size_t m_numVisibleQuadrantOnPerimeter;
		size_t m_numCacheQuadrantOnPerimeter;
		// Node cache
		GDN_TheWorld_Globals* m_globals;
		GDN_TheWorld_Edit* m_editModeUIControl;
		//Ref<ShaderMaterial> m_regularMaterial;
		//Ref<ShaderMaterial> m_lookDevMaterial;
		
		// Shader parameters
		godot::Plane m_shaderParam_groundUVScale;
		bool m_shaderParam_depthBlending;
		bool m_shaderParam_triplanar;
		godot::Plane m_shaderParam_tileReduction;
		float m_shaderParam_globalmapBlendStart;
		float m_shaderParam_globalmapBlendDistance;
		float m_shaderParam_quad_border_line_thickness = 0.005f;
		float m_shaderParam_mouse_hit_radius = 0.001f;
		godot::Plane  m_shaderParam_colormapOpacity;
		bool m_shaderParamChanged;
		bool m_reloadAllShadersRequired = false;
		bool m_reloadSelectedQuadrantShaderRequired = false;

		// streamer thread
		std::thread m_streamerThread;
		bool m_streamerThreadRequiredExit;
		bool m_streamerThreadRunning;

		// Editor stuff
		godot::EditorInterface* m_editorInterface;
		godot::Control* m_editor3dOverlay;
		size_t m_depthQuadOnPerimeter;
		size_t m_cacheQuadOnPerimeter;
		bool m_forceShaderElevation0 = false;

		godot::Vector3 m_lastProcessedCameraPosForQuadrants;
		Chunk* m_lastCameraChunk = nullptr;
		int64_t m_timeElapsedFromLastProcessCameraPosForQuadrants = 0;
		godot::Vector3 m_lastProcessedCameraPosForChunks;

		float m_lastCameraAltitudeSetToShader = -1.0f;
		godot::Vector3 m_lastCameraPosSetToShader;
		bool m_lastCameraPosSetToShaderInitialized = false;

		TheWorld_Utils::ThreadPool m_tpClientWorker;

		bool m_quadrantSyncRunning = false;

		// Loaded quadrant images (mostly for testing)
		TheWorld_Utils::QuadrantPos m_tempPos;
		TheWorld_Utils::TerrainEdit m_tempTerrainEdit;
		godot::Ref<godot::Image> m_tempHeightmapImage;
		godot::Ref<godot::Image> m_tempNormalmapImage;
		godot::Ref<godot::Image> m_tempSplatmapImage;
		godot::Ref<godot::Image> m_tempGlobalmapImage;
		godot::Ref<godot::Image> m_tempColormapImage;
	};
}

//	/root
//		...											created in Godot editor
//			/TheWorld_Main							created in Godot editor
//				/GDN_TheWorld_MainNode				Only to startup the viewer and globals
//				/GDN_TheWorld_Globals
//				/GDN_TheWorld_Viewer
//				/GDN_TheWorld_Camera
//				/EditModeUI
