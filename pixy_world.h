#pragma once
#include <utility>
#include <fstream>
#include <string>
#include <cstdio>


bool initPixyWorld(const std::string& homographyPath);
bool updateWorldPuck(double& outX, double& outY);
