#include <stdio.h>
#include "platform.h"
#include "lw_usb/GenericMacros.h"
#include "lw_usb/GenericTypeDefs.h"
#include "lw_usb/MAX3421E.h"
#include "lw_usb/USB.h"
#include "lw_usb/usb_ch9.h"
#include "lw_usb/transfer.h"
#include "lw_usb/HID.h"

#include "xparameters.h"
#include <xgpio.h>

#include "hdmi_text_controller.h"

#define HDMI_BASE XPAR_HDMI_TEXT_CONTROLLER_0_AXI_BASEADDR

#define SHEET_WIDTH 288
#define SCREEN_W 320
#define SCREEN_H 240

#define TYPE_GRUNT 0
#define TYPE_HULK 1
#define TYPE_SPHEROID 2
#define TYPE_ENFORCER 3
#define TYPE_BRAIN 4
#define TYPE_PROG 5
#define TYPE_QUARK 6
#define TYPE_TANK 7
#define TYPE_ELECTRODE 8 
#define HUMAN_M 9
#define HUMAN_W 10
#define HUMAN_C 11
#define TYPE_SCORE 12

#define SCORE_1000 16
#define SCORE_2000 32
#define SCORE_3000 48
#define SCORE_4000 64
#define SCORE_5000 80

#define GRUNT_ID 36864
#define HULK_ID 13872
#define ENFORCER_ID 27648
#define SPHEROID_ID 14032
#define BRAIN_ID 18432
#define PROG_ID 4800
#define QUARK_ID 36944
#define TANK_ID 59964
#define ELECTRODE_ID 23232
#define SPRITE_X_ADDR (HDMI_BASE + 0x0000)
#define SPRITE_Y_ADDR (HDMI_BASE + 0x0004)
#define PLAYER_DIR_ADDR (HDMI_BASE + 0x000C)

#define GRUNT_ADDR_FRONT  GRUNT_ID
#define GRUNT_ADDR_BACK   GRUNT_ID
#define GRUNT_ADDR_LEFT   GRUNT_ID
#define GRUNT_ADDR_RIGHT  GRUNT_ID

#define HULK_ADDR_FRONT   HULK_ID
#define HULK_ADDR_BACK    HULK_ID
#define HULK_ADDR_LEFT    HULK_ID - 48
#define HULK_ADDR_RIGHT   HULK_ID + 48

#define BRAIN_ADDR_FRONT  BRAIN_ID
#define BRAIN_ADDR_BACK   BRAIN_ID + 48
#define BRAIN_ADDR_LEFT   BRAIN_ID - 96
#define BRAIN_ADDR_RIGHT  BRAIN_ID - 48

#define CHILD_ADDR_FRONT1 9312
#define CHILD_ADDR_FRONT2 9328
#define CHILD_ADDR_FRONT3 9344
#define CHILD_ADDR_RIGHT1 9264
#define CHILD_ADDR_RIGHT2 9280
#define CHILD_ADDR_RIGHT3 9296
#define CHILD_ADDR_LEFT1 9216
#define CHILD_ADDR_LEFT2 9232
#define CHILD_ADDR_LEFT3 9248
#define CHILD_ADDR_BACK1 9360
#define CHILD_ADDR_BACK2 9376
#define CHILD_ADDR_BACK3 9392

#define WOMAN_ADDR_FRONT1 192
#define WOMAN_ADDR_FRONT2 192
#define WOMAN_ADDR_FRONT3 224
#define WOMAN_ADDR_RIGHT1 144
#define WOMAN_ADDR_RIGHT2 160
#define WOMAN_ADDR_RIGHT3 176
#define WOMAN_ADDR_LEFT1 96
#define WOMAN_ADDR_LEFT2 112
#define WOMAN_ADDR_LEFT3 128
#define WOMAN_ADDR_BACK1 240
#define WOMAN_ADDR_BACK2 256
#define WOMAN_ADDR_BACK3 272

#define MAN_ADDR_FRONT1 4704
#define MAN_ADDR_FRONT2 4720
#define MAN_ADDR_FRONT3 4736
#define MAN_ADDR_RIGHT1 4656
#define MAN_ADDR_RIGHT2 4672
#define MAN_ADDR_RIGHT3 4688
#define MAN_ADDR_LEFT1 4608
#define MAN_ADDR_LEFT2 4624
#define MAN_ADDR_LEFT3 4624
#define MAN_ADDR_BACK1 4752
#define MAN_ADDR_BACK2 4768
#define MAN_ADDR_BACK3 4784

#define ENFORCER_BULLET_ADDR 27760
#define BRAIN_BULLET_ADDR 32352
#define TANK_BULLET_ADDR 64512
#define ADDR_2084 32272

#define HEALTH_ADDR      (HDMI_BASE + 0x0008) // slv_regs[2]
#define GAME_STATE_REG   (HDMI_BASE + 0x0014) // slv_regs[5]
#define SCORE_REG_LOWER  (HDMI_BASE + 0x0020) // slv_regs[8] 
#define SCORE_REG_UPPER  (HDMI_BASE + 0x0024) // slv_regs[9]
#define WAVE_REG         (HDMI_BASE + 0x0028) // slv_regs[10]

#define NUM_BULLETS 8
#define BULLET_BASE_INDEX 15
#define BULLET_X_ADDR(i)      (HDMI_BASE + ((BULLET_BASE_INDEX + (i*3)) * 4))
#define BULLET_Y_ADDR(i)      (HDMI_BASE + ((BULLET_BASE_INDEX + 1 + (i*3)) * 4))
#define BULLET_STATUS_ADDR(i) (HDMI_BASE + ((BULLET_BASE_INDEX + 2 + (i*3)) * 4))

#define NUM_ENEMIES 50
#define ENEMY_BASE_INDEX 50

#define ENEMY_SPRITE_REG(i) (HDMI_BASE + ((ENEMY_BASE_INDEX + (i*2)) * 4))
#define ENEMY_COORD_REG(i)  (HDMI_BASE + ((ENEMY_BASE_INDEX + 1 + (i*2)) * 4))

#define NUM_ENEMY_BULLETS 25

#define DISPLAY_REGS_START (HDMI_BASE + 0x0350) 
#define DISPLAY_REGS_END (HDMI_BASE + 0x07FC) 

// GLOBAL VARIABLES
int gameState = 0; // 0 is start screen, 1 is enemy spawning, 2 is player spawning, 3 is for waves, 4 is for wave transition (upon win), 5 is for end screen
int current_wave = 1;

uint32_t player_score = 0; // HDMI_BASE + 0x002C
int player_health = 3; // HDMI_BASE + 0x0008
int humansSaved = 0; // reset every wave

int sprite_x = 149;
int sprite_y = 113;
int player_dir = 4;

int player_iframes = 0;

// store unsigned 32 bit scores
uint32_t leader_score[5];
// store names by bytes, 4 chars
uint32_t leader_names[5];

int enemy_x[NUM_ENEMIES] = {0};
int enemy_y[NUM_ENEMIES] = {0};
int enemy_base_sprite[NUM_ENEMIES];

//more arrays for complex enemies like type and dir
int enemy_type[NUM_ENEMIES];
int enemy_state1[NUM_ENEMIES];
int enemy_state2[NUM_ENEMIES];

// bullets
int e_bulletX[NUM_ENEMY_BULLETS];
int e_bulletY[NUM_ENEMY_BULLETS];
int e_bvelocityX[NUM_ENEMY_BULLETS];
int e_bvelocityY[NUM_ENEMY_BULLETS];
int e_bID[NUM_ENEMY_BULLETS];
int e_bLife[NUM_ENEMY_BULLETS];

int spawn_timer = 0;

int bullet_x[NUM_BULLETS] = {0};
int bullet_y[NUM_BULLETS] = {0};
int bullet_active[NUM_BULLETS] = {0};
int bullet_dir[NUM_BULLETS] = {0};
int bullet_speed = 4;
int fire_cooldown = 0;
int new_life = 25000;
// Utility functions so vitis doesn't throw warnings
int get_player_sprite_addr(int dir, int is_moving, int anim_tick);
int get_animated_enemy_sprite(int enemy_type, int base_id, int frameCount, int dx, int dy);
unsigned short fastRand();
void get_safe_pos(int *x, int *y);
void spawnScore(int x, int y, int sprite_id);
int scoreSprite(int* eX, int* eY, int* timer, int eID);
void initLeaderboard(); // init leaderboard with dummy names and score 0, run at beginning of prgm
void newLeaderboard(); // run after death
void menuScreen(BOOT_KBD_REPORT* kbdbuf);
int waveDone();
int stageTransition();
void system_init();
void clear_all_bullets();
void initWave(int waveNum);
void update_hud(uint32_t score, int health, int wave);
int playerCollision(int pX, int pY, int eX, int eY, int eID);

// Enemy Logic functions
int grunt(int* eX, int* eY, int eID, int bX[], int bY[], int bOn[], int pX, int pY, int frameCount);
int hulk(int* eX, int* eY, int* dirX, int* dirY, int bX[], int bY[], int bOn[], int bDir[], int frameCount, int pX, int pY, int eID);
int enforcer(int* eX, int* eY, int pX, int pY, int frameCount, int bX[], int bY[], int bOn[], int eID);
int spheroid(int* eX, int* eY, int* spawnCount, int frameCount, int bX[], int bY[], int bOn[], int pX, int pY, int eID);
int brain(int* eX, int* eY, int pX, int pY, int bX[], int bY[], int bOn[], int frameCount, int eID);
int prog(int* eX, int* eY, int pX, int pY, int bX[], int bY[], int bOn[], int eID, int frameCount);
int quark(int* eX, int* eY, int frameCount, int bX[], int bY[], int bOn[], int pX, int pY, int eID);
int tank(int* eX, int* eY, int pX, int pY, int frameCount, int bX[], int bY[], int bOn[], int eID);
int human(int* eX, int* eY, int* dirX, int* dirY, int pX, int pY, int* eID, int frameCount, int* eType);
int electrode(int* eX, int* eY, int bX[], int bY[], int bOn[], int pX, int pY, int eID, int frameCount);

