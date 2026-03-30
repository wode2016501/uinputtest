// sender_configurable.c - 可配置分辨率的发送端
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 6666

// 分辨率配置（从命令行读取或使用默认值）
int sender_width = 1080;
int sender_height = 2376;
int receiver_width = 720;
int receiver_height = 1600;

// 坐标转换函数
int map_x(int x) {
    return x * receiver_width / sender_width;
}

int map_y(int y) {
    return y * receiver_height / sender_height;
}

void print_usage(char *prog) {
    printf("用法: %s <接收端IP> [选项]\n", prog);
    printf("\n选项:\n");
    printf("  -sw <宽>   发送端宽度 (默认: 1080)\n");
    printf("  -sh <高>   发送端高度 (默认: 2376)\n");
    printf("  -rw <宽>   接收端宽度 (默认: 720)\n");
    printf("  -rh <高>   接收端高度 (默认: 1600)\n");
    printf("\n示例:\n");
    printf("  %s 192.168.1.100\n", prog);
    printf("  %s 192.168.1.100 -sw 1080 -sh 2376 -rw 720 -rh 1600\n", prog);
    printf("  %s 192.168.1.100 -sw 720 -sh 1600 -rw 1080 -sh 2376\n", prog);
}

int main(int argc, char *argv[]) {
    int physical_fd;
    int sock;
    struct sockaddr_in server_addr;
    struct input_event ev;
    char *server_ip = NULL;
    int event_count = 0;
    
    // 解析参数
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    server_ip = argv[1];
    
    // 解析可选参数
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-sw") == 0 && i+1 < argc) {
            sender_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-sh") == 0 && i+1 < argc) {
            sender_height = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-rw") == 0 && i+1 < argc) {
            receiver_width = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-rh") == 0 && i+1 < argc) {
            receiver_height = atoi(argv[++i]);
        }
    }
    
    printf("========================================\n");
    printf("远程触摸屏发送端（带坐标转换）\n");
    printf("========================================\n\n");
    printf("分辨率映射:\n");
    printf("  发送端: %dx%d\n", sender_width, sender_height);
    printf("  接收端: %dx%d\n", receiver_width, receiver_height);
    printf("  转换比例: X: %.2f%%, Y: %.2f%%\n", 
           (float)receiver_width/sender_width*100,
           (float)receiver_height/sender_height*100);
    printf("\n");
    
    // 打开物理触摸屏
    physical_fd = open("/dev/input/event3", O_RDONLY);
    if (physical_fd < 0) {
        perror("打开物理触摸屏失败");
        printf("尝试 /dev/input/event0...\n");
        physical_fd = open("/dev/input/event0", O_RDONLY);
        if (physical_fd < 0) {
            perror("打开设备失败");
            return 1;
        }
    }
    printf("✓ 物理触摸屏已打开\n");
    
    // 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("创建 socket 失败");
        close(physical_fd);
        return 1;
    }
    
    // 连接接收端
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    
    printf("正在连接到 %s:%d...\n", server_ip, PORT);
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("连接失败");
        close(physical_fd);
        close(sock);
        return 1;
    }
    printf("✓ 已连接到接收端\n\n");
    
    printf("开始发送触摸事件（已转换坐标）...\n");
    printf("按 Ctrl+C 停止\n\n");
    
    // 读取并发送事件
    while (1) {
        if (read(physical_fd, &ev, sizeof(ev)) != sizeof(ev)) {
            break;
        }
        
        struct input_event send_ev = ev;
        
        // 转换坐标
        if (ev.type == EV_ABS) {
            if (ev.code == ABS_MT_POSITION_X || ev.code == ABS_X) {
                send_ev.value = map_x(ev.value);
                printf("[%d] X: %d -> %d\n", ++event_count, ev.value, send_ev.value);
            } 
            else if (ev.code == ABS_MT_POSITION_Y || ev.code == ABS_Y) {
                send_ev.value = map_y(ev.value);
                printf("[%d] Y: %d -> %d\n", event_count, ev.value, send_ev.value);
            }
        }
        
        // 发送
        if (send(sock, &send_ev, sizeof(send_ev), 0) != sizeof(send_ev)) {
            perror("发送失败");
            break;
        }
    }
    
    close(physical_fd);
    close(sock);
    
    return 0;
}
