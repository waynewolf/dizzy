#include <memory>
#include "scene.h"
#include "log.h"
#include "scene_graph.h"
#include "mesh.h"
#include "material.h"
#include "camera.h"
#include "light.h"
#include "animation.h"
#include "assimp_adapter.h"

using namespace std;

namespace dzy {

AssetIOStream::AssetIOStream() {
}

AssetIOStream::~AssetIOStream() {
}

std::size_t AssetIOStream::Read(void* buffer, std::size_t size, std::size_t count) {
    return 0;
}

std::size_t AssetIOStream::Write(const void* buffer, std::size_t size, std::size_t count) {
    return 0;
}

aiReturn AssetIOStream::Seek(std::size_t offset, aiOrigin origin) {
    return aiReturn_SUCCESS;
}

std::size_t AssetIOStream::Tell() const {
    return 0;
}

std::size_t AssetIOStream::FileSize() const {
    return 0;
}

void AssetIOStream::Flush() {
}

shared_ptr<Camera> AIAdapter::typeCast(aiCamera *camera) {
    shared_ptr<Camera> cam(new Camera(
        typeCast(camera->mPosition),
        typeCast(camera->mUp),
        typeCast(camera->mLookAt),
        camera->mHorizontalFOV,
        camera->mClipPlaneNear,
        camera->mClipPlaneFar,
        camera->mAspect));
    cam->setName(typeCast(camera->mName));

    return cam;
}

shared_ptr<Light> AIAdapter::typeCast(aiLight *light) {
    Light::LightSourceType type;
    switch(light->mType) {
    case aiLightSource_UNDEFINED:
        type = Light::LIGHT_SOURCE_UNDEFINED;
        break;
    case aiLightSource_DIRECTIONAL:
        type = Light::LIGHT_SOURCE_DIRECTIONAL;
        break;
    case aiLightSource_POINT:
        type = Light::LIGHT_SOURCE_POINT;
        break;
    case aiLightSource_SPOT:
        type = Light::LIGHT_SOURCE_SPOT;
        break;
    default:
        ALOGE("unknown light source type");
        return NULL;
    }

    shared_ptr<Light> lt(new Light(
        type,
        light->mAttenuationConstant,
        light->mAttenuationLinear,
        light->mAttenuationQuadratic,
        light->mAngleInnerCone,
        light->mAngleOuterCone));
    lt->setName(typeCast(light->mName));
    lt->mPosition      = typeCast(light->mPosition);
    lt->mDirection     = typeCast(light->mDirection);
    lt->mColorDiffuse  = typeCast(light->mColorDiffuse);
    lt->mColorSpecular = typeCast(light->mColorSpecular);
    lt->mColorAmbient  = typeCast(light->mColorAmbient);

    return lt;
}

shared_ptr<Texture>AIAdapter::typeCast(aiTexture *texture) {
    shared_ptr<Texture> tex(new Texture);
    return tex;
}

shared_ptr<Animation> AIAdapter::typeCast(aiAnimation *animation) {
    shared_ptr<Animation> anim(new Animation(
        animation->mDuration, animation->mTicksPerSecond));
    anim->setName(typeCast(animation->mName));
    for (int i=0; i<animation->mNumChannels; i++) {
        aiNodeAnim* nodeAnim = animation->mChannels[i];
        shared_ptr<NodeAnim> na(typeCast(nodeAnim));
        na->mAnimation = anim;
        anim->mNodeAnims.push_back(na);
    }
    for (int i=0; i<animation->mNumMeshChannels; i++) {
        aiMeshAnim* meshAnim = animation->mMeshChannels[i];
        shared_ptr<MeshAnim> ma(typeCast(meshAnim));
        ma->mAnimation = anim;
        anim->mMeshAnims.push_back(ma);
    }
    DUMP(Log::F_ANIMATION, "animation %s: duration %f, tps %f, na# %d, ma# %d",
        anim->getName().c_str(),
        anim->getDuration(), anim->getTicksPerSecond(),
        anim->getNumNodeAnims(), anim->getNumMeshAnims());
    return anim;
}

shared_ptr<NodeAnim> AIAdapter::typeCast(aiNodeAnim *nodeAnim) {
    const char* name = nodeAnim->mNodeName.C_Str();
    shared_ptr<NodeAnim> na(new NodeAnim(name));
    for (int i=0; i<nodeAnim->mNumPositionKeys; i++) {
        aiVectorKey position = nodeAnim->mPositionKeys[i];
        VectorState translationState(position.mTime, typeCast(position.mValue));
        na->mTranslationState.push_back(translationState);
        DUMP(Log::F_ANIMATION,
            "%-10s NA: %d/%d "
            "ts {time %f, value (%f, %f, %f)} ",
            name, i+1, nodeAnim->mNumPositionKeys,
            translationState.getTime(),
            translationState.getValue().x, translationState.getValue().y, translationState.getValue().z
        );
    }
    for (int i=0; i<nodeAnim->mNumRotationKeys; i++) {
        aiQuatKey rotation = nodeAnim->mRotationKeys[i];
        RotationState rotationState(rotation.mTime, typeCast(rotation.mValue));
        na->mRotationState.push_back(rotationState);
        DUMP(Log::F_ANIMATION,
            "%-10s NA: %d/%d "
            "rs {time %f, value (%f, %f, %f, %f)} ",
            name, i+1, nodeAnim->mNumRotationKeys,
            rotationState.getTime(),
            rotationState.getValue().w, rotationState.getValue().x,
            rotationState.getValue().y, rotationState.getValue().z
        );
    }
    for (int i=0; i<nodeAnim->mNumScalingKeys; i++) {
        aiVectorKey scale = nodeAnim->mScalingKeys[i];
        VectorState scaleState(scale.mTime, typeCast(scale.mValue));
        na->mScaleState.push_back(scaleState);
        DUMP(Log::F_ANIMATION,
            "%-10s NA: %d/%d "
            "ss {time %f, value (%f, %f, %f)} ",
            name, i+1, nodeAnim->mNumScalingKeys,
            scaleState.getTime(),
            scaleState.getValue().x, scaleState.getValue().y, scaleState.getValue().z
        );
    }
    return na;
}

shared_ptr<MeshAnim> AIAdapter::typeCast(aiMeshAnim *meshAnim) {
    shared_ptr<MeshAnim> ma(new MeshAnim);
    const char* name = meshAnim->mName.C_Str();
    ma->setName(name);
    for (int i=0; i<meshAnim->mNumKeys; i++) {
        aiMeshKey mk = meshAnim->mKeys[i];
        MeshState ms(mk.mTime, mk.mValue);
        ma->mMeshState.push_back(ms);
        DUMP(Log::F_ANIMATION,
            "%s ma: %d/%d "
            "ts {time %f, value %u} ",
            name, i+1, meshAnim->mNumKeys,
            ms.getTime(), ms.getValue()
        );
    }
    return ma;
}

shared_ptr<Material> AIAdapter::typeCast(aiMaterial *material) {
    shared_ptr<Material> ma(new Material);
    aiColor3D diffuse, specular, ambient, emission;
    float shininess;
    material->Get(AI_MATKEY_COLOR_DIFFUSE, diffuse);
    material->Get(AI_MATKEY_COLOR_SPECULAR, specular);
    material->Get(AI_MATKEY_COLOR_AMBIENT, ambient);
    material->Get(AI_MATKEY_COLOR_EMISSIVE, emission);
    material->Get(AI_MATKEY_SHININESS, shininess);
    ma->setDiffuse(typeCast(diffuse));
    ma->setSpecular(typeCast(specular));
    ma->setAmbient(typeCast(ambient));
    ma->setEmission(typeCast(emission));
    ma->setShininess(shininess);
    return ma;
}

shared_ptr<Mesh> AIAdapter::typeCast(aiMesh *mesh) {
    bool hasPoint = false, hasLine = false, hasTriangle = false;
    unsigned int type(mesh->mPrimitiveTypes);
    if (type & aiPrimitiveType_POLYGON) {
        ALOGE("polygon primivite type not supported");
        return NULL;
    }
    if (type & aiPrimitiveType_POINT)       hasPoint    = true;
    if (type & aiPrimitiveType_LINE)        hasLine     = true;
    if (type & aiPrimitiveType_TRIANGLE)    hasTriangle = true;

    if (!(hasPoint || hasLine || hasTriangle)) {
        ALOGE("no pritimive type found");
        return NULL;
    }

    // assimp document says "SortByPrimitiveType" can be used to
    // make sure output meshes contain only one primitive type each
    if ((hasPoint && hasLine) ||
        (hasPoint && hasTriangle) ||
        (hasLine && hasTriangle)) {
        ALOGE("mixed pritimive type not allowed, consider using assimp with different options");
        return NULL;
    }

    shared_ptr<Mesh> me(new Mesh(
        Mesh::PRIMITIVE_TYPE_TRIANGLE,
        mesh->mNumVertices));
    me->setName(typeCast(mesh->mName));

    int estimatedSize = 0;
    if (mesh->HasPositions())
        estimatedSize += me->mNumVertices * 3 *sizeof(float);
    estimatedSize += me->mNumVertices * mesh->GetNumColorChannels() * 4 * sizeof(float);
    estimatedSize += me->mNumVertices * mesh->GetNumUVChannels() * 3 * sizeof(float);
    if (mesh->HasNormals())
        estimatedSize += me->mNumVertices * 3 *sizeof(float);
    if (mesh->HasTangentsAndBitangents())
        estimatedSize += me->mNumVertices * 3 *sizeof(float) * 2;

    me->reserveDataStorage(estimatedSize);

    // Attention: rely on continuous memory layout of vertices in assimp
    if (mesh->HasPositions()) {
        me->appendVertexPositions(reinterpret_cast<unsigned char*>(mesh->mVertices),
            3, sizeof(float));
    }

    for (int i=0; i< mesh->GetNumColorChannels(); i++) {
        me->appendVertexColors(reinterpret_cast<unsigned char*>(mesh->mColors[i]),
            4, sizeof(float), i);
    }

    for (int i=0; i< mesh->GetNumUVChannels(); i++) {
        me->appendVertexTextureCoords(reinterpret_cast<unsigned char*>(mesh->mTextureCoords[i]),
            mesh->mNumUVComponents[i], sizeof(float), i);
    }

    if (mesh->HasNormals()) {
        me->appendVertexNormals(reinterpret_cast<unsigned char*>(mesh->mNormals),
            3, sizeof(float));
    }

    if (mesh->HasTangentsAndBitangents()) {
        me->appendVertexTangents(reinterpret_cast<unsigned char*>(mesh->mTangents),
            3, sizeof(float));
        me->appendVertexBitangents(reinterpret_cast<unsigned char*>(mesh->mBitangents),
            3, sizeof(float));
    }
    if (mesh->HasFaces()) {
        // cannot use Mesh::buildIndexBuffer,  assimp aiFace's indices buffer
        // in the neighbouring faces are not continuous
        me->mNumFaces = mesh->mNumFaces;
        me->mTriangleFaces.resize(me->mNumFaces);
        for (int i=0; i<me->mNumFaces; i++) {
            assert(mesh->mFaces[i].mNumIndices == 3);
            me->mTriangleFaces[i].mIndices[0] = mesh->mFaces[i].mIndices[0];
            me->mTriangleFaces[i].mIndices[1] = mesh->mFaces[i].mIndices[1];
            me->mTriangleFaces[i].mIndices[2] = mesh->mFaces[i].mIndices[2];
        }
    }
    if (mesh->HasBones()) {
        me->allocateTransformDataArea();
        for (int i=0; i<mesh->mNumBones; i++) {
            aiBone* bone = mesh->mBones[i];
            shared_ptr<Bone> b(typeCast(bone));
            me->mBones.push_back(b);
        }
    }

    me->mMaterialIndex = mesh->mMaterialIndex;

    return me;
}

shared_ptr<Bone> AIAdapter::typeCast(aiBone *bone) {
    const char* name = bone->mName.C_Str();
    shared_ptr<Bone> b(new Bone(name));
    for (int i=0; i<bone->mNumWeights; i++) {
        aiVertexWeight vw = bone->mWeights[i];
        VertexWeight vertexWeight(vw.mVertexId, vw.mWeight);
        b->mWeights.push_back(vertexWeight);
    }
    aiVector3D scale, translation;
    aiQuaternion rotation;
    bone->mOffsetMatrix.Decompose(scale, rotation, translation);
    b->mTransform = Transform(typeCast(translation), typeCast(rotation), typeCast(scale));
    b->mTransform.dump(Log::F_BONE, "%-10s bone offset matrix", name);

    return b;
}

shared_ptr<Node> AIAdapter::typeCast(aiNode *node) {
    shared_ptr<Node> n(new Node(node->mName.C_Str()));
    aiVector3D scale, translation;
    aiQuaternion rotation;
    node->mTransformation.Decompose(scale, rotation, translation);
    n->setLocalScale(typeCast(scale));
    n->setLocalRotation(typeCast(rotation));
    n->setLocalTranslation(typeCast(translation));
    return n;
}

glm::vec3 AIAdapter::typeCast(const aiVector3D &vec3d) {
    return glm::vec3(vec3d.x, vec3d.y, vec3d.z);
}

glm::vec3 AIAdapter::typeCast(const aiColor3D &color3d) {
    return glm::vec3(color3d.r, color3d.g, color3d.b);
}

glm::vec4 AIAdapter::typeCast(const aiColor4D &color4d) {
    return glm::vec4(color4d.r, color4d.g, color4d.b, color4d.a);
}

glm::quat AIAdapter::typeCast(const aiQuaternion& quaternion) {
    return glm::quat(quaternion.w, quaternion.x, quaternion.y, quaternion.z);
}

// assimp matrix has different memory layout with glm matrix
glm::mat4 AIAdapter::typeCast(const aiMatrix4x4 &mat4) {
    glm::mat4 to;
    to[0][0] = mat4.a1; to[1][0] = mat4.a2; to[2][0] = mat4.a3; to[3][0] = mat4.a4;
    to[0][1] = mat4.b1; to[1][1] = mat4.b2; to[2][1] = mat4.b3; to[3][1] = mat4.b4;
    to[0][2] = mat4.c1; to[1][2] = mat4.c2; to[2][2] = mat4.c3; to[3][2] = mat4.c4;
    to[0][3] = mat4.d1; to[1][3] = mat4.d2; to[2][3] = mat4.d3; to[3][3] = mat4.d4;
    return to;
}

string AIAdapter::typeCast(const aiString &str) {
    return string(str.C_Str());
}

void AIAdapter::buildSceneGraph(shared_ptr<Scene> scene, aiNode *aroot) {
    shared_ptr<Node> root = typeCast(aroot);
    linkNode(scene, root, aroot);
    scene->mRootNode->attachChild(root);
}

void AIAdapter::postProcess(shared_ptr<Scene> scene) {
    shared_ptr<Node> rootNode(scene->mRootNode);
    assert(rootNode != nullptr);
    // compute camera local transform
    for (int i=0; i<scene->getNumCameras(); i++) {
        shared_ptr<Camera> camera(scene->getCamera(i));
        shared_ptr<NodeObj> cameraNode(rootNode->getChild(camera->getName()));
        Transform transform = cameraNode->getWorldTransform();
        camera->translate(transform.getTranslation());
        camera->rotate(transform.getRotation());
        camera->scale(transform.getScale());
        // FIXME: blender exported camera transform is buggy, different
        // with what we can see in blender, remove the following lines
        // after figuring out the blender exporter issue.
        camera->setPostion(DEFAULT_CAMERA_POS);
        camera->setLookAt(DEFAULT_CAMERA_CENTER);
        camera->setUp(DEFAULT_CAMERA_UP);
        camera->resetTransform();

        // TODO: remove camera node from scene graph
        //cameraNode->detachFromParent();
    }
    // compute light local transform
    for (int i=0; i<scene->getNumLights(); i++) {
        shared_ptr<Light> light(scene->getLight(i));
        shared_ptr<NodeObj> lightNode(rootNode->getChild(light->getName()));
        Transform transform = lightNode->getWorldTransform();
        light->translate(transform.getTranslation());
        light->rotate(transform.getRotation());
        light->scale(transform.getScale());

        // TODO: remove light node from scene graph
        //lightNode->detachFromParent();
    }
    // Attach animation to Node
    for (int i=0; i<scene->getNumAnimations(); i++) {
        shared_ptr<Animation> animation(scene->getAnimation(i));
        for (int j=0; j<animation->getNumNodeAnims(); j++) {
            shared_ptr<NodeAnim> nodeAnim(animation->getNodeAnim(j));
            shared_ptr<NodeObj> node(rootNode->getChild(nodeAnim->getName()));
            if (!node) {
                ALOGW("NodeAnim affected Node %s not found in scene graph",
                    nodeAnim->getName().c_str());
                continue;
            } else {
                node->setAnimation(nodeAnim);
                DUMP(Log::F_ANIMATION, "%-10s NA attached to %-10s Node",
                    nodeAnim->getName().c_str(), node->getName().c_str());
            }
        }
    }
}

void AIAdapter::linkNode(shared_ptr<Scene> scene,
    shared_ptr<Node> node, aiNode *anode) {
    for (unsigned int i = 0; i < anode->mNumChildren; i++) {
        shared_ptr<Node> c = typeCast(anode->mChildren[i]);
        c->setUpdateFlag(NodeObj::F_UPDATE_BONE_TRANSFORM, false);
        node->attachChild(c);
        linkNode(scene, c, anode->mChildren[i]);
    }
    // build one Geometry to reference one Mesh,
    // only Geometry allowed to attach one and only one Mesh,
    // this can simplify scene graph code, but may increase
    // scene graph depth compared to assimp
    for (unsigned int i = 0; i < anode->mNumMeshes; i++) {
        unsigned int meshIdx = anode->mMeshes[i];
        shared_ptr<Mesh> mesh(scene->mMeshes[meshIdx]);
        shared_ptr<Geometry> c(new Geometry(mesh));
        c->setUpdateFlag(NodeObj::F_UPDATE_WORLD_TRANSFORM, false);
        c->setUpdateFlag(NodeObj::F_UPDATE_BONE_TRANSFORM, false);
        c->setMaterial(scene->mMaterials[mesh->mMaterialIndex]);
        node->attachChild(c);
    }
}

} // namespace dzy
