# Audio · ESP32-S3 마이크 실습

XIAO ESP32S3 Sense의 PDM 마이크 또는 WROOM 개발 보드에 연결한 INMP441로 녹음합니다.
저장 형식은 **16 kHz · 16비트 · 모노 PCM WAV**입니다. PC 마이크를 사용하는 프로그램이 아닙니다.

## 다운로드와 업로드

1. [오디오 실습 페이지](https://midnightradio.github.io/physical-ai-lab/audio/)의 **보드 코드 ZIP 다운로드**를 누릅니다.
2. 압축을 해제하고 `esp32_audio_capture/esp32_audio_capture.ino`를 Arduino IDE로 엽니다.
3. 보드 매니저에서 **esp32 by Espressif Systems 3.x**를 설치합니다.
4. 아래 보드·USB 설정을 선택해 업로드합니다.
5. 시리얼 모니터·플로터를 닫고 실습 페이지에서 **보드 연결**을 누릅니다.

`ESP_I2S.h`는 보드 패키지에 포함됩니다. 별도 I²S 라이브러리는 설치하지 않습니다.
두 보드에 같은 스케치를 사용하며, 보드 선택으로 마이크 방식이 정해집니다.
다른 실습 코드가 올라가 있다면 오디오 코드를 먼저 업로드하세요.

## 보드 설정

| 항목 | XIAO ESP32S3 Sense | WROOM N16R8 + INMP441 |
| --- | --- | --- |
| Arduino 보드 | `XIAO_ESP32S3` | `ESP32S3 Dev Module` |
| Flash Size | 8MB | 16MB |
| PSRAM | OPI PSRAM 권장 | OPI PSRAM 권장 |
| 마이크 | Sense 확장 보드의 PDM | 외부 INMP441 |

- 직접 USB: **USB Mode = Hardware CDC and JTAG**, **USB CDC On Boot = Enabled**.
- USB-UART 변환 포트: **USB CDC On Boot = Disabled**.
- 업로드와 녹음에 같은 포트를 사용합니다.
- Sense 마이크는 CLK=GPIO42, DATA=GPIO41입니다. 일반 XIAO ESP32S3에는 마이크가 없습니다.

## WROOM과 INMP441 배선

USB 전원을 분리한 상태에서 연결합니다.

| INMP441 | 개발 보드 |
| --- | --- |
| VDD | 3V3 |
| GND | GND |
| SCK / BCLK | GPIO4 |
| WS / LRCLK | GPIO5 |
| SD / DOUT | GPIO6 |
| L/R | GND · 왼쪽 채널 |

카메라와 GPIO4·5·6이 겹치는 강의의 WROOM CAM에서는 전원을 분리한 뒤 카메라 리본 케이블을 제거합니다.
다른 실습에서 쓴 센서 배선도 확인합니다. 마이크 VDD는 3.3V로 연결하고 L/R은 GND에 연결합니다.

## 녹음과 데이터 저장

1. 보드 연결 후 `XIAO_PDM` 또는 `INMP441` 표시를 확인합니다.
2. `yes`·`no`·`silent`·`unknown` 버튼 또는 직접 입력으로 라벨을 지정합니다.
3. 학생/화자 ID를 지정하고 처음에는 녹음 길이를 1초로 선택합니다.
4. **녹음 시작 · 3초 준비**를 누르고 녹음 표시가 뜨면 한 단어를 말합니다.
5. 재생해 확인하고 잘못된 샘플은 삭제합니다.
6. **전체 WAV를 ZIP으로 저장**을 누릅니다. WAV는 `audio/`에 모입니다.

`silent`도 실제 주변 소리를 녹음합니다. 학습·테스트 구분은 수집 후 화자와 세션을 고려해 정리합니다.
Edge Impulse에서 압축을 푼 WAV를 업로드하고 라벨·주파수·길이를 확인합니다.
수집 데이터는 이 창의 메모리에 있으므로 새로고침 전에 저장하세요.

실제 보드의 마이크 음질·배선·USB 전송과 저장 파일은 실습 전에 확인하세요.
