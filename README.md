**1. 헤드램프** <br />
gpio 83번 사용 <br />
sudo insmod headlamp_driver.ko 한 뒤, ioctl로 활용 <br /><br />

**2. 와이퍼** <br />
gpio 89번 (pwm 0번 채널) 사용 <br />
driver 폴더에서 sudo insmod wiper_driver.ko 한 뒤, <br />
user 폴더에서 ./wiper_daemon & 수행 <br />
테스트: user 폴더에서 ./wiper_setter [fast/slow/off] <br /><br />

**3. 에어컨** <br />
gpio 114 (pwm 1번 채널) 사용 <br />
driver 폴더에서 sudo insmod aircon_driver.ko 한 뒤, <br />
user 폴더에서 ./aircon_daemon & 수행 <br />
테스트: user 폴더에서 ./aircon_setter [low/mid/high/off] <br /><br />

**4. 창문** <br />
IN1 84번, IN2 85번, 하단 리미트 90번, 상단 리미트 65번 <br />
driver 폴더에서 sudo insmod window_driver.ko 한 뒤, <br />
테스트: user 폴더에서 ./window_setter [open/close/stop] <br /><br />

**5. 엠비언트** <br />
SPI1 MOSI를 노란색 선에 꽂기 <br />
driver 폴더에서 insmod ambient_driver.ko 한 뒤, <br />
user 폴더에서 ./ambient_daemon & 수행 <br />
테스트: user 폴더에서 ./ambient_setter color [red, yellow, green, blue, rainbow] , ./ambient_setter brightness [0~100] <br /> <br />

<img width="833" height="691" alt="image" src="https://github.com/user-attachments/assets/4178cfea-bc6f-4fa1-b68e-00be327ff388" />