// Spawner funcs
void spawnEnforcer(int x, int y);
void spawnTank(int x, int y);
void spawnEnforcerBullet(int startX, int startY, int targetX, int targetY);
void spawnBrainProjectile(int startX, int startY, int targetX, int targetY);
void spawnTankBullet(int startX, int startY, int targetX, int targetY);

int bulletBounce(int* x, int* y, int* vx, int* vy, int id, int pX, int pY);
int bulletNoBounce(int* x, int* y, int* vx, int* vy, int id, int pX, int pY);
int bulletHoming(int* x, int* y, int* vx, int* vy, int targetX, int targetY, int id, int pX, int pY);



/*
wave init function, run at beginning of each wave
just inits a bunch of enemies at locations.
returns nothing
wave 1: 10 enemies, 1 human
wave 2: 20 enemies, 2 human
wave 3: 30 enemies, 3 human
wave 4: 35 enemies, 4 human
wave 5: 40 enemies, 5 human
wave 6+: wave 5
use fastRand to decide XY, and pick enemy ID
can spawn grunt, spheroid, hulk, quark, brain, electrode (6)
store humans at array idx 40-44
*/
void initWave(int waveNum) {

	clear_all_bullets();
	humansSaved = 0;

	// clear current regs
    for (int i = 0; i < NUM_ENEMIES; i++) {
        enemy_x[i] = -1;
        enemy_y[i] = -1;
        enemy_type[i] = -1;
        enemy_base_sprite[i] = -1;
        Xil_Out32(ENEMY_SPRITE_REG(i), -1);
    }

    int numEnemies = 0;
    int numHumans = 0;
    int typeRange = 0; // restrict enemy range for lower waves

    // handle wave 6+ by capping at wave 5 settings
    int effectiveWave = (waveNum > 5) ? 5 : waveNum; // above 6? equal to 5

    // define wave params based on input; wave 6+ corresponds to another wave 5
    switch (effectiveWave) {
        case 1: numEnemies = 10; numHumans = 1; typeRange = 2; break; // grunt, hulk
        case 2: numEnemies = 20; numHumans = 2; typeRange = 3; break; // + electrode
        case 3: numEnemies = 30; numHumans = 3; typeRange = 4; break; // + brain
        case 4: numEnemies = 40; numHumans = 4; typeRange = 5; break; // + quark
        case 5: numEnemies = 40; numHumans = 5; typeRange = 6; break; // + spheroid
        default: return; // default just in case
    }

    // spawn enemies in idx range 0-39
    for (int i = 0; i < numEnemies; i++) {
        // redundant break; not great logic but whatever
        if (i >= 40) break;

        get_safe_pos(&enemy_x[i], &enemy_y[i]);

        // determine type of enemy corresponding to range
        int typePick = fastRand() % typeRange;
        switch (typePick) {
            case 0: enemy_type[i] = TYPE_GRUNT;     enemy_base_sprite[i] = GRUNT_ID; break;
            case 1: enemy_type[i] = TYPE_HULK;      enemy_base_sprite[i] = HULK_ID; break;
            case 2: enemy_type[i] = TYPE_ELECTRODE; enemy_base_sprite[i] = ELECTRODE_ID; break;
            case 3: enemy_type[i] = TYPE_BRAIN;     enemy_base_sprite[i] = BRAIN_ID; break;
            case 4: enemy_type[i] = TYPE_QUARK;     enemy_base_sprite[i] = QUARK_ID; break;
            case 5: enemy_type[i] = TYPE_SPHEROID;  enemy_base_sprite[i] = SPHEROID_ID; break;
        }
    }

    // spawn Humans (idx 40 to 44)
	for (int i = 0; i < numHumans; i++) {
		int idx = 40 + i;
		if (idx >= NUM_ENEMIES) break;

		get_safe_pos(&enemy_x[idx], &enemy_y[idx]);

		// human type selection
		int hType = fastRand() % 3;

		// Assign type and sprite addrs
		if (hType == 0) {
			enemy_type[idx] = HUMAN_M;
			enemy_base_sprite[idx] = MAN_ADDR_FRONT1;
		}
		else if (hType == 1) {
			enemy_type[idx] = HUMAN_W;
			enemy_base_sprite[idx] = WOMAN_ADDR_FRONT1;
		}
		else {
			enemy_type[idx] = HUMAN_C;
			enemy_base_sprite[idx] = CHILD_ADDR_FRONT1;
		}
	}
}

/*
code below generates xy position for humans/enemies during wave gen
makes sure we aren't touching border or a 40x40 box in the center of the screen (where the player will be spawned)
*/
void get_safe_pos(int *x, int *y) {
    // Expand the bounding box to account for 16x16 sprite overlap
    // X range: 124 to 180 (keeps enemy right edge away from player left edge)
    // Y range: 84 to 140 (keeps enemy bottom edge away from player top edge)
    do {
    	// fastRand() yields 0 to 65535.
		// 65535 * 299 >> 16 = max offset of 298. (3 + 298 = 301 Max X)
		// 65535 * 197 >> 16 = max offset of 196. (18 + 196 = 214 Max Y)
		*x = 3 + ((fastRand() * 299) >> 16);
		*y = 18 + ((fastRand() * 197) >> 16);
	} while (*x >= 124 && *x <= 180 && *y >= 84 && *y <= 140);
}

// ascii to rom
int get_ascii_sprite(char c) {
    if (c >= 'A' && c <= 'H') return 50848 + (c - 'A') * 16;
    if (c >= 'I' && c <= 'X') return 55296 + (c - 'I') * 16;
    if (c == 'Y') return 55552; 
    if (c == 'Z') return 55568;
    if (c >= '0' && c <= '9') return 50688 + (c - '0') * 16;
    return -1; // invalid char
}

// draws string to screen
void drawString(int* current_reg, int start_x, int start_y, const char* str, int spacing) {
    int x = start_x;
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] != ' ') {
            int sprite_id = get_ascii_sprite(str[i]);
            if (sprite_id != -1 && *current_reg <= 510) { // bounds check
                uint32_t packed_xy = ((uint32_t)x << 16) | (start_y & 0xFFFF);
                Xil_Out32(HDMI_BASE + ((*current_reg) * 4), packed_xy);
                Xil_Out32(HDMI_BASE + (((*current_reg) + 1) * 4), sprite_id);
                (*current_reg) += 2;
            }
        }
        x += spacing; 
    }
}

void drawStartScreen() {
    int reg = 212; // display reg start

    // Clear screen
    for (int i = 212; i <= 511; i++) {
        Xil_Out32(HDMI_BASE + (i * 4), -1);
    }

    // ROBOTRON 2084
    drawString(&reg, 80, 20, "ROBOTRON 2084", 14);
    uint32_t packed_xy = ((uint32_t)196 << 16) | (20 & 0xFFFF);
    Xil_Out32(HDMI_BASE + (reg * 4), packed_xy);
    reg += 2;

    // Headers
    drawString(&reg, 85, 60, "LEADERBOARD", 14);
    drawString(&reg, 60, 90, "NAME   SCORE", 14);

    // Leaderboard Entries
    for (int i = 0; i < 5; i++) {
        int y = 115 + (i * 16);

        Xil_Out32(HDMI_BASE + ((39 + (i*2)) * 4), leader_names[i]);
        Xil_Out32(HDMI_BASE + ((40 + (i*2)) * 4), leader_score[i]);

        char name_str[5];
        for (int j = 0; j < 4; j++) {
            uint8_t hid = (leader_names[i] >> (j * 8)) & 0xFF;
            char c = ' ';
            if (hid >= 0x04 && hid <= 0x1D) c = 'A' + (hid - 0x04);
            else if (hid == 0x27) c = '0';
            else if (hid >= 0x1E && hid <= 0x26) c = '1' + (hid - 0x1E);
            name_str[j] = c;
        }
        name_str[4] = '\0';
        drawString(&reg, 60, y, name_str, 14);

		char score_str[10];
		uint32_t temp = leader_score[i];
		for(int d = 8; d >= 0; d--) {
			score_str[d] = '0' + (temp % 10);
			temp /= 10;
		}
		score_str[9] = '\0';
		drawString(&reg, 150, y, score_str, 14);
    }

    // Draw Instructions
    drawString(&reg, 85, 210, "PRESS ENTER", 14);
}

void drawEndScreen() {
    int reg = 212;

    // Clear screen
    for (int i = 212; i <= 511; i++) {
        Xil_Out32(HDMI_BASE + (i * 4), -1);
    }

    // Game Over
    drawString(&reg, 120, 100, "GAME OVER", 14);
    drawString(&reg, 85, 140, "PRESS ENTER", 14);
}

extern HID_DEVICE hid_device;
static XGpio Gpio_hex;

static BYTE addr = 1; 				//hard-wired USB address
const char* const devclasses[] = { " Uninitialized", " HID Keyboard", " HID Mouse", " Mass storage" };

BYTE GetDriverandReport() {
	BYTE i;
	BYTE rcode;
	BYTE device = 0xFF;
	BYTE tmpbyte;

	DEV_RECORD* tpl_ptr;
	xil_printf("Reached USB_STATE_RUNNING (0x40)\n");
	for (i = 1; i < USB_NUMDEVICES; i++) {
		tpl_ptr = GetDevtable(i);
		if (tpl_ptr->epinfo != NULL) {
			xil_printf("Device: %d", i);
			xil_printf("%s \n", devclasses[tpl_ptr->devclass]);
			device = tpl_ptr->devclass;
		}
	}
	//Query rate and protocol
	rcode = XferGetIdle(addr, 0, hid_device.interface, 0, &tmpbyte);
	if (rcode) {   //error handling
		xil_printf("GetIdle Error. Error code: ");
		xil_printf("%x \n", rcode);
	} else {
		xil_printf("Update rate: ");
		xil_printf("%x \n", tmpbyte);
	}
	xil_printf("Protocol: ");
	rcode = XferGetProto(addr, 0, hid_device.interface, &tmpbyte);
	if (rcode) {   //error handling
		xil_printf("GetProto Error. Error code ");
		xil_printf("%x \n", rcode);
	} else {
		xil_printf("%d \n", tmpbyte);
	}
	return device;
}

