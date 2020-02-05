;Source tuto pong 0.1
;Gameboy DMG WLA-DX
;CC furrtek.org 2011

.ROMDMG                         ;Pas de features CGB
.NAME "PONGDEMO"                ;Nom du ROM inscrit dans le header
.CARTRIDGETYPE 0                ;ROM only
.RAMSIZE 0
.COMPUTEGBCHECKSUM              ;WLA-DX écrira le checksum lui-même (nécessaire sur une vraie GB)
.COMPUTEGBCOMPLEMENTCHECK       ;WLA-DX écrira le code de verif du header (nécessaire sur une vraie GB)
.LICENSEECODENEW "00"           ;Code de license Nintendo, j'en ai pas donc...
.EMPTYFILL $00                  ;Padding avec des 0

.MEMORYMAP
SLOTSIZE $4000
DEFAULTSLOT 0
SLOT 0 $0000
SLOT 1 $4000
.ENDME

.ROMBANKSIZE $4000              ;Deux banks de 16Ko
.ROMBANKS 2

.BANK 0 SLOT 0

.ENUM $C000
RaquetteY  DB
BalleX     DB
BalleY     DB
VitX       DB
VitY       DB
Raquette2Y DB
.ENDE

.ORG $0040
call      VBlank                ;L'interruption VBlank tombe ici
reti

.ORG $0100
nop
jp    start                     ;Entry point

.ORG $0104
;Logo Nintendo, obligatoire
.db $CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C
.db $00,$0D,$00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6
.db $DD,$DD,$D9,$99,$BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC
.db $99,$9F,$BB,$B9,$33,$3E

.org $0150
start:
  di                            ;Interruption désactivées
  ld     sp,$FFF4               ;Début du stack à $FFF4 parce que Nintendo le veut

  xor a
  ldh    ($26),a                ;Coupe le circuit son

waitvbl:
  ldh    a,($44)                ;Attend le début d'un VBL (première ligne hors de l'écran, Y>144)
  cp     144
  jr     c, waitvbl

  ld     a,%00010001            ;Éteins l'écran
  ldh    ($40),a

  ;Charge 3 tiles
  ld     b,8*3*2                ;3*8 lignes 2BPP
  ld     de,tiles
  ld     hl,$8000
ldt:
  ld     a,(de)
  ldi    (hl),a
  inc    de
  dec    b
  jr     nz,ldt

  ld     de,32*32               ;Vide la map BKG
  ld     hl,$9800
clmap:
  xor    a
  ldi    (hl),a
  dec    de
  ld     a,e
  or     d
  jr     nz,clmap    

  ld     hl,$FE00      ; PC=0x17c        ;Vide l'OAM
  ld     b,40*4
clspr:
  ld     (hl),$00
  inc    l                      ;Contournement bug hardware
  dec    b
  jr     nz,clspr

  xor    a       ;PC=0x187
  ldh    ($42),a                ;Scroll Y
  ldh    ($43),a                ;Scroll X

  ld     hl,$FE00
  ld     b,4
  xor    a
ldspr:
  ld     (hl),a                 ;OAM raquette Y
  add    8                      ;Prochain Y ++8
  inc    l                      ;Pas de LDI ou de INC HL ici, bug hardware
  ld     (hl),$10               ;OAM raquette X
  inc    l
  ld     (hl),$01               ;OAM raquette tile
  inc    l
  ld     (hl),$00               ;OAM raquette attribut
  inc    l
  dec    b
  jr     nz,ldspr
  
  ld     (hl),$80               ;OAM balle Y
  inc    l
  ld     (hl),$80               ;OAM balle X
  inc    l
  ld     (hl),$02               ;OAM balle tile
  inc    l
  ld     (hl),$40                ;OAM balle attribut

  inc l
  ld     b,4
  xor    a
ldspr2:
  ld     (hl),a                 ;OAM raquette Y
  add    8                      ;Prochain Y ++8
  inc    l                      ;Pas de LDI ou de INC HL ici, bug hardware
  ld     (hl),160-16               ;OAM raquette X
  inc    l
  ld     (hl),$01               ;OAM raquette tile
  inc    l
  ld     (hl),$20               ;OAM raquette attribut
  inc    l
  dec    b
  jr     nz,ldspr2

  ld     a,$20
  ld     (RaquetteY),a           ;Inits variables
  ld     (Raquette2Y),a          
  ld     a,$80
  ld     (BalleX),a
  ld     (BalleY),a
  ld     a,1
  ld     (VitX),a
  ld     (VitY),a

  ld     a,%11100100            ;Palette BG
  ldh    ($47),a
  ldh    ($48),a                ;Palette sprite 0
  ldh    ($49),a                ;Palette sprite 1 (sert pas)
  ld     a,%10010011            ;Allume l écran, BG on, tiles à $8000
  ldh    ($40),a

  ld     a,%00010000            ;Interruptions VBlank activées
  ldh    ($41),a
  ld     a,%00000001            ;Interruptions VBlank activée (double activation à la con)
  ldh    ($FF),a

  ei

loop:
  jr     loop



VBlank:
  push   af
  push   hl

  ld     a,%00100000            ;Selection touches de direction
  ldh    ($00),a
  
  ldh    a,($00)
  ld     b,a

  bit    $3,b                   ;Bas
  jr     nz,nod
  ld     a,(RaquetteY)
  inc    a
  inc    a
  ld     (RaquetteY),a
  cp     144+16-32              ;Bordure écran bas
  jr     c,nod
  ld     a,144+16-32
  ld     (RaquetteY),a
nod:

  bit    $2,b                   ;Haut
  jr     nz,nou
  ld     a,(RaquetteY)
  dec    a
  dec    a
  ld     (RaquetteY),a
  cp     16                     ;Bordure écran haut
  jr     nc,nou
  ld     a,16
  ld     (RaquetteY),a
