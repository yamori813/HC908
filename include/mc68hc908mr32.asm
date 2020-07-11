
		.PROCESSOR	68908

;
; variant specific defines
;

		#ifndef RAM_START
RAM_START       .EQU            $60
		#endif

		#ifndef RAM_SIZE
RAM_SIZE       .EQU             768
		#endif

		#ifndef FLASH_START
FLASH_START		.EQU            $8000
		#endif

		#ifndef FLASH_SIZE
FLASH_SIZE		.EQU            32256
		#endif

	#include	"reg68hc908mr32.asm"

	#include	"macros.asm"

	.macro	switch_to_pll_clock
		clr 	PCTL
		bset	5,PCTL	 		; switch PLL on
		bset	7,PBWC			; automatic bandwidth
PLL_lock_wait:
		brclr	6,PBWC,PLL_lock_wait
		bset	4,PCTL			; select PLL clock as system
	.endm


; ROM specific routines
WriteSerial		.EQU		0xFEC3

;	.macro	d_ms
;  			ldA	{1}		  		; [2] ||
;_L2_{2}:	clrX		  		; [1] ||
;_L1_{2}:
;			dbnzX	_L1_{2}		; [3] |    256*3 = 768T
;  			dbnzA	_L2_{2}		; [3] || (768+3)*(arg-1) + 2 T
;  	.endm

;	.macro d_us
;  			ldA	{1}				; [2]
;_L1_{2}:
;  			dbnzA	_L1_{2}		; [3] 3*(arg-1) + 2 T
;  	.endm

	.macro	fw_delay
		ldX		#{1}					;[2]
		ldA		#{2}					;[2]
		jsr		MoniRomDelayLoop		;[5]		// TODO: find this in the monitor code
	.endm

	.macro	declare_delay
Delay{1}:
		fw_delay {2},{3}
		rts								;[4]
	.endm

;	; macro-names MUST be all lowercase
;	.macro	fw_flash_erase_proc
;		store_reg	fwCtrlByte,#$40
;		store_reg	fwCpuSpeed,#(4*F_CPU)
;		ldHX		#FLASH_START
;		jsr			FlashErase
;	.endm

;		.macro	delay100us
;			fw_delay	1,64		; ((((64-3)*3 +10)*1)+7) := 100 �s
;		.endm

;	.IF (F_CPU > 1900000)
;		.macro	delay100us
;			fw_delay	1,64		; ((((64-3)*3 +10)*1)+7) := 100 �s
;		.endm
;	#else
;	#endif
;

; ((((4-3)*3 +10)*1)+7) := 20 ticks @ 2MHz := 10 �s
;	.macro	delay10us
;		ldx		#1						;[2]
;		lda		#4						;[2]
;		jsr		MoniRomDelayLoop		;[5]
;	.endm

; ((((222-3)*3 +10)*6)+7) := 4.002 ms
;	.macro	delay4ms
;		ldx		#7
;		lda		#242
;		jsr		MoniRomDelayLoop
;	.endm
;
;	#else
;
;	.macro	delay10us
;		bsr		Delay10us
;	.endm

;	.macro	delay100us
;		bsr		Delay100us
;	.endm

;	.macro	delay4ms
;		bsr		Delay4ms
;	.endm
;
;
;	#endif


