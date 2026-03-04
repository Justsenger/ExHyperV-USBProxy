// input_protocol.h

#pragma once
#include <cstdint>

// 消息类型枚举
enum class MessageType : uint32_t {
    // Client -> Server (Host -> VM)
    CONFIG,
    KEY_DOWN,
    KEY_UP,
    MOUSE_MOVE,
    MOUSE_DOWN,
    MOUSE_UP,
    MOUSE_WHEEL,
    MOUSE_RELATIVE_MOVE,

    // Server -> Client (VM -> Host)
    COMPRESSED_FRAME
};

// 鼠标按钮枚举
enum class MouseButton : uint32_t {
    LEFT,
    RIGHT,
    MIDDLE
};

// ==========================================================
//                 ===>  重要：先定义枚举 <===
// ==========================================================
// 编码器类型枚举
enum class EncoderType : int {
    JPEG,
    H264
};


#pragma pack(push, 1)

// 通用消息头
struct MessageHeader {
    MessageType type;
    uint32_t payloadSize;
};

// --- Payloads for Client -> Server ---

// ==========================================================
//                ===>  再使用枚举定义结构体 <===
// ==========================================================
// 配置消息的载荷
struct ConfigPayload {
    int jpegQuality;
    int chromaSubsampling;
    int flags;
    int pixelFormat;
    EncoderType encoderType; // <-- 现在编译器知道 EncoderType 是什么了
};


// 键盘事件的载荷
struct KeyboardPayload {
    uint32_t virtualKeyCode;
    uint32_t scanCode;
};

// 鼠标移动事件的载荷
struct MouseMovePayload { // 绝对移动
    float x;
    float y;
};

// 相对移动的载荷
struct MouseRelativeMovePayload {
    int32_t dx;
    int32_t dy;
};

// 鼠标按钮事件的载荷
struct MouseButtonPayload {
    MouseButton button;
};

// 鼠标滚轮事件的载荷
struct MouseWheelPayload {
    float delta;
};

#pragma pack(pop)
