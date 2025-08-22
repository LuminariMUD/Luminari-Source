/**
 * @file wilderness_kb.h
 * Wilderness Knowledge Base Generator
 * 
 * Generates comprehensive documentation of the Luminari wilderness world map
 * optimized for LLM consumption with multiple data representations.
 */

#ifndef _WILDERNESS_KB_H_
#define _WILDERNESS_KB_H_

/* Core data structures for analysis */

struct terrain_stats {
    int total_tiles;
    int sector_counts[NUM_ROOM_SECTORS];
    float sector_percentages[NUM_ROOM_SECTORS];
    int min_elevation, max_elevation, avg_elevation;
    int min_temp, max_temp, avg_temp;
    int min_moisture, max_moisture, avg_moisture;
};

struct landmass_info {
    int id;
    int start_x, start_y;
    int min_x, max_x, min_y, max_y;
    int tile_count;
    int peak_elevation;
    char dominant_biome[64];
    struct landmass_info *next;
};

struct resource_hotspot {
    int x, y;
    int resource_type;
    float density;
    struct resource_hotspot *next;
};

struct mountain_range {
    int id;
    struct ridge_point {
        int x, y;
        struct ridge_point *next;
    } *ridge_points;
    int num_points;
    int highest_peak;
    int lowest_pass;
    char name[128];
    struct mountain_range *next;
};

struct climate_zone {
    int min_y, max_y;
    int avg_temperature;
    char description[256];
    struct climate_zone *next;
};

struct spatial_relationship {
    int from_x, from_y;
    int to_x, to_y;
    float distance;
    int direction; /* 0-7 for N,NE,E,SE,S,SW,W,NW */
    char relation_type[32]; /* "adjacent", "contains", "overlaps" */
    struct spatial_relationship *next;
};

struct resource_correlation {
    int resource_type_1, resource_type_2;
    float correlation_coefficient;
    int co_occurrence_count;
    float avg_distance_between;
};

struct transition_zone {
    int x, y;
    int from_biome, to_biome;
    float gradient_strength;
    int edge_type; /* 0=sharp, 1=gradual, 2=ecotone */
    int transition_width;
    struct transition_zone *next;
};

struct ocean_depth_profile {
    int distance_from_shore;
    int avg_depth, min_depth, max_depth;
    int shelf_width;
    float slope_angle;
};

struct habitability_score {
    int x, y;
    float water_access;       /* 0.0-1.0 */
    float resource_richness;  /* 0.0-1.0 */
    float terrain_difficulty; /* 0.0-1.0, lower is better */
    float defense_rating;     /* 0.0-1.0 */
    float agriculture_potential; /* 0.0-1.0 */
    float trade_access;       /* 0.0-1.0 */
    float overall_score;      /* 0.0-1.0 */
};

struct network_node {
    int x, y;
    int node_type; /* 0=intersection, 1=endpoint, 2=waypoint */
    int *connected_nodes;
    int num_connections;
    float centrality_score;
    int is_bottleneck;
    struct network_node *next;
};

struct feature_cluster {
    int cluster_id;
    int feature_type;
    int center_x, center_y;
    int *member_coords; /* Flattened x,y pairs */
    int num_members;
    float density;
    float avg_distance_between_members;
    struct feature_cluster *next;
};

/* Main function */
void generate_wilderness_knowledge_base(const char *output_filename);

/* Analysis functions */
void document_noise_layers(FILE *fp);
void analyze_world_grid(FILE *fp, struct terrain_stats *stats);
struct landmass_info *detect_landmasses(FILE *fp);
void trace_mountain_ranges(FILE *fp);
void analyze_climate_zones(FILE *fp);
void analyze_resource_distribution(FILE *fp);
void analyze_spatial_relationships(FILE *fp);
void map_transition_zones(FILE *fp);
void analyze_ocean_systems(FILE *fp);
void construct_path_network_graph(FILE *fp);
void analyze_civilization_potential(FILE *fp);

/* MySQL integration functions */
void document_all_regions(FILE *fp);
void document_all_paths(FILE *fp);

/* Output generation functions */
void write_header_section(FILE *fp);
void write_continental_geography(FILE *fp, struct landmass_info *landmasses);
void write_elevation_analysis(FILE *fp);
void write_hydrological_systems(FILE *fp);
void write_climate_analysis(FILE *fp);
void write_biome_distribution(FILE *fp);
void write_resource_analysis(FILE *fp);
void write_strategic_locations(FILE *fp);
void write_navigation_guide(FILE *fp);
void generate_ascii_visualization(FILE *fp, int map_type);
void write_query_reference(FILE *fp);

/* JSON output functions */
void write_landmasses_json(FILE *fp, struct landmass_info *landmasses);
void write_resources_json(FILE *fp);
void write_climate_json(FILE *fp);
void write_path_network_json(FILE *fp);

/* Statistical analysis functions */
void calculate_accessibility_matrix(FILE *fp);
void calculate_diversity_indices(FILE *fp);
void calculate_exploration_coverage(FILE *fp);
void calculate_fractal_dimensions(FILE *fp);
void model_economic_potential(FILE *fp);

/* Advanced features (7% Sprint) */
void calculate_dijkstra_paths(FILE *fp, int source_x, int source_y, int dest_x, int dest_y);
void analyze_noise_spectrum(FILE *fp);
void write_mountain_ranges_json(FILE *fp);
void write_transitions_json(FILE *fp);
void write_habitability_json(FILE *fp);
void write_ocean_json(FILE *fp);

/* Utility functions */
void report_progress(const char *stage, int percent);
int is_land_tile(int x, int y);
int get_biome_at(int x, int y);
float get_resource_density(int x, int y, int resource_type);
float calculate_distance(int x1, int y1, int x2, int y2);
int get_direction(int from_x, int from_y, int to_x, int to_y);

/* Map types for ASCII visualization */
#define MAP_TYPE_TERRAIN     0
#define MAP_TYPE_ELEVATION   1
#define MAP_TYPE_CLIMATE     2
#define MAP_TYPE_RESOURCE    3
#define MAP_TYPE_OCEAN_DEPTH 4
#define MAP_TYPE_HABITABILITY 5
#define MAP_TYPE_BIOME       6
#define MAP_TYPE_NETWORK     7

/* Edge types for transitions */
#define EDGE_SHARP    0
#define EDGE_GRADUAL  1
#define EDGE_ECOTONE  2

/* Node types for network */
#define NODE_INTERSECTION 0
#define NODE_ENDPOINT     1
#define NODE_WAYPOINT     2

#endif /* _WILDERNESS_KB_H_ */