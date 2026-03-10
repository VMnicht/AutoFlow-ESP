# 代码规范（S3_WIFI）

## 1. 目标与适用范围
本规范用于 2–3 人协作、业务简单的嵌入式项目，核心目标：
- 功能解耦：彻底避免“所有业务挤在一个文件”
- 并行开发：每人独立改动不同模块，减少冲突
- 可维护：新成员 5 分钟读懂结构，10 分钟提交首个 PR

---

## 2. 统一目录结构（≤3层）与命名规则

### 2.1 目录树（立即可用）
```text
S3_WIFI/
├─ include/
│  ├─ app/
│  │  └─ app.h
│  ├─ core/
│  │  ├─ task_engine.h
│  │  └─ command_parser.h
│  ├─ drivers/
│  │  ├─ board_io.h
│  │  └─ timer_tick.h
│  └─ services/
│     ├─ mqtt_client.h
│     └─ voice_serial.h
├─ src/
│  ├─ main.cpp
│  ├─ app/
│  │  └─ app.cpp
│  ├─ core/
│  │  ├─ task_engine.cpp
│  │  └─ command_parser.cpp
│  ├─ drivers/
│  │  ├─ board_io.cpp
│  │  └─ timer_tick.cpp
│  └─ services/
│     ├─ mqtt_client.cpp
│     └─ voice_serial.cpp
├─ test/
│  └─ unit/
│     ├─ test_task_engine.cpp
│     └─ test_command_parser.cpp
├─ tools/
│  ├─ cppcheck/
│  │  └─ suppressions.txt
│  └─ misra/
│     └─ misra_required_rules.md
├─ platformio.ini
└─ 代码规范.md
```

### 2.2 命名规则
- 文件名：全小写+下划线，如 `task_engine.cpp`
- 头/源同名：`xxx.h` + `xxx.cpp`
- 模块名直读即义：`mqtt_client`、`board_io`，禁止 `manager2`、`utils_final`
- 每个模块仅暴露 1 个公开头文件（放在 `include/`）
- 模块内部私有函数、私有常量仅在 `.cpp` 内部 `static`/匿名命名空间可见

---

## 3. 模块职责拆分（最小必要）

- `app`：系统编排层（启动、循环调度、模块连接）
- `core/task_engine`：任务状态机（运行/暂停/恢复/复位/步进/倒计时）
- `core/command_parser`：指令解析（串口命令、MQTT JSON 转内部事件）
- `drivers/board_io`：引脚读写、传感器读取、通道输出、扫描脉冲
- `drivers/timer_tick`：1s Tick 中断与计时基准
- `services/mqtt_client`：MQTT 收发与重连（仅做通信，不含业务）
- `services/voice_serial`：语音串口播报、扫码串口输入读取（仅 I/O，不含业务）

### 2–3 人并行建议
- A（业务）：`core/task_engine` + `core/command_parser`
- B（外设）：`drivers/board_io` + `drivers/timer_tick`
- C（通信与集成）：`services/*` + `app/*`
- 冲突边界：只在 `app.cpp` 汇合，避免多人同时改 `main.cpp`

---

## 4. 头文件模板规范

### 4.1 防重复包含
必须使用工程前缀 Include Guard：
- 规则：`S3_WIFI_<DIR>_<FILE>_H_`
- 示例：`S3_WIFI_CORE_TASK_ENGINE_H_`

### 4.2 extern "C" 规则
- C++ 头文件默认**不**加 `extern "C"`
- 仅当该头需要同时被 C 与 C++ 调用时，使用：

```c
#ifdef __cplusplus
extern "C" {
#endif

/* C ABI declarations */

#ifdef __cplusplus
}
#endif
```

### 4.3 include 顺序
每个 `.cpp` 文件必须按顺序包含：
1. 自身头文件（第一位）
2. 同层模块头
3. 跨层公开头
4. 第三方库头（Arduino/WiFi/PubSubClient/ArduinoJson）
5. 标准库头

---

## 5. 接口设计原则（强制）

- 禁止全局变量  
  - 状态通过 `struct Context` 由 `app` 持有并显式传递
- 禁止跨层直接调用  
  - `core` 不得直接调用 `services`  
  - `services` 不得修改 `core` 内部状态  
  - 统一由 `app` 做事件转发
- ISR 守则  
  - ISR 只做：置标志/计数器自减  
  - ISR 禁止：`Serial.print`、动态内存、JSON 解析、阻塞延时
- 接口最小化  
  - 模块公开函数不超过“初始化 + 输入事件 + 周期处理 + 查询状态”四类
- 错误处理  
  - 所有对外接口返回明确状态码（OK/ERR_xxx）

---

## 6. 编码风格

- 缩进：2 空格，不使用 Tab
- 大括号：K&R 风格（函数与控制语句同一行开括号）
- 命名：
  - 类型：`PascalCase`（如 `InfusionTask`）
  - 函数：`lower_snake_case`
  - 常量：`UPPER_SNAKE_CASE`
  - 私有静态变量：`s_` 前缀
