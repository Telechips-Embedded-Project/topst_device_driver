**driver 폴더: .ko 파일들**<br />
보드의 /lib/modules/5.4.159-tcc/extra/ 아래에 복사해주기 <br /> <br />

부팅 후 자동으로 올라오게 만들기 위해 <br />
```
cat >/etc/modules-load.d/topst.conf <<'EOF'
> ambient_driver
> wiper_driver
> window_driver
> aircon_driver
> headlamp_driver
> EOF
```
모듈, 장치 자동으로 올라옴. <br />
<img width="937" height="302" alt="image" src="https://github.com/user-attachments/assets/ee90516b-29ef-428f-abfc-ad87706f823a" />
<img width="647" height="157" alt="image" src="https://github.com/user-attachments/assets/42b1b94a-5010-40bd-aff0-4724a827b663" />

<br />
<br />

**user 폴더: 데몬 실행 파일들** 
<br />
```
./aircon_daemon &
./ambient_daemon &
./wiper_daemon &
```

+) 수동 제어 테스트 가능
```
./aircon_setter [low/mid/high]
./ambient_setter color [red/yellow/green/rainbow]
./ambient_setter brightness [0~100]
./wiper_setter [slow/fast]
```
