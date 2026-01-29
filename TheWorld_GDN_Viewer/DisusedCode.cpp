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

//void ShaderTerrainData::loadGroundTextures(godot::String dirName, GDN_TheWorld_Viewer* viewer)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("getGroundTextures 1 ") + __FUNCTION__, "all");
//
//	TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint("Reading ground textures");
//
//	godot::String _dirName = dirName;
//	if (_dirName.length() == 0)
//	{
//		_dirName = ShaderTerrainData::getGroundTexturesDir().c_str();
//	}
//
//	godot::String globalizedDirName = ProjectSettings::get_singleton()->globalize_path(_dirName);
//
//	std::map<std::string, bool> textures;
//
//	std::string scanningDir = std::string(globalizedDirName.utf8().get_data());
//	if (!fs::exists(scanningDir))
//		return;
//
//	for (const auto& entry : fs::directory_iterator(scanningDir))
//		if (entry.is_directory())
//		{
//			std::string dirname = entry.path().filename().generic_string();
//			if (not dirname.starts_with("_"))
//				textures[dirname] = true;
//		}
//
//	godot::Ref<godot::Texture2DArray> albedoTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> normalTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> roughnessTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> displacementTexture2DArray = memnew(godot::Texture2DArray);
//	godot::Ref<godot::Texture2DArray> ambientOcclusionTexture2DArray = memnew(godot::Texture2DArray);
//	godot::TypedArray<godot::Image> albedoImageArray, normalImageArray, roughnessImageArray, displacementImageArray/*, ambientOcclusionImageArray*/;
//
//	std::map<std::string, int32_t>& groundTextureLayers = ShaderTerrainData::getGroundTextureLayers();
//	groundTextureLayers.clear();
//
//	const int desiredImageSize = 1024;
//	const int texturePreviewDesiredImageSize = 512;
//	const godot::Image::Format desiredImageFormat = godot::Image::Format::FORMAT_RGB8;
//
//	int layer = 0;
//	for (const auto& texture : textures)
//	{
//		std::string textureDir = scanningDir + texture.first;
//		//std::string albedoDisplTextureFilePath, normalRoughnessTextureFilePath;
//		std::string albedoTextureFilePath, normalTextureFilePath, roughnessTextureFilePath, displacementTextureFilePath, ambientOcclusionFilePath, texturePreviewFilePath;
//
//		for (const auto& entry : fs::directory_iterator(textureDir))
//		{
//			if (entry.is_regular_file())
//			{
//				std::string filename = entry.path().filename().generic_string();
//
//				if (GDN_TheWorld_Globals::isImageExt(filename))
//				{
//					if (GDN_TheWorld_Globals::isAlbedoTexture(filename))
//					{
//						albedoTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isNormalTexture(filename))
//					{
//						normalTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isRoughnessTexture(filename))
//					{
//						roughnessTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isDisplacementTexture(filename))
//					{
//						displacementTextureFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isAmbientOcclusionTexture(filename))
//					{
//						ambientOcclusionFilePath = textureDir + "/" + filename;
//					}
//					else if (GDN_TheWorld_Globals::isTexturePreview(filename))
//					{
//						texturePreviewFilePath = textureDir + "/" + filename;
//					}
//				}
//			}
//		}
//
//		if (albedoTextureFilePath.size() > 0)
//		{
//			TheWorld_Viewer_Utils::MsTimePoint start = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//
//			s_groundTextures[texture.first] = std::make_unique<ShaderTerrainData::GroundTexture>();
//			ShaderTerrainData::GroundTexture* groundTexture = s_groundTextures[texture.first].get();
//
//			groundTexture->m_albedo_tex = loadGroundTexture(albedoTextureFilePath, "ALBEDO", texture.first, desiredImageSize, desiredImageFormat, false, viewer);
//			my_assert(groundTexture->m_albedo_tex != nullptr);
//			albedoImageArray.push_back(groundTexture->m_albedo_tex->get_image());
//
//			groundTexture->m_normal_tex = loadGroundTexture(normalTextureFilePath, "NORMAL", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_normal_tex != nullptr);
//			normalImageArray.push_back(groundTexture->m_normal_tex->get_image());
//
//			groundTexture->m_roughness_tex = loadGroundTexture(roughnessTextureFilePath, "ROUGHNESS", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_roughness_tex != nullptr);
//			roughnessImageArray.push_back(groundTexture->m_roughness_tex->get_image());
//
//			groundTexture->m_displacement_tex = loadGroundTexture(displacementTextureFilePath, "DISPLACEMENT", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			my_assert(groundTexture->m_displacement_tex != nullptr);
//			displacementImageArray.push_back(groundTexture->m_displacement_tex->get_image());
//
//			//groundTexture->m_ambient_occlusion_tex = loadGroundTexture(ambientOcclusionFilePath, "AMBIENT OCCLUSION", texture.first, desiredImageSize, desiredImageFormat, true, viewer);
//			//my_assert(groundTexture->m_ambient_occlusion_tex != nullptr);
//			//ambientOcclusionImageArray.push_back(groundTexture->m_ambient_occlusion_tex->get_image());
//
//			if (texturePreviewFilePath.size() > 0)
//			{
//				groundTexture->m_texture_preview = loadGroundTexture(texturePreviewFilePath, "TEXTURE PREVIEW", texture.first, texturePreviewDesiredImageSize, desiredImageFormat, false, viewer);
//				my_assert(groundTexture->m_texture_preview != nullptr);
//			}
//			else
//			{
//				godot::Ref<godot::Image> image = godot::Image::create_from_data(texturePreviewDesiredImageSize, texturePreviewDesiredImageSize, false, groundTexture->m_albedo_tex->get_image()->get_format(), groundTexture->m_albedo_tex->get_image()->get_data());
//				/*if (ambientOcclusionFilePath.size() > 0)
//				{
//					if (not groundTexture->m_ambient_occlusion_tex.is_valid())
//					{
//						groundTexture->m_ambient_occlusion_tex = loadGroundTexture(ambientOcclusionFilePath, "AMBIENT OCCLUSION", texture.first, texturePreviewDesiredImageSize, desiredImageFormat, true, viewer);
//						my_assert(groundTexture->m_ambient_occlusion_tex != nullptr);
//					}
//
//					godot::Ref<godot::Image> aoImage = groundTexture->m_ambient_occlusion_tex->get_image();
//					godot::Rect2i sourceBlendingRect(0, 0, image->get_width(), image->get_height());
//					godot::Vector2i destBlendingRect(0, 0);
//
//					//aoImage->blend_rect(image, sourceBlendingRect, destBlendingRect);		// Alpha-blending is not what I want
//					//image = aoImage;
//					for(int y = 0; y < image->get_height(); y++)
//						for (int x = 0; x < image->get_width(); x++)
//						{
//							godot::Color color = image->get_pixel(x, y);
//							godot::Color aoImageColor = aoImage->get_pixel(x, y);
//							godot::Color mixedColor = color * aoImageColor;
//							image->set_pixel(x, y, mixedColor);
//						}
//				}*/
//
//				texturePreviewFilePath = textureDir + "/" + texture.first + "_texture_preview" + ".png";
//				//if (not godot::Engine::get_singleton()->is_editor_hint())
//				image->save_png(texturePreviewFilePath.c_str());
//				groundTexture->m_texture_preview = godot::ImageTexture::create_from_image(image);
//			}
//
//			groundTextureLayers[texture.first] = layer;
//
//			TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//			long long elapsed = (now - start).count();
//			if (viewer != nullptr)
//				viewer->Globals()->debugPrint((std::string("Loaded ground_texture [") + texture.first + "] ==> layer = " + std::to_string(layer) + " - Elapsed = " + std::to_string(elapsed) + " ms").c_str());
//
//			layer++;
//		}
//		else
//		{
//			if (viewer != nullptr)
//			{
//				std::string msg = std::string("Ground texture ") + texture.first + " discarded";
//				viewer->Globals()->debugPrint(msg.c_str());
//			}
//
//		}
//	}
//
//	my_assert(albedoImageArray.size() == normalImageArray.size());
//	my_assert(albedoImageArray.size() == roughnessImageArray.size());
//	my_assert(albedoImageArray.size() == displacementImageArray.size());
//	//my_assert(albedoImageArray.size() == ambientOcclusionImageArray.size());
//
//	godot::Error e = albedoTexture2DArray->create_from_images(albedoImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_ALBEDO_ARRAY, albedoTexture2DArray);
//	e = normalTexture2DArray->create_from_images(normalImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_NORMAL_ARRAY, normalTexture2DArray);
//	e = roughnessTexture2DArray->create_from_images(roughnessImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_ROUGHNESS_ARRAY, roughnessTexture2DArray);
//	e = displacementTexture2DArray->create_from_images(displacementImageArray);
//	my_assert(e == godot::Error::OK);
//	godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_DISPL_ARRAY, displacementTexture2DArray);
//	//e = ambientOcclusionTexture2DArray->create_from_images(ambientOcclusionImageArray);
//	//my_assert(e == godot::Error::OK);
//	//godot::RenderingServer::get_singleton()->global_shader_parameter_set(GLOBAL_SHADER_PARAM_GROUND_AO_ARRAY, ambientOcclusionTexture2DArray);
//
//	// Free PBR Textures because they are already sent to the global shader variables
//	for (auto& groundTexture : s_groundTextures)
//	{
//		if (groundTexture.second->m_albedo_tex.is_valid())
//			groundTexture.second->m_albedo_tex.unref();
//		if (groundTexture.second->m_normal_tex.is_valid())
//			groundTexture.second->m_normal_tex.unref();
//		if (groundTexture.second->m_roughness_tex.is_valid())
//			groundTexture.second->m_roughness_tex.unref();
//		if (groundTexture.second->m_ambient_occlusion_tex.is_valid())
//			groundTexture.second->m_ambient_occlusion_tex.unref();
//		if (groundTexture.second->m_displacement_tex.is_valid())
//			groundTexture.second->m_displacement_tex.unref();
//	}
//
//	TheWorld_Viewer_Utils::MsTimePoint now = std::chrono::time_point_cast<TheWorld_Viewer_Utils::MsTimePoint::duration>(std::chrono::system_clock::now());
//	long long elapsed = (now - start).count();
//
//	if (viewer != nullptr)
//		viewer->Globals()->debugPrint((std::string("Reading ground textures done! - Elapsed ") + std::to_string(elapsed) + " ms").c_str());
//
//	//String materialPath = "res://addons/twviewer/shaders/m_sea.tres";
//	//s_seaMat = godot::ResourceLoader::get_singleton()->load(materialPath);
//}


