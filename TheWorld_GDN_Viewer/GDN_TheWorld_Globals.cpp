//#include "pch.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_TheWorld_Camera.h"

#pragma warning(push, 0)
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/time.hpp>
#pragma warning(pop)

static bool g_plogInitialized = false;

//#include <MapManager.h>
#include <plog/Initializers/RollingFileInitializer.h>

std::string getModuleLoadPath(void);

// Gestire le eccezioni della comunicazione verso il server

const godot::Color godot::GDN_TheWorld_Globals::g_color_white = godot::Color(255.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red = godot::Color(255.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_dark_red = godot::Color(139.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_red_salmon = godot::Color(250.0F / 255.0F, 128.0F / 255.0F, 114.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink = godot::Color(255.0F / 255.0F, 192.0F / 255.0F, 203.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_pink = godot::Color(255.0F / 255.0F, 182.0F / 255.0F, 193.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_amaranth = godot::Color(159.0F / 255.0F, 43.0F / 255.0F, 104.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_bisque = godot::Color(242.0F / 255.0F, 210.0F / 255.0F, 189.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pink_cerise = godot::Color(222.0F / 255.0F, 49.0F / 255.0F, 99.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green = godot::Color(0.0F / 255.0F, 255.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_green1 = godot::Color(0.0F / 255.0F, 128.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_pale_green = godot::Color(152.0F / 255.0F, 251.0F / 255.0F, 152.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_green = godot::Color(144.0F / 255.0F, 238.0F / 255.0F, 144.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_darksea_green = godot::Color(143.0F / 255.0F, 188.0F / 255.0F, 143.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_aquamarine_green = godot::Color(127.0F / 255.0F, 255.0F / 255.0F, 212.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_blue = godot::Color(0.0F / 255.0F, 0.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_cyan = godot::Color(0.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_light_cyan = godot::Color(150.0F / 255.0F, 255.0F / 255.0F, 255.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow = godot::Color(255.0F / 255.0F, 255.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_almond = godot::Color(234.0F / 255.0F, 221.0F / 255.0F, 202.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_amber = godot::Color(255.0F / 255.0F, 191.0F / 255.0F, 0.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_yellow_apricot = godot::Color(251.0F / 255.0F, 206.0F / 255.0F, 177.0F / 255.0F);
const godot::Color godot::GDN_TheWorld_Globals::g_color_black = godot::Color(0.0F / 255.0F, 0.0F / 255.0F, 0.0F / 255.0F);

// DEBUG
//size_t godot::GDN_TheWorld_Globals::s_num = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed1 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed2 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed3 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed4 = 0;
//size_t godot::GDN_TheWorld_Globals::s_elapsed5 = 0;

const std::string godot::GDN_TheWorld_Globals::c_groundTexturesDir = "res://assets/textures/ground/";
const std::string godot::GDN_TheWorld_Globals::c_albedo_bump_file_ext = "_albedo_bump.ground";
const std::string godot::GDN_TheWorld_Globals::c_normal_roughness_file_ext = "_normal_roughness.ground";

namespace godot
{

	std::string GDN_TheWorld_Globals::getClientDataDir(void)
	{
		String userPath = OS::get_singleton()->get_user_data_dir();
		godot::PackedByteArray array = userPath.to_ascii_buffer();
		std::string dir((char*)array.ptr(), array.size());
		size_t l = dir.length();
		//const char* s = userPath.utf8().get_data();
		//std::string dir = std::string(s) + "\\TheWorld";

		if (dir.length() < 10)
		{
			std::string msg = std::string("Beccato " + dir);
			PLOG_ERROR << msg;
			throw(GDN_TheWorld_Exception(__FUNCTION__, msg.c_str()));
		}

		return dir;
	}

	String GDN_TheWorld_Globals::getCacheDir(int level)
	{
		std::string clientDataDir = GDN_TheWorld_Globals::getClientDataDir();
		if (clientDataDir.length() == 0)
			return "";

		std::string mapName = getMapName();
		if (mapName.length() == 0)
			return "";

		std::string cacheDir = TheWorld_Utils::MeshCacheBuffer::cacheDir(clientDataDir, mapName, gridStepInWU(), numVerticesPerQuadrantSize(), level);

		return cacheDir.c_str();
	}

	String GDN_TheWorld_Globals::getConfigFilename(int level)
	{
		String cacheDir = getCacheDir(level);
		if (cacheDir.length() == 0)
			return "";
		
		return cacheDir + "\\TheWorldConfig.cfg";
	}


	godot::Error GDN_TheWorld_Globals::connectSignal(godot::Node* node, godot::String nodeType, godot::String signal, godot::Object* callableObject, godot::String callableMethod, godot::Variant custom1, godot::Variant custom2, godot::Variant custom3)
	{
		// parameters added to those inserted by the emit method
		godot::Array args;
		args.append(node);
		args.append(signal);
		args.append(nodeType);
		args.append(node->get_instance_id());
		args.append(node->get_name());
		args.append(custom1);
		args.append(custom2);
		args.append(custom3);

		int64_t size = args.size();
		godot::Error e = node->connect(signal, Callable(callableObject, callableMethod).bindv(args));
		return e;
	}

	GDN_TheWorld_Globals_Client::GDN_TheWorld_Globals_Client(GDN_TheWorld_Globals* globals, plog::Severity sev) : TheWorld_ClientServer::ClientInterface(sev)
	{
		m_globals = globals;
	}

	bool GDN_TheWorld_Globals_Client::quitting(void)
	{
		return m_globals->quitting();
	}

	void GDN_TheWorld_Globals_Client::MapManagerSetLogMaxSeverity(plog::Severity sev)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back((int)sev);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, (TheWorld_ClientServer::ClientCallback*)m_globals);
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(),"", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::ServerInitializeSession(bool isInEditor, plog::Severity sev)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		inputParams.push_back((int)sev);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, (TheWorld_ClientServer::ClientCallback*)m_globals);
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodAsync ==> " THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerGetQuadrantVertices(bool isInEditor, float cameraX, float cameraY, float cameraZ, float cameraYaw, float cameraPitch, float cameraRoll, float lowerXGridVertex, float lowerZGridVertex, int anchorType, int numVerticesPerSize, float gridStepinWU, int level, bool setCamera, float cameraDistanceFromTerrainForced, std::string meshId)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraZ);
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		std::vector<unsigned char> _meshId(meshId.begin(), meshId.end());
		inputParams.push_back(_meshId);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_GETQUADRANTVERTICES " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::Sync(bool isInEditor, bool inGameEditMode, size_t numVerticesPerSize, float gridStepinWU, std::string& quadrants)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		std::vector<unsigned char> _quadrants(quadrants.begin(), quadrants.end());
		size_t l1 = quadrants.size();
		size_t l2 = _quadrants.size();
		//inputParams.push_back(quadrants);
		inputParams.push_back(_quadrants);
		inputParams.push_back(inGameEditMode);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_SYNC " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerUploadBuffer(bool isInEditor, float lowerXGridVertex, float lowerZGridVertex, int numVerticesPerSize, float gridStepinWU, int level, std::string buffer)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(lowerXGridVertex);
		inputParams.push_back(lowerZGridVertex);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(level);
		std::vector<unsigned char> _buffer(buffer.begin(), buffer.end());
		inputParams.push_back(_buffer);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER, ref, inputParams, THEWORLD_CLIENTSERVER_MAPVERTICES_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_UPLOADCACHEBUFFER " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerDeployLevel(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepinWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(level);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraZ);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYLEVEL " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerDeployWorld(bool isInEditor, int level, size_t numVerticesPerSize, float gridStepinWU, bool setCamera, float cameraX, float cameraY, float cameraZ, float cameraDistanceFromTerrainForced, float cameraYaw, float cameraPitch, float cameraRoll)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(level);
		inputParams.push_back(numVerticesPerSize);
		inputParams.push_back(gridStepinWU);
		inputParams.push_back(setCamera);
		inputParams.push_back(cameraX);
		inputParams.push_back(cameraY);
		inputParams.push_back(cameraZ);
		inputParams.push_back(cameraDistanceFromTerrainForced);
		inputParams.push_back(cameraYaw);
		inputParams.push_back(cameraPitch);
		inputParams.push_back(cameraRoll);
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_DEPLOYWORLD " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	void GDN_TheWorld_Globals_Client::MapManagerUndeployWorld(bool isInEditor)
	{
		std::vector<ClientServerVariant> replyParams;
		std::vector<ClientServerVariant> inputParams;
		inputParams.push_back(isInEditor);
		std::string ref;
		int rc = execMethodAsync(THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD, ref, inputParams, THEWORLD_CLIENTSERVER_DEFAULT_TIME_TO_LIVE, m_globals->Viewer());
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
		{
			std::string m = std::string("ClientInterface::execMethodSync ==> " THEWORLD_CLIENTSERVER_METHOD_MAPM_UNDEPLOYWORLD " error ") + std::to_string(rc);
			throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", rc));
		}
	}

	GDN_TheWorld_Globals::GDN_TheWorld_Globals()
	{
		m_startTime = godot::Time::get_singleton()->get_ticks_msec();
		
		m_quitting = false;
		m_initialized = false;
		m_ready = false;
		setStatus(TheWorldStatus::uninitialized);
		m_isDebugEnabled = DEFAULT_DEBUG_ENABLED;
		m_bAppInError = false;
		m_lastErrorCode = 0;

		bool b = resize(THEWORLD_VIEWER_CHUNK_SIZE_SHIFT, THEWORLD_VIEWER_HEIGHTMAP_RESOLUTION_SHIFT, true);
		assert(b);

		//m_mapManager = NULL;
		m_client = nullptr;
		m_gridStepInWU = 0;

		m_viewer = nullptr;
		m_isInEditor = false;

		m_totElementToInitialize = m_numElementInitialized = 0;
		
		_init();
	}

	GDN_TheWorld_Globals::~GDN_TheWorld_Globals()
	{
		deinit();
	}

	void GDN_TheWorld_Globals::_bind_methods()
	{
		ClassDB::bind_method(D_METHOD("get_cache_dir"), &GDN_TheWorld_Globals::getCacheDir);
		ClassDB::bind_method(D_METHOD("get_config_filename"), &GDN_TheWorld_Globals::getConfigFilename);

		ClassDB::bind_method(D_METHOD("get_status"), &GDN_TheWorld_Globals::get_status);
		ClassDB::bind_method(D_METHOD("set_status"), &GDN_TheWorld_Globals::set_status);

		ClassDB::bind_method(D_METHOD("connect_to_server"), &GDN_TheWorld_Globals::connectToServer);
		ClassDB::bind_method(D_METHOD("prepare_disconnect_from_server"), &GDN_TheWorld_Globals::prepareDisconnectFromServer);
		ClassDB::bind_method(D_METHOD("disconnect_from_server"), &GDN_TheWorld_Globals::disconnectFromServer);

		ClassDB::bind_method(D_METHOD("set_debug_enabled"), &GDN_TheWorld_Globals::setDebugEnabled);
		ClassDB::bind_method(D_METHOD("is_debug_enabled"), &GDN_TheWorld_Globals::isDebugEnabled);
		ClassDB::bind_method(D_METHOD("debug_print"), &GDN_TheWorld_Globals::debugPrint);
		ClassDB::bind_method(D_METHOD("error_print"), &GDN_TheWorld_Globals::errorPrint);
		ClassDB::bind_method(D_METHOD("warning_print"), &GDN_TheWorld_Globals::warningPrint);
		ClassDB::bind_method(D_METHOD("info_print"), &GDN_TheWorld_Globals::infoPrint);
		ClassDB::bind_method(D_METHOD("print"), &GDN_TheWorld_Globals::print);

		ClassDB::bind_method(D_METHOD("set_app_in_error"), &GDN_TheWorld_Globals::setAppInError);
		ClassDB::bind_method(D_METHOD("get_app_in_error"), &GDN_TheWorld_Globals::getAppInError);
		ClassDB::bind_method(D_METHOD("get_app_in_error_code"), &GDN_TheWorld_Globals::getAppInErrorCode);
		ClassDB::bind_method(D_METHOD("get_app_in_error_message"), &GDN_TheWorld_Globals::getAppInErrorMsg);

		ClassDB::bind_method(D_METHOD("get_num_vertices_per_chunk_side"), &GDN_TheWorld_Globals::numVerticesPerChuckSide);
		ClassDB::bind_method(D_METHOD("get_bitmap_resolution"), &GDN_TheWorld_Globals::heightmapResolution);
		ClassDB::bind_method(D_METHOD("get_lod_max_depth"), &GDN_TheWorld_Globals::lodMaxDepth);
		ClassDB::bind_method(D_METHOD("get_num_lods"), &GDN_TheWorld_Globals::numLods);
		ClassDB::bind_method(D_METHOD("get_chunks_per_bitmap_side"), &GDN_TheWorld_Globals::numChunksPerHeightmapSide);
		ClassDB::bind_method(D_METHOD("get_grid_step_in_wu"), &GDN_TheWorld_Globals::gridStepInHeightmapWUs);

		ClassDB::bind_method(D_METHOD("get_tot_element_to_initialize"), &GDN_TheWorld_Globals::getTotElementToInitialize);
		ClassDB::bind_method(D_METHOD("add_element_to_initialize"), &GDN_TheWorld_Globals::addElementToInitialize);
		ClassDB::bind_method(D_METHOD("get_num_element_initialized"), &GDN_TheWorld_Globals::getNumElementInitialized);
		ClassDB::bind_method(D_METHOD("add_num_element_initialized"), &GDN_TheWorld_Globals::addNumElementInitialized);

		ClassDB::bind_method(D_METHOD("viewer"), &GDN_TheWorld_Globals::Viewer);

		ClassDB::bind_method(D_METHOD("quadrant_size_in_wus"), &GDN_TheWorld_Globals::heightmapSizeInWUs);
		ClassDB::bind_method(D_METHOD("quadrant_step_in_wus"), &GDN_TheWorld_Globals::gridStepInWU);

		ClassDB::bind_method(D_METHOD("set_texture_blending_size"), &GDN_TheWorld_Globals::setTextureBlendingSize);
		ClassDB::bind_method(D_METHOD("set_splatmap_blending_size"), &GDN_TheWorld_Globals::setSplatmapBlendingSize);

		// perfparam_
		ClassDB::bind_method(D_METHOD("set_perfparam_stream_world"), &GDN_TheWorld_Globals::setStreamWorld);
		ClassDB::bind_method(D_METHOD("get_perfparam_stream_world"), &GDN_TheWorld_Globals::getStreamWorld);
		ClassDB::bind_method(D_METHOD("set_perfparam_profiler_active"), &GDN_TheWorld_Globals::setParamProfilerActive);
		ClassDB::bind_method(D_METHOD("get_perfparam_profiler_active"), &GDN_TheWorld_Globals::getParamProfilerActive);
		//ClassDB::bind_method(D_METHOD("set_perfparam_cache_terrain_images", "p_value"), &GDN_TheWorld_Globals::setParamCacheTerrainImages);
		//ClassDB::bind_method(D_METHOD("get_perfparam_cache_terrain_images"), &GDN_TheWorld_Globals::getParamCacheTerrainImages);
		//ClassDB::bind_method(D_METHOD("set_perfparam_cache_terrain_textures_async", "p_value"), &GDN_TheWorld_Globals::setParamCacheTerrainTexturesAsync);
		//ClassDB::bind_method(D_METHOD("get_perfparam_cache_terrain_textures_async"), &GDN_TheWorld_Globals::getParamCacheTerrainTexturesAsync);
		//ClassDB::bind_method(D_METHOD("set_perfparam_cache_heights_invisible_quadrants", "p_value"), &GDN_TheWorld_Globals::setParamCacheHeightsForInvisibleQuadrants);
		//ClassDB::bind_method(D_METHOD("get_perfparam_cache_heights_invisible_quadrants"), &GDN_TheWorld_Globals::getParamCacheHeightsForInvisibleQuadrants);
		//ClassDB::bind_method(D_METHOD("set_perfparam_cache_heights_lowres_quadrants", "p_value"), &GDN_TheWorld_Globals::setParamCacheHeightsForLowResQuadrants);
		//ClassDB::bind_method(D_METHOD("get_perfparam_cache_heights_lowres_quadrants"), &GDN_TheWorld_Globals::getParamCacheHeightsForLowResQuadrants);
		ClassDB::bind_method(D_METHOD("set_perfparam_threshold_cache_heights", "p_value"), &GDN_TheWorld_Globals::setParamThresholdCacheHeights);
		ClassDB::bind_method(D_METHOD("get_perfparam_threshold_cache_heights"), &GDN_TheWorld_Globals::getParamThresholdCacheHeights);
		ClassDB::bind_method(D_METHOD("set_perfparam_force_hires_collision_info", "p_value"), &GDN_TheWorld_Globals::setParamForceHiResCollisionInfo);
		ClassDB::bind_method(D_METHOD("get_perfparam_force_hires_collision_info"), &GDN_TheWorld_Globals::getParamForceHiResCollisionInfo);
		//ClassDB::bind_method(D_METHOD("set_perfparam_depth_quad_with_collision_info"), &GDN_TheWorld_Globals::setParamDepthQuadWithCollisionInfo);
		//ClassDB::bind_method(D_METHOD("get_perfparam_depth_quad_with_collision_info"), &GDN_TheWorld_Globals::getParamDepthQuadWithCollisionInfo);
		ClassDB::bind_method(D_METHOD("set_perfparam_disable_terrain_collisions", "p_value"), &GDN_TheWorld_Globals::setParamDisableTerrainCollisions);
		ClassDB::bind_method(D_METHOD("get_perfparam_disable_terrain_collisions"), &GDN_TheWorld_Globals::getParamDisableTerrainCollisions);
		ClassDB::bind_method(D_METHOD("get_perfparam_refresh_quadrants_threshold"), &GDN_TheWorld_Globals::getRefreshQuadrantTreeThreshold);
		ClassDB::bind_method(D_METHOD("set_perfparam_refresh_quadrants_factor", "p_value"), &GDN_TheWorld_Globals::setParamRefreshQuadrantsFactor);
		ClassDB::bind_method(D_METHOD("get_perfparam_refresh_quadrants_factor"), &GDN_TheWorld_Globals::getParamRefreshQuadrantsFactor);
		ClassDB::bind_method(D_METHOD("get_perfparam_refresh_chunk_lod_threshold"), &GDN_TheWorld_Globals::getRefreshChunkLodThreshold);
		ClassDB::bind_method(D_METHOD("set_perfparam_refresh_chunks_factor", "p_value"), &GDN_TheWorld_Globals::setParamRefreshChunksFactor);
		ClassDB::bind_method(D_METHOD("get_perfparam_refresh_chunks_factor"), &GDN_TheWorld_Globals::getParamRefreshChunksFactor);
		ClassDB::bind_method(D_METHOD("set_perfparam_threshold_for_globalmap", "p_value"), &GDN_TheWorld_Globals::setParamThresholdForGlobalmap);
		ClassDB::bind_method(D_METHOD("get_perfparam_threshold_for_globalmap"), &GDN_TheWorld_Globals::getParamThresholdForGlobalmap);
		ClassDB::bind_method(D_METHOD("set_perfparam_threshold_for_map_texture_optimization", "p_value"), &GDN_TheWorld_Globals::setParamThresholdForMapTextureOptimization);
		ClassDB::bind_method(D_METHOD("get_perfparam_threshold_for_map_texture_optimization"), &GDN_TheWorld_Globals::getParamThresholdForMapTextureOptimization);
		ClassDB::bind_method(D_METHOD("set_perfparam_texture_optimization_increment_factor", "p_value"), &GDN_TheWorld_Globals::setParamTextureOptimizationIncrementFactor);
		ClassDB::bind_method(D_METHOD("get_perfparam_texture_optimization_increment_factor"), &GDN_TheWorld_Globals::getParamTextureOptimizationIncrementFactor);
		ClassDB::bind_method(D_METHOD("set_perfparam_texture_optimization_resolution_increment", "p_value"), &GDN_TheWorld_Globals::setParamTextureOptimizationResolutionIncrement);
		ClassDB::bind_method(D_METHOD("get_perfparam_texture_optimization_resolution_increment"), &GDN_TheWorld_Globals::getParamTextureOptimizationResolutionIncrement);
		ClassDB::bind_method(D_METHOD("set_perfparam_origin_shift_factor", "p_value"), &GDN_TheWorld_Globals::setParamOriginShiftFactor);
		ClassDB::bind_method(D_METHOD("get_perfparam_origin_shift_factor"), &GDN_TheWorld_Globals::getParamOriginShiftFactor);
		ClassDB::bind_method(D_METHOD("set_viewparam_show_sea", "p_value"), &GDN_TheWorld_Globals::setParamShowSea);
		ClassDB::bind_method(D_METHOD("get_viewparam_show_sea"), &GDN_TheWorld_Globals::getParamShowSea);
		ClassDB::bind_method(D_METHOD("set_viewparam_show_terrain", "p_value"), &GDN_TheWorld_Globals::setParamShowTerrain);
		ClassDB::bind_method(D_METHOD("get_viewparam_show_terrain"), &GDN_TheWorld_Globals::getParamShowTerrain);
		ClassDB::bind_method(D_METHOD("set_viewparam_show_chunks", "p_value"), &GDN_TheWorld_Globals::setParamShowChunks);
		ClassDB::bind_method(D_METHOD("get_viewparam_show_chunks"), &GDN_TheWorld_Globals::getParamShowChunks);
		ClassDB::bind_method(D_METHOD("set_viewparam_elevation_zero", "p_value"), &GDN_TheWorld_Globals::setParamElevationZero);
		ClassDB::bind_method(D_METHOD("get_viewparam_elevation_zero"), &GDN_TheWorld_Globals::getParamElevationZero);

		ClassDB::add_property_group("GDN_TheWorld_Globals", "Performance", "perfparam_");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_stream_world"), "set_perfparam_stream_world", "get_perfparam_stream_world");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_profiler_active"), "set_perfparam_profiler_active", "get_perfparam_profiler_active");
		//ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_cache_terrain_images"), "set_perfparam_cache_terrain_images", "get_perfparam_cache_terrain_images");
		//ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_cache_terrain_textures_async"), "set_perfparam_cache_terrain_textures_async", "get_perfparam_cache_terrain_textures_async");
		//ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_cache_heights_invisible_quadrants"), "set_perfparam_cache_heights_invisible_quadrants", "get_perfparam_cache_heights_invisible_quadrants");
		//ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_cache_heights_lowres_quadrants"), "set_perfparam_cache_heights_lowres_quadrants", "get_perfparam_cache_heights_lowres_quadrants");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_threshold_cache_heights"), "set_perfparam_threshold_cache_heights", "get_perfparam_threshold_cache_heights");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_force_hires_collision_info"), "set_perfparam_force_hires_collision_info", "get_perfparam_force_hires_collision_info");
		// WARNING: 0 mean that all quadrants have collider data set consuming a lot of memory in the engine, but setting this value permit less memory usage but more time to read data from disk to set data
		//ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::INT, "perfparam_depth_quad_with_collision"), "set_perfparam_depth_quad_with_collision_info", "get_perfparam_depth_quad_with_collision_info");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "perfparam_disable_terrain_collisions"), "set_perfparam_disable_terrain_collisions", "get_perfparam_disable_terrain_collisions");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_refresh_quadrants_factor", PROPERTY_HINT_RANGE, "0.0, 10.0, 0.5, or_greater", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), "set_perfparam_refresh_quadrants_factor", "get_perfparam_refresh_quadrants_factor");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_refresh_quadrants_threshold", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_perfparam_refresh_quadrants_threshold");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_refresh_chunks_factor", PROPERTY_HINT_RANGE, "0.0, 10.0, 0.1, or_greater", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_UPDATE_ALL_IF_MODIFIED), "set_perfparam_refresh_chunks_factor", "get_perfparam_refresh_chunks_factor");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_refresh_chunk_lod_threshold", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY), "", "get_perfparam_refresh_chunk_lod_threshold");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_threshold_globalmap"), "set_perfparam_threshold_for_globalmap", "get_perfparam_threshold_for_globalmap");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_threshold_tex_opt"), "set_perfparam_threshold_for_map_texture_optimization", "get_perfparam_threshold_for_map_texture_optimization");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::FLOAT, "perfparam_tex_opt_increment_factor"), "set_perfparam_texture_optimization_increment_factor", "get_perfparam_texture_optimization_increment_factor");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::INT, "perfparam_tex_opt_resolution_increment"), "set_perfparam_texture_optimization_resolution_increment", "get_perfparam_texture_optimization_resolution_increment");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::INT, "perfparam_origin_shift_factor"), "set_perfparam_origin_shift_factor", "get_perfparam_origin_shift_factor");

		ClassDB::add_property_group("GDN_TheWorld_Globals", "View", "viewparam_");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "viewparam_show_terrain"), "set_viewparam_show_terrain", "get_viewparam_show_terrain");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "viewparam_show_sea"), "set_viewparam_show_sea", "get_viewparam_show_sea");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "viewparam_show_chunks"), "set_viewparam_show_chunks", "get_viewparam_show_chunks");
		ClassDB::add_property("GDN_TheWorld_Globals", PropertyInfo(Variant::BOOL, "viewparam_elevation_zero"), "set_viewparam_elevation_zero", "get_viewparam_elevation_zero");

		ADD_SIGNAL(MethodInfo("tw_status_changed", PropertyInfo(Variant::INT, "old_value"), PropertyInfo(Variant::INT, "new_value")));
	}

	void GDN_TheWorld_Globals::setStatus(enum class TheWorldStatus status)
	{
		bool statusChanged = false;
		if (m_status != status)
			statusChanged = true;

		if (m_statusChangeClock.counterStarted() && statusChanged)
		{
			m_statusChangeClock.tock();
			infoPrint((std::string("Status changed: ") + statusStr(m_status) + std::string("(") + std::to_string((int)m_status) + std::string(") ==> ") + statusStr(status) + std::string("(") + std::to_string((int)status) + std::string(") (") + std::to_string(m_statusChangeClock.duration().count()) + std::string(" ms)")).c_str());
		}

		if (statusChanged)
		{
			//emit_signal("tw_status_changed", (int)m_status, (int)status);
			call_deferred("emit_signal", "tw_status_changed", (int)m_status, (int)status);
			m_status = status;
			m_statusChangeClock.tick();

			if (m_status == TheWorldStatus::worldDeployed)
			{
				m_viewer->finishedResizingGrid(false);
			}
		}
	}

	void GDN_TheWorld_Globals::replyFromServer(TheWorld_ClientServer::ClientServerExecution& reply)
	{
		std::string method = reply.getMethod();

		if (reply.error())
		{
			setStatus(TheWorldStatus::error);
			errorPrint((std::string("GDN_TheWorld_Globals::replyFromServer: error method ") + method + std::string(" rc=") + std::to_string(reply.getErrorCode()) + std::string(" ") + reply.getErrorMessage()).c_str());
		}

		try
		{
			if (method == THEWORLD_CLIENTSERVER_METHOD_SERVER_INITIALIZATION)
			{
				if (reply.getNumReplyParams() != 2)
				{
					std::string m = std::string("Server::initializeSession: error reply params ");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str(), "", 0));
				}

				ClientServerVariant v = reply.getReplyParam(0);
				const auto ptr(std::get_if<float>(&v));
				if (ptr == NULL)
				{
					std::string m = std::string("Server::initializeSession did not return a float as first param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				m_gridStepInWU = *ptr;

				v = reply.getReplyParam(1);
				const auto ptr1(std::get_if<std::string>(&v));
				if (ptr == NULL)
				{
					std::string m = std::string("Server::initializeSession did not return a string as second param");
					throw(GDN_TheWorld_Exception(__FUNCTION__, m.c_str()));
				}
				m_mapName = *ptr1;

				Viewer()->initRequired(true);

				//Viewer()->init();
				
				//setStatus(TheWorldStatus::sessionInitialized);
			}
			else if (method == THEWORLD_CLIENTSERVER_METHOD_MAPM_SETLOGMAXSEVERITY)
			{
				method = "to remove";
			}
			else
			{
				setStatus(TheWorldStatus::error);
				errorPrint((std::string("GDN_TheWorld_Globals::replyFromServer: unknow method ") + method).c_str());
			}
		}
		catch (GDN_TheWorld_Exception ex)
		{
			setStatus(TheWorldStatus::error);
			errorPrint(String(ex.exceptionName()) + String(" caught - ") + ex.what());
		}
		catch (...)
		{
			setStatus(TheWorldStatus::error);
			errorPrint("GDN_TheWorld_Globals::replyFromServer: exception caught");
		}
	}

	bool GDN_TheWorld_Globals::resize(int chunkSizeShift, int heightmapResolutionShift, bool force)
	{
		if (chunkSizeShift == -1 || heightmapResolutionShift == -1)
			return false;

		if (!force && m_chunkSizeShift == chunkSizeShift && m_heightmapResolutionShift == heightmapResolutionShift)
			return false;

		m_chunkSizeShift = chunkSizeShift;
		m_heightmapResolutionShift = heightmapResolutionShift;
		m_numVerticesPerChuckSide = 1 << m_chunkSizeShift;
		m_heightmapResolution = 1 << m_heightmapResolutionShift;
		m_lodMaxDepth = m_heightmapResolutionShift - m_chunkSizeShift;
		m_numLods = m_lodMaxDepth + 1;
		return true;
	}

	void GDN_TheWorld_Globals::init(bool isInEditor)
	{
		m_isInEditor = isInEditor;

		string logPath;
		if (IS_EDITOR_HINT())
			logPath = getModuleLoadPath() + "\\TheWorld_Viewer_EDITOR_log.txt";
		else
			logPath = getModuleLoadPath() + "\\TheWorld_Viewer_log.txt";
		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;
		if (!g_plogInitialized)
		{
			plog::init(sev, logPath.c_str(), 1000000, 3);
			TheWorld_Utils::Utils::plogInit(sev, plog::get());
			g_plogInitialized = true;
		}
		PLOG(plog::get()->getMaxSeverity()) << "***************";
		PLOG(plog::get()->getMaxSeverity()) << "Log initialized!";
		PLOG(plog::get()->getMaxSeverity()) << "***************";

		PLOG_INFO << "TheWorld Globals Initializing...";

		//setDebugEnabled(m_isDebugEnabled);

		m_initialized = true;
		setStatus(TheWorldStatus::initialized);
		PLOG_INFO << "TheWorld Globals Initialized!";
	}

	void GDN_TheWorld_Globals::preDeinit(void)
	{
	}

	bool GDN_TheWorld_Globals::canDeinit(void)
	{
		return true;
	}

	void GDN_TheWorld_Globals::deinit(void)
	{
		if (m_initialized)
		{
			PLOG_INFO << "TheWorld Globals Deinitializing...";

			PLOG_INFO << "TheWorld Globals Deinitialized!";

			PLOG(plog::get()->getMaxSeverity()) << "*****************";
			PLOG(plog::get()->getMaxSeverity()) << "Log Terminated!";
			PLOG(plog::get()->getMaxSeverity()) << "*****************";

			TheWorld_Utils::Utils::plogDenit();

			m_initialized = false;
			setStatus(TheWorldStatus::uninitialized);

			debugPrint("GDN_TheWorld_Globals::deinit DONE!");
		}
	}

	float GDN_TheWorld_Globals::gridStepInWU(void) const
	{
		return m_gridStepInWU;
	}

	std::string GDN_TheWorld_Globals::getMapName(void)
	{
		return m_mapName;
	}

	void GDN_TheWorld_Globals::_init(void)
	{
		//Cannot find Globals pointer as current node is not yet in the scene
		//godot::UtilityFunctions::print("GDN_TheWorld_Globals::Init");

		set_name(THEWORLD_GLOBALS_NODE_NAME);
	}

	bool GDN_TheWorld_Globals::connectedToServer(void)
	{
		if (m_client != nullptr && m_client->connected())
			return true;
		else
			return false;
	}
	
	void GDN_TheWorld_Globals::connectToServer(void)
	{
		if (status() != TheWorldStatus::initialized)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Connect to server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

		if (m_client != nullptr)
			throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Connect to server failed: pointer to client is not null").c_str())));

		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (m_isDebugEnabled)
			sev = PLOG_DEBUG_LEVEL;

		m_client = new GDN_TheWorld_Globals_Client(this, sev);
		int rc = m_client->connect();
		if (rc != THEWORLD_CLIENTSERVER_RC_OK)
			throw(GDN_TheWorld_Exception(__FUNCTION__, "Connect to server failed with code " + rc));

		setStatus(TheWorldStatus::connectedToServer);

		infoPrint("GDN_TheWorld_Globals::connectToServer DONE!");

		m_client->ServerInitializeSession(IS_EDITOR_HINT(), sev);
	}

	void GDN_TheWorld_Globals::prepareDisconnectFromServer(void)
	{
		if ((int)status() >= (int)TheWorldStatus::connectedToServer)
		{
			if (m_client != nullptr)
				m_client->prepareDisconnect();
		}
	}

	bool GDN_TheWorld_Globals::canDisconnectFromServer(void)
	{
		bool can;
		
		if (m_client == nullptr)
			can = true;
		else
			can = m_client->canDisconnect();

		if (can)
			return true;
		else
			return false;
	}

	void GDN_TheWorld_Globals::disconnectFromServer(void)
	{
		if ((int)status() < (int)TheWorldStatus::connectedToServer)
			return;
			//throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: not valid status, status=") + std::to_string((size_t)status())).c_str()));

		if (m_client == nullptr)
			return;
			//throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Disconnect from server failed: pointer to client is null").c_str())));

		m_client->disconnect();
		delete m_client;
		m_client = nullptr;

		infoPrint("GDN_TheWorld_Globals::disconnectFromServer DONE!");

		setStatus(TheWorldStatus::initialized);
	}

	void GDN_TheWorld_Globals::_ready(void)
	{
		debugPrint("GDN_TheWorld_Globals::_ready");

		//get_node(NodePath("/root/Main/Reset"))->connect("pressed", this, "on_Reset_pressed");
		m_ready = true;
	}

	void GDN_TheWorld_Globals::_input(const Ref<InputEvent>& event)
	{
	}

	void GDN_TheWorld_Globals::_notification(int p_what)
	{
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
				if (m_quitting)
					debugPrint("GDN_TheWorld_Globals::_notification (NOTIFICATION_PREDELETE) - Destroy Globals", false);
				else
					debugPrint("GDN_TheWorld_Globals::_notification (NOTIFICATION_PREDELETE) - Destroy Globals");
			}
		}
		break;
		}
	}

	void GDN_TheWorld_Globals::shaderParamGlobalTimeApplied(double d)
	{
		m_shaderGlobalTimeApplied = d;
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_TIME, m_shaderGlobalTimeApplied);
	}

	void GDN_TheWorld_Globals::_process(double _delta)
	{
		// To activate _process method add this Node to a Godot Scene
		//debugPrint("GDN_TheWorld_Globals::_process");

		// Global parameters depending on Shader Global Time
		{
			shaderParamGlobalTimeApplied(internalTimeInSeconds(_delta));

			GDN_TheWorld_Viewer* viewer = Viewer();
			if (viewer != nullptr and viewer->initialized())
			{
				godot::Camera3D* activeCamera = viewer->getCamera();
				if (activeCamera != nullptr)
				{
					GDN_TheWorld_Camera* cam = Object::cast_to<GDN_TheWorld_Camera>(activeCamera);
					if (cam != nullptr)
						cam->updateCameraUnderWater();
				}
			}
		}
	}

	GDN_TheWorld_Viewer* GDN_TheWorld_Globals::Viewer(bool useCache)
	{
		if (m_viewer == NULL || !useCache)
		{
			SceneTree* scene = get_tree();
			if (!scene)
				return NULL;
			Viewport* root = scene->get_root();
			if (!root)
				return NULL;
			m_viewer = Object::cast_to<GDN_TheWorld_Viewer>(root->find_child(THEWORLD_VIEWER_NODE_NAME, true, false));
		}

		return m_viewer;
	}

	void GDN_TheWorld_Globals::setStreamWorld(bool b)
	{
		if (m_streamWorld != b)
		{
			m_streamWorld = b;
			if (m_streamWorld && (int)Globals()->status() >= (int)TheWorldStatus::worldDeployInProgress)
				Viewer()->recalcQuadrantsInViewNeeded(true);
		}
	}

	void GDN_TheWorld_Globals::setParamProfilerActive(bool b)
	{
		if (b)
			TheWorld_Utils::Profiler::activate();
		else
			TheWorld_Utils::Profiler::deactivate();
	}

	//void GDN_TheWorld_Globals::setParamCacheTerrainImages(bool b)
	//{
	//	m_paramCacheTerrainImages = b;
	//}

	//void GDN_TheWorld_Globals::setParamCacheTerrainTexturesAsync(bool b)
	//{
	//	m_paramCacheTerrainTexturesAsync = b;
	//}

	//void GDN_TheWorld_Globals::setParamCacheHeightsForInvisibleQuadrants(bool b)
	//{
	//	m_paramCacheHeightsForInvisibleQuadrants = b;
	//	GDN_TheWorld_Viewer* viewer = Viewer();
	//	if (viewer != nullptr)
	//		viewer->resetMaterialParamsForEveryQuadrant();
	//}

	//void GDN_TheWorld_Globals::setParamCacheHeightsForLowResQuadrants(bool b)
	//{
	//	m_paramCacheHeightsForLowResQuadrants = b;
	//	GDN_TheWorld_Viewer* viewer = Viewer();
	//	if (viewer != nullptr)
	//		viewer->resetMaterialParamsForEveryQuadrant();
	//}

	void GDN_TheWorld_Globals::setParamThresholdCacheHeights(float f)
	{
		m_paramThresholdCacheHeights = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamForceHiResCollisionInfo(bool b)
	{
		m_paramForceHiResCollisionInfo = b;
		//GDN_TheWorld_Viewer* viewer = Viewer();
		//if (viewer != nullptr)
		//	viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamDisableTerrainCollisions(bool b)
	{
		m_paramDisableTerrainCollisions = b;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamRefreshQuadrantsFactor(float f)
	{
		if (f <= 0)
			f = 1.0f;
		m_paramRefreshQuadrantsFactor = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamRefreshChunksFactor(float f)
	{
		if (f <= 0)
			f = 1.0f;
		m_paramRefreshChunksFactor = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamThresholdForGlobalmap(float f)
	{
		m_paramThresholdForGlobalmap = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamThresholdForMapTextureOptimization(float f)
	{
		m_paramThresholdForMapTextureOptimization = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamTextureOptimizationIncrementFactor(float f)
	{
		m_paramTextureOptimizationIncrementFactor = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamTextureOptimizationResolutionIncrement(int64_t f)
	{
		m_paramTextureOptimizationResolutionIncrement = f;
		GDN_TheWorld_Viewer* viewer = Viewer();
		if (viewer != nullptr)
			viewer->resetMaterialParamsForEveryQuadrant();
	}

	void GDN_TheWorld_Globals::setParamOriginShiftFactor(int64_t f)
	{
		m_paramOriginShiftFactor = f;
	}

	void GDN_TheWorld_Globals::setParamShowSea(bool b)
	{
		if (m_paramShowSea != b)
		{
			m_paramShowSea = b;
			//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_SEA, m_paramShowSea);
			m_paramShowSeaChanged = true;
		}

	}
	
	void GDN_TheWorld_Globals::setParamShowSeaChanged(bool b)
	{
		m_paramShowSeaChanged = b;
	}

	void GDN_TheWorld_Globals::setParamShowTerrain(bool b)
	{
		if (m_paramShowTerrain != b)
		{
			m_paramShowTerrain = b;
			//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_TERRAIN, m_paramShowSea);
			m_paramShowTerrainChanged = true;
		}

	}

	void GDN_TheWorld_Globals::setParamShowTerrainChanged(bool b)
	{
		m_paramShowTerrainChanged = b;
	}

	void GDN_TheWorld_Globals::setParamShowChunks(bool b)
	{
		m_paramShowChunks = b;
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_SHOW_CHUNKS, m_paramShowChunks);
	}

	void GDN_TheWorld_Globals::setParamElevationZero(bool b)
	{
		m_paramElevationZero = b;
		int paramElevationZero = b ? 1 : 0;
		godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_FORCE_ELEVATION_0, paramElevationZero);
		Viewer()->terrainHeightsChangedEvent();
	}

	//void GDN_TheWorld_Globals::setParamDepthQuadWithCollisionInfo(int64_t f)
	//{
	//	m_paramDepthQuadWithCollisionInfo = f;
	//	GDN_TheWorld_Viewer* viewer = Viewer();
	//	if (viewer != nullptr)
	//		viewer->resetMaterialParamsForEveryQuadrant();
	//}

	//void GDN_TheWorld_Globals::setParamSetColliderForInvisibleQuadrants(bool b)
	//{
	//	m_paramSetColliderForInvisibleQuadrants = b;
	//	GDN_TheWorld_Viewer* viewer = Viewer();
	//	if (viewer != nullptr)
	//		viewer->resetMaterialParamsForEveryQuadrant();
	//}

	void GDN_TheWorld_Globals::setDebugEnabled(bool b)
	{
		if (m_client == nullptr)
			return;
		
		m_isDebugEnabled = b;

		plog::Severity sev = PLOG_DEFAULT_LEVEL;
		if (b)
			sev = PLOG_DEBUG_LEVEL;

		plog::get()->setMaxSeverity(sev);
		PLOG(plog::get()->getMaxSeverity()) << "Log severity changed to: " << std::to_string(sev);
		TheWorld_Utils::Utils::plogSetMaxSeverity(sev);
		m_client->MapManagerSetLogMaxSeverity(sev);
	}

	size_t GDN_TheWorld_Globals::resolutionForOptimizationLevel(size_t optimizationLevel)
	{
		size_t resolution = (size_t)heightmapResolution() + 1;
		if (optimizationLevel > 0)
		{
			size_t newResolutionShift = heightmapResolutionShift() - optimizationLevel;
			resolution = ((size_t)1 << newResolutionShift) + 1;
		}
		return resolution;
	}

	size_t GDN_TheWorld_Globals::getMaxTextureOptimizationLevel(void)
	{
		return heightmapResolutionShift() - MIN_RESOLUTION_SHIFT_FOR_TEXTURE_OPTIMIZATIONS;
	}
}

std::string getModuleLoadPath(void)
{
	char path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)getModuleLoadPath, &hm) == 0)
	{
		return "";
	}
	if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
	{
		return "";
	}

	int l = (int)strlen(path);
	for (int i = l; i >= 0; i--)
		if (path[i] == '\\')
		{
			path[i] = '\0';
			break;
		}

	std::string s = path;

	return s;
}
