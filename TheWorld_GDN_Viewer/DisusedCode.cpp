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


//
//float gapToFlatten = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//float sumHeightStep = 0, sumModifiedHeightStep = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//	//float h1 = h;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) = heightStep;
//	if (x == 1024)
//	{
//		sumHeightStep += heightStep;
//		sumModifiedHeightStep += modifiedHeightStep;
//		//PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	}
//	TheWorld_Utils::FLOAT_32 f(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half;
//	idx++;
//}
//if (x == 1024)
//{
//	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten << ",sumHeightStep = " << sumHeightStep << ", sumModifiedHeightStep" << sumModifiedHeightStep;
//}
//
//gapToFlatten = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//idx = 0;
//sumHeightStep = 0, sumModifiedHeightStep = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//	//float h1 = h;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= heightStep;
//	if (x == 1024)
//	{
//		sumHeightStep += heightStep;
//		sumModifiedHeightStep += modifiedHeightStep;
//		//PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	}
//	TheWorld_Utils::FLOAT_32 f(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half;
//	idx++;
//}
//if (x == 1024)
//{
//	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten << ",sumHeightStep = " << sumHeightStep << ", sumModifiedHeightStep" << sumModifiedHeightStep;
//}

//#define	MIN_PERCENT					0.2f
//#define	MAX_PERCENT					0.5f
//#define	MAX_STEP_MODIFIER_FACTOR	100.0f	// crescendo incrementa la curvatura
//
//std::hash<std::string> hasher;
//size_t seed = hasher(data.meshId + northData.meshId);
//srand((unsigned int)seed);
//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
//float random_f = (((MAX_PERCENT - MIN_PERCENT) / 100) * random_i) + MIN_PERCENT;
//numVerticesToAdjustInEachQuadrant = size_t(numVerticesToAdjustInEachQuadrant * random_f);
//float radiantStepPerVertex = TheWorld_Utils::kPi / (numVerticesToAdjustInEachQuadrant);
//
//float gapToFlatten = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant - 1; z >= 0; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h1 = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//	//float h = h1;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) -= modifiedHeightStep;
//	//data.heights32Buffer->at<float>(x, z, numVerticesPerSize) = h1;
//	//if (x == 1024)
//	//{
//	//	PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	//}
//	TheWorld_Utils::FLOAT_32 f(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half;
//	idx++;
//
//	if (z == 1)
//	{
//		float h1 = data.heights32Buffer->at<float>(x, z, numVerticesPerSize);
//		float h2 = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//		saveHeight = (h1 + h2) / 2;
//	}
//}
////if (x == 1024)
////{
////	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten;
////}
//
//gapToFlatten = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - northData.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//heightStep = gapToFlatten / numVerticesToAdjustInEachQuadrant;
//heightStepFactor = heightStep * MAX_STEP_MODIFIER_FACTOR;
//idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant - 1; z >= 0; z--)
//{
//	float modifiedHeightStep = heightStep + sin(float(idx) * radiantStepPerVertex) * heightStepFactor;
//	//float h1 = northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize);
//	//float h = h1;
//	//h -= heightStep;
//	//h1 -= modifiedHeightStep;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= modifiedHeightStep;
//	//northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) = h1;
//	//if (x == 1024)
//	//{
//	//	PLOG_DEBUG << "z = " << z << ", heightStep = " << heightStep << ", modifiedHeightStep = " << modifiedHeightStep << ", h = " << h << ", h1 = " << h1;
//	//}
//	TheWorld_Utils::FLOAT_32 f(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half = half_from_float(f.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half;
//	idx++;
//}
////if (x == 1024)
////{
////	PLOG_DEBUG << "gapToFlatten = " << gapToFlatten;
////}

//std::hash<std::string> hasher;
//size_t seed = hasher(data.meshId + northData.meshId);
//srand((unsigned int)(seed));
//int random_i = (rand() % 100) + 1;			// random integer between 1 and 100
//float random_f = (((MAX_PERCENT - MIN_PERCENT) / 100) * random_i) + MIN_PERCENT;
//numVerticesToAdjustInEachQuadrant = size_t(ceilf(numVerticesToAdjustInEachQuadrant * random_f));
//float radiantStepPerVertex = TheWorld_Utils::kPi / (numVerticesToAdjustInEachQuadrant - 1);
//
//float gapToFlatten1 = data.heights32Buffer->at<float>(x, numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - data.heights32Buffer->at<float>(x, 1, numVerticesPerSize);
//float heightStep1 = gapToFlatten1 / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor1 = heightStep1 * MAX_STEP_MODIFIER_FACTOR;
//float gapToFlatten2 = northData.heights32Buffer->at<float>(x, maxIndex - numVerticesToAdjustInEachQuadrant, numVerticesPerSize) - northData.heights32Buffer->at<float>(x, maxIndex - 1, numVerticesPerSize);
//float heightStep2 = gapToFlatten2 / numVerticesToAdjustInEachQuadrant;
//float heightStepFactor2 = heightStep2 * MAX_STEP_MODIFIER_FACTOR;
//size_t idx = 0;
//for (int z = (int)numVerticesToAdjustInEachQuadrant; z >= 1; z--)
//{
//	float modifiedHeightStep1 = heightStep1 + sin(float(idx) * radiantStepPerVertex) * heightStepFactor1;
//	data.heights32Buffer->at<float>(x, z, numVerticesPerSize) -= modifiedHeightStep1;
//	TheWorld_Utils::FLOAT_32 f1(data.heights32Buffer->at<float>(x, z, numVerticesPerSize));
//	uint16_t half1 = half_from_float(f1.u32);
//	data.heights16Buffer->at<uint16_t>(x, z, numVerticesPerSize) = half1;
//
//	float modifiedHeightStep2 = heightStep2 + sin(float(idx) * radiantStepPerVertex) * heightStepFactor2;
//	northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize) -= modifiedHeightStep2;
//	TheWorld_Utils::FLOAT_32 f2(northData.heights32Buffer->at<float>(x, maxIndex - z, numVerticesPerSize));
//	uint16_t half2 = half_from_float(f2.u32);
//	northData.heights16Buffer->at<uint16_t>(x, maxIndex - z, numVerticesPerSize) = half2;
//
//	idx++;
//}
