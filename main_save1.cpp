#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// ------------------- 引脚定义 (新增) -------------------
#define PIN_SENSOR_1 20
#define PIN_SENSOR_2 21
#define PIN_SENSOR_3 47
#define PIN_SENSOR_4 48
#define PIN_SCAN_CTRL 45

// ------------------- 用户可配置参数 -------------------

// 收到任务(ID=1)后，语音播报完毕等待多久开始执行任务 (单位: 毫秒)
const unsigned long DELAY_BEFORE_TASK_START = 15000; 

// 收到复位(ID=2)后，语音播报完毕等待多久恢复默认全开 (单位: 毫秒)
const unsigned long DELAY_BEFORE_RESET = 1000;

// ------------------- GBK 编码字符串定义 -------------------
// 使用十六进制转义，确保发送给语音模块的是标准的 GBK 字节流

const char* GBK_EMERGENCY_STOP = "\xBD\xF4\xBC\xB1\xCD\xA3\xD6\xB9";          // 紧急停止
const char* GBK_ENTER_INIT     = "\xBD\xF8\xC8\xEB\xB3\xF5\xCA\xBC\xD7\xB4\xCC\xAC"; // 进入初始状态
const char* GBK_RECV_TASK      = "\xCA\xD5\xB5\xBD\xC8\xCE\xCE\xF1";          // 收到任务
const char* GBK_DI             = "\xB5\xDA";                                  // 第
const char* GBK_BU             = "\xB2\xBD";                                  // 步
const char* GBK_CHANNEL        = "\xCD\xA8\xB5\xC0";                          // 通道
const char* GBK_DURATION       = "\xCA\xB1\xB3\xA4";                          // 时长
const char* GBK_SECOND         = "\xC3\xEB";                                  // 秒
const char* GBK_COMMA          = "\xA3\xAC";                                  // ，(中文逗号)
const char* GBK_PERIOD         = "\xA3\xAE";                                  // 。(中文句号)
const char* GBK_DRUG_ID        = "\xD2\xA9\xC6\xB7ID";                        // 药品ID
const char* GBK_NURSE_ID       = "\xBB\xA4\xCA\xBFID";                        // 护士ID
const char* GBK_TIME           = "\xCA\xB1\xBC\xE4";                          // 时间
const char* GBK_MINUTE         = "\xB7\xD6";                                  // 分
const char* GBK_COLON          = "\xA3\xBA";                                  // ：(中文冒号)

// --- 新增 GBK 常量 (用户需求) ---
const char* GBK_PAUSE          = "\xD4\xDD\xCD\xA3";                          // 暂停
const char* GBK_RESUME         = "\xBC\xCC\xD0\xF8\xCA\xE4\xD2\xBA";          // 继续输液
const char* GBK_ERROR          = "\xD2\xEC\xB3\xA3";                          // 异常
const char* GBK_SETTING        = "\xC9\xE8\xD6\xC3";                          // 设置

// --------------------------------------------------------

WiFiClient espClient;
PubSubClient client(espClient);

// 定义定时器句柄
hw_timer_t * timer = NULL;

// ------------------- 数据结构定义 -------------------

struct InfusionTask {
  bool isRunning;           // 任务是否正在运行
  bool isPaused;            // (新增) 任务是否暂停
  bool isDefaultOpen;       // 是否处于“默认全开”模式
  uint8_t executionOrder[4];// 执行顺序
  uint32_t durations[4];    // 每个通道对应的时长
  int currentStepIndex;     // 当前步骤
  volatile uint32_t remainingTime; // 倒计时
} taskMgr;

// 定义待执行动作类型（用于非阻塞延时）
enum PendingActionType {
  ACTION_NONE,
  ACTION_RESET,      // 待执行：恢复默认全开
  ACTION_START_TASK  // 待执行：开启新任务
};

struct PendingAction {
  PendingActionType type;
  unsigned long triggerTime; // 触发的具体时间点 (millis)
} pendingAction;

// ------------------- 业务逻辑控制 -------------------

// 停止所有任务逻辑 (不影响全开标志)
void stopTaskLogic() {
  taskMgr.isRunning = false;
  taskMgr.isPaused = false;
  taskMgr.remainingTime = 0;
  taskMgr.currentStepIndex = -1;
}

// 恢复到默认全开状态 (实际执行函数)
void resetToDefaultOpen() {
  stopTaskLogic();
  taskMgr.isDefaultOpen = true; // 激活全开模式
  Serial.println("Mode Switched: DEFAULT OPEN (All channels ON)");
}

// 紧急停止 (id=0) - 立即执行
void emergencyStop() {
  taskMgr.isDefaultOpen = false; // 关闭全开
  stopTaskLogic();               // 停止任务
  
  // 取消任何正在倒计时的待执行动作
  pendingAction.type = ACTION_NONE;
  
  Serial.println("!!! EMERGENCY STOP: ALL OFF !!!");
}

