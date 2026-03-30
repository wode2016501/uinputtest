// android_touch_fixed.c
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 2376

// 发送绝对坐标事件
void send_abs(int fd, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

// 发送按键事件
void send_key(int fd, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

// 发送同步事件
void send_sync(int fd) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

// Android 标准的多点触摸序列
void touch_down(int fd, int id, int x, int y) {
    send_abs(fd, ABS_MT_SLOT, id);
    send_abs(fd, ABS_MT_TRACKING_ID, id + 1);
    send_abs(fd, ABS_MT_POSITION_X, x);
    send_abs(fd, ABS_MT_POSITION_Y, y);
    send_abs(fd, ABS_MT_TOUCH_MAJOR, 10);
    send_abs(fd, ABS_MT_WIDTH_MAJOR, 10);
    send_key(fd, BTN_TOUCH, 1);
    send_sync(fd);
}

void touch_move(int fd, int id, int x, int y) {
    send_abs(fd, ABS_MT_SLOT, id);
    send_abs(fd, ABS_MT_POSITION_X, x);
    send_abs(fd, ABS_MT_POSITION_Y, y);
    send_sync(fd);
}

void touch_up(int fd, int id) {
    send_abs(fd, ABS_MT_SLOT, id);
    send_abs(fd, ABS_MT_TRACKING_ID, -1);
    send_key(fd, BTN_TOUCH, 0);
    send_sync(fd);
}

int main() {
    int fd;
    struct uinput_user_dev uidev;
    
    // 打开 uinput
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    // 启用事件类型
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    
    // 启用绝对坐标
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TOUCH_MAJOR);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_WIDTH_MAJOR);
    
    // 关键：设置 INPUT_PROP_DIRECT 属性，标识为触摸屏
    ioctl(fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);
    
    // 配置设备
    memset(&uidev, 0, sizeof(uidev));
    strcpy(uidev.name, "Virtual Touchscreen");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x0eef;
    uidev.id.product = 0x0001;
    uidev.id.version = 1;
    
    // 设置坐标范围
    uidev.absmin[ABS_MT_POSITION_X] = 0;
    uidev.absmax[ABS_MT_POSITION_X] = SCREEN_WIDTH - 1;
    uidev.absmin[ABS_MT_POSITION_Y] = 0;
    uidev.absmax[ABS_MT_POSITION_Y] = SCREEN_HEIGHT - 1;
    uidev.absmin[ABS_MT_TOUCH_MAJOR] = 0;
    uidev.absmax[ABS_MT_TOUCH_MAJOR] = 255;
    uidev.absmin[ABS_MT_WIDTH_MAJOR] = 0;
    uidev.absmax[ABS_MT_WIDTH_MAJOR] = 255;
    
    write(fd, &uidev, sizeof(uidev));
    ioctl(fd, UI_DEV_CREATE);
    
    printf("虚拟触摸屏已创建 (带 DIRECT 属性)\n");
    printf("屏幕分辨率: %dx%d\n\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    // 等待系统识别
    sleep(2);
    
    // 执行向上滑动
    printf("开始向上滑动...\n");
    
    int start_x = SCREEN_WIDTH / 2;
    int start_y = SCREEN_HEIGHT - 200;
    int end_y = 200;
    int steps = 30;
    
    // 手指按下
    printf("手指按下: (%d, %d)\n", start_x, start_y);
    touch_down(fd, 0, start_x, start_y);
    usleep(100000);
    
    // 向上滑动
    for (int step = 1; step <= steps; step++) {
        int current_y = start_y - (step * (start_y - end_y) / steps);
        printf("滑动: Y = %d\n", current_y);
        touch_move(fd, 0, start_x, current_y);
        usleep(20000);
    }
    
    // 手指抬起
    printf("手指抬起\n");
    touch_up(fd, 0);
    
    printf("滑动完成！\n\n");
    printf("设备保持运行，按 Ctrl+C 退出\n");
    
    // 保持运行
    while (1) {
        sleep(1);
    }
    
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
