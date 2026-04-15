#include "pixy_world.h"
#include <pixy.h>
#include <fstream>
#include <cmath>

static double H_[3][3];
static bool homographyLoaded_ = false;

static bool loadHomography(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            if (!(in >> H_[r][c])) return false;
        }
    }
    return true;
}

static bool applyHomography(double x, double y, double& X, double& Y) {
    double denom = H_[2][0] * x + H_[2][1] * y + H_[2][2];
    if (std::fabs(denom) < 1e-12) return false;

    X = (H_[0][0] * x + H_[0][1] * y + H_[0][2]) / denom;
    Y = (H_[1][0] * x + H_[1][1] * y + H_[1][2]) / denom;
    return true;
}

static bool getBestPixyBlock(int& px, int& py) {
    Block blocks[100];
    int count = pixy_get_blocks(50, blocks);
    if (count <= 0) return false;

    int best = -1;
    int bestArea = 0;

    for (int i = 0; i < count; ++i) {
        if (blocks[i].signature != 1) continue;

        int area = blocks[i].width * blocks[i].height;
        if (area > bestArea) {
            bestArea = area;
            best = i;
        }
    }

    if (best < 0) return false;

    px = blocks[best].x;
    py = blocks[best].y;
    return true;
}

bool initPixyWorld(const std::string& homographyPath) {
    if (pixy_init() != 0) return false;
    homographyLoaded_ = loadHomography(homographyPath);
    return homographyLoaded_;
}

bool updateWorldPuck(double& outX, double& outY) {
    if (!homographyLoaded_) return false;

    int px, py;
    if (!getBestPixyBlock(px, py)) return false;

    return applyHomography(px, py, outX, outY);
}
