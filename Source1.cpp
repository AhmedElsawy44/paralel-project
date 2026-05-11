#include <iostream>
#include <fstream>
#include <vector>
#include <cstdlib>
#include <ctime>

using namespace std;

int main() {

    srand(time(0));

    // 1. Generate Data for Heat Diffusion (e.g., 1000 x 1000 grid)
    int heat_rows = 1000, heat_cols = 1000;
    ofstream heat_file("heat_grid.txt");
    heat_file << heat_rows << " " << heat_cols << "\n";
    for (int i = 0; i < heat_rows * heat_cols; ++i) {
        // Random double values between 0.0 and 100.0
        heat_file << (rand() % 10000) / 100.0 << " ";
        if ((i + 1) % heat_cols == 0) heat_file << "\n";
    }
    heat_file.close();
    cout << "Generated heat_grid.txt (1000x1000)\n";

    // 2. Generate Data for Game of Life (e.g., 1000 x 1000 grid of 0s and 1s)
    int gol_rows = 1000, gol_cols = 1000;
    ofstream gol_file("gol_grid.txt");
    gol_file << gol_rows << " " << gol_cols << "\n";
    for (int i = 0; i < gol_rows * gol_cols; ++i) {
        // Random 0 or 1
        gol_file << rand() % 2 << " ";
        if ((i + 1) % gol_cols == 0) gol_file << "\n";
    }
    gol_file.close();
    cout << "Generated gol_grid.txt (1000x1000)\n";

    // 3. Generate Big Data for Category B (e.g., 1,000,000 elements)
    int large_n = 1000000;
    ofstream arr_file("large_array.txt");
    arr_file << large_n << "\n";
    for (int i = 0; i < large_n; ++i) {
        arr_file << rand() % 1000000 << "\n";
    }
    arr_file.close();
    cout << "Generated large_array.txt (1,000,000 elements)\n";

    return 0;

}