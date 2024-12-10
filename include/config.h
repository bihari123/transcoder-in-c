// config.h
#ifndef CONFIG_H
#define CONFIG_H

#define PART_DURATION 0.2         // 200ms parts for LL-HLS
#define SEGMENT_DURATION 1        // 1 second segments
#define MAX_SEGMENTS_IN_LIST 6    // Keep 6 segments in playlist
#define BUFFER_SIZE (8192 * 1024) // 8MB buffer
#define GOP_SIZE 60               // GOP size for keyframes
#define MAX_QUALITY_LEVELS 3      // Number of quality levels
#define MONITORING_INTERVAL 1     // Stats update interval (seconds)
#define DEBUG_MODE 1              // Enable debug output

#endif // CONFIG_H
