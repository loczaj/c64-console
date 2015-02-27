 ; ---------------------------------------------
 ;
 ;   adapter.asm v1.20
 ;   
 ;   C64 assembly for connecting C64 & PC
 ;
 ;   For use with the linux kernel module
 ;   "c64_console"
 ;
 ; ---------------------------------------------
 ;   Lohner Roland, 2003.
 ; -----------------------
 ; 


 ; ------------------------------
 ;  Define addresses
 ; ------------------------------
 ;
WRITE_IN	equ $EB35	; Writing a byte in the keyboard buffer

STOP_FLAG	equ $91		; Stop switch

OUTPUT_VEC	equ $0326	; OUTPUT vector
OUTPUT_HAN	equ $F1CA	; OUTPUT routine

NMI_VEC		equ $0318	; NMI vector
NMI_HAN		equ $FE56	; NMI routine
NMI_RET		equ $FEBC	; Return from NMI

IRQ_RET         equ $EA7E	; Return from IRQ

CIA2            equ $DD00       ; CIA 2 base address
PRA		equ CIA2	; Port register A of CIA 2
PRB		equ CIA2 + 1	; Port register B of CIA 2
DDRA		equ CIA2 + 2	; Data direction register A of CIA 2
DDRB		equ CIA2 + 3	; Data direction register B of CIA 2
ICR		equ CIA2 + 13	; Interrupt control register of CIA 2
 
 
 ; ------------------------------
 ;  Processor type and origin
 ; ------------------------------
 ;
 	PROCESSOR 6502
	ORG $C000
 
 
 ; ------------------------------
 ;  Start routine - initialise
 ; ------------------------------
 ;
	lda #<Interr
	sta NMI_VEC		; Set NMI Vector low byte
	lda #<Output
	sta OUTPUT_VEC		; Set OUTPUT Vector low byte
	lda #>Interr
	sta NMI_VEC + 1		; Set NMI Vector high byte
	sta OUTPUT_VEC + 1	; Set OUTPUT Vector high byte
	lda DDRA
	ora #%00000100		; Set DDRA of CIA2:
	sta DDRA		; Switch the 2nd bit of PORT A (PA2) to output mode
	lda #%01111111		; Set Interrupt Control Register (ICR) of CIA2:
	sta ICR 		; Disable all interrupts
	lda #%10010000		; Set ICR again
	sta ICR			; Enable interrupt by falling edge on pin -FLAG
PrbIn:	lda #%00000000
	sta DDRB		; Switch all bits of port B to input mode
	rts			; Exit
 
 
 ; ------------------------------
 ;  Interrupt routine
 ; ------------------------------
 ;
Interr:	pha			; Save accumulator
	txa
	pha			; Save X register
	tya
	pha			; Save Y register
	lda ICR			; Read ICR2: What caused the interrupt ?
	and #%00010000		; Trigger on FLAG ?
	bne ReadP		; Yes.
	jmp NMI_HAN		; No, jump to default NMI handler
ReadP:	jsr PrbIn		; Switch all bits of port B to input mode
	lda PRB			; Read port register B (PRB)
	beq Stop		; Is the byte received null ? (stop key)
	cmp #%11111111		; Is the byte received 0xFF ? (bus free)
	beq GotBus
	jsr WRITE_IN		; No, write character into the keyboard buffer
	jmp NMI_RET		; Return from NMI
Stop:	lda #$7F		; Stop received
	sta STOP_FLAG		; Set stop flag
	jmp IRQ_RET		; Return from IRQ
GotBus:	tay			; Set Y to 0xFF: Output routine can send
	pla			; Read stack (saved Y)
	jmp NMI_RET + 2		; Return from NMI (+2: do not read Y from stack)
 
 
 ; ------------------------------
 ;  OUTPUT routine
 ; ------------------------------
 ;
Output:	sei			; Disable all interrupts from CIA1
	sta PRB			; Store the byte to send in PRB
	pha			; Save A
	tya
	pha			; Save Y
	ldy #$00		; Set Y to 0x00
	lda PRA 
	and #%11111011		; Clear PA2 (->ACK)
	sta PRA
	ora #%00000100		; Set PA2 (->ACK): Interrupt request
	sta PRA
GetBus: tya			; Y -> A and set SR
	beq GetBus		; Bus busy: wait for data bus
	sta DDRB		; Bus free: switch port B to output mode
				; Transaction ready
	pla
	tay			; Load Y
	jsr PrbIn		; Switch port B to input
	pla			; Load A
	jmp OUTPUT_HAN		; Jump to default OUTPUT routine

