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

## 6. 数据流

1. WebSocket 连接到 Hyperliquid
2. 订阅 orderbook 和 trades 数据
3. 解析 JSON 更新本地订单簿状态
4. 计算 OFI / Microprice / Depth Imbalance
5. printf 输出结果

---

## 7. 后续迭代

- [ ] 替换 nlohmann/json 为 simdjson 提升性能
- [ ] 替换为共享内存/gRPC 输出
- [ ] 支持多币种
- [ ] 添加内核旁路优化
