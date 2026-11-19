# Audio · ESP32-S3 오디오 데이터 수집

공식 `ESP_I2S` 라이브러리의 `recordWAV()`로 녹음하고 WAV를 생성합니다.
XIAO ESP32S3 Sense와 WROOM N16R8 CAM + INMP441의 USB·SD 예제를 함께 제공합니다.
파일 형식은 **16 kHz · 16비트 · 모노 PCM WAV**, 웹 수집기의 녹음 길이는 **1·2·3·5·10초**입니다.

## 예제 선택과 다운로드

[오디오 실습 페이지](https://midnightradio.github.io/physical-ai-lab/audio/)에서 **보드 코드 ZIP 다운로드**를 누릅니다.
ZIP의 폴더 구조를 유지하고 아래에서 사용할 예제의 `.ino`를 엽니다.

| 장치 | 저장 방식 | 실습 코드 |
| --- | --- | --- |
| XIAO ESP32S3 Sense | USB → 웹 수집기 | [audio_capture_xiao_usb](audio_capture_xiao_usb/audio_capture_xiao_usb.ino) |
| WROOM CAM + INMP441 | USB → 웹 수집기 | [audio_capture_inmp441_usb](audio_capture_inmp441_usb/audio_capture_inmp441_usb.ino) |
| XIAO ESP32S3 Sense | 내장 microSD | [audio_capture_xiao_sd](audio_capture_xiao_sd/audio_capture_xiao_sd.ino) |
| WROOM CAM + INMP441 | 내장 microSD | [audio_capture_inmp441_sd](audio_capture_inmp441_sd/audio_capture_inmp441_sd.ino) |

**웹 수집기에는 USB 예제를 사용합니다.** SD 예제는 브라우저 없이 시리얼 모니터에서 실행합니다.
USB 예제는 `CollectorLink.h`가 `.ino`와 같은 폴더에 있어야 합니다. 파일 하나만 옮기지 마세요.
이 보조 파일은 USB 연결과 전송을 담당하며, 녹음·WAV 생성은 `.ino`의 공식 `recordWAV()` 호출에서 확인합니다.
이전 공통 `esp32_audio_capture.ino` 대신 위의 보드별 예제를 사용합니다.

## 공통 보드 설정

Arduino IDE의 보드 매니저에서 **esp32 by Espressif Systems 3.3.12**를 설치합니다.
ESP_I2S·SD·SD_MMC는 보드 패키지에 포함됩니다. 별도 오디오 라이브러리는 설치하지 않습니다.

| 항목 | XIAO ESP32S3 Sense | 강의의 WROOM N16R8 CAM |
| --- | --- | --- |
| Arduino 보드 | `XIAO_ESP32S3` | `ESP32S3 Dev Module` |
| Flash Size | 8MB | 16MB |
| PSRAM | **OPI PSRAM** | **OPI PSRAM** |
| 마이크 | Sense의 내장 PDM · CLK42/DATA41 | 외부 INMP441 · GPIO1/2/42 |

- 직접 USB: **USB Mode = Hardware CDC and JTAG**, **USB CDC On Boot = Enabled**.
- USB-UART 포트: **USB CDC On Boot = Disabled**.
- 업로드와 수집에 같은 포트를 사용합니다.
- 일반 XIAO ESP32S3에는 마이크와 SD 슬롯이 없습니다. Sense 확장 보드가 필요합니다.
- 10초 WAV를 메모리에 담을 수 있도록 PSRAM을 활성화합니다.

## WROOM과 INMP441 배선

USB 전원을 분리하고 아래처럼 연결합니다. **이 버전의 마이크 핀은 GPIO1·2·42입니다.**

| INMP441 | 개발 보드 |
| --- | --- |
| VDD | 3V3 |
| GND | GND |
| SCK / BCLK | GPIO1 |
| WS / LRCLK | GPIO2 |
| SD / DOUT | GPIO42 |
| L/R | GND · 왼쪽 채널 |

USB·SD 예제는 같은 배선을 사용합니다. 강의의 CAM 보드에서 카메라 핀 GPIO4·5·6 및
내장 SD 핀 GPIO38·39·40과 겹치지 않도록 옮긴 구성입니다. 카메라 자체를 초기화하지는 않습니다.
다른 센서 배선과 충돌하지 않는지 확인하고, 마이크 VDD에는 3.3V를 공급합니다.

## USB로 웹 수집기에 녹음

1. 자신의 보드에 맞는 **USB 예제**를 업로드합니다.
2. Arduino 시리얼 모니터·플로터를 닫고 [오디오 실습 페이지](https://midnightradio.github.io/physical-ai-lab/audio/)를 PC용 Chrome 또는 Edge로 엽니다.
3. **보드 연결**을 누르고 `XIAO_PDM` 또는 `INMP441` 표시를 확인합니다.
4. 라벨 버튼 또는 직접 입력으로 라벨을 정하고 학생/화자 ID와 녹음 길이를 선택합니다.
5. **녹음 시작 · 3초 준비**를 누릅니다. 화면의 3초 준비 뒤 1초 마이크 예열이 추가됩니다.
6. **녹음 중** 표시가 뜨면 소리를 내고, 재생·파형으로 확인합니다.
7. 개별 WAV 또는 전체 WAV ZIP으로 저장합니다. 모든 WAV는 `audio/`에 모입니다.

`silent`도 실제 주변 소리를 녹음합니다. 학습·테스트 구분은 수집 후 화자와 세션을 고려해 정리합니다.
수집 데이터는 브라우저 메모리에 있으므로 새로고침 전에 저장하세요.

## 내장 microSD에 녹음

FAT32 microSD를 **보드의 내장 슬롯**에 넣고 자신의 보드에 맞는 SD 예제를 업로드합니다.
시리얼 모니터를 **115200 baud**로 열고 `r`을 보냅니다. 시작 안내가 안 보이면 모니터를 연 상태에서 RST를 누릅니다.
예열 후 녹음하고 `/recording_1.wav`, `/recording_2.wav`처럼 다음 번호로 저장합니다.
저장 완료를 확인하고 전원을 끈 뒤 카드를 꺼내 PC에서 재생합니다.

| 보드 | 내부 SD 연결 · 추가 점퍼선 없음 |
| --- | --- |
| XIAO Sense | SPI SD · CS=GPIO21 |
| 강의의 WROOM CAM | SD_MMC 1비트 · CLK=GPIO39, CMD=GPIO38, D0=GPIO40 |

`N16R8`은 메모리 표기입니다. 다른 제조사·형태의 보드는 SD 슬롯과 배선이 다를 수 있습니다.
두 보드의 SD 예제에서 녹음 길이는 다음 상수로 지정합니다. 기본은 3초입니다.

```cpp
const int RECORD_SECONDS = 3;  // 10으로 바꾸면 10초
```

## 확인 범위

이번 GitHub 구성에서는 최신 실습 파일과 수집기를 옮기고, XIAO SD의 녹음 길이를
최근 대화의 `RECORD_SECONDS` 방식으로 맞췄습니다. 실제 보드의 음질·배선·SD·USB 동작은 실습 전에 확인하세요.
