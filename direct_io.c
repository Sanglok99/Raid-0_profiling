#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef O_DIRECT
#  define O_DIRECT 040000
#endif

#define ALIGN 4096
#define SIZE  4096

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

int main(void) {
    const char *path = "/mnt/test/a.txt";  // 고정 경로

    // 4KB 정렬 버퍼 준비 (O_DIRECT 요구사항: 메모리/크기/오프셋 정렬)
    void *buf = NULL;
    int rc = posix_memalign(&buf, ALIGN, SIZE);
    if (rc != 0) {
        fprintf(stderr, "posix_memalign: %s\n", strerror(rc));
        return EXIT_FAILURE;
    }
    memset(buf, 'A', SIZE);

    // 파일 열기: 버퍼드 I/O 금지 (O_DIRECT), 없으면 생성, 기존 내용 제거
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, 0644);
    if (fd < 0) die("open");

    // 오프셋 0도 4KB 배수라서 OK
    size_t to_write = SIZE;
    unsigned char *p = (unsigned char *)buf;
    while (to_write > 0) {
        ssize_t n = write(fd, p, to_write);
        if (n < 0) {
            if (errno == EINTR) continue; // 신호 재시도
            // 일부 파일시스템은 O_DIRECT 미지원 -> EINVAL 가능
            die("write");
        }
        p += n;
        to_write -= (size_t)n;
    }

    // 디스크 동기화 (선택적이지만 안전하게)
//    if (fsync(fd) < 0) die("fsync");

    if (close(fd) < 0) die("close");
    free(buf);
    return 0;
}