// 暂停任务 (ID=3 或 水位异常)
void pauseTask(bool isSensorError, int errorChannel = 0) {
  if (taskMgr.isRunning && !taskMgr.isPaused) {
    taskMgr.isPaused = true;
    Serial.println("Task PAUSED.");
    
    // 语音播报
    if (isSensorError) {
        // "通道X异常"
        String msg = String(GBK_CHANNEL) + String(errorChannel) + String(GBK_ERROR);
        Serial2.println(msg);
        Serial.printf("Sensor Error on Ch %d\n", errorChannel);
    } else {
        // "暂停"
        Serial2.println(GBK_PAUSE);
    }
  }
}

// 继续任务 (ID=4)
void resumeTask() {
  if (taskMgr.isRunning && taskMgr.isPaused) {
    taskMgr.isPaused = false;
    Serial.println("Task RESUMED.");
    // "继续输液"
    Serial2.println(GBK_RESUME);
  }
}

// ------------------- 定时器中断 -------------------

void IRAM_ATTR onTimer() {
  // 只有在运行且未暂停时才倒计时
  if (taskMgr.isRunning && !taskMgr.isPaused && taskMgr.remainingTime > 0) {
    taskMgr.remainingTime--;
  }
}

// 执行任务中的某一步
void executeCurrentStep() {
  int channelId = taskMgr.executionOrder[taskMgr.currentStepIndex];
  uint32_t duration = taskMgr.durations[channelId - 1];
  taskMgr.remainingTime = duration;
  Serial.printf("Task Step %d: Channel %d for %d sec\n", 
                taskMgr.currentStepIndex, channelId, duration);
}

// 任务流转逻辑
void processInfusionLogic() {
  if (!taskMgr.isRunning) return;
  if (taskMgr.isPaused) return; // 暂停时不流转
  if (taskMgr.remainingTime > 0) return;

  taskMgr.currentStepIndex++;

  if (taskMgr.currentStepIndex >= 4) {
    Serial.println("Task sequence finished.");
    taskMgr.isRunning = false;
    taskMgr.currentStepIndex = -1;
    return;
  }
  
  executeCurrentStep();
}

// 检测水位传感器 (优化版：只检测已设置任务的通道)
void checkSensors() {
  // 如果已经处于暂停状态，或者任务根本没在运行，则不重复检测
  if (taskMgr.isPaused || !taskMgr.isRunning) return;

  // 这里的引脚数组方便循环遍历
  const int sensorPins[4] = {PIN_SENSOR_1, PIN_SENSOR_2, PIN_SENSOR_3, PIN_SENSOR_4};

  for (int i = 0; i < 4; i++) {
    // 逻辑变更：只有当该通道的时长 > 0 (表示已设置任务) 时，才检测该引脚
    if (taskMgr.durations[i] > 0) {
      if (digitalRead(sensorPins[i]) == HIGH) { // HIGH 表示无水/异常
        // 触发暂停，传入通道编号 (i+1)
        pauseTask(true, i + 1);
        return; // 发现一个异常就退出循环，避免多次播报
      }
    }
  }
}

// 处理非阻塞延时的动作
void handlePendingActions() {
  if (pendingAction.type == ACTION_NONE) return;

  if (millis() >= pendingAction.triggerTime) {
    if (pendingAction.type == ACTION_RESET) {
      resetToDefaultOpen();
    } 
    else if (pendingAction.type == ACTION_START_TASK) {
      Serial.println("Delay finished. Starting Task...");
      taskMgr.currentStepIndex = 0;
      taskMgr.isRunning = true;
      taskMgr.isPaused = false;
      executeCurrentStep();
    }
    pendingAction.type = ACTION_NONE;
  }
}

// ------------------- 串口指令解析 -------------------

String formatTimeVoice(uint32_t totalSeconds) {
  int mins = totalSeconds / 60;
  int secs = totalSeconds % 60;
  String res = "";
  if (mins > 0) {
    res += String(mins) + String(GBK_MINUTE);
  }
  res += String(secs) + String(GBK_SECOND);
  return res;
}

