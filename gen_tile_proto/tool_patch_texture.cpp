//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

#define ENABLE_EDGE_WRAPPING 1

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static double g_patch_size_factor = 1.0;
static double g_horizontal_growth_bias = 1.0;

//================================================================================================================================
//=> - Types and structures -
//================================================================================================================================

struct Color {
    unsigned char r, g, b;
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

struct PatchData {
    int px_count;
    Color color;
};

struct Point {
    int x, y;
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

//================================================================================================================================
//=> - Class: PatchTextureGenerator -
//================================================================================================================================

class PatchTextureGenerator {
private:
    int height;
    int width;
    Color background_color;
    std::vector<PatchData> patch_data;
    std::vector<std::vector<Color>> canvas;
    std::mt19937 rng;

    std::vector<PatchData> parsePatchFile(const std::string& patch_path) {
        std::vector<PatchData> result;
        std::ifstream file(patch_path);
        if (!file.is_open()) {
            printf("*** Warning: Patch file not found: %s\n", patch_path.c_str());
            return result;
        }
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            size_t pos1 = line.find(':');
            if (pos1 == std::string::npos) continue;
            size_t pos2 = line.find(':', pos1 + 1);
            if (pos2 == std::string::npos) continue;
            size_t pos3 = line.find(':', pos2 + 1);
            if (pos3 == std::string::npos) continue;
            try {
                int px_count = std::stoi(line.substr(0, pos1));
                int r = std::stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));
                int g = std::stoi(line.substr(pos2 + 1, pos3 - pos2 - 1));
                int b = std::stoi(line.substr(pos3 + 1));
                result.push_back({px_count, {(unsigned char)r, (unsigned char)g, (unsigned char)b}});
            } catch (...) {
                continue;
            }
        }
        return result;
    }

    std::set<Point> generatePatchShape(int pixel_count) {
        std::set<Point> patch_coords;
        std::set<Point> candidates_set;
        std::set<int> patch_y_coords;  // Track y-coordinates that have patches for O(log n) horizontal check
        int adjusted_count = (int)(pixel_count * g_patch_size_factor);
        if (adjusted_count <= 0) {
            return patch_coords;
        }
        Point seed = {0, 0};
        patch_coords.insert(seed);
        patch_y_coords.insert(seed.y);
        Point neighbors[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        for (const auto& n : neighbors) {
            candidates_set.insert({seed.x + n.x, seed.y + n.y});
        }
        std::uniform_int_distribution<int> dist(0, 1000000);
        std::uniform_real_distribution<double> prob_dist(0.0, 1.0);
        while ((int)patch_coords.size() < adjusted_count && !candidates_set.empty()) {
            std::vector<Point> candidates_vec(candidates_set.begin(), candidates_set.end());
            std::vector<Point> horizontal_candidates;
            std::vector<Point> vertical_candidates;
            for (const auto& cand : candidates_vec) {
                if (patch_y_coords.find(cand.y) != patch_y_coords.end()) {
                    horizontal_candidates.push_back(cand);
                } else {
                    vertical_candidates.push_back(cand);
                }
            }
            Point chosen;
            if (!horizontal_candidates.empty() && !vertical_candidates.empty()) {
                double horizontal_prob = g_horizontal_growth_bias / (g_horizontal_growth_bias + 1.0);
                if (prob_dist(rng) < horizontal_prob) {
                    int idx = dist(rng) % horizontal_candidates.size();
                    chosen = horizontal_candidates[idx];
                } else {
                    int idx = dist(rng) % vertical_candidates.size();
                    chosen = vertical_candidates[idx];
                }
            } else if (!horizontal_candidates.empty()) {
                int idx = dist(rng) % horizontal_candidates.size();
                chosen = horizontal_candidates[idx];
            } else if (!vertical_candidates.empty()) {
                int idx = dist(rng) % vertical_candidates.size();
                chosen = vertical_candidates[idx];
            } else {
                int idx = dist(rng) % candidates_vec.size();
                chosen = candidates_vec[idx];
            }
            candidates_set.erase(chosen);
            patch_coords.insert(chosen);
            patch_y_coords.insert(chosen.y);
            for (const auto& n : neighbors) {
                Point new_neighbor = {chosen.x + n.x, chosen.y + n.y};
                if (candidates_set.find(new_neighbor) == candidates_set.end() && 
                    patch_coords.find(new_neighbor) == patch_coords.end()) {
                    candidates_set.insert(new_neighbor);
                }
            }
        }
        return patch_coords;
    }

    std::vector<Point> findPlacement(const std::set<Point>& patch_coords) {
        int min_x = INT_MAX, min_y = INT_MAX;
        for (const auto& p : patch_coords) {
            if (p.x < min_x) min_x = p.x;
            if (p.y < min_y) min_y = p.y;
        }
        std::vector<Point> normalized_coords;
        int max_x = 0, max_y = 0;
        for (const auto& p : patch_coords) {
            Point norm = {p.x - min_x, p.y - min_y};
            normalized_coords.push_back(norm);
            if (norm.x > max_x) max_x = norm.x;
            if (norm.y > max_y) max_y = norm.y;
        }
        std::uniform_int_distribution<int> x_dist(0, width - 1);
        std::uniform_int_distribution<int> y_dist(0, height - 1);
        std::vector<Point> best_placement;
        int best_overlap = INT_MAX;
        for (int attempt = 0; attempt < 1000; attempt++) {
            int start_x, start_y;
            #if ENABLE_EDGE_WRAPPING
                start_x = x_dist(rng);
                start_y = y_dist(rng);
            #else
                start_x = (max_x < width) ? (x_dist(rng) % (width - max_x)) : 0;
                start_y = (max_y < height) ? (y_dist(rng) % (height - max_y)) : 0;
            #endif
            int overlap_count = 0;
            bool out_of_bounds = false;
            for (const auto& p : normalized_coords) {
                int canvas_x = start_x + p.x;
                int canvas_y = start_y + p.y;
                #if ENABLE_EDGE_WRAPPING
                    canvas_x = ((canvas_x % width) + width) % width;
                    canvas_y = ((canvas_y % height) + height) % height;
                #else
                    if (canvas_x >= width || canvas_y >= height) {
                        out_of_bounds = true;
                        break;
                    }
                #endif
                if (canvas[canvas_y][canvas_x] != background_color) {
                    overlap_count++;
                }
            }
            if (out_of_bounds) {
                continue;
            }
            if (overlap_count == 0) {
                std::vector<Point> result;
                for (const auto& p : normalized_coords) {
                    int result_x = start_x + p.x;
                    int result_y = start_y + p.y;
                    #if ENABLE_EDGE_WRAPPING
                        result_x = ((result_x % width) + width) % width;
                        result_y = ((result_y % height) + height) % height;
                    #endif
                    result.push_back({result_x, result_y});
                }
                return result;
            }
            if (overlap_count < best_overlap) {
                best_overlap = overlap_count;
                best_placement.clear();
                for (const auto& p : normalized_coords) {
                    int result_x = start_x + p.x;
                    int result_y = start_y + p.y;
                    #if ENABLE_EDGE_WRAPPING
                        result_x = ((result_x % width) + width) % width;
                        result_y = ((result_y % height) + height) % height;
                    #endif
                    best_placement.push_back({result_x, result_y});
                }
            }
        }
        return best_placement;
    }

    Point findPlacement1px() {
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (canvas[y][x] == background_color) {
                    return {x, y};
                }
            }
        }
        return {-1, -1};
    }

    bool placePatch(const std::set<Point>& patch_coords, const Color& color) {
        std::vector<Point> placement = findPlacement(patch_coords);
        if (!placement.empty()) {
            for (const auto& p : placement) {
                if (p.x >= 0 && p.x < width && p.y >= 0 && p.y < height) {
                    canvas[p.y][p.x] = color;
                }
            }
            return true;
        }
        return false;
    }

    bool placePatch1px(const Color& color) {
        Point placement = findPlacement1px();
        if (placement.x >= 0 && placement.y >= 0) {
            canvas[placement.y][placement.x] = color;
            return true;
        }
        return false;
    }

    void fillRemainingBackground() {
        std::vector<Color> non_bg_colors;
        for (const auto& patch : patch_data) {
            if (patch.color != background_color) {
                non_bg_colors.push_back(patch.color);
            }
        }
        if (non_bg_colors.empty()) {
            return;
        }
        std::uniform_int_distribution<int> dist(0, non_bg_colors.size() - 1);
        int filled_count = 0;
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (canvas[y][x] == background_color) {
                    canvas[y][x] = non_bg_colors[dist(rng)];
                    filled_count++;
                }
            }
        }
        printf("*** Filled %d remaining background pixels\n", filled_count);
    }

    int estimateMultiplier(const std::vector<PatchData>& data) {
        int total_patch_pixels = 0;
        for (const auto& patch : data) {
            total_patch_pixels += patch.px_count;
        }
        int canvas_pixels = width * height;
        if (total_patch_pixels == 0) {
            return 1;
        }
        double adjusted_total = total_patch_pixels * g_patch_size_factor;
        int multiplier = std::max(1, (int)((canvas_pixels + adjusted_total - 1) / adjusted_total));
        return multiplier;
    }

    void generateTexture() {
        canvas.resize(height);
        for (int y = 0; y < height; y++) {
            canvas[y].resize(width, background_color);
        }
        std::vector<PatchData> expanded_patches;
        for (const auto& patch : patch_data) {
            expanded_patches.push_back(patch);
        }
        int multiplier = estimateMultiplier(patch_data);
        printf("*** Estimated multiplier: %d\n", multiplier);
        for (int i = 0; i < multiplier; i++) {
            for (const auto& patch : patch_data) {
                expanded_patches.push_back(patch);
            }
        }
        std::sort(expanded_patches.begin(), expanded_patches.end(),
            [](const PatchData& a, const PatchData& b) {
                return a.px_count > b.px_count;
            });
        std::vector<PatchData> multi_px_patches;
        std::vector<PatchData> single_px_patches;
        for (const auto& patch : expanded_patches) {
            if (patch.px_count > 1) {
                multi_px_patches.push_back(patch);
            } else {
                single_px_patches.push_back(patch);
            }
        }
        int multi_px_count = multi_px_patches.size();
        int single_px_count = single_px_patches.size();
        int multi_px_placed = 0;
        int multi_px_failed = 0;
        for (const auto& patch : multi_px_patches) {
            std::set<Point> patch_coords = generatePatchShape(patch.px_count);
            if (placePatch(patch_coords, patch.color)) {
                multi_px_placed++;
            } else {
                multi_px_failed++;
            }
            printf("*** Placing multi-pixel patches: %d/%d placed\r", multi_px_placed, multi_px_count);
            fflush(stdout);
        }
        printf("\n");
        printf("*** Placed %d multi-pixel patches\n", multi_px_placed);
        int single_px_placed = 0;
        int single_px_failed = 0;
        for (const auto& patch : single_px_patches) {
            if (placePatch1px(patch.color)) {
                single_px_placed++;
            } else {
                single_px_failed++;
            }
            printf("*** Placing single-pixel patches: %d/%d placed\r", single_px_placed, single_px_count);
            fflush(stdout);
        }
        printf("\n");
        printf("*** Placed %d single-pixel patches\n", single_px_placed);
        printf("*** Total placed: %d patches\n", multi_px_placed + single_px_placed);
        printf("*** Failed placements: %d multi-pixel, %d single-pixel (total: %d)\n", 
               multi_px_failed, single_px_failed, multi_px_failed + single_px_failed);
        fillRemainingBackground();
    }

    void saveTexture(const std::string& output_path) {
        std::string ppm_path = output_path;
        size_t pos = ppm_path.rfind(".png");
        if (pos != std::string::npos) {
            ppm_path.replace(pos, 4, ".ppm");
        } else {
            ppm_path += ".ppm";
        }
        FILE* file = fopen(ppm_path.c_str(), "wb");
        if (!file) {
            printf("*** Error: Could not open file for writing: %s\n", ppm_path.c_str());
            return;
        }
        fprintf(file, "P6\n%d %d\n255\n", width, height);
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                fputc(canvas[y][x].r, file);
                fputc(canvas[y][x].g, file);
                fputc(canvas[y][x].b, file);
            }
        }
        fclose(file);
        printf("*** Saved texture to %s\n", ppm_path.c_str());
    }