void printHex (u32 data, unsigned channel)
{
	XGpio_DiscreteWrite (&Gpio_hex, channel, data);
}

/*
 * Takes standard integers and packs them into Hex-aligned digits for the FPGA
 * Example: Score 123456789 becomes 0x23456789 in Lower, 0x00000001 in Upper
 */
void update_hud(uint32_t score, int health, int wave) {
    uint32_t lower_score = 0;
    uint32_t upper_score = 0;
    uint32_t temp_score = score;

    // Pack the first 8 digits into SCORE_REG_LOWER
    for (int i = 0; i < 8; i++) {
        uint32_t digit = temp_score % 10;
        lower_score |= (digit << (i * 4)); // Shift left by 4 bits (1 hex slot)
        temp_score /= 10;
    }
    // The 9th digit goes into SCORE_REG_UPPER
    upper_score = temp_score % 10;

    // Pack the 2 Wave digits into WAVE_REG
    uint32_t packed_wave = 0;
    packed_wave |= (wave % 10);              // Ones
    packed_wave |= ((wave / 10) % 10) << 4;  // Tens

    Xil_Out32(SCORE_REG_LOWER, lower_score);
    Xil_Out32(SCORE_REG_UPPER, upper_score);
    Xil_Out32(WAVE_REG, packed_wave);
    Xil_Out32(HEALTH_ADDR, health);
}


