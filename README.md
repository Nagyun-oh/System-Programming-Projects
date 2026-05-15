# System Programming Projects

시스템 프로그래밍 수업에서 진행한 Proxy Server 기반 과제들을 정리한 저장소입니다.

본 프로젝트는 Linux 환경에서 C언어를 사용하여 Proxy Cache Server를 단계적으로 구현하며, 파일 시스템 처리, 소켓 프로그래밍, 프로세스 생성, 시그널 처리, 세마포어 기반 동기화, 스레드 생성 및 관리까지 실습하는 것을 목표로 합니다.

## Overview

이 프로젝트는 단순한 파일 생성 및 URL 해시 처리부터 시작하여, 실제 웹 브라우저와 웹 서버 사이에서 동작하는 Proxy Server를 구현하는 방향으로 확장됩니다.

주요 구현 내용은 다음과 같습니다.

- URL을 SHA-1로 해싱하여 Cache 파일 경로 생성
- Cache HIT / MISS 판별 및 로그 기록
- Web Browser와 Proxy Server 간 HTTP request 처리
- Proxy Server와 Web Server 간 HTTP response 중계
- `fork()`를 활용한 다중 프로세스 기반 요청 처리
- `signal()`을 활용한 `SIGCHLD`, `SIGALRM`, `SIGINT` 처리
- `Semaphore`를 활용한 Critical Section 동기화
- `pthread`를 활용한 Thread 기반 로그 기록

## Tech Stack

| Category | Tech |
|---|---|
| Language | C |
| Build | Makefile |
| OS | Linux / Ubuntu |
| Network | Socket Programming |
| Process | `fork()`, `waitpid()` |
| Signal | `SIGCHLD`, `SIGALRM`, `SIGINT` |
| Synchronization | Semaphore |
| Thread | POSIX Thread |
| Hash | SHA-1 |
| Browser Test | Firefox |

## Project Structure

```text
System-Programming-Projects/
├── Proxy1-1/
├── Proxy1-2/
├── Proxy1-3/
├── Proxy2-1/
├── Proxy2-2/
├── Proxy2-3/
├── Proxy2-4/
├── Proxy3-1/
└── Proxy3-2/
```

## Assignments

| Assignment | Description |
|---|---|
| Proxy1-1 | URL을 SHA-1로 해싱하고, 해시값을 기반으로 Cache 디렉토리 및 파일 경로를 생성 |
| Proxy1-2 | 입력된 URL에 대해 Cache HIT / MISS 여부를 판별하고 로그 파일에 기록 |
| Proxy1-3 | Cache 관리 기능을 확장하고, HIT / MISS 처리 흐름을 개선 |
| Proxy2-1 | Socket을 활용하여 Client와 Server 간 기본 통신 구조 구현 |
| Proxy2-2 | Web Browser로부터 HTTP request를 수신하고 request header에서 URL 정보 추출 |
| Proxy2-3 | Proxy Server가 Web Server에 HTTP request를 전달하고, HTTP response를 Browser에 중계 및 Cache에 저장 |
| Proxy2-4 | `SIGINT` 신호를 처리하여 Proxy Server 종료 시 전체 실행 시간과 Child Process 수를 로그에 기록 |
| Proxy3-1 | Semaphore를 사용하여 여러 프로세스가 공유 로그 파일에 접근할 때 발생하는 Race Condition 해결 |
| Proxy3-2 | Critical Section 내부에서 Thread를 생성하여 로그를 기록하고, Process와 Thread의 동작 관계 확인 |

## Main Features

### 1. Hash-based Cache Structure

입력받은 URL을 SHA-1 알고리즘으로 해싱하여 고유한 해시값을 생성합니다.  
생성된 해시값을 기반으로 Cache 디렉토리와 파일을 구성하여, 동일한 URL 요청이 들어왔을 때 기존 Cache를 재사용할 수 있도록 구현했습니다.

```text
URL → SHA-1 Hash → Cache Directory / Cache File
```

### 2. Cache HIT / MISS

브라우저로부터 요청받은 URL이 기존 Cache 파일에 존재하면 `HIT`, 존재하지 않으면 `MISS`로 판단합니다.

- `HIT`: Cache 파일을 읽어 Browser에 전달
- `MISS`: Web Server에 직접 요청 후 response를 Browser에 전달하고 Cache에 저장

