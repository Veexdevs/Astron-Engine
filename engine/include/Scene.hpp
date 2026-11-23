#pragma once
#include "../core_c/astron_math.h"
#include "Json.hpp"
#include <string>
#include <vector>

namespace astron {

enum class MeshKind : int {
    Cube = 0,
    Sphere = 1,
    Cylinder = 2,
    Block = 3,
    Terrain = 4,
    Light = 5,
    Camera = 6
};

enum class MaterialKind : int {
    Plastic = 0,
    Metal = 1,
    Wood = 2,
    Glass = 3,
    Neon = 4,
    Matte = 5
};

struct SceneObject {
    std::string id;
    std::string name;
    MeshKind mesh = MeshKind::Cube;

    AstronVec3 position = { 0.0f, 0.5f, 0.0f };
    AstronVec3 rotation = { 0.0f, 0.0f, 0.0f };
    AstronVec3 scale    = { 1.0f, 1.0f, 1.0f };

    // Runtime play-mode state
    AstronVec3 velocity = { 0.0f, 0.0f, 0.0f };
    AstronVec3 savedPos = { 0.0f, 0.5f, 0.0f };

    std::string colorHex = "#FFFFFF";
    MaterialKind material = MaterialKind::Plastic;
    float transparency = 0.0f;
    bool visible = true;

    float mass = 1.0f;
    bool gravity = false;
    std::string collision = "Ativado";
    std::string tag = "Model";
    std::string scriptPath = "res://scripts/Player.cs";
};

class Scene {
public:
    std::string name = "Cena Principal";
    float worldGravity = 18.0f;
    std::vector<SceneObject> objects;
    int selectedIndex = 0;

    static Scene createDefault();

    SceneObject* selected();
    const SceneObject* selected() const;

    int addObject(const std::string& label, MeshKind kind, AstronVec3 pos = { 0.0f, 0.5f, 0.0f });
    void removeSelected();

    void beginPlay();
    void stopPlay();
    void stepPhysics(float dt, float moveX, float moveZ, bool jumpPressed);

    Json toJson() const;
    static Scene fromJson(const Json& j);

    static const char* materialName(MaterialKind m);
    static MaterialKind materialFromName(const std::string& s);
    static const char* meshName(MeshKind m);
    static MeshKind meshFromName(const std::string& s);
};

} // namespace astron