// MAIN FUNCTION
int main() {
    init_platform();
    XGpio_Initialize(&Gpio_hex, XPAR_GPIO_USB_KEYCODE_DEVICE_ID);
   	XGpio_SetDataDirection(&Gpio_hex, 1, 0x00000000); //configure hex display GPIO
   	XGpio_SetDataDirection(&Gpio_hex, 2, 0x00000000); //configure hex display GPIO

   	initLeaderboard();
   	system_init();

   	BYTE rcode;
	BOOT_MOUSE_REPORT buf;		//USB mouse report
	BOOT_KBD_REPORT kbdbuf;

	BYTE runningdebugflag = 0;//flag to dump out a bunch of information when we first get to USB_STATE_RUNNING
	BYTE errorflag = 0; //flag once we get an error device so we don't keep dumping out state info
	BYTE device;

	xil_printf("initializing MAX3421E...\n");
	MAX3421E_init();
	xil_printf("initializing USB...\n");
	USB_init();

	// setting init conditions
	int frameCt = 0;
	int anim_tick = 0;

    system_init(); // init all variables

	while (1) {
		xil_printf("."); //A tick here means one loop through the USB main handler
		frameCt++;
		MAX3421E_Task();
		USB_Task();
		if (GetUsbTaskState() == USB_STATE_RUNNING) {
			if (!runningdebugflag) {
				runningdebugflag = 1;
				device = GetDriverandReport();
			} else if (device == 1) {
				rcode = kbdPoll(&kbdbuf);

				if (rcode && rcode != hrNAK) {
					continue;
				}

				printHex (kbdbuf.keycode[0] + (kbdbuf.keycode[1]<<8) + (kbdbuf.keycode[2]<<16) + (kbdbuf.keycode[3]<<24), 1);

				// ---------------------------------------------------------
				// STATE 0 & 5: MENUS (use display regs)
				// ---------------------------------------------------------
				if (gameState == 0 || gameState == 5) {
					menuScreen(&kbdbuf);
				}
				// ---------------------------------------------------------
				// STATE 1: ENEMY SPAWNING
				// ---------------------------------------------------------
				else if (gameState == 1) { //using timer to freeze enemies
					if (spawn_timer > 0) {
						spawn_timer--;
					} else {
						gameState = 2;
						Xil_Out32(GAME_STATE_REG, gameState);
						spawn_timer = 60; 

						sprite_x = 149;
						sprite_y = 113;
					}

					// only push enemy sprites, no movement
					for (int i = 0; i < NUM_ENEMIES; i++) {
						if (enemy_base_sprite[i] != -1) {
							Xil_Out32(ENEMY_SPRITE_REG(i), enemy_base_sprite[i]);
							uint32_t packed_coords = (enemy_x[i] << 16) | (enemy_y[i] & 0xFFFF);
							Xil_Out32(ENEMY_COORD_REG(i), packed_coords);
						}
					}

					// same with player
					Xil_Out32(SPRITE_X_ADDR, sprite_x);
					Xil_Out32(SPRITE_Y_ADDR, sprite_y);
					update_hud(player_score, player_health, current_wave);
				}

				// ---------------------------------------------------------
				// STATE 2: PLAYER SPAWNING
				// ---------------------------------------------------------
				else if (gameState == 2) {
					if (spawn_timer > 0) {
						spawn_timer--;
					} else {
						gameState = 3;
						Xil_Out32(GAME_STATE_REG, gameState);
					}

					for (int i = 0; i < NUM_ENEMIES; i++) {
						if (enemy_base_sprite[i] != -1) {
							Xil_Out32(ENEMY_SPRITE_REG(i), enemy_base_sprite[i]);
							uint32_t packed_coords = (enemy_x[i] << 16) | (enemy_y[i] & 0xFFFF);
							Xil_Out32(ENEMY_COORD_REG(i), packed_coords);
						}
					}

					Xil_Out32(SPRITE_X_ADDR, sprite_x);
					Xil_Out32(SPRITE_Y_ADDR, sprite_y);
					update_hud(player_score, player_health, current_wave);
				}
				// ---------------------------------------------------------
                // STATE 3: GAMEPLAY
                // ---------------------------------------------------------
                else if (gameState == 3) {

                	if (player_iframes > 0) {
                	    player_iframes--;
                	}

                    // player movement
                    int move_x = 0;
                    int move_y = 0;

                    for (int i = 0; i < 6; i++) {
                        if (kbdbuf.keycode[i] == 0x1A) move_y -= 2; // W
                        if (kbdbuf.keycode[i] == 0x16) move_y += 2; // S
                        if (kbdbuf.keycode[i] == 0x04) move_x -= 2; // A
                        if (kbdbuf.keycode[i] == 0x07) move_x += 2; // D
                    }

                    // Apply movement
                    sprite_x += move_x;
                    sprite_y += move_y;

                    // Determine facing direction based on movement to use for sprite addrs
                    if (move_y < 0 && move_x == 0) player_dir = 0;      // UP
                    else if (move_y < 0 && move_x > 0) player_dir = 1;  // UP-RIGHT
                    else if (move_y == 0 && move_x > 0) player_dir = 2; // RIGHT
                    else if (move_y > 0 && move_x > 0) player_dir = 3;  // DOWN-RIGHT
                    else if (move_y > 0 && move_x == 0) player_dir = 4; // DOWN
                    else if (move_y > 0 && move_x < 0) player_dir = 5;  // DOWN-LEFT
                    else if (move_y == 0 && move_x < 0) player_dir = 6; // LEFT
                    else if (move_y < 0 && move_x < 0) player_dir = 7;  // UP-LEFT

                    int is_moving = (move_x != 0 || move_y != 0);
                    anim_tick++;
                    int sprite_addr = get_player_sprite_addr(player_dir, is_moving, anim_tick);

                    // Wall Collisions
                    if (sprite_x < 3) sprite_x = 3;
                    if (sprite_x > 301) sprite_x = 301;
                    if (sprite_y < 18) sprite_y = 18;
                    if (sprite_y > 214) sprite_y = 214;

                    // BULLET LOGIC
                    if (fire_cooldown > 0) fire_cooldown--;

                    // Read Arrow Keys
                    int s_up = 0, s_down = 0, s_left = 0, s_right = 0;
                    for (int i = 0; i < 6; i++) {
                        if (kbdbuf.keycode[i] == 0x52) s_up = 1;    // Up
                        if (kbdbuf.keycode[i] == 0x51) s_down = 1;  // Down
                        if (kbdbuf.keycode[i] == 0x50) s_left = 1;  // Left
                        if (kbdbuf.keycode[i] == 0x4F) s_right = 1; // Right
                    }

                    // firing
                    if (fire_cooldown == 0) {
                        int requested_dir = -1;
                        // 8-directional bullet paths
                        if (s_up && !s_right && !s_left)        requested_dir = 0;
                        else if (s_up && s_right)               requested_dir = 1;
                        else if (s_right && !s_up && !s_down)   requested_dir = 2;
                        else if (s_down && s_right)             requested_dir = 3;
                        else if (s_down && !s_right && !s_left) requested_dir = 4;
                        else if (s_down && s_left)              requested_dir = 5;
                        else if (s_left && !s_up && !s_down)    requested_dir = 6;
                        else if (s_up && s_left)                requested_dir = 7;

                        if (requested_dir != -1) {
                            // look for first available inactive bullet in the array
                            for (int i = 0; i < NUM_BULLETS; i++) {
                                if (!bullet_active[i]) {
                                    bullet_active[i] = 1;
                                    bullet_dir[i] = requested_dir;
                                    bullet_x[i] = sprite_x + 5;
                                    bullet_y[i] = sprite_y + 5;
                                    fire_cooldown = 5; // controls bullet rates
                                    break; //only spawn one bullet for a keypress
                                }
                            }
                        }
                    }

                    for (int i = 0; i < NUM_BULLETS; i++) {
                        if (bullet_active[i]) {
                            // Apply directional velocity
                            if (bullet_dir[i] == 0 || bullet_dir[i] == 1 || bullet_dir[i] == 7) bullet_y[i] -= bullet_speed;
                            if (bullet_dir[i] == 4 || bullet_dir[i] == 3 || bullet_dir[i] == 5) bullet_y[i] += bullet_speed;
                            if (bullet_dir[i] == 2 || bullet_dir[i] == 1 || bullet_dir[i] == 3) bullet_x[i] += bullet_speed;
                            if (bullet_dir[i] == 6 || bullet_dir[i] == 7 || bullet_dir[i] == 5) bullet_x[i] -= bullet_speed;

                            // Despawn bullet if it hits the borders
                            if (bullet_x[i] <= 2 || bullet_x[i] >= 312 || bullet_y[i] <= 17 || bullet_y[i] >= 225) {
                                bullet_active[i] = 0;
                            }
                        }
                    }

                    // push to mmio
                    for (int i = 0; i < NUM_BULLETS; i++) {
                        Xil_Out32(BULLET_X_ADDR(i), bullet_x[i]);
                        Xil_Out32(BULLET_Y_ADDR(i), bullet_y[i]);

                        int status = 0;
                        if (bullet_active[i]) {
                            status = (bullet_dir[i] << 1) | 1;
                        }
                        Xil_Out32(BULLET_STATUS_ADDR(i), status);
                    }

                    // Enemy Projectiles
                    for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
                        if (e_bID[i] != -1) {

                            // player bullet collision
                            int bullet_shot = 0;
                            for (int j = 0; j < NUM_BULLETS; j++) {
                                if (bullet_active[j]) {
                                    // overlap check
                                    if (bullet_x[j] < e_bulletX[i] + 16 && bullet_x[j] + 6 > e_bulletX[i] &&
                                        bullet_y[j] < e_bulletY[i] + 16 && bullet_y[j] + 6 > e_bulletY[i]) {
                                        bullet_active[j] = 0; // Destroy player bullet
                                        e_bID[i] = -1;        // Destroy enemy bullet
                                        player_score += 50;   // Award points for shooting projectile
                                        bullet_shot = 1;
                                        break;
                                    }
                                }
                            }

                            if (bullet_shot) {
                                // get rid of buller
                                Xil_Out32(HDMI_BASE + ((150 + (i*3) + 2) * 4), -1);
                                continue;
                            }

                            // custom enemy projectile behavior
							if (frameCt % 4 == 0) {

								if (e_bID[i] == TANK_BULLET_ADDR) {
									if (e_bLife[i] > 0) {
										e_bLife[i]--; // timer for enemy bullets that bounce on wall collision
										e_bID[i] = bulletBounce(&e_bulletX[i], &e_bulletY[i], &e_bvelocityX[i], &e_bvelocityY[i], e_bID[i], sprite_x, sprite_y);
									} else {
										e_bID[i] = -1; // Timer ran out, kill bullet
									}
								}
								else if (e_bID[i] == BRAIN_BULLET_ADDR) {
									// Brain missiles home in on the player
									e_bID[i] = bulletHoming(&e_bulletX[i], &e_bulletY[i], &e_bvelocityX[i], &e_bvelocityY[i], sprite_x, sprite_y, e_bID[i], sprite_x, sprite_y);
								}
								else {
									// Enforcer sparks go in a straight line
									e_bID[i] = bulletNoBounce(&e_bulletX[i], &e_bulletY[i], &e_bvelocityX[i], &e_bvelocityY[i], e_bID[i], sprite_x, sprite_y);
								}
							}

                            if (e_bID[i] == -100) {
                                e_bID[i] = -1;
                            }

                            if (e_bID[i] != -1) {
                                uint32_t packed_eb_xy = (e_bulletX[i] << 16) | (e_bulletY[i] & 0xFFFF);
                                Xil_Out32(HDMI_BASE + ((150 + (i*3)) * 4), packed_eb_xy);
                                Xil_Out32(HDMI_BASE + ((150 + (i*3) + 2) * 4), e_bID[i]);
                            } else {
                                Xil_Out32(HDMI_BASE + ((150 + (i*3) + 2) * 4), -1);
                            }
                        }
                    }

                    // enemy logic
                    for (int i = 0; i < NUM_ENEMIES; i++) {
                        if (enemy_base_sprite[i] != -1) {
                            // route based on enemy type
                            switch(enemy_type[i]) {
                                case TYPE_GRUNT:
                                    enemy_base_sprite[i] = grunt(&enemy_x[i], &enemy_y[i], enemy_base_sprite[i], bullet_x, bullet_y, bullet_active, sprite_x, sprite_y, frameCt);
                                    break;
                                case TYPE_ELECTRODE:
									enemy_base_sprite[i] = electrode(&enemy_x[i], &enemy_y[i], bullet_x, bullet_y, bullet_active, sprite_x, sprite_y, enemy_base_sprite[i], frameCt);
									break;
                                case TYPE_HULK:
									enemy_base_sprite[i] = hulk(&enemy_x[i], &enemy_y[i], &enemy_state1[i], &enemy_state2[i], bullet_x, bullet_y, bullet_active, bullet_dir, frameCt, sprite_x, sprite_y, enemy_base_sprite[i]);
									break;
                                case TYPE_SPHEROID:
                                    enemy_base_sprite[i] = spheroid(&enemy_x[i], &enemy_y[i], &enemy_state1[i], frameCt, bullet_x, bullet_y, bullet_active, sprite_x, sprite_y, enemy_base_sprite[i]);
                                    break;
                                case TYPE_ENFORCER:
                                    enemy_base_sprite[i] = enforcer(&enemy_x[i], &enemy_y[i], sprite_x, sprite_y, frameCt, bullet_x, bullet_y, bullet_active, enemy_base_sprite[i]);
                                    break;
                                case TYPE_BRAIN:
                                    enemy_base_sprite[i] = brain(&enemy_x[i], &enemy_y[i], sprite_x, sprite_y, bullet_x, bullet_y, bullet_active, frameCt, enemy_base_sprite[i]);
                                    break;
                                case TYPE_PROG:
                                    enemy_base_sprite[i] = prog(&enemy_x[i], &enemy_y[i], sprite_x, sprite_y, bullet_x, bullet_y, bullet_active, enemy_base_sprite[i], frameCt);
                                    break;
                                case TYPE_QUARK:
                                    enemy_base_sprite[i] = quark(&enemy_x[i], &enemy_y[i], frameCt, bullet_x, bullet_y, bullet_active, sprite_x, sprite_y, enemy_base_sprite[i]);
                                    break;
                                case TYPE_TANK:
                                    enemy_base_sprite[i] = tank(&enemy_x[i], &enemy_y[i], sprite_x, sprite_y, frameCt, bullet_x, bullet_y, bullet_active, enemy_base_sprite[i]);
                                    break;
                                case TYPE_SCORE:
									enemy_base_sprite[i] = scoreSprite(&enemy_x[i], &enemy_y[i], &enemy_state1[i], enemy_base_sprite[i]);
									break;
                                default: // Humans
                                    enemy_base_sprite[i] = human(&enemy_x[i], &enemy_y[i], &enemy_state1[i], &enemy_state2[i], sprite_x, sprite_y, &enemy_base_sprite[i], frameCt, &enemy_type[i]);
                                    break;
                            }

                            Xil_Out32(ENEMY_SPRITE_REG(i), enemy_base_sprite[i]);
                            if (enemy_base_sprite[i] != -1) {
                                uint32_t packed_coords = (enemy_x[i] << 16) | (enemy_y[i] & 0xFFFF);
                                Xil_Out32(ENEMY_COORD_REG(i), packed_coords);
                            }
                        }
                    }

                    // update gameplay HUD
                    update_hud(player_score, player_health, current_wave);

                    // write player data
                    Xil_Out32(SPRITE_X_ADDR, sprite_x);
                    Xil_Out32(SPRITE_Y_ADDR, sprite_y);
                    Xil_Out32(PLAYER_DIR_ADDR, sprite_addr);

                    // check for wave completion
					if (player_health <= 0) {
						gameState = 5;
						Xil_Out32(GAME_STATE_REG, gameState);
						newLeaderboard();

					}
					else if (waveDone()) {
						gameState = 4; // next state
						Xil_Out32(GAME_STATE_REG, gameState);
					}
                }
				// ---------------------------------------------------------
				// STATE 4: TRANSITION
				// ---------------------------------------------------------
				else if (gameState == 4) {
					int isDone = 0;
					while (!isDone){
						isDone = stageTransition();
						}

					if (isDone) {
						for (int i = 212; i <= 511; i++) {
							Xil_Out32(HDMI_BASE + (i * 4), -1);
						}

						current_wave++;
						initWave(current_wave);

						gameState = 1;
						Xil_Out32(GAME_STATE_REG, gameState);
						spawn_timer = 60;
						sprite_x = 400;
						sprite_y = 400;
					}
				}
			}

			else if (device == 2) {
				rcode = mousePoll(&buf);
				if (rcode == hrNAK) {
					//NAK means no new data
					continue;
				} else if (rcode) {
					xil_printf("Rcode: %x \n", rcode);
					continue;
				}
				xil_printf("X displacement: %d ", (signed char) buf.Xdispl);
				xil_printf("Y displacement: %d ", (signed char) buf.Ydispl);
				xil_printf("Buttons: %x\n", buf.button);
			}
		} else if (GetUsbTaskState() == USB_STATE_ERROR) {
			if (!errorflag) {
				errorflag = 1;
				xil_printf("USB Error State\n");
				//print out string descriptor here
			}
		} else { //not in USB running state
			xil_printf("USB task state: %x\n", GetUsbTaskState());
			if (runningdebugflag) {	//previously running, reset USB hardware just to clear out any funky state, HS/FS etc
				runningdebugflag = 0;
				MAX3421E_init();
				USB_init();
			}
			errorflag = 0;
		}

        // new life after 25k points 
		if (player_score >= new_life) {
			new_life += 25000;
			player_health++;
            update_hud(player_score, player_health, current_wave);
		}
	}

    cleanup_platform();
	return 0;
}

// Utility Functions