// 监听 Serial (USB/Debug口) 的输入指令 - 扫码枪模拟
void handleSerialInput() {
  if (Serial2.available()) {
    String input = Serial2.readStringUntil('\n');
    input.trim(); 

    if (input.length() == 0) return;

    // 指令 1: 添加/设置任务
    if (input.startsWith("command:add")) {
      
      // --- 新增: Pin 45 跳变逻辑 (重启扫码模块) ---
      // 拉高 -> 延时 -> 拉低 (恢复默认LOW)
      // 注意：这里用 blocking delay 200ms 是为了保证脉冲宽度，对于用户交互影响不大
      digitalWrite(PIN_SCAN_CTRL, HIGH);
      delay(200);
      digitalWrite(PIN_SCAN_CTRL, LOW);
      // ------------------------------------------

      int timeIdx = input.indexOf("time:");
      int idIdx = input.indexOf("id:");
      
      if (timeIdx > 0 && idIdx > 0) {
        int commaAfterTime = input.indexOf(',', timeIdx);
        if (commaAfterTime == -1) commaAfterTime = input.length();
        String timeStr = input.substring(timeIdx + 5, commaAfterTime);
        uint32_t timeVal = timeStr.toInt();

        String idStr = input.substring(idIdx + 3);
        int commaAfterId = idStr.indexOf(',');
        if (commaAfterId != -1) idStr = idStr.substring(0, commaAfterId);

        // 寻找空闲通道
        int targetIndex = -1;
        for (int i = 0; i < 4; i++) {
          if (taskMgr.durations[i] == 0) {
            targetIndex = i;
            break;
          }
        }

        if (targetIndex != -1) {
          taskMgr.durations[targetIndex] = timeVal;
          if (taskMgr.executionOrder[targetIndex] == 0) {
             taskMgr.executionOrder[targetIndex] = targetIndex + 1;
          }
          
          Serial.printf("Added to Channel %d: Time=%d, ID=%s\n", targetIndex + 1, timeVal, idStr.c_str());

          // --- 修改: 语音播报包含当前通道 ---
          // "设置通道X，药品ID：xxx，时间xxx"
          String voiceMsg = String(GBK_SETTING) + String(GBK_CHANNEL) + String(targetIndex + 1) + String(GBK_COMMA);
          voiceMsg += String(GBK_DRUG_ID) + String(GBK_COLON) + idStr + String(GBK_COMMA);
          voiceMsg += String(GBK_TIME) + formatTimeVoice(timeVal);
          
          Serial2.println(voiceMsg);
        } else {
          Serial.println("Full! No empty channels available.");
        }
      }
    }
    // 指令 2: 开始任务
    else if (input.startsWith("command:start")) {
      int idIdx = input.indexOf("id:");
      if (idIdx > 0) {
        String idStr = input.substring(idIdx + 3);
        Serial.println("Start Command Received via Serial.");
        
        taskMgr.isDefaultOpen = false;
        taskMgr.isRunning = true;
        taskMgr.isPaused = false;
        taskMgr.currentStepIndex = 0;
        
        for(int i=0; i<4; i++) {
           if(taskMgr.executionOrder[i] == 0) taskMgr.executionOrder[i] = i+1;
        }
        executeCurrentStep();

        String voiceMsg = String(GBK_NURSE_ID) + String(GBK_COLON) + idStr;
        Serial2.println(voiceMsg);
      }
    }
  }
}

// ------------------- MQTT & JSON 解析 -------------------

void parseJsonCommand(String message) {
  JsonDocument doc; 
  DeserializationError error = deserializeJson(doc, message);
  if (error) {
    Serial.println("JSON Parse Error");
    return;
  }

  const char* idStr = doc["id"]; 
  
  if (strcmp(idStr, "0") == 0) { // 紧急停止
    Serial2.println(GBK_EMERGENCY_STOP);
    emergencyStop();
  } 
  else if (strcmp(idStr, "2") == 0) { // 复位
    Serial2.println(GBK_ENTER_INIT);
    pendingAction.type = ACTION_RESET;
    pendingAction.triggerTime = millis() + DELAY_BEFORE_RESET;
  } 
  else if (strcmp(idStr, "1") == 0) { // 新任务
    Serial.println("New Task Received.");
    taskMgr.isDefaultOpen = false; 
    stopTaskLogic();
    pendingAction.type = ACTION_NONE;
    
    JsonArray channel = doc["channel"];
    JsonArray times = doc["time"];
    
    if (channel.size() == 4 && times.size() == 4) {
      String voiceMsg = String(GBK_RECV_TASK) + String(GBK_PERIOD);
      for(int i=0; i<4; i++) {
        taskMgr.executionOrder[i] = channel[i];
        taskMgr.durations[i] = times[i];
        
        voiceMsg += String(GBK_DI) + String(i + 1) + String(GBK_BU);
        voiceMsg += String(GBK_COMMA);
        voiceMsg += String(GBK_CHANNEL) + String((int)channel[i]);
        voiceMsg += String(GBK_COMMA);
        voiceMsg += String(GBK_DURATION) + String((int)times[i]) + String(GBK_SECOND);
        voiceMsg += String(GBK_PERIOD);
      }
      Serial2.println(voiceMsg);
      pendingAction.type = ACTION_START_TASK;
      pendingAction.triggerTime = millis() + DELAY_BEFORE_TASK_START;
    }
  }
  // --- 新增 MQTT 命令 ---
  else if (strcmp(idStr, "3") == 0) { // 暂停
    pauseTask(false); // false = 不是传感器错误，是手动暂停
  }
  else if (strcmp(idStr, "4") == 0) { // 继续
    resumeTask();
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) message += (char)payload[i];
  if (strcmp(topic, "/command") == 0) parseJsonCommand(message);
}

