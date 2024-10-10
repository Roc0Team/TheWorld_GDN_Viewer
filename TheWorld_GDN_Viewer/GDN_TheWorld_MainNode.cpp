//#include "pch.h"
#include "GDN_TheWorld_MainNode.h"
#include "GDN_TheWorld_Globals.h"
#include "GDN_TheWorld_Viewer.h"
#include "GDN_Template.h"
//#include "Utils.h"

#pragma warning (push, 0)
//#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/viewport.hpp>
#include <godot_cpp/classes/window.hpp>
#pragma warning (pop)

using namespace godot;

void GDN_TheWorld_MainNode::_bind_methods()
{
	ClassDB::bind_method(D_METHOD("init", "p_world_main_node"), &GDN_TheWorld_MainNode::init);
	ClassDB::bind_method(D_METHOD("pre_deinit"), &GDN_TheWorld_MainNode::preDeinit);
	ClassDB::bind_method(D_METHOD("can_deinit"), &GDN_TheWorld_MainNode::canDeinit);
	ClassDB::bind_method(D_METHOD("deinit"), &GDN_TheWorld_MainNode::deinit);
	ClassDB::bind_method(D_METHOD("globals"), &GDN_TheWorld_MainNode::Globals);
	ClassDB::bind_method(D_METHOD("initialized"), &GDN_TheWorld_MainNode::initialized);
}

GDN_TheWorld_MainNode::GDN_TheWorld_MainNode()
{
	m_quitting = false;
	m_initialized = false;
	m_ready = false;
	m_initInProgress = false;
	m_globals = nullptr;
	m_viewer = nullptr;

	_init();
}

GDN_TheWorld_MainNode::~GDN_TheWorld_MainNode()
{
	deinit();
}

void GDN_TheWorld_MainNode::_init(void)
{
	//Cannot find Globals pointer as current node is not yet in the scene
	//godot::UtilityFunctions::print("GDN_TheWorld_MainNode::_init");

	set_name(THEWORLD_MAIN_NODE_NAME);
}

