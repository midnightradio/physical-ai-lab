/*
  ESP32-S3-WROOM-1 N16R8 CAM + INMP441 + 보드 뒷면 microSD 슬롯
  시리얼 모니터 115200에서 r을 보내면 녹음하여 SD 카드에 저장합니다.
  보드: ESP32S3 Dev Module / esp32 3.3.12 / PSRAM: OPI PSRAM
  INMP441: 3V3, GND, SCK=1, WS=2, SD=42, L/R=GND
  내장 microSD: CLK=39, CMD=38, D0=40 / SD_MMC 1비트 모드
  카메라는 초기화하지 않습니다. 마이크 핀은 카메라·SD 핀과 겹치지 않습니다.
*/
#include <ESP_I2S.h>
#include <SD_MMC.h>

const int RECORD_SECONDS = 3;  // 녹음 시간입니다. 10으로 바꾸면 10초입니다.
const int SD_CLK = 39;
const int SD_CMD = 38;
const int SD_D0 = 40;

I2SClass microphone;
bool ready = false;
int fileNumber = 1;

void setup() {
  Serial.begin(115200);
  delay(500);
  if (psramFound() == false) {
    Serial.println("PSRAM을 OPI PSRAM으로 설정해서 업로드하세요.");
    return;
  }

  // 보드 내부 배선입니다. 외부 점퍼선을 연결하지 않습니다.
  SD_MMC.setPins(SD_CLK, SD_CMD, SD_D0);
  if (SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT) == false) {
    Serial.println("보드 뒷면 SD 카드와 FAT32 포맷을 확인하세요.");
    return;
  }

  microphone.setPins(1, 2, -1, 42);  // BCLK, WS, 미사용 출력, 마이크 입력
  microphone.setTimeout((RECORD_SECONDS + 2) * 1000);
  ready = true;
  Serial.println("r을 보내면 녹음하여 SD 카드에 저장합니다.");
}

void loop() {
  if (ready && Serial.available() && Serial.read() == 'r') {
    recordToSd();
  }
  delay(10);
}

void recordToSd() {
  if (microphone.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT,
                       I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT) == false) {
    microphone.end();
    Serial.println("마이크 시작에 실패했습니다.");
    return;
  }
  // INMP441의 32비트 입력을 라이브러리에서 16비트로 변환합니다.
  if (microphone.configureRX(16000, I2S_DATA_BIT_WIDTH_32BIT,
      I2S_SLOT_MODE_MONO, I2S_RX_TRANSFORM_32_TO_16) == false) {
    microphone.end();
    Serial.println("마이크 입력 변환에 실패했습니다.");
    return;
  }

  size_t wavBytes = 0;
  Serial.println("마이크 예열 중...");
  uint8_t *warmup = microphone.recordWAV(1, &wavBytes);
  if (warmup == nullptr) {
    microphone.end();
    Serial.println("마이크 읽기에 실패했습니다.");
    return;
  }
  free(warmup);

  Serial.println("지금부터 녹음합니다.");
  uint8_t *wav = microphone.recordWAV(RECORD_SECONDS, &wavBytes);
  microphone.end();
  if (wav == nullptr) {
    Serial.println("녹음에 실패했습니다.");
    return;
  }

  // 기존 파일을 덮어쓰지 않도록 다음 번호를 찾습니다.
  String path = "/recording_" + String(fileNumber) + ".wav";
  while (SD_MMC.exists(path)) {
    fileNumber = fileNumber + 1;
    path = "/recording_" + String(fileNumber) + ".wav";
  }
  File file = SD_MMC.open(path, FILE_WRITE);
  if (file == false) {
    free(wav);
    Serial.println("파일을 열 수 없습니다.");
    return;
  }
  size_t written = file.write(wav, wavBytes);
  file.close();
  free(wav);

  if (written == wavBytes) {
    Serial.print("저장 완료: ");
    Serial.println(path);
  } else {
    Serial.println("SD 카드에 저장하는 중 오류가 발생했습니다.");
  }
}
