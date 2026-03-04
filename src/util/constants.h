#ifndef CONSTANTS_H
#define CONSTANTS_H

// Image pipeline
constexpr int    MAX_QUEUE_SIZE       = 5;      // max frames in image queue before dropping
constexpr int    QUEUE_EMPTY_SLEEP_MS = 10;     // sleep interval (ms) when queue is empty
constexpr double FONT_SCALE_DIVISOR   = 1024.0; // display height divisor for overlay font scaling

// Auto-MCP (auto brightness)
constexpr int    AUTO_MCP_DIVISOR     = 200;    // pixel count divisor for histogram threshold

// MCP range by pixel depth
constexpr int    MAX_MCP_8BIT         = 255;    // max MCP for 8-bit devices
constexpr int    MAX_MCP_12BIT        = 4095;   // max MCP for 12-bit devices

// Parameter update throttling
constexpr int    THROTTLE_MS          = 40;     // minimum interval (ms) between parameter updates

// Thread pool
constexpr int    THREAD_POOL_SIZE     = 40;     // number of worker threads

// Physics
constexpr double LIGHT_SPEED_M_NS    = 3e8 * 1e-9 / 2;  // one-way speed of light (~0.15 m/ns)

#endif // CONSTANTS_H
