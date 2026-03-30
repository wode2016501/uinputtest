// inject_to_physical.c - 注入到物理触摸屏
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

int main() {
    int physical_fd;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct input_event ev;
    ssize_t bytes_read;
    
    printf("========================================\n");
    printf("远程触摸屏接收端\n");
    printf("========================================\n\n");
    
    // 1. 打开物理触摸屏设备
    physical_fd = open("/dev/input/event6", O_WRONLY);
    if (physical_fd < 0) {
        perror("打开物理触摸屏失败");
        printf("尝试其他设备节点...\n");
        
        // 尝试其他可能的触摸屏设备
        physical_fd = open("/dev/input/event0", O_WRONLY);
        if (physical_fd < 0) {
            perror("打开设备失败");
            return 1;
        }
    }
    printf("✓ 物理触摸屏已打开: /dev/input/event3\n");
    
    // 2. 创建 socket 服务器
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("创建 socket 失败");
        close(physical_fd);
        return 1;
    }
    
    // 设置端口重用
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    // 绑定地址
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("绑定失败");
        close(physical_fd);
        close(server_socket);
        return 1;
    }
    
    // 监听
    if (listen(server_socket, 5) < 0) {
        perror("监听失败");
        close(physical_fd);
        close(server_socket);
        return 1;
    }
    
    printf("等待发送端连接，端口: %d...\n", PORT);
    
    // 接受连接
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("接受连接失败");
        close(physical_fd);
        close(server_socket);
        return 1;
    }
    
    printf("✓ 已连接到发送端: %s\n", inet_ntoa(client_addr.sin_addr));
    printf("开始接收触摸事件并注入到物理触摸屏...\n\n");
    
    // 3. 接收事件并注入
    int event_count = 0;
    while (1) {
        bytes_read = recv(client_socket, &ev, sizeof(ev), 0);
        
        if (bytes_read == sizeof(ev)) {
            // 注入到物理触摸屏
            if (write(physical_fd, &ev, sizeof(ev)) == sizeof(ev)) {
                event_count++;
                
                // 显示事件
                if (ev.type == EV_ABS) {
                    printf("[%d] 触摸坐标: type=%d code=%d value=%d\n", 
                           event_count, ev.type, ev.code, ev.value);
                } else if (ev.type == EV_KEY && ev.code == BTN_TOUCH) {
                    printf("[%d] 触摸状态: %s\n", event_count, ev.value ? "按下" : "抬起");
                } else if (ev.type == EV_SYN) {
                    printf("[%d] 同步\n", event_count);
                }
            } else {
                perror("写入物理触摸屏失败");
            }
        } else if (bytes_read == 0) {
            printf("连接已断开\n");
            break;
        } else if (bytes_read < 0) {
            perror("接收数据失败");
            break;
        }
    }
    
    close(client_socket);
    close(server_socket);
    close(physical_fd);
    
    return 0;
}
