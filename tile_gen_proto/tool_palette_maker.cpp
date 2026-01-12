//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <climits>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <random>
#include <string>
#include <fstream>
#include <queue>

//================================================================================================================================
//=> - Types and structures -
//================================================================================================================================

struct Color {
    unsigned char r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(unsigned char r_, unsigned char g_, unsigned char b_) : r(r_), g(g_), b(b_) {}
    
    bool operator<(const Color& other) const {
        if (r != other.r) return r < other.r;
        if (g != other.g) return g < other.g;
        return b < other.b;
    }
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    
    bool operator!=(const Color& other) const {
        return !(*this == other);
    }
};

struct Point {
    int x, y;
    Point(int x_, int y_) : x(x_), y(y_) {}
};

//================================================================================================================================
//=> - Class: PaletteMaker -
//================================================================================================================================

class PaletteMaker {
private:
    std::string image_path;
    std::string save_path;
    std::string palette_img_save;
    int num_colors;
    int width;
    int height;
    std::vector<std::vector<Color>> image_array;
    std::map<Color, std::vector<Point>> color_to_coords;
    std::vector<Color> palette_colors;
    std::mt19937 rng;

    int colorDistance(const Color& color1, const Color& color2) {
        return abs((int)color1.r - (int)color2.r) + 
               abs((int)color1.g - (int)color2.g) + 
               abs((int)color1.b - (int)color2.b);
    }

    void loadImage() {
        FILE* file = fopen(image_path.c_str(), "rb");
        if (file == nullptr) {
            printf("*** Error: Could not open image file: %s\n", image_path.c_str());
            image_array.clear();
            return;
        }
        
        char magic[3];
        if (fread(magic, 1, 2, file) != 2 || magic[0] != 'P' || magic[1] != '6') {
            printf("*** Error: Not a valid PPM file (P6 format required): %s\n", image_path.c_str());
            fclose(file);
            image_array.clear();
            return;
        }
        magic[2] = '\0';
        
        int c;
        while ((c = fgetc(file)) != EOF && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {}
        if (c == '#') {
            while ((c = fgetc(file)) != EOF && c != '\n') {}
            c = fgetc(file);
        }
        ungetc(c, file);
        
        if (fscanf(file, "%d %d", &width, &height) != 2) {
            printf("*** Error: Could not read image dimensions from: %s\n", image_path.c_str());
            fclose(file);
            image_array.clear();
            return;
        }
        
        int max_val;
        if (fscanf(file, "%d", &max_val) != 1) {
            printf("*** Error: Could not read max value from: %s\n", image_path.c_str());
            fclose(file);
            image_array.clear();
            return;
        }
        
        fgetc(file);
        
        image_array.resize(height);
        for (int y = 0; y < height; y++) {
            image_array[y].resize(width);
            for (int x = 0; x < width; x++) {
                unsigned char r = fgetc(file);
                unsigned char g = fgetc(file);
                unsigned char b = fgetc(file);
                if (feof(file)) {
                    printf("*** Error: Unexpected end of file while reading pixel data\n");
                    fclose(file);
                    image_array.clear();
                    return;
                }
                image_array[y][x] = Color(r, g, b);
            }
        }
        
        fclose(file);
        analyzeColors();
    }

    void analyzeColors() {
        if (image_array.empty()) {
            return;
        }
        
        color_to_coords.clear();
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color color = image_array[y][x];
                color_to_coords[color].push_back(Point(x, y));
            }
        }
        