void GDN_TheWorld_MainNode::_ready(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals != nullptr)
		globals->debugPrint("GDN_TheWorld_MainNode::_ready");

	m_ready = true;

	//// Test UTC Date/Time

	////std::chrono::system_clock::time_point now1_tp_utc_1 = std::chrono::system_clock::now();		// UTC Time
	////std::string now1_str_utc_1 = std::format("{}", now1_tp_utc_1);
	////std::time_t now1_time_t_utc_1 = std::chrono::system_clock::to_time_t(now1_tp_utc_1);
	////std::chrono::system_clock::time_point now1_tp_utc_2 = std::chrono::system_clock::from_time_t(now1_time_t_utc_1);
	////std::string now1_str_utc_2 = std::format("{}", now1_tp_utc_2);
	////std::time_t now1_time_t_utc_2 = 0;
	////std::vector<std::string> v = TheWorld_Utils::Utils::split(now1_str_utc_2, '.');
	////std::string s = v[0];
	//////s += "Z";
	////std::tm t = {};
	////std::istringstream ss(v[0]);
	////ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
	////if (not ss.fail())
	////{
	////	//now1_time_t_utc_2 = std::mktime(&t);
	////	now1_time_t_utc_2 = _mkgmtime(&t);
	////}

	//std::time_t now_time_t_utc_1 = TheWorld_Utils::current_utc_time();
	//std::string now_str_utc_1 = TheWorld_Utils::time_t_to_str_time(now_time_t_utc_1);
	//std::time_t now_time_t_utc_2 = TheWorld_Utils::str_time_to_time_t(now_str_utc_1);
	//assert(now_time_t_utc_1 == now_time_t_utc_2);
	//
	//FILE* outFile = nullptr;
	//std::string tempPath = "c:/temp/prova.txt";
	//errno_t err = fopen_s(&outFile, tempPath.c_str(), "w");
	//std::string shortBuffer = "AAAAAAAAAAA";
	//size_t num = fwrite(shortBuffer.c_str(), shortBuffer.size(), 1, outFile);
	//fclose(outFile);
	//
	////const auto fileTime_utc = std::filesystem::last_write_time(tempPath);					// UTC Time
	////std::string fileTime_str1_utc = std::format("{}", fileTime_utc);
	////const auto systemTime_utc = std::chrono::clock_cast<std::chrono::system_clock>(fileTime_utc);
	////std::time_t fileTime_time_t_utc = std::chrono::system_clock::to_time_t(systemTime_utc);
	////std::chrono::system_clock::time_point fileTime_tp_utc = std::chrono::system_clock::from_time_t(fileTime_time_t_utc);
	////std::string fileTime_str2_utc = std::format("{}", fileTime_tp_utc);
	//
	//std::time_t fileTime_time_t_utc_1 = TheWorld_Utils::last_write_time_utc(tempPath);
	//std::string fileTime_str_utc_1 = TheWorld_Utils::last_write_time_utc_str(tempPath);
	//std::time_t fileTime_time_t_utc_2 = TheWorld_Utils::str_time_to_time_t(fileTime_str_utc_1);
	//assert(fileTime_time_t_utc_1 == fileTime_time_t_utc_2);

	//std::string meshId = TheWorld_Utils::MeshCacheBuffer::generateNewMeshId();
	//std:string meshIdWithoutBinaryTime;
	//std::chrono::system_clock::time_point time_tp = TheWorld_Utils::MeshCacheBuffer::getTimeFromMeshId(meshId, meshIdWithoutBinaryTime);
	//std::string meshIdEmptyBuffer = TheWorld_Utils::MeshCacheBuffer::generateNewMeshIdForEmptyBuffer();
	//bool b = TheWorld_Utils::MeshCacheBuffer::isEmptyBuffer(meshIdEmptyBuffer);

	//// Test UTC Date/Time
}

void GDN_TheWorld_MainNode::_notification(int p_what)
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
			//m_quitting = false;	// to reproduce access violation on exit
			GDN_TheWorld_Globals* globals = Globals();
			if (globals != nullptr)
				if (m_quitting)
					globals->debugPrint("GDN_TheWorld_MainNode::_notification (NOTIFICATION_PREDELETE) - Destroy Main Node", false);
				else
					globals->debugPrint("GDN_TheWorld_MainNode::_notification (NOTIFICATION_PREDELETE) - Destroy Main Node");
		}
	}
	break;
	}
}

void GDN_TheWorld_MainNode::_input(const Ref<InputEvent>& event)
{
	//Globals()->debugPrint("GDN_TheWorld_MainNode::_ready");
}

void GDN_TheWorld_MainNode::_process(double _delta)
{
	// To activate _process method add this Node to a Godot Scene
	//Globals()->debugPrint("GDN_TheWorld_MainNode::_process");

	//if (!m_initialized && m_initInProgress)
	//{
	//	godot::Spatial* pWorldMainNode = (Spatial*)get_parent();
	//	
	//	GDN_TheWorld_Globals* globals = GDN_TheWorld_Globals::_new();
	//	if (globals)
	//	{
	//		//Node* parent = globals->get_parent();
	//		pWorldMainNode->add_child(globals);
	//		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);
	//		globals->init();
	//		//m_globals = globals;
	//	}

	//	PLOG_INFO << "TheWorld Main Node Initializing...";

	//	GDN_TheWorld_Viewer* viewer = GDN_TheWorld_Viewer::_new();
	//	if (viewer)
	//	{
	//		pWorldMainNode->add_child(viewer);
	//		viewer->set_name(THEWORLD_WORLD_VIEWER_NODE_NAME);
	//		Transform gt = pWorldMainNode->get_global_transform();
	//		viewer->set_global_transform(gt);
	//		//viewer->init();
	//	}

	//	m_initialized = true;
	//	m_initInProgress = false;

	//	PLOG_INFO << "TheWorld Main Node Initialized!";
	//}
}

