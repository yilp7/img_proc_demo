#ifndef CONSTANTS_H
#define CONSTANTS_H

// Image pipeline
constexpr int    MAX_QUEUE_SIZE       = 5;      // max frames in image queue before dropping
constexpr int    QUEUE_EMPTY_SLEEP_MS = 10;
constexpr double FONT_SCALE_DIVISOR   = 1024.0; // display height divisor for overlay font scaling

// Auto-MCP (auto brightness)
constexpr int    AUTO_MCP_DIVISOR     = 200;    // pixel count divisor for histogram threshold

// MCP range by pixel depth
constexpr int    MAX_MCP_8BIT         = 255;
constexpr int    MAX_MCP_12BIT        = 4095;

// Parameter update throttling
constexpr int    THROTTLE_MS          = 40;     // minimum interval (ms) between parameter updates

constexpr int    THREAD_POOL_SIZE     = 40;

// Physics
constexpr double LIGHT_SPEED_M_NS    = 3e8 * 1e-9 / 2;  // one-way speed of light (~0.15 m/ns)

#endif // CONSTANTS_H
