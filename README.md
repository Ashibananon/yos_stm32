# YOS
I've tried to transport [YOS on ATmega328P](https://github.com/Ashibananon/yos) implemented previously for ATmega328P to STM32.

前に作っている[ATmega328Pで動いているYOS](https://github.com/Ashibananon/yos)をSTM32に移植してみます。

## Hardware
STM32F401RCT6 MCU

### PIN Connection（PIN接続）
| STM32F401RCT6 PIN | Function（機能） | Connect to Device（接続先） | PIN of connected device（接続先PIN） |
| --- | --- | --- | --- |
| PA10/RX | UART Receive  | Serial/USB-Serial module | TX |
| PA9/TX | UART Transmit | Serial/USB-Serial module | RX |
| PB6 | IIC SCL | SSD1306 OLED | SCL |
| | | AHT20 | SCL |
| PB7 | IIC SDA | SSD1306 OLED | SDA |
| | | AHT20 | SDA |
| PA4 | SPI CS | SD Card Module | CS |
| PA5 | SPI SCK | SD Card Module | CLK |
| PA6 | SPI MISO | SD Card Module | MOSI |
| PA7 | SPI MOSI | SD Card Module | MISO |
| PB12 | I2S(Output) WS | I2S DAC Module | WS |
| PB13 | I2S(Output) CLK | I2S DAC Module | BCK |
| PB15 | I2S(Output) SD | I2S DAC Module | DIN |
| PA15 | I2S(Input) WS | I2S MIC(L) | WS |
| | | I2S MIC(R) | WS |
| PB3 | I2S(Input) CLK | I2S MIC(L) | CLK |
| | | I2S MIC(R) | CLK |
| PB5 | I2S(Input) SD | I2S MIC(L) | SD |
| | | I2S MIC(R) | SD |



### STM32F401RCT6
- ARM Cortex M4
- 84 MHz CPU
- 64 KB SRAM
- 256 KB Flash

## Development Environment （開発環境）
- VS Code with PlatformIO extension
- libopencm3

# Features
## Core
- Up to 8 tasks

  最大8個タスク

- Scheduling by time slice

  タイムスライスでスゲージュリング

- delay/msleep/schedule functions to give up current running chance

  delay/msleep/schedule関数で自発的にスゲジュウルします

- Mutex

  Mutex

- Up to 8 User Timer

  最大8個ユーザー用タイマー

## Hardware Interface Driver（ハードウェア　インタフェース　ドライバー）
- USART

  For cmdline use

  コマンドライン用

- IIC
- SPI
- I2S

## Others（その他）
- A cmdline interface

  コマンドライン

- SSD1306 oled (3rd library used)

  SSD1306 OLED（第三者ライブラリ利用）

- AHT20 Temperature & Humidity Sensor (3rd library used)

  AHT20温湿度センサー（第三者ライブラリ利用）

- SD Card Module (3rd library used)

  SDカードモジュール（第三者ライブラリ利用）

  ** Please notice that the SDSC card seems not supported by the 3rd library, and only one type of SDHC card is confirmed: KIOXIA 16GB

  ※利用している第三者ライブラリの説明によって、SDSCカードは利用できません。今の時点ではたった一つのSDHCカードに動作確認しました：KIOXIA 16GB

- YFS

  A abstract file system, and the interfaces are implemented with FAT and LittleFS

  抽象化層ファイルシステム、インタフェースはFAT、LittleFSに実現しています

- WAV Player

  Play wav file from SD card

  SDカードのwavファイルを再生します

- Audio Recorder

  Record audio to SD card as wav file

  録音してwavファイルとしてSDカードに保存します

# How to use（使い方）
It is quite easy to use YOS, maybe it will be faster to go and have a look at main()

YOSは使いやすいと思います、main()関数を見ていただければすぐ分かるようになりますでしょう

The main() does some initializations like USART, IIC, ADC firstly.

main()関数は最初にいくつかの初期化処理を行います、例えばUSART、IIC、ADCなど。

And then call yos_init() to initialize the YOS.

そしてyos_init()を呼び出して、YOSの初期化します。

After that, some tasks are created with yos_create_task function.

次に、yos_create_task関数を呼び出して、いくつかのタスクを作成します。

At last, yos_start() is called and tasks created are scheduled to run.

最後に、yos_start()を呼び出してから、YOSを動かせて、さっき作成されたタスクはそれぞれスゲージュルされて動くようになります


	int main()
	{
		system_clock_setup();
		user_timer_init();

		if (basic_io_init(yusart_io_operations) != 0) {
			return -1;
		}

		if (yiic_master_init(100000) != 0) {
			basic_io_printf("IIC master init failed\n");
			return -1;
		}

		basic_io_printf("-------------\n");
		basic_io_printf("YOS starts on STM32\n");

		yos_init();

		if (yos_create_task(_cmdline_task, NULL, 1024, "cmdtask") < 0) {
			return -1;
		}

	#if (HAS_SSD1306_OLED == 1)
		if (yos_create_task(_oled_task, NULL, 1024, "oledtak") < 0) {
			basic_io_printf("Failed to create oled task\n");
			return -1;
		}
	#endif

	#if (HAS_AHT20_SENSOR == 1)
		if (yos_create_task(_aht20_task, NULL, 1024, "ahttsk") < 0 ) {
			basic_io_printf("Failed to create aht20 task\n");
			return -1;
		}
	#endif

		yos_start();

		/*
		 * Not going here
		 *
		 * ここに着くことはありません
		 */
		return 0;
	}

And you can find each task functions which are as below:

タスク関数は全部下記のような形となります：

	int _XXX_task(void *para)
	{
		/* Do prepare work first */
		_XXX_prepare_work_1();
		......
		_XXX_prepare_work_N();

		while (1) {
			/* Do work repeatedly */
			_XXX_do_work_1();
			......
			_XXX_do_work_M();

			/* Sleep some time for the next loop */
			yos_task_msleep(1000);
		}

		return 0;
	}

## About cmdline（cmdlineについて）
USART settings are like below:

- Baudrate: 115200 8N1
- Flow control: None
- Newline character（改行コード）: LF

Right now the following commands are available:

いまは下記のコマンドが利用できます：
- help

	Show all the available commands

	全部利用できるコマンドを表示します

- echo

	Echo the input as argument arrays(i.e. argc and argv)

	入力された内容をアーギュメント配列として表示します（あるいはargc、argvの形で）

- sd

	SD card commands(init, deinit, info, etc)

	SDカード操作用コマンドです(初期化、解除、情報表示など)

- si

	Display some system info

	いくつかのシステム情報を表示します

- sleep

	Sleep for given time(ms)

	指定する時間（ミリ秒）で待ち合わせます

- ts

	Show task status

	タスク情報を表示します

	ID: Task ID（タスクID）

	ST: Task status（タスク状態）

	SS: Stack size（スタックサイズ）

	MSS: Max size used in stack by now（今までスタックを利用している最大サイズ）

	NAME: Task name（タスク名）

- yfs

	YFS commands(mount, umount, status, etc)

	YFS操作コマンドです(マウント、解除、情報表示など)

- yls

	list files or directories

	ファイルまたフォルダをリストします

- ycat

	display file contents

	ファイル内容を表示します

- yapp

	append data to the end of file

	ファイルにデータを追加します

- ymkdir

	make directory

	フォルダを作成します

- ymv

	move file/directory

	ファイルまたフォルダを移動します

- yrm

	remove file/directory

	ファイルまたフォルダを削除します

- ycp

	copy file

	ファイルをコピーします

- ydd

	copy file data

	ファイルデータをコピーします

- ytouch

	create an empty file

	空ファイルを新規します

- yaudio

	audio play or record

	音声ファイル再生、または録音します

- exit

	Exit command line

	コマンドを終了します

Example（例）:

		-------------
		YOS starts on STM32

		STM32 cmdline started.
		Input [help] to show all available commands,
		or [exit] to exit STM32 cmdline.

- Command [help]

  コマンド[help]

		STM32> help
		            help        Show cmd info briefly
		            echo        Echo cmdline info
		              sd        SD card cmds
		              si        Show system info
		           sleep        Sleep given ms
		              ts        Show tasks info
		             yfs        mount yfs
		             yls        list files of file system
		            ycat        display file contents
		            yapp        append data to the end of file
		          ymkdir        make directory
		             ymv        move file/directory
		             yrm        remove file/directory
		             ycp        copy file
		             ydd        data copy
		          ytouch        create an empty file
				  yaudio        play audio file
		            exit        Exit cmdline

- Command [echo]

  コマンド[echo]

		STM32> echo hello yos on stm32!
		Echo got 5 paramaters:
		[ 0]    [echo]
		[ 1]    [hello]
		[ 2]    [yos]
		[ 3]    [on]
		[ 4]    [stm32!]
		Last cmd ret: [0]

		STM32> echo "hello yos on stm32!"
		Echo got 2 paramaters:
		[ 0]    [echo]
		[ 1]    [hello yos on stm32!]
		Last cmd ret: [0]

- Command [sd]

  コマンド[sd]

		STM32> sd init
		[lib\sd-spi-driver\src\sd_utils.c:43] sd_card_into_idle: CMD0 idle success
		This is a SDHC card
		  > Name: "sdcard0"
		  > Capacity: 14784 MB
		  > Block size: 512 B
		  > Erase sector size: 65536 KB
		init sd OK

		STM32> sd info
		got sd info:
		  Type: 3
		  Sector Size: 512
		  Sector Number: 30277632
		  Erase block size: 67108864

- Command [si]

  コマンド[si]

		STM32> si
		MCU: STM32F401RCT6 Max Freq: 84000000 Hz
		Flash: 262144 Bytes
		RAM: 65536 Bytes
		sz char=1
		sz short=2
		sz int=4
		sz long=4
		sz float=4
		sz double=8
		sz void *=4

- Command [sleep]

  コマンド[sleep]

		STM32> sleep 5000
		sleep 5000 ms
		5000 ms slept

- Command [ts]

  コマンド[ts]

		STM32> ts
		ID    ST      SS     MSS    NAME
		000    1     128      72    yosidle
		001    1    1024     488    cmdtask

- Command [yfs]

  [sd init] command must be executed before [yfs start]

  コマンド[yfs]

  [sd init]コマンドを実行してから[yfs start]を行います

		STM32> yfs start
		[lib\sd-spi-driver\src\sd_utils.c:43] sd_card_into_idle: CMD0 idle success
		This is a SDHC card
		  > Name: "sdcard0"
		  > Capacity: 14784 MB
		  > Block size: 512 B
		  > Erase sector size: 65536 KB
		YFS started OK

		STM32> yfs status
		YFS status:
		  type: 0
		  subtype: 3
		  block size: 65536
		  block count: 1890050
		  block allocated: 32
		  max filename length: 255
		  max file size: 0
		  mount point: /sd

  **Please note that the default mount point is [/sd]**

  **SDカードのデフォルトマウントポイントは「/sd」になることをご注意ください**

- YFS commands examples

  YFS操作コマンド例

		STM32> yls /sd
		Files on /sd:
		T         Size Name
		-            0 System Volume Information
		d              TestScripts
		-        18549 f1
		-            0 newfile

		STM32> yapp at /sd/hello.txt "Hello YFS World!"
		yapp: write 16 bytes to file[/sd/hello.txt]

		STM32> yls /sd
		Files on /sd:
		T         Size Name
		-            0 System Volume Information
		d              TestScripts
		-        18549 f1
		-            0 newfile
		-           16 hello.txt

		STM32> ycat t /sd/hello.txt
		Hello YFS World!

		STM32>

- Command [yaudio]

  コマンド[yaudio]

  To play a wav file from SD or record audios to SD, SD card must be initialized and YFS must starts.

  wavファイルの再生、または録音してSDに保存するには、SDカードの初期化とYFS起動することが必要です。

  (1) play wav file (wavファイル再生)

		/* 1. Check wav files on sd　(SDカードのwavファイル情報を確認します) */
		yos_stm32> yls /sd
		Files on /sd:
		T         Size Name
		-     52920114 music2.wav
		-     25319394 music1.wav
		d              audios

		/* 2. Set wav file to play (再生するwavファイルを指定します) */
		yos_stm32> yaudio set /sd/music1.wav
		Send cmd to set audio file [/sd/music1.wav] ok
		[YAUDIO]Receive cmd: set audio file to [/sd/musiyos_stm32> c1.wav]
		/* The output message of the last line seems disorder, no need to worry.
		 * 上の行の出力情報は混乱しているが、気にしなくては大丈夫です。
		 */

		/* 3. Play (再生) */
		yos_stm32> yaudio play
		Send play cmd ok
		yos_stm32> [YAUDIO]Receive cmd: play
		[YAUDIO]File[/sd/music1.wav] open ok
		[YAUDIO]Open wav file [/sd/music1.wav] OK
		[YAUDIO]Audio info:
		[YAUDIO]  channels: [2]
		[YAUDIO]  sample rate: [44100]
		[YAUDIO]  bit depth: [16]
		[YAUDIO]  sample num: [0][6329820]
		/* You will see the wav file info output like above, if everything goes well.
		 * 問題なければ、上記のようなwavファイルの情報が出力されます。
		 */

		/* When play ends, the following message will output.
		 * 再生が終わったら、下記の情報が出力されます。
		 */
		[YAUDIO]EOF reached
		[YAUDIO]File[/sd/music1.wav] closed
		[YAUDIO]pcm data read times: 0:21100
		[YAUDIO]pcm data wait times: 0:28020
		[YAUDIO]dma data wait times: 0:0

		/* During playing, you can pause or stop it by the following command
		 *
		 * 再生中ですが、下記のコマンドを入力して一時停止、また再生終了ができます
		 *
		 * [yaudio pause] -- pause (一時停止)
		 * [yaudio play]  -- play (再生)
		 * [yaudio stop]  -- stop playing (再生終了)
		 */


  (2) record (録音)

		/* 1. Check wav files on sd　(SDカードのwavファイル情報を確認します) */
		yos_stm32> yls /sd
		Files on /sd:
		T         Size Name
		-     52920114 music2.wav
		-     25319394 music1.wav
		d              audios

		/* 2. Set a wav file name to record.
		 *    Please specify a new file name, or the record will fail for the file
		 *    already exists.
		 *
		 *    録音データを保存するwavファイルを指定します。
		 *    存在しないwavファイル名を指定してください、でないと録音はできません。
		 *    既に存在しているファイルを上書きないのです。
		 */
		yos_stm32> yaudio set /sd/test.wav
		Send cmd to set audio file [/sd/test.wav] ok
		yo[YAUDIO]Receive cmd: set audio file to [/sd/tests_stm32> .wav]
		/* The output message of the last line seems disorder, no need to worry.
		 * 上の行の出力情報は混乱しているが、気にしなくては大丈夫です。
		 */

		/* 3. Start record
		 *
		 *    録音を開始します
		 */
		yos_stm32> yaudio record
		Send record cmd ok
		yos_stm32> [YAUDIO]Receive cmd: record
		[YAUDIO]File[/sd/test.wav] open ok
		[YAUDIO]Open wav file [/sd/test.wav] OK
		[YAUDIO]Audio info:
		[YAUDIO]  channels: [2]
		[YAUDIO]  sample rate: [8000]
		[YAUDIO]  bit depth: [32]
		[YAUDIO]  sample num: [0][0]

		/* 4. Stop record
		 *
		 *    録音終了します
		 */
		yos_stm32> yaudio stop
		Send stop cmd ok
		yos_stm32> [YAUDIO]Receive cmd: stop
		[YAUDIO]File[/sd/test.wav] closed
		[YAUDIO]pcm data read times: 0:576
		[YAUDIO]pcm data wait times: 0:3380
		[YAUDIO]dma data wait times: 0:0

		/* 5. Check record wav file.
		 *    You can just play it with [yaudio play].
		 *
		 *    録音wavファイルを確認します
		 *    コマンド「yaudio play」を実行して再生できます
		 */
		yos_stm32> yls /sd
		Files on /sd:
		T         Size Name
		-       691244 test.wav
		-     52920114 music2.wav
		-     25319394 music1.wav
		d              audios


# About OLED（OLEDについて）
The following infomations are displayed on OLED:

OLEDで下記の情報が表示されます

- YOS OLED
- UP Time

  起動してから経った時間

- Temperature and humidity

  温湿度

The OLED inverses display every minute.

OLEDの表示は「分（60秒）」毎で色反転になります

# Thanks（感謝）
The following 3rd libraries are used and let me thanks the authors.

下記の第三者ライブラリを利用しています、著作者方々に感謝の意を表させてください。

- libopencm3(https://github.com/libopencm3/libopencm3)

  Framework for this project

  フレームワークとしてこのプロジェクトを開発します

- For AHT20(https://github.com/kpierzynski/AVR_aht20)

  is under the folder [lib/AVR_aht20]

  フォルダ[lib/AVR_aht20]に格納します

- For SSD1306 OLED(https://github.com/tinusaur/ssd1306xled)

  is under the folder [lib/ssd1306xled]

  フォルダ[lib/ssd1306xled]に格納します

- For FatFS(https://elm-chan.org/fsw/ff/)

  is under the folder [lib/ff16]

  フォルダ[lib/ff16]に格納します

- For SD Card(https://github.com/SouthernSandbox/sd-spi-driver)

  is under the folder [lib/sd-spi-driver]

  フォルダ[lib/sd-spi-driver]に格納します

- For wav file playing and recording(https://github.com/mackron/dr_libs)

  is under the folder [lib/yaudio_libs/dr_libs-wav-0.14.2]

  フォルダ[lib/yaudio_libs/dr_libs-wav-0.14.2]に格納します

## About modifications（ソース修正について）
It seems that it is difficult to exclude source files of a 3rd-library from building with PlatformIO, and some source files make building errors.

I changed the file name with a suffix of [.org]

PlatformIOを利用していますが、第三者ライブラリの中に指定するソースファイルだけをビルド対象にするのは難しそうです。いくつかのソースファイルのビルドにエラーが出ました。

それらのソースファイル名に「.org」をつけて変更させていただきました。

And to make it better to do with YOS, some source files are modified and some new source files are added.

いくつかの新規ソースファイルも追加させていただきました、YOSの動けるようにするためです。

# Copyright & License
MIT License

Copyright (c) 2025 Ashibananon(Yuan)