bool GDN_TheWorld_MainNode::init(Node3D* pWorldMainNode, bool isInEditor)
{
	assert(!m_initialized);
	
	// Must exist a Spatial Node acting as the world; globals and the viewer will be a child of it
	if (!pWorldMainNode)
		return false;

	int64_t worldMainNodeId = pWorldMainNode->get_instance_id();

	godot::Node* parent = get_parent();
	if (parent != nullptr)
	{
		int64_t parentId = parent->get_instance_id();
		if (parentId != worldMainNodeId)
		{
			parent->remove_child(this);
			pWorldMainNode->add_child(this);
		}
	}
	else
		pWorldMainNode->add_child(this);
	
	set_name(THEWORLD_MAIN_NODE_NAME);

	SceneTree* scene = get_tree();
	Node* sceneRoot = nullptr;
	//if (isInEditor)		// it should be fixed in https://github.com/godotengine/godot/pull/77633 commit 31/05
	if (IS_EDITOR_HINT())
		sceneRoot = scene->get_edited_scene_root();
	else
		sceneRoot = scene->get_root();

	set_owner(sceneRoot);

	m_initInProgress = true;

	GDN_TheWorld_Globals* globals = Globals(false);
	if (globals == nullptr)
		globals = memnew(GDN_TheWorld_Globals);
	if (globals)
	{
		globals->set_name(THEWORLD_GLOBALS_NODE_NAME);

		Node* parent = globals->get_parent();
		if (parent != nullptr)
		{
			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			{
				parent->remove_child(globals);
				pWorldMainNode->add_child(globals);
			}
		}
		else
			pWorldMainNode->add_child(globals);
		globals->set_owner(sceneRoot);

		globals->init(isInEditor);
	}
	else
		return false;

	PLOG_INFO << "TheWorld Main Node Initializing...";

	GDN_TheWorld_Viewer* viewer = globals->Viewer(false);
	if (viewer == nullptr)
		viewer = memnew(GDN_TheWorld_Viewer);
	if (viewer)
	{
		viewer->set_name(THEWORLD_WORLD_VIEWER_NODE_NAME);
		Transform3D gt = pWorldMainNode->get_global_transform();
		viewer->set_global_transform(gt);

		Node* parent = viewer->get_parent();
		if (parent != nullptr)
		{
			int64_t parentId = parent->get_instance_id();
			if (parentId != worldMainNodeId)
			{
				parent->remove_child(viewer);
				pWorldMainNode->add_child(viewer);
			}
		}
		else
			pWorldMainNode->add_child(viewer);
		viewer->set_owner(sceneRoot);
	}
	else
		return false;

	//GDN_TheWorld_WorldViewportContainer* worldViewportContainer = viewer->WorldViewportContainer(false);
	//if (worldViewportContainer == nullptr)
	//	worldViewportContainer = memnew(GDN_TheWorld_WorldViewportContainer);
	//if (worldViewportContainer)
	//{
	//	worldViewportContainer->set_name(THEWORLD_WORLD_VIEWPORT_CONTAINER_NODE_NAME);
	//	worldViewportContainer->setViewer(viewer);

	//	Node* parent = worldViewportContainer->get_parent();
	//	if (parent != nullptr)
	//	{
	//		int64_t viewerId = viewer->get_instance_id();
	//		int64_t parentId = parent->get_instance_id();
	//		if (parentId != viewerId)
	//		{
	//			parent->remove_child(worldViewportContainer);
	//			viewer->add_child(worldViewportContainer);
	//		}
	//	}
	//	else
	//		viewer->add_child(worldViewportContainer);
	//	worldViewportContainer->set_owner(sceneRoot);
	//}
	//else
	//	return false;

	//GDN_TheWorld_WorldViewport* worldViewport = viewer->WorldViewport(false);
	//if (worldViewport == nullptr)
	//	worldViewport = memnew(GDN_TheWorld_WorldViewport);
	//if (worldViewport)
	//{
	//	worldViewport->set_name(THEWORLD_WORLD_VIEWPORT_NODE_NAME);
	//	worldViewport->setWorldViewportContainer(worldViewportContainer);
	//	worldViewportContainer->setWorldViewport(worldViewport);

	//	Node* parent = worldViewport->get_parent();
	//	if (parent != nullptr)
	//	{
	//		int64_t worldViewportContainerId = worldViewportContainer->get_instance_id();
	//		int64_t parentId = parent->get_instance_id();
	//		if (parentId != worldViewportContainerId)
	//		{
	//			parent->remove_child(worldViewport);
	//			worldViewportContainer->add_child(worldViewport);
	//		}
	//	}
	//	else
	//		worldViewportContainer->add_child(worldViewport);
	//	worldViewport->set_owner(sceneRoot);
	//}
	//else
	//	return false;

	viewer = globals->Viewer(false);
	viewer = Viewer(false);
	
	m_initInProgress = false;
	m_initialized = true;
	PLOG_INFO << "TheWorld Main Node Initialized!";

	return true;
}