//void GDN_TheWorld_Edit::editModeGenNormalsAction(void)
//{
//	if (m_actionInProgress)
//		return;
//
//	m_actionInProgress = true;
//	m_allItems = 0;
//	setElapsed(0, true);
//	setNote1(0);
//
//	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeGenNormals, this);
//	m_tp.QueueJob(f);
//}

//void GDN_TheWorld_Edit::editModeGenNormals(void)
//{
//	editModeGenNormals_1(true, false);
//}
//
//void GDN_TheWorld_Edit::editModeGenNormals_1(bool forceGenSelectedQuad, bool evaluateSelectedQuadOnly)
//{
//	// TO BE FIXED: IT IS IN CONFLICT WITH QUADRANT MODDER
//	
//	if (m_actionClock.counterStarted())
//	{
//		m_actionInProgress = false;
//		return;
//	}
//
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1 ") + __FUNCTION__, "ALL");
//
//	setMessage(godot::String("Generating normals..."));
//	
//	m_actionClock.tick();
//
//	QuadTree* quadTreeSel = nullptr;
//	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
//
//	bool allCheched = m_innerData->m_allCheckBox->is_pressed();
//
//	if (!allCheched && quadrantSelPos.empty())
//	{
//		m_actionClock.tock();
//		m_actionInProgress = false;
//		setMessage(godot::String("Completed!"), true);
//		return;
//	}
//
//	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
//	m_viewer->getAllQuadrantPos(allQuadrantPos);
//
//	std::vector<TheWorld_Utils::QuadrantPos> quadrantPosToRegen;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.1 ") + __FUNCTION__, "Select quads");
//
//		if (evaluateSelectedQuadOnly)
//		{
//			if (quadTreeSel != nullptr)
//			{
//				if (forceGenSelectedQuad)
//				{
//					quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
//					quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//					quadrantPosToRegen.push_back(quadrantSelPos);
//				}
//				else
//				{
//					TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTreeSel->getQuadrant()->getNormalsBuffer(true);
//					if (normalsBuffer.size() == 0 || quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//					{
//						quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//						quadrantPosToRegen.push_back(quadrantSelPos);
//					}
//				}
//			}
//
//		}
//		else
//		{
//			//if (quadTreeSel != nullptr && forceGenSelectedQuad)
//			//	quadTreeSel->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;
//
//			int num = 0;
//			for (auto& pos : allQuadrantPos)
//			{
//				num++;
//				QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//				if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
//				{
//					//quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = true;		// SUPERDEBUGRIC: force to generate normals
//					TheWorld_Utils::MemoryBuffer* pn = nullptr;
//					{
//						TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.1.1 ") + __FUNCTION__, "getNormalsBuffer quad");
//						TheWorld_Utils::MemoryBuffer& n = quadTree->getQuadrant()->getNormalsBuffer(true);
//						pn = &n;
//					}
//					TheWorld_Utils::MemoryBuffer& normalsBuffer = *pn;
//					if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//					{
//						if (allCheched || pos == quadrantSelPos)
//						{
//							quadTree->getQuadrant()->setNeedUploadToServer(true);
//							quadrantPosToRegen.push_back(pos);
//
//							if (!allCheched)
//								break;
//						}
//					}
//				}
//			}
//		}
//	}
//
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2 ") + __FUNCTION__, "Loop quads");
//
//		m_completedItems = 0;
//		m_elapsedCompleted = 0;
//		m_lastElapsed = 0;
//		m_allItems = quadrantPosToRegen.size();
//		for (auto& pos : quadrantPosToRegen)
//		{
//			if (m_actionStopRequested)
//			{
//				m_actionStopRequested = false;
//				break;
//			}
//
//			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//			if (quadTree != nullptr)
//			{
//				bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//				if (lockAcquired)
//				{
//					editModeGenNormals_2(quadTree);
//					quadTree->getQuadrant()->unlockInternalData();
//				}
//
//				m_completedItems++;
//				size_t partialCount = m_actionClock.partialDuration().count();
//				m_lastElapsed = partialCount - m_elapsedCompleted;
//				m_elapsedCompleted = partialCount;
//			}
//		}
//	}
//
//	m_actionClock.tock();
//
//	m_actionInProgress = false;
//
//	size_t duration = m_actionClock.duration().count();
//	setElapsed(duration, false);
//	setCounter(m_completedItems, m_allItems);
//	setNote1(m_lastElapsed);
//
//	//size_t numToSave = 0;
//	//size_t numToUpload = 0;
//	//size_t numLocked = 0;
//	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);
//
//	setMessage(godot::String("Completed!"), true);
//}

//void GDN_TheWorld_Edit::editModeGenNormals_2(QuadTree* quadTree)
//{
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1 ") + __FUNCTION__, "getNormalsBuffer single quad");
//
//	TheWorld_Utils::MemoryBuffer* pn = nullptr;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.1 ") + __FUNCTION__, "getNormalsBuffer quad");
//		TheWorld_Utils::MemoryBuffer& n = quadTree->getQuadrant()->getNormalsBuffer(true);
//		pn = &n;
//	}
//	TheWorld_Utils::MemoryBuffer& normalsBuffer = *pn;
//	if (normalsBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen)
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.2 ") + __FUNCTION__, "Single QuadTree");
//
//		TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
//		TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer(true);
//
//		{
//			TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.1.2.1 ") + __FUNCTION__, "Single QuadTree - GPU");
//			generateNormals(quadTree, &float32HeigthsBuffer, &normalsBuffer);
//		}
//
//		//{
//		//	TheWorld_Utils::GuardProfiler profiler(std::string("EditGenNormals 1.2.2.1 ") + __FUNCTION__, "Single QuadTree - CPU");
//		//	cache.generateNormalsForBlendedQuadrants(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), float32HeigthsBuffer, *eastHeights32Buffer, *southHeights32Buffer, normalsBuffer, m_requestedThreadExit);
//		//	
//		//	if (m_requestedThreadExit)
//		//		return;
//		//}
//
//		//std::vector<float> vectGridHeights;
//		//heightsBuffer.populateFloatVector(vectGridHeights);
//		//cache.deprecated_generateNormals(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), vectGridHeights, normalsBuffer);
//
//		{
//			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
//			//m_mapQuadToSave[pos] = "";
//			bool manual = true;
//			addQuadToSave(pos, manual);
//		}
//		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
//		quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//		quadTree->getQuadrant()->getTerrainEdit()->normalsNeedRegen = false;
//		quadTree->getQuadrant()->setNormalsUpdated(true);
//
//		quadTree->getQuadrant()->resetSplatmapBuffer(__FUNCTION__);
//
//		quadTree->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);
//
//		quadTree->materialParamsNeedReset(true);
//	}
//}

