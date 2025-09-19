# YOS
I've tried to transport [YOS on ATmega328P](https://github.com/Ashibananon/yos) implemented previously for ATmega328P to STM32.

前に作っている[ATmega328Pで動いているYOS](https://github.com/Ashibananon/yos)をSTM32に移植してみます。

## Hardware
STM32F103C8T6 MCU

### PIN Connection（PIN接続）
| STM32F103C8T6 PIN | Function（機能） | Connect to Device（接続先） | PIN of connected device（接続先PIN） |
| --- | --- | --- | --- |
| PA10/RX | UART Receive  | Serial/USB-Serial module | TX |
| PA9/TX | UART Transmit | Serial/USB-Serial module | RX |

### STM32F103C8T6
- Max 72 MHz CPU
- 20 KB SRAM
- 64 KB Flash

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
- USART(For cmdline only)

  コマンドライン用USARTのみ

## Others（その他）
- A cmdline interface

  コマンドライン

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

		basic_io_printf("-------------\n");
		basic_io_printf("YOS starts on STM32\n");

		yos_init();

		if (yos_create_task(_cmdline_task, NULL, 1024, "cmdtask") < 0) {
			return -1;
		}

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

- exit

	Exit command line

	コマンドを終了します

Example（例）:

		-------------
		YOS starts on STM32

		STM32 cmdline started.
		Input [help] to show all available commands,
		or [exit] to exit STM32 cmdline.
		STM32> help
		            help        Show cmd info briefly
		            echo        Echo cmdline info
		              si        Show system info
		           sleep        Sleep given ms
		              ts        Show tasks info
		            exit        Exit cmdline
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
		STM32> si
		MCU: STM32F103C8T6 Max Freq: 72000000 Hz
		Flash: 65536 Bytes
		RAM: 20480 Bytes
		sz char=1
		sz short=2
		sz int=4
		sz long=4
		sz float=4
		sz double=8
		sz void *=4
		STM32> sleep 5000
		sleep 5000 ms
		5000 ms slept
		STM32> ts
		ID    ST      SS     MSS    NAME
		000    1     128      72    yosidle
		001    1    1024     488    cmdtask
		STM32>

# Copyright & License
MIT License

Copyright (c) 2025 Ashibananon(Yuan)
