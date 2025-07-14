#ifndef MOSAIC_LAYOUT_H
#define MOSAIC_LAYOUT_H

#include <vector>
#include <string>

struct LayoutConfig {
    int output_width;
    int output_height;
    int grid_cols;
    int grid_rows;
    int cell_padding;
    std::string background_color;
};

class MosaicLayout {
public:
    MosaicLayout();
    ~MosaicLayout();

    bool loadConfig(const std::string& config_file);
    void calculateCellDimensions();
    
    void getCellPosition(int grid_x, int grid_y, int& x, int& y);
    void getCellSize(int& width, int& height);
    
    LayoutConfig& getConfig() { return config_; }

private:
    LayoutConfig config_;
    int cell_width_;
    int cell_height_;
};

#endif // MOSAIC_LAYOUT_H