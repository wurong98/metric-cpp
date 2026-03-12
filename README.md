# Hyperliquid 实时指标计算器

> 基于 C++ 的低延迟 WebSocket 客户端，连接 Hyperliquid 永续合约数据流，计算并输出 OFI、Microprice、Depth Imbalance 指标。

## 项目介绍

本项目实现了一个高性能的 C++ 程序，用于连接 Hyperliquid 永续合约的 WebSocket 数据流，实时计算以下指标：

- **OFI (Order Flow Imbalance)**: 订单流不平衡度，衡量买卖双方势力
- **Microprice**: 考虑订单簿深度的微观价格，比中间价更能反映真实价格
- **Depth Imbalance**: 订单簿深度不平衡度

### 技术栈

- **C++17**: 现代 C++ 特性
- **CMake**: 构建系统
- **libwebsockets**: WebSocket 客户端
- **nlohmann/json**: JSON 解析
- **yaml-cpp**: YAML 配置文件解析
- **Google Test**: 单元测试

### 项目结构

```
metric-cpp/
├── CMakeLists.txt           # 顶层 CMake 配置
├── config/
│   └── config.yaml          # 配置文件
├── include/
│   └── metric/
│       ├── calculator/
│       │   └── metrics_calculator.h  # 指标计算器
│       ├── config.h                # 配置结构
│       ├── models/
│       │   └── order_book.h        # 订单簿模型
│       └── websocket/
│           └── client.h             # WebSocket 客户端
├── src/
│   ├── calculator/
│   │   └── metrics_calculator.cpp
│   ├── config/
│   │   └── config_loader.cpp
│   ├── main.cpp               # 主程序
│   ├── models/
│   │   └── order_book.cpp
│   └── websocket/
│       └── client.cpp
├── tests/
│   ├── CMakeLists.txt
│   ├── test_metrics_calculator.cpp
│   └── test_order_book.cpp
└── .gitignore
```

---

## 快速开始

### 1. 环境要求

- Ubuntu 20.04+
- GCC 9.4+
- CMake 3.16+

### 2. 安装依赖

```bash
# 安装系统依赖
sudo apt-get update
sudo apt-get install -y libwebsockets-dev libyaml-cpp-dev libgtest-dev

# 编译 GTest (如果需要)
cd /usr/src/googletest
sudo cmake . && sudo make -j4
sudo cp lib/* /usr/lib/
```

### 3. 编译项目

```bash
cd /home/wurong/workspaces/metric-cpp
mkdir -p build && cd build
cmake .. && make -j4
```

### 4. 运行程序

```bash
# 从项目根目录运行
cd /home/wurong/workspaces/metric-cpp
./build/src/metric_hyperliquid
```

指定配置文件：

```bash
./build/src/metric_hyperliquid config/config.yaml
```

**输出示例：**

```
Loaded config from config/config.yaml
WebSocket URL: wss://api.hyperliquid.xyz/ws
Starting WebSocket client...
Microprice: 50000.4545 | DepthImbalance: -0.0909 | OFI: 0.0000
```

---

## 运行测试

### 运行所有测试

```bash
cd /home/wurong/workspaces/metric-cpp/build
ctest --output-on-failure
```

**输出：**

```
Test project /home/wurong/workspaces/metric-cpp/build
    Start 1: OrderBookTest
1/2 Test #1: OrderBookTest ....................   Passed    0.00 sec
    Start 2: MetricsCalculatorTest
2/2 Test #2: MetricsCalculatorTest ............   Passed    0.00 sec

100% tests passed, 0 tests failed out of 2
```

### 运行单个测试

```bash
# 测试订单簿
./tests/test_order_book

# 测试指标计算器
./tests/test_metrics_calculator
```

---

## 配置文件说明

配置文件位于 `config/config.yaml`：

```yaml
websocket:
  url: "wss://api.hyperliquid.xyz/ws"  # Hyperliquid WebSocket 地址
  ping_interval: 30      # ping 间隔(秒)
  reconnect_delay: 1     # 重连延迟(秒)
  max_reconnect_attempts: 10  # 最大重连次数

symbol:
  name: "BTC"           # 交易对简称
  pair: "BTC-USD-PERP" # 完整交易对标识

calculator:
  ofi_window: 10        # OFI 窗口大小(计算最近 N 笔交易)
  depth_levels: 10      # 深度级别数量(计算深度时考虑的档位数)
  smoothing: 0.1       # EMA 平滑系数(0-1，越小越平滑)

log_level: "info"      # 日志级别: debug, info, warn, error
```

---

## 指标计算说明

### Microprice (微观价格)

考虑订单簿深度不平衡的加权价格：

```
microprice = mid_price + imbalance * spread / 2
```

其中：
- `mid_price = (best_bid + best_ask) / 2`
- `spread = best_ask - best_bid`
- `imbalance = (bid_depth - ask_depth) / (bid_depth + ask_depth)`

使用 EMA 平滑：

```
microprice_ema = smoothing * raw_microprice + (1 - smoothing) * microprice_ema
```

### Depth Imbalance (深度不平衡)

```
depth_imbalance = (bid_depth - ask_depth) / (bid_depth + ask_depth)
```

- 值域：[-1, 1]
- 正值表示买方深度占优
- 负值表示卖方深度占优

### OFI (订单流不平衡)

```
ofi = Σ(买入量) - Σ(卖出量)
```

计算最近 N 笔交易的订单流不平衡。

---

## 当前实现状态

### ✅ 已完成

- [x] 项目基础结构搭建
- [x] 订单簿数据模型 (OrderBook)
- [x] 指标计算器 (MetricsCalculator)
- [x] WebSocket 客户端框架
- [x] 主程序集成
- [x] 单元测试

### 🚧 待改进

- [ ] 完整的 WebSocket 连接实现
- [ ] Hyperliquid 订单簿数据解析
- [ ] Hyperliquid 交易数据解析
- [ ] 实时指标输出
- [ ] 日志系统
- [ ] 性能优化

---

## 常见问题

### 编译失败：找不到 libwebsockets

```bash
sudo apt-get install libwebsockets-dev
```

### 编译失败：找不到 yaml-cpp

```bash
sudo apt-get install libyaml-cpp-dev
```

### 运行失败：配置文件找不到

确保从项目根目录运行程序：

```bash
cd /home/wurong/workspaces/metric-cpp
./build/src/metric_hyperliquid
```

---

## 开发指南

### 添加新模块

1. 在 `include/metric/` 下添加头文件
2. 在 `src/` 下添加实现文件
3. 在 `src/CMakeLists.txt` 中添加源文件

### 添加单元测试

1. 在 `tests/` 下创建测试文件
2. 在 `tests/CMakeLists.txt` 中添加测试目标

### 代码规范

- 使用 C++17 标准
- 遵循 Google Style
- 所有公共接口需要有文档注释
- 单元测试覆盖率应达到 80%+

---

## 许可证

MIT License