```text
Request URL
    ├── Cache Exists → HIT
    └── Cache Not Exists → MISS
```

### 3. Multi-process Proxy Server

Proxy Server는 웹 브라우저의 연결 요청을 수신한 뒤, 각 요청마다 `fork()`를 사용하여 Child Process를 생성합니다.

Parent Process는 계속해서 새로운 연결을 대기하고, Child Process는 실제 HTTP request 처리, Cache 확인, Web Server 연결, response 전달 등의 작업을 수행합니다.

```text
Parent Process
    ├── accept()
    ├── fork()
    └── wait for next connection

Child Process
    ├── read HTTP request
    ├── parse URL
    ├── check cache
    ├── connect web server if MISS
    └── send response to browser
```

### 4. Signal Handling

Proxy Server 동작 중 발생하는 다양한 Signal을 처리했습니다.

| Signal | Description |
|---|---|
| `SIGCHLD` | Child Process 종료 처리 |
| `SIGALRM` | Web Server 응답 지연 시 timeout 처리 |
| `SIGINT` | `Ctrl + C` 입력 시 Proxy Server 종료 처리 |

특히 `SIGINT`를 catch하여 서버가 종료될 때 전체 실행 시간과 처리한 Child Process 수를 `logfile.txt`에 기록하도록 구현했습니다.

### 5. Semaphore Synchronization

여러 Child Process가 동시에 `logfile.txt`에 접근하면 Race Condition이 발생할 수 있습니다.  
이를 해결하기 위해 Semaphore를 사용하여 한 번에 하나의 Process만 Critical Section에 진입하도록 구현했습니다.

```text
P operation → Lock
Critical Section
V operation → Unlock
```

이를 통해 공유 자원인 로그 파일에 대한 동시 접근 문제를 제어할 수 있었습니다.

### 6. Thread-based Logging

마지막 단계에서는 Critical Section 내부에서 로그 기록을 담당하는 Thread를 생성했습니다.

`pthread_create()`와 `pthread_join()`을 사용하여 Thread를 생성하고 종료를 기다리며, Process 내부에서 Thread가 어떤 방식으로 실행되는지 확인했습니다.

이를 통해 Process와 Thread의 관계, Thread ID 출력, Thread 종료 시점 등을 직접 확인할 수 있었습니다.

## Execution

각 과제 폴더로 이동한 뒤 Makefile을 사용하여 빌드할 수 있습니다.

```bash
cd Proxy3-2
make
```

실행 예시는 다음과 같습니다.

```bash
./proxy_cache
```

## Test Environment

- Ubuntu Linux
- GCC
- Make
- Firefox Browser
- HTTP 기반 테스트 사이트

> 본 프로젝트는 수업 과제 목적의 HTTP Proxy Server 구현이므로, HTTPS 요청은 주요 구현 대상이 아닙니다.

## What I Learned

이 프로젝트를 통해 시스템 프로그래밍의 여러 핵심 개념을 단계적으로 학습할 수 있었습니다.

- Linux 파일 시스템과 권한 처리
- URL 해싱 기반 Cache 구조 설계
- Socket 기반 네트워크 프로그래밍
- HTTP request / response 처리 흐름
- `fork()` 기반 다중 프로세스 구조
- Signal handler를 이용한 비동기 이벤트 처리
- Semaphore를 활용한 Race Condition 해결
- POSIX Thread를 활용한 Thread 생성 및 동기화
- Web Browser가 하나의 페이지를 로딩할 때 여러 HTTP request를 병렬적으로 발생시킨다는 점

특히 Proxy Server가 단순히 요청과 응답을 전달하는 프로그램이 아니라, 프로세스 관리, 파일 시스템, 네트워크 통신, 동기화, 시그널 처리 등 다양한 시스템 프로그래밍 개념이 결합된 구조라는 것을 이해할 수 있었습니다.

## Repository Purpose

이 저장소는 시스템 프로그래밍 수업에서 수행한 Proxy Server 과제를 정리하기 위한 목적입니다.  
각 과제별 README에는 구현 내용, Flow Chart, Pseudo Code, 실행 결과, Discussion을 정리하여 학습 과정과 구현 흐름을 함께 기록했습니다.

## Author

- Oh Nagyun
- Kwangwoon University
- Computer Engineering
