// MPU6050의 가속도 3축과 각속도 3축을 읽어 USB로 보내는 예제
// 라이브러리: Adafruit MPU6050 (의존 라이브러리도 함께 설치)
// XIAO: SDA=D4(GPIO5), SCL=D5(GPIO6) / WROOM: SDA=GPIO7, SCL=GPIO6
// AD0는 GND에 연결합니다. 전원 배선은 학생 안내문을 참고하세요.
// 업로드 후 시리얼 플로터(115200 baud)를 열면 6축 그래프가 나옵니다.
// Chrome 연결 시 CSV로 자동 전환됩니다. 플로터로 돌아올 때는 p를 보내세요.

#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#if defined(ARDUINO_XIAO_ESP32S3)
const int SDA_PIN = 5;
const char BOARD_NAME[] = "XIAO_ESP32S3";
#else
const int SDA_PIN = 7;
const char BOARD_NAME[] = "WROOM_N16R8";
#endif
const int SCL_PIN = 6;
const byte MPU_ADDRESS = 0x68;  // AD0가 3V3이면 0x69로 변경
const int SAMPLE_INTERVAL = 20; // 20ms 간격: 약 50Hz

Adafruit_MPU6050 mpu;
bool collecting = true;
bool plotterMode = true;
unsigned long startTime = 0;
unsigned long lastSampleTime = 0;

void setup() {
  Serial.begin(115200);
  delay(1000);
  Wire.begin(SDA_PIN, SCL_PIN, 400000);

  // 1. MPU6050을 연결하고 측정 범위를 정합니다.
  if (!mpu.begin(MPU_ADDRESS, &Wire)) {
    Serial.println("# ERROR MPU6050 not found. Check wiring and address.");
    while (true) delay(1000);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G); // 가속도 범위: ±8g
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);      // 각속도 범위: ±500도/초
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);  // 센서 잡음을 줄이는 필터
  mpu.setSampleRateDivisor(4);                // 센서 내부 갱신: 200Hz
  lastSampleTime = millis();
}

void loop() {
  readCommand(); // 플로터·브라우저 전환과 시작·중지 (아래 보조 함수)
  delay(1);
  if (!collecting) return;

  // 2. 약 20ms마다 한 번 측정합니다.
  unsigned long now = millis();
  if (now - lastSampleTime < SAMPLE_INTERVAL) return;
  lastSampleTime = now;

  sensors_event_t acceleration, gyro, temperature;
  mpu.getEvent(&acceleration, &gyro, &temperature);

  // 3. 가속도는 m/s² → g, 각속도는 rad/s → 도/초로 바꿉니다.
  float ax = acceleration.acceleration.x / 9.80665;
  float ay = acceleration.acceleration.y / 9.80665;
  float az = acceleration.acceleration.z / 9.80665;
  float gx = gyro.gyro.x * 180.0 / PI;
  float gy = gyro.gyro.y * 180.0 / PI;
  float gz = gyro.gyro.z * 180.0 / PI;

  // 4. 플로터에는 이름:값 형식으로 센서 6축만 보냅니다.
  if (plotterMode) {
    Serial.print("accX_g:"); Serial.print(ax, 5); Serial.print(",");
    Serial.print("accY_g:"); Serial.print(ay, 5); Serial.print(",");
    Serial.print("accZ_g:"); Serial.print(az, 5); Serial.print(",");
    Serial.print("gyrX_dps:"); Serial.print(gx, 5); Serial.print(",");
    Serial.print("gyrY_dps:"); Serial.print(gy, 5); Serial.print(",");
    Serial.print("gyrZ_dps:"); Serial.println(gz, 5);
    return;
  }

  // Chrome에는 시간, 센서 6축, 온도를 기존 CSV 형식으로 보냅니다.
  Serial.print(now - startTime); Serial.print(",");
  Serial.print(ax, 5); Serial.print(",");
  Serial.print(ay, 5); Serial.print(",");
  Serial.print(az, 5); Serial.print(",");
  Serial.print(gx, 5); Serial.print(",");
  Serial.print(gy, 5); Serial.print(",");
  Serial.print(gz, 5); Serial.print(",");
  Serial.println(temperature.temperature, 2);
}

// 아래는 출력 모드 전환과 collector.html 연결용 보조 코드입니다.
// 센서 학습은 위의 setup()과 loop()에 집중하면 됩니다.
void sendReady() {
  Serial.print("# MOTION1 board=");
  Serial.print(BOARD_NAME);
  Serial.println(" sensor=MPU6050 rate=50 accel=g gyro=dps temp=C");
  Serial.println("# READY sample_hz=50");
}

void readCommand() {
  if (!Serial.available()) return;
  char command = Serial.read();
  if (command == '?') {
    plotterMode = false;
    collecting = false;
    sendReady();
  }
  if (command == 's') {
    plotterMode = false;
    collecting = true;
    startTime = millis();
    lastSampleTime = startTime;
    Serial.println("# START");
    Serial.println("timestamp,accX,accY,accZ,gyrX,gyrY,gyrZ,tempC");
  }
  if (command == 'x') {
    collecting = false;
    Serial.println("# STOP");
  }
  if (command == 'p') {
    plotterMode = true;
    collecting = true;
    lastSampleTime = millis();
  }
}
