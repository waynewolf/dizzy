#include <algorithm>
#include <sstream>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include "log.h"
#include "program.h"
#include "scene.h"
#include "render.h"
#include "mesh.h"
#include "scene_graph.h"

using namespace std;

namespace dzy {

int Node::mMonoCount = 0;

Node::Node()
    : mUseAutoProgram(true) {
    ostringstream os;
    os << "Node-" << mMonoCount++;
    mName = os.str();
}

Node::Node(const string& name)
    : mName(name)
    , mUseAutoProgram(true) {
    mMonoCount++;
}

void Node::attachChild(shared_ptr<Node> childNode) {
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
        shared_ptr<Node> oldParent(childNode->mParent.lock());
        childNode->mParent = shared_from_this();
        if (oldParent) {
            // remove childNode from oldParent
            oldParent->mChildren.erase(remove(
                oldParent->mChildren.begin(), oldParent->mChildren.end(), childNode),
                oldParent->mChildren.end());
        }
    }
}

shared_ptr<Node> Node::getParent() {
    return mParent.lock();
}

shared_ptr<Node> Node::findNode(const string &name) {
    if (mName == name) return shared_from_this();
    for (size_t i = 0; i < mChildren.size(); i++) {
        shared_ptr<Node> node(mChildren[i]->findNode(name));
        if (node) return node;
    }
    return NULL;
}

void Node::dfsTraversal(shared_ptr<Scene> scene, VisitFunc visit) {
    visit(scene, shared_from_this());
    std::for_each(mChildren.begin(), mChildren.end(), [&] (shared_ptr<Node> c) {
        c->dfsTraversal(scene, visit);
    });
}

bool Node::isAutoProgram() {
    return mUseAutoProgram;
}

void Node::setProgram(std::shared_ptr<Program> program) {
    mUseAutoProgram = false;
    mProgram = program;
}

shared_ptr<Program> Node::getProgram() {
    if (mUseAutoProgram) return ProgramManager::get()->getDefaultProgram();
    return mProgram;
}

bool Node::initGpuData() {
    std::for_each(mChildren.begin(), mChildren.end(), [&] (shared_ptr<Node> c) {
        c->initGpuData();
    });
    return true;
}

void Node::draw(Render &render, shared_ptr<Scene> scene) {
    render.drawNode(scene, shared_from_this());
    std::for_each(mChildren.begin(), mChildren.end(), [&] (shared_ptr<Node> c) {
        c->draw(render, scene);
    });
}

void Node::translate(float x, float y, float z) {
    glm::vec3 v(x, y, z);
    mTransformation = glm::translate(mTransformation, v);
}

void Node::scale(float x, float y, float z) {
    glm::vec3 v(x, y, z);
    mTransformation = glm::scale(mTransformation, v);
}

void Node::rotate(float radian, float axisX, float axisY, float axisZ) {
    glm::vec3 axis(axisX, axisY, axisZ);
    mTransformation = glm::rotate(mTransformation, radian, axis);
}

void Node::setName(string name) {
    mName = name;
}

string Node::getName() {
    return mName;
}

bool GeoNode::initGpuData() {
    if (!mMesh) {
        ALOGE("One GeoNode must attach one Mesh");
        return false;
    }

    // Load vertex and index data into buffer object
    glGenBuffers(1, &mVertexBO);
    glGenBuffers(1, &mIndexBO);
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

void GeoNode::draw(Render &render, shared_ptr<Scene> scene) {
    render.drawNode(scene, shared_from_this());
    render.drawMesh(scene, mMesh, getProgram(), mVertexBO, mIndexBO);
}

} //namespace