- 注释：
  - 只写“为什么”，不写“做了什么”
  - 每个模块头部写职责说明，函数注释只写边界条件
- 魔法值处理：
  - 禁止裸字面量直接出现于逻辑中
  - 引脚号、超时、协议字段统一收敛到模块常量区

---

## 7. 模块依赖图与允许方向

### 7.1 依赖图（单向）
```text
main.cpp
  ↓
app
  ↓
core      services      drivers
  ↓           ↓            ↓
         （不得反向依赖）
```

### 7.2 允许依赖
- `main -> app`
- `app -> core/services/drivers`
- `services -> drivers`（仅 I/O 适配场景可选）
- `core -> (none)` 或 `core -> core/common_types`

### 7.3 禁止依赖
- `core -> services`
- `drivers -> core/services/app`
- 任意跨层“越级调用”

---

## 8. 单元测试与静态检查准入标准

### 8.1 单元测试（必须）
- 至少覆盖：
  - 任务步进与结束
  - 暂停/恢复状态转换
  - 指令解析成功/失败分支
- 最低门槛：
  - 语句覆盖率 ≥ 70%
  - 核心状态机分支覆盖率 ≥ 90%

### 8.2 cppcheck（必须）
- 命令（Windows PowerShell）：
```powershell
cppcheck .\src .\include --enable=warning,style,performance,portability --std=c++17 --error-exitcode=2 --inline-suppr --suppressions-list=.\tools\cppcheck\suppressions.txt
```
- 准入标准：
  - error = 0
  - warning = 0
  - 未在 suppressions 明确登记的问题不得忽略

### 8.3 MISRA-C（必须过项清单）
至少满足以下检查项（以工具可映射规则为准）：
- Rule 2.2：无死代码
- Rule 8.4：外部可见函数先声明后定义
- Rule 8.7：仅内部使用对象必须内部链接
- Rule 8.13：只读参数应加 `const`
- Rule 13.2：表达式求值次序不依赖副作用
- Rule 17.7：不得忽略有返回值函数结果
- Rule 21.x：禁止不安全库接口（按工具映射执行）

---

## 9. Git 提交格式与分支模型

### 9.1 分支模型
- 长期分支：`main`、`dev`
- 功能分支：`feat-xxx`
- 规则：
  - 禁止直接推送 `main`
  - 功能分支从 `dev` 拉出，PR 合入 `dev`
  - 发布时由管理员 `dev -> main`

### 9.2 提交信息格式
- 格式：`type(scope): subject`
- type：`feat | fix | refactor | test | chore`
- 示例：
  - `feat(task_engine): add pause-resume state transition`
  - `fix(mqtt_client): reconnect on broker timeout`

### 9.3 PR 准入
- 通过编译 + 单测 + cppcheck + MISRA 必过项
- 至少 1 人 Review
- 禁止“巨型 PR”（建议单 PR 只改 1 个模块）

---

## 10. 30 分钟迁移现有单文件工程（可执行清单）

### 第 1 步：建目录与空壳（5 分钟）
- 创建 `include/src/test/tools` 目录树
- `main.cpp` 只保留 `setup/loop` 调用 `app_init/app_loop`

### 第 2 步：抽离 core（10 分钟）
- 从现有文件迁出：
  - 任务结构体与状态机逻辑 -> `core/task_engine`
  - 串口/MQTT 命令解析 -> `core/command_parser`
- 目标：`core` 不依赖硬件库，只处理纯业务状态

### 第 3 步：抽离 drivers/services（10 分钟）
- 引脚、传感器、扫描脉冲、串口下发 -> `drivers/board_io`
- 定时器 ISR -> `drivers/timer_tick`
- MQTT 连接与订阅发布 -> `services/mqtt_client`
- 语音串口输入输出 -> `services/voice_serial`

### 第 4 步：app 编排接线（5 分钟）
- `app.cpp` 内完成事件流转：
  - 输入（MQTT/Serial）→ parser → task_engine
  - task_engine 状态 → board_io/mqtt 上报

---

## 11. 迁移后验证步骤（编译/下载/运行）

### 11.1 编译
```powershell
pio run
```

### 11.2 下载
```powershell
pio run -t upload
```

### 11.3 串口观察
```powershell
pio device monitor -b 115200
```

### 11.4 运行确认（最小验收）
- 上电后打印：`WiFi connected`
- 复位指令后打印：`Mode Switched: DEFAULT OPEN`
- 下发开始任务后：
  - `PIN_SCAN_CTRL` 出现高电平工作态
  - 任务结束后出现一次脉冲跳变
- 若有板载 LED：
  - 在 `app_loop` 每 500ms 翻转一次作为心跳
  - 心跳存在即证明主循环未阻塞

---

## 12. 新成员上手路径（5 分钟读懂，10 分钟首 PR）
- 先读本文件第 2、3、7、9 章
- 仅选择 1 个模块改动（例如 `core/task_engine`）
- 本地通过第 11 章全部检查
- 按第 9 章提交 `feat-xxx` 分支并发起 PR