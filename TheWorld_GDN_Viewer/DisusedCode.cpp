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
