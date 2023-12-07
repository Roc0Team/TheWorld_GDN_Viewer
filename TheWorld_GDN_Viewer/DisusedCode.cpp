//float gridStepInWU = m_viewer->Globals()->gridStepInWU();
//float numVerticesPerChuckSide = (float)m_viewer->Globals()->numVerticesPerChuckSide();
//item.second->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
//item.second->set_shader_parameter(SHADER_PARAM_VERTICES_PER_CHUNK, numVerticesPerChuckSide);
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_INVERSE_TRANSFORM) + String(" t=") + String(t));
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_NORMAL_BASIS) + String(" b=") + String(b));
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_GRID_STEP) + String(" grid_step=") + String(std::to_string(f).c_str()));
//currentMaterial->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
//m_viewer->Globals()->debugPrint("setting shader_param=" + String(SHADER_PARAM_EDITMODE_SELECTED) + String(" quad_selected=") + String(std::to_string(sel).c_str()));

							// X-axis growing
							//end = mouseHit + Vector3Right * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_red);

							// Y-axis growing
							//end = mouseHit + Vector3Up * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_green);

							// Z-axis growing
							//end = mouseHit + Vector3Back * lineSize;
							//m_normalDrawer->drawLine(mouseHit, end);
							//m_normalDrawer->drawSphere(end, radius, GDN_TheWorld_Globals::g_color_blue);

				//TheWorld_Utils::MeshCacheBuffer::CacheQuadrantData quadrantData;
				//quadrantData.terrainEdit = quadTree->getQuadrant()->getTerrainEdit();
				//quadrantData.meshId = quadTree->getQuadrant()->getMeshCacheBuffer().getMeshId();
				//quadrantData.minHeight = quadTree->getQuadrant()->getTerrainEdit()->minHeight;
				//quadrantData.maxHeight = quadTree->getQuadrant()->getTerrainEdit()->maxHeight;
				//TheWorld_Utils::MemoryBuffer& h16Buffer = quadTree->getQuadrant()->getFloat16HeightsBuffer();
				//quadrantData.heights16Buffer = &h16Buffer;
				//TheWorld_Utils::MemoryBuffer& h32Buffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
				//quadrantData.heights32Buffer = &h32Buffer;
				//TheWorld_Utils::MemoryBuffer& normals = quadTree->getQuadrant()->getNormalsBuffer();
				//quadrantData.normalsBuffer = &normals;
				//TheWorld_Utils::MemoryBuffer& splatmap = quadTree->getQuadrant()->getSplatmapBuffer();
				//quadrantData.splatmapBuffer = &splatmap;

			//srand((unsigned int)time(NULL));			// random seed
			//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
			//float random_f = (float)random_i / 100;		// random float from 0.01 and 1.0 (step 0.01)
			//random_f += 0.5f;							// random float from 0.51 and 1.5 (step 0.01)
			//numVerticesToAdjustInEachQuadrant = (size_t)ceilf(heightGapForEachQuandrantOnBorder * random_f);