nou:

  ld     hl,$FE00
  ld     a,(RaquetteY)
  ld     (hl),a                 ;OAM raquette 0 Y
  ld     hl,$FE04
  add    $8
  ld     (hl),a                 ;OAM raquette 1 Y
  ld     hl,$FE08
  add    $8
  ld     (hl),a                 ;OAM raquette 2 Y
  ld     hl,$FE0C
  add    $8
  ld     (hl),a                 ;OAM raquette 3 Y

;RAQUETTE 2
  ld     a,%00010000            ;Selection touches 
  ldh    ($00),a
  
  ldh    a,($00)
  ld     b,a

  bit    $0,b                   ;Bas(A)
  jr     nz,nod2
  ld     a,(Raquette2Y)
  inc    a
  inc    a
  ld     (Raquette2Y),a
  cp     144+16-32              ;Bordure écran bas
  jr     c,nod2
  ld     a,144+16-32
  ld     (Raquette2Y),a
nod2:

  bit    $1,b                   ;Haut
  jr     nz,nou2
  ld     a,(Raquette2Y)
  dec    a
  dec    a
  ld     (Raquette2Y),a
  cp     16                     ;Bordure écran haut
  jr     nc,nou2
  ld     a,16
  ld     (Raquette2Y),a
nou2:

  ld     hl,$FE14
  ld     a,(Raquette2Y)
  ld     (hl),a                 ;OAM raquette 0 Y
  ld     hl,$FE18
  add    $8
  ld     (hl),a                 ;OAM raquette 1 Y
  ld     hl,$FE1C
  add    $8
  ld     (hl),a                 ;OAM raquette 2 Y
  ld     hl,$FE20
  add    $8
  ld     (hl),a                 ;OAM raquette 3 Y
  


  ld     hl,BalleX
  ld     a,(VitX)
  add    (hl)

  cp     160                    ;BalleX < 160: pas de collision mur droit
  jr     c,nocxr
  call   lowbeep
  ld     a,$FF                  ;-2 ( $FE => 1111 1111 => 0000 0001 )
  ld     (VitX),a
  ld     a,160                  ;Limite à 160
nocxr:

  cp     2
  jr     nc,nocxl               ;BalleX > 2: pas de collision mur gauche
  call   lowbeep
  ld     a,1
  ld     (VitX),a
  ld     a,8
nocxl:
  ld     (hl),a
  


  ld     hl,BalleY
  ld     a,(VitY)
  add    (hl)

  cp     144+8
  jr     c,nocyr                ;Collision bas
  call   lowbeep
  ld     a,$FF
  ld     (VitY),a
  ld     a,144+8                ;Limite
nocyr:
  cp     8+8
  jr     nc,nocyl               ;Collision haut
  call   lowbeep
  ld     a,1
  ld     (VitY),a
  ld     a,8+8                  ;Limite
nocyl:
  ld     (hl),a
  


  ld     a,(BalleX)
  cp     8+16
  jr     nc,nopaddle            ;BalleX > 8+16: pas de collision raquette
  cp     8+10
  jr     c,nopaddle             ;BalleX < 8+10: pas de collision raquette
  ld     a,(VitX)
  cp     1
  jr     z,nopaddle             ;Vitesse positive: pas de collision raquette

  ld     a,(BalleY)
  add    8
  ld     b,a
  ld     a,(RaquetteY)
  cp     b
  jr     nc,nopaddle            ;PaddleY > BalleY+8: pas de collision raquette haute

  ld     hl,BalleY
  ld     a,(RaquetteY)
  add    32
  cp     (hl)
  jr     c,nopaddle             ;PaddleY+32 < BalleY: pas de collision raquette basse
  
  call   hibeep                 ;Rebond raquette
  ld     a,1
  ld     (VitX),a

nopaddle:
;RAQUETTE2 COLLISIONS
  ld     a,(BalleX)
  cp     160-16-8
  jr     c,nopaddle2            ;BalleX <: pas de collision raquette
  cp     160-10-8
  jr     nc,nopaddle2             ;BalleX > pas de collision raquette
  ld     a,(VitX)
  cp     $FF
  jr     z,nopaddle2             ;Vitesse négative: pas de collision raquette

  ld     a,(BalleY)
  add    8
  ld     b,a
  ld     a,(Raquette2Y)
  cp     b
  jr     nc,nopaddle2            ;PaddleY > BalleY+8: pas de collision raquette haute

  ld     hl,BalleY
  ld     a,(Raquette2Y)
  add    32
  cp     (hl)
  jr     c,nopaddle2             ;PaddleY+32 < BalleY: pas de collision raquette basse
  
  call   hibeep                 ;Rebond raquette
  ld     a,$FF
  ld     (VitX),a

nopaddle2:


  ld     hl,$FE10
  ld     a,(BalleY)
  ld     (hl),a                 ;OAM balle Y
  inc    l
  ld     a,(BalleX)
  ld     (hl),a                 ;OAM balle X

  pop    hl
  pop    af
  ret


lowbeep:
  call   setsnd
  ld     a,%00000000
  ldh    ($13),a
  ld     a,%11000111
  ldh    ($14),a
  ret

  
hibeep:
  call   setsnd
  ld     a,%11000000
  ldh    ($13),a
  ld     a,%11000111
  ldh    ($14),a
  ret


setsnd:
  ld     a,%10000000
  ldh    ($26),a

  ld     a,%01110111
  ldh    ($24),a
  ld     a,%00010001
  ldh    ($25),a

  ld     a,%10111000
  ldh    ($11),a
  ld     a,%11110000
  ldh    ($12),a
  ret

  .ORG   $0800
tiles:
  .INCBIN "tiles.bin"