/*
returns 1 if there is a collision, 0 if none. this is just for debugging purposes.
compares 16x16 player square with 16x16 enemy squares and bullet squares. pXY and eXY are top left corners of the squares
decreases player health if there is a collision with enemy
no change if collision with
*/
int playerCollision(int pX, int pY, int eX, int eY, int eID) {
    // overlap
    if (pX < eX + 16 && pX + 16 > eX && pY < eY + 16 && pY + 16 > eY) {

        // Check if the entity is a human
        int is_human = (
            eID == MAN_ADDR_FRONT1 || eID == MAN_ADDR_FRONT2 || eID == MAN_ADDR_FRONT3 ||
            eID == MAN_ADDR_RIGHT1 || eID == MAN_ADDR_RIGHT2 || eID == MAN_ADDR_RIGHT3 ||
            eID == MAN_ADDR_LEFT1 || eID == MAN_ADDR_LEFT2 || eID == MAN_ADDR_LEFT3 ||
            eID == MAN_ADDR_BACK1 || eID == MAN_ADDR_BACK2 || eID == MAN_ADDR_BACK3 ||
            eID == WOMAN_ADDR_FRONT1 || eID == WOMAN_ADDR_FRONT2 || eID == WOMAN_ADDR_FRONT3 ||
            eID == WOMAN_ADDR_RIGHT1 || eID == WOMAN_ADDR_RIGHT2 || eID == WOMAN_ADDR_RIGHT3 ||
            eID == WOMAN_ADDR_LEFT1 || eID == WOMAN_ADDR_LEFT2 || eID == WOMAN_ADDR_LEFT3 ||
            eID == WOMAN_ADDR_BACK1 || eID == WOMAN_ADDR_BACK2 || eID == WOMAN_ADDR_BACK3 ||
            eID == CHILD_ADDR_FRONT1 || eID == CHILD_ADDR_FRONT2 || eID == CHILD_ADDR_FRONT3 ||
            eID == CHILD_ADDR_RIGHT1 || eID == CHILD_ADDR_RIGHT2 || eID == CHILD_ADDR_RIGHT3 ||
            eID == CHILD_ADDR_LEFT1 || eID == CHILD_ADDR_LEFT2 || eID == CHILD_ADDR_LEFT3 ||
            eID == CHILD_ADDR_BACK1 || eID == CHILD_ADDR_BACK2 || eID == CHILD_ADDR_BACK3
        );

        // Deduct health
        if (player_iframes == 0 && !is_human) {
            player_health--;
            player_iframes = 60; // approx 1 second of invincibility
        }
        return 1;
    }
    return 0;
}

int get_player_sprite_addr(int dir, int is_moving, int anim_tick) {
    int offset_x = 96; // default to facing down
    int offset_y = 80; // default to facing down

    switch(dir) {
        case 0: // UP
            offset_x = 144; offset_y = 80; break;
        case 4: // DOWN
            offset_x = 96; offset_y = 80; break;
        case 1: // UP-RIGHT
        case 2: // RIGHT
        case 3: // DOWN-RIGHT
            offset_x = 48; offset_y = 80; break;
        case 5: // DOWN-LEFT
        case 6: // LEFT
        case 7: // UP-LEFT
            offset_x = 0; offset_y = 80; break;
    }

    // cycle frames
    if (is_moving) {
        int frame = (anim_tick / 10) % 2;
        if (frame == 0) offset_x += 16; // first walking frame
        else offset_x += 32; // second walking frame
    }
    return offset_x + (offset_y * SHEET_WIDTH);
}

int get_animated_enemy_sprite(int enemy_type, int base_id, int frameCount, int dx, int dy) {
    int anim_speed = 10;
    int num_frames = 1;
    int active_base = base_id;

    // similar logic but for enemies
    if (enemy_type == HUMAN_M || enemy_type == HUMAN_W || enemy_type == HUMAN_C ||
        enemy_type == TYPE_GRUNT || enemy_type == TYPE_HULK ||
        enemy_type == TYPE_BRAIN || enemy_type == TYPE_PROG) {

        num_frames = 3; // 3 frame walk cycles

        if (enemy_type == HUMAN_M) {
            anim_speed = 8;
            if (dy > 0) active_base = MAN_ADDR_FRONT1;
            else if (dy < 0) active_base = MAN_ADDR_BACK1;
            else if (dx > 0) active_base = MAN_ADDR_RIGHT1;
            else if (dx < 0) active_base = MAN_ADDR_LEFT1;
            else active_base = MAN_ADDR_FRONT1; 
        }
        else if (enemy_type == HUMAN_W) {
            anim_speed = 8;
            if (dy > 0) active_base = WOMAN_ADDR_FRONT1;
            else if (dy < 0) active_base = WOMAN_ADDR_BACK1;
            else if (dx > 0) active_base = WOMAN_ADDR_RIGHT1;
            else if (dx < 0) active_base = WOMAN_ADDR_LEFT1;
            else active_base = WOMAN_ADDR_FRONT1;
        }
        else if (enemy_type == HUMAN_C) {
            anim_speed = 8;
            if (dy > 0) active_base = CHILD_ADDR_FRONT1;
            else if (dy < 0) active_base = CHILD_ADDR_BACK1;
            else if (dx > 0) active_base = CHILD_ADDR_RIGHT1;
            else if (dx < 0) active_base = CHILD_ADDR_LEFT1;
            else active_base = CHILD_ADDR_FRONT1;
        }
        else if (enemy_type == TYPE_GRUNT || enemy_type == TYPE_PROG) {
            anim_speed = 6;
            if (dy > 0) active_base = GRUNT_ADDR_FRONT;
            else if (dy < 0) active_base = GRUNT_ADDR_BACK;
            else if (dx > 0) active_base = GRUNT_ADDR_RIGHT;
            else if (dx < 0) active_base = GRUNT_ADDR_LEFT;
            else active_base = GRUNT_ADDR_FRONT;
        }
        else if (enemy_type == TYPE_HULK) {
            anim_speed = 16; // Hulks are slow, animate slower
            if (dy > 0) active_base = HULK_ADDR_FRONT;
            else if (dy < 0) active_base = HULK_ADDR_BACK;
            else if (dx > 0) active_base = HULK_ADDR_RIGHT;
            else if (dx < 0) active_base = HULK_ADDR_LEFT;
            else active_base = HULK_ADDR_FRONT;
        }
        else if (enemy_type == TYPE_BRAIN) {
            anim_speed = 12;
            if (dy > 0) active_base = BRAIN_ADDR_FRONT;
            else if (dy < 0) active_base = BRAIN_ADDR_BACK;
            else if (dx > 0) active_base = BRAIN_ADDR_RIGHT;
            else if (dx < 0) active_base = BRAIN_ADDR_LEFT;
            else active_base = BRAIN_ADDR_FRONT;
        }
    }
    else {
        // Entities with unique non 3 cycle animations
        switch(enemy_type) {
            case TYPE_TANK:
                anim_speed = 12;
                num_frames = 2; //slow
                break;
            case TYPE_ENFORCER:
            	num_frames = 1; // static
            case TYPE_QUARK:
				anim_speed = 6;
				num_frames = 7; //7 frame seq
				active_base = 36912;
				break;
            case TYPE_SPHEROID:
				anim_speed = 6;
				num_frames = 9; // 9 frame seq
				active_base = 13968;
				break;
        }
    }

    //static sprite
    if (num_frames <= 1) return active_base;

    int current_frame = (frameCount / anim_speed) % num_frames;
    return active_base + (current_frame * 16);
}

// grunt code
int grunt(int* eX, int* eY, int eID, int bX[], int bY[], int bOn[], int pX, int pY, int frameCount) {
	// collision detection, does enemy sprite overlap with bullet coords
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
                // bullet hit enemy, delete bullet and enemy
				player_score += 100;
                bOn[i] = 0;  // deactivate bullet
                *eX = -100;  // mark enemy as deleted (off-screen or dead)
                *eY = -100;
                return -1;   // enemy has been deleted
            }
        }
    }
	// change this to change player speed
    // move towards player
    int dx = 0, dy = 0;
    if (frameCount % 2 == 0) {
		if (*eX < pX) { (*eX)++; dx = 1; }
		else if (*eX > pX) { (*eX)--; dx = -1; }

		if (*eY < pY) { (*eY)++; dy = 1; }
		else if (*eY > pY) { (*eY)--; dy = -1; }
	}

	playerCollision(pX, pY, *(eX), *(eY), eID);

	return get_animated_enemy_sprite(TYPE_GRUNT, GRUNT_ID, frameCount, dx, dy);
}

// hulk movement
int hulk(int* eX, int* eY, int* dirX, int* dirY, int bX[], int bY[], int bOn[], int bDir[], int frameCount, int pX, int pY, int eID) {

    int hit = 0;

    // bullets do not destroy Hulk, but push them back
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
                bOn[i] = 0; // Destroy bullet
                hit = 1;    // Stun Hulk
                
                // stun by pushing back in bullet dir
                int dir = bDir[i];
                if (dir == 0 || dir == 1 || dir == 7) *eY -= 6; 
                if (dir == 4 || dir == 3 || dir == 5) *eY += 6; 
                if (dir == 2 || dir == 1 || dir == 3) *eX += 6; 
                if (dir == 6 || dir == 7 || dir == 5) *eX -= 6; 
            }
        }
    }

    // occasionally change direction (Slower decision making)
    if ((frameCount % 60) == 0 && !hit) {
        int r = fastRand() & 3;
        *dirX = (r == 0) ? 1 : (r == 1) ? -1 : 0; 
        *dirY = (r == 2) ? 1 : (r == 3) ? -1 : 0; 
    }

    // move hulk every second frame to slow down
    if ((frameCount % 2 == 0) && !hit) {
        *eX += *dirX;
        *eY += *dirY;
    }

    playerCollision(pX, pY, *(eX), *(eY), eID);

    // boundary check
    if (*eX <= 2 || *eX >= SCREEN_W - 16) *dirX *= -1;
    if (*eY <= 18 || *eY >= SCREEN_H - 32) *dirY *= -1;
    if (*eX < 2) *eX = 2;
    if (*eX > SCREEN_W - 16) *eX = SCREEN_W - 16;
    if (*eY < 18) *eY = 18;
    if (*eY > SCREEN_H - 32) *eY = SCREEN_H - 32;

    return get_animated_enemy_sprite(TYPE_HULK, HULK_ID, frameCount, *dirX, *dirY);
}

