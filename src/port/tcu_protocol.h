#ifndef TCU_PROTOCOL_H
#define TCU_PROTOCOL_H

// TCU frame format
constexpr unsigned char TCU_HEAD    = 0x88;     // frame header byte
constexpr unsigned char TCU_TAIL    = 0x99;     // frame tail byte

// Query
constexpr unsigned char TCU_QUERY_CMD   = 0x15; // query command byte
constexpr unsigned char TCU_QUERY_REPLY = 0x15; // expected reply byte

// Clock cycle divisor by TCU type
constexpr int TCU_CLOCK_8 = 8;                  // type 0 clock cycle
constexpr int TCU_CLOCK_4 = 4;                  // type 1/2 clock cycle

// Frequency limits
constexpr double TCU_MAX_PRF_KHZ = 30.0;        // max pulse repetition frequency (kHz)

#endif // TCU_PROTOCOL_H