// ------------------- Setup & Loop -------------------

void setup() {
  Serial.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 16, 17);
  Serial2.begin(9600, SERIAL_8N1, 38, 39);
  
  // --- 新增引脚初始化 ---
  // 水位传感器: 上拉输入，悬空时为HIGH(无水)，下拉为LOW(有水)
  pinMode(PIN_SENSOR_1, INPUT_PULLUP);
  pinMode(PIN_SENSOR_2, INPUT_PULLUP);
  pinMode(PIN_SENSOR_3, INPUT_PULLUP);
  pinMode(PIN_SENSOR_4, INPUT_PULLUP);

  // 扫码控制脚: 默认为低
  pinMode(PIN_SCAN_CTRL, OUTPUT);
  digitalWrite(PIN_SCAN_CTRL, LOW);
  // ---------------------
  
  pendingAction.type = ACTION_NONE;
  memset(&taskMgr, 0, sizeof(taskMgr));

  resetToDefaultOpen();

  timer = timerBegin(0, 80, true);
  timerAttachInterrupt(timer, &onTimer, true);
  timerAlarmWrite(timer, 1000000, true);
  timerAlarmEnable(timer);

  WiFi.begin("FakeGDUT", "gdut_404");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".....");
    delay(500); 
  }
  Serial.println("WiFi connected");
  client.setServer("8.138.244.66", 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    if (client.connect("ESP_MQTT", "ESP_MQTT", "ESP_MQTT")) {
      client.subscribe("/command");
    }
  }
  client.loop();    

  handlePendingActions();
  handleSerialInput();
  checkSensors();       // 新增：检测水位
  processInfusionLogic();

  // --- Pin 45 控制逻辑 ---
  // "当执行任务时45号引脚始终为高电平"
  // "暂停所有任务" -> 意味着不执行 -> 低电平
  // "默认为拉低" -> 非运行状态为低
  if (taskMgr.isRunning && !taskMgr.isPaused) {
    digitalWrite(PIN_SCAN_CTRL, HIGH);
  } else {
    // 只有在非运行状态下才允许为 LOW
    // 注意：如果是串口设置触发的 LOW->HIGH->LOW 脉冲已经在 handleSerialInput 里的 delay 处理了
    digitalWrite(PIN_SCAN_CTRL, LOW);
  }

  // 每500ms同步状态
  static unsigned long lastUpdate = 0;
  if (millis() - lastUpdate >= 500) {
    lastUpdate = millis();
    
    // 计算通道开关状态
    bool chStatus[4] = {false, false, false, false};
    int currentActiveCh = (taskMgr.isRunning && taskMgr.currentStepIndex >= 0) 
                          ? taskMgr.executionOrder[taskMgr.currentStepIndex] : 0;

    // 如果暂停了，强制全部关闭 (False)
    // 如果没暂停，才根据逻辑判断
    if (!taskMgr.isPaused) {
      for (int i = 0; i < 4; i++) {
        if (taskMgr.isDefaultOpen || (taskMgr.isRunning && currentActiveCh == (i + 1))) {
          chStatus[i] = true;
        }
      }
    }

    // MQTT 上报
    JsonDocument statusDoc;
    statusDoc["id"] = "1";
    JsonObject bottles = statusDoc.createNestedObject("bolltes");
    bottles["status1"] = chStatus[0] ? "open" : "close";
    bottles["status2"] = chStatus[1] ? "open" : "close";
    bottles["status3"] = chStatus[2] ? "open" : "close";
    bottles["status4"] = chStatus[3] ? "open" : "close";
    
    // statusNow 逻辑：如果暂停，报 PAUSE，否则正常报
    if (taskMgr.isPaused) {
      bottles["statusNow"] = "PAUSE";
    } else {
      bottles["statusNow"] = taskMgr.isDefaultOpen ? "ALL" : String(currentActiveCh);
    }
    bottles["timeLeft"] = taskMgr.remainingTime;

    String output;
    serializeJson(statusDoc, output);
    client.publish("/data", output.c_str());

    // 串口下发
    for (int i = 0; i < 4; i++) {
      uint8_t packet[6] = {0xAA, 0x55, 0x02, (uint8_t)i, (uint8_t)(chStatus[i] ? 1 : 0), 0x0D};
      Serial1.write(packet, 6); 
      delay(10); 
    }
  }
  delay(10);
}