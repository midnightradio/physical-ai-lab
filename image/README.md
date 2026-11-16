# Image · ESP32-S3 카메라 실습

XIAO ESP32S3 Sense 또는 강의의 Freenove 계열 WROOM N16R8 CAM을 USB로 연결해 촬영합니다.
라벨과 촬영 세션을 지정하고 JPG 또는 ZIP으로 저장합니다. Wi-Fi와 SD 카드는 사용하지 않습니다.

## 다운로드와 업로드

1. [이미지 실습 페이지](https://midnightradio.github.io/physical-ai-lab/image/)의 **보드 코드 ZIP 다운로드**를 누릅니다.
2. 압축을 해제하고 `esp32_image_capture/esp32_image_capture.ino`를 Arduino IDE로 엽니다.
3. 보드 매니저에서 **esp32 by Espressif Systems 3.x**를 설치합니다.
4. 아래 보드·메모리·USB 설정을 선택해 업로드합니다.
5. 시리얼 모니터·플로터를 닫고 실습 페이지에서 **보드 연결**을 누릅니다.

`esp_camera.h`는 보드 패키지에 포함됩니다. 별도 카메라 라이브러리는 설치하지 않습니다.
두 보드에 같은 스케치를 사용하며, 보드 선택으로 카메라 핀이 정해집니다.
CameraWebServer나 다른 실습 코드가 올라가 있다면 이 이미지 코드를 먼저 업로드하세요.

## 보드 설정과 카메라

| 항목 | XIAO ESP32S3 Sense | 강의의 WROOM N16R8 CAM |
| --- | --- | --- |
| Arduino 보드 | `XIAO_ESP32S3` | `ESP32S3 Dev Module` |
| Flash Size | 8MB | 16MB |
| PSRAM | OPI PSRAM | OPI PSRAM |
| 카메라 | Sense 확장 보드 | Freenove / ESP32S3_EYE 핀 배선 |

**PSRAM이 활성화되어 있어야 합니다.** Core Debug Level은 None으로 둡니다.
Partition Scheme은 선택한 Flash Size에 맞춥니다.

- 직접 USB: **USB Mode = Hardware CDC and JTAG**, **USB CDC On Boot = Enabled**.
- USB-UART 변환 포트: **USB CDC On Boot = Disabled**.
- 업로드와 촬영에는 같은 USB 포트를 사용합니다.
- 일반 XIAO ESP32S3에는 카메라가 없습니다. Sense 확장 보드가 필요합니다.
- `N16R8`은 메모리 표기이며 모든 제조사의 카메라 배선이 같다는 뜻은 아닙니다.
- WROOM의 앞선 INMP441 배선은 전원을 분리한 뒤 제거합니다. GPIO4·5·6이 카메라와 겹칩니다.
- 카메라 리본 케이블을 장착·제거할 때도 전원을 먼저 분리합니다.

## 촬영과 데이터 저장

1. **미리보기 시작**으로 구도를 확인합니다. 미리보기는 데이터로 저장하지 않습니다.
2. 라벨 버튼 또는 직접 입력으로 라벨을 지정하고 학생/촬영 세션 ID를 입력합니다.
3. **한 장 촬영** 또는 **연속 촬영 시작**을 누릅니다.
4. 사진을 눌러 확인하고 잘못된 사진은 삭제합니다.
5. **전체 JPG를 ZIP으로 저장**을 누릅니다. 사진은 `images/`에 모입니다.

창을 닫거나 새로고침하기 전에 저장하세요. 사진은 서버에 자동 저장되지 않습니다.
학습·테스트용은 수집 후 촬영 세션을 고려해 나눕니다.
Edge Impulse에서 ZIP을 푼 `images` 폴더를 업로드하고 이미지 분류는 파일명으로 라벨을 추론할 수 있습니다.
객체 검출·FOMO는 업로드 후 별도로 객체마다 박스와 라벨을 지정합니다.

실제 보드의 카메라 초기화·촬영·USB 전송과 저장 파일은 실습 전에 확인하세요.
