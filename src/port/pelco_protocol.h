#ifndef PELCO_PROTOCOL_H
#define PELCO_PROTOCOL_H

// Shared protocol constants (PTZ and lens)
constexpr unsigned char SYNC_BYTE     = 0xFF;      // sync byte (first byte of every command)
constexpr unsigned char MIN_SPEED     = 0x01;
constexpr unsigned char MAX_SPEED     = 0x3F;
constexpr int READ_TIMEOUT_MS         = 40;         // read timeout for PTZ/lens communication

// Angle encoding: degrees * ANGLE_SCALE
constexpr int    ANGLE_SCALE     = 100;             // angle multiplier (1 degree = 100 units)
constexpr int    FULL_ROTATION   = 36000;           // 360 degrees in scaled units
constexpr int    VERTICAL_LIMIT  = 4000;            // +/-40 degree vertical range in scaled units
constexpr double VERTICAL_DEG    = 40.0;            // vertical limit in degrees

// PTZ query/reply command bytes
constexpr unsigned char PTZ_QUERY_H = 0x51;         // query horizontal angle
constexpr unsigned char PTZ_REPLY_H = 0x59;         // reply horizontal angle
constexpr unsigned char PTZ_QUERY_V = 0x53;         // query vertical angle
constexpr unsigned char PTZ_REPLY_V = 0x5B;         // reply vertical angle

// PTZ set angle command bytes
constexpr unsigned char PTZ_SET_H   = 0x4B;         // set horizontal angle
constexpr unsigned char PTZ_SET_V   = 0x4D;         // set vertical angle

// PTZ direction command bytes (byte 3)
constexpr unsigned char PTZ_DIR_STOP       = 0x00;
constexpr unsigned char PTZ_DIR_RIGHT      = 0x02;
constexpr unsigned char PTZ_DIR_LEFT       = 0x04;
constexpr unsigned char PTZ_DIR_SELF_CHECK = 0x07;
constexpr unsigned char PTZ_DIR_UP         = 0x08;
constexpr unsigned char PTZ_DIR_UP_RIGHT   = 0x0A;
constexpr unsigned char PTZ_DIR_UP_LEFT    = 0x0C;
constexpr unsigned char PTZ_DIR_DOWN       = 0x10;
constexpr unsigned char PTZ_DIR_DOWN_RIGHT = 0x12;
constexpr unsigned char PTZ_DIR_DOWN_LEFT  = 0x14;

// PTZ self-check data byte
constexpr unsigned char PTZ_SELF_CHECK_DATA = 0x77;

// Lens query command bytes (byte 3)
constexpr unsigned char LENS_QUERY_ZOOM   = 0x55;
constexpr unsigned char LENS_QUERY_FOCUS  = 0x56;
constexpr unsigned char LENS_QUERY_RADIUS = 0x57;

// Lens query reply identifiers (byte 3)
constexpr unsigned char LENS_REPLY_ZOOM   = 0x5D;
constexpr unsigned char LENS_REPLY_FOCUS  = 0x5E;
constexpr unsigned char LENS_REPLY_RADIUS = 0x70;

// Lens direction commands — Command 2 (byte 3)
constexpr unsigned char LENS_CMD_STOP      = 0x00;
constexpr unsigned char LENS_CMD_ZOOM_IN   = 0x20;
constexpr unsigned char LENS_CMD_ZOOM_OUT  = 0x40;
constexpr unsigned char LENS_CMD_FOCUS_FAR = 0x80;

// Lens direction commands — Command 1 (byte 2)
constexpr unsigned char LENS_CMD_FOCUS_NEAR  = 0x01;
constexpr unsigned char LENS_CMD_RADIUS_UP   = 0x02;
constexpr unsigned char LENS_CMD_RADIUS_DOWN = 0x04;

// Lens set position command bytes (byte 3)
constexpr unsigned char LENS_SET_ZOOM   = 0x4F;
constexpr unsigned char LENS_SET_FOCUS  = 0x4E;
constexpr unsigned char LENS_SET_RADIUS = 0x81;

// Lens relay command bytes (byte 3)
constexpr unsigned char LENS_RELAY_ON  = 0x09;
constexpr unsigned char LENS_RELAY_OFF = 0x0B;

#endif // PELCO_PROTOCOL_H
