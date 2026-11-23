#include "Scene.hpp"
#include <cmath>

namespace astron {

Scene Scene::createDefault() {
    Scene s;
    s.name = "Cena Principal";

    SceneObject model;
    model.id = "part-model";
    model.name = "Model";
    model.mesh = MeshKind::Cube;
    model.position = { 0.0f, 0.0f, 0.0f };
    model.rotation = { 0.0f, 0.0f, 0.0f };
    model.scale    = { 1.0f, 1.0f, 1.0f };
    model.colorHex = "#FFFFFF";
    model.material = MaterialKind::Plastic;
    model.transparency = 0.0f;
    model.visible = true;
    model.mass = 1.0f;
    model.gravity = true;
    model.collision = "Ativado";
    model.tag = "Player";
    model.scriptPath = "res://scripts/Player.cs";
    s.objects.push_back(model);

    s.selectedIndex = 0;
    return s;
}

SceneObject* Scene::selected() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(objects.size())) return nullptr;
    return &objects[static_cast<size_t>(selectedIndex)];
}

const SceneObject* Scene::selected() const {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(objects.size())) return nullptr;
    return &objects[static_cast<size_t>(selectedIndex)];
}

int Scene::addObject(const std::string& label, MeshKind kind, AstronVec3 pos) {
    SceneObject o;
    int num = static_cast<int>(objects.size()) + 1;
    o.id = "obj-" + std::to_string(num);
    o.name = label + " " + std::to_string(num);
    o.mesh = kind;
    o.position = pos;
    if (kind == MeshKind::Block) {
        o.scale = { 2.0f, 0.5f, 1.4f };
    } else if (kind == MeshKind::Terrain) {
        o.scale = { 12.0f, 0.25f, 12.0f };
        o.position = { 0.0f, -0.125f, 0.0f };
        o.colorHex = "#3D4654";
        o.gravity = false;
    } else {
        o.scale = { 1.0f, 1.0f, 1.0f };
    }
    objects.push_back(std::move(o));
    selectedIndex = static_cast<int>(objects.size()) - 1;
    return selectedIndex;
}

void Scene::removeSelected() {
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(objects.size())) return;
    objects.erase(objects.begin() + selectedIndex);
    if (objects.empty()) selectedIndex = -1;
    else if (selectedIndex >= static_cast<int>(objects.size())) selectedIndex = static_cast<int>(objects.size()) - 1;
}

void Scene::beginPlay() {
    for (auto& o : objects) {
        o.savedPos = o.position;
        o.velocity = { 0.0f, 0.0f, 0.0f };
    }
}

void Scene::stopPlay() {
    for (auto& o : objects) {
        o.position = o.savedPos;
        o.velocity = { 0.0f, 0.0f, 0.0f };
    }
}

void Scene::stepPhysics(float dt, float moveX, float moveZ, bool jumpPressed) {
    if (dt <= 0.0f) return;
    if (dt > 0.05f) dt = 0.05f;

    for (size_t i = 0; i < objects.size(); ++i) {
        auto& o = objects[i];
        if (!o.visible) continue;

        bool isPlayer = (i == 0 || o.tag == "Player");
        if (isPlayer) {
            o.velocity.x = moveX * 5.0f;
            o.velocity.z = moveZ * 5.0f;
        }

        if (o.gravity) {
            o.velocity.y -= worldGravity * dt;
        }

        o.position.x += o.velocity.x * dt;
        o.position.y += o.velocity.y * dt;
        o.position.z += o.velocity.z * dt;

        float halfH = o.scale.y * 0.5f;
        if (o.collision == "Ativado" && o.position.y < halfH) {
            o.position.y = halfH;
            if (o.velocity.y < 0.0f) o.velocity.y = 0.0f;
            if (isPlayer && jumpPressed) {
                o.velocity.y = 7.5f;
            }
        }
    }
}

const char* Scene::materialName(MaterialKind m) {
    switch (m) {
        case MaterialKind::Plastic: return "Plastic";
        case MaterialKind::Metal:   return "Metal";
        case MaterialKind::Wood:    return "Wood";
        case MaterialKind::Glass:   return "Glass";
        case MaterialKind::Neon:    return "Neon";
        case MaterialKind::Matte:   return "Matte";
    }
    return "Plastic";
}

MaterialKind Scene::materialFromName(const std::string& s) {
    if (s == "Metal") return MaterialKind::Metal;
    if (s == "Wood")  return MaterialKind::Wood;
    if (s == "Glass") return MaterialKind::Glass;
    if (s == "Neon")  return MaterialKind::Neon;
    if (s == "Matte") return MaterialKind::Matte;
    return MaterialKind::Plastic;
}