// enforcer move
int enforcer(int* eX, int* eY, int pX, int pY, int frameCount, int bX[], int bY[], int bOn[], int eID) {
	// Check Bullet Collisions
	for (int i = 0; i < NUM_BULLETS; i++) {
		if (bOn[i]) {
			if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
				player_score += 150; 
				bOn[i] = 0;   
				*eX = -100;      
				*eY = -100;
				return -1; 
			}
		}
	}
    // floaty movement
    if (frameCount % 8 == 0) {
        *eX += (fastRand() & 3) - 1;
        *eY += (fastRand() & 3) - 1;
    }
	playerCollision(pX, pY, *(eX), *(eY), eID);
    // shoot near player
    if (frameCount % 50 == 0) {
        int offsetX = (fastRand() & 15) - 8;
        int offsetY = (fastRand() & 15) - 8;
        spawnEnforcerBullet(*eX, *eY, pX + offsetX, pY + offsetY); // write this function
    }
    if (*eX < 2) *eX = 2;
	if (*eX > SCREEN_W - 16) *eX = SCREEN_W - 16;
	if (*eY < 18) *eY = 18;
	if (*eY > SCREEN_H - 24) *eY = SCREEN_H - 24;
    return get_animated_enemy_sprite(TYPE_ENFORCER, ENFORCER_ID, frameCount, 0 , 0);
}

// spheroid movement
int spheroid(int* eX, int* eY, int* spawnCount, int frameCount, int bX[], int bY[], int bOn[], int pX, int pY, int eID) {
    // Check Bullet Collisions
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
                player_score += 1000;
                bOn[i] = 0;
                *eX = -100;
                *eY = -100;
                return -1;
            }
        }
    }

    if (frameCount % 6 == 0) {
        *eX += (fastRand() & 15) - 7;
        *eY += (fastRand() & 15) - 7;
    }

    if (*eX < 2) *eX = 2;
    if (*eX > SCREEN_W - 16) *eX = SCREEN_W - 16;
    if (*eY < 18) *eY = 18;
    if (*eY > SCREEN_H - 24) *eY = SCREEN_H - 24;

    if (frameCount % 120 == 0 && *spawnCount < 3) {
        spawnEnforcer(*eX, *eY);
        (*spawnCount)++;
    }

    playerCollision(pX, pY, *(eX), *(eY), eID);
    return get_animated_enemy_sprite(TYPE_SPHEROID, SPHEROID_ID, frameCount, 0, 0);
}

// brain move
int brain(int* eX, int* eY, int pX, int pY, int bX[], int bY[], int bOn[], int frameCount, int eID) {
	// Check Bullet Collisions
	for (int i = 0; i < NUM_BULLETS; i++) {
		if (bOn[i]) {
			if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
				player_score += 500;
				bOn[i] = 0; 
				*eX = -100;  
				*eY = -100;
				return -1;  
			}
		}
	}

    int targetX = -1, targetY = -1; // find human with minimum distance
	int minDist = 400; // min distance in 1 dimension. always positive. use to determine which human to set target as
    for (int i = 0; i < NUM_ENEMIES; i++) {
		int eID_human = enemy_type[i];
        // if entity is a human
        if (eID_human == MAN_ADDR_FRONT1 || eID_human == WOMAN_ADDR_FRONT1 || eID_human == CHILD_ADDR_FRONT1) {
            int xDist = (enemy_x[i] > *eX) ? (enemy_x[i] - *eX) : (*eX - enemy_x[i]);
            int yDist = (enemy_y[i] > *eY) ? (enemy_y[i] - *eY) : (*eY - enemy_y[i]);

            if (xDist < minDist) { minDist = xDist; targetX = enemy_x[i]; targetY = enemy_y[i]; }
            if (yDist < minDist) { minDist = yDist; targetX = enemy_x[i]; targetY = enemy_y[i]; }
            break;
        }
    }

    int dx = 0, dy = 0;
    if (frameCount % 8 == 0) {
		if (targetX != -1) {
			if (*eX < targetX) { (*eX)++; dx = 1; }
			else if (*eX > targetX) { (*eX)--; dx = -1; }
			if (*eY < targetY) { (*eY)++; dy = 1; }
			else if (*eY > targetY) { (*eY)--; dy = -1; }
		} else {
			if (*eX < pX) { (*eX)++; dx = 1; }
			else if (*eX > pX) { (*eX)--; dx = -1; }
			if (*eY < pY) { (*eY)++; dy = 1; }
			else if (*eY > pY) { (*eY)--; dy = -1; }
		}
	}

    // wandering projectile
    if (frameCount % 60 == 0) {
        int offsetX = (fastRand() & 31) - 16;
        int offsetY = (fastRand() & 31) - 16;
        spawnBrainProjectile(*eX, *eY, *eX + offsetX, *eY + offsetY); // write this function
    }
	playerCollision(pX, pY, *(eX), *(eY), eID);
	return get_animated_enemy_sprite(TYPE_BRAIN, BRAIN_ID, frameCount, dx, dy);
}

// prog move, just faster grunt basically
int prog(int* eX, int* eY, int pX, int pY, int bX[], int bY[], int bOn[], int eID, int frameCount) {
	for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
                // bullet hit enemy, delete bullet and enemy
				player_score += 100;
                bOn[i] = 0;  // deactivate bullet
                *eX = -100;  // mark enemy as deleted (off-screen or dead)
                *eY = -100;
                return -1;   // enemy has been deleted
            }
        }
    }
    if (*eX < pX) (*eX) += 1;
    else if (*eX > pX) (*eX) -= 1;
    if (*eY < pY) (*eY) += 1;
    else if (*eY > pY) (*eY) -= 1;
	playerCollision(pX, pY, *(eX), *(eY), eID);
    return PROG_ID;
}

// quark move
int quark(int* eX, int* eY, int frameCount, int bX[], int bY[], int bOn[], int pX, int pY, int eID) {
	// Check Bullet Collisions
	for (int i = 0; i < NUM_BULLETS; i++) {
		if (bOn[i]) {
			if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
				player_score += 1000;
				bOn[i] = 0;  
				*eX = -100; 
				*eY = -100;
				return -1;
			}
		}
	}
    if (frameCount % 6 == 0) {
        *eX += (fastRand() & 7) - 3;
        *eY += (fastRand() & 7) - 3;
    }

    // quark bounding
	if (*eX < 2) *eX = 2;
	if (*eX > SCREEN_W - 16) *eX = SCREEN_W - 16;
	if (*eY < 18) *eY = 18;
	if (*eY > SCREEN_H - 24) *eY = SCREEN_H - 24;

    if (frameCount % 120 == 0) {
        spawnTank(*eX, *eY); // write this function
    }
	playerCollision(pX, pY, *(eX), *(eY), eID);
    return get_animated_enemy_sprite(TYPE_QUARK, QUARK_ID, frameCount, 0 ,0);
}

// tanks
int tank(int* eX, int* eY, int pX, int pY, int frameCount, int bX[], int bY[], int bOn[], int eID) {
    // bullets should destroy tank
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
					player_score += 200;
					bOn[i] = 0;
					*eX = -100;     // mark enemy as deleted (off-screen or dead)
                	*eY = -100;
                	return -1;      // enemy has been deleted
            }
        }
    }
    // slow movement
    if (frameCount % 3 == 0) {
        if (*eX < pX) (*eX)++;
        else if (*eX > pX) (*eX)--;
        if (*eY < pY) (*eY)++;
        else if (*eY > pY) (*eY)--;
    }
    // 20-shot spread with slight randomness
    if (frameCount % 80 == 0) {
        for (int i = 0; i < 20; i++) {
            int offsetX = ((i % 5) * 4 - 10) + ((fastRand() & 3) - 1);
            int offsetY = ((i / 5) * 4 - 10) + ((fastRand() & 3) - 1);
            spawnTankBullet(*eX, *eY, pX + offsetX, pY + offsetY); // write this function
        }
    }
	playerCollision(pX, pY, *(eX), *(eY), eID);
    return get_animated_enemy_sprite(TYPE_TANK, TANK_ID, frameCount, 0 , 0);
}

static unsigned int x = 123456789, y = 362436069, z = 521288629; // Seeds for psuedo random func

unsigned short fastRand() {
    unsigned int t;
    x ^= x << 16;
    x ^= x >> 5;
    x ^= x << 1;

    t = x;
    x = y;
    y = z;
    z = t ^ x ^ y;

    return (unsigned short)(z & 0xFFFF);
}

int electrode(int* eX, int* eY, int bX[], int bY[], int bOn[], int pX, int pY, int eID, int frameCount) {
    // Check Bullet Collisions
    for (int i = 0; i < NUM_BULLETS; i++) {
        if (bOn[i]) {
            if ((bX[i] >= *eX && bX[i] <= *eX + 16) && (bY[i] >= *eY && bY[i] <= *eY + 16)) {
                bOn[i] = 0;  
                *eX = -100; 
                *eY = -100;
                return -1;   
            }
        }
    }
    // electrodes are stationary, check for player collision
    playerCollision(pX, pY, *(eX), *(eY), eID);

    return get_animated_enemy_sprite(TYPE_ELECTRODE, ELECTRODE_ID, frameCount, 0, 0);
}


// code for bullet that bounces, only accounts for wall collision
int bulletBounce(int* x, int* y, int* vx, int* vy, int id, int pX, int pY) {
    // update position
    *x += *vx;
    *y += *vy;

    // check X bounds
    if (*x <= 0) {
        *x = 0;
        *vx = -(*vx);
    }
    else if (*x >= SCREEN_W) {
        *x = SCREEN_W;
        *vx = -(*vx);
    }

    // check Y bounds
    if (*y <= 20) {
        *y = 20;
        *vy = -(*vy);
    }
    else if (*y >= SCREEN_H-20) {
        *y = SCREEN_H-20;
        *vy = -(*vy);
    }
    if (playerCollision(pX, pY, *(x), *(y), id)) {
        return -100;
    }
    return id;
}

// non bouncing bullet
int bulletNoBounce(int* x, int* y, int* vx, int* vy, int id, int pX, int pY) {
    // update position
    *x += *vx;
    *y += *vy;

    // check bounds
    if (*x < 0 || *x > SCREEN_W || *y < 20 || *y > SCREEN_H-20) {
        return -100;  // signal despawn
    }
    if (playerCollision(pX, pY, *(x), *(y), id)) {
		return -100;
	}
    return id;
}

