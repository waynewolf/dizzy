#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <GLES3/gl3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include "nameobj.h"

namespace dzy {

class Program;
class Render;
class Scene;
class Mesh;
class Node;
class Material;
class Camera;
class Light;
/// Base class for "element" in the scene graph
class NodeObj : public NameObj {
public:
    NodeObj(const std::string& name);
    virtual ~NodeObj();

    void resetTransform();
    void translate(float x, float y, float z);
    void scale(float x, float y, float z);
    void rotate(float radian, float axisX, float axisY, float axisZ);

    /// get parent of this node
    ///
    ///     @return the parent of this node, null if this is the root node
    std::shared_ptr<Node> getParent();

    /// set parent of this node
    ///
    ///     simply set parent of the current node, will not
    ///     add this node to the new parent's children list
    void setParent(std::shared_ptr<Node> parent);

    /// check if the node use auto program
    bool isAutoProgram();

    /// set the program that is attached to this node
    ///
    ///     @param program the program to be attached to this node
    void setProgram(std::shared_ptr<Program> program);

    /// get the program that is attached to this node
    ///
    ///     @return the current program attached to this node
    std::shared_ptr<Program> getProgram();

    /// get the current program attached to this node
    ///
    ///     if using auto program, the material and mesh are used to
    ///     generate program
    ///
    ///     @param material the material used to generate program
    ///     @param hasLight the scene has light
    ///     @param mesh the mesh used to generate program
    ///     @return the current program attached to this node
    std::shared_ptr<Program> getProgram(
        std::shared_ptr<Material> material,
        bool hasLight,
        std::shared_ptr<Mesh> mesh);

    void setMaterial(std::shared_ptr<Material> material);
    std::shared_ptr<Material> getMaterial();

    // TODO: support multiple lights
    void setLight(std::shared_ptr<Light> light);
    std::shared_ptr<Light> getLight();

    void setCamera(std::shared_ptr<Camera> camera);
    std::shared_ptr<Camera> getCamera();


    /// one time initialization of the node
    virtual bool initGpuData() = 0;

    /// Recursively draw the node and it's children
    virtual void draw(Render &render, std::shared_ptr<Scene> scene) = 0;

    friend class AIAdapter;
    friend class Render;

protected:
    glm::mat4                               mTransformation;
    std::weak_ptr<Node>                     mParent;
    bool                                    mUseAutoProgram;
    std::shared_ptr<Program>                mProgram;
    std::shared_ptr<Material>               mMaterial;
    std::shared_ptr<Light>                  mLight;
    std::shared_ptr<Camera>                 mCamera;
    static int                              mMonoCount;
};

class Node : public NodeObj, public std::enable_shared_from_this<Node> {
public:
    typedef std::function<void(std::shared_ptr<Scene>, std::shared_ptr<NodeObj>)> VisitSceneFunc;
    typedef std::function<void(std::shared_ptr<NodeObj>)> VisitFunc;

    Node(const std::string& name);

    /// attach a child node into the scene graph
    ///
    ///     @param childNode the child node to attach
    void attachChild(std::shared_ptr<NodeObj> childNode);

    /// returns a child at a given index
    std::shared_ptr<NodeObj> getChild(int idx);

    /// returns a child match the given name, search recursively from current node
    std::shared_ptr<NodeObj> getChild(const std::string &name);

    /// depth first traversal of the scene graph(tree)
    ///
    ///     function template to do dfs traversal, starting from the current node,
    ///     not from the root of the whole scene graph
    ///
    ///     @param scene the scene that hosts the scene graph
    ///     @param visit the functor gets called whenever a node is visited
    void depthFirstTraversal(std::shared_ptr<Scene> scene, VisitSceneFunc visit);

    /// depth first traversal of the scene graph(tree)
    ///
    ///     function template to do dfs traversal, starting from the current node,
    ///     not from the root of the whole scene graph
    ///
    ///     @param visit the functor gets called whenever a node is visited
    void depthFirstTraversal(VisitFunc visit);

    virtual bool initGpuData();
    virtual void draw(Render &render, std::shared_ptr<Scene> scene);

    /// dump the scene graph hierarchy starting from the current node.
    void dumpHierarchy();

protected:
    std::vector<std::shared_ptr<NodeObj> >  mChildren;
};

class Geometry : public NodeObj, public std::enable_shared_from_this<Geometry> {
public:
    Geometry(std::shared_ptr<Mesh> mesh);
    Geometry(const std::string& name, std::shared_ptr<Mesh> mesh);

    virtual bool initGpuData();
    virtual void draw(Render &render, std::shared_ptr<Scene> scene);

    std::shared_ptr<Mesh> getMesh();

protected:
    // one on one mapping between Geometry and Mesh
    std::shared_ptr<Mesh>       mMesh;
    // one vertex and index buffer object per Geometry
    // logically BO handles should be put in Mesh class,
    // but I prefer not to have Mesh class depend on gl
    GLuint                      mVertexBO;
    GLuint                      mIndexBO;
};

}

#endif
