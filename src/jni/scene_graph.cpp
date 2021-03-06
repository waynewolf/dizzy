#include <algorithm>
#include <sstream>
#include <functional>
#include <queue>
#include <stack>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "log.h"
#include "program.h"
#include "scene.h"
#include "render.h"
#include "mesh.h"
#include "material.h"
#include "animation.h"
#include "scene_graph.h"

using namespace std;

namespace dzy {

NodeObj::NodeObj(const string& name)
    : NameObj(name)
    , mUseAutoProgram(true)
    , mUpdateFlags(0) {
}

NodeObj::~NodeObj() {
    TRACE(getName().c_str());
}

glm::quat NodeObj::getWorldRotation() {
    updateWorldTransform();
    return mWorldTransform.getRotation();
}

glm::vec3 NodeObj::getWorldTranslation() {
    updateWorldTransform();
    return mWorldTransform.getTranslation();
}

glm::vec3 NodeObj::getWorldScale() {
    updateWorldTransform();
    return mWorldTransform.getScale();
}

Transform NodeObj::getWorldTransform() {
    updateWorldTransform();
    return mWorldTransform;
}

glm::quat NodeObj::getLocalRotation() {
    return mLocalTransform.getRotation();
}

void NodeObj::setLocalRotation(const glm::quat& quaternion) {
    mLocalTransform.setRotation(quaternion);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

void NodeObj::setLocalRotation(float w, float x, float y, float z) {
    glm::quat q(w, x, y, z);
    setLocalRotation(q);
}

glm::vec3 NodeObj::getLocalScale() {
    return mLocalTransform.getScale();
}

void NodeObj::setLocalScale(float scale) {
    mLocalTransform.setScale(scale);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

void NodeObj::setLocalScale(float x, float y, float z) {
    mLocalTransform.setScale(x, y, z);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

void NodeObj::setLocalScale(const glm::vec3& scale) {
    mLocalTransform.setScale(scale);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

glm::vec3 NodeObj::getLocalTranslation() {
    return mLocalTransform.getTranslation();
}

void NodeObj::setLocalTranslation(const glm::vec3& translation) {
    mLocalTransform.setTranslation(translation);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

void NodeObj::setLocalTranslation(float x, float y, float z) {
    mLocalTransform.setTranslation(x, y, z);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

void NodeObj::setLocalTransform(const Transform& transform) {
    mLocalTransform = transform;
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
}

Transform NodeObj::getLocalTransform() {
    return mLocalTransform;
}

NodeObj& NodeObj::translate(float x, float y, float z) {
    return translate(glm::vec3(x, y, z));
}

NodeObj& NodeObj::translate(const glm::vec3& offset) {
    glm::vec3 translate = mLocalTransform.getTranslation();
    mLocalTransform.setTranslation(translate + offset);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
    return *this;
}

NodeObj& NodeObj::scale(float s) {
    return scale(s, s, s);
}

NodeObj& NodeObj::scale(const glm::vec3& v) {
    return scale(v.x, v.y, v.z);
}

NodeObj& NodeObj::scale(float x, float y, float z) {
    glm::vec3 scale = mLocalTransform.getScale();
    mLocalTransform.setScale(glm::vec3(x, y, z) * scale);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
    return *this;
}

NodeObj& NodeObj::rotate(const glm::quat& rotation) {
    glm::quat rot = mLocalTransform.getRotation();
    mLocalTransform.setRotation(rotation * rot);
    setUpdateFlag(F_UPDATE_WORLD_TRANSFORM, true);
    return *this;
}

NodeObj& NodeObj::rotate(float axisX, float axisY, float axisZ) {
    glm::quat quaternion(glm::vec3(axisX, axisY, axisZ));
    return rotate(quaternion);
}

Transform NodeObj::getBoneTransform(double timeStamp) {
    updateBoneTransform(timeStamp);
    return mBoneTransform;
}

void NodeObj::updateWorldTransform() {
    if ((mUpdateFlags & F_UPDATE_WORLD_TRANSFORM) == 0) {
        return;
    }

    shared_ptr<NodeObj> parent(getParent());
    shared_ptr<NodeObj> nodeObj = shared_from_this();
    vector<shared_ptr<NodeObj> > path;
    while (parent && (parent->mUpdateFlags & F_UPDATE_WORLD_TRANSFORM) != 0) {
        path.push_back(nodeObj);
        nodeObj = parent;
        parent = nodeObj->getParent();
    }
    path.push_back(nodeObj);

    for (int i = path.size() - 1; i >= 0; i--) {
        nodeObj = path[i];
        nodeObj->doUpdateWorldTransform();
    }
}

void NodeObj::doUpdateWorldTransform() {
    shared_ptr<Node> parent(getParent());
    if (!parent) {
        mWorldTransform = mLocalTransform;
        mUpdateFlags &= ~F_UPDATE_WORLD_TRANSFORM;
    } else {
        assert ((parent->mUpdateFlags & F_UPDATE_WORLD_TRANSFORM) == 0);
        mWorldTransform = mLocalTransform;
        mWorldTransform.combine(parent->mWorldTransform);
        mUpdateFlags &= ~F_UPDATE_WORLD_TRANSFORM;
    }
}

void NodeObj::updateBoneTransform(double timeStamp) {
    if ((mUpdateFlags & F_UPDATE_BONE_TRANSFORM) == 0) {
        return;
    }

    shared_ptr<NodeObj> parent(getParent());
    shared_ptr<NodeObj> nodeObj = shared_from_this();
    vector<shared_ptr<NodeObj> > path;
    while (parent && (parent->mUpdateFlags & F_UPDATE_BONE_TRANSFORM) != 0) {
        path.push_back(nodeObj);
        nodeObj = parent;
        parent = nodeObj->getParent();
    }
    path.push_back(nodeObj);

    for (int i = path.size() - 1; i >= 0; i--) {
        nodeObj = path[i];
        nodeObj->doUpdateBoneTransform(timeStamp);
    }
}

void NodeObj::doUpdateBoneTransform(double timeStamp) {
    mBoneTransform = mLocalTransform;
    // if node anim is attached to this node, use that transform instead
    shared_ptr<NodeAnim> nodeAnim(getAnimation());
    if (nodeAnim) {
        // get current time stamp, interpolate TRS states in NodeAnim
        glm::vec3 T = nodeAnim->getTranslation(timeStamp);
        glm::quat R = nodeAnim->getRotation(timeStamp);
        glm::vec3 S = nodeAnim->getScale(timeStamp);
        mBoneTransform = Transform(T, R, S);
    }

    shared_ptr<Node> parent(getParent());
    if (parent && parent->getName() != "dzyroot") {
        assert ((parent->mUpdateFlags & F_UPDATE_BONE_TRANSFORM) == 0);
        mBoneTransform.combine(parent->mBoneTransform);
    }
    mUpdateFlags &= ~F_UPDATE_BONE_TRANSFORM;
}

void NodeObj::setUpdateFlag(UpdateFlag f, bool recursive) {
    recursive = recursive;
    mUpdateFlags |= f;
}

shared_ptr<Node> NodeObj::getParent() {
    return mParent.lock();
}

void NodeObj::setParent(shared_ptr<Node> parent) {
    mParent = parent;
}

bool NodeObj::isAutoProgram() {
    return mUseAutoProgram;
}

void NodeObj::setProgram(shared_ptr<Program> program) {
    mUseAutoProgram = false;
    mProgram = program;
}

shared_ptr<Program> NodeObj::getProgram() {
    return mProgram.lock();
}

shared_ptr<Program> NodeObj::getProgram(
    shared_ptr<Material> material, bool hasLight, shared_ptr<Mesh> mesh) {
    if (!getProgram()) {
        if (mUseAutoProgram) {
            mProgram = ProgramManager::get()->getCompatibleProgram(material, hasLight, mesh);
        } else {
            ALOGE("No shader program found");
        }
    }
    return mProgram.lock();
}

void NodeObj::setMaterial(shared_ptr<Material> material) {
    mMaterial = material;
}

shared_ptr<Material> NodeObj::getMaterial() {
    return mMaterial;
}

void NodeObj::setLight(shared_ptr<Light> light) {
    mLight = light;
}

shared_ptr<Light> NodeObj::getLight() {
    return mLight;
}

void NodeObj::setCamera(shared_ptr<Camera> camera) {
    mCamera = camera;
}

shared_ptr<Camera> NodeObj::getCamera() {
    return mCamera;
}

void NodeObj::setAnimation(std::shared_ptr<NodeAnim> nodeAnim) {
    mAnimation = nodeAnim;
}

std::shared_ptr<NodeAnim> NodeObj::getAnimation() {
    return mAnimation;
}

void NodeObj::updateAnimation(double timeStamp) {
    shared_ptr<NodeAnim> nodeAnim(getAnimation());
    if (nodeAnim) {
        //glm::vec3 nodeLocalTranslation = node->getLocalTranslation();
        //glm::quat nodeLocalRotation = node->getLocalRotation();
        //glm::vec3 nodeLocalScale = node->getLocalScale();

        // get current time stamp, interpolate TRS states in NodeAnim
        glm::vec3 animTranslation = nodeAnim->getTranslation(timeStamp);
        glm::quat animRotation = nodeAnim->getRotation(timeStamp);
        glm::vec3 animScale = nodeAnim->getScale(timeStamp);
        // do not use traslate, rotate & scale functions, because these are accumulative,
        // we need to reset all and transform from scratch.
        setLocalTranslation(animTranslation);
        setLocalRotation(animRotation);
        setLocalScale(animScale);
    }
}

Node::Node(const string& name)
    : NodeObj(name) {
}

Node::~Node() {
    TRACE(getName().c_str());
}

void Node::attachChild(shared_ptr<NodeObj> childNode) {
    // no duplicate Node in children list
    bool exist = false;
    for (auto iter = mChildren.begin(); iter != mChildren.end(); iter++) {
        if (*iter == childNode) {
            exist = true;
            ALOGW("avoid inserting duplicate Node");
            break;
        }
    }

    if (!exist) {
        mChildren.push_back(childNode);
        shared_ptr<Node> oldParent(childNode->getParent());
        childNode->setParent(dynamic_pointer_cast<Node>(shared_from_this()));
        if (oldParent) {
            // remove childNode from oldParent
            oldParent->mChildren.erase(remove(
                oldParent->mChildren.begin(), oldParent->mChildren.end(), childNode),
                oldParent->mChildren.end());
        }
    }
}

shared_ptr<NodeObj> Node::getChild(int idx) {
    return mChildren[idx];
}

shared_ptr<NodeObj> Node::getChild(const string &name) {
    queue<shared_ptr<NodeObj> > q;
    q.push(shared_from_this());
    while(!q.empty()) {
        shared_ptr<NodeObj> nodeObj(q.front());
        q.pop();
        if (nodeObj->getName() == name) return nodeObj;
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(nodeObj);
        if (node) {
            for (size_t i = 0; i < node->mChildren.size(); i++) {
                q.push(node->mChildren[i]);
            }
        }
    }
    return nullptr;
}

void Node::depthFirstTraversal(shared_ptr<Scene> scene, VisitSceneFunc visit) {
    stack<shared_ptr<NodeObj> > stk;
    stk.push(shared_from_this());
    while (!stk.empty()) {
        shared_ptr<NodeObj> nodeObj = stk.top();
        stk.pop();
        visit(scene, nodeObj);
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(nodeObj);
        if (!node) {
            for (auto it = node->mChildren.rbegin(); it != node->mChildren.rend(); ++it) {
                stk.push(*it);
            }
        }
    }
}

void Node::depthFirstTraversal(VisitFunc visit) {
    stack<shared_ptr<NodeObj> > stk;
    stk.push(shared_from_this());
    while (!stk.empty()) {
        shared_ptr<NodeObj> nodeObj = stk.top();
        stk.pop();
        visit(nodeObj);
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(nodeObj);
        if (node) {
            for (auto it = node->mChildren.rbegin(); it != node->mChildren.rend(); ++it) {
                stk.push(*it);
            }
        }
    }
}

void Node::update(double timeStamp) {
    setUpdateFlag(NodeObj::F_UPDATE_BONE_TRANSFORM, false);
    std::for_each(mChildren.begin(), mChildren.end(), [=] (shared_ptr<NodeObj> c) {
        c->update(timeStamp);
    });
}

void Node::draw(Render &render, shared_ptr<Scene> scene, double timeStamp) {
    NodeObj::updateAnimation(timeStamp);
    render.drawNode(scene, dynamic_pointer_cast<Node>(shared_from_this()));
    std::for_each(mChildren.begin(), mChildren.end(), [&] (shared_ptr<NodeObj> c) {
        c->draw(render, scene, timeStamp);
    });
}

void Node::dumpHierarchy(Log::Flag f) {
    if (!Log::flagEnabled(f) || !Log::debugSwitchOn()) return;

    typedef pair<shared_ptr<NodeObj>, int> STACK_ELEM;
    stack<STACK_ELEM> stk;
    stk.push(make_pair(shared_from_this(), 0));
    while (!stk.empty()) {
        STACK_ELEM elem = stk.top();
        stk.pop();
        shared_ptr<NodeObj> nodeObj = elem.first;
        int depth = elem.second;
        ostringstream os;
        for (int i=0; i<depth; i++) os << "    ";
        os << "%d:%s";
        DUMP(f, os.str().c_str(), depth, nodeObj->getName().c_str());
        shared_ptr<Node> node = dynamic_pointer_cast<Node>(nodeObj);
        if (node) {
            for (auto it = node->mChildren.rbegin(); it != node->mChildren.rend(); ++it) {
                stk.push(make_pair(*it, depth+1));
            }
        }
    }
}

void Node::setUpdateFlag(UpdateFlag f, bool recursive) {
    NodeObj::setUpdateFlag(f, false);
    if (recursive) {
        for_each(mChildren.begin(), mChildren.end(), [=](shared_ptr<NodeObj> &nodeObj) {
            if ((nodeObj->mUpdateFlags & f) == 0)
                nodeObj->setUpdateFlag(f, true);
        });
    }
}

Geometry::Geometry(shared_ptr<Mesh> mesh)
    : Geometry(string("Geometry-") + mesh->getName(), mesh) {
}

Geometry::Geometry(const string& name, shared_ptr<Mesh> mesh)
    : NodeObj(name)
    , mMesh(mesh)
    , mBOUpdated(false) {
    glGenBuffers(1, &mVertexBO);
    glGenBuffers(1, &mIndexBO);
}

Geometry::~Geometry() {
    TRACE(getName().c_str());
}

bool Geometry::updateBufferObject() {
    if (!mMesh) {
        ALOGE("One Geometry must attach one Mesh");
        return false;
    }

    // Load vertex and index data into buffer object
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBO);
    glBufferData(GL_ARRAY_BUFFER, mMesh->getVertexBufSize(),
        mMesh->getVertexBuf(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mMesh->getIndexBufSize(),
        mMesh->getIndexBuf(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    return true;
}

void Geometry::update(double timeStamp) {
    // find the skeleton root
/* from assimp doc:
a) Create a map or a similar container to store which nodes are necessary for the skeleton. Pre-initialise it for all nodes with a "no".
b) For each bone in the mesh:
b1) Find the corresponding node in the scene's hierarchy by comparing their names.
b2) Mark this node as "yes" in the necessityMap.
b3) Mark all of its parents the same way until you 1) find the mesh's node or 2) the parent of the mesh's node.
c) Recursively iterate over the node hierarchy
c1) If the node is marked as necessary, copy it into the skeleton and check its children
c2) If the node is marked as not necessary, skip it and do not iterate over its children.
*/
}

void Geometry::draw(Render &render, shared_ptr<Scene> scene, double timeStamp) {
    NodeObj::updateAnimation(timeStamp);

    shared_ptr<Node> rootNode(scene->getRootNode());
    assert(rootNode);
    vector<Transform> boneTransforms;
    // do vertex skinning
    bool cpuBoneTransform = false;
    for (int i=0; i<mMesh->getNumBones(); i++) {
        shared_ptr<Bone> bone(mMesh->getBone(i));
        shared_ptr<NodeObj> boneNode(rootNode->getChild(bone->getName()));
        if (boneNode) {
            Transform nodeTransform = boneNode->getBoneTransform(timeStamp);
            boneTransforms.push_back(nodeTransform);
            bone->transform(mMesh, nodeTransform);
            cpuBoneTransform = true;
        } else {
            ALOGW("%-10s bone has no node in scene graph", bone->getName().c_str());
        }
    }

#if 0
    int boneTransformSize = boneTransforms.size();
    for (int i=0; i<boneTransformSize; i++) {
        boneTransforms[i].dump(Log::F_BONE, "bone transform %d/%d", i, boneTransformSize);
    }
#endif

    if (!mBOUpdated || cpuBoneTransform) {
        updateBufferObject();
        mBOUpdated = true;
    }

    if (render.drawGeometry(scene, dynamic_pointer_cast<Geometry>(shared_from_this())))
        // program attached to Geometry node only when drawGeometry returns true
        render.drawMesh(scene, mMesh, getProgram(), mVertexBO, mIndexBO);
}

std::shared_ptr<Mesh> Geometry::getMesh() {
    return mMesh;
}

} //namespace
