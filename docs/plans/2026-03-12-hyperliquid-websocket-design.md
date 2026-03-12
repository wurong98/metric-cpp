# Hyperliquid WebSocket 数据源设计

**日期**: 2026-03-12
**目标**: 设计低延迟 C++ WebSocket 客户端，连接 Hyperliquid 永续合约数据流，计算 OFI、Microprice、Depth Imbalance

---

## 1. 需求概述

- **交易标的**: 永续合约 (Perpetual)
- **币种**: 单币种 (如 BTC)
- **延迟要求**: < 1ms
- **输出**: OFI + Microprice + Depth Imbalance
- **输出方式**: printf (后续迭代)

---

## 2. 技术选型

### 方案 1 (推荐): libwebsockets + nlohmann/json

| 组件 | 技术选型 | 理由 |
|------|----------|------|
| WebSocket | libwebsockets | 轻量、成熟、支持 epoll |
| JSON解析 | nlohmann/json | Header-only、易用 |
| YAML配置 | yaml-cpp | C++ YAML 标准库 |
| 编译构建 | CMake | 项目标准 |

---

## 3. 架构设计

```
┌─────────────────┐
│   config.yaml   │  ← 参数配置
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│  Config Loader  │  ← yaml-cpp
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────────────────────────┐
│                    Main Application                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  │
│  │  WebSocket   │  │   Calculator │  │    Logger    │  │
│  │   Client     │  │ (OFI/MP/DI)  │  │              │  │
│  └──────────────┘  └──────────────┘  └──────────────┘  │
└─────────────────────────────────────────────────────────┘
                          │
                          ▼
                    printf 输出
```

---

## 4. YAML 配置设计

```yaml
# Hyperliquid WebSocket 配置
websocket:
  url: "wss://api.hyperliquid.xyz/ws"
  ping_interval: 30
  reconnect_delay: 1
  max_reconnect_attempts: 10

# 交易标的
symbol:
  name: "BTC"
  pair: "BTC-USD-PERP"

# 计算参数
calculator:
  ofi_window: 10        # OFI 滑动窗口大小（笔数）
  depth_levels: 10       # 订单簿深度层级
  smoothing: 0.1        # 平滑因子 (EMA)

# 日志级别: debug, info, warn, error
log_level: "info"
```

---

## 5. 核心计算公式

### Microprice
```
Microprice = mid_price + (bid_vol - ask_vol) / (bid_vol + ask_vol) * spread / 2
```

### Depth Imbalance
```
Depth_Imbalance = (bid_depth - ask_depth) / (bid_depth + ask_depth)
```

### OFI (Order Flow Imbalance)
```
OFI = Σ(buy_volume) - Σ(sell_volume)  (滑动窗口内)
```

---

## 6. Hyperliquid WebSocket 协议

### 6.1 连接信息

- **URL**: `wss://api.hyperliquid.xyz/ws`
- **协议**: WSS (WebSocket Secure)

### 6.2 订阅消息格式

**订阅请求**:
```json
{
  "method": "subscribe",
  "subscription": {
    "type": "<type>",
    "coin": "<coin>"
  }
}
```

**取消订阅**:
```json
{
  "method": "unsubscribe",
  "subscription": {
    "type": "<type>",
    "coin": "<coin>"
  }
}
```

### 6.3 订阅类型

| 类型 | 用途 | 订阅请求 |
|------|------|----------|
| `l2Book` | 订单簿 Level 2 数据 | `{"type": "l2Book", "coin": "BTC"}` |
| `trades` | 成交数据 | `{"type": "trades", "coin": "BTC"}` |
| `allMids` | 所有币种中间价 | `{"type": "allMids", "dex": "PERP"}` |
| `candle` | K线数据 | `{"type": "candle", "coin": "BTC", "interval": "1m"}` |

### 6.4 响应消息格式

**订阅响应**:
```json
{
  "channel": "subscriptionResponse",
  "data": {
    "type": "l2Book",
    "coin": "BTC"
  }
}
```

**错误响应**:
```json
{
  "type": "error",
  "error": {
    "type": "invalid_request_error",
    "message": "错误描述"
  }
}
```

---

## 7. 数据格式详解

### 7.1 订单簿数据 (WsBook)

**订阅类型**: `l2Book`

