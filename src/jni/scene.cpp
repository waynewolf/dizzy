#include <android/asset_manager.h>
#include <fstream>
#include <assert.h>
#include "app_context.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "log.h"
#include "scene.h"

using namespace std;

namespace dzy {

Scene::Scene() :
    mSceneData(NULL),
    mSceneSize(0) {
}

Scene::~Scene() {
}

FlatScene::FlatScene() {
}

FlatScene::~FlatScene() {
}

bool FlatScene::loadAsset(shared_ptr<AppContext> appContext,
    const string &assetFile) {    
    AAssetManager *assetManager = appContext->getAssetManager();
    assert(assetManager != NULL);

    AAsset* asset = AAssetManager_open(assetManager,
        assetFile.c_str(), AASSET_MODE_BUFFER);

    if (!asset) {
        ALOGE("Failed to open asset: %s", assetFile.c_str());
        return false;
    }

    off_t length = AAsset_getLength(asset);
    char * buffer = new char[length];
    size_t sz = AAsset_read(asset, buffer, length);
    AAsset_close(asset);
    if (sz != length) {
        ALOGE("Partial read %d bytes", sz);
        delete[] buffer;
        return false;
    }

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFileFromMemory(
        buffer, length,
        aiProcess_Triangulate |
        aiProcess_JoinIdenticalVertices |
        aiProcess_SortByPType);
    if (!scene) {
        ALOGE("assimp failed to load %s: %s", assetFile.c_str(),
            importer.GetErrorString());
        delete[] buffer;
        return false;
    }

    // parse aiScene to FlatScene

    delete[] buffer;

    return true;
}

bool FlatScene::load(shared_ptr<AppContext> appContext,
    const string &file) {
    ifstream ifs(file.c_str(), ifstream::binary);
    if (!ifs) {
        ALOGE("ifstream ctor failed for %s", file.c_str());
        return false;
    }

    ifs.seekg(0, ifs.end);
    int length = ifs.tellg();
    ifs.seekg(0, ifs.beg);
    char * buffer = new char[length];
    ifs.read(buffer, length);
    ifs.close();
    if (!ifs) {
        ALOGE("Partial read %d bytes", ifs.gcount());
        delete[] buffer;
        return false;
    }

    // process the buffer
    buffer[length - 1] = 0;
    ALOGD("%s: %s", file.c_str(), buffer);
    delete[] buffer;

    return true;
}

bool FlatScene::listAssetFiles(shared_ptr<AppContext> appContext,
    const string &dir) {
    AAssetManager *assetManager = appContext->getAssetManager();
    assert(assetManager != NULL);

    AAssetDir* assetDir = AAssetManager_openDir(assetManager, dir.c_str());
    if (!assetDir) {
        ALOGE("Failed to open asset root dir");
        return false;
    }

    const char * assetFile = NULL;
    while (assetFile = AAssetDir_getNextFileName(assetDir)) {
        ALOGD("%s", assetFile);
    }

    AAssetDir_close(assetDir);

    return true;
}

SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

shared_ptr<Scene> SceneManager::createScene(SceneType sceneType) {
    shared_ptr<Scene> scene(NULL);
    if (sceneType == SCENE_TYPE_FLAT)
        scene.reset(new FlatScene);
    return scene;
}

} // namespace dzy
