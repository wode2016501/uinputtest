#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

// 触摸屏分辨率（适配 1080x2376 设备）
#define SCREEN_MAX_X 1079  // 0-1079，共1080个点
#define SCREEN_MAX_Y 2375  // 0-2375，共2376个点

// 等待时间（微秒）
#define WAIT_US 100000

// 打开 uinput 设备
int open_uinput_device() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("无法打开 /dev/uinput");
        return -1;
    }
    return fd;
}

// 设置设备支持的事件类型
void setup_events(int fd) {
    // 启用按键事件（触摸按键）
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    
    // 启用绝对坐标事件
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    
    // 设置单点触摸的绝对坐标
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_PRESSURE);
    
    // 设置多点触摸（MT - Multi-Touch）
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);           // 触摸槽，用于区分多个手指
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);     // 多点触摸X坐标
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);     // 多点触摸Y坐标
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);    // 手指跟踪ID
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_PRESSURE);       // 多点触摸压力
}

// 配置设备属性
void configure_device(int fd) {
    struct uinput_setup usetup = {0};
    struct uinput_abs_setup abs_setup = {0};
    
    // 设置设备基本信息
    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234;
    usetup.id.product = 0x5678;
    usetup.id.version = 1;
    strcpy(usetup.name, "虚拟多点触摸屏");
    
    ioctl(fd, UI_DEV_SETUP, &usetup);
    
    // 配置绝对坐标范围（单点触摸）
    abs_setup.code = ABS_X;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_X;
    abs_setup.absinfo.fuzz = 0;
    abs_setup.absinfo.flat = 0;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_Y;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    // 配置多点触摸坐标范围
    abs_setup.code = ABS_MT_POSITION_X;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_X;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_POSITION_Y;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = SCREEN_MAX_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    // 配置压力范围
    abs_setup.code = ABS_PRESSURE;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 255;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_PRESSURE;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 255;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    // 配置触摸槽（支持最多10个手指）
    abs_setup.code = ABS_MT_SLOT;
    abs_setup.absinfo.minimum = 0;
    abs_setup.absinfo.maximum = 9;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    // 创建设备
    ioctl(fd, UI_DEV_CREATE);
    printf("虚拟触摸屏设备已创建 (分辨率: %dx%d)\n", SCREEN_MAX_X + 1, SCREEN_MAX_Y + 1);
}

// 发送多点触摸事件
void send_mt_event(int fd, int slot, int tracking_id, int x, int y, int pressure, int touch) {
    struct input_event ev;
    
    // 选择触摸槽（哪个手指）
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_SLOT;
    ev.value = slot;
    write(fd, &ev, sizeof(ev));
    
    // 设置跟踪ID（-1 表示手指离开）
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_TRACKING_ID;
    ev.value = tracking_id;
    write(fd, &ev, sizeof(ev));
    
    if (tracking_id != -1) {
        // 手指按下或移动
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_ABS;
        ev.code = ABS_MT_POSITION_X;
        ev.value = x;
        write(fd, &ev, sizeof(ev));
        
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_ABS;
        ev.code = ABS_MT_POSITION_Y;
        ev.value = y;
        write(fd, &ev, sizeof(ev));
        
        memset(&ev, 0, sizeof(ev));
        ev.type = EV_ABS;
        ev.code = ABS_MT_PRESSURE;
        ev.value = pressure;
        write(fd, &ev, sizeof(ev));
    }
}

// 发送同步事件（告知系统事件包结束）
void sync_events(int fd) {
    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
}

// 模拟放大手势（双指张开）- 适配1080x2376分辨率
void simulate_pinch_gesture(int fd) {
    printf("正在模拟双指缩放手势...\n");
    
    // 屏幕中心点
    int center_x = SCREEN_MAX_X / 2;   // 约 539
    int center_y = SCREEN_MAX_Y / 2;   // 约 1187
    
    // 初始位置：两个手指靠近屏幕中心
    int finger1_x = center_x - 150;
    int finger1_y = center_y;
    int finger2_x = center_x + 150;
    int finger2_y = center_y;
    int distance = finger2_x - finger1_x;  // 初始间距 300
    
    // 手指按下
    printf("手指按下\n");
    send_mt_event(fd, 0, 1, finger1_x, finger1_y, 128, 1);
    send_mt_event(fd, 1, 2, finger2_x, finger2_y, 128, 1);
    sync_events(fd);
    usleep(WAIT_US);
    
    // 向外张开（放大）- 分15步移动
    printf("向外张开（放大）...\n");
    for (int step = 1; step <= 15; step++) {
        int new_distance = distance + (step * 30);  // 每次增加30，最终间距 300+450=750
        int offset = new_distance / 2;
        
        finger1_x = center_x - offset;
        finger2_x = center_x + offset;
        
        printf("第 %d 步：间距 = %d\n", step, new_distance);
        
        send_mt_event(fd, 0, 1, finger1_x, finger1_y, 128, 1);
        send_mt_event(fd, 1, 2, finger2_x, finger2_y, 128, 1);
        sync_events(fd);
        usleep(WAIT_US / 2);
    }
    
    // 保持最大间距一会儿
    usleep(WAIT_US);
    
    // 向内捏合（缩小）- 分15步移动
    printf("向内捏合（缩小）...\n");
    for (int step = 15; step >= 0; step--) {
        int new_distance = distance + (step * 30);
        int offset = new_distance / 2;
        
        finger1_x = center_x - offset;
        finger2_x = center_x + offset;
        
        printf("第 %d 步：间距 = %d\n", 15 - step, new_distance);
        
        send_mt_event(fd, 0, 1, finger1_x, finger1_y, 128, 1);
        send_mt_event(fd, 1, 2, finger2_x, finger2_y, 128, 1);
        sync_events(fd);
        usleep(WAIT_US / 2);
    }
    
    // 手指抬起
    printf("手指抬起\n");
    send_mt_event(fd, 0, -1, 0, 0, 0, 0);
    send_mt_event(fd, 1, -1, 0, 0, 0, 0);
    sync_events(fd);
    
    printf("双指缩放手势完成\n");
}

