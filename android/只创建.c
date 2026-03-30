/*
 * 完整的测试流程

1. 在 B手机（接收端）启动虚拟触摸屏

```bash
# 运行你的虚拟触摸屏程序
./standard_zoom

# 在另一个终端查看虚拟设备
getevent -lt /dev/input/event10
```

2. 在 A手机（发送端）转发真实触摸屏

```bash
# 转发真实触摸屏事件
cat /dev/input/event3 | nc -l -p 6666

# 或者转发你之前创建的虚拟设备
# cat /dev/input/event10 | nc -l -p 6666
```

3. 在 B手机（接收端）接收事件

```bash
# 接收并注入到虚拟设备
nc 192.168.1.100 6666 > /dev/input/event10
```
*/



// create_device_only.c - 只创建设备，不执行手势
#include <linux/input.h>
#include <linux/uinput.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#define SCREEN_WIDTH 1080
#define SCREEN_HEIGHT 2376

int main() {
    int fd;
    struct uinput_user_dev uidev;
    
    // 打开设备
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (fd < 0) {
        perror("打开 /dev/uinput 失败");
        return 1;
    }
    
    printf("配置虚拟触摸屏...\n");
    
    // 配置设备能力
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
    
    printf("✓ 虚拟触摸屏创建成功\n");
    printf("  分辨率: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    printf("  按 Ctrl+C 退出\n\n");
    
    // 保持设备运行
    while (1) {
        sleep(1);
    }
    
    // 清理
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    return 0;
}