        printf("*** Found %zu unique colors\n", color_to_coords.size());
        mergeColors();
    }

    void mergeColors() {
        if (image_array.empty() || color_to_coords.empty()) {
            return;
        }
        
        std::vector<Color> color_list;
        for (const auto& pair : color_to_coords) {
            color_list.push_back(pair.first);
        }
        
        int initial_count = (int)color_list.size();
        printf("*** Starting merge: %d colors -> %d colors\n", initial_count, num_colors);
        printf("*** Creating distance table for %d colors", initial_count);
        fflush(stdout);
        
        std::map<Color, std::vector<std::pair<int, Color>>> distance_lists;
        for (size_t i = 0; i < color_list.size(); i++) {
            Color color1 = color_list[i];
            distance_lists[color1] = std::vector<std::pair<int, Color>>();
            for (size_t j = 0; j < color_list.size(); j++) {
                if (i != j) {
                    Color color2 = color_list[j];
                    int distance = colorDistance(color1, color2);
                    distance_lists[color1].push_back(std::make_pair(distance, color2));
                }
            }
            std::sort(distance_lists[color1].begin(), distance_lists[color1].end());
        }
        
        printf("\n");
        
        int iteration = 0;
        while ((int)color_list.size() > num_colors) {
            if (distance_lists.empty()) {
                break;
            }
            
            int min_distance = INT_MAX;
            Color color1_min, color2_min;
            bool found = false;
            
            for (const auto& color_entry : distance_lists) {
                const Color& color = color_entry.first;
                const std::vector<std::pair<int, Color>>& dist_list = color_entry.second;
                if (!dist_list.empty()) {
                    int dist = dist_list[0].first;
                    if (dist < min_distance) {
                        min_distance = dist;
                        color1_min = color;
                        color2_min = dist_list[0].second;
                        found = true;
                    }
                }
            }
            
            if (!found) {
                break;
            }
            
            Color color1 = color1_min;
            Color color2 = color2_min;
            int count1 = (int)color_to_coords[color1].size();
            int count2 = (int)color_to_coords[color2].size();
            
            Color merge_from, merge_to;
            if (count1 < count2) {
                merge_from = color1;
                merge_to = color2;
            } else if (count2 < count1) {
                merge_from = color2;
                merge_to = color1;
            } else {
                std::uniform_real_distribution<double> dist(0.0, 1.0);
                if (dist(rng) < 0.5) {
                    merge_from = color1;
                    merge_to = color2;
                } else {
                    merge_from = color2;
                    merge_to = color1;
                }
            }
            
            distance_lists.erase(merge_from);
            
            for (auto& color_entry : distance_lists) {
                std::vector<std::pair<int, Color>>& dist_list = color_entry.second;
                dist_list.erase(
                    std::remove_if(dist_list.begin(), dist_list.end(),
                        [&merge_from](const std::pair<int, Color>& p) {
                            return p.second == merge_from;
                        }),
                    dist_list.end()
                );
            }
            
            color_to_coords[merge_to].insert(
                color_to_coords[merge_to].end(),
                color_to_coords[merge_from].begin(),
                color_to_coords[merge_from].end()
            );
            color_to_coords.erase(merge_from);
            
            color_list.erase(std::remove(color_list.begin(), color_list.end(), merge_from), color_list.end());
            
            iteration++;
            int remaining = (int)color_list.size();
            printf("*** Merging iteration %d: %d colors remaining\r", iteration, remaining);
            fflush(stdout);
        }
        
        printf("\n");
        
        std::vector<Color> final_colors;
        for (const auto& pair : color_to_coords) {
            final_colors.push_back(pair.first);
        }
        
        printf("*** Merge complete: %zu colors\n", final_colors.size());
        setPalette(final_colors);
        repaintImage();
        
        std::ofstream file(save_path);
        if (file.is_open()) {
            for (const auto& pair : color_to_coords) {
                Color color = pair.first;
                file << (int)color.r << ":" << (int)color.g << ":" << (int)color.b << "\n";
            }
            file.close();
        }
    }

    void repaintImage() {
        if (image_array.empty() || color_to_coords.empty()) {
            return;
        }
        
        std::vector<Color> palette_colors_list;
        for (const auto& pair : color_to_coords) {
            palette_colors_list.push_back(pair.first);
        }
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color original_color = image_array[y][x];
                Color closest_color;
                int min_dist = INT_MAX;
                
                for (const Color& palette_color : palette_colors_list) {
                    int dist = colorDistance(original_color, palette_color);
                    if (dist < min_dist) {
                        min_dist = dist;
                        closest_color = palette_color;
                    }
                }
                
                image_array[y][x] = closest_color;
            }
        }
        
        saveRepaintedImage();
    }

    void saveRepaintedImage() {
        FILE* file = fopen(palette_img_save.c_str(), "wb");
        if (!file) {
            printf("*** Error: Could not open file for writing: %s\n", palette_img_save.c_str());
            return;
        }
        
        char header[64];
        int header_len = snprintf(header, sizeof(header), "P6\n%d %d\n255\n", width, height);
        fwrite(header, 1, header_len, file);
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                Color color = image_array[y][x];
                fputc(color.r, file);
                fputc(color.g, file);
                fputc(color.b, file);
            }
        }
        
        fclose(file);
        printf("*** Saved repainted image to %s\n", palette_img_save.c_str());
    }

public:
    PaletteMaker(const std::string& image_path_, int num_colors_) 
        : image_path(image_path_), num_colors(num_colors_), rng(std::random_device{}()) {
        save_path = image_path;
        size_t pos = save_path.rfind(".png");
        if (pos != std::string::npos) {
            save_path.replace(pos, 4, "_palette.txt");
        } else {
            save_path += "_palette.txt";
        }
        
        palette_img_save = image_path;
        pos = palette_img_save.rfind(".png");
        if (pos != std::string::npos) {
            palette_img_save.replace(pos, 4, "_palette.ppm");
        } else {
            // Remove any existing extension and add .ppm
            size_t last_dot = palette_img_save.find_last_of(".");
            if (last_dot != std::string::npos) {
                palette_img_save = palette_img_save.substr(0, last_dot) + "_palette.ppm";
            } else {
                palette_img_save += "_palette.ppm";
            }
        }
        
        palette_colors.resize(num_colors, Color(0, 0, 0));
        loadImage();
    }

    void setPalette(const std::vector<Color>& colors) {
        palette_colors = colors;
        if ((int)palette_colors.size() < num_colors) {
            palette_colors.resize(num_colors, Color(0, 0, 0));
        } else if ((int)palette_colors.size() > num_colors) {
            palette_colors.resize(num_colors);
        }
    }

    std::vector<Color> getPalette() const {
        return palette_colors;
    }
};

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("*** Error: Usage: %s <image_path>\n", argv[0]);
        return 1;
    }
    
    std::string image_path = argv[1];
    int num_colors = 8;
    PaletteMaker maker(image_path, num_colors);
    
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
