#include "mosaic_layout.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>

MosaicLayout::MosaicLayout() 
    : cell_width_(0)
    , cell_height_(0) {
    // Default configuration
    config_.output_width = 1920;
    config_.output_height = 1080;
    config_.grid_cols = 2;
    config_.grid_rows = 2;
    config_.cell_padding = 5;
    config_.background_color = "#000000";
}

MosaicLayout::~MosaicLayout() {
}

bool MosaicLayout::loadConfig(const std::string& config_file) {
    try {
        YAML::Node config = YAML::LoadFile(config_file);
        
        // Load output configuration
        if (config["output"]) {
            if (config["output"]["width"]) {
                config_.output_width = config["output"]["width"].as<int>();
            }
            if (config["output"]["height"]) {
                config_.output_height = config["output"]["height"].as<int>();
            }
        }
        
        // Load layout configuration
        if (config["layout"]) {
            if (config["layout"]["grid_cols"]) {
                config_.grid_cols = config["layout"]["grid_cols"].as<int>();
            }
            if (config["layout"]["grid_rows"]) {
                config_.grid_rows = config["layout"]["grid_rows"].as<int>();
            }
            if (config["layout"]["cell_padding"]) {
                config_.cell_padding = config["layout"]["cell_padding"].as<int>();
            }
            if (config["layout"]["background_color"]) {
                config_.background_color = config["layout"]["background_color"].as<std::string>();
            }
        }
        
        // Calculate cell dimensions
        calculateCellDimensions();
        
        std::cout << "Loaded configuration:" << std::endl;
        std::cout << "  Output: " << config_.output_width << "x" << config_.output_height << std::endl;
        std::cout << "  Grid: " << config_.grid_cols << "x" << config_.grid_rows << std::endl;
        std::cout << "  Cell size: " << cell_width_ << "x" << cell_height_ << std::endl;
        std::cout << "  Padding: " << config_.cell_padding << std::endl;
        
        return true;
        
    } catch (const YAML::Exception& e) {
        std::cerr << "Error loading configuration: " << e.what() << std::endl;
        
        // Use default configuration
        calculateCellDimensions();
        return true;  // Continue with defaults
    }
}

void MosaicLayout::calculateCellDimensions() {
    // Calculate cell dimensions based on output size and grid
    int total_padding_x = (config_.grid_cols + 1) * config_.cell_padding;
    int total_padding_y = (config_.grid_rows + 1) * config_.cell_padding;
    
    cell_width_ = (config_.output_width - total_padding_x) / config_.grid_cols;
    cell_height_ = (config_.output_height - total_padding_y) / config_.grid_rows;
    
    // Ensure dimensions are even (required for many video codecs)
    cell_width_ = (cell_width_ / 2) * 2;
    cell_height_ = (cell_height_ / 2) * 2;
}

void MosaicLayout::getCellPosition(int grid_x, int grid_y, int& x, int& y) {
    // Calculate actual pixel position including padding
    x = config_.cell_padding + (grid_x * (cell_width_ + config_.cell_padding));
    y = config_.cell_padding + (grid_y * (cell_height_ + config_.cell_padding));
}

void MosaicLayout::getCellSize(int& width, int& height) {
    width = cell_width_;
    height = cell_height_;
}