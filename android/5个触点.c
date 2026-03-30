// max_contacts_test.c - 测试多点触摸
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 2376
#define MAX_CONTACTS 10  // 最大支持10个触摸点

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
    struct uinput_abs_setup abs_setup;
    
    // 打开设备
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    // 配置设备
    printf("配置虚拟触摸屏...\n");
    
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
    strcpy(uidev.name, "Multi-Touch Screen (10 Points)");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor = 0x0eef;
    uidev.id.product = 0x0001;
    uidev.id.version = 1;
    
    uidev.absmin[ABS_MT_POSITION_X] = 0;
    uidev.absmax[ABS_MT_POSITION_X] = SCREEN_WIDTH - 1;
    uidev.absmin[ABS_MT_POSITION_Y] = 0;
    uidev.absmax[ABS_MT_POSITION_Y] = SCREEN_HEIGHT - 1;
    uidev.absmin[ABS_MT_SLOT] = 0;
    uidev.absmax[ABS_MT_SLOT] = MAX_CONTACTS - 1;  // 设置最大触点
    uidev.absmin[ABS_MT_TRACKING_ID] = 0;
    uidev.absmax[ABS_MT_TRACKING_ID] = 65535;
    
    write(fd, &uidev, sizeof(uidev));
    
    // 显式设置触摸槽范围
    memset(&abs_setup, 0, sizeof(abs_setup));
    abs_setup.code = ABS_MT_SLOT;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = MAX_CONTACTS - 1;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    ioctl(fd, UI_DEV_CREATE);
    
    printf("✓ 虚拟触摸屏创建成功\n");
    printf("  最大支持触点: %d\n", MAX_CONTACTS);
    printf("  分辨率: %dx%d\n\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    
    sleep(2);
    
    // 测试多个手指
    printf("测试多点触摸...\n\n");
    
    // 同时按下5个手指
    printf("同时按下5个手指:\n");
    for (int i = 0; i < 5; i++) {
        int x = 100 + i * 200;
        int y = 500 + i * 100;
        printf("  手指%d: (%d, %d)\n", i+1, x, y);
        finger_down(fd, i, i+1, x, y);
        usleep(50000);
    }
    
    // 发送触摸事件
    set_touch(fd, 1);
    usleep(1000000);
    
    // 抬起所有手指
    printf("\n抬起所有手指:\n");
    for (int i = 0; i < 5; i++) {
        printf("  手指%d 抬起\n", i+1);
        finger_up(fd, i);
        usleep(50000);
    }
    set_touch(fd, 0);
    
    printf("\n✓ 测试完成\n");
    printf("设备保持运行，按 Ctrl+C 退出\n");
    
    while (1) {
        sleep(1);
    }
    
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
