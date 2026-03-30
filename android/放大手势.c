// fixed_center_zoom.c - 修复版中心放大手势
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 2376

// 颜色输出
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_CYAN "\033[1;36m"
#define COLOR_RESET "\033[0m"

void send_event(int fd, int type, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

void send_sync(int fd) {
    send_event(fd, EV_SYN, SYN_REPORT, 0);
}

// 关键修复：一次只处理一个手指的事件序列
void finger_down(int fd, int slot, int id, int x, int y) {
    // 选择槽位
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    // 设置跟踪ID
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    // 设置位置
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    // 重要：每个手指单独同步
    send_sync(fd);
}

// 关键修复：移动手指时只更新坐标，不重复发送跟踪ID
void finger_move(int fd, int slot, int x, int y) {
    // 选择槽位
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    // 只更新位置，不重新发送跟踪ID
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    // 移动后立即同步
    send_sync(fd);
}

// 关键修复：抬起手指
void finger_up(int fd, int slot) {
    // 选择槽位
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    // 设置跟踪ID为-1表示抬起
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    send_sync(fd);
}

// 发送BTN_TOUCH事件（所有手指共享）
void set_touch(int fd, int value) {
    send_event(fd, EV_KEY, BTN_TOUCH, value);
    send_sync(fd);
}

// 双指放大（从中心向外）
void zoom_out(int fd, int center_x, int center_y, int start_dist, int end_dist, int steps) {
    int slot1 = 0, slot2 = 1;
    int id1 = 100, id2 = 200;
    
    printf("  步骤1: 双指按下\n");
    
    // 按下第一个手指
    finger_down(fd, slot1, id1, center_x - start_dist/2, center_y);
    usleep(50000);
    
    // 按下第二个手指
    finger_down(fd, slot2, id2, center_x + start_dist/2, center_y);
    usleep(50000);
    
    // 发送触摸事件（重要：在所有手指按下后发送）
    set_touch(fd, 1);
    usleep(100000);
    
    printf("  步骤2: 向外张开\n");
    
    // 逐步移动手指
    for (int step = 1; step <= steps; step++) {
        int dist = start_dist + (step * (end_dist - start_dist) / steps);
        int offset = dist / 2;
        
        int x1 = center_x - offset;
        int x2 = center_x + offset;
        
        // 移动第一个手指
        finger_move(fd, slot1, x1, center_y);
        usleep(15000);
        
        // 移动第二个手指
        finger_move(fd, slot2, x2, center_y);
        usleep(15000);
        
        if (step % 5 == 0 || step == steps) {
            printf("    进度: %d/%d 间距:%dpx\n", step, steps, dist);
        }
    }
    
    usleep(200000);
}

// 双指缩小（向中心捏合）
void zoom_in(int fd, int center_x, int center_y, int start_dist, int end_dist, int steps) {
    int slot1 = 0, slot2 = 1;
    
    printf("  步骤3: 向内捏合\n");
    
    for (int step = 1; step <= steps; step++) {
        int dist = start_dist - (step * (start_dist - end_dist) / steps);
        int offset = dist / 2;
        
        int x1 = center_x - offset;
        int x2 = center_x + offset;
        
        // 移动第一个手指
        finger_move(fd, slot1, x1, center_y);
        usleep(15000);
        
        // 移动第二个手指
        finger_move(fd, slot2, x2, center_y);
        usleep(15000);
        
        if (step % 5 == 0 || step == steps) {
            printf("    进度: %d/%d 间距:%dpx\n", step, steps, dist);
        }
    }
    
    usleep(200000);
}

int main() {
    int fd;
    struct uinput_user_dev uidev;
    
    // 1. 打开设备
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    // 2. 配置设备能力
    printf(COLOR_GREEN "配置虚拟触摸屏...\n" COLOR_RESET);
    
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    
    // 3. 配置设备参数
    memset(&uidev, 0, sizeof(uidev));
    strcpy(uidev.name, "Virtual Multi-Touch Screen");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x0eef;
    uidev.id.product = 0x0001;
    uidev.id.version = 1;
    
    uidev.absmin[ABS_MT_POSITION_X] = 0;
    uidev.absmax[ABS_MT_POSITION_X] = SCREEN_WIDTH - 1;
    uidev.absmin[ABS_MT_POSITION_Y] = 0;
    uidev.absmax[ABS_MT_POSITION_Y] = SCREEN_HEIGHT - 1;
    uidev.absmin[ABS_MT_SLOT] = 0;
    uidev.absmax[ABS_MT_SLOT] = 9;
    uidev.absmin[ABS_MT_TRACKING_ID] = 0;
    uidev.absmax[ABS_MT_TRACKING_ID] = 65535;
    
    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);
    
    printf(COLOR_GREEN "✓ 虚拟触摸屏创建成功\n" COLOR_RESET);
    printf("  分辨率: %dx%d\n\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    sleep(1);
    
    // 倒计时
    printf(COLOR_YELLOW "倒计时 5 秒...\n" COLOR_RESET);
    for (int i = 5; i > 0; i--) {
        printf("  %d\n", i);
        sleep(1);
    }
    
    printf(COLOR_GREEN "\n开始手势！\n" COLOR_RESET);
    printf("========================================\n\n");
    
    int center_x = SCREEN_WIDTH / 2;
    int center_y = SCREEN_HEIGHT / 2;
    
    // 执行放大手势
    zoom_out(fd, center_x, center_y, 80, 500, 25);
    
    // 等待一下
    usleep(300000);
    
    // 执行缩小手势
    zoom_in(fd, center_x, center_y, 500, 80, 25);
    
    // 抬起所有手指
    printf("  步骤4: 抬起手指\n");
    finger_up(fd, 0);
    usleep(50000);
    finger_up(fd, 1);
    usleep(50000);
    set_touch(fd, 0);
    
    printf(COLOR_GREEN "\n✓ 手势完成！\n" COLOR_RESET);
    printf("========================================\n\n");
    
    printf(COLOR_YELLOW "设备保持运行，按 Ctrl+C 退出\n" COLOR_RESET);
    
    while (1) {
        sleep(1);
    }
    
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