void GDN_TheWorld_MainNode::preDeinit(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals)
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
		if (viewer)
			viewer->preDeinit();
		
		globals->prepareDisconnectFromServer();

		globals->preDeinit();
	}
}

bool GDN_TheWorld_MainNode::canDeinit(void)
{
	GDN_TheWorld_Globals* globals = Globals();
	if (globals)
	{
		GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
		if (viewer && !viewer->canDeinit())
			return false;

		if (!globals->canDisconnectFromServer())
			return false;

		return globals->canDeinit();
	}
	else
		return true;
}

void GDN_TheWorld_MainNode::deinit(void)
{
	if (m_initialized)
	{
		PLOG_INFO << "TheWorld Main Node Deinitializing...";

		GDN_TheWorld_Globals* globals = Globals();
		if (globals)
		{
			GDN_TheWorld_Viewer* viewer = Globals()->Viewer();
			if (viewer)
			{
				viewer->deinit();
				Node* parent = viewer->get_parent();
				if (parent)
				{
					//parent->call_deferred("remove_child", viewer);	// TESTING
					//parent->remove_child(viewer);
				}
				//viewer->set_owner(nullptr);							// TESTING
				//viewer->queue_free();									// TESTING
			}
			PLOG_INFO << "TheWorld Main Node Deinitialized!";

			globals->deinit();
			//Node* parent = globals->get_parent();
			//if (parent)
			//{
			//	parent->call_deferred("remove_child", globals);		// TESTING
			//	parent->remove_child(globals);
			//}
			//globals->set_owner(nullptr);								// TESTING
			//globals->queue_free();									// TESTING
		}

		m_initialized = false;

		Globals()->debugPrint("GDN_TheWorld_MainNode::deinit DONE!");

		//Node* parent = get_parent();
		//if (parent)
		//{
		//	parent->call_deferred("remove_child", this);				// TESTING
		//	parent->remove_child(this);
		//}
		//set_owner(nullptr);											// TESTING
	}
}

GDN_TheWorld_Globals* GDN_TheWorld_MainNode::Globals(bool useCache)
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

GDN_TheWorld_Viewer* GDN_TheWorld_MainNode::Viewer(bool useCache)
{
	if (m_viewer == NULL || !useCache)
	{
		if (not is_inside_tree())
			return NULL;
		SceneTree* scene = get_tree();
		if (!scene)
			return NULL;
		Viewport* root = scene->get_root();
		if (!root)
			return NULL;
		m_viewer = Object::cast_to<GDN_TheWorld_Viewer>(root->find_child(THEWORLD_WORLD_VIEWER_NODE_NAME, true, false));
	}

	return m_viewer;
}