//void GDN_TheWorld_Edit::generateGlobalmap(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* globalmapBuffer)
//{
//	my_assert(m_mapModder != nullptr);
//
//	size_t numHeights = (size_t)sqrt(float(quadtree->getQuadrant()->getFloat32HeightsBuffer().size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == quadtree->getQuadrant()->getFloat32HeightsBuffer().size());
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//
//	m_mapModder->initForGlobalMap(quadtree);
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedGlobalmap = m_mapModder->getOutBuffer();
//
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(TheWorld_Utils::_RGB)) == cookedGlobalmap->size());
//	globalmapBuffer->reserve(cookedGlobalmap->size());
//	memcpy(globalmapBuffer->ptr(), cookedGlobalmap->ptr(), cookedGlobalmap->size());
//	globalmapBuffer->adjustSize(cookedGlobalmap->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::generateNormals(QuadTree* quadtree, TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer)
//{
//	// distance == 0 ==> all the quadrant (distance is ignored atm considering "all the quadrant" by design)
//	my_assert(m_mapModder != nullptr);
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//	
//	size_t numHeights = (size_t)sqrt(float(float32HeigthsBuffer->size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == float32HeigthsBuffer->size());
//
//	m_mapModder->initForNormals(quadtree, float32HeigthsBuffer, 1024, 1);		// atm border = 0 will be the same
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedNormals = m_mapModder->getOutBuffer();
//	
//	my_assert(normalsBuffer->size() == 0 || cookedNormals->size() == normalsBuffer->size());
//	size_t numNormals = (size_t)sqrt(float(cookedNormals->size() / sizeof(TheWorld_Utils::_RGB)));
//	my_assert(size_t(pow((float)numNormals, 2) * sizeof(TheWorld_Utils::_RGB)) == cookedNormals->size());
//	my_assert(numHeights == numNormals);
//	normalsBuffer->reserve(cookedNormals->size());
//	memcpy(normalsBuffer->ptr(), cookedNormals->ptr(), cookedNormals->size());
//	normalsBuffer->adjustSize(cookedNormals->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::generateSplatmapForBlendedQuadrants(QuadTree* quadtree, size_t numVerticesPerSize, float gridStepInWU,
//	float x, float z, float distance,
//	TheWorld_Utils::MemoryBuffer* float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* normalsBuffer, TheWorld_Utils::MemoryBuffer* splatmapBuffer, TheWorld_Utils::TerrainEdit* terrainEdit,
//	TheWorld_Utils::MemoryBuffer* west_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* west_normalsBuffer, TheWorld_Utils::MemoryBuffer* west_splatmapBuffer, TheWorld_Utils::TerrainEdit* west_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* east_normalsBuffer, TheWorld_Utils::MemoryBuffer* east_splatmapBuffer, TheWorld_Utils::TerrainEdit* east_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* north_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* north_normalsBuffer, TheWorld_Utils::MemoryBuffer* north_splatmapBuffer, TheWorld_Utils::TerrainEdit* north_terrainEdit,
//	TheWorld_Utils::MemoryBuffer* south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer* south_normalsBuffer, TheWorld_Utils::MemoryBuffer* south_splatmapBuffer, TheWorld_Utils::TerrainEdit* south_terrainEdit,
//	float slopeVerticalFactor,	// raising it slope will have higher maximum and so rocks will be more diffuse on slopes
//	float slopeFlatFactor,		// raising it slope will have lesser mininum and maximum so rocks will be less diffuse on slopes
//	float dirtOnRocksFactor,	// raising it dirt will be more diffuse on rocks
//	float highElevationFactor,	// raising it high elevation amount will be greater at lower altitude
//	float lowElevationFactor,	// raising it low elevation amount will be greater at higher altitude
//	size_t splatMapMode)
//{
//	// distance == 0 ==> all the quadrant (distance is ignored atm considering "all the quadrant" by design)
//	my_assert(m_mapModder != nullptr);
//
//	while (!m_mapModder->ready())
//		Sleep(5);
//
//	size_t numHeights = (size_t)sqrt(float(float32HeigthsBuffer->size() / sizeof(float)));
//	my_assert(size_t(pow((float)numHeights, 2) * sizeof(float)) == float32HeigthsBuffer->size());
//	size_t numNormals = (size_t)sqrt(float(normalsBuffer->size() / sizeof(TheWorld_Utils::_RGB)));
//	my_assert(size_t(pow((float)numNormals, 2) * sizeof(TheWorld_Utils::_RGB)) == normalsBuffer->size());
//	my_assert(numNormals == 0 || numHeights == numNormals);
//
//	m_mapModder->initForSplatmap(quadtree, float32HeigthsBuffer, normalsBuffer, slopeVerticalFactor, slopeFlatFactor,	dirtOnRocksFactor, highElevationFactor,	lowElevationFactor, terrainEdit->minHeight, terrainEdit->maxHeight, 1024);
//	while (m_mapModder->working())
//		Sleep(5);
//	TheWorld_Utils::MemoryBuffer* cookedSplatmap = m_mapModder->getOutBuffer();
//
//	my_assert(splatmapBuffer->size() == 0 || cookedSplatmap->size() == splatmapBuffer->size());
//	size_t numPixels = (size_t)sqrt(float(cookedSplatmap->size() / sizeof(TheWorld_Utils::_RGBA)));
//	my_assert(size_t(pow((float)numPixels, 2) * sizeof(TheWorld_Utils::_RGBA)) == cookedSplatmap->size());
//	my_assert(numHeights == numPixels);
//	splatmapBuffer->reserve(cookedSplatmap->size());
//	memcpy(splatmapBuffer->ptr(), cookedSplatmap->ptr(), cookedSplatmap->size());
//	splatmapBuffer->adjustSize(cookedSplatmap->size());
//	m_mapModder->deinitRequired(true);
//}

//void GDN_TheWorld_Edit::editModeApplyTexturesAction(void)
//{
//	if (m_actionInProgress)
//		return;
//
//	m_actionInProgress = true;
//	m_allItems = 0;
//	setElapsed(0, true);
//	setNote1(0);
//
//	std::function<void(void)> f = std::bind(&GDN_TheWorld_Edit::editModeApplyTextures, this);
//	m_tp.QueueJob(f);
//}

//void GDN_TheWorld_Edit::editModeApplyTextures(void)
//{
//	editModeApplyTextures_1(true, false);
//}
//
//void GDN_TheWorld_Edit::editModeApplyTextures_1(bool forceApplySelectedQuad, bool evaluateSelectedQuadOnly)
//{
//	// TO BE FIXED: IT IS IN CONFLICT WITH QUADRANT MODDER
//
//	if (m_actionClock.counterStarted())
//	{
//		m_actionInProgress = false;
//		return;
//	}
//
//	m_actionInProgress = true;
//
//	TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1 ") + __FUNCTION__, "ALL");
//
//	setMessage(godot::String("Applying textures..."));
//
//	m_actionClock.tick();
//
//	QuadTree* quadTreeSel = nullptr;
//	TheWorld_Utils::QuadrantPos quadrantSelPos = m_viewer->getWorldQuadrantSelForEdit(&quadTreeSel);
//
//	bool allCheched = m_innerData->m_allCheckBox->is_pressed();
//
//	if (!allCheched && quadrantSelPos.empty())
//	{
//		m_actionClock.tock();
//		m_actionInProgress = false;
//		setMessage(godot::String("Completed!"), true);
//		return;
//	}
//
//	std::vector<TheWorld_Utils::QuadrantPos> allQuadrantPos;
//	m_viewer->getAllQuadrantPos(allQuadrantPos);
//
//	std::vector<TheWorld_Utils::QuadrantPos> quadrantPos;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.1 ") + __FUNCTION__, "Select Quads");
//
//		if (evaluateSelectedQuadOnly)
//		{
//			if (quadTreeSel != nullptr)
//			{
//				if (forceApplySelectedQuad)
//				{
//					quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(true);
//					quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//					quadrantPos.push_back(quadrantSelPos);
//				}
//				else
//				{
//					TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTreeSel->getQuadrant()->getSplatmapBuffer(true);
//					if (splatmapBuffer.size() == 0 || quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//					{
//						quadTreeSel->getQuadrant()->setNeedUploadToServer(true);
//						quadrantPos.push_back(quadrantSelPos);
//					}
//				}
//			}
//
//		}
//		else
//		{
//			if (quadTreeSel != nullptr && forceApplySelectedQuad)
//				quadTreeSel->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(true);
//
//			{
//				for (auto& pos : allQuadrantPos)
//				{
//					QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//					if (quadTree != nullptr && !quadTree->getQuadrant()->empty())
//					{
//						//quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatmapNeedRegen = true;		// SUPERDEBUGRIC: force to generate textures
//						TheWorld_Utils::MemoryBuffer* sp = nullptr;
//						{
//							TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.1.1 ") + __FUNCTION__, "getSplatmapBuffer quad");
//							TheWorld_Utils::MemoryBuffer& s = quadTree->getQuadrant()->getSplatmapBuffer(true);
//							sp = &s;
//						}
//						TheWorld_Utils::MemoryBuffer& splatmapBuffer = *sp;
//						if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//						{
//							if (allCheched || pos == quadrantSelPos)
//							{
//								quadrantPos.push_back(pos);
//								quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//								if (!allCheched)
//									break;
//							}
//						}
//					}
//				}
//			}
//		}
//	}
//
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2 ") + __FUNCTION__, "Loop Quads");
//
//		m_completedItems = 0;
//		m_elapsedCompleted = 0;
//		m_lastElapsed = 0;
//		m_allItems = quadrantPos.size();
//		for (auto& pos : quadrantPos)
//		{
//			if (m_actionStopRequested)
//			{
//				m_actionStopRequested = false;
//				break;
//			}
//
//			QuadTree* quadTree = m_viewer->getQuadTree(pos, false);
//			if (quadTree != nullptr)
//			{
//				bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//				if (lockAcquired)
//				{
//					editModeApplyTextures_2(quadTree);
//					quadTree->getQuadrant()->unlockInternalData();
//				}
//
//				m_completedItems++;
//				size_t partialCount = m_actionClock.partialDuration().count();
//				m_lastElapsed = partialCount - m_elapsedCompleted;
//				m_elapsedCompleted = partialCount;
//			}
//		}
//	}
//
//	m_actionClock.tock();
//
//	m_actionInProgress = false;
//
//	size_t duration = m_actionClock.duration().count();
//	setElapsed(duration, false);
//	setCounter(m_completedItems, m_allItems);
//	setNote1(m_lastElapsed);
//
//	//size_t numToSave = 0;
//	//size_t numToUpload = 0;
//	//size_t numLocked = 0;
//	//refreshNumToSaveUpload(numToSave, numToUpload, numLocked);
//
//	setMessage(godot::String("Completed!"), true);
//
//}

