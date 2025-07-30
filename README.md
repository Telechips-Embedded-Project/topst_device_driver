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

<img width="833" height="691" alt="image" src="https://github.com/user-attachments/assets/4178cfea-bc6f-4fa1-b68e-00be327ff388" />