```json
{
  "channel": "l2Book",
  "data": {
    "coin": "BTC",
    "levels": [
      [
        {"px": "50000.0", "sz": "1.5", "n": 10},
        {"px": "49999.0", "sz": "2.0", "n": 5}
      ],
      [
        {"px": "50001.0", "sz": "1.2", "n": 8},
        {"px": "50002.0", "sz": "3.0", "n": 3}
      ]
    ],
    "time": 1700000000000
  }
}
```

**字段说明**:
| 字段 | 类型 | 说明 |
|------|------|------|
| `levels[0]` | Array | 买单数组 (价格升序) |
| `levels[1]` | Array | 卖单数组 (价格升序) |
| `px` | string | 价格 (字符串，需转换) |
| `sz` | string | 数量 (字符串，需转换) |
| `n` | number | 订单数量 |
| `time` | number | 时间戳 (毫秒) |

### 7.2 交易数据 (WsTrade)

**订阅类型**: `trades`

```json
{
  "channel": "trades",
  "data": {
    "coin": "BTC",
    "trades": [
      {
        "coin": "BTC",
        "side": "Buy",
        "px": "50000.0",
        "sz": "0.5",
        "hash": "0xabc...",
        "time": 1700000000000,
        "tid": 12345,
        "users": ["buyer_address", "seller_address"]
      }
    ]
  }
}
```

**字段说明**:
| 字段 | 类型 | 说明 |
|------|------|------|
| `side` | string | "Buy" 或 "Sell" |
| `px` | string | 价格 |
| `sz` | string | 数量 |
| `time` | number | 时间戳 (毫秒) |
| `tid` | number | 交易 ID |

---

## 8. 数据流设计

```
                    ┌─────────────────────────────────────┐
                    │     Hyperliquid WebSocket Server    │
                    │        wss://api.hyperliquid.xyz/ws │
                    └──────────────────┬──────────────────┘
                                       │
                    ┌──────────────────▼──────────────────┐
                    │        WebSocket Client (lws)       │
                    │  1. connect()                        │
                    │  2. subscribe(l2Book, BTC)          │
                    │  3. subscribe(trades, BTC)          │
                    └──────────────────┬──────────────────┘
                                       │
                    ┌──────────────────▼──────────────────┐
                    │         Message Parser              │
                    │  • 解析 channel 类型                 │
                    │  • 解析 l2Book → OrderBook          │
                    │  • 解析 trades → Trade[]            │
                    └──────────────────┬──────────────────┘
                                       │
                    ┌──────────────────▼──────────────────┐
                    │       Metrics Calculator            │
                    │  • calculate_microprice()          │
                    │  • calculate_depth_imbalance()     │
                    │  • calculate_ofi()                 │
                    └──────────────────┬──────────────────┘
                                       │
                    ┌──────────────────▼──────────────────┐
                    │         printf 输出                  │
                    │  Microprice: 50000.45 | DI: 0.12   │
                    └─────────────────────────────────────┘
```

---

## 9. 实现要点

### 9.1 libwebsockets 回调处理

```cpp
// 回调事件处理
enum lws_callback_reasons {
    LWS_CALLBACK_CLIENT_ESTABLISHED,   // 连接成功
    LWS_CALLBACK_CLIENT_RECEIVE,       // 收到消息
    LWS_CALLBACK_CLIENT_WRITEABLE,     // 可发送消息
    LWS_CALLBACK_CLOSED,               // 连接关闭
    LWS_CALLBACK_WSI_DESTROY,          // 套接字销毁
    // ... 其他事件
};
```

### 9.2 消息解析流程

1. **接收消息**: `LWS_CALLBACK_CLIENT_RECEIVE`
2. **解析 JSON**: 使用 nlohmann/json
3. **检查 channel**: 判断数据类型
4. **解析具体数据**:
   - `l2Book` → 更新 OrderBook
   - `trades` → 更新 trades 队列
5. **触发计算**: 调用 MetricsCalculator

### 9.3 订阅时机

- 连接成功后立即订阅
- 收到 `subscriptionResponse` 确认订阅成功

### 9.4 重连机制

- 监听 `LWS_CALLBACK_CLOSED` 事件
- 指数退避重连 (1s, 2s, 4s, ... 最大 30s)
- 记录重连次数，超过上限退出

---

## 10. 后续迭代

- [ ] 替换 nlohmann/json 为 simdjson 提升性能
- [ ] 替换为共享内存/gRPC 输出
- [ ] 支持多币种
- [ ] 添加内核旁路优化
