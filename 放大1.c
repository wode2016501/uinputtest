#include <linux/uinput.h>
#include <linux/input.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SCREEN_MAX_X 1079
#define SCREEN_MAX_Y 2375

int main() {
    int fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    // 设置基本能力
    ioctl(fd, UI_SET_EVBIT, EV_KEY);
    ioctl(fd, UI_SET_KEYBIT, BTN_TOUCH);
    ioctl(fd, UI_SET_EVBIT, EV_ABS);
    ioctl(fd, UI_SET_ABSBIT, ABS_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_X);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_POSITION_Y);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_SLOT);
    ioctl(fd, UI_SET_ABSBIT, ABS_MT_TRACKING_ID);
    
    // 配置设备
    struct uinput_setup usetup = {0};
    usetup.id.bustype = BUS_USB;
    strcpy(usetup.name, "Test Touchscreen");
    ioctl(fd, UI_DEV_SETUP, &usetup);
    
    struct uinput_abs_setup abs_setup = {0};
    abs_setup.code = ABS_X;
    abs_setup.absinfo.maximum = SCREEN_MAX_X;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_Y;
    abs_setup.absinfo.maximum = SCREEN_MAX_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_POSITION_X;
    abs_setup.absinfo.maximum = SCREEN_MAX_X;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    abs_setup.code = ABS_MT_POSITION_Y;
    abs_setup.absinfo.maximum = SCREEN_MAX_Y;
    ioctl(fd, UI_ABS_SETUP, &abs_setup);
    
    ioctl(fd, UI_DEV_CREATE);
    printf("设备已创建！PID: %d\n", getpid());
    printf("请运行: sudo evtest\n");
    
    // 发送一个简单的触摸事件
    struct input_event ev;
    
    // 选择触摸槽
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_SLOT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
    
    // 设置跟踪ID
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_TRACKING_ID;
    ev.value = 1;
    write(fd, &ev, sizeof(ev));
    
    // 设置坐标（屏幕中心）
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_POSITION_X;
    ev.value = SCREEN_MAX_X / 2;
    write(fd, &ev, sizeof(ev));
    
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_POSITION_Y;
    ev.value = SCREEN_MAX_Y / 2;
    write(fd, &ev, sizeof(ev));
    
    // 触摸按下
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = BTN_TOUCH;
    ev.value = 1;
    write(fd, &ev, sizeof(ev));
    
    // 同步
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
    
    printf("已发送触摸事件，等待3秒...\n");
    sleep(3);
    
    // 抬起手指
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_ABS;
    ev.code = ABS_MT_TRACKING_ID;
    ev.value = -1;
    write(fd, &ev, sizeof(ev));
    
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_KEY;
    ev.code = BTN_TOUCH;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
    
    memset(&ev, 0, sizeof(ev));
    ev.type = EV_SYN;
    ev.code = SYN_REPORT;
    ev.value = 0;
    write(fd, &ev, sizeof(ev));
    
    printf("触摸结束\n");
    sleep(5);
    
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