//void GDN_TheWorld_Edit::editModeApplyTextures_2(QuadTree* quadTree)
//{
//	TheWorld_Utils::MemoryBuffer* sp = nullptr;
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.1 ") + __FUNCTION__, "getSplatmapBuffer quad");
//		TheWorld_Utils::MemoryBuffer& s = quadTree->getQuadrant()->getSplatmapBuffer(true);
//		sp = &s;
//	}
//	TheWorld_Utils::MemoryBuffer& splatmapBuffer = *sp;
//	if (splatmapBuffer.size() == 0 || quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen())
//	{
//		TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2 ") + __FUNCTION__, "Single QuadTree");
//
//		editModeGenNormals_2(quadTree);
//
//		TheWorld_Utils::QuadrantPos pos = quadTree->getQuadrant()->getPos();
//
//		TheWorld_Utils::MeshCacheBuffer& cache = quadTree->getQuadrant()->getInternalMeshCacheBuffer();
//
//		TheWorld_Utils::MemoryBuffer& normalsBuffer = quadTree->getQuadrant()->getNormalsBuffer(true);
//		TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer = quadTree->getQuadrant()->getFloat32HeightsBuffer();
//		TheWorld_Utils::MemoryBuffer& splatmapBuffer = quadTree->getQuadrant()->getSplatmapBuffer();
//		size_t numElements = float32HeigthsBuffer.size() / sizeof(float);
//		my_assert(numElements == pos.getNumVerticesPerSize() * pos.getNumVerticesPerSize());
//
//		//{
//		//	TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2.1 ") + __FUNCTION__, "Single QuadTree - CPU");
//		//	cache.generateSplatmap(pos.getNumVerticesPerSize(), pos.getGridStepInWU(), quadTree->getQuadrant()->getTerrainEdit(), float32HeigthsBuffer, normalsBuffer, splatmapBuffer,
//		//		slopeVerticalFactor(),
//		//		slopeFlatFactor(),
//		//		dirtOnRocksFactor(),
//		//		highElevationFactor(),
//		//		lowElevationFactor(),
//		//		splatMapMode());
//		//}
//
//		{
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeVerticalFactor = slopeVerticalFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeFlatFactor = slopeFlatFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtOnRocksFactor = dirtOnRocksFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationFactor = highElevationFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationFactor = lowElevationFactor();
//			quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatMapMode = splatMapMode();
//
//			TheWorld_Utils::GuardProfiler profiler(std::string("EditSetTextures 1.2.2.2 ") + __FUNCTION__, "Single QuadTree - GPU");
//			TheWorld_Utils::MemoryBuffer* westHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southHeights32Buffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* westNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southNormalsBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* westSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* eastSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* northSplatmapBuffer = nullptr;
//			TheWorld_Utils::MemoryBuffer* southSplatmapBuffer = nullptr;
//			generateSplatmapForBlendedQuadrants(quadTree, pos.getNumVerticesPerSize(), pos.getGridStepInWU(), 0.0f, 0.0f, 0.0f,
//				&float32HeigthsBuffer, &normalsBuffer, &splatmapBuffer, quadTree->getQuadrant()->getTerrainEdit(),
//				westHeights32Buffer, westNormalsBuffer, westSplatmapBuffer, nullptr,
//				eastHeights32Buffer, eastNormalsBuffer, eastSplatmapBuffer, nullptr,
//				northHeights32Buffer, northNormalsBuffer, northSplatmapBuffer, nullptr,
//				southHeights32Buffer, southNormalsBuffer, southSplatmapBuffer, nullptr,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeVerticalFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.slopeFlatFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtOnRocksFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationFactor,
//				quadTree->getQuadrant()->getTerrainEdit()->extraValues.splatMapMode);
//
//			quadTree->getQuadrant()->resetGlobalmapBuffer(__FUNCTION__);
//		}
//
//		{
//			std::lock_guard<std::recursive_mutex> lock(m_mtxMapQuadToSave);
//			//m_mapQuadToSave[pos] = "";
//			bool manual = true;
//			addQuadToSave(pos, manual);
//		}
//		quadTree->getQuadrant()->getTerrainEdit()->needUploadToServer = true;
//		quadTree->getQuadrant()->setNeedUploadToServer(true);
//
//		quadTree->getQuadrant()->getTerrainEdit()->splatmapNeedRegen(false);
//		quadTree->getQuadrant()->setSplatmapUpdated(true);
//
//		{
//			bool lockAcquired = quadTree->getQuadrant()->lockInternalData();
//			if (lockAcquired)
//			{
//				TheWorld_Utils::MemoryBuffer& globalmap = quadTree->getQuadrant()->getGlobalmapBuffer();
//				generateGlobalmap(quadTree, &globalmap);
//				quadTree->getQuadrant()->unlockInternalData();
//				quadTree->getQuadrant()->getTerrainEdit()->globalmapNeedRegen = false;
//				quadTree->getQuadrant()->setGlobalmapUpdated(true);
//			}
//
//		}
//
//		quadTree->materialParamsNeedReset(true);
//
//		{
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.lowElevationTexName_r) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::lowElevation);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.highElevationTexName_g) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::highElevation);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.dirtTexName_b) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::dirt);
//			if (strlen(quadTree->getQuadrant()->getTerrainEdit()->extraValues.rocksTexName_a) == 0)
//				quadTree->getQuadrant()->getTerrainEdit()->setTextureNameForTerrainType(TheWorld_Utils::TerrainEdit::TextureType::rocks);
//		}
//	}
//}

		//__declspec(dllexport) static void generateGroundImage(std::string outdir, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage);
		//__declspec(dllexport) static void generateGroundImage(MemoryBuffer& albedoBumpImage, MemoryBuffer& normalRoughnessImage, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage);

	//void TerrainEdit::generateGroundImage(MemoryBuffer& albedoBumpImage, MemoryBuffer& normalRoughnessImage, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage)
	//{
	//	size_t width, heigth;
	//	double d;

	//	// Color image formal controls
	//	d = sqrt(colorImage.size() / sizeof(struct TheWorld_Utils::_RGB));
	//	if (colorImage.size() == 0)
	//		throw(std::exception((std::string(__FUNCTION__) + std::string(" Color image empty")).c_str()));
	//	if (d * d * sizeof(struct TheWorld_Utils::_RGB) != colorImage.size())
	//		throw(std::exception((std::string(__FUNCTION__) + std::string(" Color image length not valid")).c_str()));
	//	width = heigth = (size_t)d;

	//	// Bump/Displ/Depth image formal controls
	//	if (bumpImage.size() > 0)
	//	{
	//		d = sqrt(bumpImage.size());
	//		if (d * d != bumpImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image differs from color image size")).c_str()));
	//	}

	//	// Normal image formal controls
	//	if (normalImage.size() > 0)
	//	{
	//		d = sqrt(normalImage.size() / sizeof(struct TheWorld_Utils::_RGB));
	//		if (d * d * sizeof(struct TheWorld_Utils::_RGB) != normalImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Normal image differs from color image size")).c_str()));
	//	}

	//	// Rough image formal controls
	//	if (roughImage.size() > 0)
	//	{
	//		d = sqrt(roughImage.size());
	//		if (d * d != roughImage.size())
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Rough image length not valid")).c_str()));
	//		if ((size_t)d != width)
	//			throw(std::exception((std::string(__FUNCTION__) + std::string(" Rough image differs from color image size")).c_str()));
	//	}

	//	size_t bufferSize = width * heigth * sizeof(struct TheWorld_Utils::_RGBA);

	//	albedoBumpImage.clear(__FUNCTION__);
	//	albedoBumpImage.reserve(bufferSize);
	//	albedoBumpImage.adjustSize(bufferSize);
	//	struct TheWorld_Utils::_RGBA* rgba = (struct TheWorld_Utils::_RGBA*)albedoBumpImage.ptr();
	//	struct TheWorld_Utils::_RGB* rgb = (struct TheWorld_Utils::_RGB*)colorImage.ptr();
	//	BYTE* a = bumpImage.size() > 0 ? bumpImage.ptr() : nullptr;
	//	for (size_t row = 0; row < heigth; row++)
	//		for (size_t col = 0; col < width; col++)
	//		{
	//			rgba->r = rgb->r;
	//			rgba->g = rgb->g;
	//			rgba->b = rgb->b;

	//			if (a != nullptr)
	//			{
	//				rgba->a = *a;
	//				a++;
	//			}
	//			else
	//				rgba->a = 255;

	//			rgba++;
	//			rgb++;
	//		}

	//	normalRoughnessImage.clear(__FUNCTION__);
	//	if (normalImage.size() > 0 || roughImage.size() > 0)
	//	{
	//		normalRoughnessImage.reserve(bufferSize);
	//		normalRoughnessImage.adjustSize(bufferSize);
	//		struct TheWorld_Utils::_RGBA* rgba = (struct TheWorld_Utils::_RGBA*)normalRoughnessImage.ptr();
	//		struct TheWorld_Utils::_RGB* rgb = normalImage.size() > 0 ? (struct TheWorld_Utils::_RGB*)normalImage.ptr() : nullptr;
	//		BYTE* a = roughImage.size() > 0 ? roughImage.ptr() : nullptr;
	//		for (size_t row = 0; row < heigth; row++)
	//			for (size_t col = 0; col < width; col++)
	//			{
	//				if (rgb != nullptr)
	//				{
	//					rgba->r = rgb->r;
	//					if (flipY)
	//						rgba->g = 255 - rgb->g;
	//					else
	//						rgba->g = rgb->g;
	//					rgba->b = rgb->b;
	//					rgb++;
	//				}
	//				else
	//				{
	//					rgba->r = 127;
	//					rgba->g = 127;
	//					rgba->b = 255;
	//				}

	//				if (a != nullptr)
	//				{
	//					rgba->a = *a;
	//					a++;
	//				}
	//				else
	//					rgba->a = 255;

	//				rgba++;
	//			}
	//	}
	//}

	//void TerrainEdit::generateGroundImage(std::string outdir, std::string groundTypeName, size_t imageSize, bool flipY, MemoryBuffer& colorImage, MemoryBuffer& bumpImage, MemoryBuffer& normalImage, MemoryBuffer& roughImage)
	//{
	//	MemoryBuffer albedoBumpImage;
	//	MemoryBuffer normalRoughnessImage;

	//	generateGroundImage(albedoBumpImage, normalRoughnessImage, groundTypeName, imageSize, flipY, colorImage, bumpImage, normalImage, roughImage);

	//	if (outdir.substr(outdir.size() - 1, 1) != "\\")
	//		outdir += "\\";
	//	FILE* outFile = nullptr;

	//	std::string albedoBumpImagePath = outdir + groundTypeName + "_albedo_bump.ground";
	//	errno_t err = fopen_s(&outFile, albedoBumpImagePath.c_str(), "wb");
	//	if (err != 0)
	//	{
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Open ") + albedoBumpImagePath.c_str() + " in errore - Err=" + std::to_string(err)).c_str()));
	//	}

	//	if (fwrite(albedoBumpImage.ptr(), albedoBumpImage.size(), 1, outFile) != 1)
	//	{
	//		fclose(outFile);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing ") + albedoBumpImagePath.c_str()).c_str()));
	//	}

	//	fclose(outFile);

	//	std::string normalRoughnessImagePath = outdir + groundTypeName + "_normal_roughness.ground";
	//	err = fopen_s(&outFile, normalRoughnessImagePath.c_str(), "wb");
	//	if (err != 0)
	//	{
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Open ") + normalRoughnessImagePath.c_str() + " in errore - Err=" + std::to_string(err)).c_str()));
	//	}

	//	if (fwrite(normalRoughnessImage.ptr(), normalRoughnessImage.size(), 1, outFile) != 1)
	//	{
	//		fclose(outFile);
	//		throw(GDN_TheWorld_Exception(__FUNCTION__, (std::string("Error writing ") + normalRoughnessImagePath.c_str()).c_str()));
	//	}

	//	fclose(outFile);
	//}

		//int shaderType = 0;		// StandardMaterial3D
		//int shaderType = 1;		// LookDev

		//if (shaderType == 0)
		//{
		//	godot::Ref<godot::StandardMaterial3D> material = memnew(godot::StandardMaterial3D);
		//	material->set_albedo(godot::Color(0.0f, 1.0f, 0.0f));
		//	terrainPlaneMaterial = material;
		//}
		//else if (shaderType == 1)
		//{
		//	godot::Ref<godot::ShaderMaterial> currentMaterial = m_quadTree->getQuadrant()->getShaderTerrainData()->getLookDevMaterial(ShaderTerrainData::LookDev::Splat);		// SHADER_PARAM_LOOKDEV_MAP
		//	godot::Ref<godot::ShaderMaterial> material = memnew(godot::ShaderMaterial);
		//	material->set_shader(currentMaterial->get_shader());

		//	int numVerticesPerQuadrantSize = (int)m_viewer->Globals()->numVerticesPerQuadrantSize();

		//	{
		//		size_t uint16_t_size = sizeof(uint16_t);	// the size of an half ==> float_16
		//		godot::PackedByteArray data;
		//		size_t heightmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * uint16_t_size;
		//		data.resize((int)heightmapBufferSizeInBytes);
		//		size_t heightsBufferSize = m_quadTree->getQuadrant()->getFloat16HeightsBuffer().size();
		//		assert(heightmapBufferSizeInBytes == heightsBufferSize);
		//		{
		//			memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getFloat16HeightsBuffer().ptr(), heightsBufferSize);
		//		}
		//		godot::Ref<godot::Image> image = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::Format::FORMAT_RH, data);
		//		// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
		//		//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_TERRAIN_HEIGHTMAP, tex);
		//	}

		//	{
		//		godot::Ref<godot::Image> image;
		//		size_t normalsBufferSize = 0;
		//		normalsBufferSize = m_quadTree->getQuadrant()->getNormalsBuffer().size();
		//		if (normalsBufferSize == 0)
		//		{
		//			godot::Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::Format::FORMAT_RGB8);
		//			image->fill(godot::Color(0, 0, 0, 1.0));
		//			image = im;
		//		}
		//		else
		//		{
		//			godot::PackedByteArray data;
		//			size_t normalmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGB);
		//			data.resize((int)normalmapBufferSizeInBytes);
		//			assert(normalmapBufferSizeInBytes == normalsBufferSize);
		//			{
		//				memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getNormalsBuffer().ptr(), normalsBufferSize);
		//			}
		//			godot::Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::Format::FORMAT_RGB8, data);
		//			image = im;
		//		}
		//		// Look at https://docs.godotengine.org/en/4.0/tutorials/shaders/shader_reference/shading_language.html#uniforms to activate hints corresponding to flags in the shader
		//		//tex->create_from_image(image, godot::Texture::FLAG_FILTER);
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_TERRAIN_NORMALMAP, tex);
		//	}

		//	{
		//		Ref<godot::Image> image;
		//		size_t splatmapBufferSize = 0;
		//		splatmapBufferSize = m_quadTree->getQuadrant()->getSplatmapBuffer().size();
		//		if (splatmapBufferSize == 0)
		//		{
		//			Ref<godot::Image> im = godot::Image::create(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::Format::FORMAT_RGBA8);
		//			image = im;
		//			image->fill(godot::Color(1.0f, 0.0f, 0.0f, 0.0f));
		//		}
		//		else
		//		{
		//			godot::PackedByteArray data;
		//			size_t splatmapBufferSizeInBytes = numVerticesPerQuadrantSize * numVerticesPerQuadrantSize * sizeof(struct TheWorld_Utils::_RGBA);
		//			data.resize((int)splatmapBufferSizeInBytes);
		//			assert(splatmapBufferSizeInBytes == splatmapBufferSize);
		//			{
		//				memcpy((char*)data.ptrw(), m_quadTree->getQuadrant()->getSplatmapBuffer().ptr(), splatmapBufferSizeInBytes);
		//			}
		//			Ref<godot::Image> im = godot::Image::create_from_data(numVerticesPerQuadrantSize, numVerticesPerQuadrantSize, false, godot::Image::Format::FORMAT_RGBA8, data);
		//			image = im;
		//			//std::string outFileName = m_quadTree->getQuadrant()->getCacheFilePath() + ".splatmap.png";
		//			//image->save_png(outFileName.c_str());
		//		}
		//		Ref<godot::ImageTexture> tex = godot::ImageTexture::create_from_image(image);
		//		material->set_shader_parameter(SHADER_PARAM_LOOKDEV_MAP, tex);
		//	}

		//	godot::Transform3D t(godot::Basis(), godot::Vector3(0.0f, 0.0f, 0.0f));
		//	godot::Variant inverseTransform = t.affine_inverse();
		//	material->set_shader_parameter(SHADER_PARAM_INVERSE_TRANSFORM, inverseTransform);
		//	Basis normalBasis = t.basis.inverse().transposed();
		//	material->set_shader_parameter(SHADER_PARAM_NORMAL_BASIS, normalBasis);
		//	material->set_shader_parameter(SHADER_PARAM_GRID_STEP, gridStepInWU);
		//	material->set_shader_parameter(SHADER_PARAM_FORCE_ELEVATION_0, true);
		//	//material->set_shader_parameter(SHADER_PARAM_EDITMODE_SELECTED, float(0.0f));
		//	//material->set_shader_parameter(SHADER_PARAM_MOUSE_HIT, godot::Vector2(-1, -1));
		//	//material->set_shader_parameter(SHADER_PARAM_QUAD_BORDER_LINE_THICKNESS, m_viewer->getShaderParamQuadBorderLineThickness());

		//	terrainPlaneMaterial = material;
		//}
		//else if (shaderType == 2)


		//__declspec(dllexport) bool checkBlendingQuadrantNeeded(size_t numVerticesPerSize, float gridStepInWU, bool lastPhase,
		//	CacheQuadrantData& data,
		//	CacheQuadrantData& northData,
		//	CacheQuadrantData& southData,
		//	CacheQuadrantData& westData,
		//	CacheQuadrantData& eastData,
		//	CacheQuadrantData& northwestData,
		//	CacheQuadrantData& northeastData,
		//	CacheQuadrantData& southwestData,
		//	CacheQuadrantData& southeastData);

	//bool MeshCacheBuffer::checkBlendingQuadrantNeeded(size_t numVerticesPerSize, float gridStepInWU, bool lastPhase,
	//	CacheQuadrantData& data,
	//	CacheQuadrantData& northData,
	//	CacheQuadrantData& southData,
	//	CacheQuadrantData& westData,
	//	CacheQuadrantData& eastData,
	//	CacheQuadrantData& northwestData,
	//	CacheQuadrantData& northeastData,
	//	CacheQuadrantData& southwestData,
	//	CacheQuadrantData& southeastData)
	//{
	//	return true;
	//}

	//__declspec(dllexport) void generateNormalsForBlendedQuadrants(size_t numVerticesPerSize, float gridStepInWU, TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& normalsBuffer, bool& requestedExit);
		//__declspec(dllexport) void generateSplatmap(size_t numVerticesPerSize, float gridStepInWU, TheWorld_Utils::TerrainEdit* terrainEdit, TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& normalsBuffer, TheWorld_Utils::MemoryBuffer& splatmapBuffer,
		//											float slopeVerticalFactor = 4.0f, float slopeFlatFactor = 1.0f, float dirtOnRocksFactor = 2.0f, float highElevationFactor = 4.0f, float lowElevationFactor = 2.0f, size_t splatMapMode = 1);
		//__declspec(dllexport) void deprecated_generateNormals(size_t numVerticesPerSize, float gridStepInWU, std::vector<float>& vectGridHeights, TheWorld_Utils::MemoryBuffer& normalsBuffer);
		//__declspec(dllexport) void deprecated_generateNormals(size_t numVerticesPerSize, float gridStepInWU, std::vector<float>& vectGridHeights, BYTE* normalsBuffer, const size_t normalsBufferSize, size_t& usedBufferSize);

	//void MeshCacheBuffer::generateNormalsForBlendedQuadrants(size_t numVerticesPerSize, float gridStepInWU, TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& east_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& south_float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& normalsBuffer, bool& requestedExit)
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer generateNormals 1 ") + __FUNCTION__, "ALL");

	//	size_t numVertices = float32HeigthsBuffer.size() / sizeof(float);
	//	my_assert(float32HeigthsBuffer.size() == numVertices * sizeof(float));
	//	my_assert(numVertices == 0 || numVertices == numVerticesPerSize * numVerticesPerSize);

	//	size_t requiredBufferSize = numVertices == 0 ? sizeof(struct TheWorld_Utils::_RGB) : numVertices * sizeof(struct TheWorld_Utils::_RGB);
	//	normalsBuffer.clear(__FUNCTION__);
	//	normalsBuffer.reserve(requiredBufferSize);
	//	normalsBuffer.adjustSize(requiredBufferSize);

	//	struct TheWorld_Utils::_RGB* _tempNormalmapBuffer = (struct TheWorld_Utils::_RGB*)normalsBuffer.ptr();

	//	size_t maxIndex = numVerticesPerSize - 1;

	//	if (numVertices > 0)
	//	{
	//		//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer generateNormals 1.2 ") + __FUNCTION__, "generation");

	//		for (int z = 0; z < numVerticesPerSize; z++)			// m_heightMapImage->get_height()
	//		{
	//			for (int x = 0; x < numVerticesPerSize; x++)		// m_heightMapImage->get_width()
	//			{
	//				if (requestedExit)
	//					return;
	//				
	//				float h = float32HeigthsBuffer.at<float>(x, z, numVerticesPerSize);

	//				Eigen::Vector3d normal;

	//				{
	//					//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer generateNormals 1.2.1 ") + __FUNCTION__, "calc vector");

	//					// h = height of the point for which we are computing the normal
	//					// hr = height of the point on the rigth (x growing)
	//					// hl = height of the point on the left (x lessening)
	//					// hf = height of the forward point (z growing)
	//					// hb = height of the backward point (z lessening)
	//					// step = step in WUs between points
	//					// we compute normal normalizing the vector (h - hr, step, h - hf) or (hl - h, step, hb - h)
	//					// according to https://hterrain-plugin.readthedocs.io/en/latest/ section "Procedural generation" it should be (h - hr, step, hf - h)
	//					
	//					//Eigen::Vector3d P(float(x) * gridStepInWU, h, float(z) * gridStepInWU);	// Verify
	//					
	//					if (x < maxIndex && z < maxIndex)
	//					{
	//						float hr = float32HeigthsBuffer.at<float>((size_t)(x + 1), (size_t)z, numVerticesPerSize);
	//						float hf = float32HeigthsBuffer.at<float>((size_t)x, (size_t)(z + 1), numVerticesPerSize);
	//						normal = Eigen::Vector3d(h - hr, gridStepInWU, h - hf).normalized();
	//																																						//normal = Eigen::Vector3d(h - hr, gridStepInWU, hf - h).normalized();
	//						//{	// Verify
	//						//	float nx = float(normal.x());
	//						//	float ny = float(normal.y());
	//						//	float nz = float(normal.z());
	//						//	Eigen::Vector3d PR(float(x) * gridStepInWU + gridStepInWU, hr, float(z) * gridStepInWU);
	//						//	Eigen::Vector3d PF(float(x) * gridStepInWU, hf, float(z) * gridStepInWU + gridStepInWU);
	//						//	Eigen::Vector3d normal1 = (PF - P).cross(PR - P).normalized();
	//						//	float n1x = float(normal1.x());
	//						//	float n1y = float(normal1.y());
	//						//	float n1z = float(normal1.z());
	//						//	if (!TheWorld_Utils::Utils::isEqualVectorWithLimitedPrecision(normal1, normal, 7))	// DEBUGRIC
	//						//		normal1 = normal;
	//						//}	// Verify
	//					}
	//					else if (x == maxIndex && z == maxIndex)
	//					{
	//						if (east_float32HeigthsBuffer.size() > 0 && south_float32HeigthsBuffer.size() > 0)
	//						{
	//							float hr = east_float32HeigthsBuffer.at<float>(1, z, numVerticesPerSize);
	//							float hf = south_float32HeigthsBuffer.at<float>(x, 1, numVerticesPerSize);
	//							normal = Eigen::Vector3d(h - hr, gridStepInWU, h - hf).normalized();
	//						}
	//						else
	//						{
	//							float hl = float32HeigthsBuffer.at<float>((size_t)(x - 1), (size_t)z, numVerticesPerSize);
	//							float hb = float32HeigthsBuffer.at<float>((size_t)x, (size_t)(z - 1), numVerticesPerSize);
	//							normal = Eigen::Vector3d(hl - h, gridStepInWU, hb - h).normalized();
	//						}
	//					}
	//					else if (x == maxIndex)	// && z < maxIndex
	//					{
	//						if (east_float32HeigthsBuffer.size() > 0)
	//						{
	//							float hr = east_float32HeigthsBuffer.at<float>(1, z, numVerticesPerSize);
	//							float hf = float32HeigthsBuffer.at<float>((size_t)x, (size_t)z + 1, numVerticesPerSize);
	//							normal = Eigen::Vector3d(h - hr, gridStepInWU, h - hf).normalized();
	//						}
	//						else
	//						{
	//							float hl = float32HeigthsBuffer.at<float>((size_t)x - 1, (size_t)z, numVerticesPerSize);
	//							float hf = float32HeigthsBuffer.at<float>((size_t)x, (size_t)z + 1, numVerticesPerSize);
	//							normal = Eigen::Vector3d(hl - h, gridStepInWU, h - hf).normalized();
	//						}
	//					}
	//					else	// x < maxIndex && z == maxIndex
	//					{
	//						if (south_float32HeigthsBuffer.size() > 0)
	//						{
	//							float hr = float32HeigthsBuffer.at<float>((size_t)x + 1, (size_t)z, numVerticesPerSize);
	//							float hf = south_float32HeigthsBuffer.at<float>((size_t)x, 1, numVerticesPerSize);
	//							normal = Eigen::Vector3d(h - hr, gridStepInWU, h - hf).normalized();
	//						}
	//						else
	//						{
	//							float hr = float32HeigthsBuffer.at<float>((size_t)x + 1, (size_t)z, numVerticesPerSize);
	//							float hb = float32HeigthsBuffer.at<float>((size_t)x, (size_t)z - 1, numVerticesPerSize);
	//							normal = Eigen::Vector3d(h - hr, gridStepInWU, hb - h).normalized();
	//						}
	//					}
	//				}

	//				{
	//					//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer generateNormals 1.2.3 ") + __FUNCTION__, "RGB");

	//							//float nx2 = float(normal.x());		// DEBUG ONLY
	//							//float ny2 = float(normal.y());		// DEBUG ONLY
	//							//float nz2 = float(normal.z());		// DEBUG ONLY
	//					Eigen::Vector3d packedNormal = packNormal(normal);
	//							//float nx = float(packedNormal.x());	// DEBUG ONLY
	//							//float ny = float(packedNormal.y());	// DEBUG ONLY
	//							//float nz = float(packedNormal.z());	// DEBUG ONLY
	//					//Eigen::Vector3d unpackedNormal = unpackNormal(packedNormal);	// DEBUG ONLY
	//					//		float nx1 = float(unpackedNormal.x());	// DEBUG ONLY
	//					//		float ny1 = float(unpackedNormal.y());	// DEBUG ONLY
	//					//		float nz1 = float(unpackedNormal.z());	// DEBUG ONLY
	//					struct _RGB rgb(0, 0, 0);
	//					rgb.r = (BYTE)(packedNormal.x() * 255);	// normals coords range from 0 to 1 but if expressed as color in a normalmap range from 0 to 255
	//					rgb.g = (BYTE)(packedNormal.y() * 255);
	//					rgb.b = (BYTE)(packedNormal.z() * 255);
	//					*_tempNormalmapBuffer = rgb;
	//					_tempNormalmapBuffer++;
	//				}
	//			}
	//		}
	//	}
	//	else
	//	{
	//		struct _RGB rgb(0, 0, 0);
	//		rgb.r = rgb.g = rgb.b = 0;
	//		*_tempNormalmapBuffer = rgb;
	//		_tempNormalmapBuffer++;
	//	}

	//	my_assert((BYTE*)_tempNormalmapBuffer - normalsBuffer.ptr() == normalsBuffer.size());
	//}

	//void MeshCacheBuffer::generateSplatmap(size_t numVerticesPerSize, float gridStepInWU, TheWorld_Utils::TerrainEdit* terrainEdit, TheWorld_Utils::MemoryBuffer& float32HeigthsBuffer, TheWorld_Utils::MemoryBuffer& normalsBuffer, TheWorld_Utils::MemoryBuffer& splatmapBuffer,
	//										float slopeVerticalFactor,	// raising it slope will have higher maximum and so rocks will be more diffuse on slopes
	//										float slopeFlatFactor,		// raising it slope will have lesser mininum and maximum so rocks will be less diffuse on slopes
	//										float dirtOnRocksFactor,	// raising it dirt will be more diffuse on rocks
	//										float highElevationFactor,	// raising it high elevation amount will be greater at lower altitude
	//										float lowElevationFactor,	// raising it low elevation amount will be greater at higher altitude
	//										size_t splatMapMode)
	//{
	//	assert(float32HeigthsBuffer.size() == numVerticesPerSize * numVerticesPerSize * sizeof(float));
	//	assert(normalsBuffer.size() == numVerticesPerSize * numVerticesPerSize * sizeof(struct TheWorld_Utils::_RGB));
	//	
	//	splatmapBuffer.clear(__FUNCTION__);
	//	size_t splatmapSize = numVerticesPerSize * numVerticesPerSize * sizeof(struct TheWorld_Utils::_RGBA);
	//	splatmapBuffer.reserve(splatmapSize);
	//	splatmapBuffer.adjustSize(splatmapSize);

	//	float diffAltitude = terrainEdit->maxHeight - terrainEdit->minHeight;
	//	
	//	for (size_t z = 0; z < numVerticesPerSize; z++)
	//		for (size_t x = 0; x < numVerticesPerSize; x++)
	//		{
	//			float h = float32HeigthsBuffer.at<float>(x, z, numVerticesPerSize);
	//			struct TheWorld_Utils::_RGB rgbNormal = normalsBuffer.at<TheWorld_Utils::_RGB>(x, z, numVerticesPerSize);
	//			// normals coords range from 0 to 255 in normals buffer we need they range from 0.0 to 1.0
	//			Eigen::Vector3d packedNormal((const double)(double(rgbNormal.r) / 255), (const double)(double(rgbNormal.g) / 255), (const double)(double(rgbNormal.b) / 255));
	//					//float nx = (float)packedNormal.x();	// DEBUG ONLY
	//					//float ny = (float)packedNormal.y();	// DEBUG ONLY
	//					//float nz = (float)packedNormal.z();	// DEBUG ONLY
	//			Eigen::Vector3d normal = unpackNormal(packedNormal);
	//					//nx = (float)normal.x();				// DEBUG ONLY
	//					//ny = (float)normal.y();				// DEBUG ONLY
	//					//nz = (float)normal.z();				// DEBUG ONLY
	//			//normal = normal.normalized();					// DEBUG ONLY
	//					//nx = (float)normal.x();				// DEBUG ONLY
	//					//ny = (float)normal.y();				// DEBUG ONLY
	//					//nz = (float)normal.z();				// DEBUG ONLY
	//			Eigen::Vector3d up(0.0f, 1.0f, 0.0f);
	//			
	//			// component on up direction of the normal (it is the y component of the normal if up is classic up == 0, 1, 0) - It ranges from 0=vertical terrain to 1=flat terrain
	//			float upComponent = float(normal.dot(up));
	//			
	//			// (1.0f - upComponent) ranges from 0=flat terrain to 1=vertical terrain so slope ranges from 0-slopeFlatFactor=flat terrain to slopeVerticalFactor-slopeFlatFactor=vertical terrain
	//			float slope = (slopeVerticalFactor * (1.0f - upComponent)) - slopeFlatFactor;

	//			// amounts range from 0.0f to 1.0f
	//			float rocksAmount = TheWorld_Utils::Utils::clamp<float>(slope, 0.0f, 1.0f);								// rocksAmount is max when slope is greater than 1 (vertical enough) and is null when slope is less than 0 (flat enough), otherwise it grows with the slope (if slope between 0 and 1)
	//			float dirtAmount = TheWorld_Utils::Utils::clamp<float>(rocksAmount * dirtOnRocksFactor, 0.0f, 1.0f);	// dirt grows on rocks
	//			float highElevationAmount = highElevationFactor * ((h - terrainEdit->minHeight) / diffAltitude) - lowElevationFactor;		// ranges from -lowElevationFactor to highElevationFactor-lowElevationFactor
	//			highElevationAmount = TheWorld_Utils::Utils::clamp<float>(highElevationAmount, 0.0f, 1.0f);				// highElevationAmount is capped from 0 and 1
	//			float lowElevationAmount = 1.0f - highElevationAmount;													// lowElevationAmount is the complement of highElevationAmount in range 0/1

	//			//if (rocksAmount == 0.0f)
	//			//	rocksAmount = 0.0f;

	//			struct TheWorld_Utils::_RGBA* rgba = splatmapBuffer.at_ptr<TheWorld_Utils::_RGBA>(x, z, numVerticesPerSize);

	//			float r, g, b, a;
	//			if (splatMapMode == 1)
	//			{
	//				r = lowElevationAmount;
	//				r = std::lerp(r, 0.0f, highElevationAmount);	// with higher priority: if highElevationAmount=0 r is unchanged, if highElevationAmount==>1 r==>0 (highElevationAmount can lower lowElevationAmount)
	//				r = std::lerp(r, 0.0f, dirtAmount);				// with higher priority: if dirtAmount=0 r is unchanged, if dirtAmount==>1 r==>0 (dirtAmount can lower lowElevationAmount)
	//				r = std::lerp(r, 0.0f, rocksAmount);			// with higher priority: if rocksAmount=0 r is unchanged, if rocksAmount==>1 r==>0 (rocksAmount can lower lowElevationAmount)

	//				g = highElevationAmount;
	//				g = std::lerp(g, 0.0f, dirtAmount);				// with higher priority: if dirtAmount=0 g is unchanged, if dirtAmount==>1 g==>0 (dirtAmount can lower highElevationAmount)
	//				g = std::lerp(g, 0.0f, rocksAmount);			// with higher priority: if rocksAmount=0 g is unchanged, if rocksAmount==>1 g==>0 (rocksAmount can lower highElevationAmount)

	//				b = dirtAmount;
	//				b = std::lerp(b, 0.0f, rocksAmount);			// with higher priority: if rocksAmount=0 b is unchanged, if rocksAmount==>1 b==>0 (rocksAmount can lower dirtAmount)

	//				a = rocksAmount;
	//			}
	//			else
	//			{
	//				a = rocksAmount;
	//				a = std::lerp(a, 0.0f, dirtAmount);
	//				a = std::lerp(a, 0.0f, highElevationAmount);
	//				a = std::lerp(a, 0.0f, lowElevationAmount);

	//				b = dirtAmount;
	//				b = std::lerp(b, 0.0f, highElevationAmount);
	//				b = std::lerp(b, 0.0f, lowElevationAmount);

	//				g = highElevationAmount;
	//				g = std::lerp(g, 0.0f, lowElevationAmount);

	//				r = lowElevationAmount;
	//			}

	//			rgba->r = BYTE(r * 255);
	//			rgba->g = BYTE(g * 255);
	//			rgba->b = BYTE(b * 255);
	//			rgba->a = BYTE(a * 255);
	//		}
	//}

	//void MeshCacheBuffer::deprecated_generateNormals(size_t numVerticesPerSize, float gridStepInWU, std::vector<float>& vectGridHeights, BYTE* normalsBuffer, const size_t normalsBufferSize, size_t& usedBufferSize)
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals1 1 ") + __FUNCTION__, "ALL");

	//	usedBufferSize = 0;

	//	size_t requiredBufferSize = vectGridHeights.size() == 0 ? sizeof(struct TheWorld_Utils::_RGB) : vectGridHeights.size() * sizeof(struct TheWorld_Utils::_RGB);

	//	my_assert(vectGridHeights.size() == 0 || vectGridHeights.size() == numVerticesPerSize * numVerticesPerSize);
	//	my_assert(normalsBufferSize >= requiredBufferSize);

	//	struct TheWorld_Utils::_RGB* _tempNormalmapBuffer = (struct TheWorld_Utils::_RGB*)normalsBuffer;

	//	size_t maxIndex = numVerticesPerSize - 1;

	//	if (vectGridHeights.size() > 0)
	//	{
	//		//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals1 1.2 ") + __FUNCTION__, "generation");

	//		size_t idx = 0;
	//		for (int z = 0; z < numVerticesPerSize; z++)			// m_heightMapImage->get_height()
	//		{
	//			for (int x = 0; x < numVerticesPerSize; x++)		// m_heightMapImage->get_width()
	//			{
	//				float h = vectGridHeights[idx];

	//				Eigen::Vector3d v;

	//				{
	//					//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals1 1.2.1 ") + __FUNCTION__, "calc vector");

	//					// h = height of the point for which we are computing the normal
	//					// hr = height of the point on the rigth
	//					// hl = height of the point on the left
	//					// hf = height of the forward point (z growing)
	//					// hb = height of the backward point (z lessening)
	//					// step = step in WUs between points
	//					// we compute normal normalizing the vector (h - hr, step, h - hf) or (hl - h, step, hb - h)
	//					// according to https://hterrain-plugin.readthedocs.io/en/latest/ section "Procedural generation" it should be (h - hr, step, hf - h)
	//					//Eigen::Vector3d P((float)x, h, (float)z);	// Verify
	//					if (x < maxIndex && z < maxIndex)
	//					{
	//						float hr = vectGridHeights[(size_t)z * numVerticesPerSize + (size_t)x + 1];
	//						float hf = vectGridHeights[((size_t)z + 1) * numVerticesPerSize + (size_t)x];
	//						v = Eigen::Vector3d(h - hr, gridStepInWU, h - hf).normalized();
	//						//{		// Verify
	//						//	Eigen::Vector3d PR((float)(x + gridStepInWU), hr, (float)z);
	//						//	Eigen::Vector3d PF((float)x, hf, (float)(z + gridStepInWU));
	//						//	Eigen::Vector3d normal1 = (PF - P).cross(PR - P).normalized();
	//						//	if (!equal(normal1, normal))	// DEBUGRIC
	//						//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//						//}
	//					}
	//					else
	//					{
	//						if (x == maxIndex && z == 0)
	//						{
	//							float hf = vectGridHeights[((size_t)z + 1) * numVerticesPerSize + x];
	//							float hl = vectGridHeights[(size_t)z * numVerticesPerSize + x - 1];
	//							v = Eigen::Vector3d(hl - h, gridStepInWU, h - hf).normalized();
	//							//{		// Verify
	//							//	Eigen::Vector3d PL((float)(x - gridStepInWU), hl, (float)z);
	//							//	Eigen::Vector3d PF((float)x, hf, (float)(z + gridStepInWU));
	//							//	Eigen::Vector3d normal1 = (PL - P).cross(PF - P).normalized();
	//							//	if (!equal(normal1, normal))	// DEBUGRIC
	//							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//							//}
	//						}
	//						else if (x == 0 && z == maxIndex)
	//						{
	//							float hr = vectGridHeights[(size_t)z * numVerticesPerSize + x + 1];
	//							float hb = vectGridHeights[(size_t)(z - 1) * numVerticesPerSize + x];
	//							v = Eigen::Vector3d(h - hr, gridStepInWU, hb - h).normalized();
	//							//{		// Verify
	//							//	Eigen::Vector3d PR((float)(x + gridStepInWU), hr, (float)z);
	//							//	Eigen::Vector3d PB((float)(x), hb, (float)(z - gridStepInWU));
	//							//	Eigen::Vector3d normal1 = (PR - P).cross(PB - P).normalized();
	//							//	if (!equal(normal1, normal))	// DEBUGRIC
	//							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//							//}
	//						}
	//						else
	//						{
	//							float hl = vectGridHeights[(size_t)z * numVerticesPerSize + x - 1];
	//							float hb = vectGridHeights[(size_t)(z - 1) * numVerticesPerSize + x];
	//							v = Eigen::Vector3d(hl - h, gridStepInWU, hb - h).normalized();
	//							//{		// Verify
	//							//	Eigen::Vector3d PB((float)x, hb, (float)(z - gridStepInWU));
	//							//	Eigen::Vector3d PL((float)(x - gridStepInWU), hl, (float)z);
	//							//	Eigen::Vector3d normal1 = (PB - P).cross(PL - P).normalized();
	//							//	if (!equal(normal1, normal))	// DEBUGRIC
	//							//		m_viewer->Globals()->debugPrint("Normal=" + String(normal) + " - Normal1= " + String(normal1));
	//							//}
	//						}
	//					}
	//				}
	//				
	//				Eigen::Vector3d normal;
	//				
	//				{
	//					//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals1 1.2.2 ") + __FUNCTION__, "normalize");

	//					normal = v.normalized();
	//				}

	//				{
	//					//TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals1 1.2.3 ") + __FUNCTION__, "RGB");

	//					Eigen::Vector3d packNormal(Eigen::Vector3d normal);
	//					Eigen::Vector3d packedNormal = packNormal(normal);
	//					struct _RGB rgb;
	//					rgb.r = (BYTE)(packedNormal.x() * 255);	// normals coord are from 0 to 1 but if expressed as color in a normlamap are from 0 to 255
	//					rgb.g = (BYTE)(packedNormal.y() * 255);
	//					rgb.b = (BYTE)(packedNormal.z() * 255);
	//					*_tempNormalmapBuffer = rgb;
	//					_tempNormalmapBuffer++;
	//				}

	//				idx++;
	//			}
	//		}
	//	}
	//	else
	//	{
	//		struct _RGB rgb(0, 0, 0);
	//		rgb.r = rgb.g = rgb.b = 0;
	//		*_tempNormalmapBuffer = rgb;
	//		_tempNormalmapBuffer++;
	//	}

	//	usedBufferSize = (BYTE*)_tempNormalmapBuffer - normalsBuffer;
	//}

	//void MeshCacheBuffer::deprecated_generateNormals(size_t numVerticesPerSize, float gridStepInWU, std::vector<float>& vectGridHeights, TheWorld_Utils::MemoryBuffer& normalsBuffer)
	//{
	//	TheWorld_Utils::GuardProfiler profiler(std::string("MeshCacheBuffer deprecated_generateNormals2 1 ") + __FUNCTION__, "serialize normals");

	//	my_assert(vectGridHeights.size() == 0 || vectGridHeights.size() == numVerticesPerSize * numVerticesPerSize);

	//	size_t requiredBufferSize = vectGridHeights.size() == 0 ? sizeof(struct TheWorld_Utils::_RGB) : vectGridHeights.size() * sizeof(struct TheWorld_Utils::_RGB);

	//	normalsBuffer.reserve(requiredBufferSize);

	//	size_t usedBufferSize = 0;
	//	deprecated_generateNormals(numVerticesPerSize, gridStepInWU, vectGridHeights, normalsBuffer.ptr(), normalsBuffer.reserved(), usedBufferSize);

	//	normalsBuffer.adjustSize(usedBufferSize);
	//}