// 模拟完整的多点触摸手势（带轨迹）- 适配1080x2376分辨率
void simulate_advanced_gesture(int fd) {
    printf("\n=== 高级多点触摸手势 ===\n");
    
    // 第一个手指：从左上移动到右下
    // 第二个手指：从右上移动到左下（对角线交叉）
    
    // 手指按下
    send_mt_event(fd, 0, 100, 100, 100, 128, 1);           // 手指1：左上角附近
    send_mt_event(fd, 1, 101, SCREEN_MAX_X - 100, 100, 128, 1);  // 手指2：右上角附近
    sync_events(fd);
    usleep(WAIT_US);
    
    // 移动手指
    printf("交叉移动中...\n");
    for (int step = 1; step <= 20; step++) {
        int progress_x = step * (SCREEN_MAX_X / 20);  // 每次移动约54像素
        int progress_y = step * (SCREEN_MAX_Y / 20);  // 每次移动约119像素
        
        // 手指1：从左上到右下
        int f1_x = 100 + progress_x;
        int f1_y = 100 + progress_y;
        
        // 手指2：从右上到左下
        int f2_x = (SCREEN_MAX_X - 100) - progress_x;
        int f2_y = 100 + progress_y;
        
        // 确保坐标不超出边界
        if (f1_x > SCREEN_MAX_X) f1_x = SCREEN_MAX_X;
        if (f1_y > SCREEN_MAX_Y) f1_y = SCREEN_MAX_Y;
        if (f2_x < 0) f2_x = 0;
        if (f2_y > SCREEN_MAX_Y) f2_y = SCREEN_MAX_Y;
        
        send_mt_event(fd, 0, 100, f1_x, f1_y, 128, 1);
        send_mt_event(fd, 1, 101, f2_x, f2_y, 128, 1);
        sync_events(fd);
        usleep(WAIT_US / 4);
    }
    
    // 手指抬起
    printf("手指抬起\n");
    send_mt_event(fd, 0, -1, 0, 0, 0, 0);
    send_mt_event(fd, 1, -1, 0, 0, 0, 0);
    sync_events(fd);
    
    printf("高级手势完成\n");
}

// 模拟单点触摸滑动
void simulate_swipe_gesture(int fd) {
    printf("\n=== 单点滑动手势 ===\n");
    
    // 从底部向上滑动
    int start_x = SCREEN_MAX_X / 2;     // 水平中心
    int start_y = SCREEN_MAX_Y - 200;   // 底部向上200像素
    int end_y = 200;                    // 顶部向下200像素
    
    printf("从底部向上滑动...\n");
    
    // 手指按下
    send_mt_event(fd, 0, 200, start_x, start_y, 128, 1);
    sync_events(fd);
    usleep(WAIT_US / 2);
    
    // 滑动
    int steps = 20;
    for (int step = 1; step <= steps; step++) {
        int current_y = start_y - (step * (start_y - end_y) / steps);
        send_mt_event(fd, 0, 200, start_x, current_y, 128, 1);
        sync_events(fd);
        usleep(WAIT_US / 4);
    }
    
    // 手指抬起
    send_mt_event(fd, 0, -1, 0, 0, 0, 0);
    sync_events(fd);
    
    printf("滑动完成\n");
}

int main() {
    int fd;
    
    // 打开 uinput 设备
    fd = open_uinput_device();
    if (fd < 0) {
        return 1;
    }
    
    // 设置设备能力
    setup_events(fd);
    
    // 配置设备属性
    configure_device(fd);
    
    printf("\n虚拟触摸屏已就绪！\n");
    printf("分辨率: %dx%d\n", SCREEN_MAX_X + 1, SCREEN_MAX_Y + 1);
    printf("使用以下命令验证：sudo evtest\n");
    printf("或：xinput list\n\n");
    
    // 模拟放大手势
    simulate_pinch_gesture(fd);
    
    usleep(WAIT_US);
    
    // 模拟滑动手势
    simulate_swipe_gesture(fd);
    
    usleep(WAIT_US);
    
    // 模拟更复杂的手势
    simulate_advanced_gesture(fd);
    
    printf("\n手势模拟完成。设备将在5秒后销毁...\n");
    sleep(50);
    
    // 销毁设备
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    
    printf("设备已销毁\n");
    return 0;
}