// homing bullet, doesnt bounce
int bulletHoming(int* x, int* y, int* vx, int* vy, int targetX, int targetY, int id, int pX, int pY) {
    // adjust velocity toward target
    if (*x < targetX) (*vx)++;
    else if (*x > targetX) (*vx)--;

    if (*y < targetY) (*vy)++;
    else if (*y > targetY) (*vy)--;

    // clamp velocity
    if (*vx > 3) *vx = 3;
    if (*vx < -3) *vx = -3;
    if (*vy > 3) *vy = 3;
    if (*vy < -3) *vy = -3;

    // update position
    *x += *vx;
    *y += *vy;

    // despawn if out of bounds
    if (*x < 0 || *x > SCREEN_W || *y < 0 || *y > SCREEN_H) {
        return -100;
    }
    if (playerCollision(pX, pY, *(x), *(y), id)) {
		return -100;
	}
    return id;
}


void spawnEnforcer(int x, int y) {
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (enemy_base_sprite[i] == -1) { // find empty slot
            enemy_x[i] = x;
            enemy_y[i] = y;
            enemy_base_sprite[i] = ENFORCER_ID;
            enemy_type[i] = TYPE_ENFORCER;
            break; //only spawn one
        }
    }
}

void spawnTank(int x, int y) {
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (enemy_base_sprite[i] == -1) { //same logic
            enemy_x[i] = x;
            enemy_y[i] = y;
            enemy_base_sprite[i] = TANK_ID;
            enemy_type[i] = TYPE_TANK;

            int r = fastRand() & 3;
			enemy_state1[i] = (r == 0) ? 2 : (r == 1) ? -2 : 0; // dirX
			enemy_state2[i] = (r == 2) ? 2 : (r == 3) ? -2 : 0; // dirY
            break;
        }
    }
}

void spawnEnforcerBullet(int startX, int startY, int targetX, int targetY) {
    // find an empty slot for the bullet
    for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
        if (e_bID[i] == -1) { // empty slot found
            // set bullet starting position
            e_bulletX[i] = startX + 8;  // center of enemy (16x16 sprite)
            e_bulletY[i] = startY + 8;

            // calculate direction vector from start to target
            int dx = targetX - startX;
            int dy = targetY - startY;

            // normalize and set velocity (speed 3)
            if (dx != 0 || dy != 0) {
                // simple scaling to get reasonable speed
                if (dx < 0) e_bvelocityX[i] = -2;
                else if (dx > 0) e_bvelocityX[i] = 2;
                else e_bvelocityX[i] = 0;

                if (dy < 0) e_bvelocityY[i] = -2;
                else if (dy > 0) e_bvelocityY[i] = 2;
                else e_bvelocityY[i] = 0;

                // handle diagonals to maintain speed
                if (dx != 0 && dy != 0) {
                    e_bvelocityX[i] = (dx > 0) ? 1 : -1;
                    e_bvelocityY[i] = (dy > 0) ? 1 : -1;
                }
            } else {
                // default direction if target is same as start
                e_bvelocityX[i] = 0;
                e_bvelocityY[i] = 2;
            }
            e_bID[i] = ENFORCER_BULLET_ADDR; // mark as active enforcer bullet
            break;
        }
    }
}

void spawnBrainProjectile(int startX, int startY, int targetX, int targetY) {
    // find an empty slot for the projectile
    for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
        if (e_bID[i] == -1) { // empty slot found
            // set projectile starting position
            e_bulletX[i] = startX + 8; //same logic
            e_bulletY[i] = startY + 8;

            int dx = targetX - startX;
            int dy = targetY - startY;

            if (dx != 0 || dy != 0) {
                if (dx < 0) e_bvelocityX[i] = -1;
                else if (dx > 0) e_bvelocityX[i] = 1;
                else e_bvelocityX[i] = 0;

                if (dy < 0) e_bvelocityY[i] = -1;
                else if (dy > 0) e_bvelocityY[i] = 1;
                else e_bvelocityY[i] = 0;
            } else {
                e_bvelocityX[i] = 0;
                e_bvelocityY[i] = 1;
            }
            e_bID[i] = BRAIN_BULLET_ADDR; 
            break;
        }
    }
}

void spawnTankBullet(int startX, int startY, int targetX, int targetY) { // same logic
    for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
        if (e_bID[i] == -1) {
            e_bulletX[i] = startX + 8; 
            e_bulletY[i] = startY + 8;

            int dx = targetX - startX;
            int dy = targetY - startY;

            if (dx != 0 || dy != 0) {
                if (dx < 0) e_bvelocityX[i] = -3;
                else if (dx > 0) e_bvelocityX[i] = 3;
                else e_bvelocityX[i] = 0;

                if (dy < 0) e_bvelocityY[i] = -3;
                else if (dy > 0) e_bvelocityY[i] = 3;
                else e_bvelocityY[i] = 0;

                if (dx != 0 && dy != 0) {
                    e_bvelocityX[i] = (dx > 0) ? 2 : -2;
                    e_bvelocityY[i] = (dy > 0) ? 2 : -2;
                }
            } else {
                e_bvelocityX[i] = 0;
                e_bvelocityY[i] = 3;
            }
            e_bID[i] = TANK_BULLET_ADDR;
            e_bLife[i] = 180;
            break;
        }
    }
}


// human ai function
/*
parse thru enemy array
if touching brain, return prog ID
random move
if touching player, despawn and iter score based on humans saved
*/
int human(int* eX, int* eY, int* dirX, int* dirY, int pX, int pY, int* eID, int frameCount, int* eType) {
    int hit = 0;
	for (int i = 0; i < NUM_ENEMIES; i++) {
		if (enemy_type[i] == TYPE_BRAIN) {
			if (enemy_x[i] < *eX + 16 &&
        		enemy_x[i] + 16 > *eX &&
        		enemy_y[i] < *eY + 16 &&
        		enemy_y[i] + 16 > *eY) {

                *(eType) = TYPE_PROG;
                *(eID) = PROG_ID;
				return PROG_ID;

            }
        }
	}

	// occasionally change direction
    if ((frameCount % 30) == 0 && !hit) {
        int r = fastRand() & 3;
        *dirX = (r == 0) ? 2 : (r == 1) ? -2 : 0;
        *dirY = (r == 2) ? 2 : (r == 3) ? -2 : 0;
    }

    *eX += *dirX;
    *eY += *dirY;

	if (*eX <= 2 || *eX >= SCREEN_W - 16) *dirX *= -1;
	if (*eY <= 18 || *eY >= SCREEN_H - 24) *dirY *= -1;

	if (*eX < 2) *eX = 2;
	if (*eX > SCREEN_W - 16) *eX = SCREEN_W - 16;
	if (*eY < 18) *eY = 18;
	if (*eY > SCREEN_H - 24) *eY = SCREEN_H - 24;

	if (playerCollision(pX, pY, *(eX), *(eY), *(eID))) {
		humansSaved++;

		int points = 5000;
		int s_id = SCORE_5000;

		if (humansSaved == 1)      { points = 1000; s_id = SCORE_1000; }
		else if (humansSaved == 2) { points = 2000; s_id = SCORE_2000; }
		else if (humansSaved == 3) { points = 3000; s_id = SCORE_3000; }
		else if (humansSaved == 4) { points = 4000; s_id = SCORE_4000; }

		player_score += points;

		spawnScore(*eX, *eY, s_id);

		return -1; // despawn human
	}

	return get_animated_enemy_sprite(*eType, *eID, frameCount, *dirX, *dirY);
}


/*
init leaderboard at beginning of program.
init names to NONE, x11, x12, x11, x08
init scores to 0
*/
void initLeaderboard() {
	for (int i = 0; i < 5; i++) {
		leader_names[i] = 0x08111211; // converts to NONE
		leader_score[i] = 0;
	}
}

/*
compare global uint32_t player_score variable to all 5 current scores
if larger, replace that score and shift all scores below down (lowest score is kicked off leaderboard)
poll for 4 keycodes and fill in the leader_names slot correspondingly
each keycode byte occupies a byte in the 4 byte register, in the order of the letters (reference initLeaderboard())
*/
void newLeaderboard() {
    // check if player_score beats any existing score
    int insert_pos = -1;
    for (int i = 0; i < 5; i++) {
        if (player_score > leader_score[i]) {
            insert_pos = i;
            break;
        }
    }

    for (int i = 212; i <= 511; i++) {
            Xil_Out32(HDMI_BASE + (i * 4), -1);
        }

    int reg = 212;

    if (insert_pos == -1) {
		drawString(&reg, 120, 100, "GAME OVER", 14);
		drawString(&reg, 85, 140, "PRESS ENTER", 14);
		return;
	}

    // shift lower scores down (from bottom up to maintain order)
    for (int i = 4; i > insert_pos; i--) {
        leader_score[i] = leader_score[i-1];
        leader_names[i] = leader_names[i-1];
    }

    // insert the new score
    leader_score[insert_pos] = player_score;
    drawString(&reg, 120, 40, "GAME OVER", 14);
    drawString(&reg, 85, 80, "LEADERBOARD", 14);

    // poll for 4 keycodes to create the player's name
    // each keycode byte occupies a byte in the 4-byte register
    // format: byte0 = least significant byte (rightmost character on display)
    //         byte3 = most significant byte (leftmost character on display)
    // Convert the player's score to a 9-digit string
	char score_str[10];
	uint32_t temp = player_score;
	for(int d = 8; d >= 0; d--) {
		score_str[d] = '0' + (temp % 10);
		temp /= 10;
	}
	score_str[9] = '\0';

	// Draw score on right 
	drawString(&reg, 150, 120, score_str, 14);

	int name_reg_start = reg;

	reg += 10;
	drawString(&reg, 85, 200, "TYPE NAME AND", 14);
	reg += 10;
	drawString(&reg, 85, 216, "PRESS ENTER ", 14);

	BOOT_KBD_REPORT kbdbuf;
	BYTE rcode;
	int all_keys_released = 0;
	while (!all_keys_released) {
		MAX3421E_Task();
		USB_Task();
		if (GetUsbTaskState() == USB_STATE_RUNNING) {
			if (kbdPoll(&kbdbuf) == 0) {
				all_keys_released = 1; // Assume released
				for(int j = 0; j < 6; j++) {
					if(kbdbuf.keycode[j] != 0) {
						all_keys_released = 0; // Found a pressed key, keep waiting
					}
				}
			}
		}
	}


	// Live Typing Loop
	uint32_t name_bytes = 0;
	char live_name[5] = "_   "; // underscore cursor

	for (int char_idx = 0; char_idx < 4; char_idx++) {

		// update current frame
		int temp_reg = name_reg_start;
		drawString(&temp_reg, 85, 120, live_name, 14);

		int got_key = 0;

		while (!got_key) {
			MAX3421E_Task();
			USB_Task();

			if (GetUsbTaskState() == USB_STATE_RUNNING) {
				rcode = kbdPoll(&kbdbuf);
				if (rcode == 0) {
					for (int i = 0; i < 6; i++) {
						if (kbdbuf.keycode[i] != 0) {
							uint8_t hid = kbdbuf.keycode[i];

							// convert to char
							char c = ' ';
							if (hid >= 0x04 && hid <= 0x1D) c = 'A' + (hid - 0x04);
							else if (hid == 0x27) c = '0';
							else if (hid >= 0x1E && hid <= 0x26) c = '1' + (hid - 0x1E);

							// check for invalid char
							if (c != ' ') {
								live_name[char_idx] = c;
								live_name[char_idx + 1] = (char_idx < 3) ? '_' : ' ';

								uint32_t shift = char_idx * 8;
								name_bytes |= ((uint32_t)hid) << shift;
								got_key = 1;

								// wait for key to release so not double typing
								int key_released = 0;
								while(!key_released) {
									MAX3421E_Task();
									USB_Task();
									if (kbdPoll(&kbdbuf) == 0) {
										int still_pressed = 0;
										for(int j=0; j<6; j++) {
											if(kbdbuf.keycode[j] == hid) still_pressed = 1;
										}
										if(!still_pressed) key_released = 1;
									}
								}
								break;
							}
						}
					}
				}
			}
		}
	}

	int temp_reg = name_reg_start;
	drawString(&temp_reg, 85, 120, live_name, 14);
    // store the name in the leaderboard
    leader_names[insert_pos] = name_bytes;
}