public:
    PatchTextureGenerator(int h, int w, const std::string& patch_file_path) : 
            height(h), 
            width(w), 
            background_color({255, 0, 255}), 
            rng(std::random_device{}()) 
        {
        printf("*** Reading patch file: %s\n", patch_file_path.c_str());
        patch_data = parsePatchFile(patch_file_path);
        std::sort(patch_data.begin(), patch_data.end(),
            [](const PatchData& a, const PatchData& b) {
                return a.px_count > b.px_count;
            });
        int total_pixel_count = 0;
        for (const auto& patch : patch_data) {
            total_pixel_count += patch.px_count;
        }
        printf("*** Loaded %zu patch entries\n", patch_data.size());
        printf("*** Total patch pixel count: %d\n", total_pixel_count);
        generateTexture();
        std::string output_path = patch_file_path;
        size_t pos = output_path.find("_patches.txt");
        if (pos != std::string::npos) {
            output_path.replace(pos, 12, "_texture");
        } else {
            pos = output_path.rfind(".txt");
            if (pos != std::string::npos) {
                output_path.replace(pos, 4, "_texture");
            } else {
                output_path += "_texture";
            }
        }
        saveTexture(output_path);
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main(int argc, char* argv[]) {
    if (argc < 6) {
        printf("Usage: %s <patch_file_path> <width> <height> <patch_size_factor> <horizontal_growth_bias>\n", argv[0]);
        return 1;
    }
    std::cout << "*** Starting patch texture generation" << std::endl;
    std::cout << "*** Patch file path: " << argv[1] << std::endl;
    std::cout << "*** Width: " << argv[2] << std::endl;
    std::cout << "*** Height: " << argv[3] << std::endl;
    std::cout << "*** Patch size factor: " << argv[4] << std::endl;
    std::cout << "*** Horizontal growth bias: " << argv[5] << std::endl;

    std::string patch_file_path = argv[1];
    int width = std::stoi(argv[2]);
    int height = std::stoi(argv[3]);
    g_patch_size_factor = std::stod(argv[4]);
    g_horizontal_growth_bias = std::stod(argv[5]);
    PatchTextureGenerator generator(height, width, patch_file_path);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

