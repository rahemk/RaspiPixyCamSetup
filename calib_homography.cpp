#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <thread>
#include <chrono>
#include <cmath>

#include "pixy.h"

static const int TARGET_SIGNATURE = 2; //Specify Signature

struct Pt2
{
  double x;
  double y;
};

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

static bool solveLinearSystem(std::vector<std::vector<double>>& A,
                              std::vector<double>& b,
                              std::vector<double>& x)
{
  int n = (int)A.size();
  x.assign(n, 0.0);

  for (int col = 0; col < n; col++)
  {
    int pivot = col;
    double best = std::fabs(A[col][col]);

    for (int r = col + 1; r < n; r++)
    {
      double v = std::fabs(A[r][col]);
      if (v > best)
      {
        best = v;
        pivot = r;
      }
    }

    if (best < 1e-12)
    {
      return false;
    }

    if (pivot != col)
    {
      std::swap(A[pivot], A[col]);
      std::swap(b[pivot], b[col]);
    }

    double div = A[col][col];
    for (int c = col; c < n; c++)
    {
      A[col][c] /= div;
    }
    b[col] /= div;

    for (int r = 0; r < n; r++)
    {
      if (r == col)
      {
        continue;
      }

      double factor = A[r][col];
      if (std::fabs(factor) < 1e-12)
      {
        continue;
      }

      for (int c = col; c < n; c++)
      {
        A[r][c] -= factor * A[col][c];
      }
      b[r] -= factor * b[col];
    }
  }

  for (int i = 0; i < n; i++)
  {
    x[i] = b[i];
  }

  return true;
}

static bool computeHomography4(const std::vector<Pt2>& img,
                               const std::vector<Pt2>& world,
                               double H[3][3])
{
  if (img.size() != 4 || world.size() != 4)
  {
    return false;
  }

  std::vector<std::vector<double>> A(8, std::vector<double>(8, 0.0));
  std::vector<double> b(8, 0.0);

  for (int i = 0; i < 4; i++)
  {
    double x = img[i].x;
    double y = img[i].y;
    double X = world[i].x;
    double Y = world[i].y;

    int r0 = 2 * i;
    A[r0][0] = x;
    A[r0][1] = y;
    A[r0][2] = 1.0;
    A[r0][6] = -X * x;
    A[r0][7] = -X * y;
    b[r0] = X;

    int r1 = 2 * i + 1;
    A[r1][3] = x;
    A[r1][4] = y;
    A[r1][5] = 1.0;
    A[r1][6] = -Y * x;
    A[r1][7] = -Y * y;
    b[r1] = Y;
  }

  std::vector<double> h;
  if (!solveLinearSystem(A, b, h))
  {
    return false;
  }

  H[0][0] = h[0];
  H[0][1] = h[1];
  H[0][2] = h[2];
  H[1][0] = h[3];
  H[1][1] = h[4];
  H[1][2] = h[5];
  H[2][0] = h[6];
  H[2][1] = h[7];
  H[2][2] = 1.0;

  return true;
}

int main()
{
  std::cout << "Pixy init...\n";
  if (pixy_init() != 0)
  {
    std::cerr << "Pixy init failed\n";
    return 1;
  }

  Block blocks[100];
  std::vector<Pt2> imgPts;
  std::vector<Pt2> worldPts;

  std::cout << "\n[INSTRUCTIONS]\n";
  std::cout << "Calib will collect 4 point pairs.\n";
  std::cout << "For each point:\n";
  std::cout << "  1) Place puck on a known location\n";
  std::cout << "  2) Press ENTER to capture averaged Pixy image coordinates\n";
  std::cout << "  3) Enter world X Y\n\n";

  for (int p = 0; p < 4; p++)
  {
    std::string line;
    std::cout << "Point " << (p + 1) << "/4: press ENTER to capture...";
    std::getline(std::cin, line);

    std::vector<std::pair<int, int>> samples;

    for (int k = 0; k < 10; k++)
    {
      int count = pixy_get_blocks(50, blocks);
      int x;
      int y;

      if (count > 0 && getBestBlockXY(blocks, count, x, y))
      {
        samples.push_back({x, y});
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if ((int)samples.size() < 5)
    {
      std::cout << "Not enough detections. Try again.\n";
      p--;
      continue;
    }

    double sx = 0.0;
    double sy = 0.0;

    for (const auto& s : samples)
    {
      sx += s.first;
      sy += s.second;
    }

    double mx = std::lround(sx / samples.size());
    double my = std::lround(sy / samples.size());

    std::cout << "[CAPTURED] image (x,y)=(" << mx << "," << my << ")\n";
    std::cout << "Enter world X Y: ";

    std::getline(std::cin, line);
    std::stringstream ss(line);

    double X;
    double Y;

    if (!(ss >> X >> Y))
    {
      std::cout << "Bad input. Use: number number\n";
      p--;
      continue;
    }

    imgPts.push_back({mx, my});
    worldPts.push_back({X, Y});

    std::cout << "[PAIR] (" << mx << "," << my << ") -> (" << X << "," << Y << ")\n\n";
  }

  double H[3][3];

  if (!computeHomography4(imgPts, worldPts, H))
  {
    std::cerr << "Failed to compute homography\n";
    return 1;
  }

  std::ofstream out("homography.txt");
  if (!out)
  {
    std::cerr << "Could not open homography.txt for writing\n";
    return 1;
  }

  for (int r = 0; r < 3; r++)
  {
    out << H[r][0] << " " << H[r][1] << " " << H[r][2] << "\n";
  }

  out.close();

  std::cout << "Saved homography.txt\n";

  return 0;
}
