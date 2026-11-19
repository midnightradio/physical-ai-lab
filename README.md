# Physical AI Lab

ESP32-S3 모션·이미지·오디오 실습을 위한 웹 수집기와 Arduino 코드입니다.
각 실습 폴더에서 웹페이지, 개발 보드에 업로드할 코드, 학생 안내를 함께 관리합니다.

## 학생용 실습

GitHub Pages 게시 후 [실습 시작 페이지](https://midnightradio.github.io/physical-ai-lab/)에서 실습을 선택하세요.

| 실습 | 보드 코드 | 안내 | 웹 수집기 |
| --- | --- | --- | --- |
| Motion · MPU6050 | [esp32_motion_capture.ino](motion/esp32_motion_capture/esp32_motion_capture.ino) | [모션 안내](motion/README.md) | [모션 실습](https://midnightradio.github.io/physical-ai-lab/motion/) |
| Image · 카메라 | [esp32_image_capture.ino](image/esp32_image_capture/esp32_image_capture.ino) | [이미지 안내](image/README.md) | [이미지 실습](https://midnightradio.github.io/physical-ai-lab/image/) |
| Audio · 마이크 | [보드별 USB·SD 예제 4개](audio/README.md#예제-선택과-다운로드) | [오디오 안내](audio/README.md) | [오디오 실습](https://midnightradio.github.io/physical-ai-lab/audio/) |

1. 실습 페이지에서 **보드 코드 ZIP 다운로드**를 누릅니다.
2. ZIP을 압축 해제하고, 스케치 폴더 안의 같은 이름 `.ino` 파일을 Arduino IDE로 엽니다.
3. 보드와 USB 설정을 선택해 업로드합니다.
4. 시리얼 모니터·플로터를 닫고, 같은 실습 페이지에서 USB 보드를 연결합니다.
5. 데이터를 수집하고 창을 닫기 전에 CSV·JPG·WAV 또는 데이터 ZIP으로 저장합니다.

**보드 코드 ZIP은 ESP32에 업로드할 프로그램입니다.** 수집기에서 저장하는 데이터 ZIP과 구분하세요.
학생 PC에는 Arduino IDE와 PC용 Chrome 또는 Edge가 필요합니다. Python 설치는 필요 없습니다.
실습 종류를 바꾸면 해당 실습의 코드를 다시 업로드합니다. 수집 데이터는 서버에 자동 저장되지 않습니다.

## 저장소 구성

```text
physical-ai-lab/
├── index.html                     # 실습 선택 화면
├── assets/lab.css                 # 공통 화면 스타일
├── motion/
│   ├── index.html                 # 모션 웹 수집기
│   ├── README.md                  # 배선·보드 설정·실습 안내
│   └── esp32_motion_capture/
│       └── esp32_motion_capture.ino
├── image/
│   ├── index.html
│   ├── README.md
│   └── esp32_image_capture/
│       └── esp32_image_capture.ino
├── audio/
│   ├── index.html
│   ├── README.md
│   ├── audio_capture_xiao_sd/
│   ├── audio_capture_xiao_usb/
│   ├── audio_capture_inmp441_usb/
│   ├── audio_capture_inmp441_sd/
│   ├── START_HERE_라이브러리실습.txt
│   └── 검증안내.txt
├── tools/build_site.py             # 웹 배포 파일·코드 ZIP 생성 및 링크 검증
└── .github/workflows/pages.yml     # main에 push하면 ZIP 생성 후 Pages 배포
```

각 `.ino` 파일이 해당 실습 코드의 원본입니다. 배포할 때 원본과 같은 폴더의 README를 묶어 아래 파일을 자동 생성합니다.

- `motion/downloads/motion-code.zip`
- `image/downloads/image-code.zip`
- `audio/downloads/audio-code.zip`

ZIP에는 `README.md`와 각 실습의 스케치 폴더가 들어 있습니다. 오디오는 보드별 USB·SD 예제 4개와 USB 통신 보조 파일 `CollectorLink.h`, 상세 안내를 함께 제공합니다. Arduino IDE가 요구하는 폴더명과 `.ino` 파일명을 유지합니다.
생성 파일은 `_site/`에만 두며, 저장소에는 중복 커밋하지 않습니다.

## GitHub Pages 최초 설정

1. 이 저장소의 **Settings → Pages**를 엽니다.
2. **Build and deployment → Source → GitHub Actions**를 선택합니다.
3. **Actions → Deploy Physical AI Lab → Run workflow**를 실행합니다.
4. 배포 성공 후 실습 주소를 학생에게 안내합니다.

코드 ZIP을 자동 생성하므로 `Deploy from a branch` 대신 **GitHub Actions**를 사용합니다.
그다음부터는 `main`에 push할 때 웹 수집기와 코드 ZIP을 함께 배포합니다.
배포 전에 ZIP 내용이 원본과 일치하는지, 웹페이지의 내부 링크가 연결되는지 검사합니다.
다른 브랜치와 pull request에서는 배포 없이 같은 검사를 수행합니다.

## 교수자 수정 및 로컬 확인

보드 코드는 해당 실습의 `.ino`를, 웹 수집기는 같은 폴더의 `index.html`을 수정합니다.
보드 설정이나 배선이 바뀌면 해당 `README.md`와 페이지의 준비 안내도 함께 갱신하세요.
교수자 PC에 Python 3.9 이상이 있으면 외부 라이브러리 없이 확인할 수 있습니다.

```sh
python3 tools/build_site.py
python3 -m http.server 8080 --directory _site
```

로컬 서버에서 첫 화면, 세 수집기와 코드 ZIP 다운로드를 확인할 수 있습니다.
외부 호스팅은 HTTPS를 사용합니다. LMS에서는 페이지를 직접 여는 링크를 안내하세요.

## 확인 범위

기존 수집기와 보드 코드에 실습 간 이동·코드 다운로드·웹 사용 안내를 추가한 배포 구성입니다.
오디오 예제는 공식 ESP_I2S의 recordWAV() 기반이며, 보드별 USB·SD 구성을 제공합니다. 자세한 사용법과 확인 범위는 각 실습 폴더의 안내를 참고하세요.
실제 보드 업로드, 배선과 수집 상태는 수업 전에 보드별로 확인하세요.
