#include <mpi.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;

// ==========================================
// 1. خوارزمية انتشار الحرارة (Category A - Blocking)
// ==========================================
void runHeatDiffusion(int rank, int size) {
    int rows = 0, cols = 0;
    int success_flag = 1;
    vector<double> global_grid;

    if (rank == 0) {
        cout << "\n--- Starting Heat Diffusion ---" << endl;
        ifstream infile("heat_grid.txt");
        if (!infile) {
            cerr << "Error: Cannot open heat_grid.txt!" << endl;
            success_flag = 0;
        }
        else {
            infile >> rows >> cols;
            global_grid.resize(rows * cols);
            for (int i = 0; i < rows * cols; ++i) infile >> global_grid[i];
            infile.close();
        }
    }

    MPI_Bcast(&success_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (success_flag == 0) return;

    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> sendcounts(size), displs(size);
    int sum = 0, base_rows = rows / size, extra_rows = rows % size;
    for (int i = 0; i < size; ++i) {
        int local_rows = base_rows + (i < extra_rows ? 1 : 0);
        sendcounts[i] = local_rows * cols;
        displs[i] = sum;
        sum += sendcounts[i];
    }

    int my_local_rows = sendcounts[rank] / cols;
    vector<double> local_grid((my_local_rows + 2) * cols, 0.0);
    vector<double> next_grid = local_grid;

    MPI_Scatterv(global_grid.data(), sendcounts.data(), displs.data(), MPI_DOUBLE,
        local_grid.data() + cols, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (int iter = 0; iter < 100; ++iter) {
        // ==========================================================
        // [DEADLOCK CORRECTED SOLUTION]
        // Deadlock Scenario: If all processes call MPI_Send first, they might block indefinitely.
        // Solution: Odd-Even Phase Ordering.
        // ==========================================================
        if (rank % 2 == 0) {
            if (rank != size - 1) {
                MPI_Send(local_grid.data() + my_local_rows * cols, cols, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD);
                MPI_Recv(local_grid.data() + (my_local_rows + 1) * cols, cols, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
            if (rank != 0) {
                MPI_Send(local_grid.data() + cols, cols, MPI_DOUBLE, rank - 1, 2, MPI_COMM_WORLD);
                MPI_Recv(local_grid.data(), cols, MPI_DOUBLE, rank - 1, 3, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
        else {
            if (rank != 0) {
                MPI_Recv(local_grid.data(), cols, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(local_grid.data() + cols, cols, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD);
            }
            if (rank != size - 1) {
                MPI_Recv(local_grid.data() + (my_local_rows + 1) * cols, cols, MPI_DOUBLE, rank + 1, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(local_grid.data() + my_local_rows * cols, cols, MPI_DOUBLE, rank + 1, 3, MPI_COMM_WORLD);
            }
        }

        for (int i = 1; i <= my_local_rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                double current = local_grid[i * cols + j];
                double top = local_grid[(i - 1) * cols + j], bottom = local_grid[(i + 1) * cols + j];
                double left = (j == 0) ? current : local_grid[i * cols + (j - 1)];
                double right = (j == cols - 1) ? current : local_grid[i * cols + (j + 1)];
                next_grid[i * cols + j] = current + 0.1 * (top + bottom + left + right - 4 * current);
            }
        }
        local_grid = next_grid;
    }

    MPI_Gatherv(local_grid.data() + cols, sendcounts[rank], MPI_DOUBLE, global_grid.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        ofstream outfile("heat_result.txt");
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) outfile << global_grid[i * cols + j] << " ";
            outfile << "\n";
        }
        outfile.close();
        cout << "Success: Saved to 'heat_result.txt'" << endl;
    }
}

// ==========================================
// 2. خوارزمية لعبة الحياة (Category A - Non-Blocking)
// ==========================================
void runGameOfLife(int rank, int size) {
    int rows = 0, cols = 0;
    int success_flag = 1;
    vector<double> global_grid;

    if (rank == 0) {
        cout << "\n--- Starting Game of Life ---" << endl;
        ifstream infile("gol_grid.txt");
        if (!infile) {
            cerr << "Error: Cannot open gol_grid.txt!" << endl;
            success_flag = 0;
        }
        else {
            infile >> rows >> cols;
            global_grid.resize(rows * cols);
            for (int i = 0; i < rows * cols; ++i) {
                infile >> global_grid[i];
            }
            infile.close();
        }
    }

    MPI_Bcast(&success_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (success_flag == 0) return;

    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> sendcounts(size), displs(size);
    int sum = 0, base_rows = rows / size, extra_rows = rows % size;
    for (int i = 0; i < size; ++i) {
        sendcounts[i] = (base_rows + (i < extra_rows ? 1 : 0)) * cols;
        displs[i] = sum;
        sum += sendcounts[i];
    }

    int my_local_rows = (sendcounts[rank] / cols);
    vector<double> local_grid((my_local_rows + 2) * cols, 0.0);
    vector<double> next_grid = local_grid;

    MPI_Scatterv(global_grid.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, local_grid.data() + cols, sendcounts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);

    for (int gen = 0; gen < 50; ++gen) {
        MPI_Request reqs[4]; int r_cnt = 0;
        if (rank != 0) {
            MPI_Isend(local_grid.data() + cols, cols, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &reqs[r_cnt++]);
            MPI_Irecv(local_grid.data(), cols, MPI_DOUBLE, rank - 1, 1, MPI_COMM_WORLD, &reqs[r_cnt++]);
        }
        if (rank != size - 1) {
            MPI_Isend(local_grid.data() + my_local_rows * cols, cols, MPI_DOUBLE, rank + 1, 1, MPI_COMM_WORLD, &reqs[r_cnt++]);
            MPI_Irecv(local_grid.data() + (my_local_rows + 1) * cols, cols, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &reqs[r_cnt++]);
        }
        MPI_Waitall(r_cnt, reqs, MPI_STATUSES_IGNORE);

        for (int i = 1; i <= my_local_rows; ++i) {
            for (int j = 0; j < cols; ++j) {
                int neighbors = 0;
                for (int di = -1; di <= 1; ++di) {
                    for (int dj = -1; dj <= 1; ++dj) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di;
                        int nj = j + dj;
                        if (nj >= 0 && nj < cols) {
                            neighbors += (int)local_grid[ni * cols + nj];
                        }
                    }
                }

                if (local_grid[i * cols + j] == 1 && (neighbors == 2 || neighbors == 3)) next_grid[i * cols + j] = 1;
                else if (local_grid[i * cols + j] == 0 && neighbors == 3) next_grid[i * cols + j] = 1;
                else next_grid[i * cols + j] = 0;
            }
        }
        local_grid = next_grid;
    }

    MPI_Gatherv(local_grid.data() + cols, sendcounts[rank], MPI_DOUBLE, global_grid.data(), sendcounts.data(), displs.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        ofstream outfile("game_of_life_result.txt");
        for (int i = 0; i < rows; ++i) {
            for (int j = 0; j < cols; ++j) outfile << (int)global_grid[i * cols + j] << " ";
            outfile << "\n";
        }
        outfile.close();
        cout << "Success: Saved to 'game_of_life_result.txt'" << endl;
    }
}

// ==========================================
// 3. خوارزمية الترتيب (Category B - Odd-Even)
// ==========================================
void runOddEvenSort(int rank, int size) {
    int n = 0;
    int success_flag = 1;
    vector<double> global_arr;

    if (rank == 0) {
        cout << "\n--- Starting Odd-Even Sort (Big Data) ---" << endl;
        ifstream infile("large_array.txt");
        if (!infile) {
            cerr << "Error: Cannot open large_array.txt!" << endl;
            success_flag = 0;
        }
        else {
            infile >> n;
            global_arr.resize(n);
            for (int i = 0; i < n; i++) infile >> global_arr[i];
            infile.close();
        }
    }

    MPI_Bcast(&success_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (success_flag == 0) return;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> scnts(size), dpls(size);
    int sum = 0, base = n / size, extra = n % size;
    for (int i = 0; i < size; ++i) {
        scnts[i] = base + (i < extra ? 1 : 0);
        dpls[i] = sum; sum += scnts[i];
    }

    vector<double> local_arr(scnts[rank]);
    MPI_Scatterv(global_arr.data(), scnts.data(), dpls.data(), MPI_DOUBLE, local_arr.data(), scnts[rank], MPI_DOUBLE, 0, MPI_COMM_WORLD);
    sort(local_arr.begin(), local_arr.end());

    for (int p = 0; p < size; p++) {
        int partner = (p % 2 == 0) ? (rank % 2 == 0 ? rank + 1 : rank - 1) : (rank % 2 != 0 ? rank + 1 : rank - 1);
        if (partner >= 0 && partner < size) {
            int p_size;
            // ==========================================================
            // [DEADLOCK CORRECTED SOLUTION]
            // Solution: Use MPI_Sendrecv.
            // ==========================================================
            MPI_Sendrecv(&scnts[rank], 1, MPI_INT, partner, 0, &p_size, 1, MPI_INT, partner, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            vector<double> p_arr(p_size);
            MPI_Sendrecv(local_arr.data(), scnts[rank], MPI_DOUBLE, partner, 1, p_arr.data(), p_size, MPI_DOUBLE, partner, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            vector<double> m(scnts[rank] + p_size);
            merge(local_arr.begin(), local_arr.end(), p_arr.begin(), p_arr.end(), m.begin());
            if (rank < partner) copy(m.begin(), m.begin() + scnts[rank], local_arr.begin());
            else copy(m.begin() + p_size, m.end(), local_arr.begin());
        }
    }

    MPI_Gatherv(local_arr.data(), scnts[rank], MPI_DOUBLE, global_arr.data(), scnts.data(), dpls.data(), MPI_DOUBLE, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        ofstream outfile("sorted_array_result.txt");
        outfile << "Top 100 elements sorted from " << n << " elements:\n";
        for (int i = 0; i < min(n, 100); i++) outfile << global_arr[i] << "\n";
        outfile.close();
        cout << "Success: Saved first 100 sorted elements to 'sorted_array_result.txt'" << endl;
    }
}

// ==========================================
// 4. خوارزمية Prefix Sum (Category B - Comm Split)
// ==========================================
void runPrefixSum(int rank, int size) {
    int n = 0;
    int success_flag = 1;
    vector<int> global_data;

    if (rank == 0) {
        cout << "\n--- Starting Prefix Sum (Big Data) ---" << endl;
        ifstream infile("large_array.txt");
        if (!infile) {
            cerr << "Error: Cannot open large_array.txt!" << endl;
            success_flag = 0;
        }
        else {
            infile >> n;
            global_data.resize(n);
            for (int i = 0; i < n; i++) {
                double temp; infile >> temp; // reading double then casting
                global_data[i] = (int)temp;
            }
            infile.close();
        }
    }

    MPI_Bcast(&success_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (success_flag == 0) return;
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Comm sub;
    MPI_Comm_split(MPI_COMM_WORLD, rank % 2, rank, &sub);

    vector<int> scnts(size), dpls(size);
    int sum = 0, base = n / size, extra = n % size;
    for (int i = 0; i < size; ++i) {
        scnts[i] = base + (i < extra ? 1 : 0);
        dpls[i] = sum; sum += scnts[i];
    }

    vector<int> local_data(scnts[rank]);
    MPI_Scatterv(global_data.data(), scnts.data(), dpls.data(), MPI_INT, local_data.data(), scnts[rank], MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> local_prefix(scnts[rank]);
    local_prefix[0] = local_data[0];
    for (int i = 1; i < scnts[rank]; i++) {
        local_prefix[i] = local_prefix[i - 1] + local_data[i];
    }

    int offset = 0;
    int last_val = local_prefix[scnts[rank] - 1];
    MPI_Exscan(&last_val, &offset, 1, MPI_INT, MPI_SUM, sub);

    int sub_rank; MPI_Comm_rank(sub, &sub_rank);
    if (sub_rank == 0) offset = 0;

    for (int i = 0; i < scnts[rank]; i++) {
        local_prefix[i] += offset;
    }

    vector<int> global_prefix;
    if (rank == 0) global_prefix.resize(n);
    MPI_Gatherv(local_prefix.data(), scnts[rank], MPI_INT, global_prefix.data(), scnts.data(), dpls.data(), MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        ofstream outfile("prefix_sum_result.txt");
        outfile << "Top 100 Prefix Sum results from " << n << " elements:\n";
        for (int i = 0; i < min(n, 100); i++) outfile << global_prefix[i] << " ";
        outfile << "\n";
        outfile.close();
        cout << "Success: Saved first 100 results to 'prefix_sum_result.txt'" << endl;
    }
    MPI_Comm_free(&sub);
}

// ==========================================
// 5. خوارزمية انتشار الحرارة (Bonus - 2D Decomposition)
// ==========================================
void runHeatDiffusion2D(int rank, int size) {
    int rows = 0, cols = 0;
    int success_flag = 1;
    vector<double> global_grid;

    if (rank == 0) {
        cout << "\n--- Starting Heat Diffusion (2D Decomposition Bonus) ---" << endl;
        ifstream infile("heat_grid.txt");
        if (!infile) {
            cerr << "Error: Cannot open heat_grid.txt!" << endl;
            success_flag = 0;
        }
        else {
            infile >> rows >> cols;
            global_grid.resize(rows * cols);
            for (int i = 0; i < rows * cols; ++i) infile >> global_grid[i];
            infile.close();
        }
    }

    MPI_Bcast(&success_flag, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (success_flag == 0) return;

    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int dims[2] = { 0, 0 };
    MPI_Dims_create(size, 2, dims);
    int periods[2] = { 0, 0 };
    MPI_Comm cart_comm;
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 0, &cart_comm);

    int coords[2];
    MPI_Cart_coords(cart_comm, rank, 2, coords);

    int north, south, east, west;
    MPI_Cart_shift(cart_comm, 0, 1, &north, &south);
    MPI_Cart_shift(cart_comm, 1, 1, &west, &east);

    int local_rows = rows / dims[0];
    int local_cols = cols / dims[1];

    int padded_rows = local_rows + 2;
    int padded_cols = local_cols + 2;
    vector<double> local_grid(padded_rows * padded_cols, 0.0);
    vector<double> next_grid = local_grid;

    if (rank == 0) {
        for (int p = 0; p < size; ++p) {
            int p_coords[2];
            MPI_Cart_coords(cart_comm, p, 2, p_coords);
            int start_row = p_coords[0] * local_rows;
            int start_col = p_coords[1] * local_cols;

            vector<double> block(local_rows * local_cols);
            for (int r = 0; r < local_rows; ++r) {
                for (int c = 0; c < local_cols; ++c) {
                    block[r * local_cols + c] = global_grid[(start_row + r) * cols + (start_col + c)];
                }
            }
            if (p == 0) {
                for (int r = 0; r < local_rows; ++r) {
                    for (int c = 0; c < local_cols; ++c) {
                        local_grid[(r + 1) * padded_cols + (c + 1)] = block[r * local_cols + c];
                    }
                }
            }
            else {
                MPI_Send(block.data(), local_rows * local_cols, MPI_DOUBLE, p, 0, MPI_COMM_WORLD);
            }
        }
    }
    else {
        vector<double> block(local_rows * local_cols);
        MPI_Recv(block.data(), local_rows * local_cols, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int r = 0; r < local_rows; ++r) {
            for (int c = 0; c < local_cols; ++c) {
                local_grid[(r + 1) * padded_cols + (c + 1)] = block[r * local_cols + c];
            }
        }
    }

    MPI_Datatype column_type;
    MPI_Type_vector(local_rows, 1, padded_cols, MPI_DOUBLE, &column_type);
    MPI_Type_commit(&column_type);

    for (int iter = 0; iter < 100; ++iter) {
        MPI_Request reqs[8];
        int req_count = 0;

        if (north != MPI_PROC_NULL) {
            MPI_Irecv(&local_grid[0 * padded_cols + 1], local_cols, MPI_DOUBLE, north, 1, cart_comm, &reqs[req_count++]);
            MPI_Isend(&local_grid[1 * padded_cols + 1], local_cols, MPI_DOUBLE, north, 2, cart_comm, &reqs[req_count++]);
        }
        if (south != MPI_PROC_NULL) {
            MPI_Irecv(&local_grid[(local_rows + 1) * padded_cols + 1], local_cols, MPI_DOUBLE, south, 2, cart_comm, &reqs[req_count++]);
            MPI_Isend(&local_grid[local_rows * padded_cols + 1], local_cols, MPI_DOUBLE, south, 1, cart_comm, &reqs[req_count++]);
        }
        if (west != MPI_PROC_NULL) {
            MPI_Irecv(&local_grid[1 * padded_cols + 0], 1, column_type, west, 3, cart_comm, &reqs[req_count++]);
            MPI_Isend(&local_grid[1 * padded_cols + 1], 1, column_type, west, 4, cart_comm, &reqs[req_count++]);
        }
        if (east != MPI_PROC_NULL) {
            MPI_Irecv(&local_grid[1 * padded_cols + (local_cols + 1)], 1, column_type, east, 4, cart_comm, &reqs[req_count++]);
            MPI_Isend(&local_grid[1 * padded_cols + local_cols], 1, column_type, east, 3, cart_comm, &reqs[req_count++]);
        }

        MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);

        for (int i = 1; i <= local_rows; ++i) {
            for (int j = 1; j <= local_cols; ++j) {
                double current = local_grid[i * padded_cols + j];
                double top = local_grid[(i - 1) * padded_cols + j];
                double bottom = local_grid[(i + 1) * padded_cols + j];
                double left = local_grid[i * padded_cols + (j - 1)];
                double right = local_grid[i * padded_cols + (j + 1)];
                next_grid[i * padded_cols + j] = current + 0.1 * (top + bottom + left + right - 4 * current);
            }
        }
        local_grid = next_grid;
    }

    if (rank != 0) {
        vector<double> block(local_rows * local_cols);
        for (int r = 0; r < local_rows; ++r) {
            for (int c = 0; c < local_cols; ++c) {
                block[r * local_cols + c] = local_grid[(r + 1) * padded_cols + (c + 1)];
            }
        }
        MPI_Send(block.data(), local_rows * local_cols, MPI_DOUBLE, 0, 99, MPI_COMM_WORLD);
    }
    else {
        for (int r = 0; r < local_rows; ++r) {
            for (int c = 0; c < local_cols; ++c) {
                global_grid[r * cols + c] = local_grid[(r + 1) * padded_cols + (c + 1)];
            }
        }
        for (int p = 1; p < size; ++p) {
            int p_coords[2];
            MPI_Cart_coords(cart_comm, p, 2, p_coords);
            int start_row = p_coords[0] * local_rows;
            int start_col = p_coords[1] * local_cols;

            vector<double> block(local_rows * local_cols);
            MPI_Recv(block.data(), local_rows * local_cols, MPI_DOUBLE, p, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            for (int r = 0; r < local_rows; ++r) {
                for (int c = 0; c < local_cols; ++c) {
                    global_grid[(start_row + r) * cols + (start_col + c)] = block[r * local_cols + c];
                }
            }
        }

        ofstream outfile("heat_result_2d.txt");
        outfile << "2D Decomposition Result (Top left 10x10):\n";
        for (int i = 0; i < min(rows, 10); ++i) {
            for (int j = 0; j < min(cols, 10); ++j) outfile << global_grid[i * cols + j] << " ";
            outfile << "\n";
        }
        outfile.close();
        cout << "Success: Saved top 10x10 to 'heat_result_2d.txt'" << endl;
    }

    MPI_Type_free(&column_type);
}

// ==========================================
// الدالة الرئيسية (Main Menu)
// ==========================================
int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int choice = 0;

    if (rank == 0) {
        cout << "\n============================================\n";
        cout << "   Advanced Parallel Grid Processing System   \n";
        cout << "============================================\n";
        cout << "1. Heat Diffusion (Category A - 1D Blocking)\n";
        cout << "2. Game of Life (Category A - 1D Non-Blocking)\n";
        cout << "3. Odd-Even Sort (Category B - Big Data)\n";
        cout << "4. Prefix Sum (Category B - Big Data)\n";
        cout << "5. Heat Diffusion (Bonus - 2D Cart Decomposition)\n";
        cout << "Choice: ";
        cin >> choice;
    }

    MPI_Bcast(&choice, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // --- بداية حساب الوقت للـ Performance Analysis ---
    double start_time = MPI_Wtime();

    switch (choice) {
    case 1: runHeatDiffusion(rank, size); break;
    case 2: runGameOfLife(rank, size); break;
    case 3: runOddEvenSort(rank, size); break;
    case 4: runPrefixSum(rank, size); break;
    case 5: runHeatDiffusion2D(rank, size); break;
    default: if (rank == 0) cout << "Invalid choice! Exiting." << endl; break;
    }

    // --- نهاية حساب الوقت ---
    double end_time = MPI_Wtime();

    // طباعة الوقت في الـ Process الأساسي فقط ولما يكون الاختيار صحيح
    if (rank == 0 && choice >= 1 && choice <= 5) {
        cout << "\n--------------------------------------------" << endl;
        cout << "Total Execution Time: " << (end_time - start_time) << " seconds." << endl;
        cout << "--------------------------------------------\n" << endl;
    }

    MPI_Finalize();
    return 0;
}