//class QuadrantControl
//{
//public:
//	int32_t numOptLvlIncrements(void) const
//	{
//		return m_numOptLvlIncrements;
//	}
//	void numOptLvlIncrements(int32_t numOptLvlIncrements)
//	{
//		m_numOptLvlIncrements = numOptLvlIncrements;
//	}
//	int32_t lowLevelTextureIndex(void) const
//	{
//		return m_lowLevelTextureIndex;
//	}
//	void lowLevelTextureIndex(int32_t lowLevelTextureIndex)
//	{
//		m_lowLevelTextureIndex = lowLevelTextureIndex;
//	}
//	int32_t highLevelTextureIndex(void) const
//	{
//		return m_highLevelTextureIndex;
//	}
//	void highLevelTextureIndex(int32_t highLevelTextureIndex)
//	{
//		m_highLevelTextureIndex = highLevelTextureIndex;
//	}
//	int32_t dirtTextureIndex(void) const
//	{
//		return m_dirtTextureIndex;
//	}
//	void dirtTextureIndex(int32_t dirtTextureIndex)
//	{
//		m_dirtTextureIndex = dirtTextureIndex;
//	}
//	int32_t rockTextureIndex(void) const
//	{
//		return m_rockTextureIndex;
//	}
//	void rockTextureIndex(int32_t rockTextureIndex)
//	{
//		m_rockTextureIndex = rockTextureIndex;
//	}
//	float highElevationFactor(void) const
//	{
//		return m_highElevationFactor;
//	}
//	void highElevationFactor(float highElevationFactor)
//	{
//		m_highElevationFactor = highElevationFactor;
//	}
//	float lowElevationFactor(void) const
//	{
//		return m_lowElevationFactor;
//	}
//	void lowElevationFactor(float lowElevationFactor)
//	{
//		m_lowElevationFactor = lowElevationFactor;
//	}
//	float minHeight(void) const
//	{
//		return m_minHeight;
//	}
//	void minHeight(float minHeight)
//	{
//		m_minHeight = minHeight;
//	}
//	float maxHeight(void) const
//	{
//		return m_maxHeight;
//	}
//	void maxHeight(float maxHeight)
//	{
//		m_maxHeight = maxHeight;
//	}
//	float slopeVerticalFactor(void) const
//	{
//		return m_slopeVerticalFactor;
//	}
//	void slopeVerticalFactor(float slopeVerticalFactor)
//	{
//		m_slopeVerticalFactor = slopeVerticalFactor;
//	}
//	float slopeFlatFactor(void) const
//	{
//		return m_slopeFlatFactor;
//	}
//	void slopeFlatFactor(float slopeFlatFactor)
//	{
//		m_slopeFlatFactor = slopeFlatFactor;
//	}
//	float dirtOnRocksFactor(void) const
//	{
//		return m_dirtOnRocksFactor;
//	}
//	void dirtOnRocksFactor(float dirtOnRocksFactor)
//	{
//		m_dirtOnRocksFactor = dirtOnRocksFactor;
//	}
//
//private:
//	// u_quadrant_control_1
//	int32_t m_numOptLvlIncrements = 0;
//	int32_t m_lowLevelTextureIndex = 0;
//	int32_t m_highLevelTextureIndex = 0;
//	int32_t m_dirtTextureIndex = 0;
//	int32_t m_rockTextureIndex = 0;
//
//	// u_quadrant_control_2
//	float m_highElevationFactor = 0;
//	float m_lowElevationFactor = 0;
//	float m_minHeight = 0;
//	float m_maxHeight = 0;
//
//	// u_quadrant_control_3
//	float m_slopeVerticalFactor = 0;
//	float m_slopeFlatFactor = 0;
//	float m_dirtOnRocksFactor = 0;
//};
//
//m_quadrantControl.numOptLvlIncrements(int32_t(optimizationLevel / m_viewer->Globals()->getParamTextureOptimizationResolutionIncrement()));
////m_quadrantControl.lowLevelTextureIndex(terrainEdit->)	calcolare l'index da terrainEdit->extraValues.lowElevationTexName_r
////m_quadrantControl.highLevelTextureIndex(terrainEdit->)	calcolare l'index da terrainEdit->extraValues.highElevationTexName_g
////m_quadrantControl.dirtTextureIndex(terrainEdit->)	calcolare l'index da terrainEdit->extraValues.dirtTexName_b
////m_quadrantControl.rockTextureIndex(terrainEdit->)	calcolare l'index da terrainEdit->extraValues.rocksTexName_a
//m_quadrantControl.highElevationFactor(terrainEdit->extraValues.amplitudeBlendFactor);
//m_quadrantControl.lowElevationFactor(terrainEdit->extraValues.lowElevationFactor);
//m_quadrantControl.minHeight(terrainEdit->minHeight);
//m_quadrantControl.maxHeight(terrainEdit->maxHeight);
//m_quadrantControl.slopeVerticalFactor(terrainEdit->extraValues.slopeVerticalFactor);
//m_quadrantControl.slopeFlatFactor(terrainEdit->extraValues.slopeFlatFactor);
//m_quadrantControl.dirtOnRocksFactor(terrainEdit->extraValues.dirtOnRocksFactor);

//#define CHECK_PROCESS_STEP(qualifier) \
//	if (processCanBeInterrupted(GDN_TheWorld_Viewer::ProcessStep::ProcessStepIdx::##qualifier)) \
//	{ \
//		m_lastProcessExecutionInterrupted = true; \
//		return; \
//	}
//
//#define PROCESS_STEP(profiler_function, profiler_name, func, qualifier) \
//	{ \
//		TheWorld_Utils::GuardProfiler profiler(std::string(#profiler_function) + __FUNCTION__, #profiler_name); \
//		func; \
//		CHECK_PROCESS_STEP(qualifier) \
//}
//
//PROCESS_STEP("process_impl 1.04", "Init new quadrants", process_initNewQuads(processCameraPosForQuadrants), process_initNewQuads)
