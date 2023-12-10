#include "MapManager.h"
#include <TextureManager.h>
#include <RenderSystem.h>
#include <CollisionSystem.h>
#include <ObjectManager.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <ArduinoJson-v6.19.4.h>

int MapManager::MAP_WIDTH = 16;
int MapManager::MAP_HEIGHT = 16;

void MapManager::init()
{
    //  create map tiles
    if(!load_map_tiles())
    {
        auto tex = TextureManager::instance().load_texture("texture/map-tiles.png");
        tile[0] = {tex, {0, 0, 64, 64}, {32, 32, 64, 64, false}};
        tile[1] = {tex, {0, 64, 64, 64}, {32, 32, 64, 64, true}};
        tile[2] = {tex, {0, 128, 64, 64}, {32, 32, 64, 64, true}};
    }
    // create collision areas
    for(int i = 0; i < MapManager::MAP_WIDTH; ++i)
        for(int j = 0; j < MapManager::MAP_HEIGHT; ++j)
        if(tile[map[i][j]].coll.barrier)
            {//          left up   mid   offs
                int x = (i * 64) + 32 - tile[map[i][j]].coll.xoffs;
                int y = (j * 64) + 32 - tile[map[i][j]].coll.yoffs;
                int width = tile[map[i][j]].coll.width;
                int height = tile[map[i][j]].coll.height;
                SDL_Rect collrect{x, y, width, height};
                auto id = ObjectManager::get_unique_id();
                CollisionSystem::instance().add(id, tile[map[i][j]].coll.xoffs, tile[map[i][j]].coll.yoffs, collrect, true);
            }
}

void MapManager::render()
{
    int x = 0;
    int y = 0;
    int xmax = 16;
    int ymax = 16;
    update_render_bounds(x, y, xmax,  ymax); // render only visible tiles
    for(int i = x; i < xmax; ++i)
        for(int j = y; j < ymax; ++j)
        {
            SDL_Rect dest{i * 64, j * 64, 64, 64};
            if(camera) camera->update_render(dest);
            SDL_RenderCopy(RenderSystem::renderer, tile[map[i][j]].tex, &tile[map[i][j]].src, &dest);
        }
}

void MapManager::set_camera(Camera *camera)
{
    this->camera = camera;
}

Camera *MapManager::get_camera()
{
    return this->camera;
}

void MapManager::update_render_bounds(int &imin, int &jmin, int &imax, int &jmax)
{
    if(!camera) return;
    imin = camera->view().x / 64;
    jmin = camera->view().y / 64;
    imax = std::min(((camera->view().x + camera->view().w) / 64) + 1, MapManager::MAP_WIDTH);
    jmax = std::min(((camera->view().y + camera->view().h) / 64) + 1, MapManager::MAP_HEIGHT);
}

bool MapManager::load_map_tiles()
{   
    StaticJsonDocument<2048> json;
    std::ifstream file("json/maptiles.json");
    if(!file.is_open())
    {
        std::cout << "Error opening maptiles.json\n";
        return false;
    }
    DeserializationError err = deserializeJson(json, file);
    if(err)
    {
        std::cout << "Error parsing json: " << err.c_str() << "\n";
        return false;
    }
    file.close();

    if(json.is<JsonArray>())
    {
        int index = 0;
        for (JsonVariant obj : json.as<JsonArray>())
        {
            auto tex = TextureManager::instance().load_texture(obj["texture"].as<const char*>());
            auto jsrc = obj["src"];
            SDL_Rect src{jsrc["x"].as<int>(), jsrc["y"].as<int>(), jsrc["w"].as<int>(), jsrc["h"].as<int>()};
            auto jcoll = obj["collision"];
            CollisionRect coll{jcoll["xoffs"].as<int>(), jcoll["yoffs"].as<int>(), jcoll["width"].as<int>(), jcoll["height"].as<int>(), jcoll["barrier"].as<bool>()};
            tile[index++] = {tex, src, coll};
        }
    }
    else
    {
        std::cout << "maptiles.json does not contains json array\n";
        return false;
    }
    std::cout << "maptiles.json loaded successfully\n";
    return true;
}

MapManager::~MapManager() {}
