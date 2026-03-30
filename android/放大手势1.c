// standard_zoom.c - 标准双指放大手势（从中心向两边）
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

void finger_down(int fd, int slot, int id, int x, int y) {
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, id);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    send_sync(fd);
}

void finger_move(int fd, int slot, int x, int y) {
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    send_event(fd, EV_ABS, ABS_MT_POSITION_X, x);
    send_event(fd, EV_ABS, ABS_MT_POSITION_Y, y);
    send_sync(fd);
}

void finger_up(int fd, int slot) {
    send_event(fd, EV_ABS, ABS_MT_SLOT, slot);
    send_event(fd, EV_ABS, ABS_MT_TRACKING_ID, -1);
    send_sync(fd);
}

void set_touch(int fd, int value) {
    send_event(fd, EV_KEY, BTN_TOUCH, value);
    send_sync(fd);
}

int main() {
    int fd;
    struct uinput_user_dev uidev;
    
    // 打开设备
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    // 配置设备
    printf(COLOR_GREEN "配置虚拟触摸屏...\n" COLOR_RESET);
    
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    
    // 配置设备参数
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
    printf(COLOR_YELLOW "倒计时 5 秒后执行放大手势...\n" COLOR_RESET);
    for (int i = 5; i > 0; i--) {
        printf("  %d\n", i);
        sleep(1);
    }
    
    printf(COLOR_GREEN "\n开始执行放大手势！\n" COLOR_RESET);
    printf("========================================\n\n");
    
    int center_x = SCREEN_WIDTH / 2;      // 540
    int center_y = SCREEN_HEIGHT / 2;     // 1188
    
    // 手势参数
    int start_spacing = 50;       // 初始间距（两指靠近）
    int end_spacing = 500;        // 最终间距（两指张开）
    int steps = 30;               // 移动步数
    
    printf(COLOR_CYAN "步骤1: 双指在中心按下\n" COLOR_RESET);
    printf("  手指1: (%d, %d)\n", center_x - start_spacing/2, center_y);
    printf("  手指2: (%d, %d)\n", center_x + start_spacing/2, center_y);
    printf("  初始间距: %dpx\n\n", start_spacing);
    
    // 按下第一个手指
    finger_down(fd, 0, 100, center_x - start_spacing/2, center_y);
    usleep(50000);
    
    // 按下第二个手指
    finger_down(fd, 1, 200, center_x + start_spacing/2, center_y);
    usleep(50000);
    
    // 发送触摸事件
    set_touch(fd, 1);
    usleep(100000);
    
    printf(COLOR_CYAN "步骤2: 向两边张开 - 放大\n" COLOR_RESET);
    
    // 逐步向外移动
    for (int step = 1; step <= steps; step++) {
        int spacing = start_spacing + (step * (end_spacing - start_spacing) / steps);
        int offset = spacing / 2;
        
        int x1 = center_x - offset;
        int x2 = center_x + offset;
        
        // 移动两个手指
        finger_move(fd, 0, x1, center_y);
        usleep(15000);
        finger_move(fd, 1, x2, center_y);
        usleep(15000);
        
        // 显示进度
        if (step % 5 == 0 || step == steps) {
            printf("  进度: %3d%% [间距: %3dpx] 左指:(%4d,%d) 右指:(%4d,%d)\n", 
                   (step * 100) / steps, spacing, x1, center_y, x2, center_y);
        }
    }
    
    usleep(200000);
    
    printf(COLOR_CYAN "\n步骤3: 保持放大状态\n" COLOR_RESET);
    printf("  手指1: (%d, %d)\n", center_x - end_spacing/2, center_y);
    printf("  手指2: (%d, %d)\n", center_x + end_spacing/2, center_y);
    printf("  最大间距: %dpx\n", end_spacing);
    usleep(500000);
    
    printf(COLOR_CYAN "\n步骤4: 向内捏合 - 缩小\n" COLOR_RESET);
    
    // 逐步向内移动（缩小）
    for (int step = 1; step <= steps; step++) {
        int spacing = end_spacing - (step * (end_spacing - start_spacing) / steps);
        int offset = spacing / 2;
        
        int x1 = center_x - offset;
        int x2 = center_x + offset;
        
        finger_move(fd, 0, x1, center_y);
        usleep(15000);
        finger_move(fd, 1, x2, center_y);
        usleep(15000);
        
        if (step % 5 == 0 || step == steps) {
            printf("  进度: %3d%% [间距: %3dpx] 左指:(%4d,%d) 右指:(%4d,%d)\n", 
                   (step * 100) / steps, spacing, x1, center_y, x2, center_y);
        }
    }
    
    usleep(200000);
    
    printf(COLOR_CYAN "\n步骤5: 抬起手指\n" COLOR_RESET);
    finger_up(fd, 0);
    usleep(50000);
    finger_up(fd, 1);
    usleep(50000);
    set_touch(fd, 0);
    
    printf(COLOR_GREEN "\n========================================\n" COLOR_RESET);
    printf(COLOR_GREEN "✓ 放大手势完成！\n" COLOR_RESET);
    printf("   手势: 中心 → 张开 → 捏合 → 抬起\n");
    printf("   效果: 放大 → 保持 → 缩小\n");
    printf("========================================\n\n");
    
    printf(COLOR_YELLOW "设备保持运行，按 Ctrl+C 退出\n" COLOR_RESET);
    
    while (1) {
        sleep(1);
    }
    
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
