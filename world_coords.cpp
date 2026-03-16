#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>

#include "pixy.h"

static const int TARGET_SIGNATURE = 2;

static bool getBestBlockXY(const Block* blocks, int count, int& x, int& y)
{
  int best_i = -1;
  int best_area = 0;

  for (int i = 0; i < count; i++)
  {
    if ((int)blocks[i].signature == TARGET_SIGNATURE)
    {
      int area = (int)blocks[i].width * (int)blocks[i].height;
      if (area > best_area)
      {
        best_area = area;
        best_i = i;
      }
    }
  }

  if (best_i < 0)
  {
    return false;
  }

  x = (int)blocks[best_i].x;
  y = (int)blocks[best_i].y;
  return true;
}

static bool loadHomography(const std::string& fileName, double H[3][3])
{
  std::ifstream in(fileName);
  if (!in)
  {
    return false;
  }

  for (int r = 0; r < 3; r++)
  {
    for (int c = 0; c < 3; c++)
    {
      if (!(in >> H[r][c]))
      {
        return false;
      }
    }
  }

  return true;
}

static bool applyHomography(const double H[3][3],
                            double x,
                            double y,
                            double& X,
                            double& Y)
{
  double denom = H[2][0] * x + H[2][1] * y + H[2][2];

  if (std::fabs(denom) < 1e-12)
  {
    return false;
  }

  X = (H[0][0] * x + H[0][1] * y + H[0][2]) / denom;
  Y = (H[1][0] * x + H[1][1] * y + H[1][2]) / denom;

  return true;
}

int main()
{
  double H[3][3];

  if (!loadHomography("homography.txt", H))
  {
    std::cerr << "Could not load homography.txt\n";
    return 1;
  }

  std::cout << "Loaded homography.txt\n";
  std::cout << "Initializing Pixy...\n";

  if (pixy_init() != 0)
  {
    std::cerr << "Pixy init failed\n";
    return 1;
  }

  Block blocks[100];

  std::cout << "Output started. Ctrl+C to quit.\n";

  while (true)
  {
    int count = pixy_get_blocks(50, blocks);

    int x;
    int y;

    if (count > 0 && getBestBlockXY(blocks, count, x, y))
    {
      double Xw;
      double Yw;

      if (applyHomography(H, x, y, Xw, Yw))
      {
        std::cout << "pixel=(" << x << "," << y << ")"
                  << "  world=(" << Xw << "," << Yw << ")\n";
      }
      else
      {
        std::cout << "Homography failed for pixel=(" << x << "," << y << ")\n";
      }
    }
    else
    {
      std::cout << "No target detected\n";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  return 0;
}
