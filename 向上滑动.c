#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

// 触摸屏分辨率
#define SCREEN_MAX_X 1079
#define SCREEN_MAX_Y 2375

// 等待时间（微秒）
#define WAIT_US 10000  // 10毫秒

// 发送绝对坐标事件
void send_abs_event(int fd, int code, int value) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = code;
    ev.value = value;
    write(fd, &ev, sizeof(ev));
}

// 发送按键事件
void send_key_event(int fd, int code, int value) {
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

// 单点向上滑动
void swipe_up(int fd, int start_x, int start_y, int end_y, int steps) {
    printf("开始向上滑动: (%d, %d) -> (%d, %d)\n", start_x, start_y, start_x, end_y);
    
    // 1. 选择触摸槽（单点触摸用 slot 0）
    send_abs_event(fd, ABS_MT_SLOT, 0);
    send_sync(fd);
    
    // 2. 设置跟踪ID（手指标识）
    send_abs_event(fd, ABS_MT_TRACKING_ID, 1);
    send_sync(fd);
    
    // 3. 设置初始位置
    send_abs_event(fd, ABS_MT_POSITION_X, start_x);
    send_abs_event(fd, ABS_MT_POSITION_Y, start_y);
    send_sync(fd);
    
    // 4. 按下手指
    send_key_event(fd, BTN_TOUCH, 1);
    send_sync(fd);
    usleep(WAIT_US);
    
    // 5. 逐步向上滑动
    for (int step = 1; step <= steps; step++) {
        int current_y = start_y - (step * (start_y - end_y) / steps);
        
        // 更新位置
        send_abs_event(fd, ABS_MT_POSITION_X, start_x);
        send_abs_event(fd, ABS_MT_POSITION_Y, current_y);
        send_sync(fd);
        
        printf("滑动中: Y = %d\n", current_y);
        usleep(WAIT_US);
    }
    
    // 6. 抬起手指
    send_abs_event(fd, ABS_MT_TRACKING_ID, -1);
    send_key_event(fd, BTN_TOUCH, 0);
    send_sync(fd);
    
    printf("滑动完成\n");
}

int main() {
    int fd;
    
    // 1. 打开 uinput 设备
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        printf("请确保: sudo modprobe uinput && sudo chmod 666 /dev/uinput\n");
        return 1;
    }
    
    // 2. 设置设备能力
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    
    // 3. 配置设备属性
    struct uinput_setup usetup = {0};
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    strcpy(usetup.name, "Swipe Test Touchscreen");
    
    ioctl(fd, UI_DEV_SETUP, &usetup);
    
    // 配置坐标范围
    struct uinput_abs_setup abs_setup = {0};
    
    abs_setup.code = ABS_MT_POSITION_X;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_X;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_POSITION_Y;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_SLOT;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 9;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_TRACKING_ID;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 65535;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    // 4. 创建设备
    ioctl(fd, UI_DEV_CREATE);
    printf("\n虚拟触摸屏已创建！\n");
    printf("设备名称: Swipe Test Touchscreen\n");
    printf("分辨率: %dx%d\n\n", SCREEN_MAX_X + 1, SCREEN_MAX_Y + 1);
    printf("请运行以下命令查看事件:\n");
    printf("  sudo evtest\n");
    printf("  xinput list\n\n");
    
    // 5. 等待一下让系统识别
    sleep(1);
    
    // 6. 执行向上滑动
    // 从屏幕底部向上滑动到顶部
    int start_x = SCREEN_MAX_X / 2;      // 水平中心
    int start_y = SCREEN_MAX_Y - 200;    // 底部向上200像素
    int end_y = 200;                      // 顶部向下200像素
    int steps = 30;                       // 滑动步数
    
    swipe_up(fd, start_x, start_y, end_y, steps);
    
    // 7. 等待一下
    sleep(2);
    
    // 8. 再次滑动（从中部向上滑）
    printf("\n第二次滑动...\n");
    start_y = SCREEN_MAX_Y / 2;          // 屏幕中部
    end_y = 100;                          // 顶部附近
    swipe_up(fd, start_x, start_y, end_y, steps);
    
    printf("\n滑动演示完成，设备将在5秒后销毁...\n");
    sleep(5);
    
    // 9. 销毁设备
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    
    printf("设备已销毁\n");
    return 0;
}