const char* Scene::meshName(MeshKind m) {
    switch (m) {
        case MeshKind::Cube:     return "Cube";
        case MeshKind::Sphere:   return "Sphere";
        case MeshKind::Cylinder: return "Cylinder";
        case MeshKind::Block:    return "Block";
        case MeshKind::Terrain:  return "Terrain";
        case MeshKind::Light:    return "Light";
        case MeshKind::Camera:   return "Camera";
    }
    return "Cube";
}

MeshKind Scene::meshFromName(const std::string& s) {
    if (s == "Sphere" || s == "Esfera")     return MeshKind::Sphere;
    if (s == "Cylinder" || s == "Cilindro") return MeshKind::Cylinder;
    if (s == "Block")                       return MeshKind::Block;
    if (s == "Terrain" || s == "Terreno")   return MeshKind::Terrain;
    if (s == "Light" || s == "Luz")         return MeshKind::Light;
    if (s == "Camera")                      return MeshKind::Camera;
    return MeshKind::Cube;
}

Json Scene::toJson() const {
    Json root = Json::object();
    root["name"] = name;
    root["engine"] = "Astron Engine 1.0.0";
    root["gravity"] = static_cast<double>(worldGravity);

    Json nodes = Json::array();
    for (const auto& o : objects) {
        Json n = Json::object();
        n["id"] = o.id;
        n["name"] = o.name;
        n["mesh"] = meshName(o.mesh);

        Json pos = Json::array();
        pos.push_back(static_cast<double>(o.position.x));
        pos.push_back(static_cast<double>(o.position.y));
        pos.push_back(static_cast<double>(o.position.z));
        n["position"] = std::move(pos);

        Json rot = Json::array();
        rot.push_back(static_cast<double>(o.rotation.x));
        rot.push_back(static_cast<double>(o.rotation.y));
        rot.push_back(static_cast<double>(o.rotation.z));
        n["rotation"] = std::move(rot);

        Json scl = Json::array();
        scl.push_back(static_cast<double>(o.scale.x));
        scl.push_back(static_cast<double>(o.scale.y));
        scl.push_back(static_cast<double>(o.scale.z));
        n["scale"] = std::move(scl);

        n["color"] = o.colorHex;
        n["material"] = materialName(o.material);
        n["transparency"] = static_cast<double>(o.transparency);
        n["visible"] = o.visible;
        n["mass"] = static_cast<double>(o.mass);
        n["gravity"] = o.gravity;
        n["collision"] = o.collision;
        n["tag"] = o.tag;
        n["script"] = o.scriptPath;
        nodes.push_back(std::move(n));
    }
    root["nodes"] = std::move(nodes);
    return root;
}

Scene Scene::fromJson(const Json& j) {
    if (!j.isObject()) return createDefault();
    Scene s;
    s.name = j["name"].strOr("Cena Principal");
    s.worldGravity = static_cast<float>(j["gravity"].num(18.0));

    const Json& nodes = j["nodes"];
    if (nodes.isArray()) {
        for (const auto& n : nodes.asArray()) {
            SceneObject o;
            o.id = n["id"].strOr("node");
            o.name = n["name"].strOr("Model");
            o.mesh = meshFromName(n["mesh"].strOr("Cube"));
            if (n["position"].isArray() && n["position"].size() >= 3) {
                o.position.x = static_cast<float>(n["position"].at(0).num(0.0));
                o.position.y = static_cast<float>(n["position"].at(1).num(0.0));
                o.position.z = static_cast<float>(n["position"].at(2).num(0.0));
            }
            if (n["rotation"].isArray() && n["rotation"].size() >= 3) {
                o.rotation.x = static_cast<float>(n["rotation"].at(0).num(0.0));
                o.rotation.y = static_cast<float>(n["rotation"].at(1).num(0.0));
                o.rotation.z = static_cast<float>(n["rotation"].at(2).num(0.0));
            }
            if (n["scale"].isArray() && n["scale"].size() >= 3) {
                o.scale.x = static_cast<float>(n["scale"].at(0).num(1.0));
                o.scale.y = static_cast<float>(n["scale"].at(1).num(1.0));
                o.scale.z = static_cast<float>(n["scale"].at(2).num(1.0));
            }
            o.colorHex = n["color"].strOr("#FFFFFF");
            o.material = materialFromName(n["material"].strOr("Plastic"));
            o.transparency = static_cast<float>(n["transparency"].num(0.0));
            o.visible = n["visible"].boolean(true);
            o.mass = static_cast<float>(n["mass"].num(1.0));
            o.gravity = n["gravity"].boolean(true);
            o.collision = n["collision"].strOr("Ativado");
            o.tag = n["tag"].strOr("Model");
            o.scriptPath = n["script"].strOr("res://scripts/Player.cs");
            s.objects.push_back(std::move(o));
        }
    }
    if (s.objects.empty()) return createDefault();
    s.selectedIndex = 0;
    return s;
}

} // namespace astron