/*
menu screen code
use game state register to determine where to go next
upon recieving enter key (keycode x28), transition game state to waves (1) if on start, start (0) if on end
*/
void menuScreen(BOOT_KBD_REPORT* kbdbuf) {
    static int enter_released = 1; 

    if (gameState == 0) {

        int enter_pressed = 0;
        for (int i = 0; i < 6; i++) {
            if (kbdbuf->keycode[i] == 0x28) { // Enter key
                enter_pressed = 1;
                break;
            }
        }

        if (enter_pressed) {
            if (enter_released) {
                gameState = 1; 
                enter_released = 0;
                spawn_timer = 60; 

                for (int i = 212; i <= 511; i++) {
                    Xil_Out32(HDMI_BASE + (i * 4), -1);
                }

                player_health = 15;
                player_score = 0;
                humansSaved = 0;
                current_wave = 1;

                sprite_x = 149;
                sprite_y = 113;

                initWave(current_wave);

                sprite_x = 400;
                sprite_y = 400;
            }
        } else {
            enter_released = 1; 
        }
    }
    else if (gameState == 5) {
        int enter_pressed = 0;
        for (int i = 0; i < 6; i++) {
            if (kbdbuf->keycode[i] == 0x28) {
                enter_pressed = 1;
                break;
            }
        }

        if (enter_pressed) {
            if (enter_released) {
                gameState = 0; 
                enter_released = 0;

                system_init();

				drawStartScreen();
           }
        } else {
            enter_released = 1; 
        }
    }

    Xil_Out32(GAME_STATE_REG, gameState); // allow display fsm to read
}

/*
 * system_init()
 * Clears all game state, enemy arrays, bullet arrays, hardware registers,
 * and resets global variables to their default values.
 * Must be called once at program start.
 */
void system_init() {
    int i;
    // reset enemy arrays
    for (i = 0; i < NUM_ENEMIES; i++) {
        enemy_x[i] = -1;
        enemy_y[i] = -1;
        enemy_base_sprite[i] = -1;
        enemy_type[i] = -1;
        enemy_state1[i] = 0;
        enemy_state2[i] = 0;
        // clear hardware sprite and coordinate registers for this enemy
        Xil_Out32(ENEMY_SPRITE_REG(i), -1);
        Xil_Out32(ENEMY_COORD_REG(i), 0);
    }

    // reset enemy bullet arrays
	for (i = 0; i < NUM_ENEMY_BULLETS; i++) {
		e_bulletX[i] = 0;
		e_bulletY[i] = 0;
		e_bvelocityX[i] = 0;
		e_bvelocityY[i] = 0;
		e_bID[i] = -1;      

		Xil_Out32(HDMI_BASE + ((150 + (i*3)) * 4), 0);    
		Xil_Out32(HDMI_BASE + ((150 + (i*3) + 1) * 4), 0);  
		Xil_Out32(HDMI_BASE + ((150 + (i*3) + 2) * 4), -1); 
	}

    // reset global game vars
	player_score = 0;
	player_health = 3;
	humansSaved = 0;
	player_iframes = 0;
	spawn_timer = 0;
	new_life = 25000;

	for (i = 0; i < NUM_BULLETS; i++) {
		bullet_active[i] = 0;
		bullet_x[i] = 0;
		bullet_y[i] = 0;
		bullet_dir[i] = 0;
		Xil_Out32(BULLET_X_ADDR(i), 0);
		Xil_Out32(BULLET_Y_ADDR(i), 0);
		Xil_Out32(BULLET_STATUS_ADDR(i), 0);
	}
	fire_cooldown = 0;


    // reset player positions/animations
    Xil_Out32(SPRITE_X_ADDR, 149);      
    Xil_Out32(SPRITE_Y_ADDR, 113);      
    Xil_Out32(PLAYER_DIR_ADDR, get_player_sprite_addr(4, 0, 0)); 
    Xil_Out32(GAME_STATE_REG, 0);   

    // clear bullets
    for (i = 0; i < NUM_BULLETS; i++) {
        // might not need since these are cleared anyways
        Xil_Out32(BULLET_X_ADDR(i), 0);
        Xil_Out32(BULLET_Y_ADDR(i), 0);
        Xil_Out32(BULLET_STATUS_ADDR(i), 0);
    }

	// clear static display sprites
	for (int i = 212; i <= 511; i++) {
		Xil_Out32(HDMI_BASE + (i * 4), -1);
	}

	drawStartScreen();

    // reset fsm
    gameState = 0;   // Start screen
    update_hud(player_score, player_health, current_wave);
}

void clear_all_bullets() {
    // Clear Enemy Bullets
    for (int i = 0; i < NUM_ENEMY_BULLETS; i++) {
        e_bulletX[i] = 0;
        e_bulletY[i] = 0;
        e_bvelocityX[i] = 0;
        e_bvelocityY[i] = 0;
        e_bID[i] = -1; 
        e_bLife[i] = 0;

        Xil_Out32(HDMI_BASE + ((150 + (i*3)) * 4), 0);    
        Xil_Out32(HDMI_BASE + ((150 + (i*3) + 1) * 4), 0); 
        Xil_Out32(HDMI_BASE + ((150 + (i*3) + 2) * 4), -1); 
    }

    // Clear Player Bullets
    for (int i = 0; i < NUM_BULLETS; i++) {
        bullet_active[i] = 0;
        bullet_x[i] = 0;
        bullet_y[i] = 0;
        bullet_dir[i] = 0;

        Xil_Out32(BULLET_X_ADDR(i), 0);
        Xil_Out32(BULLET_Y_ADDR(i), 0);
        Xil_Out32(BULLET_STATUS_ADDR(i), 0);
    }
}

/*
check if wave is finished (only hulks left)
return 1 if wave done, 0 if not
*/
int waveDone() {
	for (int i = 0; i < NUM_ENEMIES; i++) {
		if (enemy_type[i] != TYPE_HULK && enemy_type[i] != TYPE_PROG && enemy_base_sprite[i] != -1)
			return 0; 
	}
	return 1;
}

/*
prints a 2048 at every 32 bits starting from 0 on the 320x240 screen, 10x7 sprites of size 16x16, 1 at a time
start xil_out32 at DISPLAY_REGS_START
call for 70 times in a row
*/
/*
 * Plays the 2084 wipe animation.
 * Returns 0 if still animating, or 1 if the animation is completely finished.
 */
int stageTransition() {
    // persists between function calls
    static int spriteIndex = 0;

    // calculate sprite position
    int xCoord = (spriteIndex % 10) * 32;
    int yCoord = (spriteIndex / 10) * 32;

    // pack x into [31:16], y into [15:0]
    uint32_t combined = ((uint32_t)(uint16_t)xCoord << 16) | (uint16_t)yCoord;

    /*
    Each sprite uses:
      reg 0 = XY
      reg 1 = sprite address

    2 registers * 4 bytes = 8 bytes per sprite
    */
    int regOffset = spriteIndex * 8;

    // write XY register
    Xil_Out32(DISPLAY_REGS_START + regOffset, combined);

    // write sprite address register
    Xil_Out32(DISPLAY_REGS_START + regOffset + 4, ADDR_2084);

    // next sprite next call
    spriteIndex++;

    // Check if the animation is done
    if (spriteIndex >= 70) {
        spriteIndex = 1; 
        return 1;      
    }

    return 0; // still animating
}

void spawnScore(int x, int y, int sprite_id) { // spawns score when save human
    for (int i = 0; i < NUM_ENEMIES; i++) {
        if (enemy_base_sprite[i] == -1) { 
            enemy_x[i] = x;
            enemy_y[i] = y;
            enemy_base_sprite[i] = sprite_id;
            enemy_type[i] = TYPE_SCORE;
            enemy_state1[i] = 60; 
            break;
        }
    }
}

int scoreSprite(int* eX, int* eY, int* timer, int eID) {
    (*timer)--;

    if (*timer % 4 == 0) {
        (*eY)--; //float upwards effect
    }

    if (*timer <= 0) {
        *eX = -100;
        *eY = -100;
        return -1; 
    }
    
    return eID;
}
