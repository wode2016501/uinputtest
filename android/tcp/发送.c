// capture_and_send.c - 捕获物理触摸屏并发送
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
#define BUFFER_SIZE 24

int main(int argc, char *argv[]) {
    int physical_fd;
    int sock;
    struct sockaddr_in server_addr;
    struct input_event ev;
    ssize_t bytes_read;
    char *server_ip;
    
    // 检查参数
    if (argc < 2) {
        printf("用法: %s <接收端IP>\n", argv[0]);
        printf("示例: %s 192.168.1.100\n", argv[0]);
        return 1;
    }
    
    server_ip = argv[1];
    
    printf("========================================\n");
    printf("远程触摸屏发送端\n");
    printf("========================================\n\n");
    
    // 1. 打开物理触摸屏设备（只读）
    physical_fd = open("/dev/input/event3", O_RDONLY);
    if (physical_fd < 0) {
        perror("打开物理触摸屏失败");
        printf("尝试其他设备节点...\n");
        
        // 尝试其他可能的触摸屏设备
        physical_fd = open("/dev/input/event0", O_RDONLY);
        if (physical_fd < 0) {
            perror("打开设备失败");
            return 1;
        }
    }
    printf("✓ 物理触摸屏已打开: /dev/input/event3\n");
    
    // 2. 创建 socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("创建 socket 失败");
        close(physical_fd);
        return 1;
    }
    
    // 3. 连接接收端
    memset(&server_addr, 0, sizeof(server_addr));
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
    
    // 4. 读取物理触摸屏事件并发送
    printf("开始捕获触摸事件并发送...\n");
    printf("按 Ctrl+C 停止\n\n");
    
    int event_count = 0;
    while (1) {
        bytes_read = read(physical_fd, &ev, sizeof(ev));
        
        if (bytes_read == sizeof(ev)) {
            // 发送事件
            if (send(sock, &ev, sizeof(ev), 0) == sizeof(ev)) {
                event_count++;
                
                // 显示发送的事件
                if (ev.type == EV_ABS) {
                    printf("[%d] 发送坐标: type=%d code=%d value=%d\n", 
                           event_count, ev.type, ev.code, ev.value);
                } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                    printf("[%d] 触摸状态: %s\n", event_count, ev.value ? "按下" : "抬起");
                }
            } else {
                perror("发送失败");
                break;
            }
        } else if (bytes_read < 0) {
            perror("读取物理触摸屏失败");
            break;
        }
    }
    
    close(physical_fd);
    close(sock);
    
    return 0;
}
